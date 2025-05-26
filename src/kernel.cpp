#include "kernel.h"
#include "ui_manager.h"
#include "app_manager.h"
#include "input_manager.h"
#include "state_manager.h"
#include <Arduino.h>

// Forward declaration BEFORE use!
void app_task_wrapper(void *param);

// Task handlesrwtet
TaskHandle_t appTaskHandle = NULL;
extern TaskHandle_t uiTaskHandle;

void launch_app_task(App *app) {
  if (!app) {
    Serial.println("[ERROR] launch_app_task received NULL app pointer");
    return;
  }
  if (!app->launch) {
    Serial.printf("[ERROR] app '%s' has NULL launch pointer\n", app->name);
    return;
  }

  // Check if the task handle is valid
  if (appTaskHandle != NULL) {
    eTaskState state = eTaskGetState(appTaskHandle);
    if (state != eDeleted && state != eInvalid) {
      Serial.printf("[KERNEL] Deleting app task handle: %p\n", appTaskHandle);
      vTaskDelete(appTaskHandle);
    } else {
      Serial.println("[KERNEL] Skipping vTaskDelete(appTaskHandle) because it's the current task");
    }
    appTaskHandle = NULL;
    delay(10);
  }

  Serial.printf("[KERNEL] Creating app task for %s\n", app->name);
  Serial.printf("[DEBUG] Free heap before creating app task: %d bytes\n", ESP.getFreeHeap());
  
  isAppRunning = true;  // Signal UI to pause itself

  Serial.println("[DEBUG] Attempting to create app task...");
  BaseType_t result = xTaskCreatePinnedToCore(app_task_wrapper, "App Task", 8192, app, 1, &appTaskHandle, 1);
  
  if (result != pdPASS) {
    Serial.println("[ERROR] Failed to create App Task!");
    isAppRunning = false;  // Recover UI
    appTaskHandle = NULL;
    return;
  } else {
    Serial.printf("[KERNEL] App Task created on Core %d\n", xPortGetCoreID());
  }

}


void app_task_wrapper(void *param) {
  Serial.println("[APP WRAPPER] Starting...");

  App *app = (App*)param;

  // 🔍 Validate app pointer before doing anything
  if (!app || !app->launch) {
    Serial.println("[APP WRAPPER] Invalid app pointer");
    isAppRunning = false;         // ✅ Ensure UI can run again
    appTaskHandle = NULL;
    vTaskDelete(NULL);
    return;
  }

  Serial.printf("[APP WRAPPER] Launching app: %s\n", app->name);

  isAppRunning = true;            // ✅ Soft-lock UI

  // 🔁 Run the app — this is usually a task creator
  app->launch();

  Serial.printf("[APP WRAPPER] App %s exited\n", app->name);

  //isAppRunning = false;           // ✅ Let UI resume naturally
  appTaskHandle = NULL;

  vTaskDelete(NULL);              // ✅ Clean up this wrapper task
}



void kernel_init() {
  input_init();
  app_manager_init();

  if (displayMutex == NULL) {
    displayMutex = xSemaphoreCreateMutex();
    if (displayMutex == NULL) {
      Serial.println("[ERROR] Failed to create display mutex!");
      // Handle this error accordingly (halt or retry)
    }
  }
}

void kernel_run() {
  // Nothing needed
}
