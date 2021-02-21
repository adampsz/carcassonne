#ifndef __points_inc
#define __points_inc

#include "./board.h"

typedef void (*collect_points_cb)(int points, MeepleCounts meeple, CollectMeeplePos pos);
void collect_points(int x, int y, bool finish, collect_points_cb cb);
void collect_all_points(bool finish, collect_points_cb cb);

#endif
