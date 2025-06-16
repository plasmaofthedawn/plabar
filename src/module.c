#include "module.h"
#include "hashmap.h"

#include <pthread.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <dlfcn.h>

#include "log.h"


#define PLABAR_MODULE_EXTENSION ".pbm"

module_type_t *module_types;

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
// stack of modules that need to be updated
module_t** dirty_modules;
int dirty_modules_count = 0;


#define GET_DLSYM_OR_ERROR(storage, filename, handle, location) \
   storage = dlsym(handle, location);\
   if (storage == NULL) { \
      LOG_ERROR("could not load required information from module located in %s.\n", filename); \
      LOG_ERROR("dl error: %s\n", dlerror()); \
      return -2; \
   }


static inline int is_valid_module_name(struct dirent* dir_entry) {
   return (strcmp(
            dir_entry->d_name + strlen(dir_entry->d_name) - sizeof(PLABAR_MODULE_EXTENSION) + 1,
            PLABAR_MODULE_EXTENSION) == 0
          ) && (dir_entry->d_type == DT_REG || dir_entry->d_type == DT_LNK || dir_entry->d_type == DT_UNKNOWN);
}


// returns nonzero on error 
// module_dir is location of modules
// module_n is number of modules
int init_plabar_modules(const char* module_dir, int n_modules) {

   LOG_DEBUG("starting module type loading\n");
   // open module dir
   DIR* module_directory = opendir(module_dir);

   if (!module_directory) {
      LOG_ERROR("could not find module directory %s\n", module_dir);
      return -1;
   }
   LOG_INFO("seaching directory %s for modules...\n", module_dir);

   int n_module_types = 0;

   // first loop to count
   struct dirent *dir_entry;
   while ((dir_entry = readdir(module_directory))) {
      if (is_valid_module_name(dir_entry)) {
         n_module_types++;
      }
   }

   LOG_DEBUG("found %d modules types\n", n_module_types);

   
   // rewind to actually load them
   rewinddir(module_directory);
   module_types = malloc(n_module_types * sizeof(module_type_t));

   // second loop to load
   int i = 0;

   // filename shenanigans
   char filename[512];
   strcpy(filename, module_dir);
   char* copy_loc = filename + strlen(module_dir) + 1;
   copy_loc[-1] = '/';
   
   // map of hash
   hashmap_create(n_module_types, &name_to_module);
   

   const char** name;

   while ((dir_entry = readdir(module_directory))) {
      if (is_valid_module_name(dir_entry)){

         strcpy(copy_loc, dir_entry->d_name); 
         // open this object
         void* handle = dlopen(filename, RTLD_NOW);

         if (handle == NULL) {
            LOG_ERROR("could not load module in %s\n", filename);
            LOG_ERROR("dl error: %s\n", dlerror());
            return -2;
         }
         // get name
         GET_DLSYM_OR_ERROR(name, filename, handle, "MODULE_NAME");

         // load functions within
         GET_DLSYM_OR_ERROR(module_types[i].create_module, filename, handle, "module_init"); 

         LOG_INFO("found module %s in %s\n", *name, filename);
         // put it into the hashmap
         hashmap_put(&name_to_module, *name, strlen(*name), &module_types[i]);
         i++;
      }
   }

   closedir(module_directory);

   // calloc to zero
   plabar_modules = calloc(sizeof(module_t) * n_modules, 1);
   dirty_modules = malloc(sizeof(module_t*) * n_modules);
   num_modules = n_modules;

   pthread_mutex_init(&module_dirty_cond_mutex, NULL);
   pthread_cond_init(&module_dirty_condition, NULL);


   return 0;
}

// called by a module to mark itself as dirty
// should be called while protected under the module's mutex
void mark_dirty(module_t* self) {
   
   pthread_mutex_lock(&module_dirty_cond_mutex);

   // add this module to the end of the stack 
   
   dirty_modules[dirty_modules_count] = self;
   dirty_modules_count += 1;
   
   pthread_cond_signal(&module_dirty_condition);
   pthread_mutex_unlock(&module_dirty_cond_mutex);

}

// called by the graphics thread to wait for the dirty module, returns the index of the dirty module
// blocks until there's a dirty module if there isn't one
module_t* get_next_dirty_module() {

   pthread_mutex_lock(&module_dirty_cond_mutex);

   LOG_DEBUG("currently %d dirty modules waiting\n", dirty_modules_count);

   while (dirty_modules_count == 0) {
      pthread_cond_wait(&module_dirty_condition, &module_dirty_cond_mutex);
   }

   dirty_modules_count--;
   module_t* ret = dirty_modules[dirty_modules_count];
   
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
      new_module->mark_dirty = mark_dirty;

      m_type->create_module(new_module, global_config, local_config);

   } else {
      LOG_ERROR("unknown module %s\n", name);
      exit(1);
   }
}

/// INPUT STUFF

int current_cursor_module = -1;

static inline int in_module(int x, module_t* module) {
   return module->position <= x && x <= module->position + module->width;
}

#define run_if_not_null(x, ...) \
   if (x) { x(__VA_ARGS__); }

void handle_pointer_enter(int x, int y) {
   
   for (int i = 0; i < num_modules; i++) {
      if (in_module(x, &plabar_modules[i])) {
         current_cursor_module = i;
         LOG_DEBUG("cursor entered module %d\n", current_cursor_module);
         run_if_not_null(plabar_modules[i].pointer_enter, &plabar_modules[i], x - plabar_modules[i].width, y);
         return;
      }
   }
}

void handle_pointer_leave() {
   if (current_cursor_module == -1) { return; }
   run_if_not_null(plabar_modules[current_cursor_module].pointer_leave, &plabar_modules[current_cursor_module]);
}

void handle_pointer_motion(int x, int y) {
   //LOG_DEBUG("%d, %d\n", x, y); 
   //LOG_DEBUG("current module %d\n", current_cursor_module);

    for (int i = 0; i < num_modules; i++) {
      if (in_module(x, &plabar_modules[i])) {
         if (i == current_cursor_module) {
            //LOG_DEBUG("same module\n");
            run_if_not_null(plabar_modules[i].pointer_motion, &plabar_modules[i], x - plabar_modules[i].width, y);
            return;
         } else {
            // if we moved to a different module send the leave event to the old module and enter event to the new module
            if (current_cursor_module >= 0) {
               run_if_not_null(plabar_modules[current_cursor_module].pointer_leave, &plabar_modules[current_cursor_module])
            }

            current_cursor_module = i;
            run_if_not_null(plabar_modules[i].pointer_enter, &plabar_modules[i], x - plabar_modules[i].width, y);
            return;
         }
      }
   }

   if (current_cursor_module == -1) { return; }
   // else moved to no module -- run leave
   run_if_not_null(plabar_modules[current_cursor_module].pointer_leave, &plabar_modules[current_cursor_module]);
   current_cursor_module = -1;
}

void handle_pointer_button(uint32_t button, uint32_t state) {
   if (current_cursor_module == -1) { return; }
   run_if_not_null(plabar_modules[current_cursor_module].pointer_button, &plabar_modules[current_cursor_module], button, state);
}

void handle_pointer_axis(uint32_t axis, uint32_t value) {
   LOG_DEBUG("%d %d\n", axis, value);
   if (current_cursor_module == -1) { return; }
   run_if_not_null(plabar_modules[current_cursor_module].pointer_axis, &plabar_modules[current_cursor_module], axis, value);
}


