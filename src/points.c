#include <stdlib.h>
#include <string.h>

#include "./points.h"
#include "./utils.h"

static int tile_vis[BOARD_SIZE][BOARD_SIZE] = {0};
static int city_vis[BOARD_SIZE][BOARD_SIZE] = {0};

#define TILE_ID(X, Y, POS) board_tile_get(x, y)->ids[POS]

#define TILE_VISITED(X, Y, POS) (bool)(tile_vis[Y][X] & (1 << TILE_ID(X, Y, POS)))
#define TILE_VISIT(X, Y, POS) tile_vis[Y][X] |= 1 << TILE_ID(X, Y, POS)

#define CITY_VISITED(X, Y, POS) (bool)(city_vis[Y][X] & (1 << TILE_ID(X, Y, POS)))
#define CITY_VISIT(X, Y, POS) city_vis[Y][X] |= 1 << TILE_ID(X, Y, POS)
#define CITY_UNVISIT(X, Y, POS) city_vis[Y][X] &= ~(1 << TILE_ID(X, Y, POS))

#define COLLECT_ONCE(X, Y, POS)                                                                                        \
  {                                                                                                                    \
    if (TILE_VISITED(X, Y, POS))                                                                                       \
      return;                                                                                                          \
    TILE_VISIT(X, Y, POS);                                                                                             \
  }

// Field

static void collect_points_field_city_cb(int x, int y, TilePos pos, bool revisit, void *arg)
{
  UNUSED2(revisit, arg);
  CITY_UNVISIT(x, y, pos);
}

static void collect_points_field_cb(int x, int y, TilePos pos, bool revisit, void *arg)
{
  UNUSED2(revisit, arg);
  COLLECT_ONCE(x, y, pos);

  Tile *t = board_tile_get(x, y);
  TileId id = t->ids[pos];

  for (int i = 0; i < 12; i++)
  {
    if (t->types[i] != TileTypeCity)
      continue;
    if (t->ids[(i + 1) % 12] != id && t->ids[(i + 11) % 12] != id)
      continue;

    CITY_VISIT(x, y, t->ids[i]);
  }
}

static int collect_points_field(int x, int y, TilePos pos, bool finish)
{
  if (!finish || TILE_VISITED(x, y, pos))
    return 0;

  int points = 0;
  memset(city_vis, 0, sizeof(city_vis));

  // Phase 1: find all reachable cities
  board_bfs(x, y, pos, (board_bfs_cb)collect_points_field_cb, &points);

  // Phase 2: check whether reachable cities are closed
  for (int y = 0; y < BOARD_SIZE; y++)
    for (int x = 0; x < BOARD_SIZE; x++)
      if (city_vis[y][x])
        for (int pos = 0; pos < 13; pos++)
          if (CITY_VISITED(x, y, pos))
          {
            bool closed = board_bfs(x, y, pos, collect_points_field_city_cb, NULL);
            if (closed)
              points += 3;
          }

  return points;
}

// Road

static void collect_points_road_cb(int x, int y, TilePos pos, bool revisit, int *points)
{
  COLLECT_ONCE(x, y, pos);
  if (revisit)
    return;
  (*points)++;
}

static int collect_points_road(int x, int y, TilePos pos, bool finish)
{
  UNUSED(finish);
  int points = 0;
  bool completed = board_bfs(x, y, pos, (board_bfs_cb)collect_points_road_cb, &points);
  if (!completed && !finish)
    return 0;
  return points;
}

// City

static void collect_points_city_cb(int x, int y, TilePos pos, bool revisit, int *points)
{
  COLLECT_ONCE(x, y, pos);
  if (revisit)
    return;
  Tile *t = board_tile_get(x, y);
  (*points) += t->flags & TileFlagPennant ? 2 : 1;
}

static int collect_points_city(int x, int y, TilePos pos, bool finish)
{
  int points = 0;
  bool completed = board_bfs(x, y, pos, (board_bfs_cb)collect_points_city_cb, &points);
  if (!completed && !finish)
    return 0;

  return finish ? points : points * 2;
}

// Monastery

static int collect_points_monastery(int x, int y, TilePos pos, bool finish)
{
  UNUSED(pos);
  int points = 0;

  for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++)
      if (board_tile_get(x + dx, y + dy))
        points++;

  return finish || points == 9 ? points : 0;
}

static void collect_points_feature(int x, int y, TilePos pos, bool finish, collect_points_cb cb)
{
  Tile *t = board_tile_get(x, y);
  if (!t || !t->ids[pos] || !t->types[pos])
    return;

  int (*COLLECT_MAP[])(int tx, int ty, TilePos pos, bool finish) = {
      [TileTypeField] = collect_points_field,
      [TileTypeRoad] = collect_points_road,
      [TileTypeCity] = collect_points_city,
      [TileTypeMonastery] = collect_points_monastery,
  };

  int points = COLLECT_MAP[t->types[pos]](x, y, pos, finish);

  if (points)
  {
    MeepleCounts meeple;
    CollectMeeplePos meeple_pos;
    board_collect_meeple(x, y, pos, true, meeple, meeple_pos);
    cb(points, meeple, meeple_pos);
  }
}

void collect_points(int x, int y, bool finish, collect_points_cb cb)
{
  Tile *t = board_tile_get(x, y);
  if (!t)
    return;

  memset(tile_vis, 0, sizeof(tile_vis));

  for (int i = 0; i < 13; i++)
    collect_points_feature(x, y, i, finish, cb);

  for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++)
      // Update monasteries
      collect_points_feature(x + dx, y + dy, 12, finish, cb);
}

void collect_all_points(bool finish, collect_points_cb cb)
{
  memset(tile_vis, 0, sizeof(tile_vis));

  for (int y = 0; y < BOARD_SIZE; y++)
    for (int x = 0; x < BOARD_SIZE; x++)
    {
      Tile *t = board_tile_get(x, y);
      if (!t || t->meeple.color == MeepleNone)
        continue;
      collect_points_feature(x, y, t->meeple.pos, finish, cb);
    }
}
