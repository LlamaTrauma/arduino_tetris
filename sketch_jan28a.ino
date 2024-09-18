// REFACTORS TODO
// DONE Switch from using first_falling and first_clearing to proper state machines w/ switch statements
// Clean up draw_tetromino, use a switch statement too
// DONE Maybe switch all functions to assume they're using the global game variable?
// Make a sane method for requesting drawing strings
#include <Arduino.h>
#include <stdio.h>
#include <stdint.h>
#include <EEPROM.h>
#include "random.h"
#include "processes.h"
// #include "process_test.h"
#include "graphics.h"
// #include "graphics_test.h"
#include "audio.h"
#include "input.h"
#include "menu.h"
#include "tetris.h"

uint32_t numSamples = 0;
uint32_t lastPrint = 0;

void setup() {
  Serial.begin(115200);
  while(!Serial);
  initInput();
  initGraphics();
  initMenu();
  // testInput();
  loadMainMenu();
}

void loop() {
  runProcesses();
}