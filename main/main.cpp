#include "Arduino.h"
extern "C" void ble_app_start(void);

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "epaper_interface.h"

#define SERVICE_UUID        "000000ff-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000ff01-0000-1000-8000-00805f9b34fb"
// #define DEVICE_NAME "Khung ảnh E-Frame"
#define DEVICE_NAME "Khung ảnh của Hà"

// Callback class to handle client writes
class MyCallbacks: public BLECharacteristicCallbacks {
  private:
    //keep image data in memory as an array of bytes
    std::vector<uint8_t> imageData;
    void onWrite(BLECharacteristic* pCharacteristic) {
      String value = pCharacteristic->getValue();

      // Serial.print("Received ");
      // Serial.print(value.length());
      // Serial.print(" bytes");
      
      // Check if this is a write without response by examining the characteristic properties
      uint32_t properties = pCharacteristic->getProperties();
      
      if (properties & BLECharacteristic::PROPERTY_WRITE_NR) {
        Serial.print(" (Write Without Response)");
      } else {
        Serial.print(" (Write With Response)");
      }
      Serial.println(":");
      
      // Check for end marker [0xFF, 0xFF, 0xFF, 0xFF]
      if (value.length() == 4 && 
          (uint8_t)value[0] == 0xFF && 
          (uint8_t)value[1] == 0xFF && 
          (uint8_t)value[2] == 0xFF && 
          (uint8_t)value[3] == 0xFF) {
        
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
        epaper_draw_image(imageData.data(), imageData.size());
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
      imageData.resize(oldSize + value.length());
      memcpy(imageData.data() + oldSize, value.c_str(), value.length());
      
      if (imageData.size() % 1000 == 0 || imageData.size() > 4000) {
        Serial.print("Total: ");
        Serial.print(imageData.size());
        Serial.println(" bytes");
      }
    }
    
  public:
    void processCompleteImage() {
      Serial.println("Processing complete image...");
      
      // Clear the image data for next transfer
      imageData.clear();
      Serial.println("Image data cleared, ready for next transfer");
      
      // Add your image processing code here:
      // - Save to SPIFFS/SD card
      // - Display on e-paper
      // - Send confirmation back to client
    }
};

// Server callback class to handle connect/disconnect events
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println("Client connected");
      // Print current MTU
      Serial.print("Current MTU: ");
      Serial.println(pServer->getPeerMTU(pServer->getConnId()));
    }

    void onDisconnect(BLEServer* pServer) {
      Serial.println("Client disconnected");
      Serial.println("Starting advertising again...");
      BLEDevice::startAdvertising();
    }
    
    void onMtuChanged(BLEServer* pServer, esp_ble_gatts_cb_param_t* param) {
      Serial.print("MTU negotiated to: ");
      Serial.println(param->mtu.mtu);
      Serial.print("Connection ID: ");
      Serial.println(param->mtu.conn_id);
    }
};

extern "C" void app_main(){
  initArduino();
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  // Initialize GPIO pins for e-paper display
  pinMode(10, OUTPUT);  // Set GPIO 10 as output
  // Add other GPIO pins if needed for your e-paper display
  pinMode(5, OUTPUT);  // Example: Reset pin
  pinMode(9, OUTPUT);  // Example: DC pin
  pinMode(2, OUTPUT); // Example: CS pin

  BLEDevice::init(DEVICE_NAME);

  BLEDevice::setMTU(512); // Set MTU to 512 bytes

  BLEServer *pServer = BLEDevice::createServer();
  
  // Set the server callback for connection events
  pServer->setCallbacks(new MyServerCallbacks());
  
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_WRITE_NR
                                       );

  // Set the callback for handling writes
  pCharacteristic->setCallbacks(new MyCallbacks());


  pCharacteristic->setValue("Hello");
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
  // ble_app_start();
  for(;;) delay(1000);
}
