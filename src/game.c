#include <allegro5/allegro.h>

#include "./board.h"
#include "./bot.h"
#include "./deck.h"
#include "./game.h"
#include "./points.h"
#include "./resources.h"
#include "./spring.h"
#include "./utils.h"

#define PLAYER_COUNT MEEPLE_COLOR_COUNT
#define GAME_UI_S 56

// Method declarations

static void state_finish();
static void state_turn_start();
static void state_turn_end();
static void state_turn_skip();

static void player_turn_start();
static void player_turn_end();
static void player_turn_meeple();

// Globals

/**
 * Lista wszystkich graczy, liczba graczy i aktywny gracz.
 */
static struct Players
{
  Player all[PLAYER_COUNT];
  Player *current;
  int count, index;
} players;

/**
 * Stan gry
 */
static struct State
{
  bool started : 1;
  bool finished : 1;
  bool paused : 1;
  Turn turn;
} state;

/**
 * Konfiguracja gry - liczba graczy, botów
 */
static GameConfig cfg;

// Helpers

/**
 * Animacje zdobywania punktów
 */
#define NUM_COINS 20
static struct Coins
{
  bool part_a[NUM_COINS];
  Spring part_s[NUM_COINS];
  float part_v[NUM_COINS][6];
  int part_idx;
  Spring points[PLAYER_COUNT];
} coins;

/**
 * Zdobywanie punktów
 */
static void game_collect_points_cb(int points, MeepleCounts meeple, CollectMeeplePos meeple_pos)
{
  int max = 0;
  for (int i = 0; i <= MEEPLE_COLOR_COUNT; i++)
    if (max < meeple[i])
      max = meeple[i];

  for (int i = 0; i < players.count; i++)
  {
    struct Player *player = &players.all[i];
    player->meeple += meeple[player->color];
    if (max == 0 || meeple[player->color] != max)
      continue;

    player->points += points;
    coins.points[i].target = player->points + 0.5;

    int ci = coins.part_idx = (coins.part_idx + 1) % NUM_COINS;
    coins.part_a[ci] = true;
    coins.part_s[ci].value = 0;
    coins.part_s[ci].target = 1;
    coins.part_s[ci].velocity = 0;
    coins.part_v[ci][0] = (view.x + meeple_pos[player->color][0] - 0.5) * view.s;
    coins.part_v[ci][1] = GAME_UI_S * 1.5;
    coins.part_v[ci][2] = (view.y + meeple_pos[player->color][1] - 0.5) * view.s;
    coins.part_v[ci][3] = GAME_UI_S * i;
    coins.part_v[ci][4] = view.s;
    coins.part_v[ci][5] = GAME_UI_S;
  }
}

/**
 * Funkcje, które nie wykonują się od razu, tylko po pewnym czasie
 */
static struct Timeout
{
  void (*cb)();
  float seconds;
} timeout;

static void set_timeout(void (*cb)(), float seconds)
{
  timeout.seconds = seconds;
  timeout.cb = cb;
}

// State

static int _cmp_players(const Player *a, const Player *b)
{
  return b->points - a->points;
}

static void state_finish()
{
  state.finished = true;

  GameResults results = {0};
  results.count = players.count;
  for (int i = 0; i < players.count; i++)
    results.players[i] = players.all[i];

  qsort(&results.players, players.count, sizeof(Player), (int (*)(const void *, const void *))_cmp_players);
  cfg.on_finish(results);
}

static void state_turn_start()
{
  state.turn.active = true;
  players.index = (players.index + 1) % players.count;
  players.current = &players.all[players.index];
  state.turn.tile = *deck_pop();
  state.turn.meeple.color = players.current->color;
  state.turn.meeple.pos = TilePosCC;
  view_set(-state.turn.x, -state.turn.y);

  if (!board_tile_valid(&state.turn.tile))
    set_timeout(state_turn_skip, 1.0);
  else if (players.current->bot)
  {
    bot_turn(players.current, &state.turn, deck_size() / players.count);
    view_set(-state.turn.x, -state.turn.y);
    set_timeout(state_turn_end, 1.0);
  }
  else
    player_turn_start();
}

static void state_turn_end()
{
  state.turn.active = false;
  if (state.turn.skip)
    state.turn.skip = false;
  else
  {
    board_tile_place(&state.turn.tile, state.turn.x, state.turn.y);
    if (state.turn.meeple.color != MeepleNone)
    {
      board_meeple_place(&state.turn.meeple, state.turn.x, state.turn.y);
      players.current->meeple--;
    }
  }

  if (deck_size() == 0)
  {
    collect_all_points(true, game_collect_points_cb);
    set_timeout(state_finish, 5.0);
    view_blur();
    return;
  }

  collect_points(state.turn.x, state.turn.y, false, game_collect_points_cb);

  if (players.current->bot)
    set_timeout(state_turn_start, 1.0);
  else
    state_turn_start();
}

void state_turn_skip()
{
  state.turn.skip = true;
  state_turn_end();
}

// Player turn

/**
 * Ruch gracza. Oddzielony od głównej logiki gry
 */
