#include "input_manager.h"

static const int buttonPins[BUTTON_COUNT] = {
  BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_ENTER, BTN_EXIT
};

static bool lastStates[BUTTON_COUNT] = {false};
static bool justPressed[BUTTON_COUNT] = {false};

void input_init() {
  for (int i = 0; i < BUTTON_COUNT; ++i) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastStates[i] = true;       // Pull-ups mean HIGH when not pressed
    justPressed[i] = false;
  }
}

// Check current physical state (LOW = pressed)
bool is_button_pressed(Button btn) {
  return digitalRead(buttonPins[btn]) == LOW;
}

// Detect edge-triggered press (LOW transition)
bool is_button_just_pressed(Button btn) {
  bool current = digitalRead(buttonPins[btn]) == LOW;
  bool pressed = false;

  if (current && lastStates[btn] == true) {
    pressed = true;
    Serial.print("[DEBUG] Button pressed: ");
    Serial.println(btn);
  }

  lastStates[btn] = current;
  return pressed;
}