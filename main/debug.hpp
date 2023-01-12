#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define DEBUG_SETUPSERIAL(baudrate) Serial.begin(baudrate)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_SETUPSERIAL(baudrate)
#define DEBUG_PRINTLN(...)
#define DEBUG_PRINT(...)
#define DEBUG_PRINTF(...)
#endif

#endif