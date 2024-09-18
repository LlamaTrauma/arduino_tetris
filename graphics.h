#pragma once

#include "Ext_TFTLCD.h"
#include "sprites/etc/fontmono9.h"
#include "sprites/etc/fontmono12.h"

#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin

#define TFT_WIDTH 240
#define TFT_HEIGHT 320

#define DRAW_PROCESS_INTERVAL 33

Ext_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

// so much for consistent formatting ig
enum drawReqType {SPRITE_INSTRUCTIONS, SPRITE_INSTRUCTIONS_PALETTE, SPRITE_GLYPH, RECT};

#define sprite_t struct sprite
struct sprite {
  uint8_t* addr;
  uint8_t* pal;
  uint8_t color;
  uint16_t x;
  uint16_t y;
};

#define rect_t struct rect
struct rect {
  uint16_t x;
  uint16_t y;
  uint16_t h;
  uint16_t w;
  uint16_t c;
};

#define drawReq_t struct drawReq
struct drawReq {
  bool finished;
  drawReqType type;
  void* reqPtr;
};

#define drawSpriteReq_t struct drawSpriteReq
struct drawSpriteReq {
  sprite_t sprite;
};

#define drawRectReq_t struct drawRectReq
struct drawRectReq {
  rect_t rect;
};

drawReq_t drawing;

#define strReq_t struct strReq
struct strReq {
  char* str;
  uint16_t x;
  uint16_t y;
  uint16_t color;
  uint8_t* font;
  uint8_t spacing;
};

strReq_t current_str;

void requestRect (uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c) {
  rect_t rect;
  rect.x = x;
  rect.y = y;
  rect.w = w;
  rect.h = h;
  rect.c = c;
  drawRectReq_t* rectReq = malloc(sizeof(drawRectReq_t));
  rectReq->rect = rect;
  drawReq_t req;
  req.type = RECT;
  req.finished = false;
  req.reqPtr = rectReq;
  drawing = req;
}

void requestSprite (uint8_t* addr, uint16_t x, uint16_t y) {
  sprite_t s;
  s.x = x;
  s.y = y;
  s.addr = addr;
  drawSpriteReq_t* sReq = malloc(sizeof(drawSpriteReq_t));
  sReq->sprite = s;
  drawReq_t req;
  req.type = SPRITE_INSTRUCTIONS;
  req.finished = false;
  req.reqPtr = sReq;
  drawing = req;
}

void requestPaletteSprite (uint8_t* addr, uint8_t* pal, uint16_t x, uint16_t y) {
  sprite_t s;
  s.x = x;
  s.y = y;
  s.addr = addr;
  s.pal = pal;
  drawSpriteReq_t* sReq = malloc(sizeof(drawSpriteReq_t));
  sReq->sprite = s;
  drawReq_t req;
  req.type = SPRITE_INSTRUCTIONS_PALETTE;
  req.finished = false;
  req.reqPtr = sReq;
  drawing = req;
}

void requestChar (uint16_t x, uint16_t y, char c, uint8_t* font, uint8_t color = 0xFF) {
  uint16_t c_x = x;
  uint16_t c_y = y;
  uint16_t glyph_pointer = pgm_read_word(font + 1 + (c - 32) * 2);
  uint8_t* char_addr = font + glyph_pointer;
  requestSprite(char_addr, c_x, c_y);
  // hacky, whatever
  drawing.type = SPRITE_GLYPH;
  ((drawSpriteReq_t*) drawing.reqPtr)->sprite.color = color;
}

bool drawCurrentStr () {
  static uint8_t state = 0;
  static uint8_t len = 0;
  static uint16_t x;
  if (state == 0) {
    len = strlen(current_str.str);
    x = current_str.x;
  }
  if (state == len) {
    state = 0;
    return 1;
  }
  requestChar(x, current_str.y, current_str.str[state], current_str.font, current_str.color);
  x += current_str.spacing;
  state += 1;
  return 0;
};

#define MAX_REQ_FACTORIES 10
uint8_t num_req_factories = 0;
bool (*REQ_FACTORIES[MAX_REQ_FACTORIES]) (void);

