#include "module-types.h"
#include "config.h"
#include "cairo.h"

#include <pango/pangocairo.h>

#include <stdio.h>

char* MODULE_NAME = "text";

void module_init(module_t* out, struct hashmap_s *global_config, struct hashmap_s *local_config) {
   
   color_t color;
   char *text, *font_face;
   int width = -1, height;

   CONFIG_GET_OR_FAIL(local_config, "position", out->position, "text");
   CONFIG_GET_OR_FAIL(local_config, "color", color, "text");
   CONFIG_GET_OR_FAIL(local_config, "text", text, "text");
   CONFIG_GET_FALLBACK_OR_FAIL(local_config, global_config, "font-face", font_face, "text");
   CONFIG_GET(local_config, "width", height, "global");
   CONFIG_GET_OR_FAIL(global_config, "height", height, "global");

   int text_width, text_height;

   // find text extents
   cairo_t* cr = cairo_create(NULL);
   PangoLayout *layout = pango_cairo_create_layout(cr);

   pango_layout_set_text(layout, text, -1);

   PangoFontDescription *desc = pango_font_description_from_string(font_face);
   pango_layout_set_font_description(layout, desc);
   pango_font_description_free(desc);

   pango_layout_get_pixel_size(layout, &text_width, &text_height);

   cairo_destroy(cr);

   if (text_height > height) {
      LOG_WARN("Text height (%d) is greater than bar height (%d), text may be clipped\n", text_height, height);
   }

   // use it to set width if needed
   if (width >= 0) {
      out->width = width;      
   } else {
      out->width = text_width;
   }

   LOG_DEBUG("text size %dx%d\n", text_width, text_height);

   
   out->buffer = calloc(out->width * height * sizeof(color_t), 1);
   //out->buffer = malloc(out->width * height * sizeof(color_t));

   LOG_DEBUG("asas%d\n", cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, out->width));

   // create new buffer
   cairo_surface_t* surface = cairo_image_surface_create_for_data((unsigned char*) out->buffer, CAIRO_FORMAT_ARGB32, out->width, height, sizeof(color_t) * out->width);
   cr = cairo_create(surface);
  
   cairo_font_options_t* options = cairo_font_options_create();
   cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_SUBPIXEL);
   cairo_set_font_options(cr, options);

   pango_cairo_update_layout(cr, layout);

      //cairo_font_options_destroy(options);

   LOG_DEBUG("qq%s %s\n", font_face, text);
   
   //hell
   cairo_set_source_rgba(cr, ((color & 0x00FF0000) >> 16) / 255.0, ((color & 0x000000FF00) >> 8) / 255.0, ((color & 0x000000FF)) / 255.0, ((color & 0xFF000000) >> 24) / 255.0); 

   // draw
   cairo_move_to(cr, 0, (height - text_height) / 2);
   pango_cairo_show_layout(cr, layout);
   
   //free 
   cairo_destroy(cr);
   g_object_unref(layout);
   cairo_surface_destroy(surface);
   
   out->mark_dirty(out);

}
