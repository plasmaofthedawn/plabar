#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>
#include "wlr-layer-shell-unstable-v1.h"
#include "xdg-shell.h"

#include "config.h"
#include "module.h"

// wayland stuff
struct wl_display *display;
struct wl_shm *shm;
struct wl_compositor *compositor;
struct wl_shm_pool *pool;

// for input
struct wl_seat *seat;
struct wl_pointer *pointer;

// for drawing
struct zwlr_layer_shell_v1 *zwlr_layer_shell;
struct wl_surface *surface;
struct wl_buffer *buffer;

int configured;

uint32_t* pixel_buffer;

static void wl_callback_unused() {
    // do nothing
}

// ############## Input Junk

static void wl_pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) {
    handle_pointer_enter(wl_fixed_to_int(surface_x), wl_fixed_to_int(surface_y));
}

static void wl_pointer_leave(void* data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface) {
    handle_pointer_leave();
}

static void wl_pointer_motion(void* data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y){ 
    handle_pointer_motion(wl_fixed_to_int(surface_x), wl_fixed_to_int(surface_y));
   }

static void wl_pointer_button(void* data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) {
    handle_pointer_button(button, state);
}

static void wl_pointer_axis(void* data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {
    handle_pointer_axis(axis, wl_fixed_to_int(value));
}

static struct wl_pointer_listener pointer_listener = {
    .enter = wl_pointer_enter,
    .leave = wl_pointer_leave,
    .motion = wl_pointer_motion,
    .button = wl_pointer_button,
    .axis = wl_pointer_axis,
    .frame = wl_callback_unused,
    .axis_source = wl_callback_unused,
    .axis_stop = wl_callback_unused,
    .axis_discrete = wl_callback_unused,
    .axis_value120 = wl_callback_unused,
    .axis_relative_direction = wl_callback_unused,
};

/// ############# Wayland junk

static void handle_configure(void *data, struct zwlr_layer_surface_v1 *zwlr_layer_surface, uint32_t serial, uint32_t width, uint32_t height) {
	// The compositor configures our surface, acknowledge the configure event
	zwlr_layer_surface_v1_ack_configure(zwlr_layer_surface, serial);

	LOG_DEBUG("zwlr layer shell configured\n");

	if (configured) {
		// If this isn't the first configure event we've received, we already
		// have a buffer attached, so no need to do anything. Commit the
		// surface to apply the configure acknowledgement.
		wl_surface_commit(surface);
	}

	configured = 1;
}


void handle_close(void *data, struct zwlr_layer_surface_v1 *zwlr_layer_surface) {
	// Stop running if the user requests to close the toplevel
}

struct zwlr_layer_surface_v1_listener zwlr_listener = {
    .configure = handle_configure,
    .closed = handle_close,
};

static void wl_seat_capabilities(void *data, struct wl_seat *wl_seat, uint32_t capabilities) {

    int have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;

    LOG_DEBUG("have_pointer %d, pointer %p", have_pointer, pointer);

    if (have_pointer && pointer == NULL) {
	// set pointer if we don't have one already
	pointer = wl_seat_get_pointer(seat);
	wl_pointer_add_listener(pointer, &pointer_listener, NULL);
    } else if (!have_pointer && pointer) {
	// remove pointer if one doesn't exist
	wl_pointer_release(pointer);
	pointer = NULL;
    }
}

static void wl_seat_name(void *data, struct wl_seat *wl_seat, const char *name) {
       LOG_DEBUG("wayland seat name: %s\n", name);
}

static const struct wl_seat_listener wl_seat_listener = {
       .capabilities = wl_seat_capabilities,
       .name = wl_seat_name,
};
// registry creates interfaces
void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char* interface, uint32_t version) {

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        shm = wl_registry_bind(registry, name, &wl_shm_interface, version);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
	zwlr_layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, version);
    } else if (strcmp(interface, wl_seat_interface.name) == 0){
	seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
	wl_seat_add_listener(seat, &wl_seat_listener, NULL);
    }

    //printf("interface %s: version %d name %d\n", interface, version, name);
}

void registry_handle_global_remove(void* data, struct wl_registry *registry, uint32_t name) {
    // fuck you.
}

const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = registry_handle_global_remove,
};


int setup_wayland() {

    // create display
    if((display = wl_display_connect(NULL)) == NULL) {
        perror("Error opening display");
        return -1;
    }

    // setup registry so i can find the interface i need
    struct wl_registry* registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);
    wl_registry_destroy(registry);
    return 0;

}

void cleanup_wayland() {
    wl_display_disconnect(display);
}


////// ############# BUFFER JUNK

int allocate_shm_file(size_t size) {
    
    int fd = syscall(SYS_memfd_create, "plabar", 0);
    ftruncate(fd, size);
    return fd;

}

static void handle_buffer_release(void* data, struct wl_buffer *wl_buffer) {

}

struct wl_buffer_listener buffer_listener = {
   .release = handle_buffer_release,
};

struct wl_buffer* create_buffer(int width, int height) {
    const int shm_pool_size = width * height * 4;

    int fd = allocate_shm_file(shm_pool_size);
    uint8_t *pool_data = mmap(NULL, shm_pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   
    pool = wl_shm_create_pool(shm, fd, shm_pool_size);
    buffer = wl_shm_pool_create_buffer(pool, 0, width, height, width * 4, WL_SHM_FORMAT_XRGB8888);
    wl_buffer_add_listener(buffer, &buffer_listener, NULL);

    close(fd);
    
    pixel_buffer = (uint32_t*) pool_data;

    return buffer;

}


// #########       MY JUNK

void create_window(unsigned int width, unsigned int height, direction_t anchor_pos) {
    // setup wayland
    setup_wayland();

    surface = wl_compositor_create_surface(compositor);

    struct zwlr_layer_surface_v1 *zwlr_surface = zwlr_layer_shell_v1_get_layer_surface(zwlr_layer_shell, surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_TOP, "plabar");

    zwlr_layer_surface_v1_add_listener(zwlr_surface, &zwlr_listener, NULL);
    zwlr_layer_surface_v1_set_size(zwlr_surface, width, height);

    // anchor the thing to the ceiling or whatever
    uint32_t anchor;
    switch (anchor_pos) {
	case DIRECTION_TOP:    anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;    break;
	case DIRECTION_LEFT:   anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;   break;
	case DIRECTION_RIGHT:  anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;  break;
	case DIRECTiON_BOTTOM: anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM; break;
    }

    zwlr_layer_surface_v1_set_anchor(zwlr_surface, anchor);
    zwlr_layer_surface_v1_set_exclusive_zone(zwlr_surface, height);
    
    wl_surface_commit(surface);
    while (wl_display_dispatch(display) != -1 && !configured) {
    
    }

    // create the buffer for this window
    create_buffer(width, height);
}

uint32_t* get_pixel_buffer() {
    return pixel_buffer;
}

void update_window(int x, int y, int w, int h) {
    // tell wayland to update the buffer
    wl_surface_attach(surface, buffer, 0, 0);    
    wl_surface_damage_buffer(surface, x, y, w, h);
    wl_surface_commit(surface);
    wl_display_flush(display);
}

void window_loop() {
    LOG_DEBUG("looping\n");
    while (wl_display_dispatch(display) != -1) {
	//printf("p%d\n", amount);
    }
    LOG_DEBUG("Exiting\n");
    cleanup_wayland();
}