inline bool drawQueueEmpty () {
  return drawing.finished && !num_req_factories;
}

void addReqFactory (bool (*factory) (void)) {
  if (num_req_factories == MAX_REQ_FACTORIES) {
    Serial.println("Req factory queue full");
    return;
  }
  REQ_FACTORIES[num_req_factories++] = factory;
}

void removeReqFactory () {
  for (uint8_t i = 0; i < num_req_factories - 1; i ++) {
    REQ_FACTORIES[i] = REQ_FACTORIES[i + 1];
  }
  num_req_factories -= 1;
}

#define graphics_hook_t struct graphics_hook
// call some stuff a set time after vsync (so I can call logic and whatnot right before the next vsync)
struct graphics_hook {
  void (*event) (time_t);
  uint8_t millis_after;
  uint8_t level;
  char name[5];
};

#define MAX_GRAPHICS_HOOKS 5
uint8_t num_graphics_hooks = 0;
graphics_hook_t GRAPHICS_HOOKS[MAX_GRAPHICS_HOOKS];

void addGraphicsHook (void (*event) (time_t), uint8_t millis_after, uint8_t level, char* name) {
  if (num_graphics_hooks == MAX_GRAPHICS_HOOKS) {
    return;
  }
  graphics_hook_t* hook = &GRAPHICS_HOOKS[num_graphics_hooks++];
  hook->event = event;
  hook->level = level;
  hook->millis_after = millis_after;
  strcpy(hook->name, name);
}

void clearGraphicsHooks (uint8_t level) {
  uint8_t ind = 0;
  while (ind < num_graphics_hooks) {
    if (GRAPHICS_HOOKS[ind].level >= level) {
      for (uint8_t ind_ = ind; ind_ < num_graphics_hooks - 1; ind_ ++) {
        GRAPHICS_HOOKS[ind_] = GRAPHICS_HOOKS[ind_ + 1];
      }
      num_graphics_hooks -= 1;
    } else {
      ind += 1;
    }
  }
}

bool drawRect (rect_t* rect, time_t deadline) {
  static uint16_t row = 0;

  do {
    uint32_t pixels = min((rect->h - row) * (uint32_t) rect->w, 1000);
    uint8_t rows = pixels / rect->w;
    // Serial.println(pixels);
    // Serial.println(rect->w);
    // Serial.println(rect->h);
    // Serial.println(row);
    // Serial.println(rows);
    // Serial.println("------------");
    pixels = rows * rect->w;
    tft.fillRect(rect->x, rect->y+row, rect->w, rows, rect->c);
    row += rows;
  } while (row < rect->h && timeUntil(deadline) > 1);

  if (row == rect->h) {
    row = 0;
    return 1;
  }

  return 0;
}

bool drawInstructions (sprite_t* sprite, time_t deadline) {
  static uint8_t* addr = 0;
  static uint16_t instructions = 0;
  static uint8_t bits_per_palette_ind;
  static uint8_t bits_per_run_len;
  static uint8_t bits_per_addr_set;
  static uint8_t palette[128];

  static uint32_t buffer = 0;
  static uint8_t buffer_left = 32;
  static bool begin = 1;
  static uint8_t buffer_per_draw;
  static uint8_t buffer_per_addr;

  if (instructions == 0) {
    addr = sprite->addr + 1;
    uint8_t byte = pgm_read_byte(addr);
    bits_per_palette_ind = byte & 0b1111;
    bits_per_run_len = byte >> 4;
    byte = pgm_read_byte(addr + 1);
    bits_per_addr_set = byte & 0b1111;
    instructions = byte >> 4;
    byte = pgm_read_byte(addr + 2);
    instructions += byte << 4;
    addr += 3;

    buffer_per_draw = 1 + bits_per_palette_ind + bits_per_run_len;
    buffer_per_addr = 2 + bits_per_addr_set * 2;

    for (uint8_t i = 0; i < (1 << bits_per_palette_ind); i ++) {
      palette[i] = pgm_read_byte(addr++);
    }
    buffer_left = 0;
    buffer = 0;
    begin = 1;
  }

  uint8_t deadline_check = 1;
  while (instructions && (deadline_check++ % 4 || timeUntil(deadline) > 1)) {
    instructions -= 1;
    while (buffer_left < 24) {
      buffer += (uint32_t) pgm_read_byte(addr++) << buffer_left;
      buffer_left += 8;
    }
    uint8_t type = buffer & 1;
    buffer >>= 1;
    if (type == 0) {
      uint16_t l = buffer & ((1 << bits_per_run_len) - 1);
      buffer >>= bits_per_run_len;
      uint8_t p = buffer & ((1 << bits_per_palette_ind) - 1);
      buffer >>= bits_per_palette_ind;
      tft.pushEfficientColor(palette[p], l, begin);
      begin = 0;
      buffer_left -= buffer_per_draw;
    } else {
      begin = 1;
      bool which = buffer & 1;
      buffer >>= 1;
      uint16_t begin = buffer & ((1 << bits_per_addr_set) - 1);
      buffer >>= bits_per_addr_set;
      uint16_t end = buffer & ((1 << bits_per_addr_set) - 1);
      buffer >>= bits_per_addr_set;
      if (which == 0) {
        tft.setPageAddr(sprite->y + begin, sprite->y + end - 1);
      } else {
        tft.setColumnAddr(sprite->x + begin, sprite->x + end - 1);
      }
      buffer_left -= buffer_per_addr;
    }
  }

  if (!instructions) {
    return 1;
  }
  return 0;
}

