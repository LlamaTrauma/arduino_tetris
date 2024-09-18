#include "sprites/tetris/field_background.h"
#include "sprites/tetris/next_background.h"
#include "sprites/tetris/tetris_block.h"
#include "sprites/etc/wario_thumbs_up.h"
#include "tetrominos.h"

#define FIELD_HEIGHT 24
#define FIELD_WIDTH 10
// #define FIELD_HEIGHT 1
// #define FIELD_WIDTH 1
#define FIELD_X 10
#define FIELD_Y 20
#define BLOCK_SIZE 14

#define NEXT_X (FIELD_X + FIELD_WIDTH * BLOCK_SIZE + 10)
#define NEXT_Y (FIELD_Y)

#define NUM_BLOCK_SPRITES 2
uint8_t* BLOCK_SPRITES[] = {
  BLOCK_RED,
  BLOCK_BLUE,
  BLOCK_GREY,
};

#define tetromino_t struct tetromino
struct tetromino {
  uint8_t color;
  int8_t x;
  int8_t y;
  uint8_t piece;
  uint8_t rotation;
  uint16_t blocks;
};

#define tetris_t struct tetris
struct tetris {
  uint8_t fall_timer;
  // shows up in next slot
  tetromino_t next_tetromino;
  // tetromino on the board
  tetromino_t current_tetromino;
  // state of current_tetromino before last update - used for drawing
  tetromino_t past_tetromino;
  uint32_t score;
  uint16_t lines;
  uint8_t level;
  uint8_t state;
  uint8_t FIELD[FIELD_HEIGHT][FIELD_WIDTH];
  uint8_t mode;
  uint8_t start_level;
  uint8_t num_tetrominos;
  // global storage for a few values used to draw the ending hud
  uint32_t end_score;
  uint32_t best;
  bool new_best;
  bool all_cleared;
};

tetris_t game;

void newTetromino (tetromino_t* t) {
  uint8_t piece = rand8() & 0b111;
  while (piece == 7) {
    piece = rand8() & 0b111;
  }
  t->piece = piece;
  t->rotation = 0;
  t->x = START_COLS[piece];
  t->y = START_ROWS[piece];
  t->color = rand8() % NUM_BLOCK_SPRITES;
  t->blocks = TETROMINOS[piece][0];
}

uint8_t draw_row_start = 0;
uint8_t draw_row_end   = 19;
bool drawField () {
  static uint8_t x = 0;
  static int8_t y = -1;
  if (y == -1) {
    // Serial.println("new draw");
    if (draw_row_end > 19) {
      draw_row_end = 19;
    }
    y = draw_row_start;
    x = 0;
    if (y > draw_row_end) {
      y = -1;
      return 1;
    }
  }
  if (game.FIELD[y][x]) {
    requestPaletteSprite(TETRIS_BLOCK, BLOCK_SPRITES[game.FIELD[y][x] - 1], FIELD_X + x * BLOCK_SIZE, FIELD_Y + (19 - y) * BLOCK_SIZE);
  } else {
    requestSprite(FIELD_BACKGROUND, FIELD_X + x * BLOCK_SIZE, FIELD_Y + (19 - y) * BLOCK_SIZE);
  }
  x += 1;
  if (x >= FIELD_WIDTH) {
    x = 0;
    y += 1;
    if (y > draw_row_end) {
      y = -1;
      return 1;
    }
  }
  return 0;
}

bool blockInTetromino (tetromino_t* t, uint8_t x, uint8_t y) {
  uint8_t bit = (3 - x) + y * 4;
  return t->blocks & (uint16_t) 1 << bit ? 1 : 0;
}

bool absBlockInTetromino (tetromino_t* t, uint8_t x, uint8_t y) {
  if (t->x > x || t->x + 3 < x) {
    return 0;
  }
  if (t->y > y || t->y + 3 < y) {
    return 0;
  }
  return blockInTetromino(t, x - t->x, y - t->y);
}

int8_t tetrominoMinRow (tetromino_t* t) {
  int8_t y = t->y;
  if (!t->blocks) {
    return y + 3;
  }
  uint8_t row = 0;
  while ((t->blocks & 0b1111 << (row * 4)) == 0) {
    row += 1;
    y += 1;
  }
  return y;
}

