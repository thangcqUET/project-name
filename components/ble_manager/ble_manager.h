#pragma once
#include <functional>
#include <vector>
#include <stdint.h>

class BLEManager {
public:
  static BLEManager& instance();

  // Initialize BLE stack and GATT (non-blocking)
  void init(const char* device_name);

  // Start/stop advertising (optional)
  void start();
  void stop();

  // Callbacks for app logic
  void setOnConnect(std::function<void()> cb);
  void setOnDisconnect(std::function<void()> cb);
  void setOnWrite(std::function<void(const std::vector<uint8_t>&)> cb);

  struct Impl;
private:
  BLEManager();
  ~BLEManager();
  BLEManager(const BLEManager&) = delete;
  BLEManager& operator=(const BLEManager&) = delete;


  Impl* impl_;
};