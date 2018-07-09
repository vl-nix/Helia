/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia-gst-pl.h"
#include "helia-media-pl.h"
#include "helia-base.h"
#include "helia-service.h"
#include "helia-pref.h"

#include "helia-panel-pl.h"

#include "helia-eqa.h"
#include "helia-eqv.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gst/video/videooverlay.h>

#include <stdlib.h>
#include <string.h>


typedef enum
{
	GST_PLAY_FLAG_VIDEO         = (1 << 0),
	GST_PLAY_FLAG_AUDIO         = (1 << 1),
	GST_PLAY_FLAG_TEXT          = (1 << 2),
	GST_PLAY_FLAG_VIS           = (1 << 3),
	GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
	GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
	GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
	GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
	GST_PLAY_FLAG_BUFFERING     = (1 << 8),
	GST_PLAY_FLAG_DEINTERLACE   = (1 << 9),
	GST_PLAY_FLAG_SOFT_COLORBALANCE = (1 << 10)
} GstPlayFlags;


typedef struct _HeliaGstPl HeliaGstPl;

struct _HeliaGstPl
{
	guintptr win_handle_pl;
	gboolean show_slider_menu, firstfile, state_subtitle;

	GstElement *playbin_pl, *videobln, *playequal;

	guint s_time_d;
	gdouble volume_pl;
};

static HeliaGstPl hgpl;


static GstBusSyncReply helia_gst_pl_bus_sync_handler ( GstBus *bus, GstMessage *message )
{
	if ( hbgv.all_info )
		g_debug ( "helia_gst_pl_bus_sync_handler:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );

	if ( !gst_is_video_overlay_prepare_window_handle_message ( message ) )
	{
		return GST_BUS_PASS;
	}

	guintptr handle = hgpl.win_handle_pl;

	if ( handle != 0 )
	{
		GstVideoOverlay *xoverlay = GST_VIDEO_OVERLAY ( GST_MESSAGE_SRC ( message ) );
		gst_video_overlay_set_window_handle ( xoverlay, handle );

	}
	else
		g_warning ( "Should have obtained video_window_handle by now!" );

	gst_message_unref ( message );

	return GST_BUS_DROP;
}

static void helia_gst_pl_msg_err ( GstBus *bus, GstMessage *msg )
{
	GError *err = NULL;
	gchar *dbg  = NULL;

	gst_message_parse_error ( msg, &err, &dbg );

	g_critical ( "helia_msg_err_pl:: %s (%s)\n", err->message, (dbg) ? dbg : "no details" );

	helia_service_message_dialog ( "ERROR:", err->message, GTK_MESSAGE_ERROR );

	g_error_free ( err );
	g_free ( dbg );

	helia_gst_pl_playback_stop ();

	if ( hbgv.all_info )
		g_debug ( "helia_msg_err_pl:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );
}


static void helia_gst_pl_msg_cll ()
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state == GST_STATE_PLAYING )
	{
		gst_element_set_state ( hgpl.playbin_pl, GST_STATE_PAUSED  );
		gst_element_set_state ( hgpl.playbin_pl, GST_STATE_PLAYING );
	}
}

static void helia_gst_pl_msg_eos ()
{
	helia_media_pl_next ();
}

static void helia_gst_pl_msg_buf ( GstBus *bus, GstMessage *msg )
{
	if ( hbgv.base_quit ) return;

	gint percent;
	gst_message_parse_buffering ( msg, &percent );

	if ( percent == 100 )
	{
		if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state == GST_STATE_PAUSED )
			//gst_element_set_state ( hgpl.playbin_pl, GST_STATE_PLAYING );
			helia_gst_pl_playback_play ();

		helia_media_pl_set_spinner ( "", TRUE );
	}
	else
	{
		if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state == GST_STATE_PLAYING )
			//gst_element_set_state ( hgpl.playbin_pl, GST_STATE_PAUSED );
			helia_gst_pl_playback_pause ();

		gchar *text = g_strdup_printf ( " %u%s ", percent, "%" );
		helia_media_pl_set_spinner ( text, FALSE );
		g_free ( text );

		g_debug ( "buffering: %d %s", percent, "%" );
	}

	if ( hbgv.all_info )
		g_debug ( "helia_msg_buf_pl:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );
}

