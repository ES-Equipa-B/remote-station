#include "RS.hpp"

// State macros
//Task 1 state macros
#define S_T1_Setup 0
#define S_T1_StoreSample 1
#define S_T1_PrepareSms 2
#define S_T1_TriggerSmsTask 3
#define S_T1_Sleep 4

//Task 2 state macros
#define S_T2_Wait 5
#define S_T2_SendSms 6

// Set serial for debug console
#define SerialMon Serial

// Variables that are saved during ESP sleep
// Timeout
RTC_DATA_ATTR int sms_task_timeout = SMS_TASK_TIMEOUT;
// Flags
RTC_DATA_ATTR int sms_task_trigger = 0;
RTC_DATA_ATTR int sms_task_ended = 0;
RTC_DATA_ATTR bool slept = 0;

// Counters
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int measures_cnt = 0;

// Time
RTC_DATA_ATTR int sms_task_duration = 0;

// State initialization
RTC_DATA_ATTR int state_t1 = S_T1_Setup;
RTC_DATA_ATTR int state_t2 = S_T2_Wait;

// Sms
RTC_DATA_ATTR char sms_buffer_rtc[10][160];
RTC_DATA_ATTR char sms[160];

// Variables that are not saved during ESP sleep
// SMS
FixedQueue<string, 10> sms_buffer;
char sms_to_push[160];
// Counters
int sms_buffer_size = 0;
// Samples
string timestamp;
float hum, wind_speed, temp;
char encoded_sample[15];

struct tm now;

// Sensors
DHT dht(DHTPIN, DHTTYPE); // Init DHT22 sensor

// Tasks
// Task 1
TaskHandle_t Task1;
void Task1code (void * pvParameters);
// Task 2
TaskHandle_t Task2;
void Task2code (void * pvParameters);

void setup()
{

  // Set serial for debug console (to Serial Monitor, default speed 115200)
  pinMode(ANEMOMETER_PIN, INPUT);
  pinMode(PWR_PIN, OUTPUT);
    
  //SerialMon.begin(9600);
  dht.begin();
  delay(2000);
  //Serial.println("Hello!");


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

  xTaskCreatePinnedToCore(
    Task2code, /* Task function. */
    "Task2",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    1,         /* priority of the task */
    &Task2,    /* Task handle to keep track of created task */
    1);        /* pin task to core 1 */

  //SerialMon.println("Both tasks created!");
  bootCount++;



}

void loop() {

};

