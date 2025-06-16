#include <stdlib.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <cairo.h>
#include <time.h>

#include "hashmap.h"
#include "window.h"
#include "config.h"
#include "module.h"
#include "log.h"

uint32_t *pixels;

// config stuff
int width = 2256;
int height = 200;
direction_t anchor_pos;
color_t background_color = 0xFF000000;

cairo_t *cr;

// graphics #################

void copy_buffer(const color_t* restrict src, const int module_position, const int module_width) {

    LOG_DEBUG("drawing module width %d position %d\n", module_width, module_position);
    // new surf
    cairo_surface_t *surf = cairo_image_surface_create_for_data((unsigned char*) src, CAIRO_FORMAT_ARGB32, module_width, height, module_width * 4);  

    // fill background
    cairo_set_source_rgba(cr, ((background_color & 0x00FF0000) >> 16) / 255.0, ((background_color & 0x000000FF00) >> 8) / 255.0, ((background_color & 0x000000FF)) / 255.0, ((background_color & 0xFF000000) >> 24) / 255.0);
    cairo_rectangle(cr, module_position, 0, module_width, height);
    cairo_fill(cr);

    // fill buffer
    cairo_set_source_surface(cr, surf, module_position, 0);
    cairo_paint(cr);

    // kill surf
    cairo_surface_destroy(surf);
}

void* thread_function(void* unused) {
    for (;;) {
        
        module_t* module = get_next_dirty_module();


        pthread_mutex_lock(&module->buffer_mutex);
        copy_buffer(module->buffer, module->position, module->width);
        pthread_mutex_unlock(&module->buffer_mutex);

        // this can be better
        // idk yet
        update_window(0, 0, INT_MAX, INT_MAX);

    }
}

//////// stuff

int print_entry(void* const context, struct hashmap_element_s* const e) {
    LOG_DEBUG("  %s (%d): %s\n", (char *) e->key, e->key_len, (char *) e->data);
    return 0;
}

void parse_global_config(struct hashmap_s *global_map) {
    
    char* pos;

    CONFIG_GET_OR_FAIL(global_map, "width", width, "global");
    CONFIG_GET_OR_FAIL(global_map, "height", height, "global");
    CONFIG_GET_OR_FAIL(global_map, "position", pos, "global");

    CONFIG_GET(global_map, "background_color", background_color, "global");
    background_color &= 0xFFFFFFFF; // clear alpha value
    
    if (strcmp(pos, "top") == 0) {
        anchor_pos = DIRECTION_TOP;
    } else if (strcmp(pos, "bottom") == 0) {
        anchor_pos = DIRECTiON_BOTTOM;
    } else {
        PARSE_FAIL("unsupported position %s in module %s, only directions top and bottom are supported)", pos, "global"); 
    }
}

int main() {

    LOG_INFO("starting....\n");
    load_config_file("config.ini");
    LOG_INFO("config file loaded\n");

    char* module_name;
    struct hashmap_s global_map;
    struct hashmap_s local_map;

    // load the global config
    int more = parse_module_config(&module_name, &global_map);
    
    // setup modules
    int modules = count_modules();
    LOG_INFO("found %d modules\n", modules);

    if (init_plabar_modules("build", modules)) {
        LOG_ERROR("error initializing modules, exiting\n");
        return -1;
    }
    
    parse_global_config(&global_map);

    if (more) { 
        do {

            more = parse_module_config(&module_name, &local_map);

            LOG_INFO("module %s\n", module_name);

            create_module_from_name(module_name, &global_map, &local_map);

            hashmap_destroy(&local_map);

        } while (more);
    }

    close_config_file();


    LOG_INFO("starting,,,\n");

    create_window(width, height, anchor_pos);
    pixels = get_pixel_buffer();

    cairo_surface_t *surf = cairo_image_surface_create_for_data((unsigned char*) pixels, CAIRO_FORMAT_RGB24, width, height, width * 4);
    cr = cairo_create(surf);

    // fill the background color with the thing
    for (int i = 0; i < width * height; i++) {
        pixels[i] = background_color;
    }
    update_window(0, 0, INT_MAX, INT_MAX);

    pthread_t thread;
    pthread_create(&thread, NULL, thread_function, NULL);
    
    window_loop();
}
