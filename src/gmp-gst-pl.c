/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/


#include "gmp-gst-pl.h"
#include "gmp-media.h"
#include "gmp-dvb.h"
#include "gmp-pref.h"

#include "gmp-eqa.h"
#include "gmp-eqv.h"

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


struct GmpGstPl
{
	guintptr win_handle_pl;
	gboolean show_slider_menu, firstfile, all_info, state_subtitle;

	GstElement *playbin_pl, *videobln, *playequal;
	
	guint s_time_d;
	gdouble volume_pl;
	
	gboolean base_quit;
};

struct GmpGstPl gmpgstpl;


void gmp_player_base_quit ( gboolean base_quit )
{
	gmpgstpl.base_quit = base_quit;
}

static GstBusSyncReply gmp_player_bus_sync_handler ( GstBus *bus, GstMessage *message )
{
	if ( gmpgstpl.all_info )
		g_debug ( "gmp_player_bus_sync_handler:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );

    if ( !gst_is_video_overlay_prepare_window_handle_message ( message ) )
        return GST_BUS_PASS;

	guintptr handle = gmpgstpl.win_handle_pl;

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

static void gmp_msg_err_pl ( GstBus *bus, GstMessage *msg )
{
    GError *err = NULL;
    gchar *dbg  = NULL;
		
    gst_message_parse_error ( msg, &err, &dbg );
    
    g_critical ( "gmp_msg_err_pl:: %s (%s)\n", err->message, (dbg) ? dbg : "no details" );
    
    gmp_message_dialog ( "ERROR:", err->message, GTK_MESSAGE_ERROR );

    g_error_free ( err );
    g_free ( dbg );
    
	gmp_player_playback_stop ();

	g_debug ( "gmp_msg_err_pl:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );
}


static void gmp_msg_cll_pl ()
{
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state == GST_STATE_PLAYING )
    {
        gst_element_set_state ( gmpgstpl.playbin_pl, GST_STATE_PAUSED  );
        gst_element_set_state ( gmpgstpl.playbin_pl, GST_STATE_PLAYING );
    }
}

static void gmp_msg_eos_pl ()
{
	gmp_media_next_pl ();
}

static void gmp_msg_buf_pl ( GstBus *bus, GstMessage *msg )
{
	if ( gmpgstpl.base_quit ) return;
	
    gint percent;
    gst_message_parse_buffering ( msg, &percent );
    
    if ( percent == 100 )
    {
        if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state == GST_STATE_PAUSED )
            gst_element_set_state ( gmpgstpl.playbin_pl, GST_STATE_PLAYING );
            
		gmp_spinner_label_set ( "", TRUE );
    }
    else
    {
        if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state == GST_STATE_PLAYING )
            gst_element_set_state ( gmpgstpl.playbin_pl, GST_STATE_PAUSED );

        gchar *text = g_strdup_printf ( " %u%s ", percent, "%" );
            gmp_spinner_label_set ( text, FALSE );
        g_free ( text );
	
		g_debug ( "buffering: %d %s", percent, "%" );
	}

	if ( gmpgstpl.all_info )
		g_debug ( "gmp_msg_buf_pl:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );
}
/*
static void gmp_got_location ( GstObject *gstobject, GstObject *prop_object )
{
  gchar *location;
	g_object_get ( G_OBJECT (prop_object), "temp-location", &location, NULL );
	g_print ( "Temporary file: %s\n", location );
  g_free ( location );
  
  g_object_set ( G_OBJECT ( prop_object ), "temp-remove", FALSE, NULL );
}
*/
gboolean gmp_player_gst_create_pl ()
{
	gmpgstpl.volume_pl = 0.5;
	gmpgstpl.state_subtitle = TRUE;
	gmpgstpl.base_quit = FALSE;
	
    gmpgstpl.playbin_pl  = gst_element_factory_make ( "playbin", "playbin" );

    GstElement *bin_audio, *bin_video, *vsink, *asink;

    vsink  = gst_element_factory_make ( "autovideosink", NULL );
    gmpgstpl.videobln  = gst_element_factory_make ( "videobalance",      NULL );
    

    asink  = gst_element_factory_make ( "autoaudiosink", NULL );
    gmpgstpl.playequal = gst_element_factory_make ( "equalizer-nbands",  NULL );


    if ( !gmpgstpl.playbin_pl || !vsink || !gmpgstpl.videobln || !asink || !gmpgstpl.playequal )
	{
        g_printerr ( "gmp_player_gst_create_pl - not all elements could be created.\n" );
		return FALSE;	
	}

    bin_audio = gst_bin_new ( "audio_sink_bin" );
    gst_bin_add_many (GST_BIN ( bin_audio), gmpgstpl.playequal, asink, NULL );
    gst_element_link_many ( gmpgstpl.playequal, asink, NULL );

    GstPad *pad = gst_element_get_static_pad ( gmpgstpl.playequal, "sink" );
    gst_element_add_pad ( bin_audio, gst_ghost_pad_new ( "sink", pad ) );
    gst_object_unref ( pad );


    bin_video = gst_bin_new ( "video_sink_bin" );
    gst_bin_add_many ( GST_BIN (bin_video), gmpgstpl.videobln, vsink, NULL );
    gst_element_link_many ( gmpgstpl.videobln, vsink, NULL );

    GstPad *padv = gst_element_get_static_pad ( gmpgstpl.videobln, "sink" );
    gst_element_add_pad ( bin_video, gst_ghost_pad_new ( "sink", padv ) );
    gst_object_unref ( padv );


    g_object_set ( gmpgstpl.playbin_pl, "video-sink", bin_video, NULL );
    g_object_set ( gmpgstpl.playbin_pl, "audio-sink", bin_audio, NULL );

    g_object_set ( gmpgstpl.playbin_pl, "volume", gmpgstpl.volume_pl, NULL );

/*
	guint flags;

	g_object_get ( gmpgstpl.playbin_pl, "flags", &flags, NULL );
		flags |= GST_PLAY_FLAG_DOWNLOAD;
	g_object_set ( gmpgstpl.playbin_pl, "flags", flags,  NULL );
	
	g_signal_connect ( gmpgstpl.playbin_pl, "deep-notify::temp-location", G_CALLBACK ( gmp_got_location ), NULL );
*/

    GstBus *bus = gst_element_get_bus ( gmpgstpl.playbin_pl );
    gst_bus_add_signal_watch_full ( bus, G_PRIORITY_DEFAULT );
    gst_bus_set_sync_handler ( bus, (GstBusSyncHandler)gmp_player_bus_sync_handler, NULL, NULL );
    gst_object_unref ( bus );

	g_signal_connect ( bus, "message::eos",           G_CALLBACK ( gmp_msg_eos_pl ),  NULL );
    g_signal_connect ( bus, "message::error",         G_CALLBACK ( gmp_msg_err_pl ),  NULL );
    g_signal_connect ( bus, "message::clock-lost",    G_CALLBACK ( gmp_msg_cll_pl ),  NULL );
    g_signal_connect ( bus, "message::buffering",     G_CALLBACK ( gmp_msg_buf_pl ),  NULL );

    return TRUE;
}

void gmp_player_slider_changed ( gdouble value )
{
    gst_element_seek_simple ( gmpgstpl.playbin_pl, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, (gint64)( value * GST_SECOND ) );
}
gint64 gmp_player_query_duration ()
{
	GstFormat fmt = GST_FORMAT_TIME;
    gint64 duration = 0;

	if ( gst_element_query_duration ( gmpgstpl.playbin_pl, fmt, &duration ) )
		return duration;
	
	return 0;
}
gint64 gmp_player_query_position ()
{
	GstFormat fmt = GST_FORMAT_TIME;
    gint64 position = 0;

	if ( gst_element_query_position ( gmpgstpl.playbin_pl, fmt, &position ) )
		return position;
	
	return 0;
}

static gboolean gmp_player_refresh ()
{
	if ( gmpgstpl.base_quit ) return FALSE;
	
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 duration, current = -1;

    if ( gst_element_query_duration ( gmpgstpl.playbin_pl, fmt, &duration ) 
		 && gst_element_query_position ( gmpgstpl.playbin_pl, fmt, &current ) )
    {
        if ( duration == -1 || duration == 0 )
        {
            gmp_pos_dur_text ( current, duration );
            gmp_media_set_sensitive_pl ( FALSE, !gmp_player_mute_get_pl () );
        }
        else
        {
            if ( current / GST_SECOND < duration / GST_SECOND )
            {
				gmp_pos_dur_sliders ( current, duration );
                gmp_pos_dur_text ( current, duration );
				gmp_media_set_sensitive_pl ( TRUE, !gmp_player_mute_get_pl () );
            }
            else
				gmp_media_next_pl ();
        }
    }
    else
    {
		if ( gst_element_query_position ( gmpgstpl.playbin_pl, fmt, &current ) )
			gmp_pos_text ( current );
		else
			gmp_pos_dur_text ( 0, 0 );
			
		gmp_media_set_sensitive_pl ( FALSE, !gmp_player_mute_get_pl () );
	}
	
    return TRUE;
}

static gboolean gmp_player_update_win ()
{
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state != GST_STATE_PLAYING )
	{
		g_debug ( "No Player GST_STATE_PLAYING" );
		return TRUE;
	}
	else
	{		
		gint n_video = 0;
		g_object_get ( gmpgstpl.playbin_pl, "n-video", &n_video, NULL );
			
		if ( n_video > 0 ) return FALSE;
			
		gmp_base_update_win ();
	}
	
	return FALSE;
}

