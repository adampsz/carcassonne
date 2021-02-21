#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "./board.h"
#include "./bot.h"

#define RANDOMIZE(X) (X + (1.0 * rand() / RAND_MAX) - 0.5)

typedef struct FeatureIds
{
  int t[BOARD_SIZE][BOARD_SIZE][8];
} FeatureIds;

static int TILE_IDX[5][3] = {{0, -1, 1}, {1, 0, 4}, {0, 1, 7}, {-1, 0, 10}, {0, 0, 12}};

typedef struct Feature
{
  TileType type;
  int id;
  int points;
  float c_prob;
  MeepleCounts meeple;
} Feature;

static Feature features[BOARD_SIZE * 4];
static FeatureIds feature_ids;
static int last_feature_id;

/**
 * Funkcja licząca przybliżone prawdopodobieństwo tego, że do końca gry uda się wylosować
 * Płytkę, która będzie pasować na danym polu. Funkcja ta bierze pod uwagę jedynie liczbę dróg i miast
 * Sąsiadujących z danym polem.
 */
static float tile_probability(int x, int y, int remaining)
{
  static int TILE_COUNTS[5][5] = {
      {4, 2, 17, 4, 1}, {5, 0, 10, 3, 0}, {13, 0, 5, 0, 0}, {4, 3, 0, 0, 0}, {1, 0, 0, 0, 0},
  };

  int cities = 0, roads = 0, unknown = 0;

  for (int i = 0; i < 4; i++)
  {
    int dx = TILE_IDX[i][0], dy = TILE_IDX[i][1], pos = TILE_IDX[i][2];

    Tile *tile = board_tile_get(x + dx, y + dy);
    if (!tile)
    {
      unknown++;
      continue;
    }

    TileType type = tile->types[(pos + 6) % 12];
    if (type == TileTypeCity)
      cities++;
    else if (type == TileTypeRoad)
      roads++;
  }

  int tiles = 0;
  for (int i = 0; i <= unknown; i++)
    for (int j = 0; j <= unknown; j++)
      tiles += TILE_COUNTS[cities + i][roads + j];

  return 1 - pow(1 - 1.0 * tiles / 72, remaining);
}

bool C_PROB[BOARD_SIZE][BOARD_SIZE];
typedef struct FeatureData
{
  Feature *feature;
  FeatureIds *ids;
  int remaining;
} FeatureData;

/** Wylicza bezwzględną oczekiwaną wartość danego obiektu */
static float feature_value(Feature *feat)
{
  int a = feat->points;
  int b = feat->type == TileTypeCity ? a / 2 : a;
  return feat->c_prob * a + (1 - feat->c_prob) * b;
}

/** Wylicza oczekiwaną wartość dla danego gracza. Wartość ta jest dodatnia, ujemna albo zerowa
 * w zależności od liczby podwładnych własnych i przeciwnika. */
static float feature_relative_value(Feature *feat, MeepleColor player)
{
  float value = feature_value(feat);

  int own = 0, opponent = 0;
  for (int i = 0; i <= MEEPLE_COLOR_COUNT; i++)
    if (i == player)
      own = feat->meeple[i];
    else
      opponent = feat->meeple[i] > opponent ? feat->meeple[i] : opponent;

  return own == 0 && opponent == 0 ? 0 : own >= opponent ? value : -value;
}

static void evaluate_feature_cb(int x, int y, TilePos pos, bool revisit, FeatureData *data)
{
  Tile *tile = board_tile_get(x, y);
  Feature *feat = data->feature;
  data->ids->t[y][x][tile->ids[pos]] = feat->id;

  if (!revisit)
    feat->points += feat->type == TileTypeCity ? tile->flags & TileFlagPennant ? 4 : 2 : 1;

  TileId id = tile->ids[pos];
  for (int i = 0; i < 4; i++)
  {
    int dx = TILE_IDX[i][0], dy = TILE_IDX[i][1], pos = TILE_IDX[i][2];
    if (tile->ids[pos] != id || C_PROB[y + dy][x + dx] || board_tile_get(x + dx, y + dy))
      continue;
    C_PROB[y + dy][x + dx] = true;
    feat->c_prob *= tile_probability(x + dx, y + dy, data->remaining);
  }

  if (tile->meeple.color != MeepleNone && tile->ids[tile->meeple.pos] == id)
    feat->meeple[tile->meeple.color]++;
}

/** Wylicza wartość obiektu na danym polu */
static void evaluate_feature(int x, int y, TilePos pos, int remaining, FeatureIds *ids, int id)
{
  Tile *t = board_tile_get(x, y);
  int tid = t->ids[pos];
  ids->t[y][x][tid] = id;
  features[id] = (Feature){.type = t->types[pos], .id = id, .points = 0, .c_prob = 1.0};

  if (features[id].type == TileTypeMonastery)
  {
    for (int dy = -1; dy <= 1; dy++)
      for (int dx = -1; dx <= 1; dx++)
      {
        Tile *t = board_tile_get(x + dx, y + dy);
        if (t)
          features[id].points++;
        else
          features[id].c_prob *= tile_probability(x + dx, y + dy, remaining);
      }
  }
  else
  {
    FeatureData data = {.remaining = remaining, .feature = &features[id], .ids = ids};
    board_bfs(x, y, pos, (board_bfs_cb)evaluate_feature_cb, &data);
  }
}

/* Wylicza wartości wszystkich obiektów w grze */
static void evaluate_all_features(int remaining)
{
  last_feature_id = 0;
  memset(&feature_ids, 0, sizeof(feature_ids));
  for (int y = 1; y < BOARD_SIZE; y++)
    for (int x = 0; x < BOARD_SIZE; x++)
    {
      Tile *tile = board_tile_get(x, y);
      if (tile)
        for (int pos = 0; pos < 13; pos++)
        {
          if (tile->types[pos] == TileTypeField)
            continue;
          int id = tile->ids[pos];
          if (feature_ids.t[y][x][id])
            continue;
          evaluate_feature(x, y, pos, remaining, &feature_ids, ++last_feature_id);
        }
    }
}