bool tetrominoCollision () {
  for (uint8_t y = 0; y < 4; y ++) {
    for (uint8_t x = 0; x < 4; x++) {
      int8_t block_y = game.current_tetromino.y + y;
      int8_t block_x = game.current_tetromino.x + x;
      if (blockInTetromino(&(game.current_tetromino), x, y)) {
        if (block_x < 0 || block_x >= FIELD_WIDTH) {
          return 1;
        }
        if (block_y < 0) {
          return 1;
        }
        if (block_y < FIELD_HEIGHT && game.FIELD[block_y][block_x]) {
          return 1;
        }
      }
    }
  }
  return 0;
}

void moveTetrominoX (uint8_t dir) {
  game.current_tetromino.x += dir;
  if (tetrominoCollision()) {
    game.current_tetromino.x -= dir;
  }
}

// return 1 if collided and moved back
bool moveTetrominoY () {
  game.current_tetromino.y -= 1;
  if (tetrominoCollision()) {
    game.current_tetromino.y += 1;
    return 1;
  }
  return 0;
}

void rotateTetromino (uint8_t dir) {
  game.current_tetromino.rotation += dir;
  game.current_tetromino.rotation %= 4;
  game.current_tetromino.blocks = TETROMINOS[game.current_tetromino.piece][game.current_tetromino.rotation];
  if (tetrominoCollision()) {
    game.current_tetromino.rotation -= dir;
    game.current_tetromino.rotation %= 4;
    game.current_tetromino.blocks = TETROMINOS[game.current_tetromino.piece][game.current_tetromino.rotation];
  }
}

bool moveTetrominoToField () {
  for (uint8_t y = 0; y < 4; y ++) {
    for (uint8_t x = 0; x < 4; x++) {
      if (blockInTetromino(&game.current_tetromino, x, y)) {
        uint8_t block_y = game.current_tetromino.y + y;
        uint8_t block_x = game.current_tetromino.x + x;
        game.FIELD[block_y][block_x] = game.current_tetromino.color + 1;
      }
    }
  }
}

bool drawTetromino () {
  // return;
  static uint8_t y = 0;
  static uint8_t x = 0;
  static uint8_t step = 0;
  while (step < 2) {
    while (y < 4) {
      while (x < 4) {
        if (step == 0) {
          if (game.past_tetromino.y + y >= 20) {
            x += 1;
            continue;
          }
          if (blockInTetromino(&game.past_tetromino, x, y)) {
            if (!absBlockInTetromino(&game.current_tetromino, game.past_tetromino.x + x, game.past_tetromino.y + y)) {
              requestSprite(FIELD_BACKGROUND, FIELD_X + (game.past_tetromino.x + x) * BLOCK_SIZE, FIELD_Y + (19 - (game.past_tetromino.y + y)) * BLOCK_SIZE);
              x += 1;
              return 0;
            }
          }
        } else if (step == 1) {
          if (game.current_tetromino.y + y >= 20) {
            x += 1;
            continue;
          }
          if (blockInTetromino(&game.current_tetromino, x, y)) {
            if (!absBlockInTetromino(&game.past_tetromino, game.current_tetromino.x + x, game.current_tetromino.y + y)) {
              requestPaletteSprite(TETRIS_BLOCK, BLOCK_SPRITES[game.current_tetromino.color], FIELD_X + (game.current_tetromino.x + x) * BLOCK_SIZE, FIELD_Y + (19 - (game.current_tetromino.y + y)) * BLOCK_SIZE);
              x += 1;
              return 0;
            }
          }
        }
        x += 1;
      }
      x = 0;
      y += 1;
    }
    y = 0;
    step += 1;
  }
  step = 0;
  return 1;
}

