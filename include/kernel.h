#ifndef KERNEL_H
#define KERNEL_H

#include <Arduino.h>

// Forward declaration of App structure
struct App;

// Global task handles
extern TaskHandle_t appTaskHandle;
extern TaskHandle_t uiTaskHandle;

// Launch an app in its own task
void launch_app_task(App *app);

// Kernel system lifecycle
void kernel_init();
void kernel_run();

#endif
