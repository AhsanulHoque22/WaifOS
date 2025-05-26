#include "splash_screen.h"
#include <SH1106Wire.h>
#include <Arduino.h>

extern SH1106Wire display;

void show_splash_screen() {
  display.clear();
  display.setFont(ArialMT_Plain_16);

  // Fade-in text effect
  for (int y = 64; y >= 20; y -= 2) {
    display.clear();
    display.drawString(10, y, "WaifOS");
    display.setFont(ArialMT_Plain_10);
    display.drawString(10, y + 20, "Smart Gadget OS");
    display.display();
    delay(50);
  }

  // Simple stars animation (mock space theme)
  for (int i = 0; i < 30; i++) {
    display.setPixel(random(0, 128), random(0, 64));
  }
  display.display();
  delay(1500);

  // Clear for menu handover
  display.clear();
  display.display();
}
