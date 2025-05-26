#include "app_manager.h"
#include "kernel.h"
#include "snake_game.h"
#include "dummy_app.h"
#include "task_manager_app.h"
#include "input_manager.h"
#include "weather_app.h"
#include "state_manager.h"
#include "files_app.h"
#include <time.h>
#include <SH1106Wire.h>
#include <Arduino.h>

extern SH1106Wire display;

App apps[] = {
  {"Dummy App", dummy_app_run},
  {"Snake Game", snake_game_run},
  {"Task Manager", task_manager_app_run},
  {"Weather", weather_app_run},
  {"Files", files_app_run}
};

const int appCount = sizeof(apps) / sizeof(App);

int app_selectedApp = 0;
int app_scrollOffset = 0;
const int itemsPerScreen = 3;

void app_manager_init() {
  Serial.println("[INFO] App Manager Initialized");
}

void app_manager_run() {
  Serial.printf("[DEBUG] displayMutex = %p\n", displayMutex);
  if (displayMutex == NULL) {
    Serial.println("[ERROR] displayMutex is NULL!");
    return;
  }

  if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "App Menu:");
    display.setFont(ArialMT_Plain_10);

    // Show current time
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    display.drawString(80, 0, timeStr);

    for (int i = app_scrollOffset; i < app_scrollOffset + itemsPerScreen; i++) {
      if (i >= appCount) break;

      int y = 20 + (i - app_scrollOffset) * 12;
      display.setFont(ArialMT_Plain_10);

      if (i == app_selectedApp) {
        display.drawString(0, y, ">");
      } else {
        display.drawString(0, y, " ");
      }

      display.drawString(10, y, apps[i].name);
    }

    display.display();
    xSemaphoreGive(displayMutex);
  } else {
    Serial.println("[WARN] Could not take display mutex");
  }

  // Navigation: UP / DOWN
  if (is_button_just_pressed(BUTTON_DOWN)) {
    app_selectedApp = (app_selectedApp + 1) % appCount;
    if (app_selectedApp >= app_scrollOffset + itemsPerScreen) {
      app_scrollOffset = app_selectedApp - itemsPerScreen + 1;
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }

  if (is_button_just_pressed(BUTTON_UP)) {
    app_selectedApp = (app_selectedApp - 1 + appCount) % appCount;
    if (app_selectedApp < app_scrollOffset) {
      app_scrollOffset = app_selectedApp;
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }

  // Launch app: ENTER
  if (is_button_just_pressed(BUTTON_ENTER)) {
    if (app_selectedApp < 0 || app_selectedApp >= appCount) {
      Serial.println("[ERROR] Invalid app_selectedApp index");
    } else {
      App *app = &apps[app_selectedApp];
      if (app == NULL || app->launch == NULL) {
        Serial.println("[ERROR] App or launch function pointer is NULL");
      } else {
        launch_app_task(app);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }

  vTaskDelay(pdMS_TO_TICKS(100));  // Let FreeRTOS breathe
}
