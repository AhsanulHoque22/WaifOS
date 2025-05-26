#include "files_app.h"
#include <Arduino.h>
#include <SH1106Wire.h>
#include <FS.h>
#include <SPIFFS.h>
#include "input_manager.h"
#include "state_manager.h"
#include "kernel.h"

extern SH1106Wire display;
extern SemaphoreHandle_t displayMutex;

TaskHandle_t filesAppHandle = NULL;
bool filesAppShouldExit = false;

#define MAX_FILES 10
String fileList[MAX_FILES];
int fileCount = 0;
int selectedIndex = 0;
int scrollOffset = 0;
int textScrollOffset = 0;

enum Mode {
  BROWSING,
  VIEWING_TEXT,
  EDITING_TEXT
};
Mode currentMode = BROWSING;

String viewingText;
String currentFilePath;

String normalize_path(const String& path) {
  if (path.startsWith("/")) return path;
  return "/" + path;
}

void scan_files(const String& path = "/") {
  fileCount = 0;
  fileList[fileCount++] = "+ New File";

  File root = SPIFFS.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("[FILES] Failed to open root directory");
    return;
  }

  File file = root.openNextFile();
  while (file && fileCount < MAX_FILES) {
    String name = String(file.name());

    // Skip hidden/system files
    if (name.endsWith(".keep")) {
      file = root.openNextFile();
      continue;
    }

    fileList[fileCount++] = name;
    file = root.openNextFile();
  }

  Serial.printf("[FILES] Total files listed: %d\n", fileCount - 1);
}

bool load_text_file(const char* path) {
  File file = SPIFFS.open(path, "r");
  if (!file) return false;

  viewingText = "";
  while (file.available()) {
    viewingText += (char)file.read();
  }
  file.close();
  return true;
}

void render_text() {
  if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
    display.clear();
    display.setFont(ArialMT_Plain_10);

    int lineCount = 0;
    int y = 0;
    int start = 0;
    int linesToShow = 5;
    int currentLine = 0;

    for (int i = 0; i < viewingText.length(); i++) {
      if (viewingText[i] == '\n' || i == viewingText.length() - 1) {
        if (currentLine >= textScrollOffset && lineCount < linesToShow) {
          String line = viewingText.substring(start, i + 1);
          display.drawString(0, y, line);
          y += 12;
          lineCount++;
        }
        currentLine++;
        start = i + 1;
      }
    }

    display.drawString(0, 54, "e:edit UP/DOWN:scroll");
    display.display();
    xSemaphoreGive(displayMutex);
  }
}

void render_file_list() {
  if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Files:");

    for (int i = 0; i < 3; i++) {
      int idx = scrollOffset + i;
      if (idx >= fileCount) break;
      String name = fileList[idx];
      display.drawString(0, 15 + i * 12, (idx == selectedIndex ? ">" : " ") + name);
    }

    display.drawString(0, 54, "L:Clean R:Del E:Open");
    display.display();
    xSemaphoreGive(displayMutex);
  }
}

