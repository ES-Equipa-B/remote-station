#ifndef MAIN_H
#define MAIN_H

#include "driver/gpio.h"
#include "esp_sleep.h"
#include "sms.hpp"
#include "sensors.hpp"
#include "esp_sleep.h"

#define SMS_TASK_TIMEOUT 55000 // miliseconds
#define SMS_PER_HOUR 20

#define PWR_PIN 2

#endif
