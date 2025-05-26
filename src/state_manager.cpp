#include "state_manager.h"

volatile bool isAppRunning = false;
SemaphoreHandle_t displayMutex = NULL;
