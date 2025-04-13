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
// (circular) array of modules that need to be updated
// TODO: make this a stack or smth
module_t** dirty_modules;
int dirty_modules_start = 0;
int dirty_modules_end = 0;


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

   LOG_DEBUG("found %d modules\n", n_module_types);

   
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
   void (**md)(module_t*);

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

         // export functions
         GET_DLSYM_OR_ERROR(md, filename, handle, "mark_dirty"); 
         *md = &mark_dirty;
         
         LOG_INFO("found module %s in %s\n", *name, filename);
         // put it into the hashmap
         hashmap_put(&name_to_module, *name, strlen(*name), &module_types[i]);
         i++;
      }
   }

   closedir(module_directory);

   plabar_modules = malloc(sizeof(module_t) * n_modules);
   // +1 to allow for emptiness checking even when there's only one module
   dirty_modules = malloc(sizeof(module_t*) * n_modules + 1);
   num_modules = n_modules;

   pthread_mutex_init(&module_dirty_cond_mutex, NULL);
   pthread_cond_init(&module_dirty_condition, NULL);


   return 0;
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


