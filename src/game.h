#ifndef __game_inc
#define __game_inc

#include "./board.h"

typedef struct Turn
{
  bool active, skip : 1;
  int x, y;
  Tile tile;
  Meeple meeple;
} Turn;

typedef struct Player
{
  MeepleColor color;
  unsigned int meeple;
  unsigned int points;
  bool bot;
} Player;

typedef struct GameResults
{
  int count;
  Player players[MEEPLE_COLOR_COUNT];
} GameResults;

typedef struct GameConfig
{
  int players;
  int bots;
  void (*on_finish)(GameResults results);
} GameConfig;

void game_init(GameConfig cfg);
void game_deinit();

void game_keydown(int code);
void game_tick(float dt);
void game_render(float w, float h);

void game_pause(bool pause);

#endif
