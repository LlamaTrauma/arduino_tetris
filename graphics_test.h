#include "processes.h"
#include "sprites/test_sprite.h"

bool test_rect_req_factory () {
  static uint16_t x = 0;
  static uint16_t y = 0;
  static uint16_t c = 0;
  requestRect(x, y, 20, 20, c);
  c += 0x11;
  x += 20;
  if (x >= TFT_WIDTH) {
    x = 0;
    y += 20;
    if (y >= TFT_HEIGHT) {
      y = 0;
    }
  }
  return 0;
}

bool test_sprite_req_factory () {
  static uint16_t x = 0;
  static uint16_t y = 0;
  static uint8_t offset = 0;
  requestSprite(LUIGI_SPRITE, x, y);
  x += 62;
  if (x + 52 >= TFT_WIDTH) {
    y += 120;
    if (y >= TFT_HEIGHT) {
      y = 0;
      offset = offset == 0 ? 10 : 0;
    }
    x = offset;
  }
  return 0;
}

void graphicsTest () {
  addReqFactory(test_rect_req_factory);
}

void spriteTest () {
  addReqFactory(test_sprite_req_factory);
}

void floodContinually() {
  while (1) {
    tft.fillScreen((uint16_t) random());
  }
}

void rectContinually() {
  while (1) {
    tft.fillRect(100, 100, 5, 5, (uint16_t) random());
  }
}

void printScanline (process_t* p, time_t deadline) {
  Serial.println(tft.getScanline());
  // Serial.println(2);
  // p->next_run += 10;
}

void scanlineTest () {
  createProcessWithName(printScanline, 124, WHENEVER, 1, "pscn");
}