gboolean gmp_player_playback_start ()
{
	g_debug ( "gmp_player_playback_start" );

	if ( !gmpgstpl.firstfile ) return FALSE;

	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state != GST_STATE_PLAYING )
	{		
		gst_element_set_state ( gmpgstpl.playbin_pl, GST_STATE_PLAYING );
	
		gmpgstpl.s_time_d = g_timeout_add ( 100, (GSourceFunc)gmp_player_refresh, NULL );

		gmp_media_set_sensitive_pl ( TRUE, !gmp_player_mute_get_pl () );
		
		g_timeout_add ( 250, (GSourceFunc)gmp_player_update_win, NULL );
		
		return TRUE;
	}
	
	return FALSE;
}

gboolean gmp_player_playback_pause ()
{
	g_debug ( "gmp_player_playback_pause" );
	
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state == GST_STATE_PLAYING )
	{
		gst_element_set_state ( gmpgstpl.playbin_pl, GST_STATE_PAUSED );
		g_source_remove ( gmpgstpl.s_time_d );
		
		return TRUE;
	}
	
	return FALSE;
}

gboolean gmp_player_playback_stop ()
{
	g_debug ( "gmp_player_playback_stop" );
	
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state != GST_STATE_NULL )
	{
		if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state != GST_STATE_PAUSED )
			g_source_remove ( gmpgstpl.s_time_d );

		gst_element_set_state ( gmpgstpl.playbin_pl, GST_STATE_NULL );

        gmp_pos_dur_sliders ( 0, 100 );
        gmp_pos_dur_text  ( 0, 0 );
        
        gmp_spinner_label_set ( "", TRUE );
        
        gmp_media_set_sensitive_pl ( FALSE, FALSE );

		gmp_base_update_win ();
		
		return TRUE;
	}
	
	return FALSE;
}

