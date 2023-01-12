#ifndef MAIN_H
#define MAIN_H

#define DEBUG
#define CPU_FREQ 80

#include "driver/gpio.h"
#include "esp_sleep.h"
#include "sms.hpp"
#include "sensors.hpp"
#include "esp_sleep.h"
#include "debug.hpp"

#define SMS_TASK_TIMEOUT 50000 // miliseconds
#define SMS_PER_HOUR 6

#define PWR_PIN 2

#endif