void files_app_task(void* param) {
  Serial.println("[FILES] Files App started");
  Serial.printf("[FILES TASK] Running on Core: %d\n", xPortGetCoreID());

  if (!SPIFFS.begin(true)) {
    Serial.println("[FILES] SPIFFS mount failed");
    isAppRunning = false;
    vTaskDelete(NULL);
    return;
  }

  scan_files();

  while (!filesAppShouldExit) {
    if (is_button_just_pressed(BUTTON_EXIT)) {
      if (currentMode != BROWSING) {
        currentMode = BROWSING;
        scan_files();
      } else {
        filesAppShouldExit = true;
        break;
      }
    }

    if (currentMode == BROWSING) {
      if (is_button_just_pressed(BUTTON_DOWN)) {
        selectedIndex = (selectedIndex + 1) % fileCount;
        if (selectedIndex >= scrollOffset + 3) scrollOffset++;
        vTaskDelay(pdMS_TO_TICKS(200));
      }
      if (is_button_just_pressed(BUTTON_UP)) {
        selectedIndex = (selectedIndex - 1 + fileCount) % fileCount;
        if (selectedIndex < scrollOffset) scrollOffset--;
        vTaskDelay(pdMS_TO_TICKS(200));
      }

      if (is_button_just_pressed(BUTTON_LEFT)) {
        Serial.println("[FORMAT] Formatting SPIFFS...");
        SPIFFS.format();
        scan_files();
        selectedIndex = 0;
        scrollOffset = 0;
        vTaskDelay(pdMS_TO_TICKS(500));
      }

      if (is_button_just_pressed(BUTTON_RIGHT)) {
        String selected = fileList[selectedIndex];
        if (!selected.startsWith("+")) {
          selected = normalize_path(selected);
          if (SPIFFS.remove(selected)) {
            Serial.println("[DELETE] Deleted file: " + selected);
          } else {
            Serial.println("[DELETE] Failed to delete: " + selected);
          }
          scan_files();
          selectedIndex = 0;
          scrollOffset = 0;
          vTaskDelay(pdMS_TO_TICKS(200));
        }
      }

      if (is_button_just_pressed(BUTTON_ENTER)) {
        String selectedFile = fileList[selectedIndex];

        if (selectedFile == "+ New File") {
          vTaskDelay(pdMS_TO_TICKS(250));
          while (is_button_pressed(BUTTON_ENTER)) {
            vTaskDelay(pdMS_TO_TICKS(10));
          }
          String newName = "/file_" + String(millis() / 1000) + ".txt";
          File file = SPIFFS.open(newName, "w");
          if (file) {
            file.println("New file created.");
            file.close();
            Serial.println("[FILES] File created: " + newName);
          }
          scan_files();
          continue;
        }

        if (selectedFile.endsWith(".txt")) {
          currentFilePath = normalize_path(selectedFile);
          textScrollOffset = 0;
          if (load_text_file(currentFilePath.c_str())) {
            currentMode = VIEWING_TEXT;
            Serial.println("[EDITOR] Press 'e' to edit file via Serial. End with ~");
          }
        }
      }

      render_file_list();
    }

    else if (currentMode == VIEWING_TEXT) {
      if (is_button_just_pressed(BUTTON_UP)) textScrollOffset = max(0, textScrollOffset - 1);
      if (is_button_just_pressed(BUTTON_DOWN)) textScrollOffset++;
      render_text();

      if (Serial.available()) {
        char c = Serial.read();
        if (c == 'e') {
          currentMode = EDITING_TEXT;
          Serial.println("[EDITOR] Editing mode. Type content. End with '~'.");
          viewingText = "";
        }
      }
    }

    else if (currentMode == EDITING_TEXT) {
      while (Serial.available()) {
        char c = Serial.read();

        if (c == '~') {
          File file = SPIFFS.open(normalize_path(currentFilePath), "w");
          if (file) {
            file.print(viewingText);
            file.close();
            Serial.println("[EDITOR] Saved. Content:");
            Serial.println(viewingText);
          } else {
            Serial.println("[EDITOR] Failed to open file for saving.");
          }
          currentMode = VIEWING_TEXT;
          break;
        }

        if (c == '\r' || c == '\n') {
          viewingText += '\n';
          Serial.print("\n");
          continue;
        }

        if (c == 8 || c == 127) {
          if (viewingText.length() > 0) {
            viewingText.remove(viewingText.length() - 1);
            Serial.print("\b \b");
          }
        } else {
          viewingText += c;
          Serial.print(c);
        }
      }

      render_text();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  Serial.println("[FILES] Exiting Files App");
  isAppRunning = false;
  filesAppHandle = NULL;
  vTaskDelete(NULL);
}

void files_app_run() {
  filesAppShouldExit = false;
  selectedIndex = 0;
  scrollOffset = 0;
  fileCount = 0;
  currentMode = BROWSING;

  BaseType_t result = xTaskCreatePinnedToCore(
    files_app_task,
    "FilesAppTask",
    12288,
    NULL,
    1,
    &filesAppHandle,
    1
  );

  if (result != pdPASS) {
    Serial.println("[ERROR] Failed to create Files App Task!");
  }
}
