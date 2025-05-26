#include <Arduino.h>
#include <SH1106Wire.h>
#include <WiFi.h>               // Include WiFi library
#include "kernel.h"
#include "splash_screen.h"
#include "ui_manager.h"
#include "state_manager.h"
#include <WiFiType.h>

SH1106Wire display(0x3C, 4, 15);

TaskHandle_t uiTaskHandle = NULL;

// Your WiFi credentials here
const char* ssid = "Ahsan";
const char* password = "01820223007";

void initTime() {
  const char* ntpServer = "pool.ntp.org";
  const long gmtOffset_sec = 21600;           // Adjust timezone offset
  const int daylightOffset_sec = 0;        // Adjust daylight saving offset

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println("Waiting for NTP time sync...");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("\nNTP time synced!");

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.printf("Current time: %02d:%02d:%02d %02d-%02d-%04d\n", 
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
    timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
}


void setup() {
  Serial.begin(115200);

  Serial.printf("Connecting to WiFi SSID: %s\n", ssid);
  WiFi.mode(WIFI_STA);

  char ssidBuf[32];
  char passwordBuf[64];
  strncpy(ssidBuf, ssid, sizeof(ssidBuf));
  ssidBuf[sizeof(ssidBuf) - 1] = '\0';
  strncpy(passwordBuf, password, sizeof(passwordBuf));
  passwordBuf[sizeof(passwordBuf) - 1] = '\0';

  WiFi.begin(ssidBuf, passwordBuf);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed.");
  }

  initTime();
  ui_init();
  show_splash_screen();
  kernel_init();

  if (displayMutex == NULL) {
    displayMutex = xSemaphoreCreateMutex();
  }

  BaseType_t result = xTaskCreatePinnedToCore(
    ui_task, "UI Task", 4096, NULL, 1, &uiTaskHandle, 0
  );

  if (result != pdPASS) {
    Serial.println("[ERROR] Failed to create UI Task!");
  } else {
    Serial.printf("[MAIN] UI Task created on Core %d\n", xPortGetCoreID());
  }

  Serial.println("[MAIN] Setup complete");
}


void loop() {
  if (uiTaskHandle != NULL) {
    eTaskState uiState = eTaskGetState(uiTaskHandle);
    Serial.printf("[LOOP] UI Task state: %d\n", uiState); // 1=Ready, 2=Running, etc.
  } else {
    Serial.println("[LOOP] UI Task handle is NULL!");
  }

  delay(2000);
}


