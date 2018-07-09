/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/

#include "helia-panel-pl.h"
#include "helia-base.h"
#include "helia-media-pl.h"
#include "helia-service.h"

#include "helia-gst-pl.h"


typedef struct _PanelMp PanelMp;

struct _PanelMp
{
	GtkWindow *window;
	GtkVolumeButton *volbutton;
	GtkButton *button_play, *button_pause, *button_stop, *button_rec;
	GtkButton *button_text, *button_subtitle, *button_audio;
	GtkBox *m_box, *vbox, *hbox;
	
	guint size_icon;
	
	GtkBox *hbox_slider;
	HMPlSlider sl_menu;	
};

static PanelMp pmp;


static gboolean helia_panel_pl_update_text_audio ()
{
	if ( helia_gst_pl_get_current_state_playing () )
	{
		helia_gst_pl_set_text_audio ( pmp.button_text,  TRUE,  TRUE,  FALSE, FALSE );
		helia_gst_pl_set_text_audio ( pmp.button_audio, FALSE, FALSE, TRUE,  TRUE  );

		return FALSE;
	}
	else
	{
		g_debug ( "Not GST_STATE_PLAYING" );
		
		return TRUE;
	}

	return FALSE;
}

static void helia_panel_pl_hide_show ( GtkButton *button_hide, GtkButton *button_show, GtkVolumeButton *volbutton, gboolean sensitive )
{
	gtk_widget_hide ( GTK_WIDGET ( button_hide ) );
	gtk_widget_show ( GTK_WIDGET ( button_show ) );

	gtk_widget_set_sensitive ( GTK_WIDGET ( volbutton ), sensitive );	
}

