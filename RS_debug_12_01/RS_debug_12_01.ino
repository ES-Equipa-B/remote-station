#include "main.hpp"

// State macros
// Task 1 state macros
#define S_T1_Setup 0
#define S_T1_StoreSample 1
#define S_T1_PrepareSms 2
#define S_T1_TriggerSmsTask 3
#define S_T1_Sleep 4

// Task 2 state macros
#define S_T2_Wait 5
#define S_T2_SendSms 6

// Tests scenes variables
RTC_DATA_ATTR int sms_sent = -1;
RTC_DATA_ATTR int lastSync = -1;
RTC_DATA_ATTR int boot_time = -1;
RTC_DATA_ATTR int sync_min = -1;

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
void Task1code(void *pvParameters);
// Task 2
TaskHandle_t Task2;
void Task2code(void *pvParameters);

void setup()
{
  // Set serial for debug console (to Serial Monitor, default speed 115200)
  pinMode(ANEMOMETER_PIN, INPUT);
  pinMode(PWR_PIN, OUTPUT);

  DEBUG_SETUPSERIAL(9600);
  dht.begin();
  delay(2000);
  DEBUG_PRINTLN("Hello!");

  #ifdef CPU_FREQ
    DEBUG_PRINT("Setting CPU frequency to ");
    DEBUG_PRINTLN(CPU_FREQ);
    setCpuFrequencyMhz(CPU_FREQ);
  #endif

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

  DEBUG_PRINTLN("Both tasks created!");
  bootCount++;
}

void loop(){

};

