#include "Arduino.h"
extern "C" void ble_app_start(void);

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "lvgl_epaper.h"
#include "esp_pm.h"
#include "esp_sleep.h" 
#include "esp_bt.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "ble_manager.h"
#include "esp_mac.h"

#define DEVICE_NAME "E-Frame"
#define WAKEUP_GPIO GPIO_NUM_0  // GPIO0 button for wakeup
#define CONNECTION_TIMEOUT_MS (5 * 60 * 1000)  // 5 minutes timeout

// Global variables for connection timeout
unsigned long lastActivityTime = 0;
bool isConnected = false;

// Function to enter deep sleep
void enterDeepSleep(const char* reason) {
  Serial.print("Entering deep sleep: ");
  Serial.println(reason);
  Serial.println("Press GPIO0 button to wake up");
  Serial.flush();
  
  // Clean shutdown BLE if still active
  BLEManager::instance().stop();
  delay(100);
  
  esp_deep_sleep_start();
}

extern "C" void app_main(){
  initArduino();
  Serial.begin(115200);
  
  // Check wakeup reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup from GPIO0 button press");
      break;
    case ESP_SLEEP_WAKEUP_UNDEFINED:
    default:
      Serial.println("Cold boot or reset");
      break;
  }
  
  Serial.println("Starting BLE E-Frame with power management!");

  // Configure GPIO0 as wakeup source (button with pull-up)
  gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << WAKEUP_GPIO),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&io_conf);
  
  // Configure wakeup on GPIO0 (low level = button pressed) - ESP32-C3 uses gpio_wakeup
  esp_err_t wakeup_err = esp_sleep_enable_gpio_wakeup();
  if (wakeup_err != ESP_OK) {
    Serial.print("GPIO wakeup enable failed: ");
    Serial.println(esp_err_to_name(wakeup_err));
  } else {
    // Configure GPIO0 for wakeup on low level (button pressed)
    gpio_wakeup_enable(WAKEUP_GPIO, GPIO_INTR_LOW_LEVEL);
    Serial.println("GPIO0 wakeup configured (press button to wake from deep sleep)");
  }

  // Configure power management for light sleep with BLE
  esp_pm_config_esp32c3_t pm_config = {
    .max_freq_mhz = 160,          // Max CPU frequency
    .min_freq_mhz = 10,           // Min CPU frequency (light sleep) 
    .light_sleep_enable = true    // Enable automatic light sleep
  };
  
  esp_err_t pm_err = esp_pm_configure(&pm_config);
  if (pm_err != ESP_OK) {
    Serial.print("PM configure failed: ");
    Serial.println(esp_err_to_name(pm_err));
  } else {
    Serial.println("Power management configured successfully");
  }
  
  Serial.println("Light sleep with BLE enabled - device will auto-sleep when idle");

  // Initialize GPIO pins for e-paper display
  pinMode(10, OUTPUT);  // Set GPIO 10 as output
  // Add other GPIO pins if needed for your e-paper display
  pinMode(5, OUTPUT);  // Example: Reset pin
  pinMode(9, OUTPUT);  // Example: DC pin
  pinMode(2, OUTPUT); // Example: CS pin

  // Initialize connection timeout
  lastActivityTime = millis();
  isConnected = false;
  
  // Enable BLE modem sleep after BLE initialization
  esp_err_t bt_err = esp_bt_sleep_enable();
  if (bt_err != ESP_OK) {
    Serial.print("BLE sleep enable failed: ");
    Serial.println(esp_err_to_name(bt_err));
  } else {
    Serial.println("BLE modem sleep enabled successfully");
  }

  std::vector<uint8_t> imageData;
  esp_pm_lock_handle_t pm_lock = NULL;

  //get mac address
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);  // fills the same 6-byte base MAC
  std::string specific_name = "E-Frame-" + std::to_string((mac[3] << 16) | (mac[4] << 8) | mac[5]);
  BLEManager::instance().init(specific_name.c_str());
  BLEManager::instance().setOnConnect([&pm_lock]() {
    Serial.println("BLE connected (app callback)");
    isConnected = true;
    // Keep device active during data transfer
    if (pm_lock == NULL) {
      esp_err_t err = esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "ble_transfer", &pm_lock);
      if (err == ESP_OK) {
        esp_pm_lock_acquire(pm_lock);
        Serial.println("PM lock acquired - device stays active");
      } else {
        Serial.print("PM lock create failed: ");
        Serial.println(esp_err_to_name(err));
      }
    }
    
  });

  BLEManager::instance().setOnDisconnect([&pm_lock]() {
    Serial.println("BLE disconnected (app callback)");
    isConnected = false;
    lastActivityTime = millis();
    // release PM lock as before...
    // Release PM lock first
    if (pm_lock != NULL) {
      esp_pm_lock_release(pm_lock);
      esp_pm_lock_delete(pm_lock);
      pm_lock = NULL;
      Serial.println("PM lock released");
    }

    BLEManager::instance().start(); // resume advertising
  });

  BLEManager::instance().setOnWrite([&imageData](const std::vector<uint8_t>& data){
    Serial.print("Received data chunk of size: ");
    Serial.println(data.size());
    // Check for end marker [0xFF, 0xFF, 0xFF, 0xFF]
    if (data.size() == 4 && 
        (uint8_t)data[0] == 0xFF && 
        (uint8_t)data[1] == 0xFF && 
        (uint8_t)data[2] == 0xFF && 
        (uint8_t)data[3] == 0xFF) {

      // remove data to 200x200 bits
      if (imageData.size() > 5000) {
        imageData.erase(imageData.begin() + 5000, imageData.end());
        Serial.println("Image data truncated to 5000 bytes for 200x200 display");
      }
      Serial.println("*** END MARKER RECEIVED ***");
      Serial.print("Image transfer complete! Total size: ");
      Serial.print(imageData.size());
      Serial.println(" bytes");
      
      // Here you can process the complete image data
      // For example: save to file, display on e-paper, etc.
      // If LVGL bridge is enabled, push the raw 1-bit framebuffer directly
      bool ok = lvgl_epaper_draw_lv_image(imageData.data(), imageData.size());
      Serial.print("lvgl_epaper_draw_lv_image returned: "); Serial.println(ok);
      //clear the image data for next transfer
      imageData.clear();
      return; // Don't add end marker to image data
    }
    
    // Add each byte to the image data vector
    if (imageData.empty()) {
      imageData.reserve(5000); // Reserve cho ảnh 200x200
    }
    
    // Add bytes efficiently
    size_t oldSize = imageData.size();
    imageData.resize(oldSize + data.size());
    memcpy(imageData.data() + oldSize, data.data(), data.size());

    if (imageData.size() % 200 == 0 || imageData.size() > 4000) {
      Serial.print("Total: ");
      Serial.print(imageData.size());
      Serial.println(" bytes");
    }
  });

  BLEManager::instance().start();

  lvgl_epaper_init();
  
  
  // Main loop - allow system to enter light sleep automatically
  unsigned long lastLog = 0;
  for(;;) {
    delay(1000); // Check more frequently for timeout
    
    // Check for connection timeout (5 minutes)
    if (!isConnected && (millis() - lastActivityTime > CONNECTION_TIMEOUT_MS)) {
      enterDeepSleep("5 minute timeout - no connection");
    }
    
    // Log sleep status periodically (every 30 seconds)
    if (millis() - lastLog > 30000) {
      if (isConnected) {
        Serial.println("Device active - client connected");
      } else {
        unsigned long remainingTime = (CONNECTION_TIMEOUT_MS - (millis() - lastActivityTime)) / 1000;
        if (remainingTime > 0) {
          Serial.print("Device idle - sleeping in ");
          Serial.print(remainingTime);
          Serial.println(" seconds if no connection");
        }
      }
      lastLog = millis();
    }
    
    // System will automatically enter light sleep between delays
  }
}
