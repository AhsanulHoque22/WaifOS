#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

// Pin assignments for each button
#define BTN_UP     32
#define BTN_DOWN   33
#define BTN_LEFT   25
#define BTN_RIGHT  26
#define BTN_ENTER  27
#define BTN_EXIT   14

// Button enumeration for easier indexing
enum Button {
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_LEFT,
  BUTTON_RIGHT,
  BUTTON_ENTER,
  BUTTON_EXIT,
  BUTTON_COUNT
};

// Initializes button GPIOs
void input_init();

// Returns true if the given button is currently pressed
bool is_button_pressed(Button btn);

// Returns true if a button has just been pressed (edge-triggered)
bool is_button_just_pressed(Button btn);

#endif
