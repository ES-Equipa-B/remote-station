/*********
 * Course unit - Engeneering Systems
 * FEUP
 * Team B - 2022/23
 *********/
// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800   // Modem is SIM800
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb

#include <Wire.h>
#include <TinyGsmClient.h>

// SIM card PIN (leave empty, if not defined)
const char simPIN[] = "";

// Your phone number to send SMS: + (plus sign) and country code, for Portugal +351, followed by phone number
// SMS_TARGET Example for Portugal +351XXXXXXXXX
#define SMS_TARGET "+351915784796"

// Set serial for debug console (to Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to SIM800 module)
#define SerialAT Serial1

// TTGO T-Call pins
#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 23
#define MODEM_TX 27
#define MODEM_RX 26
#define I2C_SDA 21
#define I2C_SCL 22

// Define the serial console for debug prints, if needed
//#define DUMP_AT_COMMANDS

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
RTC_DATA_ATTR TinyGsm modem(debugger);
#else
RTC_DATA_ATTR TinyGsm modem(SerialAT);
#endif

#define IP5306_ADDR 0x75
#define IP5306_REG_SYS_CTL0 0x00

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
// #define TIME_TO_SLEEP 10       /* Time ESP32 will go to sleep (in seconds) */

TaskHandle_t Task1;
TaskHandle_t Task2;

/*
    Global Variables that must be safed when the ESP is sleeping
*/
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool first_exec = true;

// Mode Debug On/Off (print in Serial Monitor)
// RTC_DATA_ATTR bool DBG_Mode = false;

/** Vectors for saving the sensors readings, up to 10 measures.
 *  Vectors are clean when created a sms for sms_vec
 */
RTC_DATA_ATTR int temp[10], hum[10], wind_speed[10];
/**
 * Counter for number of sensor readings
 */
RTC_DATA_ATTR int measures_cnt = 0;
/**
 * @brief Saves until 10 string with the format of a sms, ready to send
 * as soon as possible.
 */
RTC_DATA_ATTR char sms_vec[10][140];
/**
 * size of the SMS vector, will keep the record and will be usefull
 * when the SMS was sent (so it will desappear from the SMS_vec) but
 * then the Task2 was shut down and this var wasn't updated.
 * This will allow not send twice the same SMS in some ocasions. */
RTC_DATA_ATTR int pending_sms = 0;

/* ************************************************************ */
bool T2_over = false;

/*    Global Variables that don't need to be saved */

// Mode Debug On/Off (print in Serial Monitor)
bool DBG_Mode = false;
/* ************************************************************ */

bool setPowerBoostKeepOn(int en);
void wake_up_gsm();
void shutdown_gsm();
void SMS();
void get_sensors_readings();
int getSMS_vec_size();

void setup()
{
  // Set serial for debug console (to Serial Monitor, default speed 115200)
  if(DBG_Mode)
    SerialMon.begin(115200);
  
  /*
  // Keep power when running from battery
  Wire.begin(I2C_SDA, I2C_SCL);
  bool isOk = setPowerBoostKeepOn(0);
  if(DBG_Mode)
    SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));
  */

 
  // Task 1 - Read Sensor Data, so works like the Master
  // create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
      Task1code, /* Task function. */
      "Task1",   /* name of task. */
      10000,     /* Stack size of task */
      NULL,      /* parameter of the task */
      1,         /* priority of the task */
      &Task1,    /* Task handle to keep track of created task */
      0);        /* pin task to core 0 */
  delay(500);

  // Task 2 - Send SMS when needed, so it's like a slave
  // create a task that will be executed in the Task2code() function, with priority 1 and executed on core 1
  xTaskCreatePinnedToCore(
      Task2code, /* Task function. */
      "Task2",   /* name of task. */
      10000,     /* Stack size of task */
      NULL,      /* parameter of the task */
      1,         /* priority of the task */
      &Task2,    /* Task handle to keep track of created task */
      1);        /* pin task to core 1 */
  delay(500);
}

void Task1code(void *pvParameters)
{

  // For controlling the execution time
  unsigned long start_t = millis();

  String info = "Task1 running on core" + String(xPortGetCoreID());

  // Compare the size of sms_vec and pending_sms
  int sizeOf_smsVec = getSMS_vec_size();
  if (sizeOf_smsVec < pending_sms)
  {
    // In this case, in the anterior cycle the SMS wasn't cleared from the sms_vec
    // So must update the pending_sms
    pending_sms = sizeOf_smsVec;
  }

  // Get Sensors readings (include) save values in the corresponding arrays

  measures_cnt++;
  if (DBG_Mode)
    SerialMon.println("(T1) measures_cnt: %d" + measures_cnt);
  if (measures_cnt == 10)
  {
    // create SMS
    SMS();
    if (DBG_Mode)
      SerialMon.println("(T1) pending_sms: %d " + pending_sms);
    pending_sms++;
    measures_cnt = 0;
  }

  for (;;)
  {
    // Wait for conclusion of Task2
    // OR
    // Timeout when have less then 5sec to next cycle (so has passed 55sec)
    unsigned long now = millis();
    if (T2_over || (now - start_t) > 55000)
    {
      break;
    }
  }
  // Deep Sleep ESP
  delay(100);
  unsigned long now = millis();
  // this 100ms is for the delay before deep_sleep_start()
  unsigned long TIME_TO_SLEEP = (start_t * 1000) + (60 * uS_TO_S_FACTOR) - (now * 1000) - (100 * 1000);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP);

  SerialMon.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  // esp_deep_sleep_pd_config(ESP_PD_OPTION_OFF);
  // SerialMon.println("Going to sleep now");
  delay(100);
  // SerialMon.flush();
  esp_deep_sleep_start();
}