void gmp_player_stop_set_play ( gchar *name_file, guintptr win_handle )
{
	g_debug ( "gmp_player_stop_set_play" );

    gmp_player_playback_stop ();
    
    gmpgstpl.win_handle_pl = win_handle;

    if ( g_strrstr ( name_file, "://" ) )
        g_object_set ( gmpgstpl.playbin_pl, "uri", name_file, NULL );
    else
    {
        gchar *uri = gst_filename_to_uri ( name_file, NULL );
            g_object_set ( gmpgstpl.playbin_pl, "uri", uri, NULL );
        g_free ( uri );
    }

	gmpgstpl.firstfile = TRUE;

    gmp_player_playback_start ();
}

void gmp_player_set_suburi ( gchar *name_file )
{
	g_debug ( "gmp_player_set_suburi" );

	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state == GST_STATE_PLAYING )
	{
		g_object_set ( gmpgstpl.playbin_pl, "suburi", name_file, NULL );
		g_object_set ( gmpgstpl.playbin_pl, "subtitle-font-desc", "Sans, 18", NULL );
	}
}

gboolean gmp_player_playback_record ()
{
	g_print ( "gmp_player_playback_record:: Not realize" );
	
	return TRUE;
}


gboolean gmp_player_get_current_state_playing ()
{
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl  )->current_state == GST_STATE_PLAYING ) return TRUE;
	
	return FALSE;
}
gboolean gmp_player_get_current_state_paused ()
{
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl  )->current_state == GST_STATE_PAUSED ) return TRUE;
	
	return FALSE;
}
gboolean gmp_player_get_current_state_stop ()
{
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl  )->current_state == GST_STATE_NULL ) return TRUE;
	
	return FALSE;
}

gboolean gmp_player_ega_pl ()
{	
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl  )->current_state != GST_STATE_PLAYING ) return FALSE;

	gmp_eqa_win ( gmpgstpl.playequal );
	
	return TRUE;
}
gboolean gmp_player_egv_pl ()
{
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl  )->current_state != GST_STATE_PLAYING ) return FALSE;
	
	gmp_eqv_win ( gmpgstpl.videobln );
	
	return TRUE;
}

