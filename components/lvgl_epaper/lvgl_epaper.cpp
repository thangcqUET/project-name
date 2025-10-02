#include "lvgl_epaper.h"
#include <GxEPD2_BW.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <cstring>

// E-paper display pin configuration (same as epaper_interface)
#define CS_PIN (10)
#define BUSY_PIN (5)
#define RES_PIN (9)
#define DC_PIN (2)
#define MOSI_PIN (6)
#define SCK_PIN (4)

// Instantiate local GxEPD2 display object
GxEPD2_BW<GxEPD2_154_GDEY0154D67, GxEPD2_154_GDEY0154D67::HEIGHT> display(GxEPD2_154_GDEY0154D67(CS_PIN, DC_PIN, RES_PIN, BUSY_PIN));

static lv_display_t* disp = nullptr; // LVGL display handle saved so non-LVGL callers can request a flush

// Persistent full-frame monochrome buffer for the display
static const int DISP_W = 200;
static const int DISP_H = 200;
static const int BYTES_PER_LINE = (DISP_W + 7) / 8;
static const int FULL_BUF_SIZE = BYTES_PER_LINE * DISP_H;
static uint8_t* full_mono = nullptr;

static void ensure_full_buffer() {
  if (!full_mono) {
    full_mono = (uint8_t*)heap_caps_malloc(FULL_BUF_SIZE, MALLOC_CAP_8BIT);
    if (full_mono) memset(full_mono, 0xFF, FULL_BUF_SIZE); // white
  }
}


// LVGL v9 flush callback signature: px_map is the rendered pixel map in the display's color format.
static void lvgl_epaper_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_buf) {
  ESP_LOGI("lvgl_epaper", "Flush callback called: area (%d,%d)-(%d,%d)", 
             (int)area->x1, (int)area->y1, (int)area->x2, (int)area->y2);
    
    // Calculate area dimensions
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;
    
    // LVGL sends data in its own format, we need to convert to 1-bit XBM format for GxEPD2
    // Allocate temporary buffer for converted data
    int bytes_per_line = (w + 7) / 8;
    int buf_size = bytes_per_line * h;
    uint8_t* mono_buf = (uint8_t*)malloc(buf_size);
    
    if (!mono_buf) {
        ESP_LOGE("lvgl_epaper", "Failed to allocate conversion buffer");
        lv_display_flush_ready(disp);
        return;
    }
    
    // Initialize buffer to white (0x00 in XBM format cho GxEPD_BLACK foreground) 
    memset(mono_buf, 0x00, buf_size);
    ESP_LOGI("lvgl_epaper", "Conversion buffer size: %d bytes", buf_size);

    // Convert LVGL pixel data to 1-bit XBM format
    int black_count = 0;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // LVGL pixel value (assuming 8-bit grayscale or similar)
            uint8_t pixel = px_buf[y * w + x];
            
            // Với GxEPD_BLACK foreground: pixel tối (text/content) cần set bit 1
            bool is_black = (pixel < 128);
            
            if (is_black) {
                black_count++;
                // Set bit to 1 for black content với GxEPD_BLACK foreground
                int byte_idx = y * bytes_per_line + (x / 8);
                int bit_idx = x % 8;  // LSB first
                mono_buf[byte_idx] |= (1 << bit_idx);
            }
            // Background pixels remain 0
        }
    }
    
    ESP_LOGI("lvgl_epaper", "Flush converting L8->XBM: %d black pixels in area", black_count);
    
    // Draw to e-paper using XBM format
    // Sử dụng GxEPD_BLACK để vẽ nội dung đen trên nền trắng
    display.setPartialWindow(area->x1, area->y1, w, h);
    display.firstPage();
    do {
        display.drawXBitmap(area->x1, area->y1, mono_buf, w, h, GxEPD_BLACK);
    } while (display.nextPage());
    
    // Clean up
    free(mono_buf);
    
    // Tell LVGL we're done
    lv_display_flush_ready(disp);
}

