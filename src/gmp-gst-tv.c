/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/


#include "gmp-gst-tv.h"
#include "gmp-media.h"
#include "gmp-dvb.h"
#include "gmp-mpegts.h"
#include "gmp-scan.h"
#include "gmp-pref.h"
#include "gmp-eqa.h"
#include "gmp-eqv.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gst/video/videooverlay.h>
#include <stdlib.h>


struct GmpGstTv
{
	guintptr win_handle_tv;
	gboolean video_enable, rec_status, rec_ses, all_info;

	GstElement *dvbsrc_tv, *dvb_all_n[19], *dvb_rec_all_n[6];
	GstPad *pad_a_sink[MAX_AUDIO], *pad_a_src[MAX_AUDIO], *blockpad;
	guint count_audio_track, set_audio_track;

	gchar *ch_name;
	gdouble volume_tv;
};

struct GmpGstTv gmpgsttv;


static GstBusSyncReply gmp_dvb_bus_sync_handler ( GstBus *bus, GstMessage *message )
{
	if ( gmpgsttv.all_info )
		g_debug ( "gmp_dvb_bus_sync_handler:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );

    if ( !gst_is_video_overlay_prepare_window_handle_message ( message ) )
	{
        return GST_BUS_PASS;
	}

	guintptr handle = gmpgsttv.win_handle_tv;

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

static void gmp_msg_err_dvb ( GstBus *bus, GstMessage *msg )
{
    GError *err = NULL;
    gchar *dbg  = NULL;
		
    gst_message_parse_error ( msg, &err, &dbg );
    
    g_critical ( "gmp_msg_err_dvb:: %s (%s)\n", err->message, (dbg) ? dbg : "no details" );
    
    gmp_message_dialog ( "ERROR:", err->message, GTK_MESSAGE_ERROR );

    g_error_free ( err );
    g_free ( dbg );
    
	gmp_dvb_stop ();

	g_debug ( "gmp_msg_err_dvb:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );
}


static void gmp_msg_all_dvb ( GstBus *bus, GstMessage *msg )
{
    const GstStructure *structure = gst_message_get_structure ( msg );

    if ( structure )
    {
        gint signal, snr;
        gboolean hlook = FALSE;

        if (  gst_structure_get_int ( structure, "signal", &signal )  )
        {
            gst_structure_get_int ( structure, "snr", &snr );
            gst_structure_get_boolean ( structure, "lock", &hlook );

            gmp_set_sgn_snr ( gmpgsttv.dvbsrc_tv, (signal * 100) / 0xffff, (snr * 100) / 0xffff, hlook, gmpgsttv.rec_ses );
        }
    }
    
	if ( gmpgsttv.all_info )
		g_debug ( "gmp_msg_all_dvb:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );
}

static void gmp_msg_war_dvb ( GstBus *bus, GstMessage *msg )
{
    GError *war = NULL;
    gchar *dbg = NULL;

    gst_message_parse_warning ( msg, &war, &dbg );

	g_debug ( "gmp_msg_war_dvb:: %s (%s)", war->message, (dbg) ? dbg : "no details" );

    g_error_free ( war );
    g_free ( dbg );

	if ( gmpgsttv.all_info )
		g_debug ( "gmp_msg_war_dvb:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );
}

gboolean gmp_dvb_gst_create_tv ()
{
	gmp_mpegts_initialize ();
	
	gmpgsttv.win_handle_tv = 0;
	gmpgsttv.volume_tv = 0.5;
	gmpgsttv.rec_status = FALSE;
	gmpgsttv.rec_ses = FALSE;
	gmpgsttv.ch_name = NULL;
	
	
    gmpgsttv.dvbsrc_tv  = gst_pipeline_new ( "pipeline0" );

    if ( !gmpgsttv.dvbsrc_tv )
    {
        g_critical ( "gmp_dvb_gst_create_tv:: pipeline - not created. \n" );
        return FALSE;
    }

    GstBus *bus = gst_element_get_bus ( gmpgsttv.dvbsrc_tv );
    gst_bus_add_signal_watch_full ( bus, G_PRIORITY_DEFAULT );
    gst_bus_set_sync_handler ( bus, (GstBusSyncHandler)gmp_dvb_bus_sync_handler, NULL, NULL );
    gst_object_unref (bus);

    g_signal_connect ( bus, "message",          G_CALLBACK ( gmp_msg_all_dvb ), NULL );
    g_signal_connect ( bus, "message::error",   G_CALLBACK ( gmp_msg_err_dvb ), NULL );
    g_signal_connect ( bus, "message::warning", G_CALLBACK ( gmp_msg_war_dvb ), NULL );
    
    gmp_scan_gst_create ();

    return TRUE;
}


static gboolean gmp_dvb_set_audio_track ( GstPad *pad, GstElement *element, gint set_track_audio, const gchar *name, GstElement *element_n )
{
    gboolean audio_changed = TRUE;

    if ( pad )
    {
		if ( gmpgsttv.count_audio_track >= MAX_AUDIO )
		{
			g_debug ( "gmp_dvb_set_audio_track:: MAX_AUDIO %d", MAX_AUDIO );
			return FALSE;
		}
		
        gmpgsttv.pad_a_sink[gmpgsttv.count_audio_track] = gst_element_get_static_pad ( element, "sink" );
        gmpgsttv.pad_a_src [gmpgsttv.count_audio_track] = pad;

        if ( gst_pad_link ( pad, gmpgsttv.pad_a_sink[gmpgsttv.count_audio_track] ) == GST_PAD_LINK_OK )
            gst_object_unref ( gmpgsttv.pad_a_sink[gmpgsttv.count_audio_track] );
        else
        {
            gchar *object_name = gst_object_get_name ( GST_OBJECT ( element_n ) );
                g_debug ( "Linking demux name: %s & audio pad failed - %s", object_name, name );
            g_free ( object_name );
        }

        gmpgsttv.count_audio_track++;
    }
    else
    {
        if ( !gst_pad_unlink ( gmpgsttv.pad_a_src[gmpgsttv.set_audio_track], gmpgsttv.pad_a_sink[gmpgsttv.set_audio_track] ) )
            audio_changed = FALSE;

        if ( gst_pad_link ( gmpgsttv.pad_a_src[set_track_audio], gmpgsttv.pad_a_sink[set_track_audio] ) != GST_PAD_LINK_OK )
            audio_changed = FALSE;
    }

    return audio_changed;
}

static void gmp_dvb_changed_audio_track ( gint changed_track_audio )
{
    gmp_dvb_set_audio_track ( NULL, NULL, changed_track_audio, NULL, NULL );
    gmpgsttv.set_audio_track = changed_track_audio;
}


static void gmp_dvb_gst_pad_link ( GstPad *pad, GstElement *element, const gchar *name, GstElement *element_n )
{
    GstPad *pad_va_sink = gst_element_get_static_pad ( element, "sink" );

    if ( gst_pad_link ( pad, pad_va_sink ) == GST_PAD_LINK_OK )
    	gst_object_unref ( pad_va_sink );
    else
	{
		gchar *name_en = gst_object_get_name ( GST_OBJECT ( element_n ) );	
	        g_debug ( "gmp_dvb_gst_pad_link:: linking demux/decode name %s video/audio pad failed ( name_en %s )", name, name_en );
		g_free ( name_en );
	}
}

static void gmp_pad_demux_added_audio ( GstElement *element, GstPad *pad, GstElement *element_audio )
{	
    const gchar *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );
       
    if ( g_str_has_prefix ( name, "audio" ) )
    	//gmp_dvb_gst_pad_link ( pad, element_audio, name, element );
    	gmp_dvb_set_audio_track ( pad, element_audio, gmpgsttv.count_audio_track, name, element );
}
static void gmp_pad_demux_added_video ( GstElement *element, GstPad *pad, GstElement *element_video )
{
    const gchar *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );

    if ( g_str_has_prefix ( name, "video" ) )
        gmp_dvb_gst_pad_link ( pad, element_video, name, element );
}

static void gmp_pad_decode_added ( GstElement *element, GstPad *pad, GstElement *element_va )
{	
    const gchar *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );

    gmp_dvb_gst_pad_link ( pad, element_va, name, element );
}


static void gmp_gst_tsdemux ()
{

struct dvb_all_list { const gchar *name; } dvb_all_list_n[] =
{
    { "dvbsrc" }, { "tsdemux" },
    { "tee"    }, { "queue2"  }, { "decodebin" }, { "videoconvert" }, { "tee" }, { "queue2"  },                { "videobalance"     }, { "autovideosink" },
    { "tee"    }, { "queue2"  }, { "decodebin" }, { "audioconvert" }, { "tee" }, { "queue2"  },  { "volume" }, { "equalizer-nbands" }, { "autoaudiosink" }
};
	guint c = 0;
    for ( c = 0; c < G_N_ELEMENTS ( gmpgsttv.dvb_all_n ); c++ )
    {
        gmpgsttv.dvb_all_n[c] = gst_element_factory_make ( dvb_all_list_n[c].name, NULL );
        
        if ( !gmpgsttv.dvb_all_n[c] ) 
			g_critical ( "dvb_all_list:: element (factory make)  - %s not created. \n", dvb_all_list_n[c].name );
        
        if ( !gmpgsttv.video_enable && ( c == 2 || c == 3 || c == 4 || c == 5 || c == 6 || c == 7 || c == 8 || c == 9 ) ) continue;
        
        gst_bin_add ( GST_BIN ( gmpgsttv.dvbsrc_tv ), gmpgsttv.dvb_all_n[c] );
    	
    	if (  c == 1 || c == 3 || c == 4 || c == 6 || c == 7 || c == 8 || c == 9
           || c == 11 || c == 12 || c == 14 || c == 15 || c == 16 || c == 17 || c == 18 )
           gst_element_link ( gmpgsttv.dvb_all_n[c-1], gmpgsttv.dvb_all_n[c] );
    }

    g_signal_connect ( gmpgsttv.dvb_all_n[1], "pad-added", G_CALLBACK ( gmp_pad_demux_added_audio ), gmpgsttv.dvb_all_n[10] );
    g_signal_connect ( gmpgsttv.dvb_all_n[1], "pad-added", G_CALLBACK ( gmp_pad_demux_added_video ), gmpgsttv.dvb_all_n[2] );

    if ( gmpgsttv.video_enable )    
        g_signal_connect ( gmpgsttv.dvb_all_n[4], "pad-added", G_CALLBACK ( gmp_pad_decode_added ), gmpgsttv.dvb_all_n[5] );

    g_signal_connect ( gmpgsttv.dvb_all_n[12], "pad-added", G_CALLBACK ( gmp_pad_decode_added ), gmpgsttv.dvb_all_n[13] );
}

struct list_types { const gchar *type; const gchar *parser; };

struct list_types list_type_video_n[] =
{
	{ "mpeg",   "mpegvideoparse"  },
	{ "h264",   "h264parse"       },
	{ "h265",   "h265parse"       },
	{ "vc1",    "vc1parse"        }
};
struct list_types list_type_audio_n[] =
{
    { "mpeg", "mpegaudioparse" 	},
    { "ac3",  "ac3parse" 		}, 
    { "aac",  "aacparse" 		}
};

static const gchar * gmp_iterate_elements ( GstElement *it_element, struct list_types list_types_all[], guint num )
{
	GstIterator *it = gst_bin_iterate_recurse ( GST_BIN ( it_element ) );
	GValue item = { 0, };
	gboolean done = FALSE;
	const gchar *ret = NULL;
	guint c = 0;
  
	while ( !done )
	{
		switch ( gst_iterator_next ( it, &item ) ) 
		{
			case GST_ITERATOR_OK:
			{
				g_debug ( "GST_ITERATOR_OK" );
				GstElement *element = GST_ELEMENT ( g_value_get_object (&item) );
      
				gchar *object_name = gst_object_get_name ( GST_OBJECT ( element ) );
				
					if ( g_strrstr ( object_name, "parse" ) )
					{
						for ( c = 0; c < num; c++ )
							if ( g_strrstr ( object_name, list_types_all[c].type ) )
								ret = list_types_all[c].parser;
					}

					g_debug ( "Object name: %s", object_name );
					
				g_free ( object_name );
				g_value_reset (&item);
			}
				break;
			
			case GST_ITERATOR_RESYNC:
				g_debug ( "GST_ITERATOR_RESYNC" );
				gst_iterator_resync (it);
				break;
				
			case GST_ITERATOR_ERROR:
				g_debug ( "GST_ITERATOR_ERROR" );
				done = TRUE;
				break;
				
			case GST_ITERATOR_DONE:
				g_debug ( "GST_ITERATOR_DONE" );
				done = TRUE;
				break;
		}
	}
	
	g_value_unset ( &item );
	gst_iterator_free ( it );
	
	return ret;
}

static void gmp_gst_rec_ts ()
{
	const gchar *video_parser = gmp_iterate_elements ( gmpgsttv.dvb_all_n[4],  list_type_video_n, G_N_ELEMENTS ( list_type_video_n ) );
	const gchar *audio_parser = gmp_iterate_elements ( gmpgsttv.dvb_all_n[12], list_type_audio_n, G_N_ELEMENTS ( list_type_audio_n ) );
	const gchar *audio_encode = "avenc_mp2";
	
	if ( !video_parser && !audio_parser ) return;
	
	gboolean audio_mpeg = FALSE;
	if ( g_strrstr ( audio_parser, "mpeg" ) ) audio_mpeg = TRUE;
	
	g_debug ( "gmp_gst_rec_ts:: video parser: %s | audio parser / enc: %s", 
			  video_parser, audio_mpeg ? audio_parser : audio_encode );
	
	struct dvb_rec_all_list { const gchar *name; } dvb_all_rec_list_n[] =
	{
		{ "queue2" }, { video_parser },
		{ "queue2" }, { audio_mpeg ? audio_parser : audio_encode },
		{ "mpegtsmux" }, { "filesink" }
	};
	
	gst_element_set_state ( gmpgsttv.dvbsrc_tv, GST_STATE_PAUSED );

	guint c = 0;
    for ( c = 0; c < G_N_ELEMENTS ( gmpgsttv.dvb_rec_all_n ); c++ )
    {
		if ( !dvb_all_rec_list_n[c].name ) continue;
		
        gmpgsttv.dvb_rec_all_n[c] = gst_element_factory_make ( dvb_all_rec_list_n[c].name, NULL );

    	if ( !gmpgsttv.dvb_rec_all_n[c] ) 
			g_critical ( "dvb_all_list:: element (factory make)  - %s not created. \n", dvb_all_rec_list_n[c].name );
    	
    	if ( !gmpgsttv.video_enable && ( c == 0 || c == 1 ) ) continue;
    	
    	gst_bin_add ( GST_BIN ( gmpgsttv.dvbsrc_tv ), gmpgsttv.dvb_rec_all_n[c] );
    	
    	if ( c == 1 || c == 3 || c == 5 )
			gst_element_link ( gmpgsttv.dvb_rec_all_n[c-1], gmpgsttv.dvb_rec_all_n[c] );
    }
    
    gmpgsttv.blockpad = gst_element_get_static_pad ( gmpgsttv.dvb_rec_all_n[4], "src" );

    if ( gmpgsttv.video_enable )
    {
        gst_element_link ( gmpgsttv.dvb_all_n[2],     gmpgsttv.dvb_rec_all_n[0] );
        gst_element_link ( gmpgsttv.dvb_rec_all_n[1], gmpgsttv.dvb_rec_all_n[4] );
	}

    gst_element_link ( audio_mpeg ? gmpgsttv.dvb_all_n[10] : gmpgsttv.dvb_all_n[14], gmpgsttv.dvb_rec_all_n[2] );
    gst_element_link ( gmpgsttv.dvb_rec_all_n[3], gmpgsttv.dvb_rec_all_n[4] );

	gchar *date_str = gmp_pref_get_time_date_str ();
    gchar *file_rec = g_strdup_printf ( "%s/%s_%s.%s", gmp_rec_dir, gmpgsttv.ch_name, date_str, "m2ts" );
	
		g_object_set ( gmpgsttv.dvb_rec_all_n[5], "location", file_rec, NULL );
    
    g_free ( file_rec );
    g_free ( date_str );

    for ( c = 0; c < G_N_ELEMENTS ( gmpgsttv.dvb_rec_all_n ) - 1; c++ )
		if ( !gmpgsttv.video_enable && ( c == 0 || c == 1 ) ) continue;
			gst_element_set_state ( gmpgsttv.dvb_rec_all_n[c], GST_STATE_PAUSED );

	gst_element_set_state ( gmpgsttv.dvbsrc_tv, GST_STATE_PLAYING );
}

static void gmp_gst_tsdemux_remove ()
{
	g_debug ( "gmp_gst_tsdemux_remove" );
	
	GstIterator *it = gst_bin_iterate_elements ( GST_BIN ( gmpgsttv.dvbsrc_tv ) );
	GValue item = { 0, };
	gboolean done = FALSE;
  
	while ( !done )
	{
		switch ( gst_iterator_next ( it, &item ) ) 
		{
			case GST_ITERATOR_OK:
			{
				g_debug ( "GST_ITERATOR_OK" );
				GstElement *element = GST_ELEMENT ( g_value_get_object (&item) );
      
				gchar *object_name = gst_object_get_name ( GST_OBJECT ( element ) );
				
					g_debug ( "Object remove: %s", object_name );
					
					gst_bin_remove ( GST_BIN ( gmpgsttv.dvbsrc_tv ), element );
					
				g_free ( object_name );
				g_value_reset (&item);
			}
				break;
			
			case GST_ITERATOR_RESYNC:
				g_debug ( "GST_ITERATOR_RESYNC" );
				gst_iterator_resync (it);
				break;
				
			case GST_ITERATOR_ERROR:
				g_debug ( "GST_ITERATOR_ERROR" );
				done = TRUE;
				break;
				
			case GST_ITERATOR_DONE:
				g_debug ( "GST_ITERATOR_DONE" );
				done = TRUE;
				break;
		}
	}
	
	g_value_unset ( &item );
	gst_iterator_free ( it );	

	g_debug ( "End remove \n" );
}

static void gmp_checked_video ( gchar *data )
{
	if ( !g_strrstr ( data, "video-pid" ) || g_strrstr ( data, "video-pid=0" ) )
		 gmpgsttv.video_enable = FALSE;
	else
		 gmpgsttv.video_enable = TRUE;
}

static void gmp_set_tuning_timeout ( GstElement *element )
{
    guint64 timeout = 0;
    g_object_get ( element, "tuning-timeout", &timeout, NULL );
    g_object_set ( element, "tuning-timeout", (guint64)timeout / 5, NULL );
}

static void gmp_check_a_f ( GstElement *element )
{
	guint adapter = 0, frontend = 0;
	g_object_get ( element, "adapter",  &adapter,  NULL );
    g_object_get ( element, "frontend", &frontend, NULL );
    
	gmp_get_dvb_info ( FALSE, adapter, frontend );
}

static void gmp_data_split_set_dvb ( gchar *data )
{
	g_debug ( "gmp_data_split_set_dvb:: data %s", data );
	
    GstElement *element = gmpgsttv.dvb_all_n[0];
    gmp_set_tuning_timeout ( element );

    gchar **fields = g_strsplit ( data, ":", 0 );
    guint numfields = g_strv_length ( fields );

	if ( gmpgsttv.ch_name ) g_free ( gmpgsttv.ch_name );
    gmpgsttv.ch_name = g_strdup ( fields[0] );

	guint j = 0;
    for ( j = 1; j < numfields; j++ )
    {
        if ( g_strrstr ( fields[j], "delsys" ) || g_strrstr ( fields[j], "audio-pid" ) || g_strrstr ( fields[j], "video-pid" ) ) continue;

        if ( !g_strrstr ( fields[j], "=" ) ) continue;

        gchar **splits = g_strsplit ( fields[j], "=", 0 );

			g_debug ( "gmp_data_split_set_dvb:: gst-param %s | gst-value %s", splits[0], splits[1] );
        
			if ( g_strrstr ( splits[0], "polarity" ) )
            {
				if ( splits[1][0] == 'v' || splits[1][0] == 'V' || splits[1][0] == '0' )
					g_object_set ( element, "polarity", "V", NULL );
                else
					g_object_set ( element, "polarity", "H", NULL );
				
				continue;
			}
			
			long dat = atol ( splits[1] );

            if ( g_strrstr ( splits[0], "program-number" ) )
            {
                g_object_set ( gmpgsttv.dvb_all_n[1], "program-number", dat, NULL );
            }
            else if ( g_strrstr ( splits[0], "symbol-rate" ) )
				g_object_set ( element, "symbol-rate", ( dat > 100000) ? dat/1000 : dat, NULL );
            else if ( g_strrstr ( splits[0], "lnb-type" ) )
                gmp_set_lnb ( element, dat );
            else
                g_object_set ( element, splits[0], dat, NULL );

        g_strfreev (splits);
    }

    g_strfreev (fields);
    
    gmp_check_a_f ( element );
}

static GstPadProbeReturn gmp_blockpad_probe_event ( GstPad * pad, GstPadProbeInfo * info, gpointer user_data )
{
	if ( gmpgsttv.all_info )
		g_debug ( "gmp_blockpad_probe_event:: user_data %s", (gchar *)user_data );
	
	if ( GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS )
		return GST_PAD_PROBE_PASS;

	gst_pad_remove_probe ( pad, GST_PAD_PROBE_INFO_ID (info) );
  
	gst_element_set_state ( gmpgsttv.dvb_rec_all_n[5], GST_STATE_NULL );
 
		if ( gmpgsttv.rec_ses )
		{
			gchar *date_str = gmp_pref_get_time_date_str ();
			gchar *file_rec = g_strdup_printf ( "%s/%s_%s.%s", gmp_rec_dir, gmpgsttv.ch_name, date_str, "m2ts" );
			
			g_object_set ( gmpgsttv.dvb_rec_all_n[5], "location", file_rec, NULL );
							
			g_free ( file_rec );
			g_free ( date_str );
		}
		else
			g_object_set ( gmpgsttv.dvb_rec_all_n[5], "location", "/dev/null", NULL );
  
		gst_element_link ( gmpgsttv.dvb_rec_all_n[4], gmpgsttv.dvb_rec_all_n[5] );
  
	gst_element_set_state ( gmpgsttv.dvb_rec_all_n[5], GST_STATE_PLAYING );

	return GST_PAD_PROBE_DROP;
}

static GstPadProbeReturn gmp_blockpad_probe ( GstPad * pad, GstPadProbeInfo * info, gpointer user_data )
{
	GstPad *sinkpad;

	gst_pad_remove_probe ( pad, GST_PAD_PROBE_INFO_ID (info) );

	sinkpad = gst_element_get_static_pad ( gmpgsttv.dvb_rec_all_n[5], "sink" );
  
	gst_pad_add_probe ( sinkpad, GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, 
						gmp_blockpad_probe_event, user_data, NULL );

	gst_pad_send_event ( sinkpad, gst_event_new_eos () );

	gst_object_unref ( sinkpad );

  return GST_PAD_PROBE_OK;
}

void gmp_dvb_record ()
{
	g_debug ( "gmp_dvb_record" );
	
	if ( GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv )->current_state != GST_STATE_PLAYING ) return;

    if ( !gmpgsttv.rec_status )
    {
        gmp_gst_rec_ts ();
        gmpgsttv.rec_status = !gmpgsttv.rec_status;
        gmpgsttv.rec_ses = TRUE;
    }
    else
    {
		gmpgsttv.rec_ses = !gmpgsttv.rec_ses;
		
		gst_pad_add_probe ( gmpgsttv.blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
							gmp_blockpad_probe, "blockpad", NULL );
    }
}

void gmp_dvb_stop ()
{
	g_debug ( "gmp_dvb_stop" );
	
	if ( GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv )->current_state != GST_STATE_NULL )
	{
		gst_element_set_state ( gmpgsttv.dvbsrc_tv, GST_STATE_NULL );
		gmp_gst_tsdemux_remove ();
		gmpgsttv.rec_status = FALSE;
		gmpgsttv.rec_ses = FALSE;
		
		gmpgsttv.count_audio_track = 0;
		gmpgsttv.set_audio_track   = 0;		
		
		gmp_set_sgn_snr ( gmpgsttv.dvbsrc_tv, 0, 0, FALSE, FALSE );

		gmp_base_update_win ();
	}
}