static struct PlayerTurn
{
  bool active : 1;
  enum TurnPhase
  {
    TurnPhaseTile,
    TurnPhaseMeeple
  } phase : 2;
  bool tile_valid_pos : 1;
  MeepleValidPos meeple_valid_pos;
} p_turn;

static void player_turn_start()
{
  p_turn.phase = TurnPhaseTile;
  p_turn.active = true;
}

static void player_turn_end()
{
  p_turn.active = false;
  state_turn_end();
}

static void player_turn_meeple()
{
  p_turn.phase = TurnPhaseMeeple;
  if (players.current->meeple <= 0)
  {
    state.turn.meeple.color = MeepleNone;
    player_turn_end();
  }
  else
    board_meeple_valid(&state.turn.meeple, state.turn.x, state.turn.y, p_turn.meeple_valid_pos);
}

// User interaction

static void keydown_tile(int code)
{
  switch (code)
  {
  case ALLEGRO_KEY_UP:
    state.turn.y--;
    break;
  case ALLEGRO_KEY_DOWN:
    state.turn.y++;
    break;
  case ALLEGRO_KEY_LEFT:
    state.turn.x--;
    break;
  case ALLEGRO_KEY_RIGHT:
    state.turn.x++;
    break;
  case ALLEGRO_KEY_SPACE:
    tile_rotate(&state.turn.tile);
    break;
  case ALLEGRO_KEY_ENTER:
    if (board_tile_matches(&state.turn.tile, state.turn.x, state.turn.y))
    {
      board_tile_tmp(&state.turn.tile, state.turn.x, state.turn.y);
      player_turn_meeple();
    }
  }

  p_turn.tile_valid_pos = board_tile_matches(&state.turn.tile, state.turn.x, state.turn.y);
  view_set(-state.turn.x, -state.turn.y);
}

static void keydown_meeple(int code)
{
  static TilePos POS_MAP[][4] = {
      [TilePosTL] = {TilePosBL, TilePosTC, TilePosLT, TilePosLT},
      [TilePosTC] = {TilePosBC, TilePosTR, TilePosCC, TilePosTL},
      [TilePosTR] = {TilePosBR, TilePosRT, TilePosRT, TilePosTC},
      [TilePosRT] = {TilePosTR, TilePosLT, TilePosRC, TilePosTR},
      [TilePosRC] = {TilePosRT, TilePosLC, TilePosRB, TilePosCC},
      [TilePosRB] = {TilePosRC, TilePosLB, TilePosBR, TilePosBR},
      [TilePosBR] = {TilePosRB, TilePosRB, TilePosTR, TilePosBC},
      [TilePosBC] = {TilePosCC, TilePosBR, TilePosTC, TilePosBL},
      [TilePosBL] = {TilePosLB, TilePosBC, TilePosTL, TilePosLB},
      [TilePosLB] = {TilePosLC, TilePosBL, TilePosBL, TilePosRB},
      [TilePosLC] = {TilePosLT, TilePosCC, TilePosLB, TilePosRC},
      [TilePosLT] = {TilePosTL, TilePosTL, TilePosLC, TilePosRT},
      [TilePosCC] = {TilePosTC, TilePosRC, TilePosBC, TilePosLC},
  };

  switch (code)
  {
  case ALLEGRO_KEY_UP:
    state.turn.meeple.pos = POS_MAP[state.turn.meeple.pos][0];
    break;
  case ALLEGRO_KEY_RIGHT:
    state.turn.meeple.pos = POS_MAP[state.turn.meeple.pos][1];
    break;
  case ALLEGRO_KEY_DOWN:
    state.turn.meeple.pos = POS_MAP[state.turn.meeple.pos][2];
    break;
  case ALLEGRO_KEY_LEFT:
    state.turn.meeple.pos = POS_MAP[state.turn.meeple.pos][3];
    break;
  case ALLEGRO_KEY_BACKSPACE:
    state.turn.meeple.color = MeepleNone;
    player_turn_end();
    break;
  case ALLEGRO_KEY_ENTER:
    if (p_turn.meeple_valid_pos[state.turn.meeple.pos])
      player_turn_end();
    break;
  }
}

void game_keydown(int code)
{
  if (!state.started || state.paused || state.finished)
    return;

  if (p_turn.active)
    p_turn.phase == TurnPhaseTile ? keydown_tile(code) : keydown_meeple(code);

  switch (code)
  {
  case ALLEGRO_KEY_MINUS:
    view_zoom_out();
    break;
  case ALLEGRO_KEY_EQUALS:
    view_zoom_in();
    break;
  }
}

// Init and deinit

void game_pause(bool pause)
{
  state.paused = pause;
}

