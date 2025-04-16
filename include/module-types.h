#include "hashmap.h"
#include "config.h"

#include <pthread.h>
#include <stdint.h>

typedef struct module_t {

	// the buffer this module writes to -- needs to be created
	color_t* buffer;

	// mutex for the buffer -- will be filled automatically
	pthread_mutex_t buffer_mutex;

	// position and width of this module -- probably shouldn't be edited manually 
	int32_t position;
	int32_t width;

	// any extra data -- will not be touched by plabar
	void* data;

	// callbacks -- will be filled automatically

	void (*mark_dirty)(struct module_t* module);

	// cursor input -- module defined -- skipped if null
	
	void (*pointer_enter)(struct module_t* self, int x, int y);
	void (*pointer_leave)(struct module_t* self);
	void (*pointer_motion)(struct module_t *self, int x, int y);
	void (*pointer_button)(struct module_t *self, uint32_t button, uint32_t state);
	void (*pointer_axis)(struct module_t *self, uint32_t axis, uint32_t value);

} module_t;

typedef struct module_type_t {

	void (*create_module)(module_t* out, struct hashmap_s *global_config, struct hashmap_s *local_config);

} module_type_t;

#define BUTTON_RELEASED 0
#define BUTTON_PRESSED 1

#define AXIS_VERTICAL 0
#define AXIS_HORIZONTAL 1