static gboolean gmp_dvb_update_data_win ()
{
	if ( GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv )->current_state != GST_STATE_PLAYING )
	{
		g_debug ( "No TV GST_STATE_PLAYING" );
		return TRUE;
	}
	else
	{
		if ( gmpgsttv.video_enable ) return FALSE;
			
		gmp_base_update_win ();
	}
	
	return FALSE;
}

void gmp_dvb_play ( gchar *data, guintptr win_handle )
{
	g_debug ( "gmp_dvb_play: win_handle_tv %lu", (gulong)win_handle );
	
	gmp_dvb_stop ();
	gmpgsttv.win_handle_tv = win_handle;

	if ( GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv )->current_state != GST_STATE_PLAYING )
	{
        gmp_checked_video ( data );
        gmp_gst_tsdemux ();

		g_object_set ( gmpgsttv.dvb_all_n[16], "volume", gmpgsttv.volume_tv, NULL );
		gmp_data_split_set_dvb ( data );

        gst_element_set_state ( gmpgsttv.dvbsrc_tv, GST_STATE_PLAYING );
        
        g_timeout_add ( 250, (GSourceFunc)gmp_dvb_update_data_win, NULL );
	}
}

gboolean gmp_dvb_ega_tv ()
{	
	if ( GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv  )->current_state != GST_STATE_PLAYING ) return FALSE;

	gmp_eqa_win ( gmpgsttv.dvb_all_n[17] );
	
	return TRUE;
}
gboolean gmp_dvb_egv_tv ()
{
	if ( GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv  )->current_state != GST_STATE_PLAYING ) return FALSE;
	
	gmp_eqv_win ( gmpgsttv.dvb_all_n[8] );
	
	return TRUE;
}

