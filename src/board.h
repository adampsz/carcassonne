#ifndef __board_inc
#define __board_inc

#include <stdbool.h>
#include <stdint.h>

#define TILE_COUNT 72
#define MEEPLE_COLOR_COUNT 5

#define BOARD_CENTER (TILE_COUNT + 1)
#define BOARD_SIZE (2 * BOARD_CENTER + 1)

/**
 * Kolor gracza/podwładnego
 * Pierwszy wariant oznacza 'brak podwładnego'
 */
typedef uint8_t MeepleColor;
enum MeepleColor
{
  MeepleNone,
  MeepleColorGreen,
  MeepleColorRed,
  MeepleColorBlue,
  MeepleColorYellow,
  MeepleColorBlack,
};

/**
 * Podział płytki
 * Każda płytka jest podzielona na 13 fragmentów, a każdy fragment ma przypisany typ (pole/miasto/...)
 * oraz id (pozwalający rozróżnić różne obiekty tego samego typu). Indeksy układają się na płytce następująco:
 *
 *    0 1 2
 * 11       3
 * 10  12   4
 *  9       5
 *    8 7 6
 */
typedef uint8_t TilePos;
enum TilePos
{
  TilePosTL, //    top - left
  TilePosTC, //    top - center
  TilePosTR, //    top - right
  TilePosRT, //  right - top
  TilePosRC, //  right - center
  TilePosRB, //  right - bottom
  TilePosBR, // bottom - right
  TilePosBC, // bottom - center
  TilePosBL, // bottom - left
  TilePosLB, //   left - bottom
  TilePosLC, //   left - center
  TilePosLT, //   left - top
  TilePosCC, // center - center
};

/**
 * Flagi płytki
 * `Starting` oznacza, że dana płytka powinna znajdować się na planszy w momencie rozpoczęcia gry.
 * `Pennant` oznacza biało-niebieską flagę i daje dodatkowe punkty miastom
 */
typedef uint8_t TileFlag;
enum TileFlag
{
  TileFlagStarting = 1 << 0,
  TileFlagPennant = 1 << 1,
};

/**
 * Typy obiektów
 */
typedef uint8_t TileType;
enum TileType
{
  TileTypeNone,
  TileTypeField,
  TileTypeRoad,
  TileTypeCity,
  TileTypeMonastery,
};

/**
 * Flagi renderowania. `Highlight` oznacza podświetlone płytki. `Faded` oznacza nieprawidłowy ruch.
 */
typedef uint8_t RenderFlag;
enum RenderFlag
{
  RenderFlagHighlight = 1 << 0,
  RenderFlagFaded = 1 << 1,
};

/**
 * Podwłądny. Ma on przypisany swój kolor i pozycję na płytce. Brak podwładnego jest oznaczany poprzez ustawienie
 * wartości `color` na `MeepleNone`
 */
typedef struct Meeple
{
  MeepleColor color;
  TilePos pos;
} Meeple;

/**
 * Płytka
 */
typedef uint8_t TileId;
typedef struct Tile
{
  /** podwładny stojący na płytce. Jeżeli na płytce nie stoi podwładny, to jego kolor ma wartość `MeepleNone` */
  Meeple meeple;
  /** Typy obiektów na płytce */
  TileType types[13];
  /** Id obiektów na płytce */
  TileId ids[13];
  /** Flagi płytki */
  TileFlag flags;
  /** rotacja płytki - używana tylko w celu poprawnego wyświetlenia bitmapy */
  uint8_t rot;
  /** id bitmapy */
  uint8_t bitmap;
} Tile;

/**
 * Liczby podwładnych poszczególnych kolorów, którzy stoją w danym obiekcie.
 * Używany np. przy zbieraniu punktów
 */
typedef int MeepleCounts[MEEPLE_COLOR_COUNT + 1];

/** W którym miejscu na płytce można postawić podwłądnego? */
typedef bool MeepleValidPos[13];

/**
 * Współrzędne (x,y) pierwszego podwładnego stojącego w danym obiekcie.
 * Używany w parze z `MeepleCounts`
 */
typedef int CollectMeeplePos[MEEPLE_COLOR_COUNT + 1][2];

// Board methods
void board_init();
void board_deinit();

/**
 * Algorytm BFS chodzący po danym, jednym obiekcie.
 * x, y, pos - wpółrzędne płytki i pozycja, z której algorytm powinien zacząć działanie
 * cb - funkcja wykonywana przy każdym wejściu na nową płytkę
 * data - wskaźnik przekazywany do funkcji `cb`
 * revisit - flaga oznaczająca, że daną płytkę odwiedzamy kolejny raz (np. w przypadku, kiedy
 * na jednej płytce są dwie różne drogi, które jednak łączą się poprzez inne płytki)
 */
typedef void (*board_bfs_cb)(int x, int y, TilePos pos, bool revisit, void *data);
bool board_bfs(int x, int y, TilePos pos, board_bfs_cb cb, void *data);

/** Zwraca wskaźnik do płytki na danych współrzędnych */
Tile *board_tile_get(int x, int y);
/** Sprawdza, czy płytkę można postawić na danych współrzędnych */
bool board_tile_matches(Tile *tile, int x, int y);
/** Sprawdza, czy płytkę można postawić gdziekolwiek na planszy */
bool board_tile_valid(Tile *tile);
/** Stawia płytkę */
void board_tile_place(Tile *tile, int x, int y);
/** Stawia płytkę, ale bez kopiowania jej na wewnętrzny stos. Wykorzystywana przez
 * bota w celu sprawdzenia opłacalności ruchu. W przeciwieństwie do wcześniejszej
 * funkcji, wykonanie tej można cofnąć podając za argument NULL*/
void board_tile_tmp(Tile *tile, int x, int y);

/** Sprawdza, czy podwładnego można postawić na danej płytce */
bool board_meeple_matches(Meeple *meeple, int x, int y);
/** Pyta o wszystkie indeksy, na których można postawić podwładnego na danej płytce */
void board_meeple_valid(Meeple *meeple, int x, int y, MeepleValidPos pos);
/** Stawianie podwładnego */
void board_meeple_place(Meeple *meeple, int x, int y);
/** Liczy/zbiera podwładnych z danego obiektu */
void board_collect_meeple(int x, int y, TilePos pos, bool remove, MeepleCounts meeple, CollectMeeplePos meeple_pos);

void board_render(float x, float y, float s);

// Tile methods
void tile_rotate(Tile *tile);
void tile_render(Tile *t, float x, float y, float s, RenderFlag flags);

// Meeple methods
void meeple_render(Meeple *meeple, float x, float y, float s, RenderFlag flags);

#endif
