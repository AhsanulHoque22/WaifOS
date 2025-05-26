#ifndef APP_MANAGER_H
#define APP_MANAGER_H

struct App {
  const char *name;
  void (*launch)();
};

extern App apps[];
extern const int appCount;

extern int app_selectedApp;
extern int app_scrollOffset;

void app_manager_init();
void app_manager_run();

#endif
