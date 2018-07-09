/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/

#include "helia-panel-tv.h"
#include "helia-base.h"
#include "helia-service.h"

#include "helia-gst-tv.h"
#include "helia-scan.h"
#include "helia-convert.h"


typedef struct _PanelTv PanelTv;

struct _PanelTv
{
	GtkWindow *window;
	GtkVolumeButton *volbutton;
	GtkButton *button_text, *button_audio;
	GtkBox *vbox;
};

static PanelTv ptv;


static void helia_panel_tv_base ()
{
	helia_gst_tv_stop ();

	gtk_widget_destroy ( GTK_WIDGET ( ptv.window ) );
	
	helia_base_set_window ();

	g_debug ( "helia_panel_tv_base" );
}
static void helia_panel_tv_editor ()
{
	gtk_widget_get_visible ( GTK_WIDGET ( ptv.vbox ) ) ? gtk_widget_hide ( GTK_WIDGET ( ptv.vbox ) ) : gtk_widget_show ( GTK_WIDGET ( ptv.vbox ) );

	g_debug ( "helia_panel_tv_editor" );
}
static void helia_panel_tv_eqa ()
{
	if ( helia_gst_tv_ega () ) gtk_widget_destroy ( GTK_WIDGET ( ptv.window ) );

	g_debug ( "helia_panel_tv_eqa" );
}
static void helia_panel_tv_eqv ()
{
	if ( helia_gst_tv_egv () ) gtk_widget_destroy ( GTK_WIDGET ( ptv.window ) );

	g_debug ( "helia_panel_tv_eqv" );
}
static void helia_panel_tv_muted ()
{
	helia_gst_tv_mute_set_widget ( GTK_WIDGET ( ptv.volbutton ) );

	HGTvState hgtvs = helia_gst_tv_state_all ();

	gtk_widget_set_sensitive ( GTK_WIDGET ( ptv.volbutton ), !hgtvs.state_mute );

	g_debug ( "helia_panel_tv_muted" );
}
static void helia_panel_tv_media_stop ()
{
	helia_gst_tv_stop ();

	gtk_widget_set_sensitive ( GTK_WIDGET ( ptv.volbutton ), FALSE );

	gtk_widget_hide ( GTK_WIDGET ( ptv.button_text ) );
	gtk_widget_hide ( GTK_WIDGET ( ptv.button_audio ) );

	g_debug ( "helia_panel_tv_media_stop" );
}
static void helia_panel_tv_media_record ()
{
	helia_gst_tv_record ();

	g_debug ( "helia_panel_tv_media_record" );
}
static void helia_panel_tv_display ()
{
	helia_scan_win ();

	gtk_widget_destroy ( GTK_WIDGET ( ptv.window ) );

	g_debug ( "helia_panel_tv_display" );
}
static void helia_panel_tv_convert ()
{
	helia_convert_win ( NULL );

	gtk_widget_destroy ( GTK_WIDGET ( ptv.window ) );

	g_debug ( "helia_panel_tv_convert" );
}
static void helia_panel_tv_exit ()
{
	gtk_widget_destroy ( GTK_WIDGET ( ptv.window ) );

	g_debug ( "helia_panel_tv_exit" );
}


static void helia_panel_tv_volbutton ( GtkVolumeButton *volbutton )
{
	HGTvState hgtvs = helia_gst_tv_state_all ();
	
	gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( volbutton ), hgtvs.volume_tv );
	g_signal_connect ( volbutton, "value-changed", G_CALLBACK ( helia_gst_tv_volume_changed ), NULL );

	gtk_widget_set_sensitive ( GTK_WIDGET ( volbutton ), !hgtvs.state_mute );
}

static void helia_panel_tv_clicked_audio ( GtkButton *button )
{	
	helia_gst_tv_set_text_audio ( button, FALSE, FALSE, TRUE, FALSE );
}

static GtkButton * helia_panel_tv_create_audio_button ()
{	
	GtkButton *button = (GtkButton *)gtk_button_new ();
	helia_gst_tv_set_text_audio ( button, FALSE, FALSE, TRUE, TRUE );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_panel_tv_clicked_audio ), NULL );

	return button;
}