gboolean gmp_dvb_mute_get_tv ()
{
	if ( GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv )->current_state != GST_STATE_PLAYING ) return FALSE;
	
	gboolean mute = FALSE;

    g_object_get ( gmpgsttv.dvb_all_n[16], "mute", &mute, NULL );

	return mute;
}
void gmp_dvb_mute_set_tv ( GtkWidget *widget )
{
	if ( GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv )->current_state != GST_STATE_PLAYING ) return;
	
    gboolean mute = FALSE;

    g_object_get ( gmpgsttv.dvb_all_n[16], "mute", &mute, NULL );
    g_object_set ( gmpgsttv.dvb_all_n[16], "mute", !mute, NULL );

	gtk_widget_set_sensitive ( GTK_WIDGET ( widget ), mute );

	g_debug ( "gmp_dvb_mute_set_tv:: mute %s", mute ? "TRUE" : "FALSE" );
}
void gmp_dvb_volume_changed_tv ( gdouble value )
{
    if ( GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv )->current_state == GST_STATE_PLAYING )
        g_object_set ( gmpgsttv.dvb_all_n[16], "volume", value, NULL );

	gmpgsttv.volume_tv = value;
	
	//g_debug ( "gmp_dvb_volume_changed_tv:: value %f", value );
}
gdouble gmp_dvb_get_volume_tv () { return gmpgsttv.volume_tv; }


