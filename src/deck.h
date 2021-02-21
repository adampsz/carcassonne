#ifndef __deck_inc
#define __deck_inc

#include "./board.h"

void deck_init();
void deck_deinit();
void deck_shuffle();
void deck_push(int count, Tile *t);
int deck_size();
Tile *deck_pop();

#endif