void Task1code(void *pvParameters)
{
  unsigned long store_sample_start_t = 0;
  unsigned long sms_task_start_t = 0;
  unsigned long sleep_t = 0;

  DEBUG_PRINTLN("Task1 running on core" + String(xPortGetCoreID()));

  while(1)
  {

    delay(1000);
    DEBUG_PRINTLN("");
    switch (state_t1)
    {

    case (S_T1_Setup):
      DEBUG_PRINTLN("S_T1_Setup");
      // Setup state, runs only on first boot
      // Synchronize local time with network time.
      wakeUpGsm();
      syncRtcWithNetworkTime(1);
      shutdownGsm();

      state_t1 = S_T1_StoreSample;

      // <! -------------------------
      if (sms_sent == -1)
      {
        std::string HHmm = getLocalHourMinutes();
        boot_time = stoi(HHmm.c_str());
        lastSync = stoi(HHmm.c_str());
        DEBUG_PRINTLN(String("(init)boot_time: ") + String(boot_time));
        DEBUG_PRINTLN(String("(init)lastSync: ") + String(lastSync));
        sms_sent = -1;
      }
      getLocalTime(&now, 0);

      while (!(now.tm_sec >= 30 && now.tm_sec <= 35))
      {
        delay(1000);
        getLocalTime(&now, 0);
      }
      // -------------------------- !>
      break;

    case (S_T1_StoreSample):
      digitalWrite(PWR_PIN, HIGH);

      DEBUG_PRINTLN("S_T1_StoreSample");
      measures_cnt++;
      DEBUG_PRINTLN("Measure counter: " + String(measures_cnt));

      slept = 0;
      store_sample_start_t = millis();
      // read sensors here
      timestamp = getLocalHourMinutes();
      // wind_speed=102.5;
      wind_speed = anemometer_meas();
      hum = dht.readHumidity();
      // temp = 18.2;
      temp = dht.readTemperature();
      // hum = 10.0;
      // temp=-21.1;

      digitalWrite(PWR_PIN, LOW);

      // Encode sample and add it to the sms that is being filled before sending
      DEBUG_PRINTLN("SMS before concatenation: " + String(sms));
      strcpy(encoded_sample, encodeSample(timestamp, wind_speed, hum, temp).c_str());
      DEBUG_PRINTLN("Encoded Sample: " + String(encoded_sample));
      strncat(sms, encoded_sample, 150 - strlen(encoded_sample) - 1);
      DEBUG_PRINTLN("SMS after concatenation: " + String(sms));

      if (measures_cnt == (60 / SMS_PER_HOUR))
      {
        // Current SMS is filled with (60/SMS_PER_HOUR) samples
        DEBUG_PRINTLN(String(60 / SMS_PER_HOUR) + " measures obtained.");
        state_t1 = S_T1_PrepareSms;
      }
      else
      {
        // Current SMS is not filled with (60/SMS_PER_HOUR) samples
        if (sms_buffer.size() == 0)
        {
          DEBUG_PRINTLN("No SMSs in queue.");
          state_t1 = S_T1_Sleep;
        }
        else
        {
          DEBUG_PRINTLN("SMSs in queue.");
          state_t1 = S_T1_TriggerSmsTask;
        }
      }

      break;

    case (S_T1_PrepareSms):

      DEBUG_PRINTLN("S_T1_PrepareSms");
      sms_buffer.push(sms); // Push current SMS to queue
      strcpy(sms, "");      // Delete current SMS
      DEBUG_PRINTLN("SMS queue size: " + String(sms_buffer.size()));

      // <! -------------------------
      /*
        hhmm = getLocalHourMinutes();
        esp_t = "ESP_t:" + hhmm;
        sample = " #samples" + to_string(measures_cnt);
        sms_s = " #sms_sent" + to_string(sms_sent);
        dbg =  esp_t + sample + sms_s;
        sms_buffer.push(dbg);
        DEBUG_PRINTLN(String("(PrepSMS)debug sms ") + String(dbg.c_str()));
      */
      // -------------------------- !>

      state_t1 = S_T1_TriggerSmsTask;
      break;

    case (S_T1_TriggerSmsTask):

      if (sms_task_trigger == 0)
      {
        DEBUG_PRINTLN("S_T1_TriggerSmsTask");
        // Trigger task2 to send SMS
        sms_task_trigger = 1;
        sms_task_start_t = millis();
      }
      else
      {

        sms_task_duration = (millis() - sms_task_start_t); // Check timeout

        if (sms_task_ended || sms_task_duration > sms_task_timeout)
        {
          // SMS task ended or timeout reached
          DEBUG_PRINTLN("SMS Task ended or timeout reached.");
          sms_task_trigger = 0;
          sms_task_ended = 0;
          state_t1 = S_T1_Sleep;
        }
      }

      break;

    case (S_T1_Sleep):

      DEBUG_PRINTLN("S_T1_Sleep");

      if (!slept)
      {
        // Sleep
        std::string s_now = getLocalHourMinutes();
        int t_now = stoi(s_now.c_str());
        DEBUG_PRINTLN(String("(Sleep)now: ") + String(t_now));
        DEBUG_PRINTLN(String("(Sleep)boot_time: ") + String(boot_time));
        DEBUG_PRINTLN(String("(Sleep)sync_min: ") + String(sync_min));

        if (t_now == boot_time)
        {
          String msg = String("SMS sent in the last day: ") + String(sms_sent);
          sms_buffer.push(msg.c_str());
          DEBUG_PRINTLN(msg);
          sms_sent = -1;
        }

        // Copy queue to memory before sleep
        DEBUG_PRINTLN("Copying queue to memory.");
        sms_buffer_size = sms_buffer.size();
        while (!sms_buffer.empty())
        {

          DEBUG_PRINTLN("Copying (" + String(sms_buffer.front().c_str()) + ") to position " + String(10 - sms_buffer.size()));

          strcpy(sms_buffer_rtc[10 - sms_buffer.size()], sms_buffer.front().c_str());
          sms_buffer.pop();
        }
        for (int i = 0; i < (10 - sms_buffer_size); i++)
        {
          DEBUG_PRINTLN("Erasing position " + String(i));
          DEBUG_PRINTLN("Position string: " + String(sms_buffer_rtc[i]));
          strcpy(sms_buffer_rtc[i], "");
        }
        getLocalTime(&now, 0);

        // DEBUG_PRINTLN("(Sleep)Seconds now:" + String(now.tm_sec));
        // DEBUG_PRINTLN("(Sleep)now_min:" + String(now.tm_min));
        if (now.tm_sec < 30 && sync_min != now.tm_min)
        {
          // O sync atrasou o relogio mas já temos medição
          sleep_t = 30000 - (now.tm_sec * 1000);
        }
        else
        {
          sleep_t = 60000 - (now.tm_sec * 1000) + 30000;
        }

        // DEBUG_PRINTLN("Seconds now:" + String(now.tm_sec));
        // sleep_t = 60000 - (now.tm_sec * 1000);
        // sleep_t = 60000 - (millis() - store_sample_start_t);
        DEBUG_PRINTLN("ESP will sleep for " + String(sleep_t));
        slept = 1;
        DEBUG_PRINTLN("ESP is sleeping.");

        // Normal mode with the next line
        esp_sleep_enable_timer_wakeup(sleep_t * 1000);
        // Just for speed up debeug
        // esp_sleep_enable_timer_wakeup(5000 * 1000); // Sleep for 1 second to debug
        // esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
        esp_deep_sleep_start();
      }

      else
      {
        // Woke up from sleep
        //  Copy memory to queue
        DEBUG_PRINTLN("Copying memory to queue.");
        sms_buffer.empty();

        for (int i = 0; i < 10; i++)
        {
          if (sms_buffer_rtc[i][0] != '\0')
          {
            strncpy(sms_to_push, sms_buffer_rtc[i], 160);
            DEBUG_PRINTLN("Pushing the index " + String(i) + "(" + String(sms_to_push) + ") to queue because it is not null.");
            sms_buffer.push(sms_to_push);
          }
        }

        if (measures_cnt == (int)(60 / SMS_PER_HOUR))
        {
          // Reset measures_cnt if 10 samples were already obtained.
          DEBUG_PRINTLN("ESP woke up, with 10 samples obtained.");
          measures_cnt = 0;
        }
        else
        {
          DEBUG_PRINTLN("ESP woke up, with less than 10 samples obtained.");
        }

        state_t1 = S_T1_StoreSample;
      }

      break;
    }
  }
}

void Task2code(void *pvParameters)
{

  DEBUG_PRINTLN("Task2 running on core" + String(xPortGetCoreID()));

  for (;;)
  {

    delay(1000); // uncomment delay for debug

    switch (state_t2)
    {

    case (S_T2_Wait):
      // Waits for sms_task_trigger
      DEBUG_PRINTLN("S_T2_Wait");

      if (sms_task_trigger == 1)
      {
        DEBUG_PRINTLN("Sms task triggered.");
        state_t2 = S_T2_SendSms;
      }
      break;

    case (S_T2_SendSms):
      // Sms task triggered
      DEBUG_PRINTLN("S_T2_SendSms");
      printQueue(sms_buffer);

      if (sms_buffer.size() == 0)
      {
        DEBUG_PRINTLN("There are no SMS in buffer.");
        sms_task_ended = 1;
        state_t2 = S_T2_Wait;
      }
      else
      {
        DEBUG_PRINTLN("There are SMSs in buffer.");
        printQueue(sms_buffer);

        // while(1); // Uncomment to force SMS sending failure.

        if (sendAllSmsInBuffer_sms_test(sms_buffer, lastSync, sms_sent, sync_min) == 0)
        {

          DEBUG_PRINTLN("All SMSs in buffer were sent.");
          printQueue(sms_buffer);
          sms_task_ended = 1;
          state_t2 = S_T2_Wait;
        }
      }

      break;
    }
  }
}
