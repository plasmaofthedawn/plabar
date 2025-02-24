#ifndef INCLUDE_PLABAR_WAYLANDH
#define INCLUDE_PLABAR_WAYLANDH

#include <stdint.h>

#include "config.h"

void create_window(unsigned int width, unsigned int height, direction_t anchor_pos);
uint32_t* get_pixel_buffer();

void update_window(int x, int y, int w, int h);

void window_loop();

#endif // DEBUG
