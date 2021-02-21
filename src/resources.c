#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>

#include "./resources.h"
#include "./utils.h"

static ALLEGRO_BITMAP *board, *ui;

Bitmaps bitmaps;
Fonts fonts;

void res_init()
{
  al_set_new_bitmap_flags(ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR);

  fonts.ui = al_load_ttf_font("./res/barbedor.ttf", FONT_SIZE, 0);

  board = al_load_bitmap("./res/board.png");
  MUST_INIT(board, "board bitmap");

  for (int i = 0; i < 5; i++)
    bitmaps.meeple[i] = al_create_sub_bitmap(board, BMP_MEEPLE_S * (i + 6), 0, BMP_MEEPLE_S, BMP_MEEPLE_S);
  bitmaps.coin = al_create_sub_bitmap(board, BMP_MEEPLE_S * 12, 0 * BMP_MEEPLE_S, BMP_MEEPLE_S, BMP_MEEPLE_S);

  for (int i = 0; i < 24; i++)
    bitmaps.tiles[i] =
        al_create_sub_bitmap(board, BMP_TILES_S * (i / 8), BMP_TILES_S * (i % 8), BMP_TILES_S, BMP_TILES_S);
  bitmaps.tile_highlight =
      al_create_sub_bitmap(board, BMP_TILES_S * 3.5, BMP_TILES_S * 1.5, 2 * BMP_TILES_S, 2 * BMP_TILES_S);

  ui = al_load_bitmap("./res/ui.png");
  MUST_INIT(ui, "ui bitmap");

  for (int i = 0; i < 5; i++)
    bitmaps.player_state[i] = al_create_sub_bitmap(ui, 0, BMP_UI_S * (i + 3), BMP_UI_S * 3, BMP_UI_S);
  bitmaps.player_state_a = al_create_sub_bitmap(ui, BMP_UI_S * 3, BMP_UI_S, BMP_UI_S * 3, BMP_UI_S);
  bitmaps.turns_left = al_create_sub_bitmap(ui, BMP_UI_S * 6, 0, BMP_UI_S * 2, BMP_UI_S);

  bitmaps.button = al_create_sub_bitmap(ui, 0, 0, BMP_UI_S * 3, BMP_UI_S);
  bitmaps.button_arrow = al_create_sub_bitmap(ui, 0, BMP_UI_S, BMP_UI_S * 3, BMP_UI_S);
  bitmaps.button_a = al_create_sub_bitmap(ui, BMP_UI_S * 3, 0, BMP_UI_S * 3, BMP_UI_S);

  for (int i = 0; i < 5; i++)
  {
    bitmaps.result_bg[i] = al_create_sub_bitmap(ui, BMP_UI_S * 3, BMP_UI_S * (i + 3), BMP_UI_S * 3, BMP_UI_S);
    bitmaps.result_meeple[i] = al_create_sub_bitmap(ui, BMP_UI_S * i, BMP_UI_S * 2, BMP_UI_S, BMP_UI_S);
  }

  bitmaps.splash = al_load_bitmap("./res/splash.png");
  MUST_INIT(bitmaps.splash, "splash image");

  bitmaps.bg = al_load_bitmap("./res/bg.png");
  MUST_INIT(bitmaps.bg, "background image");

  bitmaps.icon = al_load_bitmap("./res/icon.png");
  MUST_INIT(bitmaps.icon, "icon");
}

void res_deinit()
{
  for (int i = 0; i < 24; i++)
    al_destroy_bitmap(bitmaps.tiles[i]);
  al_destroy_bitmap(bitmaps.tile_highlight);

  for (int i = 0; i < 5; i++)
    al_destroy_bitmap(bitmaps.meeple[i]);
  al_destroy_bitmap(bitmaps.coin);

  al_destroy_bitmap(board);

  for (int i = 0; i < 5; i++)
    al_destroy_bitmap(bitmaps.player_state[i]);
  al_destroy_bitmap(bitmaps.player_state_a);
  al_destroy_bitmap(bitmaps.turns_left);

  for (int i = 0; i < 5; i++)
  {
    al_destroy_bitmap(bitmaps.result_bg[i]);
    al_destroy_bitmap(bitmaps.result_meeple[i]);
  }

  al_destroy_bitmap(bitmaps.button);
  al_destroy_bitmap(bitmaps.button_arrow);
  al_destroy_bitmap(bitmaps.button_a);

  al_destroy_bitmap(ui);
  al_destroy_bitmap(bitmaps.splash);
  al_destroy_bitmap(bitmaps.bg);
  al_destroy_bitmap(bitmaps.icon);

  al_destroy_font(fonts.ui);
}
