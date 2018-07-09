/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia-eqv.h"
#include "helia-service.h"

#include <glib/gi18n.h>


static struct GstEQVideoData { const gchar *name; const gchar *desc; } eqv_n[] =
{
	{ N_("Brightness"), "brightness" },
	{ N_("Contrast"  ),	"contrast"   },
	{ N_("Saturation"),	"saturation" },
	{ N_("Hue"       ),	"hue"        }
};

static GtkScale *all_scale[ G_N_ELEMENTS (eqv_n) ];
static GtkLabel *all_label[ G_N_ELEMENTS (eqv_n) ];


static void helia_eqv_changed_cbsh ( GtkRange *range, GstElement *element )
{
	guint z = 0, i = 0;

	for ( z = 0; z < G_N_ELEMENTS (eqv_n); z++ )
		if ( range == GTK_RANGE ( all_scale[z] ) )
			{ i = z; break; }

	gdouble val = gtk_range_get_value (range);

	gchar *text = g_strdup_printf ( "%d", (gint)val );
	gtk_label_set_text ( all_label[i], text );
	g_free ( text );

	if ( i == 0 || i == 3 )
		val = val / 1000;
	else
		val = (val + 1000) / 1000;

	g_object_set ( element, eqv_n[i].desc, val, NULL );

	g_debug ( "helia_eqv_changed_cbsh" );
}

static void helia_eqv_clear ()
{
	guint i = 0;
	for ( i = 0; i < G_N_ELEMENTS ( eqv_n ); i++ )
		gtk_range_set_value ( GTK_RANGE ( all_scale[i] ), 0.0 );

	g_debug ( "helia_eqv_clear" );
}

static void helia_eqv_clear_one ( GtkToolItem *item, GtkScale *scale )
{
	gtk_range_set_value ( GTK_RANGE ( scale ), 0.0 );

	g_debug ( "helia_eqv_clear_one:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( item ) ) );
}

static void helia_eqv_win_quit ( GtkWindow *window )
{
	gtk_widget_destroy ( GTK_WIDGET ( window ) );

	g_debug ( "helia_eqv_win_quit" );
}

static void helia_eqv_win_close ( GtkButton *button, GtkWindow *window )
{
	helia_eqv_win_quit ( window );

	g_debug ( "helia_eqv_win_close:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

void helia_eqv_win ( GstElement *element, GtkWindow *parent, gdouble opacity )
{
	GtkBox *m_box, *h_box;
	GtkLabel *name_label;

	GtkWindow *window_eq_video = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window_eq_video, parent );
	gtk_window_set_title    ( window_eq_video, _("Video equalizer") );
	gtk_window_set_modal    ( window_eq_video, TRUE );
	gtk_window_set_position ( window_eq_video, GTK_WIN_POS_CENTER_ON_PARENT );
	// g_signal_connect ( window_eq_video, "destroy", G_CALLBACK ( helia_eqv_win_quit ), NULL );

	m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_top    ( GTK_WIDGET ( m_box ), 10 );
	//gtk_widget_set_margin_bottom ( GTK_WIDGET ( m_box ), 10 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( m_box ), 10 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( m_box ), 10 );

	guint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( eqv_n ); c++ )
	{
		name_label = (GtkLabel *)gtk_label_new ( _(eqv_n[c].name) );
		gtk_widget_set_halign ( GTK_WIDGET ( name_label ), GTK_ALIGN_START );

		gdouble val = 0;

		g_object_get ( element, eqv_n[c].desc, &val, NULL );

		GtkBox *scales_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

		all_scale[c] = (GtkScale *)gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, -1000, 1000, 10 );

		if ( c == 0 || c == 3 )
			val = val * 1000;
		else
			val = (val * 1000) - 1000;

		gchar *text = g_strdup_printf ( "%d", (gint)val );
		all_label[c] = (GtkLabel *)gtk_label_new ( text );
		gtk_widget_set_size_request ( GTK_WIDGET ( all_label[c] ), 50, -1 );
		g_free ( text );

		gtk_scale_set_draw_value ( GTK_SCALE (all_scale[c]), 0 );
		gtk_range_set_value ( GTK_RANGE (all_scale[c]), val );
		g_signal_connect ( all_scale[c], "value-changed", G_CALLBACK ( helia_eqv_changed_cbsh ), element );

		gtk_widget_set_size_request ( GTK_WIDGET (all_scale[c]), 400, -1 );
		gtk_box_pack_start ( scales_hbox, GTK_WIDGET (all_scale[c]), TRUE, TRUE, 0 );

		gtk_box_pack_start ( m_box, GTK_WIDGET (name_label), FALSE, FALSE,  0 );

		GtkBox *hm_box = (GtkBox *)gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0 );

		gtk_box_pack_start ( hm_box, GTK_WIDGET (all_label[c]), FALSE, FALSE, 0 );
		gtk_box_pack_start ( hm_box, GTK_WIDGET (scales_hbox),  TRUE, TRUE, 0 );

		GtkButton *button = helia_service_ret_image_button ( "helia-clear", 16 );
		g_signal_connect ( button, "clicked", G_CALLBACK ( helia_eqv_clear_one ), GTK_WIDGET ( all_scale[c] ) );
		gtk_box_pack_start ( hm_box,  GTK_WIDGET ( button ), FALSE,  FALSE, 0 );

		gtk_box_pack_start ( m_box, GTK_WIDGET (hm_box), TRUE, TRUE, 0 );
	}

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 10 );

	helia_service_create_image_button ( h_box, "helia-clear", 16, helia_eqv_clear,     NULL );
	helia_service_create_image_button ( h_box, "helia-exit",  16, helia_eqv_win_close, GTK_WIDGET ( window_eq_video ) );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 10 );

	gtk_container_add ( GTK_CONTAINER ( window_eq_video ), GTK_WIDGET ( m_box ) );
	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );

	gtk_widget_show_all ( GTK_WIDGET ( window_eq_video ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window_eq_video ), opacity );

	g_debug ( "helia_eqv_win" );
}
