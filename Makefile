dir_guard=@mkdir -p $(@D)

# directories
BUILD_DIR=build
SOURCE_DIR=src
MODULE_DIR=modules
INCLUDE_DIR=include
PROTOCOL_DIR=protocol

# C
CC=gcc
CFLAGS=-Wall -g -D DEBUG_DEBUG -I$(INCLUDE_DIR)
MODULEFLAGS=-fPIC
LIBS=-ldl `pkg-config --cflags --libs pangocairo wayland-client`

# program name
PROGRAM=plabar

# compilables
OBJECTS=main.o window.o config.o module.o xdg-shell.o wlr-layer-shell-unstable-v1.o
MODULES=clock.pbm block.pbm text.pbm

# wayland
PROTOCOLS=wlr-layer-shell-unstable-v1.xml xdg-shell.xml
WAYLAND_PROTOCOLS=$(addprefix $(PROTOCOL_DIR)/, $(PROTOCOLS))
WAYLAND_HEADERS=$(addprefix $(INCLUDE_DIR)/, $(PROTOCOLS:.xml=.h))
WAYLAND_SOURCE=$(addprefix $(BUILD_DIR)/, $(PROTOCOLS:.xml=.c))

all: modules program

$(PROTOCOL_DIR)/wlr-layer-shell-unstable-v1.xml:
	$(dir_guard)
	curl -j -o $@ 'https://gitlab.freedesktop.org/wlroots/wlr-protocols/-/raw/master/unstable/wlr-layer-shell-unstable-v1.xml?inline=false'

$(PROTOCOL_DIR)/xdg-shell.xml:
	$(dir_guard)
	cp /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml $@

$(WAYLAND_HEADERS): $(WAYLAND_PROTOCOLS)
	$(dir_guard)
	wayland-scanner client-header $(PROTOCOL_DIR)/$(basename $(notdir $@)).xml $@

$(WAYLAND_SOURCE): $(WAYLAND_PROTOCOLS) $(WAYLAND_HEADERS)
	$(dir_guard)
	wayland-scanner private-code $(PROTOCOL_DIR)/$(basename $(notdir $@)).xml $@

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c
	$(dir_guard)
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $^

$(BUILD_DIR)/%.pbm: $(MODULE_DIR)/%.c 
	$(dir_guard)
	$(CC) $(CFLAGS) $(MODULEFLAGS) $(LIBS) $^ -c -o $(basename $@).o
	$(CC) -shared -o $@ $(basename $@).o

$(PROGRAM): $(addprefix $(BUILD_DIR)/, $(OBJECTS))
	$(dir_guard)
	$(CC) $(CFLAGS) $(addprefix $(BUILD_DIR)/, $(OBJECTS)) $(LIBS) -o $(PROGRAM)

modules: $(addprefix $(BUILD_DIR)/, $(MODULES))

program: $(WAYLAND_SOURCE) $(PROGRAM)

clean:
	rm -rf $(WAYLAND_HEADERS) $(WAYLAND_SOURCE) $(PROTOCOL_DIR) $(BUILD_DIR)