bool drawNext () {
  static uint8_t state = 0;
  static uint8_t y;
  static uint8_t x;
  switch (state) {
    case 0:
      requestSprite(NEXT_BACKGROUND, NEXT_X, NEXT_Y);
      state += 1;
      return 0;
    case 1:
      while (y < 4) {
        while (x < 4) {
          if (blockInTetromino(&game.next_tetromino, x, y)) {
            requestPaletteSprite(TETRIS_BLOCK, BLOCK_SPRITES[game.next_tetromino.color], NEXT_X + 7 + x * BLOCK_SIZE, NEXT_Y + 7 + (3 - y) * BLOCK_SIZE);
            x += 1;
            return 0;
          }
          x += 1;
        }
        x = 0;
        y += 1;
      }
      y = 0;
    default:
      state = 0;
      return 1;
  }
}

#define SCORE_Y 105
#define LINES_Y 175
#define LEVEL_Y 245
#define Y_GAP 25

bool drawScore () {
  static uint8_t state = 0;
  static char* str = malloc(10);
  static bool init = 0;
  switch (state) {
    case 0:
      if (!init) {
        current_str.str = malloc(10);
        strcpy(current_str.str, "SCORE");
        current_str.x = 160;
        current_str.y = SCORE_Y;
        current_str.font = FONTMONO12;
        current_str.spacing = 14;
        current_str.color = 0xFF;
        init = 1;
        requestRect(160, SCORE_Y, 80, 40, 0x00);
        return 0;
      }
      if (!drawCurrentStr()) {
        return 0;
      } else {
        free(current_str.str);
        state += 1;
        init = 0;
        return 0;
      }
      break;
    case 1:
      if (!init) {
        current_str.str = malloc(10);
        if (game.mode == 0) {
          numToStr(game.score, current_str.str);
        } else {
          numToStr(game.num_tetrominos, current_str.str);
        }
        current_str.x = 160;
        current_str.y = SCORE_Y + 20;
        current_str.font = FONTMONO9;
        current_str.spacing = 11;
        current_str.color = 0xFF;
        init = 1;
        return 0;
      }
      if (!drawCurrentStr()) {
        return 0;
      } else {
        free(current_str.str);
        state += 1;
        init = 0;
        return 0;
      }
    case 2:
      state = 0;
      return 1;
  }
  state = 0;
  return 1;
}

bool drawLines () {
  static uint8_t state = 0;
  static char* str = malloc(10);
  static bool init = 0;
  switch (state) {
    case 0:
      if (!init) {
        current_str.str = malloc(10);
        strcpy(current_str.str, "LINES");
        current_str.x = 160;
        current_str.y = LINES_Y;
        current_str.font = FONTMONO12;
        current_str.spacing = 14;
        current_str.color = 0xFF;
        init = 1;
        requestRect(160, LINES_Y, 80, 40, 0x00);
        return 0;
      }
      if (!drawCurrentStr()) {
        return 0;
      } else {
        free(current_str.str);
        state += 1;
        init = 0;
        return 0;
      }
      break;
    case 1:
      if (!init) {
        current_str.str = malloc(10);
        numToStr(game.lines, current_str.str);
        current_str.x = 160;
        current_str.y = LINES_Y + 20;
        current_str.font = FONTMONO12;
        current_str.spacing = 14;
        current_str.color = 0xFF;
        init = 1;
        return 0;
      }
      if (!drawCurrentStr()) {
        return 0;
      } else {
        free(current_str.str);
        state += 1;
        init = 0;
        return 0;
      }
    case 2:
      state = 0;
      return 1;
  }
  state = 0;
  return 1;
}

bool drawLevel () {
  static uint8_t state = 0;
  static char* str = malloc(10);
  static bool init = 0;
  switch (state) {
    case 0:
      if (!init) {
        current_str.str = malloc(10);
        strcpy(current_str.str, "LEVEL");
        current_str.x = 160;
        current_str.y = LEVEL_Y;
        current_str.font = FONTMONO12;
        current_str.spacing = 14;
        current_str.color = 0xFF;
        init = 1;
        requestRect(160, LEVEL_Y, 80, 40, 0x00);
        return 0;
      }
      if (!drawCurrentStr()) {
        return 0;
      } else {
        free(current_str.str);
        state += 1;
        init = 0;
        return 0;
      }
      break;
    case 1:
      if (!init) {
        current_str.str = malloc(10);
        numToStr(game.level, current_str.str);
        current_str.x = 160;
        current_str.y = LEVEL_Y + 20;
        current_str.font = FONTMONO12;
        current_str.spacing = 14;
        current_str.color = 0xFF;
        init = 1;
        return 0;
      }
      if (!drawCurrentStr()) {
        return 0;
      } else {
        free(current_str.str);
        state += 1;
        init = 0;
        return 0;
      }
    case 2:
      state = 0;
      return 1;
  }
  state = 0;
  return 1;
}

