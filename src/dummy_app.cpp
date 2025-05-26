#include "dummy_app.h"
#include <SH1106Wire.h>
#include <Arduino.h>
#include "input_manager.h"
#include "state_manager.h"
#include "kernel.h"

extern SH1106Wire display;
extern TaskHandle_t uiTaskHandle;
extern SemaphoreHandle_t displayMutex;

TaskHandle_t dummyAppHandle = NULL;
bool dummyAppShouldExit = false;

void dummy_app_task(void* param) {
  Serial.println("[DUMMY APP] App started");
  Serial.printf("[DUMMY APP TASK] Running on Core: %d\n", xPortGetCoreID());


  unsigned long startTime = millis();

  while (!dummyAppShouldExit) {
    // Exit on button press
    if (is_button_just_pressed(BUTTON_EXIT)) {
      Serial.println("[DUMMY APP] EXIT button pressed");
      dummyAppShouldExit = true;
      break;
    }

    unsigned long elapsed = (millis() - startTime) / 1000;

    // Use display mutex to safely draw
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "Dummy App Running");
      display.drawString(0, 20, "Time: " + String(elapsed) + "s");
      display.drawString(0, 50, "EXIT: Back to Menu");
      display.display();
      xSemaphoreGive(displayMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(100));  // Let CPU breathe
  }

  Serial.println("[DUMMY APP] Exiting app...");
  isAppRunning = false;
  dummyAppHandle = NULL;
  vTaskDelete(NULL);
}

void dummy_app_run() {
  dummyAppShouldExit = false;

  BaseType_t result = xTaskCreatePinnedToCore(
    dummy_app_task,
    "DummyAppTask",
    4096,
    NULL,
    1,
    &dummyAppHandle,
    1  // Run on Core 1
  );

  if (result != pdPASS) {
    Serial.println("[ERROR] Failed to create Dummy App Task!");
  }
}
