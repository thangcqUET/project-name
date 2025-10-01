idf_component_register(
  SRCS "ble_manager.cpp"
  INCLUDE_DIRS "."   # hoặc "include" nếu dùng thư mục include con
  REQUIRES espressif__arduino-esp32
)