gboolean helia_gst_pl_gst_create ()
{
	hgpl.volume_pl = 0.5;
	hgpl.state_subtitle = TRUE;

	hgpl.playbin_pl  = gst_element_factory_make ( "playbin", "playbin" );

	GstElement *bin_audio, *bin_video, *vsink, *asink;

	vsink  = gst_element_factory_make ( "autovideosink", NULL );
	hgpl.videobln  = gst_element_factory_make ( "videobalance",      NULL );


	asink  = gst_element_factory_make ( "autoaudiosink", NULL );
	hgpl.playequal = gst_element_factory_make ( "equalizer-nbands",  NULL );


	if ( !hgpl.playbin_pl || !vsink || !hgpl.videobln || !asink || !hgpl.playequal )
	{
		g_printerr ( "helia_gst_pl_gst_create - not all elements could be created.\n" );
		return FALSE;
	}

	bin_audio = gst_bin_new ( "audio_sink_bin" );
	gst_bin_add_many (GST_BIN ( bin_audio), hgpl.playequal, asink, NULL );
	gst_element_link_many ( hgpl.playequal, asink, NULL );

	GstPad *pad = gst_element_get_static_pad ( hgpl.playequal, "sink" );
	gst_element_add_pad ( bin_audio, gst_ghost_pad_new ( "sink", pad ) );
	gst_object_unref ( pad );


	bin_video = gst_bin_new ( "video_sink_bin" );
	gst_bin_add_many ( GST_BIN (bin_video), hgpl.videobln, vsink, NULL );
	gst_element_link_many ( hgpl.videobln, vsink, NULL );

	GstPad *padv = gst_element_get_static_pad ( hgpl.videobln, "sink" );
	gst_element_add_pad ( bin_video, gst_ghost_pad_new ( "sink", padv ) );
	gst_object_unref ( padv );


	g_object_set ( hgpl.playbin_pl, "video-sink", bin_video, NULL );
	g_object_set ( hgpl.playbin_pl, "audio-sink", bin_audio, NULL );

	g_object_set ( hgpl.playbin_pl, "volume", hgpl.volume_pl, NULL );


	GstBus *bus = gst_element_get_bus ( hgpl.playbin_pl );
	gst_bus_add_signal_watch_full ( bus, G_PRIORITY_DEFAULT );
	gst_bus_set_sync_handler ( bus, (GstBusSyncHandler)helia_gst_pl_bus_sync_handler, NULL, NULL );
	gst_object_unref ( bus );

	g_signal_connect ( bus, "message::eos",           G_CALLBACK ( helia_gst_pl_msg_eos ),  NULL );
	g_signal_connect ( bus, "message::error",         G_CALLBACK ( helia_gst_pl_msg_err ),  NULL );
	g_signal_connect ( bus, "message::clock-lost",    G_CALLBACK ( helia_gst_pl_msg_cll ),  NULL );
	g_signal_connect ( bus, "message::buffering",     G_CALLBACK ( helia_gst_pl_msg_buf ),  NULL );

	return TRUE;
}


void helia_gst_pl_slider_changed ( gdouble value )
{
	gst_element_seek_simple ( hgpl.playbin_pl, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, (gint64)( value * GST_SECOND ) );
}


static gboolean helia_gst_pl_refresh ()
{
	if ( hbgv.base_quit ) return FALSE;

	gboolean dur_b = FALSE;
	gint64 duration = 0, current = 0;
	
	if ( gst_element_query_position ( hgpl.playbin_pl, GST_FORMAT_TIME, &current ) )
	{
		if ( gst_element_query_duration ( hgpl.playbin_pl, GST_FORMAT_TIME, &duration ) ) dur_b = TRUE;
		
			if ( dur_b && duration > 0 )
			{
				if ( current / GST_SECOND < duration / GST_SECOND )
				{
					helia_media_pl_pos_dur_sliders ( current, duration, 8 );
					helia_panel_pl_pos_dur_slider  ( current, duration, 8 );
					
					helia_media_pl_set_sensitive ( TRUE );
					helia_panel_pl_set_sensitive ( TRUE );
				}
				else
					helia_media_pl_next ();
			}
			else
			{
				helia_media_pl_pos_dur_text ( current, 0, 8 );
				helia_panel_pl_pos_dur_text ( current, 0, 8 );
				
				helia_media_pl_set_sensitive ( FALSE );
				helia_panel_pl_set_sensitive ( FALSE );
			}
	}	

	return TRUE;
}


