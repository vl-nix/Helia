#############################################################################
#
# Makefile for Helia
# https://github.com/vl-nix/helia
#
#===========================================================================
#
# Set prefix = PREFIX ( install files in PREFIX )
# Verify variables: make info
#
#===========================================================================
# /usr:       /usr, /usr/bin ...
# prefix    = /usr

# /usr/local: /usr/local, /usr/local/bin ...
# prefix    = /usr/local

# Home:       $(HOME)/.local, $(HOME)/.local/bin ...
prefix      = $(HOME)/.local

program     = helia
version     = 2.5
bindir      = $(prefix)/bin
datadir     = $(prefix)/share
desktopdir  = $(datadir)/applications

cflags_libs = `pkg-config gtk+-3.0 --cflags --libs` `pkg-config gstreamer-video-1.0 --cflags --libs` `pkg-config gstreamer-mpegts-1.0 --libs`

xres  = $(wildcard res/*.xml)
gres := $(xres:.xml=.c)

srcs := $(wildcard src/*.c)
srcs += res/helia.gresource.c
objs  = $(srcs:.c=.o)


all: genres build


compile: $(objs)

build: $(objs)
	@echo '    build: ' $(program)
	@gcc -Wall $^ -o $(program) $(CFLAG) $(cflags_libs)
	@echo

%.o: %.c
	@echo 'compile: ' $@
	@gcc -Wall -Wextra -c $< -o $@ $(CFLAG) $(cflags_libs)

genres: $(gres)

%.c: %.xml
	@echo
	@echo 'gresource: ' $@
	@glib-compile-resources $< --target=$@ --generate-source
	@echo


install:
	mkdir -p $(DESTDIR)$(bindir) $(DESTDIR)$(datadir) $(DESTDIR)$(desktopdir)
	install -Dp -m0755 $(program) $(DESTDIR)$(bindir)/$(program)
	install -Dp -m0644 res/$(program).desktop $(DESTDIR)$(desktopdir)/$(program).desktop
	sed 's|bindir|$(bindir)|g' -i $(DESTDIR)$(desktopdir)/$(program).desktop

uninstall:
	rm -fr $(DESTDIR)$(bindir)/$(program) $(DESTDIR)$(desktopdir)/$(program).desktop

clean:
	rm -f $(program) src/*.o res/*.o res/*.c po/$(program).pot po/*.po~


# Show variables.
info:
	@echo
	@echo 'program      :' $(program)
	@echo 'prefix       :' $(prefix)
	@echo 'bindir       :' $(bindir)
	@echo 'datadir      :' $(datadir)
	@echo 'desktopdir   :' $(desktopdir)
	@echo


# Show help.
help:
	@echo 'Makefile for Helia'
	@echo 'https://github.com/vl-nix/helia'
	@echo
	@echo 'Installation directories:'
	@echo '  Open the Makefile and set the prefix value'
	@echo '    prefix = PREFIX 	install files in PREFIX'
	@echo
	@echo '  Verify variables: make info'
	@echo
	@echo 'Usage: make [TARGET]'
	@echo 'TARGETS:'
	@echo '  all        or make'
	@echo '  help       print this message'
	@echo '  info       show variables'
	@echo '  compile    only compile'
	@echo '  build      build'
	@echo '  install    install'
	@echo '  uninstall  uninstall'
	@echo '  clean      clean all'
	@echo	
	@echo 'Showing debug:'
	@echo '  G_MESSAGES_DEBUG=all ./$(program)'
	@echo

#############################################################################
