#include "../module.h"
#include "../config.h"

void module_block_init(module_t* out, struct hashmap_s *global_config, struct hashmap_s *local_config) {

   // i don't know why...
   //pthread_mutex_unlock(&out->buffer_mutex);
   //printf("module_block_init\n");

   char* elem;
   CONFIG_GET_OR_FAIL(local_config, "width", elem, "block");
   out->width = atoi(elem);

   CONFIG_GET_OR_FAIL(local_config, "position", elem, "block");
   out->position = atoi(elem);

   CONFIG_GET_OR_FAIL(local_config, "color", elem, "block");
   color_t color = get_color_from_value(elem, "color", "block");

   CONFIG_GET_OR_FAIL(global_config, "height", elem, "global");
   int height = atoi(elem);
   
   out->buffer = malloc(sizeof(color_t) * height * out->width);
   
   for (int i = 0; i < height * out->width; i++) {
      out->buffer[i] = color;
   }

   mark_dirty(out);

   //pthread_mutex_unlock(&out->buffer_mutex);
}
