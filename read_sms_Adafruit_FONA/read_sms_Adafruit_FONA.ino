/*
  Tutorial: Control servo movement with gesture using ESP32 and Arduino

  Board:
  - TTGO T-Call ESP32 with SIM800L GPRS Module
    https://my.cytron.io/p-ttgo-t-call-esp32-with-sim800l-gprs-module
  Output:
  - 1CH Active H/L 5V OptoCoupler Relay Module
    https://my.cytron.io/c-electronic-components/c-relays/p-1ch-active-h-l-5v-optocoupler-relay-module
  Connections   TTGO | Servo
                  5V - DC+
                 GND - DC-
                  IN - 14
  External libraries:
  - Adafruit Fona Library by Adafruit Version 1.3.5
*/

#include "Adafruit_FONA.h"
#include <Preferences.h>
Preferences preferences;

RTC_DATA_ATTR String SMS_TARGET = "";
RTC_DATA_ATTR int rateSMS = 0;
RTC_DATA_ATTR int flag = 0;
RTC_DATA_ATTR bool receive_sms = false;

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 10       /* Time ESP32 will go to sleep (in seconds) */

#define SIM800L_RX     27
#define SIM800L_TX     26
#define SIM800L_PWRKEY 4
#define SIM800L_RST    5
#define SIM800L_POWER  23

RTC_DATA_ATTR char replybuffer[255];

RTC_DATA_ATTR HardwareSerial *sim800lSerial = &Serial1;
RTC_DATA_ATTR Adafruit_FONA sim800l = Adafruit_FONA(SIM800L_PWRKEY);

RTC_DATA_ATTR uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

#define LED_BLUE  13

String smsString = "";
String callerString = "";
String codeString = "";


void setup()
{

  preferences.begin("specifications", false);
  //preferences.clear();
  rateSMS = preferences.getUInt("rateSMS", 0);
  SMS_TARGET = preferences.getString("SMS_TARGET", "");

  pinMode(LED_BLUE, OUTPUT);
  //pinMode(RELAY, OUTPUT);
  pinMode(SIM800L_POWER, OUTPUT);

  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(SIM800L_POWER, HIGH);

  Serial.begin(115200);
  Serial.println("Configuration specifications:");
  Serial.print(F("SMS rate: ")); Serial.println(rateSMS);
  Serial.print(F("SMS Target number: ")); Serial.println(SMS_TARGET);
  Serial.println(F("ESP32 with GSM SIM800L"));
  Serial.println(F("Initializing....(May take more than 5 seconds)"));

  delay(5000);

  // Make it slow so its easy to read!
  sim800lSerial->begin(9600, SERIAL_8N1, SIM800L_TX, SIM800L_RX);
  if (!sim800l.begin(*sim800lSerial)) {
    Serial.println(F("Couldn't find GSM SIM800L"));
    while (1);
  }
  Serial.println(F("GSM SIM800L is OK"));

  // Print SIM card IMEI number.
  char imei[16] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = sim800l.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("SIM card IMEI: "); Serial.println(imei);
  }

  // Set up the FONA to send a +CMTI notification
  // when an SMS is received
  sim800lSerial->print("AT+CNMI=2,1\r\n");

  Serial.println("GSM SIM800L Ready");
}

long prevMillis = 0;
int interval = 1000;
boolean ledState = false;

long prevTimeout = 0;
long timeout = 30000;

char sim800lNotificationBuffer[64];          //for notifications from the FONA
char smsBuffer[250];

void blink_led()
{
  if (millis() - prevMillis > interval)
  {
    ledState = !ledState;
    digitalWrite(LED_BLUE, ledState);
    prevMillis = millis();
  }

}

void blink_led_faster()
{
  if (millis() - prevMillis > (interval / 2))
  {
    ledState = !ledState;
    digitalWrite(LED_BLUE, ledState);
    prevMillis = millis();
  }
}

