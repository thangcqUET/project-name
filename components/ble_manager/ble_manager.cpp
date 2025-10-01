#include "ble_manager.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino.h>
#include <memory>

#define SERVICE_UUID        "000000ff-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "0000ff01-0000-1000-8000-00805f9b34fb"

struct BLEManager::Impl {
  std::function<void()> onConnect;
  std::function<void()> onDisconnect;
  std::function<void(const std::vector<uint8_t>&)> onWrite;
  BLEServer* server = nullptr;
  BLECharacteristic* characteristic = nullptr;
};

class ServerCb : public BLEServerCallbacks {
  BLEManager::Impl* i_;
public:
  ServerCb(BLEManager::Impl* i): i_(i) {}
  void onConnect(BLEServer* s) override { if (i_->onConnect) i_->onConnect(); }
  void onDisconnect(BLEServer* s) override { if (i_->onDisconnect) i_->onDisconnect(); }
  void onMtuChanged(BLEServer* s, esp_ble_gatts_cb_param_t* p) override {
    // optional: forward or log if needed
  }
};

class CharCb : public BLECharacteristicCallbacks {
  BLEManager::Impl* i_;
public:
  CharCb(BLEManager::Impl* i): i_(i) {}
  void onWrite(BLECharacteristic* p) override {
    Serial.print("Received write request size: ");
    Serial.println(p->getLength());
    String v = p->getValue();
    size_t len = v.length();

    std::vector<uint8_t> data(len);
    memcpy(data.data(), v.c_str(), len);
    //log data size
    Serial.print("Data size: ");
    Serial.println(data.size());
    if (i_->onWrite) i_->onWrite(data);
  }
};

BLEManager& BLEManager::instance() {
  static BLEManager m;
  return m;
}

BLEManager::BLEManager(): impl_(new Impl()) {}
BLEManager::~BLEManager(){ stop(); delete impl_; }

void BLEManager::init(const char* device_name) {
  BLEDevice::init(device_name);
  BLEDevice::setMTU(512);

  impl_->server = BLEDevice::createServer();
  impl_->server->setCallbacks(new ServerCb(impl_));

  BLEService *pService = impl_->server->createService(SERVICE_UUID);
  impl_->characteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  impl_->characteristic->setCallbacks(new CharCb(impl_));
  impl_->characteristic->setValue("Hello");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID("000000ff-0000-1000-8000-00805f9b34fb");
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x40);
}

void BLEManager::start() {
  BLEDevice::startAdvertising();
}

void BLEManager::stop() {
  if (impl_->server) {
    BLEDevice::deinit(true);
    impl_->server = nullptr;
    impl_->characteristic = nullptr;
  }
}

void BLEManager::setOnConnect(std::function<void()> cb) { impl_->onConnect = std::move(cb); }
void BLEManager::setOnDisconnect(std::function<void()> cb) { impl_->onDisconnect = std::move(cb); }
void BLEManager::setOnWrite(std::function<void(const std::vector<uint8_t>&)> cb) { impl_->onWrite = std::move(cb); }