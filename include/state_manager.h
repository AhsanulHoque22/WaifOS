#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include <freertos/semphr.h>

extern volatile bool isAppRunning;
extern SemaphoreHandle_t displayMutex;

#endif
