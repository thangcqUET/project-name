// #include "Arduino.h"
// #include <GxEPD2_BW.h>
// extern "C" void ble_app_start(void);

// #include <BLEDevice.h>
// #include <BLEUtils.h>
// #include <BLEServer.h>
// #include "lvgl_epaper.h"
// #include "esp_pm.h"
// #include "esp_sleep.h" 
// #include "esp_bt.h"
// #include "esp_err.h"
// #include "driver/gpio.h"
// #include "esp_system.h"
// #include "ble_manager.h"
// #include "esp_mac.h"
// #include "lvgl.h"
// #include "myfont.h"

// #define DEVICE_NAME "E-Frame"
// #define WAKEUP_GPIO GPIO_NUM_0  // GPIO0 button for wakeup
// #define CONNECTION_TIMEOUT_MS (5 * 60 * 1000)  // 5 minutes timeout


// // E-paper display pin configuration (same as epaper_interface)
// #define CS_PIN (10)
// #define BUSY_PIN (5)
// #define RES_PIN (9)
// #define DC_PIN (2)
// #define MOSI_PIN (6)
// #define SCK_PIN (4)

// // Instantiate local GxEPD2 display object
// GxEPD2_BW<GxEPD2_154_GDEY0154D67, GxEPD2_154_GDEY0154D67::HEIGHT> display(GxEPD2_154_GDEY0154D67(CS_PIN, DC_PIN, RES_PIN, BUSY_PIN));

// // Global variables for connection timeout
// unsigned long lastActivityTime = 0;
// bool isConnected = false;
// static const int DISP_W = 200;
// static const int DISP_H = 200;
// static const int BYTES_PER_LINE = (DISP_W + 7) / 8;
// static const int FULL_BUF_SIZE = BYTES_PER_LINE * DISP_H;
// // Function to enter deep sleep
// void enterDeepSleep(const char* reason) {
//   Serial.print("Entering deep sleep: ");
//   Serial.println(reason);
//   Serial.println("Press GPIO0 button to wake up");
//   Serial.flush();
  
//   // Clean shutdown BLE if still active
//   BLEManager::instance().stop();
//   delay(100);
  
//   esp_deep_sleep_start();
// }
// uint32_t my_get_millis(void)
// {
//     return millis(); // Trả về thời gian thực thay vì giá trị cố định
// }

// /* Copy rendered image to screen.
//  * This needs to be implemented by the user. */
// void my_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_buf)
// {
//     ESP_LOGI("lvgl_epaper", "Flush callback called: area (%d,%d)-(%d,%d)", 
//              (int)area->x1, (int)area->y1, (int)area->x2, (int)area->y2);
    
//     // Calculate area dimensions
//     int w = area->x2 - area->x1 + 1;
//     int h = area->y2 - area->y1 + 1;
    
//     // LVGL sends data in its own format, we need to convert to 1-bit XBM format for GxEPD2
//     // Allocate temporary buffer for converted data
//     int bytes_per_line = (w + 7) / 8;
//     int buf_size = bytes_per_line * h;
//     uint8_t* mono_buf = (uint8_t*)malloc(buf_size);
    
//     if (!mono_buf) {
//         ESP_LOGE("lvgl_epaper", "Failed to allocate conversion buffer");
//         lv_display_flush_ready(disp);
//         return;
//     }
    
//     // Initialize buffer to white (0x00 in XBM format cho GxEPD_BLACK foreground) 
//     memset(mono_buf, 0x00, buf_size);
    
//     // Convert LVGL pixel data to 1-bit XBM format
//     for (int y = 0; y < h; y++) {
//         for (int x = 0; x < w; x++) {
//             // LVGL pixel value (assuming 8-bit grayscale or similar)
//             uint8_t pixel = px_buf[y * w + x];
            
//             // Với GxEPD_BLACK foreground: pixel tối (text/content) cần set bit 1
//             bool is_black = (pixel < 128);
            
