#include "snake_game.h"
#include <SH1106Wire.h>
#include <Arduino.h>
#include "input_manager.h"
#include "state_manager.h"
#include "kernel.h"

extern SH1106Wire display;
extern SemaphoreHandle_t displayMutex;

TaskHandle_t snakeTaskHandle = NULL;
bool snakeShouldExit = false;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define CELL_SIZE 8  // Smaller snake and food
#define GRID_WIDTH (SCREEN_WIDTH / CELL_SIZE)
#define GRID_HEIGHT (SCREEN_HEIGHT / CELL_SIZE)

struct Point {
  int x;
  int y;
};

enum Direction {
  UP, DOWN, LEFT, RIGHT
};

enum GameState {
  PLAYING, GAME_OVER
};

Point snake[128];  // Max length
int snakeLength = 3;
Direction dir = RIGHT;
Point apple;
unsigned long lastMoveTime = 0;
int moveInterval = 250;
int score = 0;
GameState gameState = PLAYING;

void reset_game() {
  snakeLength = 3;
  score = 0;
  dir = RIGHT;
  gameState = PLAYING;

  snake[0] = {4, 4};
  snake[1] = {3, 4};
  snake[2] = {2, 4};

  apple = {random(0, GRID_WIDTH), random(0, GRID_HEIGHT)};
}

bool check_collision(Point head) {
  for (int i = 1; i < snakeLength; i++) {
    if (snake[i].x == head.x && snake[i].y == head.y) {
      return true;
    }
  }
  return false;
}

void move_snake() {
  Point newHead = snake[0];

  switch (dir) {
    case UP:    newHead.y--; break;
    case DOWN:  newHead.y++; break;
    case LEFT:  newHead.x--; break;
    case RIGHT: newHead.x++; break;
  }

  // Wrap-around screen
  if (newHead.x < 0) newHead.x = GRID_WIDTH - 1;
  if (newHead.x >= GRID_WIDTH) newHead.x = 0;
  if (newHead.y < 0) newHead.y = GRID_HEIGHT - 1;
  if (newHead.y >= GRID_HEIGHT) newHead.y = 0;

  if (check_collision(newHead)) {
    gameState = GAME_OVER;
    return;
  }

  // Move body
  for (int i = snakeLength; i > 0; i--) {
    snake[i] = snake[i - 1];
  }
  snake[0] = newHead;

  // Eat apple
  if (newHead.x == apple.x && newHead.y == apple.y) {
    snakeLength++;
    if (snakeLength > 127) snakeLength = 127;
    score++;
    apple = {random(0, GRID_WIDTH), random(0, GRID_HEIGHT)};
  }
}

void draw_game() {
  if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
    display.clear();

    // Draw apple (smaller)
    display.fillRect(apple.x * CELL_SIZE + 1, apple.y * CELL_SIZE + 1, CELL_SIZE - 2, CELL_SIZE - 2);

    // Draw snake
    for (int i = 0; i < snakeLength; i++) {
      display.fillRect(snake[i].x * CELL_SIZE + 1, snake[i].y * CELL_SIZE + 1, CELL_SIZE - 2, CELL_SIZE - 2);
    }

    // Draw score
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Score: " + String(score));

    display.display();
    xSemaphoreGive(displayMutex);
  }
}

void draw_game_over() {
  if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.drawString(20, 20, "Game Over");

    display.setFont(ArialMT_Plain_10);
    display.drawString(20, 45, "Score: " + String(score));
    display.drawString(0, 56, "Press ENTER to restart");

    display.display();
    xSemaphoreGive(displayMutex);
  }
}

void show_splash() {
  if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.drawString(20, 20, "Snake Game");

    display.drawRect(60, 40, 8, 8);  // Snake face
    display.fillCircle(62, 42, 1);  // Eyes
    display.fillCircle(66, 42, 1);

    display.display();
    xSemaphoreGive(displayMutex);
  }
  delay(2000);
}

void snake_game_task(void* param) {
  Serial.println("[SNAKE] Game started");
  Serial.printf("[SNAKE TASK] Running on Core: %d\n", xPortGetCoreID());

  show_splash();
  reset_game();
  lastMoveTime = millis();

  while (!snakeShouldExit) {
    if (is_button_just_pressed(BUTTON_EXIT)) {
      Serial.println("[SNAKE] EXIT pressed");
      break;
    }

    if (gameState == GAME_OVER) {
      draw_game_over();
      if (is_button_just_pressed(BUTTON_ENTER)) {
        reset_game();
      }
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    // Handle input
    if (is_button_just_pressed(BUTTON_UP) && dir != DOWN) dir = UP;
    if (is_button_just_pressed(BUTTON_DOWN) && dir != UP) dir = DOWN;
    if (is_button_just_pressed(BUTTON_LEFT) && dir != RIGHT) dir = LEFT;
    if (is_button_just_pressed(BUTTON_RIGHT) && dir != LEFT) dir = RIGHT;

    if (millis() - lastMoveTime > moveInterval) {
      move_snake();
      draw_game();
      lastMoveTime = millis();
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }

  Serial.println("[SNAKE] Exiting Snake Game");
  isAppRunning = false;
  snakeTaskHandle = NULL;
  vTaskDelete(NULL);
}

void snake_game_run() {
  snakeShouldExit = false;

  BaseType_t result = xTaskCreatePinnedToCore(
    snake_game_task,
    "SnakeGame",
    4096,
    NULL,
    1,
    &snakeTaskHandle,
    1
  );

  if (result != pdPASS) {
    Serial.println("[ERROR] Failed to create Snake Game task!");
  }
}