typedef struct TurnEvaluationState
{
  FeatureIds included_ids;
  long long int excluded_ids[BOARD_SIZE / 16];
  float ans;
  Meeple best_meeple;
  float best_meeple_value;
  int remaining;
  Player *player;
} TurnEvaluationState;

/** Aktualizuje całkowitą wartość ruchu, poprawiając wartość obiektu znajdującego się w danym miejscu */
void evaluate_turn_helper(int x, int y, TilePos pos, bool meeple, TurnEvaluationState *state)
{
  Tile *tile = board_tile_get(x, y);
  if (!tile || tile->types[pos] == TileTypeField)
    return;

  TileId tid = tile->ids[pos];
  if (!tid)
    return;

  if (!state->included_ids.t[y][x][tid])
  {
    evaluate_feature(x, y, pos, state->remaining, &state->included_ids, ++last_feature_id);
    float value = feature_relative_value(&features[last_feature_id], state->player->color);
    state->ans += RANDOMIZE(value);
    if (!value && meeple && state->player->meeple > 0)
    {
      value = RANDOMIZE(feature_value(&features[last_feature_id]));
      if (value > state->best_meeple_value)
      {
        state->best_meeple.color = state->player->color;
        state->best_meeple.pos = pos;
        state->best_meeple_value = value;
      }
    }
  }

  int fid = feature_ids.t[y][x][tid];
  if (fid && !(state->excluded_ids[fid / 64] & (1 << (fid % 64))))
  {
    float value = feature_relative_value(&features[fid], state->player->color);
    state->ans -= value;
    state->excluded_ids[fid / 64] |= 1 << (fid % 64);
  }
}

/** Oblicza oczekiwany przyrost punktów dla danego ruchu */
static float evaluate_turn(Player *player, Turn *turn, int remaining)
{
  board_tile_tmp(&turn->tile, turn->x, turn->y);

  static TurnEvaluationState state;
  memset(&state, 0, sizeof(state));
  state.remaining = remaining;
  state.player = player;

  int last_feature_id_save = last_feature_id;

  evaluate_turn_helper(turn->x, turn->y, 1, true, &state);
  evaluate_turn_helper(turn->x, turn->y, 4, true, &state);
  evaluate_turn_helper(turn->x, turn->y, 7, true, &state);
  evaluate_turn_helper(turn->x, turn->y, 10, true, &state);
  evaluate_turn_helper(turn->x, turn->y, 12, true, &state);

  evaluate_turn_helper(turn->x, turn->y - 1, 7, false, &state);
  evaluate_turn_helper(turn->x + 1, turn->y, 10, false, &state);
  evaluate_turn_helper(turn->x, turn->y + 1, 1, false, &state);
  evaluate_turn_helper(turn->x - 1, turn->y, 4, false, &state);

  evaluate_turn_helper(turn->x, turn->y - 2, 7, false, &state);
  evaluate_turn_helper(turn->x + 2, turn->y, 10, false, &state);
  evaluate_turn_helper(turn->x, turn->y + 2, 1, false, &state);
  evaluate_turn_helper(turn->x - 2, turn->y, 4, false, &state);

  evaluate_turn_helper(turn->x - 1, turn->y - 1, 4, false, &state);
  evaluate_turn_helper(turn->x - 1, turn->y - 1, 7, false, &state);
  evaluate_turn_helper(turn->x + 1, turn->y - 1, 7, false, &state);
  evaluate_turn_helper(turn->x + 1, turn->y - 1, 10, false, &state);
  evaluate_turn_helper(turn->x + 1, turn->y + 1, 10, false, &state);
  evaluate_turn_helper(turn->x + 1, turn->y + 1, 1, false, &state);
  evaluate_turn_helper(turn->x - 1, turn->y + 1, 1, false, &state);
  evaluate_turn_helper(turn->x - 1, turn->y + 1, 4, false, &state);

  evaluate_turn_helper(turn->x - 1, turn->y - 1, 12, false, &state);
  evaluate_turn_helper(turn->x, turn->y - 1, 12, false, &state);
  evaluate_turn_helper(turn->x + 1, turn->y - 1, 12, false, &state);
  evaluate_turn_helper(turn->x + 1, turn->y, 12, false, &state);
  evaluate_turn_helper(turn->x + 1, turn->y + 1, 12, false, &state);
  evaluate_turn_helper(turn->x, turn->y + 1, 12, false, &state);
  evaluate_turn_helper(turn->x - 1, turn->y + 1, 12, false, &state);
  evaluate_turn_helper(turn->x - 1, turn->y, 12, false, &state);

  board_tile_tmp(NULL, turn->x, turn->y);
  last_feature_id = last_feature_id_save;
  turn->meeple = state.best_meeple;

  return state.ans + state.best_meeple_value;
}

/** Znajduje najbardziej optymalny ruch */
void bot_turn(Player *player, Turn *turn, int remaining)
{
  Turn t = *turn;
  float best_value = -1e3;

  evaluate_all_features(remaining);

  for (int r = 0; r < 4; r++, tile_rotate(&t.tile))
    for (t.y = 1; t.y < BOARD_SIZE; t.y++)
      for (t.x = 1; t.x < BOARD_SIZE; t.x++)
        if (board_tile_matches(&t.tile, t.x, t.y))
        {
          float value = evaluate_turn(player, &t, remaining);
          if (value > best_value)
          {
            best_value = value;
            *turn = t;
          }
        }
}
