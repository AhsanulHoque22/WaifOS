#include "weather_app.h"
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SH1106Wire.h>
#include "input_manager.h"
#include "state_manager.h"
#include "kernel.h"

extern SH1106Wire display;
extern SemaphoreHandle_t displayMutex;

TaskHandle_t weatherAppHandle = NULL;
bool weatherAppShouldExit = false;

const char* CITY = "Chittagong";
const char* API_KEY = "9b82977acb2a0288f1b93675fe8802ec";  // 🔑 Insert your key
String WEATHER_API = "http://api.openweathermap.org/data/2.5/weather?lat=22.3569&lon=91.7832&units=metric&appid=" + String(API_KEY);

String condition = "--";
float temp = 0;
int humidity = 0;
String dateStr = "--";

void show_weather_splash() {
  if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.drawString(20, 5, "Weather App");

    // ☁️ Draw simple cloud
    display.drawCircle(40, 32, 10);
    display.drawCircle(55, 28, 10);
    display.drawCircle(65, 34, 8);
    display.fillRect(40, 34, 25, 10);

    display.display();
    xSemaphoreGive(displayMutex);
  }

  delay(2000);  // Show splash for 2s
}

void fetch_weather() {
  HTTPClient http;
  http.begin(WEATHER_API);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      condition = doc["weather"][0]["main"].as<String>();
      temp = doc["main"]["temp"].as<float>();
      humidity = doc["main"]["humidity"].as<int>();

      time_t now = time(nullptr);
      struct tm* timeinfo = localtime(&now);
      char buffer[20];
      sprintf(buffer, "%02d-%02d-%04d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);
      dateStr = String(buffer);
    } else {
      Serial.println("[WEATHER] JSON parsing failed.");
    }
  } else {
    Serial.printf("[WEATHER] HTTP GET failed: %d\n", httpCode);
  }

  http.end();
}

void weather_app_task(void* param) {
  Serial.println("[WEATHER] App started");
  Serial.printf("[WEATHER TASK] Running on Core: %d\n", xPortGetCoreID());

  show_weather_splash();
  fetch_weather();

  while (!weatherAppShouldExit) {
    if (is_button_just_pressed(BUTTON_EXIT)) {
      weatherAppShouldExit = true;
      break;
    }

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "Weather: Chittagong");

      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 15, "Condition: " + condition);
      display.drawString(0, 27, "Temp: " + String(temp, 1) + " C");
      display.drawString(0, 39, "Humidity: " + String(humidity) + "%");
      display.drawString(0, 51, "Date: " + dateStr);

      display.display();
      xSemaphoreGive(displayMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }

  Serial.println("[WEATHER] Exiting app...");
  isAppRunning = false;
  weatherAppHandle = NULL;
  vTaskDelete(NULL);
}

void weather_app_run() {
  weatherAppShouldExit = false;

  BaseType_t result = xTaskCreatePinnedToCore(
    weather_app_task,
    "WeatherAppTask",
    8192,
    NULL,
    1,
    &weatherAppHandle,
    1
  );

  if (result != pdPASS) {
    Serial.println("[ERROR] Failed to create Weather App Task!");
  }
}
