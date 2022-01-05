/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "helia-app.h"
#include "helia-win.h"

struct _HeliaApp
{
	GtkApplication  parent_instance;
};

G_DEFINE_TYPE ( HeliaApp, helia_app, GTK_TYPE_APPLICATION )

static void helia_new_win ( GApplication *app, GFile **files, int n_files )
{
	helia_win_new ( files, n_files, HELIA_APP ( app ) );
}

static void helia_app_activate ( GApplication *app )
{
	helia_new_win ( app, NULL, 0 );
}

static void helia_app_open ( GApplication *app, GFile **files, int n_files, G_GNUC_UNUSED const char *hint )
{
	helia_new_win ( app, files, n_files );
}

static void helia_app_init ( G_GNUC_UNUSED HeliaApp *helia_app )
{
	gst_init ( NULL, NULL );
}

static void helia_app_finalize ( GObject *object )
{
	G_OBJECT_CLASS (helia_app_parent_class)->finalize (object);
}

static void helia_app_class_init ( HeliaAppClass *class )
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	G_APPLICATION_CLASS (class)->open     = helia_app_open;
	G_APPLICATION_CLASS (class)->activate = helia_app_activate;

	object_class->finalize = helia_app_finalize;
}

HeliaApp * helia_app_new ( void )
{
	return g_object_new ( HELIA_TYPE_APP, /*"application-id", "org.gnome.helia",*/ "flags", G_APPLICATION_HANDLES_OPEN | G_APPLICATION_NON_UNIQUE, NULL );
}
