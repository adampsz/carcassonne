#include <stdlib.h>

#include "./board.h"
#include "./resources.h"
#include "./utils.h"

/**
 * Plansza
 * Ponieważ na planszy można ułożyć 72 płytki, a potencjalnie mogą one być ułożone w jednym rzędzie,
 * musi ona zmieścić 72 płytki w każdą stronę od środka.
 * Płytki są trzymane w tablicy, a plansza przechowuje jedynie wskaźniki. Ułatwia to sprawdzanie, czy w danym polu
 * leży płytka oraz oszczędza miejsce.
 */
static struct Board
{
  Tile tiles[TILE_COUNT];
  Tile *grid[BOARD_SIZE + 1][BOARD_SIZE + 1];
  size_t tile_count;
} board;

// Helpers

#define TILE_MATCHES(A, B, AI, BI)                                                                                     \
  ((A->types[AI + 0] == B->types[BI + 2]) && (A->types[AI + 1] == B->types[BI + 1]) &&                                 \
   (A->types[AI + 2] == B->types[BI + 0]))

#define TILE_MATCHES_V(UP, DOWN) TILE_MATCHES(UP, DOWN, 6, 0)
#define TILE_MATCHES_H(LEFT, RIGHT) TILE_MATCHES(LEFT, RIGHT, 3, 9)

#define TILE_AT(X, Y) board.grid[Y][X]

// BFS

#define IS_VISITED(X, Y, ID) (bool)(vis[Y][X] & (1 << (ID)))
#define MARK_VISITED(X, Y, ID) vis[Y][X] |= (1 << (ID))

#define BOARD_BFS_HELPER(T, X, Y, ID, AI, BI)                                                                          \
  {                                                                                                                    \
    Tile *U = TILE_AT(X, Y);                                                                                           \
    for (int i = 0; i < 3; i++)                                                                                        \
      if (T->ids[AI + 2 - i] == ID)                                                                                    \
      {                                                                                                                \
        if (!U)                                                                                                        \
        {                                                                                                              \
          completed = false;                                                                                           \
          break;                                                                                                       \
        }                                                                                                              \
        else if (T->types[AI + 2 - i] == U->types[BI + i])                                                             \
        {                                                                                                              \
          TileId id = U->ids[BI + i];                                                                                  \
                                                                                                                       \
          if (IS_VISITED(X, Y, id))                                                                                    \
            break;                                                                                                     \
                                                                                                                       \
          int idx = (qf + qs) % BOARD_SIZE;                                                                            \
          QX[idx] = X;                                                                                                 \
          QY[idx] = Y;                                                                                                 \
          QP[idx] = BI + i;                                                                                            \
          qs++;                                                                                                        \
          break;                                                                                                       \
        }                                                                                                              \
      }                                                                                                                \
  }

bool board_bfs(int x, int y, TilePos pos, board_bfs_cb cb, void *data)
{
  Tile *t = TILE_AT(x, y);

  if (!t)
    return true;

  static int QX[BOARD_SIZE];
  static int QY[BOARD_SIZE];
  static TileId QP[BOARD_SIZE];
  static int vis[BOARD_SIZE][BOARD_SIZE];

  memset(vis, 0, sizeof(vis));

  int qf = 0, qs = 1;
  bool completed = true;

  QX[0] = x;
  QY[0] = y;
  QP[0] = pos;

  while (qs > 0)
  {
    int x = QX[qf], y = QY[qf], pos = QP[qf];
    Tile *t = TILE_AT(x, y);
    TileId id = t->ids[pos];

    qf = (qf + 1) % BOARD_SIZE;
    qs--;

    if (cb)
      cb(x, y, pos, vis[y][x], data);

    MARK_VISITED(x, y, id);

    BOARD_BFS_HELPER(t, x, y - 1, id, 0, 6);
    BOARD_BFS_HELPER(t, x, y + 1, id, 6, 0);
    BOARD_BFS_HELPER(t, x - 1, y, id, 9, 3);
    BOARD_BFS_HELPER(t, x + 1, y, id, 3, 9);
  }

  return completed;
}

