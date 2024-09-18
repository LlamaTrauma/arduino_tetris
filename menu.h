#pragma once

#include "sprites/menu/button_wide.h"
#include "sprites/menu/button_small.h"
#include <string.h>

uint8_t numToStr (uint32_t num, char* str) {
  if (num == 0) {
    str[0] = '0';
    str[1] = '\0';
    return 1;
  }
  uint32_t place = 100000000;
  uint8_t ind = 0;
  while (place > 0) {
    if (place <= num || ind) {
      str[ind] = '0' + num / place;
      num %= place;
      ind += 1;
    }
    place /= 10;
  }
  str[ind] = '\0';
  return ind;
}

#define BUTTON_WIDE_X 60

#define button_t struct button
struct button {
  uint16_t x;
  uint16_t y;
  uint8_t row;
  uint8_t col;
  uint8_t type;
  char text[10];
  uint8_t id;
  void (*onclick) (uint8_t);
};

#define menu_controller_t struct menu_controller
struct menu_controller {
  button_t* buttons[15];
  button_t* last_selected;
  button_t* selected;
  uint8_t num_buttons;
  int8_t row;
  int8_t col;
  uint8_t num_rows;
  uint8_t num_cols;
  bool drawn;
};

menu_controller_t menu;

void addMenuButton (button_t* b) {
  button_t* b_ptr = malloc(sizeof(button_t));
  menu.buttons[menu.num_buttons++] = b_ptr;
  *b_ptr = *b;
  menu.num_rows = max(menu.num_rows, b->row + 1);
  menu.num_cols = max(menu.num_cols, b->col + 1);
}

void clearMenuButtons () {
  while (menu.num_buttons) {
    free(menu.buttons[--menu.num_buttons]);
  };
}

button_t* menuButtonAt (uint8_t r, uint8_t c) {
  for (uint8_t i = 0; i < menu.num_buttons; i ++) {
    if (menu.buttons[i]->row == r && menu.buttons[i]->col == c) {
      return menu.buttons[i];
    }
  }
  return NULL;
}

void menuSelectNew (int8_t dx, int8_t dy) {
  dx = dy ? -1 : dx;
  uint8_t start_col = menu.col;
  if (dy) {
    menu.row = (menu.row + menu.num_rows + dy) % menu.num_rows;
    if (menu.selected = menuButtonAt(menu.row, menu.col)) {
      return;
    }
  }
  while (1) {
    menu.col += dx;
    if (menu.col == -1) {
      menu.col = menu.num_cols - 1;
      if (!dy) {
        menu.row = (menu.row - 1 + menu.num_rows) % menu.num_rows;
      }
    } else if (menu.col == menu.num_cols) {
      menu.col = 0;
      if (!dy) {
        menu.row = (menu.row + 1) % menu.num_rows;
      }
    }
    if (menu.selected = menuButtonAt(menu.row, menu.col)) {
      return;
    }
  }
}

bool drawButton (button_t* button) {
  static uint8_t state = 0;
  static uint8_t len = 0;
  uint8_t str_w;
  if (state == 0) {
    len = strlen(button->text);
    switch (button->type) {
      case 0:
        if (menu.selected == button) {
          requestPaletteSprite(BUTTON_WIDE, BUTTON_WIDE_SELECTED, button->x, button->y);
        } else {
          requestPaletteSprite(BUTTON_WIDE, BUTTON_WIDE_DEFAULT, button->x, button->y);
        }
        break;
      case 1:
        if (menu.selected == button) {
          requestPaletteSprite(BUTTON_SMALL, BUTTON_SMALL_SELECTED, button->x, button->y);
        } else {
          requestPaletteSprite(BUTTON_SMALL, BUTTON_SMALL_DEFAULT, button->x, button->y);
        }
        break;
    }
    state += 1;
    return 0;
  } else {
    if (state - 1 == len) {
      state = 0;
      return 1;
    }
    switch (button->type) {
      case 0:
        str_w = 14 * len;
        requestChar(button->x + (120 - str_w) / 2 + (state - 1) * 14, button->y + 20, button->text[state - 1], FONTMONO12, 0x00);
        break;
      case 1:
        str_w = 14 * len;
        requestChar(button->x + (40 - str_w) / 2 + (state - 1) * 14, button->y + 10, button->text[state - 1], FONTMONO12, 0x00);
        break;
    }
    state += 1;
    return 0;
  }
}

bool drawMenu () {
  static uint8_t state = 0;
  static bool drawing = 0;
  while (!drawing && state < menu.num_buttons) {
    button_t* b = menu.buttons[state];
    bool shouldDraw = menu.selected != menu.last_selected;
    shouldDraw &= (menu.selected == b || menu.last_selected == b);
    shouldDraw |= !menu.drawn;
    if (shouldDraw) {
      drawing = 1;
      break;
    }
    state += 1;
  }
  if (state < menu.num_buttons) {
    if (drawButton(menu.buttons[state])) {
      state += 1;
      drawing = 0;
    }
    return 0;
  } else {
    state = 0;
    drawing = 0;
    menu.drawn = 1;
    menu.last_selected = menu.selected;
    return 1;
  }
}

void menuHook () {
  if (!drawQueueEmpty()) {
    // Serial.println("miss");
    return;
  }
  updateButtonStates();
  if (BUTTON_CLICKED[LEFT]) {
    menuSelectNew(-1, 0);
    return;
  }
  if (BUTTON_CLICKED[RIGHT]) {
    menuSelectNew(1, 0);
    return;
  }
  if (BUTTON_CLICKED[UP]) {
    menuSelectNew(0, -1);
    return;
  }
  if (BUTTON_CLICKED[DOWN]) {
    menuSelectNew(0, 1);
    return;
  }
  if (BUTTON_CLICKED[A]) {
    menu.selected->onclick(menu.selected->id);
    return;
  }
  addReqFactory(drawMenu);
}

void teardownMenu () {
  clearProcesses(1);
  clearGraphicsHooks(1);
  clearMenuButtons();
  menu.row = 0;
  menu.col = 0;
  menu.num_rows = 0;
  menu.num_cols = 0;
}