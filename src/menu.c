#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "./board.h"
#include "./game.h"
#include "./menu.h"
#include "./resources.h"
#include "./spring.h"

typedef void (*button_action)();

typedef struct Button
{
  char text[32];
  bool arrowed;
  button_action click;
  button_action left;
  button_action right;
} Button;

typedef struct MenuPage
{
  Button buttons[3];
  int button_count;
  int button_active;
  bool hidden;
  button_action esc;
} MenuPage;

#define MAKE_BUTTON(TEXT, CLICK, LEFT, RIGHT, ARROW)                                                                   \
  {                                                                                                                    \
    TEXT, ARROW, CLICK, LEFT, RIGHT                                                                                    \
  }

#define MAKE_PAGE(NUM, ESC, ...)                                                                                       \
  {                                                                                                                    \
    {__VA_ARGS__}, NUM, -1, false, ESC                                                                                 \
  }

#define ACTION(A)                                                                                                      \
  if (A)                                                                                                               \
  A()

// Menu state

static struct MenuState
{
  bool exit;
  MenuPage *page;
} menu_state;

static GameConfig cfg;
static GameResults game_results;

// Button actions

static void page_start_start();
static void page_start_quit();

static void page_options_esc();
static void page_options_player_add();
static void page_options_player_sub();
static void page_options_bot_add();
static void page_options_bot_sub();
static void page_options_start();

static void page_pause_esc();
static void page_pause_resume();
static void page_pause_quit();

static void page_results_close();

static void game_on_finish();

// Menu pages

static MenuPage start_page = MAKE_PAGE(

    2, NULL,

    MAKE_BUTTON("Start game", page_start_start, NULL, NULL, false),
    MAKE_BUTTON("Quit game", page_start_quit, NULL, NULL, false),

);

static MenuPage options_page = MAKE_PAGE(

    3, page_options_esc,

    MAKE_BUTTON("", NULL, page_options_player_sub, page_options_player_add, true),
    MAKE_BUTTON("", NULL, page_options_bot_sub, page_options_bot_add, true),
    MAKE_BUTTON("Start", page_options_start, NULL, NULL, false),

);

static MenuPage game_page = MAKE_PAGE(

    2, page_pause_esc,

    MAKE_BUTTON("Resume game", page_pause_resume, NULL, NULL, false),
    MAKE_BUTTON("Quit game", page_pause_quit, NULL, NULL, false),

);

static MenuPage results_page = MAKE_PAGE(

    1, NULL,

    MAKE_BUTTON("Close", page_results_close, NULL, NULL, false),

);

// Helpers

static void page_open(MenuPage *page)
{
  menu_state.page = page;
  page->button_active = 0;
}

static void players_buttons_update()
{
  sprintf(options_page.buttons[0].text, "Players: %d", cfg.players);
  sprintf(options_page.buttons[1].text, "Bots: %d", cfg.bots);
}

// Start page actions

static void page_start_start()
{
  page_open(&options_page);
}

static void page_start_quit()
{
  menu_state.exit = true;
}

// Options page actions

static void page_options_esc()
{
  page_open(&start_page);
}

static void page_options_player_add()
{
  if (cfg.players < MEEPLE_COLOR_COUNT)
    cfg.players++;
  if (cfg.players + cfg.bots > MEEPLE_COLOR_COUNT)
    cfg.bots = MEEPLE_COLOR_COUNT - cfg.players;
  players_buttons_update();
}

static void page_options_player_sub()
{
  if (cfg.players > 0)
    cfg.players--;
  if (cfg.players + cfg.bots < 2)
    cfg.bots = 2 - cfg.players;
  players_buttons_update();
}

static void page_options_bot_add()
{
  if (cfg.bots < MEEPLE_COLOR_COUNT)
    cfg.bots++;
  if (cfg.players + cfg.bots > MEEPLE_COLOR_COUNT)
    cfg.players = MEEPLE_COLOR_COUNT - cfg.bots;
  players_buttons_update();
}

static void page_options_bot_sub()
{
  if (cfg.bots > 0)
    cfg.bots--;
  if (cfg.players + cfg.bots < 2)
    cfg.players = 2 - cfg.bots;
  players_buttons_update();
}

static void page_options_start()
{
  page_open(&game_page);
  game_page.hidden = true;
  game_init(cfg);
}

// Pause page actions

static void page_pause_esc()
{
  game_page.hidden = !game_page.hidden;
  game_pause(!game_page.hidden);
}

static void page_pause_resume()
{
  game_page.hidden = true;
  game_pause(false);
}

static void page_pause_quit()
{
  game_deinit();
  view_blur();
  page_open(&start_page);
}

// Results page actions

static void page_results_close()
{
  game_deinit();
  view_blur();
  page_open(&start_page);
}

