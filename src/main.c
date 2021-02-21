#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>

#include "./game.h"
#include "./menu.h"
#include "./resources.h"
#include "./utils.h"

#define NAME "Carcassonne"

// Display
#define DISPLAY_W 960
#define DISPLAY_H 540
#define FPS 60

ALLEGRO_DISPLAY *display;
ALLEGRO_TIMER *timer;
ALLEGRO_EVENT_QUEUE *queue;

static struct
{
  float w, h;
} display_size;

void resize()
{
  al_acknowledge_resize(display);

  int h = al_get_display_height(display);
  int w = al_get_display_width(display);
  float s = (float)h / DISPLAY_H;

  display_size.w = w / s;
  display_size.h = h / s;

  static ALLEGRO_TRANSFORM tr;
  al_identity_transform(&tr);
  al_scale_transform(&tr, s, s);

  al_use_transform(&tr);
}

int main()
{
  int t = time(NULL);
  // printf("random seed: %d\n", t);
  srand(t);

  // Init
  MUST_INIT(al_init(), "allegro");
  MUST_INIT(al_install_keyboard(), "keyboard");
  MUST_INIT(al_init_primitives_addon(), "primitives addon");
  MUST_INIT(al_init_image_addon(), "image addon");
  MUST_INIT(al_init_font_addon(), "font addon");
  MUST_INIT(al_init_ttf_addon(), "ttf addon");

  ALLEGRO_PATH *resource_path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
  al_change_directory(al_path_cstr(resource_path, ALLEGRO_NATIVE_PATH_SEP));
  al_destroy_path(resource_path);

  // Display
  al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
  al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
  al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_RESIZABLE);

  display = al_create_display(DISPLAY_W, DISPLAY_H);
  MUST_INIT(display, "display");

  al_set_window_title(display, NAME);
  // al_set_window_constraints(display, DISPLAY_W / 10, DISPLAY_H / 10, 1e9, 1e9);
  // al_apply_window_constraints(display, true);
  // al_hide_mouse_cursor(display);

  // Other initialisations
  timer = al_create_timer(1.0 / FPS);
  queue = al_create_event_queue();
  MUST_INIT(timer, "timer");
  MUST_INIT(queue, "event queue");

  al_register_event_source(queue, al_get_display_event_source(display));
  al_register_event_source(queue, al_get_timer_event_source(timer));
  al_register_event_source(queue, al_get_keyboard_event_source());

  res_init();
  menu_init();
  al_set_display_icon(display, bitmaps.icon);

  // Main game loop
  ALLEGRO_EVENT evt;
  bool redraw = false, exit = false;

  resize();
  al_start_timer(timer);
  while (!exit)
  {
    al_wait_for_event(queue, &evt);

    switch (evt.type)
    {
    case ALLEGRO_EVENT_DISPLAY_CLOSE:
      exit = true;
      break;

    case ALLEGRO_EVENT_DISPLAY_RESIZE:
      resize();
      break;

    case ALLEGRO_EVENT_TIMER:
      redraw = true;
      break;

    case ALLEGRO_EVENT_KEY_DOWN:
      if (menu_keydown(evt.keyboard.keycode))
        exit = true;
      game_keydown(evt.keyboard.keycode);
      break;
    }

    if (redraw && al_is_event_queue_empty(queue))
    {
      al_clear_to_color(al_map_rgb(0, 0, 0));
      game_render(display_size.w, display_size.h);
      menu_render(display_size.w, display_size.h);
      al_flip_display();

      static double prev_time = 0;
      double current_time = al_get_time();

      if (prev_time)
      {
        float dt = current_time - prev_time;
        game_tick(dt);
        menu_tick(dt);
      }

      prev_time = current_time;

      /*static int fps = 0;
      static double prev_fps_time = 0;

      fps++;
      if (current_time - prev_fps_time > 1)
      {
        printf("FPS: %.2f\n", fps / (current_time - prev_fps_time));
        fps = 0;
        prev_fps_time = current_time;
      }*/

      redraw = false;
    }
  }

  menu_deinit();
  al_destroy_timer(timer);
  al_destroy_event_queue(queue);
  al_destroy_display(display);
  res_deinit();

  al_shutdown_ttf_addon();
  al_shutdown_font_addon();
  al_shutdown_image_addon();
  al_shutdown_primitives_addon();
  al_uninstall_keyboard();
  al_uninstall_system();

  return 0;
}