bool drawInstructionsPalette (sprite_t* sprite, time_t deadline) {
  // Serial.println("drawing sprite");
  static uint8_t* addr = 0;
  static uint16_t instructions = 0;
  static uint8_t bits_per_palette_ind;
  static uint8_t bits_per_run_len;
  static uint8_t bits_per_addr_set;
  static uint8_t palette[128];

  static uint32_t buffer = 0;
  static uint8_t buffer_left = 32;
  static bool begin = 1;
  static uint8_t buffer_per_draw;
  static uint8_t buffer_per_addr;

  if (instructions == 0) {
    addr = sprite->addr + 1;
    uint8_t byte = pgm_read_byte(addr);
    bits_per_palette_ind = byte & 0b1111;
    bits_per_run_len = byte >> 4;
    byte = pgm_read_byte(addr + 1);
    bits_per_addr_set = byte & 0b1111;
    instructions = byte >> 4;
    byte = pgm_read_byte(addr + 2);
    instructions += byte << 4;
    addr += 3;

    buffer_per_draw = 1 + bits_per_palette_ind + bits_per_run_len;
    buffer_per_addr = 2 + bits_per_addr_set * 2;

    uint8_t* palette_addr = sprite->pal;
    for (uint8_t i = 0; i < (1 << bits_per_palette_ind); i ++) {
      palette[i] = pgm_read_byte(palette_addr++);
    }
    buffer_left = 0;
    buffer = 0;
    begin = 1;
  }

  uint8_t deadline_check = 1;
  while (instructions && (deadline_check++ % 4 || timeUntil(deadline) > 1)) {
    instructions -= 1;
    while (buffer_left < 24) {
      buffer += (uint32_t) pgm_read_byte(addr++) << buffer_left;
      buffer_left += 8;
    }
    uint8_t type = buffer & 1;
    buffer >>= 1;
    if (type == 0) {
      uint16_t l = buffer & ((1 << bits_per_run_len) - 1);
      buffer >>= bits_per_run_len;
      uint8_t p = buffer & ((1 << bits_per_palette_ind) - 1);
      buffer >>= bits_per_palette_ind;
      tft.pushEfficientColor(palette[p], l, begin);
      begin = 0;
      buffer_left -= buffer_per_draw;
    } else {
      begin = 1;
      bool which = buffer & 1;
      buffer >>= 1;
      uint16_t begin = buffer & ((1 << bits_per_addr_set) - 1);
      buffer >>= bits_per_addr_set;
      uint16_t end = buffer & ((1 << bits_per_addr_set) - 1);
      buffer >>= bits_per_addr_set;
      if (which == 0) {
        tft.setPageAddr(sprite->y + begin, sprite->y + end - 1);
      } else {
        tft.setColumnAddr(sprite->x + begin, sprite->x + end - 1);
      }
      buffer_left -= buffer_per_addr;
    }
  }

  if (!instructions) {
    return 1;
  }
  return 0;
}