bool drawHUD () {
  static uint8_t state = 0;
  switch (state) {
    case 0:
      if (drawNext()) {
        // Serial.println("drew next");
        state = 1;
      }
      return 0;
    case 1:
      if (drawScore()) {
        // Serial.println("drew score");
        state = 2;
      }
      return 0;
    case 2:
      if (drawLines()) {
        // Serial.println("drew lines");
        state = 3;
      }
      return 0;
    case 3:
      if (drawLevel()) {
        // Serial.println("drew level");
        state = 0;
        return 1;
      }
      return 0;
    default:
      // Serial.println("bad state");
      return 0;
  }
}

void requestDrawTetromino () {
  addReqFactory(drawTetromino);
}

bool drawTetris () {
  reqScreenClear(0x00);
  addReqFactory(drawField);
  addReqFactory(drawHUD);
}

void teardownTetris () {
  clearProcesses(1);
  clearGraphicsHooks(1);
}

bool drawEndingHUD () {
  static uint8_t state = 0;
  static int8_t len = -1;
  static bool init = 0;
  static char* str;
  switch (state) {
    case 0:
      if (game.new_best) {
        requestSprite(WARIO_THUMBS_UP, 46, 235);
        state += 1;
        return 0;
      }
      state += 1;
    case 1:
      if (!init) {
        current_str.str = malloc(10);
        strcpy(current_str.str, "PAST BEST");
        current_str.x = 17;
        current_str.y = 190;
        current_str.font = FONTMONO12;
        current_str.spacing = 14;
        current_str.color = 0xFF;
        init = 1;
        requestRect(17, 190, 126, 40, 0x00);
        return 0;
      }
      if (!drawCurrentStr()) {
        return 0;
      } else {
        free(current_str.str);
        state += 1;
        init = 0;
        return 0;
      }
      break;
    case 2:
      if (!init) {
        current_str.str = malloc(10);
        numToStr(game.best, current_str.str);
        current_str.x = 17;
        current_str.y = 210;
        current_str.font = FONTMONO12;
        current_str.spacing = 14;
        current_str.color = 0xFF;
        init = 1;
      }
      if (!drawCurrentStr()) {
        return 0;
      } else {
        free(current_str.str);
        state += 1;
        init = 0;
        return 0;
      }
      break;
    case 3:
      state = 0;
      return 1;
  }
}

void (*lost_menu_fn) ();

