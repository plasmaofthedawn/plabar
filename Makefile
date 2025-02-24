CC=gcc
CFLAGS=-Wall -g -D DEBUG_DEBUG 
LIBS=`pkg-config --cflags --libs pangocairo wayland-client`

PROGRAM=plabar

OBJECTS=main.o window.o config.o module.o
WAYLAND_OBJECTS=xdg-shell-protocol.o wlr-layer-shell.o 
MODULES=block.o

BUILDDIR=build
SOURCEDIR=src
WAYLANDDIR=wayland
MODULEDIR=modules

#BUILDLIST = $(addprefix $(PROTOCOLDIR)/, $(PROTOCOLS))
BUILDLIST = $(addprefix $(BUILDDIR)/, $(OBJECTS))
BUILDLIST += $(addprefix $(BUILDDIR)/, $(addprefix $(WAYLANDDIR)/, $(WAYLAND_OBJECTS)))
#BUILDLIST += $(addprefix $(BUILDDIR)/, $(addprefix $(MODULEDIR)/, $(MODULES)))

WAYLAND_STUFF=src/wayland/wlr-layer-shell.h src/wayland/wlr-layer-shell.c src/wayland/xdg-shell-client-protocol.h src/wayland/xdg-shell-protocol.c 

all: compile run

$(BUILD_DIR)/.:
	mkdir -p $@

$(BUILD_DIR)%/.:
	mkdir -p $@

protocol/.:
	mkdir -p $@

src/wayland/.:
	mkdir -p $@

.SECONDEXPANSION:

protocol/wlr-layer-shell-unstable-v1.xml: protocol/.
	curl -j -o $@ 'https://raw.githubusercontent.com/swaywm/wlr-protocols/refs/heads/master/unstable/wlr-layer-shell-unstable-v1.xml'

protocol/xdg-shell.xml: protocol/.
	cp /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml protocol/xdg-shell.xml

src/wayland/wlr-layer-shell.h: protocol/wlr-layer-shell-unstable-v1.xml | $$(@D)/.
	wayland-scanner client-header protocol/wlr-layer-shell-unstable-v1.xml src/wayland/wlr-layer-shell.h

src/wayland/wlr-layer-shell.c: protocol/wlr-layer-shell-unstable-v1.xml | $$(@D)/.
	wayland-scanner private-code protocol/wlr-layer-shell-unstable-v1.xml src/wayland/wlr-layer-shell.c

src/wayland/xdg-shell-client-protocol.h: protocol/xdg-shell.xml | $$(@D)/.
	wayland-scanner client-header protocol/xdg-shell.xml src/wayland/xdg-shell-client-protocol.h

src/wayland/xdg-shell-protocol.c: protocol/xdg-shell.xml | $$(@D)/.
	wayland-scanner private-code protocol/xdg-shell.xml src/wayland/xdg-shell-protocol.c

$(BUILDDIR)/module.o: $(SOURCEDIR)/module.c $(SOURCEDIR)/modules/* | $$(@D)/.
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $(SOURCEDIR)/module.c

$(BUILDDIR)/%.o: $(SOURCEDIR)/%.c | $$(@D)/.
	$(CC) $(CFLAGS) $(LIBS) -c -o $@ $^

$(PROGRAM): $(BUILDLIST) $(WAYLANDBUILDLIST) | $$(@D)/.
	$(CC) $(CFLAGS) $(BUILDLIST) $(LIBS) -o $(PROGRAM)

compile: $(WAYLAND_STUFF) $(PROGRAM) 

run: $(PROGRAM)
	./plabar

clean:
	rm -r $(PROGRAM) $(BUILDDIR) protocol src/wayland
