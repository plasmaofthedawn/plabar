#include "module-types.h"
#include "config.h"

const char* MODULE_NAME = "block";

void (*mark_dirty) (module_t*);

void module_init(module_t* out, struct hashmap_s *global_config, struct hashmap_s *local_config) {

   char* elem;
   CONFIG_GET_OR_FAIL(local_config, "width", elem, MODULE_NAME);
   out->width = atoi(elem);

   CONFIG_GET_OR_FAIL(local_config, "position", elem, MODULE_NAME);
   out->position = atoi(elem);

   CONFIG_GET_OR_FAIL(local_config, "color", elem, MODULE_NAME);
   color_t color = get_color_from_value(elem, "color", MODULE_NAME);

   CONFIG_GET_OR_FAIL(global_config, "height", elem, "global");
   int height = atoi(elem);
   
   out->buffer = malloc(sizeof(color_t) * height * out->width);
   
   for (int i = 0; i < height * out->width; i++) {
      out->buffer[i] = color;
   }

   mark_dirty(out);

   //pthread_mutex_unlock(&out->buffer_mutex);
}