enum TETRIS_STATE {TETRIS_FIRST_FALLING=0, TETRIS_FALLING=1, TETRIS_FIRST_CLEARING=2, TETRIS_CLEARING=3, TETRIS_UPDATING=4, TETRIS_LOST=5};
void playTetris () {
  static uint8_t row_y = 0;
  static uint8_t clears_below = 0;
  uint8_t tetris_fall_time;
  if (!drawQueueEmpty()) {
    // Serial.println("miss");
    return;
  }
  updateButtonStates();
  switch (game.state) {
    case TETRIS_FIRST_FALLING:
    clearNotes();
    addNote(256, 250);
    playNote();
    // Serial.println("a");
    case TETRIS_FALLING:
    // Serial.println("b");
      game.past_tetromino = game.current_tetromino;
      if (game.state == TETRIS_FIRST_FALLING) {
        game.past_tetromino.blocks = 0;
        game.state = TETRIS_FALLING;
      }
      if (BUTTON_CLICKED[LEFT]) {
        moveTetrominoX(-1);
      }
      if (BUTTON_CLICKED[RIGHT]) {
        moveTetrominoX(1);
      }
      if (BUTTON_CLICKED[DOWN]) {
      }
      if (BUTTON_CLICKED[A]) {
        rotateTetromino(3);
      }
      if (BUTTON_CLICKED[B]) {
        rotateTetromino(1);
      }
      game.fall_timer += 1;
      tetris_fall_time = 20 - game.level;
      tetris_fall_time = tetris_fall_time < 2 ? 2 : tetris_fall_time;
      // uint8_t tetris_fall_time = 2;
      if (game.fall_timer >= tetris_fall_time || BUTTON_CLICKED[DOWN]) {
        game.fall_timer = 0;
        bool hit = moveTetrominoY();
        if (hit) {
          requestDrawTetromino();
          moveTetrominoToField();
          game.state = TETRIS_FIRST_CLEARING;
          return;
        }
      }
      requestDrawTetromino();
      return;
    case TETRIS_FIRST_CLEARING:
    game.state = TETRIS_CLEARING;
    row_y = tetrominoMinRow(&game.current_tetromino);
    clears_below = 0;
    case TETRIS_CLEARING:
    // Serial.println("d");
      while (row_y < 20) {
        if (row_y < 0) {
          row_y += 1;
          continue;
        }
        uint8_t copy_y = row_y + clears_below;
        uint16_t prior_row = 0;
        uint16_t row = 0;
        for (uint8_t x = 0; x < FIELD_WIDTH; x ++) {
          prior_row <<= 1;
          row <<= 1;
          prior_row += game.FIELD[row_y][x] ? 1 : 0;
          game.FIELD[row_y][x] = copy_y <= 19 ? game.FIELD[copy_y][x] : 0;
          row += game.FIELD[row_y][x] ? 1 : 0;
        }
        // draw_row_start = row_y;
        // draw_row_end = row_y;
        if (prior_row != row) {
          draw_row_end = row_y;
          if (clears_below == 0) {
            draw_row_start = row_y;
          }
        }
        if (row == 0b1111111111) {
          clears_below += 1;
        } else {
          row_y += 1;
        }
      }
      if (clears_below) {
        game.score += (game.level + 19) << clears_below;
        addReqFactory(drawField);
      }

      game.lines += clears_below;
      if (game.mode == 0) {
        game.level = max(game.level, game.lines / 20 + 1);
      }
      game.state = TETRIS_UPDATING;
      return;
    case TETRIS_UPDATING:
    // Serial.println("e");
      game.num_tetrominos += 1;
      game.current_tetromino = game.next_tetromino;
      newTetromino(&game.next_tetromino);
      addReqFactory(drawHUD);
      if (tetrominoCollision()) {
        game.state = TETRIS_LOST;
      } else {
        game.state = TETRIS_FIRST_FALLING;
      }
      if (clears_below && game.mode == 1) {
        game.all_cleared = 1;
        for (uint8_t y = 0; y < 20; y ++) {
          for (uint8_t x = 0; x < 10; x ++) {
            if (game.FIELD[y][x] == NUM_BLOCK_SPRITES + 1) {
              game.all_cleared = 0;
              break;
            }
          }
        }
        if (game.all_cleared) {
          game.state = TETRIS_LOST;
        }
      }
      return;
    case TETRIS_LOST:
      uint32_t score_addr;
      game.new_best = 0;
      if (game.mode == 0) {
        score_addr = 0x4;
        game.end_score = game.score;
        // game.best = EEPROM.read(score_addr);
        game.best = 10;
        if (game.score > game.best) {
          game.new_best = 1;
          EEPROM.write(score_addr, game.score);
        }
      } else if (game.mode == 1) {
        score_addr = 0x4 + game.start_level * 4;
        game.end_score = game.num_tetrominos;
        game.best = EEPROM.read(score_addr);
        // game.best = 1000;
        if (game.all_cleared && game.num_tetrominos < game.best) {
          game.new_best = 1;
          EEPROM.write(score_addr, game.num_tetrominos);
        }
      }
      teardownTetris();
      addReqFactory(drawEndingHUD);
      lost_menu_fn();
      break;
    default:
      Serial.println("default case");
      return;
  }
}

