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
#include "wlr-layer-shell-unstable-v1.h"
#include "xdg-shell.h"

#include "config.h"

// base display
struct wl_display *display;

struct wl_shm *shm;
struct wl_compositor *compositor;
struct wl_shm_pool *pool;

//struct xdg_wm_base *xdg_wm_base = NULL;
struct zwlr_layer_shell_v1 *zwlr_layer_shell;

struct wl_surface *surface;
struct wl_buffer *buffer;

int configured;

uint32_t* pixel_buffer;

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

// registry creates interfaces
void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char* interface, uint32_t version) {

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        shm = wl_registry_bind(registry, name, &wl_shm_interface, version);
    } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
	zwlr_layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, version);
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