void game_init(GameConfig config)
{
  cfg = config;

  deck_init();
  board_init();
  deck_shuffle();

  memset(&players, 0, sizeof(players));
  memset(&state, 0, sizeof(state));
  memset(&coins, 0, sizeof(coins));
  memset(&timeout, 0, sizeof(timeout));
  memset(&p_turn, 0, sizeof(p_turn));

  players.count = cfg.players + cfg.bots;
  for (int i = 0; i < players.count; i++)
  {
    players.all[i].color = (MeepleColor)(i + 1);
    players.all[i].meeple = 7;
    players.all[i].points = 0;
  }

  for (int i = 0; i < cfg.bots; i++)
    players.all[players.count - i - 1].bot = true;

  state.turn.x = state.turn.y = BOARD_CENTER;
  board_tile_place(deck_pop(), state.turn.x, state.turn.y);

  for (int i = 0; i < NUM_COINS; i++)
    spring_init(&coins.part_s[i], 1.5, 1);

  for (int i = 0; i < PLAYER_COUNT; i++)
    spring_init(&coins.points[i], 1, 2);

  players.index = -1;
  state.started = true;
  view_focus();
  state_turn_start();
}

void game_deinit()
{
  board_deinit();
  state.started = false;
  view_blur();
}

// Update and render

void game_tick(float dt)
{
  if (!state.started || state.paused)
    return;

  if (timeout.seconds <= 0 && timeout.cb)
  {
    void (*cb)() = timeout.cb;
    timeout.cb = NULL;
    cb();
  }

  if (timeout.seconds > 0)
    timeout.seconds -= dt;

  for (int i = 0; i < NUM_COINS; i++)
    if (coins.part_a[i])
    {
      spring_update(&coins.part_s[i], dt);
      if (spring_at_rest(&coins.part_s[i]))
        coins.part_a[i] = false;
    }

  for (int i = 0; i < PLAYER_COUNT; i++)
    spring_update(&coins.points[i], dt);
}

void game_render(float w, float h)
{
  float bs = view.s;
  float bx = w / 2 + view.x * bs;
  float by = h / 2 + view.y * bs;

  board_render(bx, by, bs);

  if (!state.started)
    return;

  float tx = bx + state.turn.x * bs, ty = by + state.turn.y * bs;

  if (p_turn.active)
  {
    if (p_turn.phase == TurnPhaseTile)
      tile_render(&state.turn.tile, tx, ty, bs, RenderFlagHighlight | (p_turn.tile_valid_pos ? 0 : RenderFlagFaded));
    else
      meeple_render(&state.turn.meeple, tx, ty, bs,
                    p_turn.meeple_valid_pos[state.turn.meeple.pos] ? 0 : RenderFlagFaded);
  }

  for (int i = 0; i < NUM_COINS; i++)
    if (coins.part_a[i])
    {
      float cv = coins.part_s[i].value;
      float cx = (coins.part_v[i][0] + w / 2) * (1 - cv) + coins.part_v[i][1] * cv;
      float cy = (coins.part_v[i][2] + h / 2) * (1 - cv) + coins.part_v[i][3] * cv;
      float cs = coins.part_v[i][4] * (1 - cv) + coins.part_v[i][5] * cv;
      al_draw_tinted_scaled_bitmap(bitmaps.coin, al_map_rgba_f(1 - cv, 1 - cv, 1 - cv, 1 - cv), 0, 0, BMP_MEEPLE_S,
                                   BMP_MEEPLE_S, cx, cy, cs, cs, 0);
    }

  al_hold_bitmap_drawing(true);

  if (!state.finished)
  {
    for (int i = 0; i < players.count; i++)
    {
      Player *player = &players.all[i];

      ALLEGRO_BITMAP *bitmap = bitmaps.player_state[player->color - 1];

      float uy = i * GAME_UI_S;

      al_draw_scaled_bitmap(bitmap, 0, 0, BMP_UI_S * 3 - 1, BMP_UI_S, 0, 0 + uy, GAME_UI_S * 3, GAME_UI_S, 0);
      al_draw_textf(fonts.ui, al_map_rgb_f(1, 1, 1), GAME_UI_S * 1.05, uy + GAME_UI_S * 0.5 - FONT_SIZE * 0.55,
                    ALLEGRO_ALIGN_CENTER, "%d", player->meeple);
      al_draw_textf(fonts.ui, al_map_rgb_f(1, 1, 1), GAME_UI_S * 2.0, uy + GAME_UI_S * 0.5 - FONT_SIZE * 0.55,
                    ALLEGRO_ALIGN_CENTER, "%d", (int)coins.points[i].value);

      if (i == players.index)
        al_draw_scaled_bitmap(bitmaps.player_state_a, 0, 0, BMP_UI_S * 3 - 1, BMP_UI_S, 0, 0 + uy, GAME_UI_S * 3,
                              GAME_UI_S, 0);
    }

    al_draw_scaled_bitmap(bitmaps.turns_left, 0, 0, 2 * BMP_UI_S, BMP_UI_S, w - GAME_UI_S * 2, 0, GAME_UI_S * 2,
                          GAME_UI_S, 0);
    al_draw_textf(fonts.ui, al_map_rgb_f(1, 1, 1), w - GAME_UI_S * 0.65, GAME_UI_S * 0.5 - FONT_SIZE * 0.55,
                  ALLEGRO_ALIGN_CENTER, "%d", deck_size() + (state.turn.active ? 1 : 0));
  }

  al_hold_bitmap_drawing(false);
}
