#include "module.h"
#include "hashmap.h"

#include <pthread.h>
#include <stdio.h>

// there has to be a better way for this
#include "modules/block.c"
#include "modules/text.c"

#include "log.h"

module_type_t module_types[2];

// all the modules in this bar -- allocated in init
module_t* plabar_modules;
int num_modules = 0;

int loaded_modules = 0;

// yay hashmaps
struct hashmap_s name_to_module;

// global pthread conditions
// i think module_dirty is needed though i'm not too sure
// bah race condition
pthread_cond_t module_dirty_condition;
pthread_mutex_t module_dirty_cond_mutex;

// protected by the module_dirty_cond_mutex above
// (circular) array of modules that need to be updated
module_t** dirty_modules;
int dirty_modules_start = 0;
int dirty_modules_end = 0;


void init_plabar_modules(int n_modules) {

   num_modules = n_modules;
   
   plabar_modules = malloc(sizeof(module_t) * n_modules);
   // +1 to allow for emptiness checking even when there's only one module
   dirty_modules = malloc(sizeof(module_t*) * n_modules + 1);

   pthread_mutex_init(&module_dirty_cond_mutex, NULL);
   pthread_cond_init(&module_dirty_condition, NULL);

   hashmap_create(2, &name_to_module);


   // module shit
   module_types[0].create_module = module_block_init;
   hashmap_put(&name_to_module, "block", strlen("block"), &module_types[0]);

   module_types[1].create_module = module_text_init;
   hashmap_put(&name_to_module, "text", strlen("text"), &module_types[1]);
   


}

// called by a module to mark itself as dirty
// should be called while protected under the module's mutex
void mark_dirty(module_t* self) {
   
   pthread_mutex_lock(&module_dirty_cond_mutex);

   // add this module to the end of the list
   dirty_modules[dirty_modules_end] = self;

   dirty_modules_end += 1;
   dirty_modules_end %= num_modules + 1;

   pthread_cond_signal(&module_dirty_condition);
   pthread_mutex_unlock(&module_dirty_cond_mutex);

}

// called by the graphics thread to wait for the dirty module, returns the index of the dirty module
// blocks until there's a dirty module if there isn't one
module_t* get_next_dirty_module() {

   pthread_mutex_lock(&module_dirty_cond_mutex);

   LOG_DEBUG("%d, %d\n", dirty_modules_start, dirty_modules_end);

   while (dirty_modules_end == dirty_modules_start) {
      pthread_cond_wait(&module_dirty_condition, &module_dirty_cond_mutex);
   }

   module_t* ret = dirty_modules[dirty_modules_start];

   dirty_modules_start += 1;
   dirty_modules_start %= num_modules + 1;

   pthread_mutex_unlock(&module_dirty_cond_mutex);
   
   // ret is seperated out due to mutex bullshit :D
   return ret;
}



void create_module_from_name(char* name, struct hashmap_s* global_config, struct hashmap_s* local_config) { 

   // 
   module_type_t* m_type;
   if ((m_type = hashmap_get(&name_to_module, name, strlen(name)))) {

      module_t* new_module = &plabar_modules[loaded_modules];
      loaded_modules++;

      LOG_DEBUG("module %d\n", loaded_modules);

      // basic setup
      pthread_mutex_init(&new_module->buffer_mutex, NULL); 

      m_type->create_module(new_module, global_config, local_config);

   } else {
      LOG_ERROR("unknown module %s\n", name);
      exit(1);
   }
}