void Task1code(void *pvParameters){
unsigned long store_sample_start_t = 0;
unsigned long sms_task_start_t = 0;
unsigned long sleep_t = 0;

//SerialMon.println("Task1 running on core" + String(xPortGetCoreID()));

for (;;) {

  delay(1000);
  //SerialMon.println("");
  switch (state_t1) {

    case (S_T1_Setup):
      //SerialMon.println("S_T1_Setup");
      // Setup state, runs only on first boot
      // Synchronize local time with network time.
      wakeUpGsm();
      syncRtcWithNetworkTime();

      state_t1 = S_T1_StoreSample;
      break;

    case (S_T1_StoreSample):
      digitalWrite(PWR_PIN,HIGH);

      //SerialMon.println("S_T1_StoreSample");
      measures_cnt++;
      //SerialMon.println("Measure counter: "  + String(measures_cnt));

      slept = 0;
      store_sample_start_t = millis();
      //read sensors here
      timestamp = getLocalHourMinutes();
      //wind_speed=102.5;
      wind_speed = anemometer_meas();
      hum = dht.readHumidity();
      //temp = 18.2;
      temp = dht.readTemperature();
      //hum = 10.0;
      //temp=-21.1;

      digitalWrite(PWR_PIN,LOW);

      // Encode sample and add it to the sms that is being filled before sending
      //SerialMon.println("SMS before concatenation: "  + String(sms));
      strcpy(encoded_sample, encodeSample(timestamp, wind_speed, hum, temp).c_str());
      //SerialMon.println("Encoded Sample: "  + String(encoded_sample));
      strncat(sms, encoded_sample, 150 - strlen(encoded_sample) - 1);
      //SerialMon.println("SMS after concatenation: "  + String(sms));

      if (measures_cnt == (60 / SMS_PER_HOUR)) {
        // Current SMS is filled with (60/SMS_PER_HOUR) samples
        //SerialMon.println(String(60 / SMS_PER_HOUR) + " measures obtained.");
        state_t1 = S_T1_PrepareSms;
      }
      else {
        // Current SMS is not filled with (60/SMS_PER_HOUR) samples
        if (sms_buffer.size() == 0) {
          //SerialMon.println("No SMSs in queue.");
          state_t1 = S_T1_Sleep;
        }
        else {
          //SerialMon.println("SMSs in queue.");
          state_t1 = S_T1_TriggerSmsTask;
        }
      }
    
      break;

    case (S_T1_PrepareSms):

      //SerialMon.println("S_T1_PrepareSms");
      sms_buffer.push(sms); // Push current SMS to queue
      strcpy(sms, ""); // Delete current SMS
      //SerialMon.println("SMS queue size: "  + String(sms_buffer.size()));

      state_t1 = S_T1_TriggerSmsTask;
      break;

    case (S_T1_TriggerSmsTask):

      //SerialMon.println("S_T1_TriggerSmsTask");

      if (sms_task_trigger == 0) {
        // Trigger task2 to send SMS
        sms_task_trigger = 1;
        sms_task_start_t = millis();
      }
      else {

        sms_task_duration = (millis() - sms_task_start_t); // Check timeout

        if (sms_task_ended || sms_task_duration > sms_task_timeout) {
          // SMS task ended or timeout reached
          //SerialMon.println("SMS Task ended or timeout reached.");
          sms_task_trigger = 0;
          sms_task_ended = 0;
          state_t1 = S_T1_Sleep;
        }
      }

      break;

    case (S_T1_Sleep):

      //SerialMon.println("S_T1_Sleep");

      if (!slept) {
        // Sleep
        //Copy queue to memory before sleep
        //SerialMon.println("Copying queue to memory.");
        sms_buffer_size = sms_buffer.size();
        while (!sms_buffer.empty()) {

          //SerialMon.println("Copying (" + String(sms_buffer.front().c_str()) + ") to position " + String(10 - sms_buffer.size()));

          strcpy(sms_buffer_rtc[10 - sms_buffer.size()], sms_buffer.front().c_str());
          sms_buffer.pop();
        }
        for (int i = 0; i < (10 - sms_buffer_size); i++) {
          //SerialMon.println("Erasing position " + String(i));
          //SerialMon.println("Position string: " + String(sms_buffer_rtc[i]));
          strcpy(sms_buffer_rtc[i], "");
        }
        getLocalTime(&now, 0);
        //SerialMon.println("Seconds now:" + String(now.tm_sec));
        sleep_t = 60000 - (now.tm_sec * 1000);
        //sleep_t = 60000 - (millis() - store_sample_start_t);
        //SerialMon.println("ESP will sleep for " + String(sleep_t));
        slept = 1;
        //SerialMon.println("ESP is sleeping.");

        // Normal mode with the next line
        esp_sleep_enable_timer_wakeup(sleep_t*1000);
        // Just for speed up debeug
        //esp_sleep_enable_timer_wakeup(5000 * 1000); // Sleep for 1 second to debug
        esp_deep_sleep_start();

      }

      else {
        //Woke up from sleep
        // Copy memory to queue
        //SerialMon.println("Copying memory to queue.");
        sms_buffer.empty();

        for (int i = 0; i < 10; i++) {
          if (sms_buffer_rtc[i][0] != '\0') {
            strncpy(sms_to_push, sms_buffer_rtc[i], 160);
            //SerialMon.println("Pushing the index " + String(i) + "(" + String(sms_to_push) + ") to queue because it is not null.");
            sms_buffer.push(sms_to_push);
          }
        }

        if (measures_cnt == (int)(60 / SMS_PER_HOUR)) {
          // Reset measures_cnt if 10 samples were already obtained.
          //SerialMon.println("ESP woke up, with 10 samples obtained.");
          measures_cnt = 0;
        }
        else {
          //SerialMon.println("ESP woke up, with less than 10 samples obtained.");
        }

        state_t1 = S_T1_StoreSample;

      }


      break;
  }
}
}

void Task2code (void * pvParameters) {

  //SerialMon.println("Task2 running on core" + String(xPortGetCoreID()));

  for (;;) {

    delay(1000); // uncomment delay for debug

    switch (state_t2) {

      case (S_T2_Wait):
        // Waits for sms_task_trigger
        //SerialMon.println("S_T2_Wait");

        if (sms_task_trigger == 1) {
          //SerialMon.println("Sms task triggered.");
          state_t2 = S_T2_SendSms;
        }
        break;

      case (S_T2_SendSms):
        // Sms task triggered
        //SerialMon.println("S_T2_SendSms");
        printQueue(sms_buffer);

        if (sms_buffer.size() == 0) {
          //SerialMon.println("There are no SMS in buffer.");
          sms_task_ended = 1;
          state_t2 = S_T2_Wait;

        }
        else {
          //SerialMon.println("There are SMSs in buffer.");
          printQueue(sms_buffer);

          //while(1); // Uncomment to force SMS sending failure.

          if (sendAllSmsInBuffer(sms_buffer) == 0) {

            //SerialMon.println("All SMSs in buffer were sent.");
            printQueue(sms_buffer);
            sms_task_ended = 1;
            state_t2 = S_T2_Wait;

          }
        }

        break;
    }
  }
}
