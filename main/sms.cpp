#include "sms.hpp"

RTC_DATA_ATTR TinyGsm modem(SerialAT);

// Sim800l

// Set Sim800l power boost.
int setPowerBoostKeepOn(int en)
{
  DEBUG_PRINTLN("Hi from setPowerBoostKeepOn !");

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
  return Wire.endTransmission();
}

// Wake up Sim800l
int wakeUpGsm()
{
  DEBUG_PRINTLN("GSM is waking up.");

  Wire.begin(I2C_SDA, I2C_SCL);

  if (setPowerBoostKeepOn(0) == 0)
  {
    DEBUG_PRINTLN(String("IP5306 KeepOn OK"));
  }
  else
  {
    DEBUG_PRINTLN(String("IP5306 KeepOn failed."));
    return -1;
  }

  // Set modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);

  // USELESS IN THE v1.3
  // pinMode(MODEM_DTR, OUTPUT);
  // digitalWrite(MODEM_DTR, HIGH);

  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  // Restart SIM800 module, it takes quite some time
  // To skip it, call init() instead of restart()

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  //// atention to this delay
  delay(3000);

  DEBUG_PRINTLN("Initializing modem.");
  modem.init();
  // use modem.init() if you don't need the complete restart

  // Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3)
  {
    modem.simUnlock(simPIN);
  }

  if (modem.waitForNetwork())
  {
    DEBUG_PRINTLN("Connected to network.");
    return 0;
  }
  else
  {
    DEBUG_PRINTLN("Failed to connect to network.");
    return -2;
  }
}

// Shutdown Sim800l
void shutdownGsm()
{

  DEBUG_PRINTLN("Shutting down GSM.");

  digitalWrite(MODEM_PWKEY, HIGH);
  delay(50);
  digitalWrite(MODEM_PWKEY, LOW);
  delay(1550);
  digitalWrite(MODEM_PWKEY, HIGH);
}

// Send SMS containing msg.
int sendSms(string msg)
{

  while (!modem.sendSMS(SMS_TARGET, String(msg.c_str())))
  {
    DEBUG_PRINTLN("SMS failed to send");
    delay(500);
  }
  DEBUG_PRINTLN("SMS successfully sent.");
  return 0;
}

// Sample encoding
// Encode sample with sensor readings and timestamp.
string encodeSample(string timestamp, float wind_speed, float hum, float temp)
{
  string encodedSample;
  string wind_speed_s;

  // For Wind Speed
  if (wind_speed < 100.0)
  {
    wind_speed_s = "0" + to_string(ceil(wind_speed * 100) / 100).substr(0, 4);
  }
  else
  {
    wind_speed_s = to_string(ceil(wind_speed * 100) / 100).substr(0, 4);
  }

  auto it = remove_if(wind_speed_s.begin(), wind_speed_s.end(), [](char const &c)
                      { return ispunct(c); });

  wind_speed_s.erase(it, wind_speed_s.end());
  // wind_speed_s = remove_if (wind_speed_s.begin (), wind_speed_s.end (), ispunct("."));
  // string wind_speed_s = to_string(ceil(wind_speed*100.0)/100.0).erase(3, 1).substr(0,4);
  // DEBUG_PRINTLN(wind_speed);
  // DEBUG_PRINTLN(wind_speed_s.c_str());

  // For Humidity
  string hum_s;
  if (hum < 100.0)
  {
    hum_s = "0" + to_string(ceil(hum * 100) / 100).substr(0, 3);
  }
  else
  {
    hum_s = to_string(ceil(hum * 100) / 100).substr(0, 3);
  }
  it = remove_if(hum_s.begin(), hum_s.end(), [](char const &c)
                 { return ispunct(c); });

  hum_s.erase(it, hum_s.end());
  // hum_s = remove_if (hum_s.begin (), hum_s.end (), ispunct("."));
  // string hum_s = to_string(hum).erase(3, 1).substr(0,3);
  // DEBUG_PRINTLN(hum);
  // DEBUG_PRINTLN(hum_s.c_str());

  // For temperature
  string temp_s;
  if (temp < -10)
    temp_s = "1" + to_string(ceil(temp * 100.0) / 100.0).erase(0, 1).substr(0, 4);
  else if (temp < 0)
    temp_s = "10" + to_string(ceil(temp * 100.0) / 100.0).erase(0, 1).substr(0, 3);
  else if (temp < 10)
    temp_s = "00" + to_string(ceil(temp * 100.0) / 100.0).substr(0, 3);
  else
    temp_s = "0" + to_string(ceil(temp * 100.0) / 100.0).substr(0, 4);

  it = remove_if(temp_s.begin(), temp_s.end(), [](char const &c)
                 { return ispunct(c); });

  temp_s.erase(it, temp_s.end());
  // DEBUG_PRINTLN(temp);
  // DEBUG_PRINTLN(temp_s.c_str());

  encodedSample = timestamp + wind_speed_s + hum_s + temp_s;

  return encodedSample;
}

