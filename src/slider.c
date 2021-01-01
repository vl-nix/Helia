/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "slider.h"

struct _Slider
{
	GtkBox parent_instance;

	GtkScale *slider;
	GtkLabel *lab_pos;
	GtkLabel *lab_dur;

	ulong slider_signal_id;
};

G_DEFINE_TYPE ( Slider, slider, GTK_TYPE_BOX )

static void slider_set_text ( GtkLabel *label, gint64 pos_dur, uint digits )
{
	char buf[100], text[100];
	sprintf ( buf, "%" GST_TIME_FORMAT, GST_TIME_ARGS ( pos_dur ) );
	snprintf ( text, strlen ( buf ) - digits + 1, "%s", buf );

	gtk_label_set_text ( label, text );
}

static void slider_set_range ( double range, double value, Slider *level )
{
	g_signal_handler_block   ( level->slider, level->slider_signal_id );

		gtk_range_set_range  ( GTK_RANGE ( level->slider ), 0, range );
		gtk_range_set_value  ( GTK_RANGE ( level->slider ),    value );

	g_signal_handler_unblock ( level->slider, level->slider_signal_id );
}

void slider_update ( gint64 pos, uint digits_pos, gint64 dur, uint digits_dur, gboolean sensitive, Slider *level )
{
	slider_set_text ( level->lab_pos, pos, digits_pos );

	if ( dur > -1 ) slider_set_text ( level->lab_dur, dur, digits_dur );

	gtk_widget_set_sensitive ( GTK_WIDGET ( level ), sensitive );

	if ( sensitive ) slider_set_range ( (double)dur / GST_SECOND, (double)pos / GST_SECOND, level );
}

static void slider_seek_changed ( GtkRange *range, Slider *level )
{
	double value = gtk_range_get_value ( GTK_RANGE (range) );

	g_signal_emit_by_name ( level, "slider-seek", value );
}

static void slider_init ( Slider *level )
{
	GtkBox *box = GTK_BOX ( level );
	gtk_orientable_set_orientation ( GTK_ORIENTABLE ( box ), GTK_ORIENTATION_HORIZONTAL );
	gtk_box_set_spacing ( box, 3 );

	level->lab_pos = (GtkLabel *)gtk_label_new ( "0:00:00" );
	level->lab_dur = (GtkLabel *)gtk_label_new ( "0:00:00" );

	level->slider  = (GtkScale *)gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0, 120*60, 1 );
	level->slider_signal_id = g_signal_connect ( level->slider, "value-changed", G_CALLBACK ( slider_seek_changed ), level );

	gtk_scale_set_draw_value ( level->slider, 0 );
	gtk_range_set_value ( GTK_RANGE ( level->slider ), 0 );

	gtk_widget_set_margin_start ( GTK_WIDGET ( GTK_BOX ( box ) ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( GTK_BOX ( box ) ), 10 );
	gtk_box_set_spacing ( GTK_BOX ( box ), 5 );

	gtk_widget_set_visible ( GTK_WIDGET ( level->lab_pos ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( level->lab_dur ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( level->slider  ), TRUE );

	gtk_box_pack_start ( GTK_BOX ( box ), GTK_WIDGET ( level->lab_pos ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( GTK_BOX ( box ), GTK_WIDGET ( level->slider  ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( GTK_BOX ( box ), GTK_WIDGET ( level->lab_dur ), FALSE, FALSE, 0 );

	gtk_widget_set_sensitive ( GTK_WIDGET ( box ), FALSE );
}

static void slider_finalize ( GObject *object )
{
	G_OBJECT_CLASS (slider_parent_class)->finalize (object);
}

static void slider_class_init ( SliderClass *class )
{
	G_OBJECT_CLASS (class)->finalize = slider_finalize;

	g_signal_new ( "slider-seek", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_DOUBLE );
}

Slider * slider_new ( void )
{
	return g_object_new ( SLIDER_TYPE_PLAYER, NULL );
}