static void helia_gst_pl_update_pos ()
{
	gboolean dur_b = FALSE;
	gint64 duration = 0, current = 0;

	if ( gst_element_query_position ( hgpl.playbin_pl, GST_FORMAT_TIME, &current ) )
	{
		if ( gst_element_query_duration ( hgpl.playbin_pl, GST_FORMAT_TIME, &duration ) ) dur_b = TRUE;
				
		if ( dur_b && duration > 0 )
		{
			helia_media_pl_pos_dur_sliders ( current, duration, 7 );
			helia_panel_pl_pos_dur_slider  ( current, duration, 7 );
		}
		else
		{
			helia_media_pl_pos_dur_text ( current, 0, 7 );
			helia_panel_pl_pos_dur_text ( current, 0, 7 );
		}
	}
}
static void helia_gst_pl_new_pos ( gint64 am )
{
    gst_element_send_event ( hgpl.playbin_pl, gst_event_new_step ( GST_FORMAT_BUFFERS, am, 1.0, TRUE, FALSE ) );
    
    helia_gst_pl_update_pos ();
}
void helia_gst_pl_frame ()
{
    gint n_video = 0;
    g_object_get ( hgpl.playbin_pl, "n-video", &n_video, NULL );
    
    if ( n_video == 0 ) return;

    if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state == GST_STATE_PLAYING )
	{ 
		helia_gst_pl_playback_pause (); 
		helia_gst_pl_new_pos ( 1 );
	}
    else if ( GST_ELEMENT_CAST( hgpl.playbin_pl )->current_state == GST_STATE_PAUSED )
        helia_gst_pl_new_pos ( 1 );
}


static gboolean helia_gst_pl_update_win ()
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state == GST_STATE_NULL ) return FALSE;

	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state == GST_STATE_PLAYING )
	{
		gint n_video = 0;
		g_object_get ( hgpl.playbin_pl, "n-video", &n_video, NULL );

		if ( n_video > 0 ) return FALSE;

		helia_base_update_win ();
	}
	else
	{
		g_debug ( "Not GST_STATE_PLAYING" );
		
		return TRUE;
	}

	return FALSE;
}

gboolean helia_gst_pl_playback_play ()
{
	g_debug ( "helia_gst_pl_playback_play" );

	if ( !hgpl.firstfile ) return FALSE;
	
	gboolean stop_state = helia_gst_pl_get_current_state_stop ();

	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state != GST_STATE_PLAYING )
	{
		gst_element_set_state ( hgpl.playbin_pl, GST_STATE_PLAYING );

		hgpl.s_time_d = g_timeout_add ( 100, (GSourceFunc)helia_gst_pl_refresh, NULL );

		helia_media_pl_set_sensitive ( TRUE );
		helia_panel_pl_set_sensitive ( TRUE );

		if ( stop_state )
			g_timeout_add ( 250, (GSourceFunc)helia_gst_pl_update_win, NULL );

		return TRUE;
	}
	else if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state == GST_STATE_PLAYING )
		helia_gst_pl_playback_pause ();

	return FALSE;
}

gboolean helia_gst_pl_playback_pause ()
{
	g_debug ( "helia_gst_pl_playback_pause" );

	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state == GST_STATE_PLAYING )
	{
		gst_element_set_state ( hgpl.playbin_pl, GST_STATE_PAUSED );
		g_source_remove ( hgpl.s_time_d );

		return TRUE;
	}

	return FALSE;
}

