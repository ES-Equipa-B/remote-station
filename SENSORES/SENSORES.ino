//Libraries
#include <DHT.h>

//Constants
  //DHT
#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino
  //ANEMOMETER
#define anemoPin 23
const float kmh_factor = 2.401;
unsigned long interval = 5000UL;


//Variables
  //DHT
int chk;
float hum;  //Stores humidity value
float temp; //Stores temperature value
  //ANEMOMETER
int state = 0;
volatile int cnt = 0;
unsigned long myTime, oldTime = 0;

//Functions
float calcWindSpeed(int cnt){
  
  float windspeed = cnt/10.0; //2*5 sendo 2 o numero de ticks por volta completa e 5 o periodo de medição

  float ws_km = windspeed * kmh_factor;

  return ws_km;
  
}

void setup() {
  
  Serial.begin(9600);
  dht.begin();
  pinMode(anemoPin, INPUT);

}

void loop() {
  
  float sensorValue = analogRead(23);
  float voltage = (sensorValue / 1023) * 5;

  if(voltage > 1.5 && state == 0){
    cnt++;
    state=1;
    //Serial.print(cnt);
    //Serial.print("\n");
  }
  if(voltage < 1.5 && state == 1){
    state=0;
  }
  
  if (millis() - oldTime > interval){ 
    
    hum = dht.readHumidity();
    temp= dht.readTemperature();
    
    //Print sensor values to monitor
    Serial.print("Windspeed in Km/h: ");
    Serial.println(calcWindSpeed(cnt));
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.print(" %, Temp: ");
    Serial.print(temp);
    Serial.println(" Celsius");
    Serial.print("\n");
    
    oldTime += interval;  
    cnt = 0;
  }

}
