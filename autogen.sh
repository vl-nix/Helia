#!/bin/sh


if [ -f Makefile ]; then
	make clean
	rm Makefile
fi


if test -n "$1"; then
if [ $1 = "clean" ]; then

	for file in `ls`
	do
		if [ $file = "data" -o $file = "src" -o $file = "po" -o $file = "autogen.sh" -o $file = "LICENSE" -o $file = "README.md" ]; 
		then echo "No remove: " $file; else rm -rf $file; fi
	done

	rm -rf ./*/Makefile*

	rm -rf src/.deps/ data/.deps/ data/.dirstamp
	rm -f data/helia.desktop data/helia.gresource.c data/helia.gresource.xml

	cd po; mkdir po-tmp; cp *.po po-tmp/; rm -f *; mv po-tmp/*.po .; rm -rf po-tmp; cd ../

	exit 0

fi
fi

#######################################################################################################################

gen_conf_ac ()
{

echo "AC_PREREQ([2.69])
AC_INIT([helia],[4.2])
AC_CONFIG_MACRO_DIR([m4])
AC_SUBST([ACLOCAL_AMFLAGS], \"-I m4\")

AM_SILENT_RULES([yes])
AM_INIT_AUTOMAKE([1.11 foreign subdir-objects tar-ustar no-dist-gzip dist-xz -Wno-portability])

GETTEXT_PACKAGE=AC_PACKAGE_TARNAME
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], [\"\$GETTEXT_PACKAGE\"], [GETTEXT package name])
AM_GNU_GETTEXT_VERSION([0.19.6])
AM_GNU_GETTEXT([external])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_SED
AC_PATH_PROG([GLIB_GENMARSHAL],[glib-genmarshal])
AC_PATH_PROG([GLIB_MKENUMS],[glib-mkenums])
AC_PATH_PROG([GLIB_COMPILE_RESOURCES],[glib-compile-resources])
PKG_PROG_PKG_CONFIG([0.22])

PKG_CHECK_MODULES(HELIA, [glib-2.0 gdk-pixbuf-2.0 gtk+-3.0 gstreamer-1.0 gstreamer-plugins-base-1.0 gstreamer-plugins-good-1.0 gstreamer-plugins-bad-1.0 gstreamer-video-1.0 gstreamer-mpegts-1.0])

LT_INIT

AC_CONFIG_FILES([
	Makefile

	src/Makefile

	data/Makefile

	po/Makefile.in
])

AC_OUTPUT
echo \"\"
echo \" \${PACKAGE} - \${VERSION}\"
echo \"\"
echo \" Options\"
echo \"\"
echo \"  Prefix ............................... : \${prefix}\"
echo \"  Libdir ............................... : \${libdir}\"
echo \"\"" > configure.ac

}


gen_make_am ()
{
	echo "SUBDIRS = data src po" > Makefile.am
}


gen_make_am_src ()
{
	cd  $1
	echo "bin_PROGRAMS = helia" > Makefile.am

	for file in "" "helia_CFLAGS = \$(HELIA_CFLAGS)" "helia_LDADD  = \$(HELIA_LIBS)" "" "helia_SOURCES  = \\"
	do	
		echo "$file" >> Makefile.am
	done

	
	for file in `ls *.c`
	do	
		echo "\t$file \\" >> Makefile.am
	done

	echo "\t../data/helia.gresource.c" >> Makefile.am
	echo "" >> Makefile.am

	cd ../
}


gen_make_am_data ()
{

echo "desktopdir   = \$(datadir)/applications
desktop_DATA = helia.desktop

helia.desktop: helia.desktop.in
	@echo \"Configure desktop	\"  \$@
	sed 's|bindir|\$(bindir)|g' $< > \$@
	sed 's|^.*bindtextdomain ( \"helia\", \".*\" );|    bindtextdomain ( \"helia\", \"\$(localedir)\" );|g' -i ../src/main.c

helia.gresource.c: helia.gresource.xml
	@echo 'Gresource	' \$@
	glib-compile-resources $< --target=\$@ --generate-source

configure:
	@echo \"Configure XML\"
	@echo \"<gresources>\" > helia.gresource.xml
	@echo \"<gresource prefix=\\\"/helia\\\">\" >> helia.gresource.xml
	@for grfile in icons/*.png; do \\
		echo \" <file preprocess=\\\"to-pixdata\\\">\$\$grfile</file>\" >> helia.gresource.xml; \\
	done
	@echo \"</gresource>\" >> helia.gresource.xml
	@echo \"</gresources>\" >> helia.gresource.xml
	

BUILT_SOURCES = \\
	configure \\
	helia.desktop \\
	helia.gresource.c

CLEANFILES = \\
	helia.gresource.c \\
	helia.desktop " > data/Makefile.am

}


gen_files_po ()
{
	ls $1/*.po | sed 's|\.po$||;s|^po/||' > $1/LINGUAS

	ls src/*.c > $1/POTFILES.in


	echo "DOMAIN = \$(PACKAGE)
 
subdir = po
top_builddir = ..

XGETTEXT_OPTIONS = --keyword=_ --keyword=N_ \\
        --keyword=C_:1c,2 --keyword=NC_:1c,2 \\
        --keyword=g_dngettext:2,3 \\
        --flag=g_dngettext:2:pass-c-format \\
        --flag=g_strdup_printf:1:c-format \\
        --flag=g_string_printf:2:c-format \\
        --flag=g_string_append_printf:2:c-format \\
        --flag=g_error_new:3:c-format \\
        --flag=g_set_error:4:c-format \\
        --flag=g_markup_printf_escaped:1:c-format \\
        --flag=g_log:3:c-format \\
        --flag=g_print:1:c-format \\
        --flag=g_printerr:1:c-format \\
        --flag=g_printf:1:c-format \\
        --flag=g_fprintf:2:c-format \\
        --flag=g_sprintf:2:c-format \\
        --flag=g_snprintf:3:c-format

PO_DEPENDS_ON_POT = no

DIST_DEPENDS_ON_UPDATE_PO = no" > $1/Makevars
}

#######################################################################################################################


gen_conf_ac
gen_make_am
gen_make_am_src src
gen_make_am_data data
gen_files_po po


# Run this to generate all the initial makefiles, etc.
test -n "$srcdir" || srcdir=$(dirname "$0")
test -n "$srcdir" || srcdir=.


NOCONFIGURE=true

olddir=$(pwd)

cd $srcdir

(test -f configure.ac) || {
	echo "*** ERROR: Directory '$srcdir' does not look like the top-level project directory ***"
	exit 1
}

# shellcheck disable=SC2016
PKG_NAME=$(autoconf --trace 'AC_INIT:$1' configure.ac)

if [ "$#" = 0 -a "x$NOCONFIGURE" = "x" ]; then
	echo "*** WARNING: I am going to run 'configure' with no arguments." >&2
	echo "*** If you wish to pass any to it, please specify them on the" >&2
	echo "*** '$0' command line." >&2
	echo "" >&2
fi

mkdir -p m4

autoreconf --verbose --force --install || exit 1

cd "$olddir"

echo "Now configure process."
