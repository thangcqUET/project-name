#include "Arduino.h"
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "epaper_interface.h"
// E-paper display pin configuration
#define CS_PIN (5)      // Chip Select - có thể thay đổi
#define BUSY_PIN (4)    // Busy status - có thể thay đổi  
#define RES_PIN (21)    // Reset - có thể thay đổi
#define DC_PIN (15)     // Data/Command - có thể thay đổi
// SPI pins (khuyến nghị giữ nguyên cho hiệu suất tốt nhất):
// SCK -> GPIO18 (hardware SPI clock)
// SDI/MOSI -> GPIO23 (hardware SPI data)
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(/*CS=5*/ CS_PIN, /*DC=*/ DC_PIN, /*RES=*/ RES_PIN, /*BUSY=*/ BUSY_PIN)); // 400x300, SSD1683

extern "C" void epaper_draw_image(const uint8_t* image_data, uint16_t image_size) {
  if (!Serial) return;
  display.init(115200, true, 2, false);
  // display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  display.fillScreen(GxEPD_BLACK);
  display.drawXBitmap(0, 0, image_data, 400, 300, GxEPD_WHITE);
  display.nextPage();
}