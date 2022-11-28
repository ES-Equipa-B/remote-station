#include "sms.hpp"
#include <Arduino.h>
#include <queue>
#include <deque>
#include <string>
#include <vector>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>


RTC_DATA_ATTR TinyGsm modem(SerialAT);

// Sim800l

    // Set Sim800l power boost.
    int setPowerBoostKeepOn(int en){
        SerialMon.println("Hi from setPowerBoostKeepOn !");

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
    int wakeUpGsm(){
        SerialMon.println("GSM is waking up.");
        
        Wire.begin(I2C_SDA, I2C_SCL);

        if(setPowerBoostKeepOn(0)==0){
            SerialMon.println(String("IP5306 KeepOn OK"));
        }
        else{
            SerialMon.println(String("IP5306 KeepOn failed."));
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

        SerialMon.println("Initializing modem.");
        modem.init();
        // use modem.init() if you don't need the complete restart


        // Unlock your SIM card with a PIN if needed
        if (strlen(simPIN) && modem.getSimStatus() != 3)
        {
            modem.simUnlock(simPIN);
        }

        if(modem.waitForNetwork()){
            SerialMon.println("Connected to network.");
            return 0;
        }
        else
        {
            SerialMon.println("Failed to connect to network.");
            return -2;
        }

    }

    // Shutdown Sim800l
    void shutdownGsm(){

        SerialMon.println("Shutting down GSM.");

        digitalWrite(MODEM_PWKEY, HIGH);
        delay(50);
        digitalWrite(MODEM_PWKEY, LOW);
        delay(1550);
        digitalWrite(MODEM_PWKEY, HIGH);
    }

    // Send SMS containing msg.
    int sendSms(string msg){

        while (!modem.sendSMS(SMS_TARGET, String(msg.c_str())))
        {
            SerialMon.println("SMS failed to send");
            delay(500);
        }
        SerialMon.println("SMS successfully sent.");
        return 0;
    }

// Sample encoding
    // Encode sample with sensor readings and timestamp. 
    string encodeSample(string timestamp,float wind_speed, float hum, float temp){
        string encodedSample;

        string wind_speed_s = to_string(ceil(wind_speed*100.0)/100.0).erase(3, 1).substr(0,4);
        //Serial.println(wind_speed_s.c_str());

        string hum_s = to_string(hum).erase(3, 1).substr(0,3);
        //Serial.println(hum_s.c_str());
        string temp_s;
        if(temp<0)
            temp_s = "1" + to_string(ceil(temp*100.0)/100.0).erase(0, 1).erase(2, 1).substr(0,4);
        else
            temp_s = "0" + to_string(ceil(temp*100.0)/100.0).erase(2, 1).substr(0,4);
        //Serial.println(temp_s.c_str());

        encodedSample = timestamp + wind_speed_s + hum_s + temp_s;

        return encodedSample;
    }

    // Test EncodeSample().
    void test_encodeSample(){
        delay(2000);
        string time;
        float temp, hum, wind_speed;
        FixedQueue<string,10> sms_buffer;

        time="HHmm";
        wind_speed=222.2;
        
        hum=100;
        temp=11.11;

        Serial.println(encodeSample(time, wind_speed, hum, temp).c_str());

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
    int sendAllSmsInBuffer(queue<string> &buffer){

        while (!buffer.empty()){
            Serial.print("Sending: ");
            Serial.print(String(buffer.front().c_str())+"\n");
            Serial.println("");

            wakeUpGsm();

            if(sendSms(buffer.front())==0){
                buffer.pop();
                shutdownGsm();
            }
            
        }

    return 0;
    }

    // Test sendAllSmsInBuffer().
    int test_sendAllSmsInBuffer(){

        FixedQueue<string,10> sms_buffer;

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
    void printLocalTime() {
    struct tm now;
    getLocalTime(&now,0);
    Serial.println(&now, "%B %d %Y %H:%M:%S (%A)");
    }

    // Returns timestamp string in format HHmm from local ESP time.
    string getLocalHourMinutes() {
        string hhmm;
        struct tm now;
        getLocalTime(&now,0);
        ostringstream oss;
        oss << std::put_time(&now, "%H%M");
        return oss.str();

    }

    // Syncs ESP local time (RTC) with network time.
    int syncRtcWithNetworkTime(){

        int yr, month, mday, hr, minute, sec, isDst;
        setenv("TZ", TZ_INFO, 1);
        tzset(); // Assign the local timezone from setenv

        if(!modem.waitForNetwork(5000)) wakeUpGsm();
        delay(5000);

        string gsm_datetime=modem.getGSMDateTime(DATE_FULL).c_str();
        Serial.printf("GSM time: %s",gsm_datetime.c_str());


        yr=stoi(gsm_datetime.substr(0,2));
        month=stoi(gsm_datetime.substr(3,2));
        mday=stoi(gsm_datetime.substr(6,2));
        hr=stoi(gsm_datetime.substr(9,2));
        minute=stoi(gsm_datetime.substr(12,2));
        sec=stoi(gsm_datetime.substr(15,2));
        isDst=stoi(gsm_datetime.substr(18,2));


        Serial.printf("%d %d %d %d %d %d %d\n",yr, month, mday, hr, minute, sec, isDst);

        struct tm tm;

        tm.tm_year = yr +100;  //(2000+yy-1900) // Set date
        tm.tm_mon = month-1;
        tm.tm_mday = mday;
        tm.tm_hour = hr;      // Set time
        tm.tm_min = minute;
        tm.tm_sec = sec;
        tm.tm_isdst = isDst;  // 1 or 0

        time_t t = mktime(&tm);
        Serial.printf("Setting time: %s\n", asctime(&tm));
        struct timeval now = { .tv_sec = t };
        settimeofday(&now, NULL); 

        return 0;

    }

    // Test syncRtcWithNetworkTime
    int test_syncRtcWithNetworkTime(){

        printLocalTime();
        syncRtcWithNetworkTime();
        printLocalTime();

        return 0;

    }

// Aux
    // Convert C++ queue to C string array.
    char** queueToCStringArray(FixedQueue<string,10> queue){

        char ** c_string_array= (char**)malloc(10 * sizeof(char*));
        
        for (int i =0 ; i < 10; ++i)
            c_string_array[i] = (char*)malloc(160 * sizeof(char));

        Serial.println("queue to char string array");

        while (!queue.empty()){

            strcpy(c_string_array[10-queue.size()],queue.front().c_str());
            queue.pop();
        }

        
        return c_string_array;

    }

    // Prints C string array.
    int printCStringArray(char**  c_string_array){

        for(int i=0; i<10;i++){

            Serial.println(c_string_array[i]);
        }

        return 0;

    }

    // Convert C string array to C++ queue.
    FixedQueue<string,10> cStringArrayToQueue(char** c_string_array){

        FixedQueue<string,10> queue;

        Serial.println("char string array to queue");

        for(int i=0; i<10;i++){

            queue.push(c_string_array[i]);
            
        }
        Serial.println("1");

        for (int i =0 ; i < 10; ++i){
            free(c_string_array[i]);
        }
        Serial.println("2");
        free(c_string_array);

        return queue;
    }

    // Prints C++ queue.
    int printQueue(queue<string> input_queue){
        Serial.println("Printing queue.");
        queue<string> temp=input_queue;
        
        while (!temp.empty())
        {
            Serial.println("SMS" + String(temp.front().c_str()));
            temp.pop();
        }
        return 0;
    }

/*
void setup(){

    Serial.begin(9600);
    Serial.println("Hello!");


};

void loop(){

    

};*/