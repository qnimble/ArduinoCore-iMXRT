
#include <Arduino.h>

#ifdef ARDUINO_QUARTO
void serialEvent2() __attribute__((weak));
void serialEvent2() {}
#else
void serialEventUSB1() __attribute__((weak));
void serialEventUSB1() {}
#endif
uint8_t _serialEventUSB1_default PROGMEM = 1;	