static void helia_panel_pl_base ()
{
	helia_gst_pl_playback_stop ();
	gtk_widget_destroy ( GTK_WIDGET ( pmp.window ) );
	helia_base_set_window ();

	g_debug ( "helia_panel_pl_base" );
}
static void helia_panel_pl_editor ()
{
	gtk_widget_get_visible ( GTK_WIDGET ( pmp.vbox ) ) ? gtk_widget_hide ( GTK_WIDGET ( pmp.vbox ) ) : gtk_widget_show ( GTK_WIDGET ( pmp.vbox ) );

	g_debug ( "helia_panel_pl_editor" );
}
static void helia_panel_pl_eqa ()
{
	if ( helia_gst_pl_ega () ) gtk_widget_destroy ( GTK_WIDGET ( pmp.window ) );

	g_debug ( "helia_panel_pl_eqa" );
}
static void helia_panel_pl_eqv ()
{
	if ( helia_gst_pl_egv () ) gtk_widget_destroy ( GTK_WIDGET ( pmp.window ) );

	g_debug ( "helia_panel_pl_eqv" );
}
static void helia_panel_pl_muted ()
{
	helia_gst_pl_mute_set_widget ( GTK_WIDGET ( pmp.volbutton ) );
	
	HGPlState hgpls = helia_gst_pl_state_all ();

	gtk_widget_set_sensitive ( GTK_WIDGET ( pmp.volbutton ), !hgpls.state_mute );

	g_debug ( "helia_panel_pl_muted" );
}
static void helia_panel_pl_media_start ()
{
	if ( !helia_gst_pl_playback_play  () ) return;
	
	HGPlState hgpls = helia_gst_pl_state_all ();
	
	g_timeout_add ( 250, (GSourceFunc)helia_panel_pl_update_text_audio, NULL );

	helia_panel_pl_hide_show ( pmp.button_play, pmp.button_pause, pmp.volbutton, !hgpls.state_mute );

	gtk_widget_show ( GTK_WIDGET ( pmp.button_text  ) );
	gtk_widget_show ( GTK_WIDGET ( pmp.button_subtitle  ) );
	gtk_widget_show ( GTK_WIDGET ( pmp.button_audio ) );

	g_debug ( "helia_panel_pl_media_start" );
}
static void helia_panel_pl_media_pause ()
{
	if ( !helia_gst_pl_playback_pause () ) return; 

	helia_panel_pl_hide_show ( pmp.button_pause, pmp.button_play,  pmp.volbutton, FALSE );

	g_debug ( "helia_panel_pl_media_pause" );
}
static void helia_panel_pl_media_stop ()
{
	if ( !helia_gst_pl_playback_stop  () ) return; 

	helia_panel_pl_hide_show ( pmp.button_pause, pmp.button_play,  pmp.volbutton, FALSE );

	gtk_widget_hide ( GTK_WIDGET ( pmp.button_text  ) );
	gtk_widget_hide ( GTK_WIDGET ( pmp.button_subtitle  ) );
	gtk_widget_hide ( GTK_WIDGET ( pmp.button_audio ) );

	g_debug ( "helia_panel_pl_media_stop" );
}
static void helia_panel_pl_media_record ()
{
	helia_gst_pl_playback_record ();

	g_debug ( "helia_panel_pl_media_record" );
}
static void helia_panel_pl_slider ()
{
	gtk_widget_get_visible ( GTK_WIDGET ( pmp.hbox ) ) ? gtk_widget_hide ( GTK_WIDGET ( pmp.hbox ) ) : gtk_widget_show ( GTK_WIDGET ( pmp.hbox ) );
	
	gtk_widget_get_visible ( GTK_WIDGET ( pmp.hbox ) ) ? gtk_widget_hide ( GTK_WIDGET ( pmp.hbox_slider ) ) : gtk_widget_show ( GTK_WIDGET ( pmp.hbox_slider ) );
	
	gtk_widget_get_visible ( GTK_WIDGET ( pmp.hbox_slider ) ) ? gtk_widget_set_margin_bottom ( GTK_WIDGET ( pmp.m_box ), 0 ) : gtk_widget_set_margin_bottom ( GTK_WIDGET ( pmp.m_box ), 10 );
	
	gtk_window_resize ( pmp.window, 400, pmp.size_icon * 2 );

	g_debug ( "helia_panel_pl_slider" );
}
static void helia_panel_pl_exit ()
{
	gtk_widget_destroy ( GTK_WIDGET ( pmp.window ) );

	g_debug ( "helia_panel_pl_exit" );
}



static void helia_panel_pl_volbutton ( GtkVolumeButton *volbutton )
{
	HGPlState hgpls = helia_gst_pl_state_all ();
	
	gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( volbutton ), hgpls.volume_pl );
	g_signal_connect ( volbutton, "value-changed", G_CALLBACK ( helia_gst_pl_volume_changed ), NULL );

	gtk_widget_set_sensitive ( GTK_WIDGET ( volbutton ), !hgpls.state_mute );
}

static void helia_panel_pl_clicked_audio ( GtkButton *button )
{
	helia_gst_pl_set_text_audio ( button, FALSE, FALSE, TRUE, FALSE );
}

static GtkButton * helia_panel_pl_create_audio_button ()
{
	GtkButton *button = (GtkButton *)gtk_button_new ();
	helia_gst_pl_set_text_audio ( button, FALSE, FALSE, TRUE, TRUE );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_panel_pl_clicked_audio ), NULL );

	return button;
}

static void helia_panel_pl_clicked_text ( GtkButton *button )
{	
	helia_gst_pl_set_text_audio ( button, TRUE, FALSE, FALSE, FALSE );
}

static GtkButton * helia_panel_pl_create_text_button ()
{	
	GtkButton *button = (GtkButton *)gtk_button_new ();
	helia_gst_pl_set_text_audio ( button, TRUE, TRUE, FALSE, FALSE );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_panel_pl_clicked_text ), NULL );
	
	return button;
}

