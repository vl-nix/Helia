#############################################################################
#
# Makefile for Helia
# https://gitlab.com/vl-nix/Helia
# git clone git@gitlab.com:vl-nix/Helia.git
#
# https://www.opendesktop.org/
#
#===========================================================================
#
# Set prefix = PREFIX ( install files in PREFIX )
# Verify variables: make info
#
#===========================================================================
# /usr:       /usr, /usr/bin ...
# prefix     = /usr

# /usr/local: /usr/local, /usr/local/bin ...
# prefix     = /usr/local

# Home:       $(HOME)/.local, $(HOME)/.local/bin ...
prefix			= $(HOME)/.local

program			= helia
version			= 4.2

bindir			= $(prefix)/bin
datadir			= $(prefix)/share
desktopdir		= $(datadir)/applications
localedir		= $(datadir)/locale

obj_locale		= $(shell echo $(LANG) | cut -f 1 -d '.')

sourcedir		= src
builddir		= build
buildsrcdir		= $(builddir)/src
builddatadir	= $(builddir)/data

binary			= $(builddir)/$(program)


sources			= $(wildcard $(sourcedir)/*.c)
objects			= $(patsubst $(sourcedir)/%.c,$(buildsrcdir)/%.o,$(sources))

xres			= $(builddatadir)/helia.gresource.xml
gres			= $(patsubst $(builddatadir)/%.xml,$(builddatadir)/%.c,$(xres))
obj_res			= $(patsubst $(builddatadir)/%.c,$(buildsrcdir)/%.o,$(gres))

desktop_in		= data/helia.desktop.in
desktop			= $(patsubst %.desktop.in,$(builddir)/%.desktop,$(desktop_in))


OS				= Linux
CC				= gcc
CFLAGS			= -Wall -Wextra
LDFLAGS			= `pkg-config gtk+-3.0 --cflags --libs` `pkg-config gstreamer-video-1.0 --cflags --libs` `pkg-config gstreamer-mpegts-1.0 --libs`



all: makeinfo configure setlcdir $(binary) revlcdir mergeinit msgfmt

makeinfo:
	@echo
	@echo " " $(program) - $(version)
	@echo
	@echo "  Prefix ........ : " $(prefix)
	@echo

configure: dirs $(xres) $(desktop)

dirs:
	@mkdir -p $(builddir) $(buildsrcdir) $(builddatadir)

$(desktop): $(builddir)/%.desktop : %.desktop.in
	@sed 's|bindir|$(bindir)|g' $< > $@

$(xres):
	@echo "<gresources>\n<gresource prefix=\"/helia\">" > $@
	@for grfile in data/icons/*.png; do \
		echo "\t<file preprocess=\"to-pixdata\">$$grfile</file>" >> $@; \
	done
	@echo "</gresource>\n</gresources>" >> $@


build: dirs $(xres) $(binary)

$(binary): $(obj_res) $(objects)
	@echo '  CCLD	' $@
	@$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo

$(objects): $(buildsrcdir)/%.o : $(sourcedir)/%.c
	@echo '  CC	' $<
	@$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

$(obj_res): $(buildsrcdir)/%.o : $(builddatadir)/%.c
	@echo '  CC	' $<
	@$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

$(gres): $(builddatadir)/%.c : $(builddatadir)/%.xml
	@echo
	@echo '  Gresource	' $@
	@glib-compile-resources $< --target=$@ --generate-source
	@echo


setlcdir:
	@sed 's|/usr/share/locale/|$(localedir)|g' -i src/main.c

revlcdir:
	@sed 's|$(localedir)|/usr/share/locale/|g' -i src/main.c

genpot:
	@mkdir -p po
	@xgettext src/*.c --language=C --keyword=N_ --keyword=_ --escape --sort-output --from-code=UTF-8 \
	--package-name=$(program) --package-version=$(version) -o po/$(program).pot
	@sed 's|charset=CHARSET|charset=UTF-8|g' -i po/$(program).pot

mergeinit: genpot
	@echo "Localization ( msginit or msgmerge ):"
	@for lang in $(obj_locale); do \
		echo "Language	" $$lang; \
		if [ ! -f po/$$lang.po ]; then msginit -i po/$(program).pot --locale=$$lang -o po/$$lang.po --no-translator; \
		else msgmerge --update po/$$lang.po po/$(program).pot; fi \
	done

mergeall: genpot
	@for lang in po/*.po; do \
		echo $$lang; \
		msgmerge --update $$lang po/$(program).pot; \
	done

msgfmt:
	@echo
	@echo "Localization ( msgfmt ):"
	@for language in po/*.po; do \
		lang=`basename $$language | cut -f 1 -d '.'`; \
		mkdir -pv $(builddir)/locale/$$lang/LC_MESSAGES/; \
		msgfmt -v $$language -o $(builddir)/locale/$$lang/LC_MESSAGES/$(program).mo; \
	done


install:
	mkdir -p $(DESTDIR)$(bindir) $(DESTDIR)$(datadir) $(DESTDIR)$(desktopdir)
	install -Dp -m0755 $(builddir)/$(program) $(DESTDIR)$(bindir)/$(program)
	install -Dp -m0644 $(builddatadir)/$(program).desktop $(DESTDIR)$(desktopdir)/$(program).desktop
	cp -r $(builddir)/locale $(DESTDIR)$(datadir)

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(program) $(DESTDIR)$(desktopdir)/$(program).desktop
	rm -fr $(DESTDIR)$(localedir)/*/*/$(program).mo

clean:
	rm -f po/*.pot po/*.po~
	rm -fr $(builddir)



# Show variables.
info:
	@echo
	@echo 'program      :' $(program)
	@echo 'version      :' $(version)
	@echo
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
	@echo 'https://gitlab.com/vl-nix/Helia'
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
	@echo '  build      build'
	@echo '  install    install'
	@echo '  uninstall  uninstall'
	@echo '  clean      clean all'
	@echo
	@echo ' Depends '
	@echo ' 		* gcc '
	@echo ' 		* make '
	@echo ' 		* gettext  ( gettext-tools ) '
	@echo ' 		* libgtk 3 ( & dev ) '
	@echo ' 		* gstreamer 1.0 ( & dev ) '
	@echo ' 		* gst-plugins 1.0 ( & dev ) '
	@echo ' 			* base, good, ugly, bad ( & dev ) '
	@echo ' 		* gst-libav '
	@echo
	@echo 'For translators:'
	@echo '  genpot     only xgettext -> pot'
	@echo '  mergeinit  genpot and ( msgmerge or msginit ) pot -> po'
	@echo '  msgfmt     only msgfmt po -> mo'
	@echo
	@echo 'Showing debug:'
	@echo '  G_MESSAGES_DEBUG=all ./$(binary)'
	@echo

#############################################################################
