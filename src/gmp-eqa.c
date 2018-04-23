/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/


#include "gmp-eqa.h"
#include "gmp-pref.h"
#include "gmp-dvb.h"

#include <glib/gi18n.h>


#define BAND_N 10
gpointer dat_gfb[3][BAND_N];

static struct GstEQAudioData { const gchar *name; const gchar *desc; } eqa_n[] =
{
    { N_("Level  dB"    ),  "gain"      },
    { N_("Frequency  Hz"),  "freq"      },
    { N_("Bandwidth  Hz"),  "bandwidth" }
};


static void gmp_all_changed ( GtkRange *range, gpointer data, gint ind )
{
    GstObject *band = GST_OBJECT ( data );
    gdouble value = gtk_range_get_value ( range );

    g_object_set ( band, eqa_n[ind].desc, value, NULL );

	g_debug ( "gmp_all_changed" );
}

static void gmp_gain_changed ( GtkRange *range, gpointer data )
{
    gmp_all_changed ( range, data, 0 );

	g_debug ( "gmp_gain_changed" );
}
static void gmp_freq_changed ( GtkRange *range, gpointer data )
{
    gmp_all_changed ( range, data, 1 );

	g_debug ( "gmp_freq_changed" );
}
static void gmp_band_changed ( GtkRange *range, gpointer data )
{
    gmp_all_changed ( range, data, 2 );

	g_debug ( "gmp_band_changed" );
}

static void gmp_default_eqa ()
{
    gdouble g = 0.0, f = 30, b = 20;

	guint i = 0;
    for ( i = 0; i < BAND_N; i++ )
    {
        gtk_range_set_value ( GTK_RANGE ( (GtkRange *)dat_gfb[0][i] ), g );
        gtk_range_set_value ( GTK_RANGE ( (GtkRange *)dat_gfb[1][i] ), f );
        gtk_range_set_value ( GTK_RANGE ( (GtkRange *)dat_gfb[2][i] ), b );

        f = f * 2; b = b * 2;
    }

	g_debug ( "gmp_default_eqa" );
}

static void gmp_clear_eqa () 
{ 
	gmp_default_eqa (); 

	g_debug ( "gmp_clear_eqa" );
}

static void gmp_create_label_box ( GtkBox *vbox )
{
    GtkBox *hboxl = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
    GtkLabel *label;

	guint i = 0;
    for ( i = 0; i < G_N_ELEMENTS ( eqa_n ); i++ )
    {
        label = (GtkLabel *)gtk_label_new ( _(eqa_n[i].name) );
        gtk_box_pack_start ( hboxl, GTK_WIDGET ( label ), TRUE, TRUE, 10 );
    }

    gtk_box_pack_start ( vbox,  GTK_WIDGET ( hboxl  ), TRUE, TRUE,  0 );

	g_debug ( "gmp_create_label_box" );
}

static GtkScale * gmp_create_g_f_b_widget ( GtkBox *scales_hbox, gdouble g_f_b, gdouble min_v, gdouble max_v, gdouble step_v )
{
	g_debug ( "gmp_create_g_f_b_widget" );

    GtkScale *widget =  (GtkScale *)gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, min_v, max_v, step_v );
    gtk_range_set_value ( GTK_RANGE ( widget ), g_f_b );

    gtk_widget_set_size_request ( GTK_WIDGET ( widget ), 100, -1 );
    gtk_box_pack_start ( scales_hbox, GTK_WIDGET ( widget ), TRUE, TRUE, 5 );

    gtk_scale_set_draw_value ( GTK_SCALE ( widget ), 1 );

    return widget;
}

static GtkButton * gmp_ega_button ( gchar *icon, guint size, void (* activate)(), gpointer data )
{
	GdkPixbuf *logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
              icon, size, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

  	GtkButton *button = (GtkButton *)gtk_button_new ();
  	GtkImage *image   = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );
 
	gtk_button_set_image ( button, GTK_WIDGET ( image ) );

  	g_signal_connect ( button, "clicked", G_CALLBACK (activate), data );

	g_debug ( "gmp_ega_button" );

	return button;
}

static void gmp_eqa_win_quit ( GtkWindow *window )
{
    gtk_widget_destroy ( GTK_WIDGET ( window ) );

	g_debug ( "gmp_eqa_win_quit" );
}

