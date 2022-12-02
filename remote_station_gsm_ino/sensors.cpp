#include "sensors.hpp"
/*void setup() {
  Serial.begin(9600);
  Serial.println(F("DHTxx test!"));

  dht.begin();
  }

  void loop() {
  // Wait a few seconds between measurements.
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();


  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  }*/

float anemometer_meas() {

  float windspeed = 0;
  float ws_km = 0;
  unsigned long start_t;
  int cnt = 0;
  int state = 0;
  //start_t = getLocalTime(&now, 0);
  int itr = 0;
  //while (getLocalTime(&now, 0) - start_t <=  interval) {
   while(itr < 500){
    float sensorValue = analogRead(ANEMOMETER_PIN);
    //SerialMon.print("Sensor Value:");
    //SerialMon.println(sensorValue);
    float voltage = (sensorValue / 1023) * Vref;

    if ((voltage > (Vref * 0.5)) && state == 0) {
      cnt++;
      state = 1;
      //Serial.print(cnt);
      //Serial.print("\n");
    }
    if ((voltage < (Vref * 0.5)) && state == 1) {
      state = 0;
    }
  delay(10);
  itr ++;
  }
  //SerialMon.print("ticks");
  //SerialMon.println(cnt);
  windspeed = cnt / 10.0; //2*5 sendo 2 o numero de ticks por volta completa e 5 o periodo de medição

  ws_km = windspeed * kmh_factor;

  //Print sensor values to monitor
  //SerialMon.print("Windspeed in Km/h: ");
  //SerialMon.println(ws_km);
  return ws_km;
}