// Test EncodeSample().
void test_encodeSample()
{
  delay(2000);
  string time;
  float temp, hum, wind_speed;
  FixedQueue<string, 10> sms_buffer;

  time = "HHmm";
  wind_speed = 222.2;

  hum = 100;
  temp = 11.11;

  DEBUG_PRINTLN(encodeSample(time, wind_speed, hum, temp).c_str());

  sms_buffer.push("1");
  sms_buffer.push("2");
  sms_buffer.push("3");
  sms_buffer.push("4");
  sms_buffer.push("5");
  sms_buffer.push("6");
  sms_buffer.push("7");
  sms_buffer.push("8");
  sms_buffer.push("9");
  sms_buffer.push("10");
  sms_buffer.push("11");
  sms_buffer.push("12");

  printQueue(sms_buffer);
}

// Send SMS task
// Send all SMS in buffer until it is empty.
int sendAllSmsInBuffer(queue<string> &buffer)
{

  wakeUpGsm();

  while (!buffer.empty())
  {
    DEBUG_PRINT("Sending: ");
    DEBUG_PRINT(String(buffer.front().c_str()) + "\n");
    DEBUG_PRINTLN("");

    if (sendSms(buffer.front()) == 0)
    {
      buffer.pop();
    }
  }
  shutdownGsm(); // WARNING: Move outside of while?

  return 0;
}

int sendAllSmsInBuffer_sms_test(queue<string> &buffer, int &lastSync, int &sms_sent_cnt, int &sync_min)
{

  wakeUpGsm();

  struct tm now;
  getLocalTime(&now, 0);
  sync_min = now.tm_min;
  DEBUG_PRINTLN("(Sync)Seconds now:" + String(now.tm_sec));
  DEBUG_PRINTLN("(Sync)now_min:" + String(now.tm_min));

  while (!buffer.empty())
  {
    DEBUG_PRINT("Sending: ");
    DEBUG_PRINTLN(String(buffer.front().c_str()));
    DEBUG_PRINTLN("");

    if (sendSms(buffer.front()) == 0)
    {
      buffer.pop();
      sms_sent_cnt++;
    }
  }

  /*
    std::string now = getLocalHourMinutes();
    int i_now = stoi(now);
    DEBUG_PRINT("(int)Now: ");
    DEBUG_PRINTLN(i_now);
    DEBUG_PRINT("(int)LastSync: ");
    DEBUG_PRINTLN(lastSync);
    if (compareTime(i_now, lastSync)) {
    lastSync = i_now;
    }
  */

  syncRtcWithNetworkTime(0);
  shutdownGsm();

  return 0;
}

// Test sendAllSmsInBuffer().
int test_sendAllSmsInBuffer()
{

  FixedQueue<string, 10> sms_buffer;

  sms_buffer.push("1");
  sms_buffer.push("2");
  sms_buffer.push("3");
  sms_buffer.push("4");
  sms_buffer.push("5");
  sms_buffer.push("6");
  sms_buffer.push("7");
  sms_buffer.push("8");
  sms_buffer.push("9");
  sms_buffer.push("10");

  sendAllSmsInBuffer(sms_buffer);

  return 0;
}

// Time
// Print ESP local time (RTC).
void printLocalTime()
{
  struct tm now;
  getLocalTime(&now, 0);
  DEBUG_PRINTLN(&now, "%B %d %Y %H:%M:%S (%A)");
}

