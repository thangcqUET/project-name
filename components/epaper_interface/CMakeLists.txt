idf_component_register(
  SRCS "epaper_interface.cpp"
  INCLUDE_DIRS "."   # hoặc "include" nếu dùng thư mục include con
  REQUIRES espressif__arduino-esp32 arduino_libs
)