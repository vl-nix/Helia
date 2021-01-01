/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "control-tv.h"
#include "default.h"
#include "button.h"

struct _ControlTv
{
	GtkWindow parent_instance;

	GtkVolumeButton *volbutton;

	uint16_t icon_size;
};

G_DEFINE_TYPE ( ControlTv, control_tv, GTK_TYPE_WINDOW )

static uint16_t size_icon = ICON_SIZE;

static void control_tv_signal_handler_num ( GtkButton *button, ControlTv *ctv )
{
	const char *name = gtk_widget_get_name ( GTK_WIDGET ( button ) );

	g_signal_emit_by_name ( ctv, "dvb-click-num", atoi ( name ) );
}

static void control_tv_volume_changed ( G_GNUC_UNUSED GtkScaleButton *button, double value, ControlTv *ctv )
{
	g_signal_emit_by_name ( ctv, "dvb-set-volume", value );
}

static void control_tv_init ( ControlTv *ctv )
{
	ctv->icon_size = size_icon;

	GtkWindow *window = GTK_WINDOW ( ctv );

	gtk_window_set_decorated ( window, FALSE  );
	gtk_window_set_title ( window, "" );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_icon_name ( window, DEF_ICON );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_default_size ( window, 400, -1 );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( m_box, 5 );

	GtkBox *b_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( b_box, 5 );

	GtkBox *v_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( v_box, 5 );

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	uint8_t BN = 5;

	const char *icons_a[] = { "🖵", "", "☰", "☰", "🔇" };
	const char *name_icons_a[] = { "helia-tv", "helia-editor", "helia-eqa", "helia-eqv", "helia-muted" };
	const gboolean swapped_a[] = { TRUE, FALSE, TRUE, TRUE, FALSE };

	uint8_t c = 0; for ( c = 0; c < BN; c++ )
	{
		GtkButton *button = helia_create_button ( h_box, name_icons_a[c], icons_a[c], ctv->icon_size );
		gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );

		char name[20];
		sprintf ( name, "%u", c );
		gtk_widget_set_name ( GTK_WIDGET ( button ), name );

		g_signal_connect ( button, "clicked", G_CALLBACK ( control_tv_signal_handler_num ), ctv );
		if ( swapped_a[c] ) g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );
	}

	gtk_box_pack_start ( v_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	const char *icons_b[] = { "⏹", "⏺", "📡", "⏼", "🞬" };
	const char *name_icons_b[] = { "helia-stop", "helia-record", "helia-display", "helia-info", "helia-exit" };
	const gboolean swapped_b[] = { FALSE, FALSE, TRUE, TRUE, TRUE };

	for ( c = 0; c < BN; c++ )
	{
		GtkButton *button = helia_create_button ( h_box, name_icons_b[c], icons_b[c], ctv->icon_size );
		gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );

		char name[20];
		sprintf ( name, "%u", c + BN );
		gtk_widget_set_name ( GTK_WIDGET ( button ), name );

		g_signal_connect ( button, "clicked", G_CALLBACK ( control_tv_signal_handler_num ), ctv );
		if ( swapped_b[c] ) g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );
	}

	gtk_box_pack_start ( v_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( b_box, GTK_WIDGET ( v_box ), TRUE, TRUE, 0 );

	ctv->volbutton = (GtkVolumeButton *)gtk_volume_button_new ();
	gtk_button_set_relief ( GTK_BUTTON ( ctv->volbutton ), GTK_RELIEF_NORMAL );
	g_signal_connect ( ctv->volbutton, "value-changed", G_CALLBACK ( control_tv_volume_changed ), ctv );

	gtk_box_pack_end ( b_box, GTK_WIDGET ( ctv->volbutton ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( b_box ), FALSE, FALSE, 0 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_widget_set_visible ( GTK_WIDGET ( m_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( b_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( v_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( ctv->volbutton ), TRUE );
}

static void control_tv_finalize ( GObject *object )
{
	G_OBJECT_CLASS (control_tv_parent_class)->finalize (object);
}

static void control_tv_class_init ( ControlTvClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = control_tv_finalize;

	g_signal_new ( "dvb-click-num", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT );

	g_signal_new ( "dvb-set-volume", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_DOUBLE );
}

ControlTv * control_tv_new ( uint16_t opacity, uint16_t icon_size, double volume, GtkWindow *win_base )
{
	size_icon = icon_size;

	ControlTv *ctv = g_object_new ( CONTROL_TYPE_TV, NULL );

	GtkWindow *window = GTK_WINDOW ( ctv );

	gtk_window_set_transient_for ( window, win_base );
	gtk_window_present ( window );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), ( (float)opacity / 100 ) );

	gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( ctv->volbutton ), volume );

	return ctv;
}