gboolean helia_gst_pl_playback_stop ()
{
	g_debug ( "helia_gst_pl_playback_stop" );

	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state != GST_STATE_NULL )
	{
		if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state != GST_STATE_PAUSED )
			g_source_remove ( hgpl.s_time_d );

		gst_element_set_state ( hgpl.playbin_pl, GST_STATE_NULL );
		
		hgpl.state_subtitle = TRUE;
		g_object_set ( hgpl.playbin_pl, "flags", 1559, NULL );

		helia_media_pl_set_spinner ( "", TRUE );
		
		helia_media_pl_pos_dur_sliders ( 0, 100, 8 );
		helia_panel_pl_pos_dur_slider  ( 0, 100, 8 );

		helia_media_pl_set_sensitive ( FALSE );
		helia_panel_pl_set_sensitive ( FALSE );

		helia_base_update_win ();

		return TRUE;
	}

	return FALSE;
}

void helia_gst_pl_stop_set_play ( gchar *name_file, guintptr win_handle )
{
	g_debug ( "helia_gst_pl_stop_set_play" );

	helia_gst_pl_playback_stop ();

	hgpl.win_handle_pl = win_handle;

	if ( g_strrstr ( name_file, "://" ) )
		g_object_set ( hgpl.playbin_pl, "uri", name_file, NULL );
	else
	{
		gchar *uri = gst_filename_to_uri ( name_file, NULL );
			g_object_set ( hgpl.playbin_pl, "uri", uri, NULL );
		g_free ( uri );
	}

	hgpl.firstfile = TRUE;

	helia_gst_pl_playback_play ();
}

gboolean helia_gst_pl_playback_record ()
{
	g_print ( "helia_gst_pl_playback_record:: Not realize" );

	return TRUE;
}


gboolean helia_gst_pl_get_current_state_playing ()
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state == GST_STATE_PLAYING ) return TRUE;

	return FALSE;
}
gboolean helia_gst_pl_get_current_state_paused ()
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state == GST_STATE_PAUSED ) return TRUE;

	return FALSE;
}
gboolean helia_gst_pl_get_current_state_stop ()
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state == GST_STATE_NULL ) return TRUE;

	return FALSE;
}

gboolean helia_gst_pl_draw_logo ()
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state == GST_STATE_NULL )
		return TRUE;

	gint n_video = 0;
	g_object_get ( hgpl.playbin_pl, "n-video", &n_video, NULL );

	if ( n_video == 0 ) return TRUE;

	return FALSE;
}

gboolean helia_gst_pl_ega ()
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state != GST_STATE_PLAYING ) return FALSE;

	helia_eqa_win ( hgpl.playequal, helia_base_win_ret (), hbgv.opacity_eq );

	return TRUE;
}
gboolean helia_gst_pl_egv ()
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state != GST_STATE_PLAYING ) return FALSE;

	helia_eqv_win ( hgpl.videobln, helia_base_win_ret (), hbgv.opacity_eq );

	return TRUE;
}

gboolean helia_gst_pl_mute_get ()
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state != GST_STATE_PLAYING ) return TRUE;

	gboolean mute = FALSE;

	g_object_get ( hgpl.playbin_pl, "mute", &mute, NULL );

	return mute;
}
void helia_gst_pl_mute_set ()
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state != GST_STATE_PLAYING ) return;

	gboolean mute = FALSE;

	g_object_get ( hgpl.playbin_pl, "mute", &mute, NULL );
	g_object_set ( hgpl.playbin_pl, "mute", !mute, NULL );

	g_debug ( "helia_gst_pl_mute_set:: mute %s", mute ? "TRUE" : "FALSE" );
}
void helia_gst_pl_mute_set_widget ( GtkWidget *widget )
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state != GST_STATE_PLAYING ) return;

	gboolean mute = FALSE;

	g_object_get ( hgpl.playbin_pl, "mute", &mute, NULL );
	g_object_set ( hgpl.playbin_pl, "mute", !mute, NULL );

	if ( widget ) gtk_widget_set_sensitive ( GTK_WIDGET ( widget ), mute );

	g_debug ( "helia_gst_pl_mute_set_widget:: mute %s", mute ? "TRUE" : "FALSE" );
}


