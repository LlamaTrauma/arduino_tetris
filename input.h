#include "MCP23008.h"

#include "sprites/button/button_a.h"
#include "sprites/button/button_b.h"
#include "sprites/button/button_start.h"
#include "sprites/button/button_left.h"
#include "sprites/button/button_right.h"
#include "sprites/button/button_up.h"
#include "sprites/button/button_down.h"

MCP23008 mcp(0x20);

#define HOLD_CLICK_DELAY 8
#define HOLD_CLICK_CYCLES 2

enum button_id {UP=0, DOWN, LEFT, RIGHT, A, B, START};
const PROGMEM uint8_t input_order[] = {3, 5, 4, 2, 1, 0, 0};

uint8_t* BUTTON_SPRITES[] = {
  BUTTON_UP,
  BUTTON_DOWN,
  BUTTON_LEFT,
  BUTTON_RIGHT,
  BUTTON_A,
  BUTTON_B,
  BUTTON_START
};

uint8_t BUTTON_PRESSED[] = {
  0, 0, 0, 0, 0, 0, 0
};

uint8_t BUTTON_CLICKED[] = {
  0, 0, 0, 0, 0, 0, 0
};

#define input_state_t struct input_state
struct input_state {
  uint8_t buttons;
};

#define historical_input_state_t struct historical_input_state
struct historical_input_state {
  time_t time;
  input_state_t state;
};

#define INPUT_BUFFER_SIZE 16
uint8_t input_buffer_end = INPUT_BUFFER_SIZE - 1;
historical_input_state_t INPUT_BUFFER[INPUT_BUFFER_SIZE];

uint8_t pollButtons () {
  // return 0b00101011;
  // Serial.println("input");
  // int byte = Serial.read();
  // if (byte == -1) {
  //   byte = 0;
  // }
  uint8_t byte = 0;
  for (int8_t i = 0; i < 7; i ++){
    // uint8_t bit = 0;
    uint8_t bit = (uint8_t) (mcp.read1(pgm_read_byte(input_order + i)) ? 0 : 1) << i;
    // if (bit) {
    //   Serial.print(i);
    //   Serial.print(' ');
    //   Serial.println(input_order[i]);
    // }
    // Serial.println(bit);
    byte += bit;
  }
  // Serial.println(byte);
  return byte;
}

void pollInput (process_t* p, time_t deadline) {
  input_buffer_end += 1;
  input_buffer_end %= INPUT_BUFFER_SIZE;
  historical_input_state_t* current = &INPUT_BUFFER[input_buffer_end];
  current->time = my_millis();
  current->state.buttons = 0;
  uint8_t polled_buttons = pollButtons();
  current->state.buttons = polled_buttons;
  p->next_run = my_millis() + 3;
}

bool buttonPressed (button_id button) {
  return INPUT_BUFFER[input_buffer_end].state.buttons & (1 << button);
}

uint8_t updateButtonStates () {
  for (uint8_t button = UP; button <= START; button ++) {
    BUTTON_CLICKED[button] = 0;
    if (buttonPressed(button)) {
      rand_next += button + 1;
      if (!BUTTON_PRESSED[button]) {
        BUTTON_CLICKED[button] = 1;
      }
      BUTTON_PRESSED[button] += 1;
      if (BUTTON_PRESSED[button] >= HOLD_CLICK_DELAY) {
        BUTTON_PRESSED[button] -= HOLD_CLICK_CYCLES;
        BUTTON_CLICKED[button] = 1;
      }
    } else {
      BUTTON_PRESSED[button] = 0;
    }
  }
}

bool drawInput () {
  static uint16_t y = 0;
  static uint8_t button = 0;
  // Serial.println(button);
  if (buttonPressed(button)) {
    // Serial.println("requesting button sprite");
    requestSprite(BUTTON_SPRITES[button], button * 10, y);
    // requestRect(button * 10, y, 10, 10, 0xFFFF - button * 0x123);
  } else {
    requestRect(button * 10, y, 10, 10, 0x0000);
  }
  button += 1;
  if (button > START) {
    button = 0;
    // y += 10;
    // if (y > TFT_HEIGHT) {
    //   y = 0;
    // }
    return 1;
  }
  return 0;
}

// time_t last_run = 0;
void drawInputProcess (process_t* p, time_t deadline) {
  // if (subtractTimes(my_millis(), last_run) < 1000) {
  //   return;
  // }
  // last_run = my_millis();
  Serial.setTimeout(10);
  addReqFactory(drawInput);
}

void testInput () {
  addGraphicsHook(drawInputProcess, 30, 0, "btnd");
}

void startInputProcess () {
  createProcessWithName(pollInput, my_millis(), WHENEVER, 1, 0, "inpt");
}

void initInput () {
  // delay(2000);
  // return;
  Wire.begin();
  mcp.begin();
  for (uint8_t i = 0; i < 7; i ++) {
    mcp.pinMode1(i, INPUT_PULLUP);
  }
  // return;
  startInputProcess();
}