gboolean gmp_player_mute_get_pl ()
{
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state != GST_STATE_PLAYING ) return FALSE;
	
	gboolean mute = FALSE;

    g_object_get ( gmpgstpl.playbin_pl, "mute", &mute, NULL );

	return mute;
}
void gmp_player_mute_set_pl ( GtkWidget *widget, GtkWidget *widget_v )
{
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state != GST_STATE_PLAYING ) return;
	
    gboolean mute = FALSE;

    g_object_get ( gmpgstpl.playbin_pl, "mute", &mute, NULL );
    g_object_set ( gmpgstpl.playbin_pl, "mute", !mute, NULL );

	gtk_widget_set_sensitive ( GTK_WIDGET ( widget ), mute );
	gtk_widget_set_sensitive ( GTK_WIDGET ( widget_v ), mute );

	g_debug ( "gmp_player_mute_set_pl:: mute %s", mute ? "TRUE" : "FALSE" );
}
void gmp_player_volume_changed_pl ( gdouble value )
{
    if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state != GST_STATE_PLAYING ) return;
    
    g_object_set ( gmpgstpl.playbin_pl, "volume", value, NULL );

	gmpgstpl.volume_pl = value;
	
	//g_debug ( "gmp_player_volume_changed_pl:: value %f", value );
}
gdouble gmp_player_get_volume_pl () { return gmpgstpl.volume_pl; }


gboolean gmp_player_scroll ( GdkEventScroll *evscroll )
{
	//g_debug ( "gmp_player_scroll" );

    if ( GST_ELEMENT_CAST(gmpgstpl.playbin_pl)->current_state != GST_STATE_NULL )
    {
        gint64 skip = GST_SECOND * 20, end = GST_SECOND * 1, pos = 0, dur = 0, new_pos = 0;

        if ( !gst_element_query_position ( gmpgstpl.playbin_pl, GST_FORMAT_TIME, &pos ) ) return TRUE;

        gst_element_query_duration ( gmpgstpl.playbin_pl, GST_FORMAT_TIME, &dur );
		
		if ( dur == -1 || dur == 0 ) return TRUE;

        if ( evscroll->direction == GDK_SCROLL_DOWN ) new_pos = (pos > skip) ? (pos - skip) : 0;

        if ( evscroll->direction == GDK_SCROLL_UP   ) new_pos = (dur > (pos + skip)) ? (pos + skip) : dur-end;

        if ( evscroll->direction == GDK_SCROLL_DOWN || evscroll->direction == GDK_SCROLL_UP )
        {
            if ( gst_element_seek_simple ( gmpgstpl.playbin_pl, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, new_pos ) )
            {
				gmp_pos_dur_sliders ( pos, dur );
                gmp_pos_dur_text ( pos, dur );
				
				g_usleep ( 1000 );
			}

            return TRUE;
        }
    }
    
    return FALSE;
}


static void gmp_player_clicked_audio_pl ( GtkButton *button )
{
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state != GST_STATE_PLAYING ) return;
	
	g_debug ( "gmp_player_clicked_audio_pl" );
	
	gint cur_audio, num_audio;
	g_object_get ( gmpgstpl.playbin_pl, "current-audio", &cur_audio, NULL );
	g_object_get ( gmpgstpl.playbin_pl, "n-audio",       &num_audio, NULL );	

	if ( cur_audio > 0 && cur_audio+1 == num_audio )
		g_object_set ( gmpgstpl.playbin_pl, "current-audio", 0, NULL );
	else
		g_object_set ( gmpgstpl.playbin_pl, "current-audio", cur_audio+1, NULL );
		
	g_object_get ( gmpgstpl.playbin_pl, "current-audio", &cur_audio,  NULL );

	gchar *text = g_strdup_printf ( "%d / %d", cur_audio+1, num_audio );

	gtk_button_set_label ( button, text );
	
	g_free  ( text );
}

static GtkButton * gmp_player_audio_button_pl ()
{
	gint cur_audio, num_audio;
	g_object_get ( gmpgstpl.playbin_pl, "current-audio", &cur_audio, NULL );
	g_object_get ( gmpgstpl.playbin_pl, "n-audio",       &num_audio, NULL );
	
	gchar *text = g_strdup_printf ( "%d / %d", cur_audio+1, num_audio );

	GtkButton *button = (GtkButton *)gtk_button_new_with_label ( text );
	g_signal_connect ( button, "clicked", G_CALLBACK ( gmp_player_clicked_audio_pl ), NULL );
	
	g_free  ( text );

	return button;
}

