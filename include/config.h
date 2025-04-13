#ifndef INCLUDE_PLABAR_CONFIGH
#define INCLUDE_PLABAR_CONFIGH


#include <errno.h>
#include <stdint.h>
#include <ctype.h>

#define DIRECTION_TOP 0
#define DIRECTION_RIGHT 1
#define DIRECTION_LEFT 2
#define DIRECTiON_BOTTOM 3

#include "hashmap.h"
#include "log.h"

typedef int direction_t;
typedef uint32_t color_t;

void load_config_file(char* filename);
void close_config_file();

int parse_module_config(char** module_name, struct hashmap_s* map);

int count_modules();

#define PARSE_FAIL(FORMAT, ...)\
  LOG_ERROR(FORMAT __VA_OPT__(,) __VA_ARGS__); \
  exit(1) \



#define CONFIG_GET_OR_FAIL(map, key, elem, module_name) \
  if ((elem = hashmap_get(map, key, sizeof(key) - 1)) == NULL) { PARSE_FAIL("module %s requires key %s which is missing\n", module_name, key); }

#define CONFIG_GET_FALLBACK_OR_FAIL(map, fallback_map, key, elem, module_name) \
  if ((elem = hashmap_get(map, key, sizeof(key) - 1)) == NULL) { \
    if ((elem = hashmap_get(fallback_map, key, sizeof(key) - 1)) == NULL) { \
      PARSE_FAIL("module %s requires key %s in local or global config which is missing", module_name, key); \
    } \
  }


__attribute__((weak)) color_t get_color_from_value(const char* value, const char* key, const char* module) {

   if (value[0] != '#') {
      PARSE_FAIL("in module %s, key %s = %s is not a valid color\n", module, key, value);
   }  


   errno = 0;
   color_t ret = strtol(&value[1], NULL, 16);

   if (errno) {
      PARSE_FAIL("in module %s, key %s = %s is not a valid color\n", module, key, value);
   }

   // make fully opaque
   if (strlen(value) <= 7) {
      ret |= 0xFF000000;
   }

   return ret;


}


__attribute__((weak)) char* get_string_from_value(char* value, const char* key, const char* module) {
   int i, j;


   for (i = 0; value[i] != '"'; i++) {
      if (!isspace(value[i])) {
         PARSE_FAIL("in module %s, key %s = %s is not a valid string \n", module, key, value);
      }
   }

   for (j = i + 1; value[j] != '"'; j++) {
      if (value[j] == 0) {
         PARSE_FAIL("in module %s, key %s = %s does not have a closing \"\n", module, key, value);
      }
   }

   value[j] = 0;
   return &value[i + 1];

}

#endif  
