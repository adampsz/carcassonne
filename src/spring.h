#ifndef __spring_inc
#define __spring_inc

// Spring

typedef struct Spring
{
  float value, target;
  float velocity;
  float k, c;
} Spring;

#define __S_BETWEEN(V, E) (-(E) < (V) && (V) < (E))

#define SPRING_EPS 0.01
#define spring_at_rest(S) (__S_BETWEEN((S)->value - (S)->target, SPRING_EPS) && __S_BETWEEN((S)->velocity, SPRING_EPS))

void spring_init(Spring *spring, float ratio, float duration_s);
void spring_set_config(Spring *spring, float ratio, float duration_s);
void spring_update(Spring *spring, float dt);

// View

typedef struct View
{
  float x, y, s;
} View;

extern View view;

void view_init();
void view_tick(float dt);
void view_render();

void view_set(int x, int y);
void view_zoom_in();
void view_zoom_out();
void view_focus();
void view_blur();

#endif
