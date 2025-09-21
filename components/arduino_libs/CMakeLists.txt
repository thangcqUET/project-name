idf_component_register(
    SRCS "Adafruit_GFX/Adafruit_GFX.cpp" "Adafruit_GFX/Adafruit_GrayOLED.cpp" "Adafruit_GFX/Adafruit_SPITFT.cpp" "Adafruit_GFX/glcdfont.c" "GxEPD2/src/GxEPD2_EPD.cpp" "GxEPD2/src/gdey/GxEPD2_420_GDEY042T81.cpp" "GxEPD2/src/gdey/GxEPD2_154_GDEY0154D67.cpp"
  INCLUDE_DIRS "GxEPD2/src" "GxEPD2/src/gdey" "Adafruit_GFX" "Adafruit_BusIO"
  REQUIRES espressif__arduino-esp32
)