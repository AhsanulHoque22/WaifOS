#include "ui_manager.h"
#include "app_manager.h"
#include "input_manager.h"
#include "state_manager.h"
#include "kernel.h"
#include <Arduino.h>
#include <SH1106Wire.h>
#include <time.h>
#include <TimeLib.h>

extern SH1106Wire display;
extern TaskHandle_t appTaskHandle;

void ui_init() {
  display.init();
  display.flipScreenVertically();
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(10, 20, "WaifOS Booting...");
  display.display();
  delay(2000);
}

void ui_task(void *pvParameters) {
  setTime(12, 0, 0, 1, 1, 2025); // Optional static time
  unsigned long lastHeartbeat = 0;

  while (true) {
    // 🔄 Log heartbeat every 3 seconds to confirm UI task is alive
    if (millis() - lastHeartbeat > 3000) {
      if (isAppRunning) {
        Serial.println("[UI TASK] Skipped (App is running)");
      } else {
        Serial.println("[UI TASK] Running (UI active)");
      }
      lastHeartbeat = millis();
    }
    // Skip UI update while app is running
    if (isAppRunning) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      display.clear();

      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 0, "App Menu:");

      display.setFont(ArialMT_Plain_10);
      for (int i = app_scrollOffset; i < app_scrollOffset + 3; i++) {
        if (i >= appCount) break;
        int y = 20 + (i - app_scrollOffset) * 12;
        if (i == app_selectedApp) {
          display.drawString(0, y, ">");
        } else {
          display.drawString(0, y, " ");
        }
        display.drawString(10, y, apps[i].name);
      }

      // Clock overlay
      //char timeStr[9];
      //sprintf(timeStr, "%02d:%02d:%02d", hour(), minute(), second());
      //display.drawString(80, 0, timeStr);

      display.display();

      xSemaphoreGive(displayMutex);
    } else {
      Serial.println("[UI TASK] Failed to acquire display mutex");
    }

    // Button navigation handled in app_manager_run()
    app_manager_run();

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
