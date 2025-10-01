#include "lvgl_epaper.h"
#include "Arduino.h"
#include <GxEPD2_BW.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <cstring>
#include <algorithm>

// E-paper display pin configuration (same as epaper_interface)
#define CS_PIN (10)
#define BUSY_PIN (5)
#define RES_PIN (9)
#define DC_PIN (2)

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
static void lvgl_epaper_flush_cb(lv_display_t* disp_lv, const lv_area_t* area, uint8_t* px_map) {
  // LVGL was configured to render directly into our full_mono buffer in FULL mode,
  // so px_map should point to the image data. We ignore area and use full_mono.
  ensure_full_buffer();
  if (!full_mono) {
    lv_display_flush_ready(disp_lv);
    return;
  }
  //init the display if not already done
  display.init(115200, true, 2, false);
  // Push the full framebuffer to the e-paper using GxEPD2 directly
  display.setFullWindow();
  display.firstPage();
  display.fillScreen(GxEPD_BLACK);
  display.drawXBitmap(0, 0, full_mono, DISP_W, DISP_H, GxEPD_WHITE);
  display.nextPage();

  // Tell LVGL the flush is ready
  lv_display_flush_ready(disp_lv);
}

void lvgl_epaper_init(void) {
  lv_init();

  // Initialize resources
  ensure_full_buffer();

  // Initialize the e-paper display hardware (match epaper_interface)
  // Parameters: baudrate, debugPrint, spiCsPin, resetWithGpio
  // Using same init as epaper_interface to ensure the display is ready before any draw
  // Note: we initialize the hardware here, and non-LVGL callers will request the LVGL flush path.
  display.init(115200, true, 2, false);

  // Create an LVGL display instance and hand over our full-frame 1-bit buffer.
  lv_display_t* disp_lv = lv_display_create(DISP_W, DISP_H);
  if (!disp_lv) return;
  // keep handle for external triggers (draw_buffer/draw_lv_image)
  disp = disp_lv;

  // Tell LVGL the display uses 1-bit indexed format (I1)
  lv_display_set_color_format(disp_lv, LV_COLOR_FORMAT_I1);

  // Provide the full framebuffer to LVGL and use FULL render mode so LVGL writes
  // directly into our buffer.
  lv_display_set_buffers(disp_lv, full_mono, NULL, FULL_BUF_SIZE, LV_DISPLAY_RENDER_MODE_FULL);

  // Set flush callback to be called when LVGL wants to present the buffer.
  lv_display_set_flush_cb(disp_lv, lvgl_epaper_flush_cb);

  // Optionally set as default display
  lv_display_set_default(disp_lv);
}


// Draw an LVGL image descriptor directly to the e-paper display.
// `lv_img_dsc_t` is an LVGL v9 image descriptor; use void* here in the header to
// avoid direct dependency in C compile units. We include lvgl.h above so cast is OK.
extern "C" bool lvgl_epaper_draw_lv_image(const uint8_t* buf, size_t buf_size) {
  if (!buf || buf_size == 0) return false;
  ensure_full_buffer();
  if (!full_mono) return false;

  // Treat buf as an lv_img_dsc_t memory blob. Caller must ensure buf contains
  // a valid lv_img_dsc_t and sufficient image data following the header.
  const lv_img_dsc_t img = {
    .header = {
        .cf = LV_COLOR_FORMAT_I1, // default to 1-bit indexed
        .w = DISP_W,
        .h = DISP_H,
    },
    .data_size = buf_size,
    .data = buf,
  };

  // Basic size checks: only support images matching the display size for now.
  if (img.header.w != DISP_W || img.header.h != DISP_H) {
    // For simplicity we only accept full-screen images. Could implement centering/scaling later.
    return false;
  }

  // If image is already 1-bit indexed, copy directly.
  if (img.header.cf == LV_COLOR_FORMAT_I1) {
    // LVGL 1-bit indexed images store 1bpp data after the palette/header.
    // The image data pointer is img.data. We'll copy FULL_BUF_SIZE bytes.
    const uint8_t* src = (const uint8_t*)img.data;
    memcpy(full_mono, src, FULL_BUF_SIZE);
  } else{
    // Fallback: assume RGB565 (common LVGL true color). Convert to 1-bit with threshold.
    const uint8_t* src = (const uint8_t*)img.data;
    // LVGL RGB565 packs two bytes per pixel.
    int byte_idx = 0;
    for (int y = 0; y < DISP_H; ++y) {
      for (int x = 0; x < DISP_W; ++x) {
        // Read two bytes (big/little depends on LVGL config); treat as little-endian
        uint16_t px = src[byte_idx] | (src[byte_idx + 1] << 8);
        byte_idx += 2;
        // Convert RGB565 to brightness (approx): R5,G6,B5
        int r = (px >> 11) & 0x1F;
        int g = (px >> 5) & 0x3F;
        int b = px & 0x1F;
        // Expand to 8-bit ranges
        int R = (r * 255) / 31;
        int G = (g * 255) / 63;
        int B = (b * 255) / 31;
        int lum = (R * 30 + G * 59 + B * 11) / 100; // luma
        // threshold at mid-level
        bool black = lum < 128;
        int byte_pos = (y * BYTES_PER_LINE) + (x / 8);
        int bit = 7 - (x & 7);
        if (black) {
          full_mono[byte_pos] &= ~(1 << bit); // black pixel = 0 in our earlier logic
        } else {
          full_mono[byte_pos] |= (1 << bit);
        }
      }
    }
  }

  // Request LVGL flush path to present the converted image
  if (disp) {
    lvgl_epaper_flush_cb(disp, nullptr, full_mono);
  }

  return true;
}
