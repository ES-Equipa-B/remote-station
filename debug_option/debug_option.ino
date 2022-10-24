/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-sim800l-send-text-messages-sms/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/


// SIM card PIN (leave empty, if not defined)
const char simPIN[] = "";

// Your phone number to send SMS: + (plus sign) and country code, for Portugal +351, followed by phone number
// SMS_TARGET Example for Portugal +351XXXXXXXXX
#define SMS_TARGET "+351915784796"

// Configure TinyGSM library
#define TINY_GSM_MODEM_SIM800   // Modem is SIM800
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb

#include <Wire.h>
#include <TinyGsmClient.h>

// TTGO T-Call pins
#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 23
#define MODEM_TX 27
#define MODEM_RX 26
#define I2C_SDA 21
#define I2C_SCL 22

// Set serial for debug console (to Serial Monitor, default speed 115200)
#define SerialMon Serial
// Set serial for AT commands (to SIM800 module)
#define SerialAT Serial1

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
#define TIME_TO_SLEEP 10       /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR bool first_exec = true;
// Mode Debug On/Off (print in Serial Monitor)
RTC_DATA_ATTR bool DBG_Mode = false;

void wake_up();
void shutdown_();
void SMS()
{
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
}

bool setPowerBoostKeepOn(int en)
{
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
}

void setup()
{
  // Set console baud rate
  if(DBG_Mode)
    SerialMon.begin(115200);
  // Keep power when running from battery
  Wire.begin(I2C_SDA, I2C_SCL);
  bool isOk = setPowerBoostKeepOn(0);
  if(DBG_Mode)
    SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));

  /*if (first_exec) {


    if (first_exec) {

      first_exec = false;

      // Put GSM to sleep
      //modem.sendAT(GF("+CSCLK="), true);
      //while (modem.waitResponse() != 1);
    }

    }*/
}
void wake_up()
{

  unsigned long start_t = millis();

  // Set modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);

  /*
      USELESS IN THE v1.3
    pinMode(MODEM_DTR, OUTPUT);
    digitalWrite(MODEM_DTR, HIGH);
  */
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
}

void shutdown_()
{

  digitalWrite(MODEM_PWKEY, HIGH);
  delay(50);
  digitalWrite(MODEM_PWKEY, LOW);
  delay(1550);
  digitalWrite(MODEM_PWKEY, HIGH);
}

void loop()
{
  // wake-up GSM
  /*
    digitalWrite(MODEM_DTR, LOW);
    delay(55);
    modem.sendAT(GF("+CSCLK="), false);
    while (modem.waitResponse() != 1);
    digitalWrite(MODEM_DTR, HIGH);
  */
  wake_up();
  SMS();
  shutdown_();
  // Put GSM to sleep

  // Increment boot number and print it every reboot
  ++bootCount;
  if(DBG_Mode)
    SerialMon.println("Boot number: " + String(bootCount));
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  // SerialMon.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");

  // esp_deep_sleep_pd_config(ESP_PD_OPTION_OFF);

  // SerialMon.println("Going to sleep now");
  delay(1000);
  // SerialMon.flush();
  esp_deep_sleep_start();
}