static void gmp_dvb_clicked_audio_tv ( GtkButton *button )
{
	if ( gmpgsttv.count_audio_track < 2 ) return;
	
	if ( GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv )->current_state != GST_STATE_PLAYING ) return;
	
	g_debug ( "gmp_dvb_clicked_audio_tv" );
		
	if ( gmpgsttv.set_audio_track > 0 && gmpgsttv.count_audio_track-1 == gmpgsttv.set_audio_track )
		gmp_dvb_changed_audio_track ( 0 );
	else
		gmp_dvb_changed_audio_track ( gmpgsttv.set_audio_track + 1 );

	gchar *text = g_strdup_printf ( "%d / %d", gmpgsttv.set_audio_track+1, gmpgsttv.count_audio_track );

	gtk_button_set_label ( button, text );
	
	g_free  ( text );
}

void gmp_dvb_audio_button_tv ( GtkBox *hbox )
{
	if ( gmpgsttv.count_audio_track < 2 ) return;
	
	gchar *text = g_strdup_printf ( "%d / %d", gmpgsttv.set_audio_track+1, gmpgsttv.count_audio_track );
	
	GtkButton *button = (GtkButton *)gtk_button_new_with_label ( text );
	g_signal_connect ( button, "clicked", G_CALLBACK ( gmp_dvb_clicked_audio_tv ), NULL );
	
	g_free  ( text );
	
	gtk_box_pack_start ( hbox, GTK_WIDGET ( button ), TRUE, TRUE, 5 );
}

gboolean gmp_dvb_draw_logo ()
{
    if ( GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv )->current_state == GST_STATE_NULL )
		return TRUE;

	if (  GST_ELEMENT_CAST ( gmpgsttv.dvbsrc_tv )->current_state != GST_STATE_NULL  )
		if ( !gmpgsttv.video_enable )
			return TRUE;
    
    return FALSE;
}
