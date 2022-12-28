/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "descr.h"
#include "info-dvb-win.h"

#include <gst/gst.h>

struct _InfoDvbWin
{
	GtkWindow parent_instance;

	GtkBox *v_box;
};

G_DEFINE_TYPE ( InfoDvbWin, info_dvb_win, GTK_TYPE_WINDOW )

static void info_dvb_win_create ( InfoDvbWin *win )
{
	GtkWindow *window = GTK_WINDOW ( win );
	gtk_window_set_title ( window,  " " );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_position ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_default_size ( window, 400, 400 );
	gtk_window_set_icon_name ( window, "helia" );

	win->v_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( win->v_box, 10 );

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_widget_set_visible ( GTK_WIDGET ( win->v_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	GtkButton *button = (GtkButton *)gtk_button_new_from_icon_name ( "helia-close", GTK_ICON_SIZE_MENU );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	gtk_box_pack_end   ( win->v_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	gtk_container_set_border_width ( GTK_CONTAINER ( win->v_box ), 10 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( win->v_box ) );
}

static void info_dvb_win_init ( InfoDvbWin *win )
{
	info_dvb_win_create ( win );
}

static void info_dvb_win_finalize ( GObject *object )
{
	G_OBJECT_CLASS ( info_dvb_win_parent_class )->finalize ( object );
}

static void info_dvb_win_class_init ( InfoDvbWinClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = info_dvb_win_finalize;
}

InfoDvbWin * info_dvb_win_new ( uint sid, const char *data, GObject *combo_lang, GtkWindow *base_win )
{
	InfoDvbWin *win = g_object_new ( INFODVB_TYPE_WIN, "transient-for", base_win, NULL );

	Descr *descr = descr_new ();

	g_signal_emit_by_name ( descr, "descr-info", sid, data, G_OBJECT ( win->v_box ), G_OBJECT ( combo_lang ) );

	g_object_unref ( descr );

	gtk_window_present ( GTK_WINDOW ( win ) );
	gtk_widget_set_opacity ( GTK_WIDGET ( win ), gtk_widget_get_opacity ( GTK_WIDGET ( base_win ) ) );

	return win;
}