void wait_for_sms()
{
  char* bufPtr = sim800lNotificationBuffer;    //handy buffer pointer
  int slot = 0; //this will be the slot number of the SMS
  int charCount = 0;

  //Read the notification into fonaInBuffer
  do {
    *bufPtr = sim800l.read();
    Serial.write(*bufPtr);
    delay(1);
  } while ((*bufPtr++ != '\n') && (sim800l.available()) && (++charCount < (sizeof(sim800lNotificationBuffer) - 1)));

  //Add a terminal NULL to the notification string
  *bufPtr = 0;

  //Scan the notification string for an SMS received notification.
  //  If it's an SMS message, we'll get the slot number in 'slot'
  if (1 == sscanf(sim800lNotificationBuffer, "+CMTI: \"SM\",%d", &slot))
  {
    Serial.print("slot: "); Serial.println(slot);

    char callerIDbuffer[32];  //we'll store the SMS sender number in here

    //Retrieve SMS sender address/phone number.
    if (!sim800l.getSMSSender(slot, callerIDbuffer, 31))
    {
      Serial.println("Didn't find SMS message in slot!");
    }
    callerString = String(callerIDbuffer);
    Serial.print(F("FROM: ")); Serial.println(callerString);

    //Retrieve SMS value.
    uint16_t smslen;
    if (sim800l.readSMS(slot, smsBuffer, 250, &smslen)) // Pass in buffer and max len!
    {
      smsString = String(smsBuffer);
      Serial.print(F("SMS received: ")); Serial.println(smsString);
    }

    if (sim800l.deleteSMS(slot)) {
      Serial.println(F("OK (deleted)!"));
    }
    else {
      Serial.print(F("Couldn't delete SMS in slot ")); Serial.println(slot);
      sim800l.print(F("AT+CMGD=?\r\n"));
    }
  }
}

bool configuration()
{
  if (sim800l.available()) {
    wait_for_sms();
  }

  //Take the first 6 chars (ES2022)
  codeString = smsString.substring(0, 6);

  //checking configuration code
  if (codeString == "ES2022") {
    Serial.println("Configuration is activated.");
    receive_sms = true;

    //save sms rate configuration in eeprom (rtcmemory)
    //Take the last chars strating from the 7th and turn them into integer
    int temp_rateSMS = smsString.substring(7).toInt();
    if (rateSMS != temp_rateSMS) {
      preferences.putUInt("rateSMS", temp_rateSMS);
      rateSMS = temp_rateSMS;
    }
    Serial.print(F("SMS rate: ")); Serial.println(rateSMS);

    //save phone number in eeprom (rtcmemory)
    if (SMS_TARGET != callerString) {
      preferences.putString("SMS_TARGET", callerString);
      SMS_TARGET = callerString;
    }
    Serial.print(F("SMS Target number: ")); Serial.println(SMS_TARGET);

    delay(100);
    return true;
  }
  else {
    return false;
  }
}

void loop()
{
  //setup routine
  //(ALTERAR PARA UMA FLAG PARA QUANDO FOR DORMIR NÃO PERDER ESTE VALOR MAS QUANDO FIZER RESET ENTRAR NESTE IF NA MESMA)(DONE)
  //if ( rateSMS == 0 && SMS_TARGET == "") {
  if ( flag == 0 ) {
    Serial.println("Waiting for SMS...");
    prevTimeout = millis();
    while (millis() - prevTimeout < timeout) {
      blink_led();
      int check = configuration();

      //Check stored number and SMS rate
      //if ( ( rateSMS != 0 ) && ( SMS_TARGET != "" ) )
      if ( check )
      {
        break;
      }
    } Serial.println("TIMEOUT");

    //Check if there is NO stored number and NO SMS rate
    if ( !( ( rateSMS != 0 ) && ( SMS_TARGET != "" ) ) )
    {
      flag = 0;
      //sleep!!!!!!!!!!!!!!
      //Serial.println("Going to sleep...");
      //esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
      //delay(1000);
      //esp_deep_sleep_start();
    }
    else {
      //Succeeded once!
      flag = 1;

      //Send back an automatic response //NÃO ESTÁ A FUNCIONAR, MAS PODEMOS USAR A BIBLIOTECA TINYGSM PARA ENVIAR ESTA SMS
      if (receive_sms) {
        Serial.println("Sending response...");
        if (sim800l.available()) {

          // Length (with one extra character for the null terminator)
          int str_len = SMS_TARGET.length() + 1;

          // Prepare the character array (the buffer)
          char sms_target_buffer[str_len];

          // Copy it over
          SMS_TARGET.toCharArray(sms_target_buffer, str_len);

          if (sim800l.sendSMS(sms_target_buffer, "Configuration successfully finished.")) {
            Serial.println(F("Sent!"));
          }
          else {
            Serial.println(F("Failed"));
          }
        }
      }
      Serial.println("Configuration successfully finished.");
    }
  }

  blink_led_faster();

}