bool drawGlyph (sprite_t* sprite, time_t deadline) {
  static uint8_t w = 0;
  static uint8_t h = 0;
  static uint8_t* addr = 0;
  bool begun = 0;
  static uint16_t pos = 0;
  static uint16_t bits = 0;
  static uint8_t byte = 0;
  static uint8_t color = 0;

  if (!begun) {
    addr = sprite->addr + 1;
    color = sprite->color;
    w = pgm_read_byte(addr);
    h = pgm_read_byte(addr + 1);
    addr += 2;
    begun = 1;
    pos = 0;
    bits = w * h;
    byte = 0;
  }

  uint8_t deadline_check = 1;

  while (pos < bits && (deadline_check % 128 || timeUntil(deadline) > 1)) {
    if (pos % 8 == 0) {
      byte = pgm_read_byte(addr++);
    }
    bool pixel = byte & (1 << (pos % 8)) ? 1 : 0;
    if (pixel) {
      uint16_t x = sprite->x + pos % w;
      uint16_t y = sprite->y + pos / w;
      tft.setPageAddr(y, y);
      tft.setColumnAddr(x, x);
      tft.pushEfficientColor(color, 1, 1);
    }
    pos += 1;
  }

  if (pos == bits) {
    begun = 0;
    return 1;
  }

  return 0;
}

bool drawCurrent(time_t deadline) {
  bool out = 0;
  switch (drawing.type) {
    case RECT:
      out = drawRect(&((drawRectReq_t*) drawing.reqPtr)->rect, deadline);
      break;
    case SPRITE_INSTRUCTIONS:
      out = drawInstructions(&((drawSpriteReq_t*) drawing.reqPtr)->sprite, deadline);
      break;
    case SPRITE_INSTRUCTIONS_PALETTE:
      out = drawInstructionsPalette(&((drawSpriteReq_t*) drawing.reqPtr)->sprite, deadline);
      break;
    case SPRITE_GLYPH:
      out = drawGlyph(&((drawSpriteReq_t*) drawing.reqPtr)->sprite, deadline);
      break;
    default:
      Serial.println("Drawing undef type");
      return 0;
      break;
  }
  if (out) {
    drawing.finished = 1;
    free(drawing.reqPtr);
  }
  return out;
}

void drawProcess (process_t* p, time_t deadline) {
  deadline = my_millis() + 20;
  do {
    while (drawing.finished) {
      if (num_req_factories == 0) {
        break;
      }
      // Serial.println("requesting graphic");
      bool factory_finished = REQ_FACTORIES[0]();
      if (factory_finished) {
        removeReqFactory();
      }
    }
    if (drawQueueEmpty()) {
      break;
    }
    drawCurrent(deadline);
  } while (timeUntil(deadline) > 1);
  for (uint8_t i = 0; i < num_graphics_hooks; i ++) {
    graphics_hook_t* h = &GRAPHICS_HOOKS[i];
    process_t* hp = createProcessWithName(h->event, p->next_run + h->millis_after, FIXED, 1, h->level, h->name);
    hp->repeat = false;
  }
  p->next_run += DRAW_PROCESS_INTERVAL;
}

void initGraphics () {
  // From Adafruit example
  // tft.reset();
  // uint16_t ID = tft.readID();
  uint16_t ID = 0x9341;
  tft.begin(ID);
  tft.setRotation(2);
  tft.initFramerate();
  // Serial.println("filling screen");
  // tft.fillScreen(0);
  // Serial.println("done");
  drawing.finished = 1;
  time_t now = my_millis();
  createProcessWithName(drawProcess, now, FIXED, 1, 0, "gfx");
}

uint8_t screen_clear_color = 0x0;
void screenClear () {
  requestRect(0, 0, TFT_WIDTH, TFT_HEIGHT, screen_clear_color);
  return 1;
}

void reqScreenClear (uint8_t color) {
  screen_clear_color = color;
  addReqFactory(screenClear);
}