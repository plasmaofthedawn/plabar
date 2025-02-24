#ifndef INCLUDE_PLABAR_CONFIGH
#define INCLUDE_PLABAR_CONFIGH


#include <stdint.h>
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

color_t get_color_from_value(char* value, char* key, char* module);
char* get_string_from_value(char* value, char* key, char* module);

#endif  
