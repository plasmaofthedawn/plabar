#include "module-types.h"
#include "config.h"

const char* MODULE_NAME = "block";

void (*mark_dirty) (module_t*);

void module_init(module_t* out, struct hashmap_s *global_config, struct hashmap_s *local_config) {

   int height;
   color_t color;

   CONFIG_GET_OR_FAIL(local_config, "width", out->width, MODULE_NAME);
   CONFIG_GET_OR_FAIL(local_config, "position", out->position, MODULE_NAME);
   CONFIG_GET_OR_FAIL(local_config, "color", color, MODULE_NAME);
   CONFIG_GET_OR_FAIL(global_config, "height", height, "global");
   
   out->buffer = malloc(sizeof(color_t) * height * out->width);
   
   for (int i = 0; i < height * out->width; i++) {
      out->buffer[i] = color;
   }

   mark_dirty(out);
}
