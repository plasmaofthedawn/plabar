#include "cairo.h"
#include "module-types.h"
#include "config.h"

#include <time.h>
#include <pthread.h> 
#include <pango/pangocairo.h>

const struct timespec sleep_time = {.tv_sec = 0, .tv_nsec = 500000000};

const char* MODULE_NAME = "clock";

typedef struct module_clock_data {
   // for strftime
   char buffer[100];

   // render options
   char format[100];
   PangoLayout *layout;
   cairo_surface_t* surface;
   cairo_t* cr;
   int height;
   
} module_clock_data;  

const time_t base_timestamp = 936659599;
struct tm base_time;


void* thread_function(void* param) {
   
   module_t* self = param;
   module_clock_data* data = self->data;
   int text_width, text_height;
   
   struct tm current_time;
   time_t temp;

   for(;;) {

      temp = time(NULL);
      current_time = *localtime(&temp);

      // clear
      cairo_set_operator(data->cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint(data->cr);
      cairo_set_operator(data->cr, CAIRO_OPERATOR_OVER);

      // get current time
      strftime(data->buffer, 100, data->format, &current_time);
      // set text
      pango_layout_set_text(data->layout, data->buffer, -1);
      
      // draw
      pango_layout_get_pixel_size(data->layout, &text_width, &text_height);
      cairo_move_to(data->cr, (self->width - text_width)/2, (data->height - text_height) / 2);
      pango_cairo_show_layout(data->cr, data->layout);   
      self->mark_dirty(self);

      nanosleep(&sleep_time, NULL);

   }  

}


void module_init(module_t* out, struct hashmap_s *global_config, struct hashmap_s *local_config) {

   // base time for printing
   // dec 31 1999 23:59:59
   base_time = *localtime(&base_timestamp);

   // storage
   out->data = malloc(sizeof(module_clock_data));
   module_clock_data *data = out->data;

   // load stuff
   
   char *t, *font_face;
   int width = -1;
   color_t color;

   CONFIG_GET_OR_FAIL(local_config, "position", out->position, "clock");
   CONFIG_GET_OR_FAIL(local_config, "color", color, "clock");
   CONFIG_GET(local_config, "width", width, "clock");

   CONFIG_GET_OR_FAIL(global_config, "height", data->height, "global");
   
   // strings
   CONFIG_GET_OR_FAIL(local_config, "format", t, "clock");
   strcpy(data->format, t);
   CONFIG_GET_FALLBACK_OR_FAIL(local_config, global_config, "font-face", font_face, "clock");

   /// find text extents 
   strftime(data->buffer, 100, data->format, &base_time);
   LOG_DEBUG("%s, %s\n", data->buffer, data->format);
   int text_width, text_height;

   cairo_t* cr = cairo_create(NULL);
   data->layout = pango_cairo_create_layout(cr);

   pango_layout_set_text(data->layout, data->buffer, -1);

   PangoFontDescription *desc = pango_font_description_from_string(font_face);
   pango_layout_set_font_description(data->layout, desc);
   pango_font_description_free(desc);
   pango_layout_get_pixel_size(data->layout, &text_width, &text_height);

   cairo_destroy(cr);

   if (text_height > data->height) {
      LOG_WARN("Text height (%d) is greater than bar height (%d), text may be clipped\n", text_height, data->height);
   }
   
   if (width >= 0) {
      out->width = width;  
   } else {
      out->width = text_width;
   }

   out->buffer = calloc(out->width * data->height * sizeof(color_t), 1);
   
   // create new buffer
   printf("%dx%d\n", out->width, data->height);
   data->surface = cairo_image_surface_create_for_data((unsigned char*) out->buffer, CAIRO_FORMAT_ARGB32, out->width, data->height, sizeof(color_t) * out->width);
   data->cr = cairo_create(data->surface);
  
   cairo_font_options_t* options = cairo_font_options_create();
   cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_SUBPIXEL);
   cairo_set_font_options(data->cr, options);

   pango_cairo_update_layout(data->cr, data->layout);
   
   cairo_set_source_rgba(data->cr, ((color & 0x00FF0000) >> 16) / 255.0, ((color & 0x000000FF00) >> 8) / 255.0, ((color & 0x000000FF)) / 255.0, ((color & 0xFF000000) >> 24) / 255.0); 
   
   pthread_t thread;
   pthread_create(&thread, NULL, &thread_function, out);

}
