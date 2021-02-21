#ifndef __res_inc
#define __res_inc

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>

#define BMP_TILES_S 256
#define BMP_MEEPLE_S (BMP_TILES_S / 2)
#define BMP_UI_S 128
#define FONT_SIZE 24

typedef struct Bitmaps
{
  // Board
  ALLEGRO_BITMAP *tiles[24];
  ALLEGRO_BITMAP *tile_highlight;
  ALLEGRO_BITMAP *meeple[6];
  ALLEGRO_BITMAP *coin;

  // Game state
  ALLEGRO_BITMAP *player_state[5];
  ALLEGRO_BITMAP *player_state_a;
  ALLEGRO_BITMAP *turns_left;

  // Results
  ALLEGRO_BITMAP *result_bg[5];
  ALLEGRO_BITMAP *result_meeple[5];

  // Ui
  ALLEGRO_BITMAP *button;
  ALLEGRO_BITMAP *button_arrow;
  ALLEGRO_BITMAP *button_a;

  // Others
  ALLEGRO_BITMAP *icon;
  ALLEGRO_BITMAP *splash;
  ALLEGRO_BITMAP *bg;
} Bitmaps;

typedef struct Fonts
{
  ALLEGRO_FONT *ui;
} Fonts;

extern Bitmaps bitmaps;
extern Fonts fonts;

void res_init();
void res_deinit();

#endif
