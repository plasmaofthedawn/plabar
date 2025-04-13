#ifndef INCLUDE_PLABAR_MODULEH
#define INCLUDE_PLABAR_MODULEH

#include "module-types.h"

int init_plabar_modules(const char* module_dir, int n_modules);
void create_module_from_name(char* name, struct hashmap_s* global_config, struct hashmap_s* local_config);

void mark_dirty(module_t* self);
module_t* get_next_dirty_module();

#endif  // INCLUDE_PLABAR_MODULEH
