#include <stdlib.h>
#include <string.h>

#include "./deck.h"

static struct Deck
{
  Tile tiles[TILE_COUNT];
  int size;
} deck;

#define TILE_ID_HELPER(DEF, IDS, LAST_ID, TYPE, ID)                                                                    \
  {                                                                                                                    \
    const TileType TYPE_MAP[] = {                                                                                      \
        ['F'] = TileTypeField,                                                                                         \
        ['R'] = TileTypeRoad,                                                                                          \
        ['C'] = TileTypeCity,                                                                                          \
        ['M'] = TileTypeMonastery,                                                                                     \
    };                                                                                                                 \
                                                                                                                       \
    char *def = DEF;                                                                                                   \
    if (def[0] == '.')                                                                                                 \
      continue;                                                                                                        \
                                                                                                                       \
    TYPE = TYPE_MAP[(size_t)def[0]];                                                                                   \
                                                                                                                       \
    TileId *id = &IDS[TYPE][(size_t)def[1] - '0'];                                                                     \
    if (!*id)                                                                                                          \
      *id = ++LAST_ID;                                                                                                 \
    ID = *id;                                                                                                          \
  }

#define TILE_FLAG_HELPER(FLAG, FLAGS)                                                                                  \
  if (FLAG != '.')                                                                                                     \
  {                                                                                                                    \
    const TileFlag FLAG_MAP[] = {                                                                                      \
        ['S'] = TileFlagStarting,                                                                                      \
        ['P'] = TileFlagPennant,                                                                                       \
    };                                                                                                                 \
    FLAGS |= FLAG_MAP[(size_t)FLAG];                                                                                   \
  }

static Tile tile_make(char defs[3 * 13 + 1], char flags[1 + 1], int bitmap)
{
  Tile tile = {0};
  tile.bitmap = bitmap;

  TileId ids[5][5] = {0};
  memset(ids, 0, sizeof(ids));
  TileId last_id = 0;

  for (int i = 0; i < 13; i++)
    TILE_ID_HELPER(&defs[i * 3], ids, last_id, tile.types[i], tile.ids[i]);

  for (int i = 0; flags[i]; i++)
    TILE_FLAG_HELPER(flags[i], tile.flags);

  return tile;
}

static void deck_swap(int i, int j)
{
  Tile tmp;
  tmp = deck.tiles[j];
  deck.tiles[j] = deck.tiles[i];
  deck.tiles[i] = tmp;
}

void deck_shuffle()
{
  for (int i = 0; i < deck.size; i++)
    deck_swap(i, rand() % deck.size);

  for (int i = 0; i < deck.size; i++)
    if (deck.tiles[i].flags & TileFlagStarting)
    {
      deck_swap(i, deck.size - 1);
      break;
    }
}

void deck_push(int count, Tile *t)
{
  for (int i = 0; i < count; i++)
    deck.tiles[deck.size++] = *t;
}

int deck_size()
{
  return deck.size;
}

Tile *deck_pop()
{
  return deck.size ? &deck.tiles[--deck.size] : NULL;
}

#define T(C, DU, DR, DD, DL, DC, F, B)                                                                                 \
  {                                                                                                                    \
    tile = tile_make(DU " " DR " " DD " " DL " " DC, F, B);                                                            \
    deck_push(C, &tile);                                                                                               \
  }

void deck_init()
{
  deck.size = 0;

  Tile tile;

  T(4, "F1 F1 F1", "F1 F1 F1", "F1 F1 F1", "F1 F1 F1", "M1", ".", 0);
  T(2, "F1 F1 F1", "F1 F1 F1", "F1 R1 F1", "F1 F1 F1", "M1", ".", 1);
  T(8, "F1 F1 F1", "F1 R1 F2", "F2 F2 F2", "F2 R1 F1", "R1", ".", 2);
  T(9, "F1 F1 F1", "F1 F1 F1", "F1 R1 F2", "F2 R1 F1", "R1", ".", 3);
  T(4, "F1 F1 F1", "F1 R1 F2", "F2 R2 F3", "F3 R3 F1", "..", ".", 4);
  T(1, "F1 R1 F2", "F2 R2 F3", "F3 R3 F4", "F4 R4 F1", "..", ".", 5);
  T(5, "C1 C1 C1", "F1 F1 F1", "F1 F1 F1", "F1 F1 F1", "F1", ".", 6);
  T(1, "C1 C1 C1", "F1 R1 F2", "F2 F2 F2", "F2 R1 F1", "R1", "S", 7);
  T(3, "C1 C1 C1", "F1 R1 F2", "F2 F2 F2", "F2 R1 F1", "R1", ".", 7);
  T(3, "C1 C1 C1", "F1 F1 F1", "F1 R1 F2", "F2 R1 F1", "R1", ".", 8);
  T(3, "C1 C1 C1", "F1 R1 F2", "F2 R1 F1", "F1 F1 F1", "R1", ".", 9);
  T(3, "C1 C1 C1", "F1 R1 F2", "F2 R2 F3", "F3 R3 F1", "..", ".", 10);
  T(1, "F1 F1 F1", "C1 C1 C1", "F2 F2 F2", "C1 C1 C1", "C1", ".", 11);
  T(2, "F1 F1 F1", "C1 C1 C1", "F2 F2 F2", "C1 C1 C1", "C1", "P", 12);
  T(3, "C1 C1 C1", "C1 C1 C1", "F1 F1 F1", "F1 F1 F1", "..", ".", 13);
  T(2, "C1 C1 C1", "C1 C1 C1", "F1 F1 F1", "F1 F1 F1", "..", "P", 14);
  T(3, "C1 C1 C1", "F1 F1 F1", "C2 C2 C2", "F1 F1 F1", "F1", ".", 15);
  T(2, "C1 C1 C1", "C2 C2 C2", "F1 F1 F1", "F1 F1 F1", "F1", ".", 16);
  T(3, "C1 C1 C1", "C1 C1 C1", "F1 R1 F2", "F2 R1 F1", "..", ".", 17);
  T(2, "C1 C1 C1", "C1 C1 C1", "F1 R1 F2", "F2 R1 F1", "..", "P", 18);
  T(3, "C1 C1 C1", "C1 C1 C1", "F1 F1 F1", "C1 C1 C1", "C1", ".", 19);
  T(1, "C1 C1 C1", "C1 C1 C1", "F1 F1 F1", "C1 C1 C1", "C1", "P", 20);
  T(1, "C1 C1 C1", "C1 C1 C1", "F1 R1 F2", "C1 C1 C1", "C1", ".", 21);
  T(2, "C1 C1 C1", "C1 C1 C1", "F1 R1 F2", "C1 C1 C1", "C1", "P", 22);
  T(1, "C1 C1 C1", "C1 C1 C1", "C1 C1 C1", "C1 C1 C1", "C1", "P", 23);
}