void initTetris () {
  game.score = 0;
  game.level = game.start_level;
  game.lines = 0;
  game.fall_timer = 0;
  game.state = 0;
  game.num_tetrominos = 0;
  game.all_cleared = 0;
  draw_row_start = 0;
  draw_row_end = 19;
  memset(game.FIELD, 0, FIELD_HEIGHT * FIELD_WIDTH);
  if (game.mode == 1) {
    for (uint8_t y = 0; y < 10; y ++) {
      uint8_t set = 0;
      uint8_t x = 0;
      while (set < 5) {
        if (rand8() & 1) {
          game.FIELD[y][x] = NUM_BLOCK_SPRITES + 1;
          set += 1;
        }
        x = (x + 1) % 10;
      }
    }
  }
  newTetromino(&game.current_tetromino);
  newTetromino(&game.next_tetromino);
}

void startTetris () {
  // return;
  initTetris();
  drawTetris();
  addGraphicsHook(playTetris, 30, 1, "play");
}

void levelOnclick (uint8_t id) {
  game.start_level = id;
  teardownMenu();
  startTetris();
}

void loadLevelMenu () {
  button_t b;
  for (uint8_t i = 1; i <= 10; i ++) {
    b.col = (i - 1) % 5;
    b.x = b.col * 40 + 20;
    b.row = (i - 1) / 5;
    b.y = b.row * 40 + 120;
    b.id = i;
    b.onclick = levelOnclick;
    numToStr(i, b.text);
    b.type = 1;
    addMenuButton(&b);
  }

  menu.selected = menu.buttons[0];
  menu.drawn = 0;
  reqScreenClear(0x00);
  addReqFactory(drawMenu);
  addGraphicsHook(menuHook, 30, 1, "lmnu");
}

void typeOnclick (uint8_t id) {
  game.mode = id;
  teardownMenu();
  loadLevelMenu();
}

void loadMainMenu () {
  button_t a_btn;
  a_btn.x = BUTTON_WIDE_X;
  a_btn.y = 40;
  a_btn.row = 0;
  a_btn.col = 0;
  a_btn.id = 0;
  a_btn.type = 0;
  strcpy(a_btn.text, "A TYPE");
  a_btn.onclick = typeOnclick;
  addMenuButton(&a_btn);

  button_t b_btn;
  b_btn.x = BUTTON_WIDE_X;
  b_btn.y = 120;
  b_btn.row = 1;
  b_btn.col = 0;
  b_btn.id = 1;
  b_btn.type = 0;
  strcpy(b_btn.text, "B TYPE");
  b_btn.onclick = typeOnclick;
  addMenuButton(&b_btn);

  menu.selected = menu.buttons[0];
  menu.drawn = 0;
  reqScreenClear(0x00);
  addReqFactory(drawMenu);
  addGraphicsHook(menuHook, 30, 1, "mmnu");
}

void lostOnclick (uint8_t id) {
  teardownMenu();
  switch (id) {
    case 0:
      startTetris();
      break;
    case 1:
      loadMainMenu();
      break;
    default:
      break;
  }
}

void loadLostMenu () {
  button_t a_btn;
  a_btn.x = 20;
  a_btn.y = 30;
  a_btn.row = 0;
  a_btn.col = 0;
  a_btn.id = 0;
  a_btn.type = 0;
  strcpy(a_btn.text, "AGAIN");
  a_btn.onclick = lostOnclick;
  addMenuButton(&a_btn);

  button_t b_btn;
  b_btn.x = 20;
  b_btn.y = 110;
  b_btn.row = 1;
  b_btn.col = 0;
  b_btn.id = 1;
  b_btn.type = 0;
  strcpy(b_btn.text, "HOME");
  b_btn.onclick = lostOnclick;
  addMenuButton(&b_btn);

  menu.selected = menu.buttons[0];
  menu.drawn = 0;
  addReqFactory(drawMenu);
  addGraphicsHook(menuHook, 30, 1, "emnu");
}

void initMenu () {
  menu.row = 0;
  menu.col = 0;
  menu.num_buttons = 0;
  menu.num_rows = 0;
  menu.num_cols = 0;
  lost_menu_fn = loadLostMenu;
}