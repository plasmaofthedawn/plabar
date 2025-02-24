#include <stdlib.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <limits.h>

#include "hashmap.h"
#include "window.h"
#include "config.h"
#include "module.h"

#include "log.h"

#include <cairo.h>

#include <time.h>

uint32_t *pixels;

// config stuff
unsigned int width = 2256;
unsigned int height = 200;
direction_t anchor_pos;
color_t background_color = 0x00000000;

cairo_t *cr;

void copy_buffer(const color_t* restrict src, const int module_position, const int module_width) {

    LOG_DEBUG("%08x\n", ((color_t*) src)[0]);

    //struct timespec start, stop;
    //clock_gettime(CLOCK_REALTIME, &start);

    // new surf
    cairo_surface_t *surf = cairo_image_surface_create_for_data((unsigned char*) src, CAIRO_FORMAT_ARGB32, module_width, height, module_width * 4);  

    // fill background
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, module_position, 0, module_position + module_width, height);
    cairo_fill(cr);

    // fill buffer
    cairo_set_source_surface(cr, surf, module_position, 0);
    cairo_paint(cr);

    // kill surf
    cairo_surface_destroy(surf);


    /*
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < module_width; x++) {

            color_t color = src[y * module_width + x]; 
            color_t alpha = (color & 0xFF000000) >> 24;
            
            color_t red_mask = 0x00FF0000;
            color_t blue_mask = 0x0000FF00;
            color_t green_mask = 0x000000FF;

                
            pixels[y * width + x + module_position] = 
                ((((background_color & red_mask) * (255 - alpha) + (color & red_mask) * (alpha)) / 255) & red_mask) | 
                ((((background_color & blue_mask) * (255 - alpha) + (color & blue_mask) * (alpha)) / 255) & blue_mask) |
                ((((background_color & green_mask) * (255 - alpha) + (color & green_mask) * (alpha))/ 255) & green_mask);

        }
    }*/

    /*
    clock_gettime(CLOCK_REALTIME, &stop);
    
    double accum = ( stop.tv_sec - start.tv_sec )
             + (double)( stop.tv_nsec - start.tv_nsec )
               / (double) 1000000000L;
    printf( "%lf\n", accum );
    */

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

        //sleep(1);

    }
}

int print_entry(void* const context, struct hashmap_element_s* const e) {
    LOG_DEBUG("  %s (%d): %s\n", (char *) e->key, e->key_len, (char *) e->data);
    return 0;
}


void parse_global_config(struct hashmap_s *global_map) {
    
    char* element;

    CONFIG_GET_OR_FAIL(global_map, "width", element, "global");
    width = atoi(element);

    CONFIG_GET_OR_FAIL(global_map, "height", element, "global");
    height = atoi(element);

    if ((element = hashmap_get(global_map, "background_color", sizeof("background_color") - 1)) != NULL) {
        background_color = get_color_from_value(element, "background_color", "global");
        background_color &= 0x00FFFFFF; // clear alpha value
    }

    CONFIG_GET_OR_FAIL(global_map, "position", element, "global");
    char* pos = get_string_from_value(element, "position", "global");


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
    init_plabar_modules(modules);
    LOG_INFO("found %d modules\n", modules);

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
