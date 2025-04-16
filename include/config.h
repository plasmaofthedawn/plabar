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

static inline color_t config_parse_color(const char* value, const char* key, const char* module) {

   if (value[0] != '#') {
      PARSE_FAIL("in module %s, key %s = %s is not a valid color\n", module, key, value);
   }  

   errno = 0;
   color_t ret = strtol(&value[1], NULL, 16);

   if (errno) {
      PARSE_FAIL("in module %s, key %s = %s is not a valid color\n", module, key, value);
   }

   // make fully opaque if short
   if (strlen(value) <= 7) {
      ret |= 0xFF000000;
   }

   LOG_DEBUG("parsed color #%x\n", ret);

   return ret;
}

static inline char* config_parse_string(char* value, const char* key, const char* module) {

   // this is horrible and i hate it but it works
   int i, j;


   for (i = 0; value[i] != '"' && value[i] != 0x7F; i++) {
      if (!isspace(value[i])) {
         PARSE_FAIL("in module %s, key %s = %s is not a valid string \n", module, key, value);
      }
   }

   char dest_char = value[i] == 0x7f ? 0 : '"';

   for (j = i + 1; value[j] != dest_char; j++) {
      if (value[j] == 0 && dest_char != 0) {
         PARSE_FAIL("in module %s, key %s = %s does not have a closing \"\n", module, key, value);
      }
   }

   value[i] = 0x7F; // marking to mark this as read, and so it should search til newline
   value[j] = 0;
   return &value[i + 1];
}

static inline int config_parse_int(char* value, const char* key, const char* module) {

   errno = 0;
   int ret = strtol(value, NULL, 10);

   if (errno) {
      PARSE_FAIL("in module %s, key %s = %s is not a valid number\n", module, key, value);
   }

   return ret;
}


#define CONFIG_PARSE(destination, value, key, module) \
   destination = _Generic((destination), \
                          color_t: config_parse_color, \
                          int: config_parse_int, \
                          char*: config_parse_string \
                 )(value, key, module);

#define CONFIG_GET(map, key, destination, module) \
   do { \
      char *_temp; \
      _temp = hashmap_get(map, key, sizeof(key) - 1); \
      if (_temp) { CONFIG_PARSE(destination, _temp, key, module); } \
   } while (0)

#define CONFIG_GET_OR_FAIL(map, key, destination, module) \
   do { \
      char *_temp; \
      _temp = hashmap_get(map, key, sizeof(key) - 1); \
      if (_temp) { CONFIG_PARSE(destination, _temp, key, module); } \
      else { PARSE_FAIL("module %s requires key %s which is missing\n", module, key); } \
   } while (0)

#define CONFIG_GET_FALLBACK_OR_FAIL(map, fallback_map, key, destination, module) \
   do { \
      char *_temp; \
      _temp = hashmap_get(map, key, sizeof(key) - 1); \
      if (_temp) { CONFIG_PARSE(destination, _temp, key, module); } \
      else { \
         _temp = hashmap_get(fallback_map, key, sizeof(key)-1); \
         if (_temp) { CONFIG_PARSE(destination, _temp, key, module); }\
         else { PARSE_FAIL("module %s requires key %s in local or global config which is missing", module, key); } \
      } \
   } while (0)

#endif  