void Task2code(void *pvParameters)
{

  String info = "Task2 running on core" + String(xPortGetCoreID());

  // Safety delay for Task 1 verify the date if it's necessary to execute Task2
  delay(50);
  /*
    This loop will execute while there are SMS to be sent
    OR
    Task 1 put everything to sleep
        In this case could happen the following scenarios:
            - Some corrupted/incompleted SMS can arrive to Main Station (MS) - should be ignored
            (If the ESP go to sleep while the SMS is uploading to network)
            - If the SMS is sent in time but ESP go to sleep before removing the SMS from the sms_vec
              - in this case the MS can detect and ignore
  */
  bool gsm_ON = false;
  while (pending_sms > 0)
  {
    if (!gsm_ON)
    {
      wake_up_gsm();
      gsm_ON = true;
    }

    // Send SMS
    SMS();
    // Clear the last SMS from sms_vec
    // TO DO ...

    pending_sms--;
    if (DBG_Mode)
      SerialMon.println("(T2) pending_sms: %d" + pending_sms);
  }

  if (gsm_ON)
  {
    shutdown_gsm();
    gsm_ON = false;
  }

  T2_over = true;
  if (DBG_Mode)
    SerialMon.println("Task 2 finished !");
  // for (;;) {  }
}

void loop()
{
  // Nothing going here !!!
}

bool setPowerBoostKeepOn(int en)
{
  if (DBG_Mode)
    SerialMon.println("Hi from setPowerBoostKeepOn !");
  /*
   Wire.beginTransmission(IP5306_ADDR);
Wire.write(IP5306_REG_SYS_CTL0);
if (en)
{
  Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
}
else
{
  Wire.write(0x35); // 0x37 is default reg value
}
return Wire.endTransmission() == 0;
  */
}
void wake_up_gsm()
{
  if (DBG_Mode)
    SerialMon.println("Hi from wake_up_gsm !");
  /*
  unsigned long start_t = millis();

// Set modem reset, enable, power pins
pinMode(MODEM_PWKEY, OUTPUT);
pinMode(MODEM_RST, OUTPUT);
pinMode(MODEM_POWER_ON, OUTPUT);


  //USELESS IN THE v1.3
  //pinMode(MODEM_DTR, OUTPUT);
  //digitalWrite(MODEM_DTR, HIGH);

 digitalWrite(MODEM_PWKEY, LOW);
 digitalWrite(MODEM_RST, HIGH);
 digitalWrite(MODEM_POWER_ON, HIGH);

 // Restart SIM800 module, it takes quite some time
 // To skip it, call init() instead of restart()

 // Set GSM module baud rate and UART pins
 SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
 //// atention to this delay
 delay(3000);
 /////////////////////////////
 if (DBG_Mode)
   SerialMon.println("Initializing modem...");
 modem.init();
 unsigned long mid1 = millis();
 // use modem.init() if you don't need the complete restart
 if (DBG_Mode)
   SerialMon.println("From begin to modem.init ends: " + String(mid1 - start_t));
 // Unlock your SIM card with a PIN if needed
 if (strlen(simPIN) && modem.getSimStatus() != 3)
 {
   modem.simUnlock(simPIN);
 }

 // int csq = modem.getSignalQuality();
 // SerialMon.println("Signal quality: " + String(csq));
   */
}
void shutdown_gsm()
{
  if (DBG_Mode)
    SerialMon.println("Hi from shutdown_gsm !");

  /*
    digitalWrite(MODEM_PWKEY, HIGH);
delay(50);
digitalWrite(MODEM_PWKEY, LOW);
delay(1550);
digitalWrite(MODEM_PWKEY, HIGH);
  */
}
void SMS()
{
  if (DBG_Mode)
    SerialMon.println("Hi from SMS !");
  /*
  bool res;
res = modem.testAT();
if (res)
{
  if (DBG_Mode)
    SerialMon.println("GSM module is ON");
}
else
{
  if (DBG_Mode)
    SerialMon.println("GSM module is OFF");
  return;
}

unsigned long start_t = millis();
// To send an SMS, call modem.sendSMS(SMS_TARGET, smsMessage)
// String smsMessage = "Cycle: " + String(bootCount);

String time = modem.getGSMDateTime(DATE_FULL);
String smsMessage = time + "/TT.T/HHH/VV.VV";
// SerialMon.println(time);
if (modem.sendSMS(SMS_TARGET, smsMessage))
{
  // SerialMon.println(smsMessage);
  if (DBG_Mode)
    SerialMon.println("Send SMS time: " + String(millis() - start_t));
}
else
{
  // SerialMon.println("SMS failed to send");
}
  */
}

void get_sensors_readings()
{
  if (DBG_Mode)
    SerialMon.println("Hi from get_sensors_readings !");
}

int getSMS_vec_size()
{
  if (DBG_Mode)
    SerialMon.println("Hi from getSMS_vec_size (ret:0) !");
  return 0;
}
