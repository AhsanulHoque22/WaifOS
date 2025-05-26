#include "bt_keyboard_manager.h"
#include <BleKeyboard.h>
#include <Arduino.h>

BleKeyboard bleKeyboard("WaifOS Keyboard", "WaifOS", 100);

void bt_keyboard_init() {
  bleKeyboard.begin();
  Serial.println("[INFO] BLE Keyboard ready (for pairing)");
}