static void helia_panel_tv_clicked_text ( GtkButton *button )
{
	helia_gst_tv_set_text_audio ( button, TRUE, FALSE, FALSE, FALSE );
}

static GtkButton * helia_panel_tv_create_text_button ()
{
	GtkButton *button = (GtkButton *)gtk_button_new ();
	helia_gst_tv_set_text_audio ( button, TRUE, TRUE, FALSE, FALSE );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_panel_tv_clicked_text ), NULL );

	return button;
}

void helia_panel_tv ( GtkBox *vbox, GtkWindow *parent, gdouble opacity, guint resize_icon )
{
	ptv.vbox = vbox;

	ptv.window = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( ptv.window, parent );
	gtk_window_set_modal     ( ptv.window, TRUE   );
	gtk_window_set_decorated ( ptv.window, FALSE  );
	gtk_window_set_position  ( ptv.window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_default_size ( ptv.window, 400, resize_icon * 2 );


	GtkBox *m_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );

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

	ptv.volbutton = (GtkVolumeButton *)gtk_volume_button_new ();
	helia_panel_tv_volbutton ( ptv.volbutton );


const struct PanelTvNameIcon { const gchar *name; void (* activate)(); } NameIcon_n[] =
{
	{ "helia-tv", 			helia_panel_tv_base			},
	{ "helia-editor", 		helia_panel_tv_editor 		},
	{ "helia-eqa", 			helia_panel_tv_eqa 			},
	{ "helia-eqv", 			helia_panel_tv_eqv 			},
	{ "helia-muted", 		helia_panel_tv_muted 		},

	{ "helia-media-stop", 	helia_panel_tv_media_stop 	},
	{ "helia-media-record", helia_panel_tv_media_record },
	{ "helia-display", 		helia_panel_tv_display 		},
	{ "helia-convert", 		helia_panel_tv_convert 		},
	{ "helia-exit", 		helia_panel_tv_exit 		}
};
	
	guint i = 0;
	for ( i = 0; i < G_N_ELEMENTS ( NameIcon_n ); i++ )
	{
		GtkButton *button = helia_service_ret_image_button ( NameIcon_n[i].name, resize_icon );
		g_signal_connect ( button, "clicked", G_CALLBACK ( NameIcon_n[i].activate ), NULL );

		if ( i == 4 ) gtk_box_pack_start ( l_box, GTK_WIDGET ( h_box ), TRUE, TRUE, 0 );

		gtk_box_pack_start ( ( i < 5 ) ? h_box : hm_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	}

	gtk_box_pack_start ( l_box, GTK_WIDGET ( hm_box ), TRUE, TRUE, 0 );


	ptv.button_text  = helia_panel_tv_create_text_button  ();
	gtk_box_pack_start ( a_box, GTK_WIDGET ( ptv.button_text ), TRUE, TRUE, 0 );

	ptv.button_audio = helia_panel_tv_create_audio_button ();
	gtk_box_pack_end ( a_box, GTK_WIDGET ( ptv.button_audio  ), TRUE, TRUE, 0 );
	

	gtk_box_pack_start ( r_box, GTK_WIDGET ( ptv.volbutton ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( b_box, GTK_WIDGET ( l_box ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( b_box, GTK_WIDGET ( a_box ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( b_box, GTK_WIDGET ( r_box ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( b_box ), TRUE,  TRUE,  0 );


	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( ptv.window ), GTK_WIDGET ( m_box ) );
	gtk_widget_show_all ( GTK_WIDGET ( ptv.window ) );

	if ( helia_gst_tv_get_state_stop () )
	{
		gtk_widget_hide ( GTK_WIDGET ( ptv.button_text  ) );
		gtk_widget_hide ( GTK_WIDGET ( ptv.button_audio ) );
	}
	
	gtk_widget_set_opacity ( GTK_WIDGET ( ptv.window ), opacity );

	gtk_window_resize ( ptv.window, 400, resize_icon * 2 );
}

