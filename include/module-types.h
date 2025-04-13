#include "hashmap.h"
#include "config.h"

#include <pthread.h>
#include <stdint.h>

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
