#include "Arduino.h"
extern "C" void ble_app_start(void);

extern "C" void app_main(){
  initArduino();
  ble_app_start();
  for(;;) delay(1000);
}