// Returns timestamp string in format HHmm from local ESP time.
string getLocalHourMinutes()
{
  // string hhmm;
  struct tm now;
  getLocalTime(&now, 0);
  ostringstream oss;
  oss << std::put_time(&now, "%H%M");
  return oss.str();
}

// Syncs ESP local time (RTC) with network time.
int syncRtcWithNetworkTime(bool first_exec)
{

  int yr, month, mday, hr, minute, sec, isDst;
  setenv("TZ", TZ_INFO, 1);
  tzset(); // Assign the local timezone from setenv

  if (!modem.waitForNetwork(5000))
    wakeUpGsm();

  string gsm_datetime;
  /* // In case of other mobile operator, like NOS (in Portugal) the network time is default, doesn't have support with this method
    while(stoi(gsm_datetime.substr(0,2)) < 2020){
     gsm_datetime=modem.getGSMDateTime(DATE_FULL).c_str();
     DEBUG_PRINTF("GSM time: %s",gsm_datetime.c_str());
     delay(1000);
    }
  */
  int i = 0;
  // because the default network time is 1/1/2004 00h00
  do
  {
    delay(1000);
    gsm_datetime = modem.getGSMDateTime(DATE_FULL).c_str();
    yr = stoi(gsm_datetime.substr(0, 2));
    i++;
  } while (i < 10 && yr <= 4);

  if (yr > 4 || first_exec) {
    month = stoi(gsm_datetime.substr(3, 2));
    mday = stoi(gsm_datetime.substr(6, 2));
    hr = stoi(gsm_datetime.substr(9, 2));
    minute = stoi(gsm_datetime.substr(12, 2));
    sec = stoi(gsm_datetime.substr(15, 2));
    isDst = stoi(gsm_datetime.substr(18, 2));

  DEBUG_PRINTF("%d %d %d %d %d %d %d\n", yr, month, mday, hr, minute, sec, isDst);

    struct tm tm;

  tm.tm_year = yr + 100; //(2000+yy-1900) // Set date
  tm.tm_mon = month - 1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;      // Set time
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;  // 1 or 0

  time_t t = mktime(&tm);
  Serial.printf("Setting time: %s\n", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
  }

  return 0;
}

// Test syncRtcWithNetworkTime
int test_syncRtcWithNetworkTime()
{

  printLocalTime();
  syncRtcWithNetworkTime(0);
  printLocalTime();

  return 0;
}

// Aux
// Convert C++ queue to C string array.
char **queueToCStringArray(FixedQueue<string, 10> queue)
{

  char **c_string_array = (char **)malloc(10 * sizeof(char *));

  for (int i = 0; i < 10; ++i)
    c_string_array[i] = (char *)malloc(160 * sizeof(char));

  DEBUG_PRINTLN("queue to char string array");

  while (!queue.empty())
  {

    strcpy(c_string_array[10 - queue.size()], queue.front().c_str());
    queue.pop();
  }

  return c_string_array;
}

// Prints C string array.
int printCStringArray(char **c_string_array)
{

  for (int i = 0; i < 10; i++)
  {

    DEBUG_PRINTLN(c_string_array[i]);
  }

  return 0;
}

// Convert C string array to C++ queue.
FixedQueue<string, 10> cStringArrayToQueue(char **c_string_array)
{

  FixedQueue<string, 10> queue;

  DEBUG_PRINTLN("char string array to queue");

  for (int i = 0; i < 10; i++)
  {

    queue.push(c_string_array[i]);
  }
  DEBUG_PRINTLN("1");

  for (int i = 0; i < 10; ++i)
  {
    free(c_string_array[i]);
  }
  DEBUG_PRINTLN("2");
  free(c_string_array);

  return queue;
}

// Prints C++ queue.
int printQueue(queue<string> input_queue)
{
  DEBUG_PRINTLN("Printing queue.");
  queue<string> temp = input_queue;

  while (!temp.empty())
  {
    DEBUG_PRINTLN("SMS" + String(temp.front().c_str()));
    temp.pop();
  }
  return 0;
}

/*
  void setup(){

    Serial.begin(9600);
    DEBUG_PRINTLN("Hello!");


  };

  void loop(){



  };*/
