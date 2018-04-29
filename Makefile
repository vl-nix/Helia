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
version     = 2.6
bindir      = $(prefix)/bin
datadir     = $(prefix)/share
desktopdir  = $(datadir)/applications
localedir   = $(datadir)/locale

obj_locale  = $(subst :, ,$(LANGUAGE))
cflags_libs = `pkg-config gtk+-3.0 --cflags --libs` `pkg-config gstreamer-video-1.0 --cflags --libs` `pkg-config gstreamer-mpegts-1.0 --libs`

xres  = $(wildcard res/*.xml)
gres := $(xres:.xml=.c)

srcs := $(wildcard src/*.c)
srcs += res/helia.gresource.c
objs  = $(srcs:.c=.o)


all: cofigure genres setlcdir build revlcdir msgfmt

cofigure:
	@sed 's|=/.*/helia|=$(bindir)/helia|g;s|bindir|$(bindir)|g' -i res/$(program).desktop
	@sed 's|gtk_about_dialog_set_version ( dialog, ".*" );|gtk_about_dialog_set_version ( dialog, "$(version)" );|g' -i src/gmp-dvb.c
	@for lang_ver in po/*.po; do \
		sed 's|"Project-Id-Version: helia .*"|"Project-Id-Version: helia $(version)\\n"|g' -i $$lang_ver; \
	done

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


setlcdir:
	@sed 's|/usr/share/locale/|$(localedir)|g' -i src/gmp-dvb.c

revlcdir:
	@sed 's|$(localedir)|/usr/share/locale/|g' -i src/gmp-dvb.c

genpot:
	mkdir -p po
	xgettext src/*.c --language=C --keyword=N_ --keyword=_ --escape --sort-output --from-code=UTF-8 \
	--package-name=$(program) --package-version=$(version) -o po/$(program).pot
	sed 's|PACKAGE VERSION|$(program) $(version)|g;s|charset=CHARSET|charset=UTF-8|g' -i po/$(program).pot

mergeinit: genpot
	for lang in $(obj_locale); do \
		echo $$lang; \
		if [ ! -f po/$$lang.po ]; then msginit -i po/$(program).pot --locale=$$lang -o po/$$lang.po; \
		else msgmerge --update po/$$lang.po po/$(program).pot; fi \
	done

msgfmt:
	@for language in po/*.po; do \
		lang=`basename $$language | cut -f 1 -d '.'`; \
		mkdir -pv locale/$$lang/LC_MESSAGES/; \
		msgfmt -v $$language -o locale/$$lang/LC_MESSAGES/$(program).mo; \
	done


install:
	mkdir -p $(DESTDIR)$(bindir) $(DESTDIR)$(datadir) $(DESTDIR)$(desktopdir)
	install -Dp -m0755 $(program) $(DESTDIR)$(bindir)/$(program)
	install -Dp -m0644 res/$(program).desktop $(DESTDIR)$(desktopdir)/$(program).desktop
	cp -r locale $(DESTDIR)$(datadir)

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(program) $(DESTDIR)$(desktopdir)/$(program).desktop
	rm -fr $(DESTDIR)$(localedir)/*/*/$(program).mo

clean:
	rm -f $(program) src/*.o res/*.o res/*.c po/$(program).pot po/*.po~
	rm -fr locale


# Show variables.
info:
	@echo
	@echo 'program      :' $(program)
	@echo 'prefix       :' $(prefix)
	@echo 'bindir       :' $(bindir)
	@echo 'datadir      :' $(datadir)
	@echo 'desktopdir   :' $(desktopdir)
	@echo 'localedir    :' $(localedir)
	@echo 'obj_locale   :' $(obj_locale)
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
	@echo 'For translators:'
	@echo '  genpot     only xgettext -> pot'
	@echo '  mergeinit  genpot and ( msgmerge or msginit ) pot -> po'
	@echo '  msgfmt     only msgfmt po -> mo'
	@echo
	@echo 'Showing debug:'
	@echo '  G_MESSAGES_DEBUG=all ./$(program)'
	@echo

#############################################################################