uint32_t get_millis() {
    return esp_timer_get_time()/1000;
}
bool static is_init = false;
void lvgl_epaper_init(void) {
    if (is_init) return;  // Already initialized
    //log
    ESP_LOGI("lvgl_epaper", "Initializing LVGL e-paper interface");
    lv_init();
    lv_tick_set_cb(get_millis);
    display.init(115200, true, 2, false);
    // Không clear/fill screen để giữ hình cũ khi reboot/kết nối lại
    // display.setFullWindow();
    // display.firstPage();
    // display.clearScreen();
    // display.fillScreen(GxEPD_WHITE);
    // display.nextPage();
    
    lv_display_t * lv_display = lv_display_create(200, 200);
    
    // Set color format to grayscale for easier conversion
    lv_display_set_color_format(lv_display, LV_COLOR_FORMAT_L8);

    /* LVGL buffer - use smaller partial buffer to save memory */
    static uint8_t buf[200 * 200];  // 200 lines of 200 pixels, 1 byte per pixel for L8
    lv_display_set_buffers(lv_display, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* This callback will display the rendered image */
    lv_display_set_flush_cb(lv_display, lvgl_epaper_flush_cb);

    // Đặt nền của screen thành trắng
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);
    is_init = true;
}


// Draw an LVGL image descriptor directly to the e-paper display.
// XBM data received from BLE: 1-bit per pixel, packed, LSB-first
// We convert to LVGL L8 (8-bit grayscale) for easier rendering
bool lvgl_epaper_draw_lv_image(const uint8_t* buf, size_t buf_size) {
  ESP_LOGI("lvgl_epaper", "Drawing image: size=%d bytes, expected=%d", buf_size, FULL_BUF_SIZE);

  // Allocate L8 buffer for LVGL (200x200 = 40000 bytes)
  static uint8_t* l8_buf = nullptr;
  if (!l8_buf) {
    l8_buf = (uint8_t*)heap_caps_malloc(DISP_W * DISP_H, MALLOC_CAP_8BIT);
    if (!l8_buf) {
      ESP_LOGE("lvgl_epaper", "Failed to allocate L8 buffer");
      return false;
    }
  }

  // Convert XBM (1-bit) to L8 (8-bit grayscale)
  // Đảo ngược: bit 1 = trắng, bit 0 = đen
  memset(l8_buf, 0, DISP_W * DISP_H); // default black
  
  int white_pixels = 0;
  for (int y = 0; y < DISP_H; y++) {
    for (int x = 0; x < DISP_W; x++) {
      int byte_idx = y * BYTES_PER_LINE + (x / 8);
      int bit_idx = x % 8; // LSB first
      
      if (byte_idx < buf_size) {
        bool is_white = (buf[byte_idx] & (1 << bit_idx)) != 0; // bit 1 = white
        if (is_white) {
          l8_buf[y * DISP_W + x] = 255; // 255=white in L8
          white_pixels++;
        } else {
          l8_buf[y * DISP_W + x] = 0; // 0=black in L8
        }
      }
    }
  }
  
  ESP_LOGI("lvgl_epaper", "Converted XBM to L8: %d white pixels, %d black pixels", white_pixels, DISP_W * DISP_H - white_pixels);

  // Use canvas to draw pixels directly (more reliable than lv_image for raw data)
  static lv_obj_t* canvas = nullptr;
  static lv_draw_buf_t draw_buf;
  
  // Always delete old canvas and create new one to ensure clean state
  if (canvas != nullptr) {
    lv_obj_del(canvas);
    canvas = nullptr;
    ESP_LOGI("lvgl_epaper", "Deleted old canvas");
  }
  
  // Create new canvas
  canvas = lv_canvas_create(lv_screen_active());
  lv_obj_set_pos(canvas, 0, 0);
  
  // Initialize canvas buffer with our L8 data
  lv_draw_buf_init(&draw_buf, DISP_W, DISP_H, LV_COLOR_FORMAT_L8, 0, l8_buf, DISP_W * DISP_H);
  lv_canvas_set_draw_buf(canvas, &draw_buf);

  ESP_LOGI("lvgl_epaper", "Canvas created with new image data");

  // Invalidate to mark for render - LVGL main loop will handle it
  lv_obj_invalidate(canvas);
  
  ESP_LOGI("lvgl_epaper", "Image queued for render (will render in main loop)");
  return true;
}