static void gmp_player_clicked_text_pl ( GtkButton *button )
{
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state != GST_STATE_PLAYING ) return;
	
	g_debug ( "gmp_player_clicked_text_pl" );
	
	gint cur_text, num_text;
	g_object_get ( gmpgstpl.playbin_pl, "current-text", &cur_text, NULL );
	g_object_get ( gmpgstpl.playbin_pl, "n-text",       &num_text, NULL );

	if ( cur_text > 0 && cur_text+1 == num_text )
		g_object_set ( gmpgstpl.playbin_pl, "current-text", 0, NULL );
	else
		g_object_set ( gmpgstpl.playbin_pl, "current-text", cur_text+1, NULL );
		
	g_object_get ( gmpgstpl.playbin_pl, "current-text", &cur_text,  NULL );

	gchar *text = g_strdup_printf ( "%d / %d", cur_text+1, num_text );

	gtk_button_set_label ( button, text );
	
	g_free  ( text );
}

static GtkButton * gmp_player_text_button_pl ()
{
	gint cur_text, num_text;
	g_object_get ( gmpgstpl.playbin_pl, "current-text", &cur_text, NULL );
	g_object_get ( gmpgstpl.playbin_pl, "n-text",       &num_text, NULL );		
	
	gchar *text = g_strdup_printf ( "%d / %d", cur_text+1, num_text );

	GtkButton *button = (GtkButton *)gtk_button_new_with_label ( text );
	g_signal_connect ( button, "clicked", G_CALLBACK ( gmp_player_clicked_text_pl ), NULL );
	
	g_free  ( text );

	return button;
}

static void gmp_player_button_sub_set_image ( GtkButton *button )
{	
	GdkPixbuf *logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
              ( gmpgstpl.state_subtitle ) ? "gmp-set" : "gmp-unset", 16, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

  	GtkImage *image = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );
  	
	gtk_button_set_image ( button, GTK_WIDGET ( image ) );	
}
static void gmp_player_on_off_subtitle ( GtkButton *button, GtkWidget *widget )
{
	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state != GST_STATE_PLAYING ) return;
	
	g_debug ( "gmp_player_on_off_subtitle" );
	
	gmpgstpl.state_subtitle = !gmpgstpl.state_subtitle;
	
	gmp_player_button_sub_set_image ( button );
	
    g_object_set ( gmpgstpl.playbin_pl, "flags", ( gmpgstpl.state_subtitle ) ? 1559 : 1555, NULL );
    
    gtk_widget_set_sensitive ( GTK_WIDGET ( widget ), gmpgstpl.state_subtitle );
}

void gmp_player_audio_text_buttons ( GtkBox *hbox )
{
	gint num_audio, num_subtitle;
	g_object_get ( gmpgstpl.playbin_pl, "n-audio", &num_audio, NULL );
	g_object_get ( gmpgstpl.playbin_pl, "n-text",  &num_subtitle, NULL );	
	
	GtkButton *button_num_sub;
	
		if ( num_subtitle > 0 )
		{
			button_num_sub = gmp_player_text_button_pl ();
			gtk_box_pack_start ( hbox, GTK_WIDGET ( button_num_sub ), TRUE, TRUE, 0 );
			
			gtk_widget_set_sensitive ( GTK_WIDGET ( button_num_sub ), gmpgstpl.state_subtitle );
		}
		
		if ( num_subtitle > 0 )
		{
			GtkButton *button = (GtkButton *)gtk_button_new ();
			gmp_player_button_sub_set_image ( button );
	
			g_signal_connect ( button, "clicked", G_CALLBACK ( gmp_player_on_off_subtitle ), button_num_sub );
			
			gtk_box_pack_start ( hbox, GTK_WIDGET ( button ), TRUE, TRUE, 0 );			
		}
		
		if ( num_audio > 1 )
			gtk_box_pack_start ( hbox, GTK_WIDGET ( gmp_player_audio_button_pl () ), TRUE, TRUE, 0 );		
	
}


gboolean gmp_player_draw_logo ()
{
    if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state == GST_STATE_NULL )
		return TRUE;

	gint n_video = 0;
    g_object_get ( gmpgstpl.playbin_pl, "n-video", &n_video, NULL );

    if ( n_video == 0 )
    {
    	if ( GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state == GST_STATE_PLAYING 
    	  || GST_ELEMENT_CAST ( gmpgstpl.playbin_pl )->current_state == GST_STATE_PAUSED  )
				return TRUE;
    }

    return FALSE;
}
