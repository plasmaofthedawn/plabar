#include "module-types.h"
#include "config.h"

#include <pango/pangocairo.h>

const char* MODULE_NAME = "pulseaudio";

typedef struct module_pulseaudio_data {
   char buffer[100];

   char format[100];
   PangoLayout *layout;
   cairo_surface_t* surface;
   cairo_t* cr;
   int height;
}

const int BASE_PERCENT = 100.0;