//             if (is_black) {
//                 // Set bit to 1 for black content với GxEPD_BLACK foreground
//                 int byte_idx = y * bytes_per_line + (x / 8);
//                 int bit_idx = x % 8;  // LSB first
//                 mono_buf[byte_idx] |= (1 << bit_idx);
//             }
//             // Background pixels remain 0
//         }
//     }
    
//     // Draw to e-paper using XBM format
//     // Sử dụng GxEPD_BLACK để vẽ nội dung đen trên nền trắng
//     display.setPartialWindow(area->x1, area->y1, w, h);
//     display.firstPage();
//     do {
//         display.drawXBitmap(area->x1, area->y1, mono_buf, w, h, GxEPD_BLACK);
//     } while (display.nextPage());
    
//     // Clean up
//     free(mono_buf);
    
//     // Tell LVGL we're done
//     lv_display_flush_ready(disp);
// }



// extern "C" void app_main(){
//   initArduino();
//   Serial.begin(115200);
  
  
//   // Initialize GPIO pins for e-paper display
//   pinMode(10, OUTPUT);  // Set GPIO 10 as output
//   // Add other GPIO pins if needed for your e-paper display
//   pinMode(5, OUTPUT);  // Example: Reset pin
//   pinMode(9, OUTPUT);  // Example: DC pin
//   pinMode(2, OUTPUT); // Example: CS pin



//     lv_init();

//     lv_tick_set_cb(my_get_millis);
//     display.init(115200, true, 2, false);
//     display.setFullWindow();
//     display.firstPage();
//     // Thử clear màn hình trước khi fill
//     display.clearScreen();
//     display.fillScreen(GxEPD_WHITE);
//     display.nextPage();
//     lv_display_t * lv_display = lv_display_create(200, 200);
    
//     // Set color format to grayscale for easier conversion
//     lv_display_set_color_format(lv_display, LV_COLOR_FORMAT_L8);

//     /* LVGL buffer - use smaller partial buffer to save memory */
//     static uint8_t buf[200 * 200];  // 200 lines of 200 pixels, 1 byte per pixel for L8
//     lv_display_set_buffers(lv_display, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);

//     /* This callback will display the rendered image */
//     lv_display_set_flush_cb(lv_display, my_flush_cb);

//     // Đặt nền của screen thành trắng
//     lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);
//     lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

//     /* Test với hiển thị đơn giản */
//     lv_obj_t * label = lv_label_create(lv_screen_active());
//     lv_label_set_text(label, "Tiếng Việt: áàả\n"
//         "ãạăắằẳẵặâ\n"
//         "ấầẩẫậđéèẻẽẹêế\n"
//         "ềểễệíìỉĩịóòỏ\n"
//         "õọôốồổỗộơớờở\n"
//         "ỡợúùủũụưứừửữự\n"
//         "ýỳỷỹỵ");
//     // Center the label  
//     lv_obj_center(label);
    
//     // Đặt màu text thành đen trên nền trắng
//     lv_obj_set_style_text_color(label, lv_color_black(), 0);

//     // Nếu bạn đã sinh font hỗ trợ tiếng Việt (myfont.c), áp dụng nó
// #if MYFONT_PROVIDED
//     lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
//     lv_obj_set_style_text_font(label, &MYFONT_SYMBOL, 0);
// #endif
    
//     // Test với một hình chữ nhật đơn giản
//     lv_obj_t * rect = lv_obj_create(lv_screen_active());
//     lv_obj_set_size(rect, 100, 50);
//     lv_obj_set_pos(rect, 50, 10);
//     lv_obj_set_style_bg_color(rect, lv_color_black(), 0);
//     lv_obj_set_style_border_width(rect, 0, 0);

//     /* Make LVGL periodically execute its tasks */
//     while(1) {
//         /* Provide updates to currently-displayed Widgets here. */
//         lv_timer_handler();
//         vTaskDelay(pdMS_TO_TICKS(20));  /* Increase delay to reduce CPU usage */
//     }
// }



