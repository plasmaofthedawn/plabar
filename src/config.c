#include "config.h"
#include "hashmap.h"

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define CONFIG_LINE_EMPTY 0
#define CONFIG_LINE_HEADER 1
#define CONFIG_LINE_PAIR 2
#define CONFIG_LINE_INVALID -1

#define COMMENT_CHARACTER '#'

#include "log.h"

/*
could this be better? probably
but i wrote all of this on my phone
half on a plane
and strip_whitespace on a shitty boat breathing in exhaust fumes
its good enough
*/


typedef struct string_t {
   
   char* pointer;
   unsigned int length;

} string_t;

char* file_buffer;
size_t file_pointer;
size_t file_size;

int line_num;

void read_fail(char* message) {
   LOG_ERROR("%s on line %d\n", message, line_num);
   exit(1);
}


void strip_whitespace(char* str, int n, string_t* out) {
   int start, end;
   for (start = 0; start < n && isspace(str[start]); start++);
   if (start == n) {
      read_fail("empty key/value/thing");
   }
   
   for(end = n; end > 0 && isspace(str[end - 1]); end--);

   out->pointer = &str[start];
   out->length = end - start;
}

int recognize_line(char* str){
   
   //printf("%c\n", str[0]);
   
   int type = CONFIG_LINE_EMPTY;
   int i=0;
   for(; str[i] != '\n' && str[i] != COMMENT_CHARACTER; i++){
      if(!isspace(str[i])){
         type = str[i] == '[' ? CONFIG_LINE_HEADER : CONFIG_LINE_PAIR;
         break;
      }
   }
   
   if(type == CONFIG_LINE_EMPTY) { return type;}
   else if(type == CONFIG_LINE_HEADER) {
      for(; str[i] != '\n' && str[i] != COMMENT_CHARACTER; i++) {
         //printf("%c", str[i]);
         if(str[i] == ']') {
            return CONFIG_LINE_HEADER;
         }
      }
      return CONFIG_LINE_INVALID;
   } else if(type == CONFIG_LINE_PAIR) {
      // empty key
      if (str[i] == '=') {
         return CONFIG_LINE_INVALID;
      }
      for (; str[i] != '\n' && str[i] != COMMENT_CHARACTER; i++) {
         if (str[i] == '=') {
            return type;
         }
      }
      return CONFIG_LINE_INVALID;
   }

    return CONFIG_LINE_INVALID;
}

void parse_header(char *str, string_t *header_name) {
   int start, end;
   for(start = 0; str[start] != '['; start++);
   for(end = start + 1; str[end] != ']'; end++);

   strip_whitespace(&str[start + 1], end - start - 1, header_name);

   header_name->pointer = &str[start + 1];
   header_name->length = end - start - 1;
}

void parse_pair(char *str, string_t *key, string_t *value) {
   
   int start, end;
   
   for(start = 0; str[start] != '='; start++);
   strip_whitespace(str, start, key);
   
   // head past the =
   start++;
   
   for (end = start; str[end] != '\n'; end++);
   strip_whitespace(&str[start], end - start, value);
   
   //printf("%s", *value);
}

void next_line() {
   // go to the next line
   for (; file_buffer[file_pointer] != '\n'; file_pointer++);
   file_pointer++;
   line_num++;
}

void load_config_file(char* filename) {
   FILE *f = fopen(filename, "rb");
   
   fseek(f, 0, SEEK_END);
   file_size = ftell(f);
   rewind(f);
   
   file_buffer = malloc(sizeof(char) * (file_size+ 1));

   size_t amount_read = 0;
   
   while (amount_read < file_size) {
      amount_read += fread(&file_buffer[amount_read], 1, file_size - amount_read, f);
   }
   
   file_buffer[file_size] = '\n';

   file_pointer = 0;
   line_num = 1;

   // seek to first header line
   int line;
   while ((line = recognize_line(&file_buffer[file_pointer])) == CONFIG_LINE_EMPTY) {
      next_line();
   }
   if (line != CONFIG_LINE_HEADER) {
      read_fail("first line is not header. go fuck urself.");
   }

   fclose(f);
}


// returns the number of modules excluding the global module
int count_modules() {

   // global module doesn't count
   int modules = 0;
   int i = 0;
   while (i < file_size) {
      if (recognize_line(&file_buffer[i]) == CONFIG_LINE_HEADER) {
         modules++;
      }
      for (; file_buffer[i] != '\n'; i++);
      i++;
   }

   return modules;

}

void close_config_file() {

   free(file_buffer);

}

// returns non zero if there's more to the file
int parse_config(struct hashmap_s* map) {

   hashmap_create(2, map);

   string_t k, v;

   while (file_pointer < file_size) {

      int line = recognize_line(&file_buffer[file_pointer]);

      //printf("%d\n", line);
      
      if (line == CONFIG_LINE_HEADER) {
         return 1;
      } else if (line == CONFIG_LINE_PAIR) {

         parse_pair(&file_buffer[file_pointer], &k, &v);

         hashmap_put(map, k.pointer, k.length, v.pointer);

         next_line();
         
         // null terminate the strings;
         k.pointer[k.length] = '\0';
         v.pointer[v.length] = '\0';


      } else if (line == CONFIG_LINE_INVALID) {

         read_fail("yo ur config is fucked up");

      } else if (line == CONFIG_LINE_EMPTY) {

         next_line();

      }
   }

   return 0;
}


// returns non zero if there's more to the file
int parse_module_config(char** module_name, struct hashmap_s* map) {

   string_t m_name;
   parse_header(&file_buffer[file_pointer], &m_name);
   next_line();
   
   m_name.pointer[m_name.length] = '\0';
   *module_name = m_name.pointer;

   return parse_config(map);

}


// ################### number/string parsing helpers


color_t get_color_from_value(char* value, char* key, char* module) {

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


char* get_string_from_value(char* value, char* key, char* module) {
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
