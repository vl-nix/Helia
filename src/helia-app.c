/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "helia-app.h"
#include "helia-win.h"

#include <gst/gst.h>

struct _HeliaApp
{
	GtkApplication  parent_instance;
};

G_DEFINE_TYPE ( HeliaApp, helia_app, GTK_TYPE_APPLICATION )

static void helia_app_activate ( GApplication *app )
{
	HeliaWin *win = helia_win_new ( HELIA_APP ( app ) );

	gtk_window_present ( GTK_WINDOW ( win ) );
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

	G_APPLICATION_CLASS (class)->activate = helia_app_activate;

	object_class->finalize = helia_app_finalize;
}

HeliaApp * helia_app_new ( void )
{
	return g_object_new ( HELIA_TYPE_APP, "application-id", "org.gnome.helia", "flags", G_APPLICATION_NON_UNIQUE, NULL );
}
