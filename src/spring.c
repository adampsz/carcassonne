#include <allegro5/allegro.h>
#include <math.h>
#include <stdbool.h>

#include "./board.h"
#include "./resources.h"
#include "./spring.h"

// Spring

#define SPRING_PI 3.141592653589793
#define SPRING_DT 0.04
#define SPRING_SQ(v) (v * v)

void spring_init(Spring *s, float r, float d)
{
  s->target = s->value = s->velocity = 0;
  spring_set_config(s, r, d);
}

void spring_set_config(Spring *s, float r, float d)
{
  s->k = (4 * SPRING_SQ(SPRING_PI)) / SPRING_SQ(d);
  s->c = 2 * r * sqrt(s->k);
}

void spring_update(Spring *s, float dt)
{
  if (spring_at_rest(s))
  {
    s->value = s->target;
    s->velocity = 0;
    return;
  }

  float a;

  while (dt > 0)
  {
    a = s->k * (s->value - s->target) + s->c * s->velocity;
    s->velocity -= a * (dt > SPRING_DT ? SPRING_DT : dt);
    s->value += s->velocity * (dt > SPRING_DT ? SPRING_DT : dt);
    dt -= SPRING_DT;
  }
}

// View

View view;

static struct ViewSprings
{
  Spring x, y, s;
} view_springs;

void view_init()
{
  view_springs.x.value = -BOARD_CENTER;
  view_springs.y.value = -BOARD_CENTER;
  view_springs.s.value = 32;
  view_blur();
}

void view_tick(float dt)
{
  spring_update(&view_springs.x, dt);
  spring_update(&view_springs.y, dt);
  spring_update(&view_springs.s, dt);

  view.x = view_springs.x.value;
  view.y = view_springs.y.value;
  view.s = view_springs.s.value;
}

void view_set(int x, int y)
{
  view_springs.x.target = x;
  view_springs.y.target = y;
}

void view_zoom_in()
{
  if (view_springs.s.target < 256)
    view_springs.s.target *= 2;
}

void view_zoom_out()
{
  if (view_springs.s.target > 8)
    view_springs.s.target /= 2;
}

void view_focus()
{
  spring_set_config(&view_springs.x, 1, 1);
  spring_set_config(&view_springs.y, 1, 1);
  spring_set_config(&view_springs.s, 1, 1);

  view_springs.x.target = -BOARD_CENTER;
  view_springs.y.target = -BOARD_CENTER;
  view_springs.s.target = 64;
}

void view_blur()
{
  spring_set_config(&view_springs.x, 1, 10);
  spring_set_config(&view_springs.y, 1, 10);
  spring_set_config(&view_springs.s, 1, 10);

  view_springs.x.target = (view_springs.x.target - BOARD_CENTER) / 2;
  view_springs.y.target = (view_springs.y.target - BOARD_CENTER) / 2;
  view_springs.s.target = 32;
}