static void gmp_eqa_win_close ( GtkButton *button, GtkWindow *window )
{
    gmp_eqa_win_quit ( window );

	g_debug ( "gmp_eqa_win_close:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

void gmp_eqa_win ( GstElement *element )
{
    GtkBox *vbox_main, *vbox, *h_box;
    GtkScale *widget;

    GtkWindow *window_eq_audio = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_transient_for ( window_eq_audio, gmp_base_win_ret () );
    gtk_window_set_title    ( window_eq_audio, _("Audio equalizer") );
    gtk_window_set_modal    ( window_eq_audio, TRUE );
    gtk_window_set_position ( window_eq_audio, GTK_WIN_POS_CENTER_ON_PARENT );
    // g_signal_connect ( G_OBJECT ( window_eq_audio ), "destroy", G_CALLBACK ( gmp_eqa_win_quit ), NULL );

    vbox_main = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
    gtk_widget_set_margin_top    ( GTK_WIDGET ( vbox_main ), 10 );

    vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

    gmp_create_label_box ( vbox_main );

    g_object_set ( G_OBJECT ( element ), "num-bands", BAND_N, NULL );

	guint c = 0;
    for ( c = 0; c < BAND_N; c++ )
    {
        gdouble freq, bw, gain;
        GObject *band = gst_child_proxy_get_child_by_index ( GST_CHILD_PROXY ( element ), c );

        g_assert ( band != NULL );
        g_object_get ( band, "gain",      &gain, NULL );
        g_object_get ( band, "freq",      &freq, NULL );
        g_object_get ( band, "bandwidth", &bw,   NULL );

        GtkBox *scales_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

        widget = gmp_create_g_f_b_widget ( scales_hbox, gain, -24.0, 12.0, 1.0 );
        g_signal_connect ( G_OBJECT ( widget ), "value-changed", G_CALLBACK ( gmp_gain_changed ), (gpointer)band );
        dat_gfb[0][c] = (gpointer)widget;

        widget = gmp_create_g_f_b_widget ( scales_hbox, freq, 10.0, 20000.0, 5.0 );
        g_signal_connect ( G_OBJECT ( widget ), "value-changed", G_CALLBACK ( gmp_freq_changed ), (gpointer)band );
        dat_gfb[1][c] = (gpointer)widget;

        widget = gmp_create_g_f_b_widget ( scales_hbox, bw, 10.0, 20000.0, 10.0 );
        g_signal_connect ( G_OBJECT ( widget ), "value-changed", G_CALLBACK ( gmp_band_changed ), (gpointer)band );
        dat_gfb[2][c] = (gpointer)widget;

        gtk_box_pack_start ( vbox, GTK_WIDGET ( scales_hbox ), TRUE, TRUE, 0 );
    }

    gtk_box_pack_start ( vbox_main, GTK_WIDGET ( vbox ), TRUE, TRUE, 0 );

    h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
    gtk_widget_set_margin_end    ( GTK_WIDGET ( h_box ), 10 );
    gtk_widget_set_margin_top    ( GTK_WIDGET ( h_box ), 10 );
	
	gtk_box_pack_end ( h_box, GTK_WIDGET ( gmp_ega_button ( "gmp-exit",  16, gmp_eqa_win_close, window_eq_audio ) ), FALSE, FALSE, 0 );
	gtk_box_pack_end ( h_box, GTK_WIDGET ( gmp_ega_button ( "gmp-clear", 16, gmp_clear_eqa, NULL ) ), FALSE, FALSE, 5 );

    gtk_box_pack_start ( vbox_main, GTK_WIDGET ( h_box ), FALSE, FALSE, 10 );

    gtk_container_add ( GTK_CONTAINER ( window_eq_audio ), GTK_WIDGET ( vbox_main ) );
    gtk_container_set_border_width ( GTK_CONTAINER ( vbox_main ), 5 );

    gtk_widget_show_all ( GTK_WIDGET ( window_eq_audio ) );

    gtk_widget_set_opacity ( GTK_WIDGET ( window_eq_audio ), opacity_eq );

	g_debug ( "gmp_eqa_win" );
}
