#ifndef INCLUDE_PLABAR_MODULEH
#define INCLUDE_PLABAR_MODULEH

#include "module-types.h"

int init_plabar_modules(const char* module_dir, int n_modules);
void create_module_from_name(char* name, struct hashmap_s* global_config, struct hashmap_s* local_config);

void mark_dirty(module_t* self);
module_t* get_next_dirty_module();

void handle_pointer_enter(int x, int y);
void handle_pointer_leave();
void handle_pointer_motion(int x, int y);
void handle_pointer_button(uint32_t button, uint32_t state);
void handle_pointer_axis(uint32_t axis, uint32_t value);


#endif  // INCLUDE_PLABAR_MODULEH