// Game actions

static void game_on_finish(GameResults results)
{
  game_results = results;
  page_open(&results_page);
}

// Init and deinit

void menu_init()
{
  menu_state.page = &start_page;

  cfg = (GameConfig){
      .players = 2,
      .bots = 0,
      .on_finish = game_on_finish,
  };

  players_buttons_update();
  view_init();
}

void menu_deinit()
{
  game_deinit();
}

// Keydown

static void button_keydown(Button *b, int code)
{
  switch (code)
  {
  case ALLEGRO_KEY_ENTER:
    ACTION(b->click);
    ACTION(b->right);
    break;
  case ALLEGRO_KEY_LEFT:
    ACTION(b->left);
    break;
  case ALLEGRO_KEY_RIGHT:
    ACTION(b->right);
    break;
  }
}

static void menu_page_keydown(MenuPage *p, int code)
{
  if (code == ALLEGRO_KEY_ESCAPE)
    ACTION(p->esc);

  if (p->hidden)
    return;

  switch (code)
  {
  case ALLEGRO_KEY_UP:
    p->button_active = p->button_active < 0 ? 0 : (p->button_active + p->button_count - 1) % p->button_count;
    break;
  case ALLEGRO_KEY_DOWN:
  case ALLEGRO_KEY_TAB:
    p->button_active = p->button_active < 0 ? 0 : (p->button_active + 1) % p->button_count;
    break;
  }

  if (p->button_active >= 0)
    button_keydown(&p->buttons[p->button_active], code);
}

bool menu_keydown(int code)
{
  menu_page_keydown(menu_state.page, code);
  return menu_state.exit;
}

// Update  and render

static float bg_idle = 5;

void menu_tick(float dt)
{
  if (menu_state.page == &game_page && !menu_state.page->hidden)
    return;

  view_tick(dt);

  if (menu_state.page == &start_page)
    bg_idle += dt;
  else
    bg_idle = 5;

  if (bg_idle > 5)
  {
    bg_idle = 0;
    view_set(-BOARD_CENTER + rand() % 16 - 8, -BOARD_CENTER + rand() % 16 - 8);
  }
}

void button_render(Button *b, float x, float y, float s, bool active)
{
  ALLEGRO_BITMAP *bitmap = b->arrowed ? bitmaps.button_arrow : bitmaps.button;
  al_draw_scaled_bitmap(bitmap, 0, 0, 3 * BMP_UI_S, BMP_UI_S, x - s * 1.5, y - s * 0.5, s * 3, s, 0);
  al_draw_text(fonts.ui, al_map_rgb_f(1, 1, 1), x, y - FONT_SIZE * 0.55, ALLEGRO_ALIGN_CENTER, b->text);

  if (active)
    al_draw_scaled_bitmap(bitmaps.button_a, 0, 0, 3 * BMP_UI_S, BMP_UI_S, x - s * 1.5, y - s * 0.5, s * 3, s, 0);
}

void menu_results_render(float x, float y, float s)
{
  for (int i = 0; i < game_results.count; i++)
  {
    float ry = y - game_results.count * s / 2 + i * s;
    al_draw_scaled_bitmap(bitmaps.result_bg[i], 0, 0, 3 * BMP_UI_S, BMP_UI_S, x - s * 1.5, ry - s * 0.5, s * 3, s, 0);
    al_draw_scaled_bitmap(bitmaps.result_meeple[game_results.players[i].color - 1], 0, 0, BMP_UI_S, BMP_UI_S, x - s,
                          ry - s * 0.5, s, s, 0);
    al_draw_textf(fonts.ui, al_map_rgb_f(1, 1, 1), x + s * 0.5, ry - FONT_SIZE * 0.55, ALLEGRO_ALIGN_CENTER, "%d",
                  game_results.players[i].points);
  }
}

void menu_page_render(MenuPage *p, float x, float y, float s)
{
  if (p->hidden)
    return;

  al_hold_bitmap_drawing(true);

  y -= s * p->button_count / 2;
  for (int i = 0; i < p->button_count; i++)
    button_render(&p->buttons[i], x, y + i * s, s, i == p->button_active);

  al_hold_bitmap_drawing(false);
}

void menu_render(float w, float h)
{
  float x = w / 2, y = h / 2, s = 64;

  if (menu_state.page == &start_page)
  {
    y += s;
    al_draw_scaled_bitmap(bitmaps.splash, 0, 0, 1024, 235, x - 4 * s, y - 4 * s, 8 * s, 2 * s, 0);
  }
  else if (menu_state.page == &results_page)
  {
    menu_results_render(x, y, s);
    y += s * game_results.count / 2 + s;
  }

  menu_page_render(menu_state.page, x, y, s);
}
