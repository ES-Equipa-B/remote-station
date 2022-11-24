# Remote Station repository
This repository will have the .ino code for LilyGo-T-Call-SIM800.

### Packages
- parallel-tasks: first sketch for parallel processing in the ESP32, taking advantage of it's 2 core CPU.
Which core will have diffent tasks, task1 reading data from sensor and task2 sending SMS.
- debug_option: small example code for testing ESP deep_sleep mode as well the sendSMS function that belongs to TinnyGSM library.
It has a flag that allows turn off the Serial Monitor, what is usefull when running on battery.
- read_sms_Adafruit_FONA: setup routine that reads an sms in order to configure the specifications for the next tasks. 
It stores this configurations in the nvs memory and only does this routine one time.
