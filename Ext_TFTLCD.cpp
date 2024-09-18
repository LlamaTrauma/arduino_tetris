#include "Ext_TFTLCD.h"
#include "pin_magic.h"
#include "pins_arduino.h"
#include "wiring_private.h"

void Ext_TFTLCD::pushColorsFromPRGMEM(uint16_t* addr, uint16_t n) {
  uint16_t color;
  CS_ACTIVE;

  CD_COMMAND;
  write8(0x2C);
  CD_DATA;

  while (n) {
    color = pgm_read_word(addr);
    write8(color >> 8);
    write8(color);
    addr++;
    n--;
  }

  CS_IDLE;
}

// Write a color with the same high and low byte to n consecutive pixels
void Ext_TFTLCD::pushEfficientColor(uint8_t color, uint8_t count, bool begin = 0) {
  CS_ACTIVE;

  CD_COMMAND;
  write8(begin ? 0x2C : 0x3C);
  CD_DATA;

  write8(color);
  WR_STROBE;

  while (--count) {
    WR_STROBE;
    WR_STROBE;
  }

  CS_IDLE;
}

// Doesn't need to be flexible, going to hardcode values
void Ext_TFTLCD::initFramerate() {
  // return;
  uint16_t color;
  CS_ACTIVE;

  CD_COMMAND;
  write8(0xB1);
  CD_DATA;
  // // set divisor to 2
  // write8(0b00000001);
  // // and undivided frame rate to 61
  // write8(0b00011111);
  write8(0b00010000);

  CS_IDLE;
}

uint16_t Ext_TFTLCD::getScanline() {
  uint16_t scanline = 0;
  uint8_t x;

  CS_ACTIVE;

  CD_COMMAND;
  write8(0x45);
  CD_DATA;
  setReadDir();
  delayMicroseconds(3);

  read8fn();
  x = read8fn();
  scanline = (x & 0b11) << 8;
  x = read8fn();
  scanline += x;
  setWriteDir();

  CS_IDLE;

  return scanline;
}

void Ext_TFTLCD::setPageAddr (uint16_t s, uint16_t e) {
  CS_ACTIVE;

  CD_COMMAND;
  write8(0x2b);
  CD_DATA;

  // start
  write8(s >> 8);
  write8(s);
  // end
  write8(e >> 8);
  write8(e);

  CS_IDLE;
}

void Ext_TFTLCD::setColumnAddr (uint16_t s, uint16_t e) {
  CS_ACTIVE;

  CD_COMMAND;
  write8(0x2a);
  CD_DATA;

  // start
  write8(s >> 8);
  write8(s);
  // end
  write8(e >> 8);
  write8(e);

  CS_IDLE;
}

// void Ext_TFTLCD::fillScreen(uint16_t c) {
//   return;
// }