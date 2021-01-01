/*
* Copyright 2020 Stepan Perun
* This program is free software.
* 
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "helia-eqv.h"
#include "default.h"
#include "button.h"

enum EQV_N
{
	NUM_B,
	NUM_C,
	NUM_S,
	NUM_H,
	NUMS
};

typedef struct _EQVideoFunc EQVideoFunc;

struct _EQVideoFunc
{
	const char *name;
	const char *desc;
	void ( *f )();
};

static void helia_eqv_changed_label_cbsh ( GtkRange *range, GtkLabel *label )
{
	double val = gtk_range_get_value (range);

	char text[20];
	sprintf ( text, "%d", (int)val );

	gtk_label_set_text ( label, text );
}

static void helia_eqv_changed_b ( GtkRange *range, GstElement *element )
{
	double val = gtk_range_get_value (range);
	val = val / 1000;
	g_object_set ( element, "brightness", val, NULL );
}
static void helia_eqv_changed_c ( GtkRange *range, GstElement *element )
{
	double val = gtk_range_get_value (range);
	val = (val + 1000) / 1000;
	g_object_set ( element, "contrast", val, NULL );
}
static void helia_eqv_changed_s ( GtkRange *range, GstElement *element )
{
	double val = gtk_range_get_value (range);
	val = (val + 1000) / 1000;
	g_object_set ( element, "saturation", val, NULL );
}
static void helia_eqv_changed_h ( GtkRange *range, GstElement *element )
{
	double val = gtk_range_get_value (range);
	val = val / 1000;
	g_object_set ( element, "hue", val, NULL );
}

static void helia_eqv_clear ( G_GNUC_UNUSED GtkButton *button, GtkScale *scale )
{
	gtk_range_set_value ( GTK_RANGE ( scale ), 0.0 );
}

void helia_eqv_win ( double opacity, GtkWindow *win_base, GstElement *element )
{
	GtkWindow *window = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title ( window, "Video EQ" );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_transient_for ( window, win_base );
	gtk_window_set_icon_name ( window, DEF_ICON );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_top    ( GTK_WIDGET ( m_box ), 10 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( m_box ), 10 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( m_box ), 10 );

	EQVideoFunc eqv_n[] =
	{
		{ "Ⓑ", "brightness", 	helia_eqv_changed_b },
		{ "Ⓒ", "contrast", 	helia_eqv_changed_c },
		{ "Ⓢ", "saturation", 	helia_eqv_changed_s },
		{ "Ⓗ", "hue", 		helia_eqv_changed_h }
	};

	GtkButton *button_all_clear = helia_create_button ( NULL, "helia-clear", "🗑", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( button_all_clear ), TRUE );

	uint8_t c = 0; for ( c = 0; c < NUMS; c++ )
	{
		GtkLabel *name_label = (GtkLabel *)gtk_label_new ( eqv_n[c].name );
		gtk_widget_set_halign ( GTK_WIDGET ( name_label ), GTK_ALIGN_START );
		gtk_widget_set_visible ( GTK_WIDGET ( name_label ), TRUE );

		double val = 0;
		g_object_get ( element, eqv_n[c].desc, &val, NULL );

		GtkBox *scales_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
		gtk_widget_set_visible ( GTK_WIDGET ( scales_hbox ), TRUE );

		if ( c == 0 || c == 3 )
			val = val * 1000;
		else
			val = (val * 1000) - 1000;

		char text[20];
		sprintf ( text, "%d", (int)val );

		GtkLabel *label = (GtkLabel *)gtk_label_new ( text );
		gtk_widget_set_size_request ( GTK_WIDGET (label ), 50, -1 );
		gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );

		GtkScale *scale = (GtkScale *)gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, -1000, 1000, 10 );
		gtk_scale_set_draw_value ( GTK_SCALE (scale), 0 );
		gtk_range_set_value ( GTK_RANGE (scale), val );
		gtk_widget_set_visible ( GTK_WIDGET ( scale ), TRUE );

		g_signal_connect ( scale, "value-changed", G_CALLBACK ( eqv_n[c].f ), element );
		g_signal_connect ( scale, "value-changed", G_CALLBACK ( helia_eqv_changed_label_cbsh ), label );

		gtk_widget_set_size_request ( GTK_WIDGET (scale), 400, -1 );
		gtk_box_pack_start ( scales_hbox, GTK_WIDGET (scale), TRUE, TRUE, 0 );

		gtk_box_pack_start ( m_box, GTK_WIDGET (name_label), FALSE, FALSE,  0 );

		GtkBox *hm_box = (GtkBox *)gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0 );
		gtk_widget_set_visible ( GTK_WIDGET ( hm_box ), TRUE );

		gtk_box_pack_start ( hm_box, GTK_WIDGET (label), FALSE, FALSE, 0 );
		gtk_box_pack_start ( hm_box, GTK_WIDGET (scales_hbox),  TRUE, TRUE, 0 );

		GtkButton *button = helia_create_button ( hm_box, "helia-clear", "🗑", 16 );
		gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
		g_signal_connect ( button, "clicked", G_CALLBACK ( helia_eqv_clear ), GTK_WIDGET ( scale ) );

		gtk_box_pack_start ( m_box, GTK_WIDGET (hm_box), TRUE, TRUE, 0 );

		g_signal_connect ( button_all_clear, "clicked", G_CALLBACK ( helia_eqv_clear ), scale );
	}

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 10 );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( button_all_clear ), TRUE, TRUE, 0 );

	GtkButton *button = helia_create_button ( h_box, "helia-exit", "🞬", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 10 );

	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( m_box ), TRUE );

	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_window_present ( window );
	gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity );
}
