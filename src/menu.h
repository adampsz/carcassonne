#ifndef __menu_inc
#define __menu_inc

void menu_init();
void menu_deinit();

bool menu_keydown(int code);
void menu_tick(float dt);
void menu_render(float w, float h);

#endif
