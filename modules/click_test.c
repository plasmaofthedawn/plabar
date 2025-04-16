#include "module-types.h"
#include "config.h"

#include <linux/input-event-codes.h>

const char* MODULE_NAME = "click-test";

int height;

void pointer_enter(module_t* self, int x, int y) {
   for (int i = 0; i < height * self->width; i++) {
      self->buffer[i] = 0xFF0000FF;
   }

   self->mark_dirty(self);

}

void pointer_exit(module_t* self) {
   for (int i = 0; i < height * self->width; i++) {
      self->buffer[i] = 0xFFFF0000;
   }

   self->mark_dirty(self);

}

void pointer_button(module_t* self, uint32_t button, uint32_t state) {
   LOG_WARN("%d=%d, %d\n", button, BTN_LEFT, state);

   if (button == BTN_LEFT) {
      if (state == BUTTON_PRESSED) {   
         for (int i = 0; i < height * self->width; i++) {
            self->buffer[i] = 0xFF00FF00;
         }
      } else {
         for (int i = 0; i < height * self->width; i++) {
            self->buffer[i] = 0xFF0000FF;
         }
      }
   }  

   self->mark_dirty(self);
}

void module_init(module_t* self, struct hashmap_s* global_config, struct hashmap_s *local_config) {

   CONFIG_GET_OR_FAIL(local_config, "width", self->width, MODULE_NAME);
   CONFIG_GET_OR_FAIL(global_config, "height", height, "global");
   CONFIG_GET_OR_FAIL(local_config, "position", self->position, MODULE_NAME);

   LOG_DEBUG("hiii\n");

   self->pointer_enter = pointer_enter;
   self->pointer_leave = pointer_exit;
   self->pointer_button = pointer_button;

   self->buffer = malloc(sizeof(color_t) * height * self->width);

   for (int i = 0; i < height * self->width; i++) {
      self->buffer[i] = 0xFFFF0000;
   }

   self->mark_dirty(self);
}