static void helia_panel_pl_clicked_subtitle ( GtkButton *button )
{
	helia_gst_pl_playbin_change_state_subtitle ();
	
	HGPlState hgpls = helia_gst_pl_state_all ();

	GdkPixbuf *logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					  ( hgpls.state_subtitle ) ? "helia-set" : "helia-unset", 16, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	GtkImage *image = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );

	gtk_button_set_image ( button, GTK_WIDGET ( image ) );
	
	gtk_widget_set_sensitive ( GTK_WIDGET ( pmp.button_text ), hgpls.state_subtitle );
}

static GtkButton * helia_panel_pl_create_subtitle_button ()
{
	HGPlState hgpls = helia_gst_pl_state_all ();
	
	GtkButton *button = helia_service_ret_image_button ( ( hgpls.state_subtitle ) ? "helia-set" : "helia-unset", 16 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_panel_pl_clicked_subtitle ), NULL );	
	
	return button;
}




void helia_panel_pl_set_sensitive ( gboolean sensitive )
{
	if ( !pmp.window ) return;
	
	gtk_widget_set_sensitive ( GTK_WIDGET ( pmp.sl_menu.slider ), sensitive );
	
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_set_sensitive" );
}
static void helia_panel_pl_label_set_text ( GtkLabel *label, gint64 pos_dur, guint digits )
{
	gchar *str   = g_strdup_printf ( "%" GST_TIME_FORMAT, GST_TIME_ARGS ( pos_dur ) );
	gchar *str_l = g_strndup ( str, strlen ( str ) - digits );

	gtk_label_set_text ( label, str_l );

	g_free ( str_l  );
	g_free ( str );
}
void helia_panel_pl_pos_dur_text ( gint64 pos, gint64 dur, guint digits )
{
	if ( !pmp.window ) return;
	
	helia_panel_pl_label_set_text ( pmp.sl_menu.lab_pos, pos, digits );
	helia_panel_pl_label_set_text ( pmp.sl_menu.lab_dur, dur, digits );
}
static void helia_panel_pl_slider_update ( GtkRange *range, gdouble data )
{
	g_signal_handler_block   ( range, pmp.sl_menu.slider_signal_id );
		gtk_range_set_value  ( range, data );
	g_signal_handler_unblock ( range, pmp.sl_menu.slider_signal_id );
}
void helia_panel_pl_pos_dur_slider ( gint64 current, gint64 duration, guint digits )
{
	if ( !pmp.window ) return;
	
	gdouble slider_max = (gdouble)duration / GST_SECOND;

	gtk_range_set_range ( GTK_RANGE ( pmp.sl_menu.slider ), 0, slider_max );

	helia_panel_pl_slider_update ( GTK_RANGE ( pmp.sl_menu.slider ), (gdouble)current / GST_SECOND );

	helia_panel_pl_pos_dur_text ( current, duration, digits );
}
static void helia_panel_pl_slider_changed ( GtkRange *range )
{
	gdouble value = gtk_range_get_value ( GTK_RANGE (range) );

		helia_gst_pl_slider_changed ( value );

	helia_panel_pl_label_set_text ( pmp.sl_menu.lab_pos, (gint64)( value * GST_SECOND ), 8 );
}
static GtkBox * helia_panel_pl_slider_menu ()
{
	pmp.sl_menu = helia_media_pl_create_slider ( helia_panel_pl_slider_changed );

	GtkBox *hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	//gtk_widget_set_margin_start ( GTK_WIDGET ( hbox ), 10 );
	//gtk_widget_set_margin_end   ( GTK_WIDGET ( hbox ), 10 );
	gtk_box_set_spacing ( hbox, 5 );

	gtk_box_pack_start ( hbox, GTK_WIDGET ( pmp.sl_menu.lab_pos ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( pmp.sl_menu.slider  ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( pmp.sl_menu.lab_dur ), FALSE, FALSE, 0 );

	gtk_widget_set_size_request ( GTK_WIDGET ( pmp.sl_menu.slider ), 200, -1 );

	return hbox;
}
static void helia_panel_pl_set_slider_value ( GtkScale *slider )
{
	gdouble dur = 0, val = gtk_range_get_value ( GTK_RANGE ( slider ) );

	GtkAdjustment *adjustment = gtk_range_get_adjustment ( GTK_RANGE ( slider ) );
	g_object_get ( adjustment, "upper", &dur, NULL );
	
	gtk_range_set_range ( GTK_RANGE ( pmp.sl_menu.slider ), 0, dur );
	
	helia_panel_pl_slider_update ( GTK_RANGE ( pmp.sl_menu.slider ), val );
	
	helia_panel_pl_pos_dur_text ( (gint64)( val * GST_SECOND ), (gint64)( dur * GST_SECOND ), 8 );
}


static void helia_panel_pl_quit ( /*GtkWindow *window*/ )
{
	pmp.window = NULL;
}

void helia_panel_pl ( GtkBox *vbox, GtkBox *hbox_sl, GtkScale *slider, GtkWindow *parent, gdouble opacity, guint resize_icon )
{
	HGPlState hgpls = helia_gst_pl_state_all ();
	
	pmp.vbox = vbox;
	pmp.hbox = hbox_sl;
	pmp.size_icon = resize_icon;

	pmp.window = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( pmp.window, parent );
	gtk_window_set_modal     ( pmp.window, TRUE   );
	gtk_window_set_decorated ( pmp.window, FALSE  );
	gtk_window_set_position  ( pmp.window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_default_size ( pmp.window, 400, resize_icon * 2 );
	g_signal_connect ( pmp.window, "destroy", G_CALLBACK ( helia_panel_pl_quit ), NULL );


	pmp.m_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );

	GtkBox *b_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	GtkBox *l_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *r_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *a_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );

	GtkBox *h_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	GtkBox *hm_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_box_set_spacing ( b_box,  5 );
	gtk_box_set_spacing ( l_box,  5 );
	gtk_box_set_spacing ( h_box,  5 );
	gtk_box_set_spacing ( hm_box, 5 );
	gtk_box_set_spacing ( a_box,  5 );

	pmp.volbutton = (GtkVolumeButton *)gtk_volume_button_new ();
	helia_panel_pl_volbutton ( pmp.volbutton );


const struct PanelMpNameIcon { const gchar *name; void (* activate)(); } NameIcon_n[] =
{
	{ "helia-mp", 			helia_panel_pl_base			},
	{ "helia-editor", 		helia_panel_pl_editor 		},
	{ "helia-eqa", 			helia_panel_pl_eqa 			},
	{ "helia-eqv", 			helia_panel_pl_eqv 			},
	{ "helia-muted", 		helia_panel_pl_muted 		},

	{ "helia-media-start", 	helia_panel_pl_media_start 	},
	{ "helia-media-pause", 	helia_panel_pl_media_pause 	},
	{ "helia-media-stop", 	helia_panel_pl_media_stop 	},
	{ "helia-media-record", helia_panel_pl_media_record },
	{ "helia-slider", 		helia_panel_pl_slider 		},
	{ "helia-exit", 		helia_panel_pl_exit 		},
};

	guint i = 0;
	for ( i = 0; i < G_N_ELEMENTS ( NameIcon_n ); i++ )
	{
		GtkButton *button = helia_service_ret_image_button ( NameIcon_n[i].name, resize_icon );
		g_signal_connect ( button, "clicked", G_CALLBACK ( NameIcon_n[i].activate ), NULL );

		if ( i == 4 ) gtk_box_pack_start ( l_box, GTK_WIDGET ( h_box ), TRUE, TRUE, 0 );

		gtk_box_pack_start ( ( i < 5 ) ? h_box : hm_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );

		if ( i == 5 ) pmp.button_play  = button;
		if ( i == 6 ) pmp.button_pause = button;
		if ( i == 7 ) pmp.button_stop  = button;
		if ( i == 8 ) pmp.button_rec   = button;
	}

	gtk_box_pack_start ( l_box, GTK_WIDGET ( hm_box ), TRUE, TRUE, 0 );

	gtk_widget_set_sensitive ( GTK_WIDGET ( pmp.button_rec ), FALSE );


	GtkBox *th_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	pmp.button_text = helia_panel_pl_create_text_button ();
	gtk_box_pack_start ( th_box, GTK_WIDGET ( pmp.button_text ), TRUE, TRUE, 0 );
	
	pmp.button_subtitle =  helia_panel_pl_create_subtitle_button ();
	gtk_box_pack_start ( th_box, GTK_WIDGET ( pmp.button_subtitle ), FALSE, FALSE, 0 );
	
	gtk_box_pack_start ( a_box, GTK_WIDGET ( th_box ), TRUE, TRUE, 0 );
	gtk_widget_set_sensitive ( GTK_WIDGET ( pmp.button_text ), hgpls.state_subtitle );

	pmp.button_audio = helia_panel_pl_create_audio_button ();
	gtk_box_pack_end ( a_box, GTK_WIDGET ( pmp.button_audio ), TRUE, TRUE, 0 );


	gtk_box_pack_start ( r_box, GTK_WIDGET ( pmp.volbutton ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( b_box, GTK_WIDGET ( l_box ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( b_box, GTK_WIDGET ( a_box ), FALSE, FALSE, 0 );
	gtk_box_pack_end   ( b_box, GTK_WIDGET ( r_box ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( pmp.m_box, GTK_WIDGET ( b_box ), TRUE,  TRUE,  0 );


	pmp.hbox_slider = helia_panel_pl_slider_menu ();
	gtk_box_pack_start ( pmp.m_box, GTK_WIDGET ( pmp.hbox_slider ), FALSE, FALSE, 0 );
	
	if ( !helia_gst_pl_get_current_state_stop () ) helia_panel_pl_set_slider_value ( slider );


	gtk_widget_set_margin_top    ( GTK_WIDGET ( pmp.m_box ), 10 );
	//gtk_widget_set_margin_bottom ( GTK_WIDGET ( pmp.m_box ), 10 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( pmp.m_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( pmp.m_box ), 10 );

	//gtk_container_set_border_width ( GTK_CONTAINER ( pmp.m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( pmp.window ), GTK_WIDGET ( pmp.m_box ) );
	gtk_widget_show_all ( GTK_WIDGET ( pmp.window ) );
	
	if ( helia_gst_pl_get_current_state_stop () )
	{
		gtk_widget_hide ( GTK_WIDGET ( pmp.button_text  ) );
		gtk_widget_hide ( GTK_WIDGET ( pmp.button_subtitle  ) );
		gtk_widget_hide ( GTK_WIDGET ( pmp.button_audio ) );
	}
	
	helia_gst_pl_get_current_state_playing () ? gtk_widget_hide ( GTK_WIDGET ( pmp.button_play ) ) : gtk_widget_hide ( GTK_WIDGET ( pmp.button_pause ) );
	
	
	helia_panel_pl_set_sensitive ( helia_gst_pl_get_current_state_stop () ? FALSE : TRUE );
	gtk_widget_get_visible ( GTK_WIDGET ( pmp.hbox ) ) ? gtk_widget_hide ( GTK_WIDGET ( pmp.hbox_slider ) ) : gtk_widget_show ( GTK_WIDGET ( pmp.hbox_slider ) );
	
	gtk_widget_get_visible ( GTK_WIDGET ( pmp.hbox_slider ) ) ? gtk_widget_set_margin_bottom ( GTK_WIDGET ( pmp.m_box ), 0 ) : gtk_widget_set_margin_bottom ( GTK_WIDGET ( pmp.m_box ), 10 );
	

	gtk_widget_set_opacity ( GTK_WIDGET ( pmp.window ), opacity );

	gtk_window_resize ( pmp.window, 400, resize_icon * 2 );
}