void helia_gst_pl_volume_changed ( GtkScaleButton *button, gdouble value )
{
	if ( GST_ELEMENT_CAST ( hgpl.playbin_pl )->current_state != GST_STATE_PLAYING ) return;

	g_object_set ( hgpl.playbin_pl, "volume", value, NULL );

	hgpl.volume_pl = value;

	if ( hbgv.all_info )
		g_debug ( "helia_gst_pl_volume_changed_pl:: value %f | widget name %s", value, gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

void helia_gst_pl_playbin_change_state_subtitle ()
{
	hgpl.state_subtitle = !hgpl.state_subtitle;
	g_object_set ( hgpl.playbin_pl, "flags", ( hgpl.state_subtitle ) ? 1559 : 1555, NULL );
}


void helia_gst_pl_set_text_audio ( GtkButton *button, gboolean text, gboolean create_text, gboolean audio, gboolean create_audio )
{
	gint cur_text, num_text;
	g_object_get ( hgpl.playbin_pl, "current-text", &cur_text, NULL );
	g_object_get ( hgpl.playbin_pl, "n-text",       &num_text, NULL );

	gint cur_audio, num_audio;
	g_object_get ( hgpl.playbin_pl, "current-audio", &cur_audio, NULL );
	g_object_get ( hgpl.playbin_pl, "n-audio",       &num_audio, NULL );
	
	gint cur = 0, num = 0;
	
	if ( text )
	{
		if ( create_text )
		{
			if ( num_text == 0 || GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state == GST_STATE_NULL ) { cur = -1; }
			if ( num_text == 1 ) { cur = 0; }
			if ( num_text >  1 ) { cur = cur_text; }
		}
		else
		{
			if ( num_text == 0 || GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state == GST_STATE_NULL ) { cur = -1; }
			if ( num_text == 1 ) { cur = 0; }
			
			if ( num_text >  1 )
			{
				if ( cur_text + 1 == num_text ) { cur = 0; } else { cur = cur_text + 1; }				
				g_object_set ( hgpl.playbin_pl, "current-text", cur, NULL );
			}
		}

		num = num_text;
	}
	
	if ( audio )
	{
		if ( create_audio )
		{
			if ( num_audio == 0 || GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state == GST_STATE_NULL ) { cur = -1; }
			if ( num_audio == 1 ) { cur = 0; }
			if ( num_audio >  1 ) { cur = cur_audio; }
		}
		else
		{
			if ( num_audio == 0 || GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state == GST_STATE_NULL ) { cur = -1; }
			if ( num_audio == 1 ) { cur = 0; }
			
			if ( num_audio >  1 )
			{
				if ( cur_audio + 1 == num_audio ) { cur = 0; } else { cur = cur_audio + 1; }
				g_object_set ( hgpl.playbin_pl, "current-audio", cur, NULL );
			}
		}
		
		num = num_audio;
	}
	
	gchar *text_set = g_strdup_printf ( "%d / %d", cur + 1, num );
		gtk_button_set_label ( button, text_set );
	g_free  ( text_set );	
}


HGPlState helia_gst_pl_state_all ()
{
	HGPlState hgpls;

	gint cur_text, num_text;
	g_object_get ( hgpl.playbin_pl, "current-text", &cur_text, NULL );
	g_object_get ( hgpl.playbin_pl, "n-text",       &num_text, NULL );

	gint cur_audio, num_audio;
	g_object_get ( hgpl.playbin_pl, "current-audio", &cur_audio, NULL );
	g_object_get ( hgpl.playbin_pl, "n-audio",       &num_audio, NULL );
	
	hgpls.num_text = num_text;
	hgpls.cur_text = cur_text;

	hgpls.num_audio = num_audio;
	hgpls.cur_audio = cur_audio;
	
	hgpls.volume_pl  = hgpl.volume_pl;
	hgpls.state_mute = helia_gst_pl_mute_get ();
	
	hgpls.state_subtitle = hgpl.state_subtitle;
	
	hgpls.state_playing = ( GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state == GST_STATE_PLAYING ) ? TRUE : FALSE;
	hgpls.state_pause   = ( GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state == GST_STATE_PAUSED  ) ? TRUE : FALSE;
	hgpls.state_stop    = ( GST_ELEMENT_CAST ( hgpl.playbin_pl  )->current_state == GST_STATE_NULL    ) ? TRUE : FALSE;
	
	return hgpls;
}



// Window Info

void helia_gst_pl_info ()
{
	g_print ( "helia_gst_pl_info \n" );
}
