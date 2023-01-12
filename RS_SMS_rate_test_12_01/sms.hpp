#ifndef SMS_H
#define SMS_H

#include <Wire.h>
#include <Arduino.h>
#include <time.h>
#include <queue>
#include <deque>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <cctype>
#include <algorithm>
using namespace std;

// Define timezone

    #define TZ_INFO "WET0WEST,M3.5.0/1,M10.5.0"

// Serial

    // Set serial for debug console (to Serial Monitor, default speed 115200)
    #define SerialMon Serial
    // Set serial for AT commands (to SIM800 module)
    #define SerialAT Serial1

// TTGO T-call Config

    // TTGO T-Call pins
    #define MODEM_RST 5
    #define MODEM_PWKEY 4
    #define MODEM_POWER_ON 23
    #define MODEM_TX 27
    #define MODEM_RX 26
    #define I2C_SDA 21
    #define I2C_SCL 22

// Configure TinyGSM library

    #define TINY_GSM_MODEM_SIM800   // Modem is SIM800
    #define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb

    #include <TinyGsmClient.h>

    // SIM card PIN (leave empty, if not defined)
    const char simPIN[] = "";

    // Your phone number to send SMS: + (plus sign) and country code, for Portugal +351, followed by phone number
    // SMS_TARGET Example for Portugal +351XXXXXXXXX
    // Main Station Default number
    #define SMS_TARGET_TEST "+351915784796" // test galaxy S4
    //#define SMS_TARGET "+351921692157"  77 campanha
    // 
    #define SMS_TARGET "+351913816269" // joao
    //#define SMS_TARGET "+351915784796"
    #define IP5306_ADDR 0x75
    #define IP5306_REG_SYS_CTL0 0x00

// C++ circular buffer template using queue.h
    template <typename T, int MaxLen, typename Container=std::deque<T>>
    class FixedQueue : public std::queue<T, Container> {
    public:
        void push(const T& value) {
            if (this->size() == MaxLen) {
            this->c.pop_front();
            }
            std::queue<T, Container>::push(value);
        }
    };

// Sim800l

    // Set Sim800l power boost.
    int setPowerBoostKeepOn(int en);
    // Wake up Sim800l
    int wakeUpGsm();
    // Shutdown Sim800l
    void shutdownGsm();
    // Send SMS containing msg.
    int sendSms(string msg);
    // Send Cnt SMS
    int sendCntSMS(int &sms_sent_cnt);
    
// Sample encoding

    // Encode sample with sensor readings and timestamp.
    string encodeSample(string timestamp,float wind_speed, float hum, float temp);
    // Test EncodeSample().
    void test_encodeSample();

// Time

    // Print ESP local time (RTC).
    void printLocalTime();

    // Returns timestamp string in format HHmm from local ESP time.
    string getLocalHourMinutes();

    // Syncs ESP local time (RTC) with network time.
    int syncRtcWithNetworkTime(bool first_exec);

    // Test syncRtcWithNetworkTime
    int test_syncRtcWithNetworkTime();

    bool compareTime(std::string t_now, std::string t_lastSync);

// Send SMS task

    // Send all SMS in buffer until it is empty.
    int sendAllSmsInBuffer(queue<string>& buffer);
    int sendAllSmsInBuffer_sms_test(queue<string> &buffer, int &lastSync, int &sms_sent_cnt, int &sync_min);
    
    
    int send_sms_test(int &sms_sent_cnt);
     
    // Test sendAllSmsInBuffer().
    int test_sendAllSmsInBuffer();


// Aux

    // Convert C++ queue to C string array.
    char** queueToCStringArray(FixedQueue<string,10> queue);

    // Prints C string array.
    int printCStringArray(char** c_string_array);

    // Convert C string array to C++ queue.
    FixedQueue<string,10> cStringArrayToQueue(char** c_string_array);

    // Prints C++ queue.
    int printQueue(queue<string> queue);

#endif
