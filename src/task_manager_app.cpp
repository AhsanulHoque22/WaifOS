#include "task_manager_app.h"
#include <SH1106Wire.h>
#include <Arduino.h>
#include "input_manager.h"
#include "state_manager.h"
#include "kernel.h"

extern SH1106Wire display;
extern TaskHandle_t appTaskHandle;
extern TaskHandle_t uiTaskHandle;
extern SemaphoreHandle_t displayMutex;

TaskHandle_t taskManagerHandle = NULL;
bool taskManagerShouldExit = false;

void task_manager_app_task(void *param) {
  Serial.println("[TASK MANAGER] Task Manager App started");
  Serial.printf("[TASK MANAGER APP TASK] Running on Core: %d\n", xPortGetCoreID());

  unsigned long lastUpdate = 0;

  while (!taskManagerShouldExit) {
    // Exit on button press
    if (is_button_just_pressed(BUTTON_EXIT)) {
      taskManagerShouldExit = true;
      break;
    }

    if (millis() - lastUpdate > 500) {
      lastUpdate = millis();

      if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
        display.clear();
        display.setFont(ArialMT_Plain_10);
        display.drawString(0, 0, "Task Manager");

        // UI Task Status
        display.drawString(0, 15, "UI Task: RUNNING");

        // App Task Status
        if (appTaskHandle != NULL) {
          display.drawString(0, 30, "App Task: RUNNING");
        } else {
          display.drawString(0, 30, "App Task: NONE");
        }

        display.drawString(0, 50, "EXIT: Back to Menu");

        display.display();
        xSemaphoreGive(displayMutex);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(50));  // Yield CPU
  }

  Serial.println("[Task Manager] Exiting App");
  isAppRunning = false;
  taskManagerHandle = NULL;
  vTaskDelete(NULL);
}

void task_manager_app_run() {
  taskManagerShouldExit = false;

  BaseType_t result = xTaskCreatePinnedToCore(
    task_manager_app_task,
    "TaskManagerApp",
    4096,
    NULL,
    1,
    &taskManagerHandle,
    1  // Run on Core 1
  );

  if (result != pdPASS) {
    Serial.println("[ERROR] Failed to create Task Manager App Task!");
  }
}
