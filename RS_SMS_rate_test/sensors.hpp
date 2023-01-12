#include <Adafruit_Sensor.h>
#include <DHT.h>

#define DHTPIN 14     // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

#define ANEMOMETER_PIN 15

const float kmh_factor = 2.401;
// time interval for measuring (ticks count) - 5s
const unsigned long interval = 5000UL;
// Reference voltage
const float Vref = 5.0;
/**
 * @brief extract wind speed based on the measuring time - interval
 * 
 */
float anemometer_meas();