// Meeple collecting

typedef struct CollectMeepleData
{
  bool remove;
  MeepleCounts meeple;
  CollectMeeplePos meeple_pos;
} CollectMeepleData;

static void board_collect_meeple_cb(int x, int y, TilePos pos, bool revisit, CollectMeepleData *data)
{
  UNUSED(revisit);
  Tile *t = TILE_AT(x, y);
  TileId id = t->ids[pos];

  if (t->meeple.color == MeepleNone || t->ids[t->meeple.pos] != id)
    return;

  data->meeple[t->meeple.color]++;
  data->meeple_pos[t->meeple.color][0] = x;
  data->meeple_pos[t->meeple.color][1] = y;

  if (data->remove)
    t->meeple.color = MeepleNone;
}

void board_collect_meeple(int x, int y, TilePos pos, bool remove, MeepleCounts meeple, CollectMeeplePos meeple_pos)
{
  Tile *tile = TILE_AT(x, y);
  if (!tile)
    return;

  CollectMeepleData data = {0};
  data.remove = remove;
  board_bfs(x, y, pos, (board_bfs_cb)board_collect_meeple_cb, (void *)&data);

  if (meeple)
    memcpy(meeple, data.meeple, sizeof(MeepleCounts));
  if (meeple_pos)
    memcpy(meeple_pos, data.meeple_pos, sizeof(CollectMeeplePos));
}

// Tile methods

void tile_rotate(Tile *t)
{
  TileType types[12];
  memcpy(types, t->types, 12 * sizeof(TileType));
  memcpy(t->types, types + 9, 3 * sizeof(TileType));
  memcpy(t->types + 3, types, 9 * sizeof(TileType));

  TileId ids[12];
  memcpy(ids, t->ids, 12 * sizeof(TileId));
  memcpy(t->ids, ids + 9, 3 * sizeof(TileId));
  memcpy(t->ids + 3, ids, 9 * sizeof(TileId));

  t->rot = (t->rot + 1) & 3;
}

Tile *board_tile_get(int x, int y)
{
  return TILE_AT(x, y);
}

bool board_tile_matches(Tile *t, int x, int y)
{
  if (TILE_AT(x, y))
    return false;

  Tile *up = board.grid[y - 1][x];
  Tile *down = board.grid[y + 1][x];
  Tile *left = board.grid[y][x - 1];
  Tile *right = board.grid[y][x + 1];

  if (!up && !down && !left && !right)
    return false;
  if (up && !TILE_MATCHES_V(up, t))
    return false;
  if (down && !TILE_MATCHES_V(t, down))
    return false;
  if (left && !TILE_MATCHES_H(left, t))
    return false;
  if (right && !TILE_MATCHES_H(t, right))
    return false;

  return true;
}

bool board_tile_valid(Tile *tile)
{
  Tile tmp = *tile;
  for (int r = 0; r < 4; r++)
  {
    for (int y = 1; y < BOARD_SIZE; y++)
      for (int x = 1; x < BOARD_SIZE; x++)
        if (board_tile_matches(&tmp, x, y))
          return true;
    tile_rotate(&tmp);
  }
  return false;
}

void board_tile_place(Tile *tile, int x, int y)
{
  size_t idx = board.tile_count++;
  board.tiles[idx] = *tile;
  TILE_AT(x, y) = &board.tiles[idx];
}

void board_tile_tmp(Tile *tile, int x, int y)
{
  TILE_AT(x, y) = tile;
}

// Meeple methods

void board_meeple_place(Meeple *m, int x, int y)
{
  Tile *t = TILE_AT(x, y);
  if (t)
    t->meeple = *m;
}

