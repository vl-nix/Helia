/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "pref.h"
#include "helia-win.h"
#include "helia-dvb.h"

#include <stdlib.h>
#include <glib/gstdio.h>

struct _HeliaWin
{
	GtkWindow  parent_instance;

	GtkBox *mn_vbox;

	HeliaDvb *dvb;

	Pref *pref;
};

G_DEFINE_TYPE ( HeliaWin, helia_win, GTK_TYPE_WINDOW )

static void helia_win_set_win_tv ( HeliaWin *win )
{
	win->dvb = helia_dvb_new ( win->pref, GTK_WINDOW ( win ) );

	gtk_box_pack_start ( win->mn_vbox, GTK_WIDGET ( win->dvb ), TRUE, TRUE, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( win->dvb ), TRUE );

	gtk_window_set_title ( GTK_WINDOW ( win ), "Helia - Digital TV" );
}

static void helia_win_create ( HeliaWin *win )
{
	gtk_icon_theme_add_resource_path ( gtk_icon_theme_get_default (), "/helia" );

	GtkWindow *window = GTK_WINDOW ( win );

	gtk_window_set_title ( window, "Helia");
	gtk_window_set_default_size ( window, 900, 400 );
	gtk_window_set_icon_name ( window, "helia" );

	win->mn_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( win->mn_vbox ), TRUE );

	helia_win_set_win_tv ( win );

	gtk_container_add  ( GTK_CONTAINER ( window ), GTK_WIDGET ( win->mn_vbox ) );
}

static void helia_win_init ( HeliaWin *win )
{
	win->dvb = NULL;

	char path_dir[PATH_MAX];
	sprintf ( path_dir, "%s/helia", g_get_user_config_dir () );

	if ( !g_file_test ( path_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR ) )
	{
		int ret = g_mkdir ( path_dir, 0775 );

		if ( ret == -1 ) g_print ( "Error: Not creating %s directory. \n", path_dir ); else g_print ( "Creating %s directory. \n", path_dir );
	}

	win->pref = pref_new ();

	helia_win_create ( win );

	g_signal_emit_by_name ( win->pref, "pref-defl", GTK_WINDOW ( win ) );
}

static void helia_win_dispose ( GObject *object )
{
	HeliaWin *win = HELIA_WIN ( object );

	GdkWindow *rwin = gtk_widget_get_window ( GTK_WIDGET ( win ) );

	if ( GDK_IS_WINDOW ( rwin ) )
	{
  		int width  = gdk_window_get_width  ( rwin );
  		int height = gdk_window_get_height ( rwin );

		g_signal_emit_by_name ( win->pref, "pref-save", ( width > 0 ) ? width : 900, ( height > 0 ) ? height : 400 );
	}

	G_OBJECT_CLASS ( helia_win_parent_class )->dispose ( object );
}

static void helia_win_finalize ( GObject *object )
{
	G_OBJECT_CLASS ( helia_win_parent_class )->finalize ( object );
}

static void helia_win_class_init ( HeliaWinClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = helia_win_finalize;

	oclass->dispose = helia_win_dispose;
}

HeliaWin * helia_win_new ( HeliaApp *app )
{
	HeliaWin *win = g_object_new ( HELIA_TYPE_WIN, "application", app, NULL );

	return win;
}
