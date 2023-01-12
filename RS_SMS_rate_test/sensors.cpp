#include "sensors.hpp"

float anemometer_meas() {

  float windspeed = 0;
  float ws_km = 0;
  unsigned long start_t;
  int cnt = 0;
  int state = 0;
  int itr = 0;

   while(itr < 500){
    float sensorValue = analogRead(ANEMOMETER_PIN);
    float voltage = (sensorValue / 1023) * Vref;

    if ((voltage > (Vref * 0.5)) && state == 0) {
      cnt++;
      state = 1;

    }
    if ((voltage < (Vref * 0.5)) && state == 1) {
      state = 0;
    }
  delay(10);
  itr ++;
  }
  windspeed = cnt / 10.0; //2*5 sendo 2 o numero de ticks por volta completa e 5 o periodo de medição
  ws_km = windspeed * kmh_factor;

  return ws_km;
}