bool board_meeple_matches(Meeple *m, int x, int y)
{
  static MeepleCounts meeple;

  Tile *t = TILE_AT(x, y);
  if (!t)
    return false;

  board_collect_meeple(x, y, m->pos, false, meeple, NULL);

  for (int j = 0; j <= MEEPLE_COLOR_COUNT; j++)
    if (meeple[j])
      return false;

  return true;
}

void board_meeple_valid(Meeple *m, int x, int y, MeepleValidPos pos)
{
  Tile *t = TILE_AT(x, y);

  static bool checked[13];
  static bool valid[13];
  memset(checked, false, sizeof(checked));
  memset(valid, true, sizeof(checked));

  TilePos mp = m->pos;

  for (int i = 0; i < 13; i++)
  {
    if (!t || !t->ids[i])
    {
      pos[i] = false;
      continue;
    }

    m->pos = i;
    if (!checked[t->ids[i]])
      valid[t->ids[i]] = board_meeple_matches(m, x, y);

    pos[i] = valid[t->ids[i]];
  }

  m->pos = mp;
}

// Init and deinit

void board_init()
{
  board.tile_count = 0;

  for (int y = 0; y < BOARD_SIZE; y++)
    for (int x = 0; x < BOARD_SIZE; x++)
      TILE_AT(x, y) = NULL;
}

void board_deinit()
{
  memset(&board, 0, sizeof(board));
}

// Rendering

void tile_render(Tile *t, float x, float y, float s, RenderFlag flags)
{
  ALLEGRO_COLOR tint = flags & RenderFlagFaded ? al_map_rgba_f(0.6, 0.6, 0.6, 1) : al_map_rgb_f(1, 1, 1);

  al_draw_tinted_scaled_rotated_bitmap(bitmaps.tiles[t->bitmap], tint, BMP_TILES_S / 2, BMP_TILES_S / 2, x, y,
                                       s / BMP_TILES_S, s / BMP_TILES_S, t->rot * ALLEGRO_PI / 2, 0);

  if (flags & RenderFlagHighlight)
    al_draw_scaled_bitmap(bitmaps.tile_highlight, 0, 0, BMP_TILES_S * 2, BMP_TILES_S * 2, x - s, y - s, 2 * s, 2 * s,
                          0);

  if (t->meeple.color != MeepleNone)
    meeple_render(&t->meeple, x, y, s, 0);
}

void meeple_render(Meeple *m, float x, float y, float s, RenderFlag flags)
{
  float MP_X[] = {.30, .50, .70, .85, .85, .85, .70, .50, .30, .15, .15, .15, .50};
  float MP_Y[] = {.15, .15, .15, .30, .50, .70, .85, .85, .85, .70, .50, .30, .50};

  float ms = s / 3;

  ALLEGRO_COLOR tint = flags & RenderFlagFaded ? al_map_rgba_f(0.4, 0.4, 0.4, 0.6) : al_map_rgb_f(1, 1, 1);

  al_draw_tinted_scaled_bitmap(bitmaps.meeple[m->color - 1], tint, 0, 0, BMP_MEEPLE_S, BMP_MEEPLE_S,
                               x + MP_X[m->pos] * s - ms / 2 - s / 2, y + MP_Y[m->pos] * s - ms / 2 - s / 2, ms, ms, 0);
}

void board_render(float bx, float by, float s)
{
  al_hold_bitmap_drawing(true);

  for (int ty = 0; ty < BOARD_SIZE; ty += 8)
    for (int tx = 0; tx < BOARD_SIZE; tx += 8)
      al_draw_scaled_bitmap(bitmaps.bg, 0, 0, 512, 512, bx + tx * s - s / 2, by + ty * s - s / 2, 8 * s, 8 * s, 0);

  for (int ty = 0; ty < BOARD_SIZE; ty++)
    for (int tx = 0; tx < BOARD_SIZE; tx++)
      if (board.grid[ty][tx])
        tile_render(board.grid[ty][tx], bx + tx * s, by + ty * s, s, 0);

  al_hold_bitmap_drawing(false);
}
