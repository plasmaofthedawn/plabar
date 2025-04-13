#include "module.h"
#include "config.h"

#include <time.h>
#include <pango/pangocairo.h>


typedef struct module_clock_data {
   char format[100];
   char buffer[100];
} module_clock_data;  

const time_t base_timestamp = 936659599;
struct tm base_time;


void module_clock_init(module_t* out, struct hashmap_s *global_config, struct hashmap_s *local_config) {

   // base time for printing
   // dec 31 1999 23:59:59
   base_time = *localtime(&base_timestamp);

   out->data = malloc(sizeof(module_clock_data));
   module_clock_data *data = out->data;

   char* elem;
   CONFIG_GET_OR_FAIL(local_config, "position", elem, "text");
   out->position = atoi(elem);

   CONFIG_GET_OR_FAIL(local_config, "color", elem, "text")
   color_t color = get_color_from_value(elem, "color", "text");

   char* format;
   CONFIG_GET_OR_FAIL(local_config, "text", format, "text");
   format = get_string_from_value(format, "text", "text");

   // prefer local config, fallback to global config
   char* font_face;
   CONFIG_GET_FALLBACK_OR_FAIL(local_config, global_config, "font-face", font_face, "text");
   font_face = get_string_from_value(font_face, "font-face", "text");

   CONFIG_GET_OR_FAIL(global_config, "height", elem, "global");
   int height = atoi(elem);
   
   strcpy(data->format, format);


   strftime(data->buffer, 100, format, &base_time);
   
   // setup pangocairo + calc width of base 
   if ((elem = hashmap_get(local_config, "width", 5))) {
      out->width = atoi(elem);      
   } else {
      // find text extents
      cairo_t* cr = cairo_create(NULL);
      PangoLayout *layout = pango_cairo_create_layout(cr);

      pango_layout_set_text(layout, data->buffer, -1);

      PangoFontDescription *desc = pango_font_description_from_string(font_face);
      pango_layout_set_font_description(layout, desc);
      pango_font_description_free(desc);

      pango_layout_get_pixel_size(layout, &text_width, &text_height);

      cairo_destroy(cr);

   }   
   
   // prefer local config, fallback to global config
   char* font_face;
   CONFIG_GET_FALLBACK_OR_FAIL(local_config, global_config, "font-face", font_face, "text");
   font_face = get_string_from_value(font_face, "font-face", "text");

   CONFIG_GET_OR_FAIL(global_config, "height", elem, "global");
   int height = atoi(elem);


   int text_width, text_height;





}
