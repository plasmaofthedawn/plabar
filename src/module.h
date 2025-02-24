#ifndef INCLUDE_PLABAR_MODULEH
#define INCLUDE_PLABAR_MODULEH


#include "config.h"
#include "hashmap.h"

#include <pthread.h>

typedef struct module_t {

	// the buffer this module writes to 
	color_t* buffer;

	// mutex for the buffer
	pthread_mutex_t buffer_mutex;

	// position and width of this module -- probably shouldn't be edited manually
	int32_t position;
	uint32_t width;

	// any extra data 
	void* data;

	//TODO: bullshit about callbacks and cursor input

} module_t;

typedef struct module_type_t {

	void (*create_module)(module_t* out, struct hashmap_s *global_config, struct hashmap_s *local_config);

} module_type_t;


void init_plabar_modules(int n_modules);
void create_module_from_name(char* name, struct hashmap_s* global_config, struct hashmap_s* local_config);

void mark_dirty(module_t* self);
module_t* get_next_dirty_module();

#endif  // INCLUDE_PLABAR_MODULEH
