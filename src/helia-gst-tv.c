/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia-gst-tv.h"
#include "helia-media-tv.h"
#include "helia-base.h"
#include "helia-service.h"
#include "helia-mpegts.h"
#include "helia-scan.h"
#include "helia-pref.h"
#include "helia-eqa.h"
#include "helia-eqv.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gst/video/videooverlay.h>
#include <stdlib.h>


typedef struct _HeliaGstTv HeliaGstTv;

struct _HeliaGstTv
{
	guintptr win_handle_tv;
	gboolean video_enable, rec_status, rec_ses;

	GstElement *dvbsrc_tv, *dvb_all_n[19], *dvb_rec_all_n[6];
	GstPad *pad_a_sink[MAX_AUDIO], *pad_a_src[MAX_AUDIO], *blockpad;
	guint count_audio_track, set_audio_track;

	gchar *ch_name;
	gdouble volume_tv;
};

static HeliaGstTv hgtv;


static GstBusSyncReply helia_gst_tv_bus_sync_handler ( GstBus *bus, GstMessage *message )
{
	if ( hbgv.all_info )
		g_debug ( "helia_gst_tv_bus_sync_handler:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );

	if ( !gst_is_video_overlay_prepare_window_handle_message ( message ) )
	{
		return GST_BUS_PASS;
	}

	guintptr handle = hgtv.win_handle_tv;

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

static void helia_gst_tv_msg_err ( GstBus *bus, GstMessage *msg )
{
	GError *err = NULL;
	gchar  *dbg = NULL;

	gst_message_parse_error ( msg, &err, &dbg );

	g_critical ( "helia_gst_tv_msg_err:: %s (%s)\n", err->message, (dbg) ? dbg : "no details" );

	helia_service_message_dialog ( "ERROR:", err->message, GTK_MESSAGE_ERROR );

	g_error_free ( err );
	g_free ( dbg );

	helia_gst_tv_stop ();

	if ( hbgv.all_info )
		g_debug ( "helia_gst_tv_msg_err:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );
}

static void helia_gst_tv_msg_war ( GstBus *bus, GstMessage *msg )
{
	GError *war = NULL;
	gchar  *dbg = NULL;

	gst_message_parse_warning ( msg, &war, &dbg );

	g_debug ( "helia_gst_tv_msg_war:: %s (%s)", war->message, (dbg) ? dbg : "no details" );

	g_error_free ( war );
	g_free ( dbg );

	if ( hbgv.all_info )
		g_debug ( "helia_gst_tv_msg_war:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );
}

static void helia_gst_tv_msg_all ( GstBus *bus, GstMessage *msg )
{
	if ( hbgv.base_quit ) return;
	
	const GstStructure *structure = gst_message_get_structure ( msg );

	if ( structure )
	{
		gint signal, snr;
		gboolean hlook = FALSE;

		if (  gst_structure_get_int ( structure, "signal", &signal )  )
		{
			gst_structure_get_int ( structure, "snr", &snr );
			gst_structure_get_boolean ( structure, "lock", &hlook );

			helia_media_tv_set_sgn_snr ( hgtv.dvbsrc_tv, (signal * 100) / 0xffff, (snr * 100) / 0xffff, hlook, hgtv.rec_ses );
		}
	}

	if ( hbgv.all_info )
		g_debug ( "helia_gst_tv_msg_all:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );
}

gboolean helia_gst_tv_gst_create ()
{
	hgtv.win_handle_tv = 0;
	hgtv.volume_tv = 0.5;
	hgtv.rec_status = FALSE;
	hgtv.rec_ses = FALSE;
	hgtv.ch_name = NULL;


	hgtv.dvbsrc_tv  = gst_pipeline_new ( "pipeline0" );

	if ( !hgtv.dvbsrc_tv )
	{
		g_critical ( "helia_gst_tv_gst_create:: pipeline - not created. \n" );
		return FALSE;
	}

	GstBus *bus = gst_element_get_bus ( hgtv.dvbsrc_tv );
	gst_bus_add_signal_watch_full ( bus, G_PRIORITY_DEFAULT );
	gst_bus_set_sync_handler ( bus, (GstBusSyncHandler)helia_gst_tv_bus_sync_handler, NULL, NULL );
	gst_object_unref (bus);

	g_signal_connect ( bus, "message",          G_CALLBACK ( helia_gst_tv_msg_all ), NULL );
	g_signal_connect ( bus, "message::error",   G_CALLBACK ( helia_gst_tv_msg_err ), NULL );
	g_signal_connect ( bus, "message::warning", G_CALLBACK ( helia_gst_tv_msg_war ), NULL );

	helia_scan_gst_create ();

	return TRUE;
}


static gboolean helia_gst_tv_set_audio_track ( GstPad *pad, GstElement *element, gint set_track_audio, const gchar *name, GstElement *element_n )
{
	gboolean audio_changed = TRUE;

	if ( pad )
	{
		if ( hgtv.count_audio_track >= MAX_AUDIO )
		{
			g_debug ( "helia_gst_tv_set_audio_track:: MAX_AUDIO %d", MAX_AUDIO );
			return FALSE;
		}

		hgtv.pad_a_sink[hgtv.count_audio_track] = gst_element_get_static_pad ( element, "sink" );
		hgtv.pad_a_src [hgtv.count_audio_track] = pad;

		if ( gst_pad_link ( pad, hgtv.pad_a_sink[hgtv.count_audio_track] ) == GST_PAD_LINK_OK )
			gst_object_unref ( hgtv.pad_a_sink[hgtv.count_audio_track] );
		else
		{
			gchar *object_name = gst_object_get_name ( GST_OBJECT ( element_n ) );
				g_debug ( "Linking demux name: %s & audio pad failed - %s", object_name, name );
			g_free ( object_name );
		}

		hgtv.count_audio_track++;
	}
	else
	{
		if ( !gst_pad_unlink ( hgtv.pad_a_src[hgtv.set_audio_track], hgtv.pad_a_sink[hgtv.set_audio_track] ) )
			audio_changed = FALSE;

		if ( gst_pad_link ( hgtv.pad_a_src[set_track_audio], hgtv.pad_a_sink[set_track_audio] ) != GST_PAD_LINK_OK )
			audio_changed = FALSE;
	}

	return audio_changed;
}

static void helia_gst_tv_changed_audio_track ( gint changed_track_audio )
{
	helia_gst_tv_set_audio_track ( NULL, NULL, changed_track_audio, NULL, NULL );
	hgtv.set_audio_track = changed_track_audio;
}


static void helia_gst_tv_gst_pad_link ( GstPad *pad, GstElement *element, const gchar *name, GstElement *element_n )
{
	GstPad *pad_va_sink = gst_element_get_static_pad ( element, "sink" );

	if ( gst_pad_link ( pad, pad_va_sink ) == GST_PAD_LINK_OK )
		gst_object_unref ( pad_va_sink );
	else
	{
		gchar *name_en = gst_object_get_name ( GST_OBJECT ( element_n ) );
	        g_debug ( "helia_gst_tv_gst_pad_link:: linking demux/decode name %s video/audio pad failed ( name_en %s )", name, name_en );
		g_free ( name_en );
	}
}

static void helia_gst_tv_pad_demux_added_audio ( GstElement *element, GstPad *pad, GstElement *element_audio )
{
	const gchar *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );

	if ( g_str_has_prefix ( name, "audio" ) )
		//helia_gst_tv_gst_pad_link ( pad, element_audio, name, element );
		helia_gst_tv_set_audio_track ( pad, element_audio, hgtv.count_audio_track, name, element );
}
static void helia_gst_tv_pad_demux_added_video ( GstElement *element, GstPad *pad, GstElement *element_video )
{
	const gchar *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );

	if ( g_str_has_prefix ( name, "video" ) )
		helia_gst_tv_gst_pad_link ( pad, element_video, name, element );
}

static void helia_gst_tv_pad_decode_added ( GstElement *element, GstPad *pad, GstElement *element_va )
{
	const gchar *name = gst_structure_get_name ( gst_caps_get_structure ( gst_pad_query_caps ( pad, NULL ), 0 ) );

	helia_gst_tv_gst_pad_link ( pad, element_va, name, element );
}


static void helia_gst_tv_tsdemux ()
{

struct dvb_all_list { const gchar *name; } dvb_all_list_n[] =
{
	{ "dvbsrc" }, { "tsdemux" },
	{ "tee"    }, { "queue2"  }, { "decodebin" }, { "videoconvert" }, { "tee" }, { "queue2"  },                { "videobalance"     }, { "autovideosink" },
	{ "tee"    }, { "queue2"  }, { "decodebin" }, { "audioconvert" }, { "tee" }, { "queue2"  },  { "volume" }, { "equalizer-nbands" }, { "autoaudiosink" }
};
	guint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( hgtv.dvb_all_n ); c++ )
	{
		hgtv.dvb_all_n[c] = gst_element_factory_make ( dvb_all_list_n[c].name, NULL );

		if ( !hgtv.dvb_all_n[c] )
			g_critical ( "dvb_all_list:: element (factory make)  - %s not created. \n", dvb_all_list_n[c].name );

		if ( !hgtv.video_enable && ( c == 2 || c == 3 || c == 4 || c == 5 || c == 6 || c == 7 || c == 8 || c == 9 ) ) continue;

		gst_bin_add ( GST_BIN ( hgtv.dvbsrc_tv ), hgtv.dvb_all_n[c] );

		if (  c == 1 || c == 3 || c == 4 || c == 6 || c == 7 || c == 8 || c == 9
			|| c == 11 || c == 12 || c == 14 || c == 15 || c == 16 || c == 17 || c == 18 )
				gst_element_link ( hgtv.dvb_all_n[c-1], hgtv.dvb_all_n[c] );
	}

	g_signal_connect ( hgtv.dvb_all_n[1], "pad-added", G_CALLBACK ( helia_gst_tv_pad_demux_added_audio ), hgtv.dvb_all_n[10] );
	g_signal_connect ( hgtv.dvb_all_n[1], "pad-added", G_CALLBACK ( helia_gst_tv_pad_demux_added_video ), hgtv.dvb_all_n[2] );

	if ( hgtv.video_enable )
		g_signal_connect ( hgtv.dvb_all_n[4], "pad-added", G_CALLBACK ( helia_gst_tv_pad_decode_added ), hgtv.dvb_all_n[5] );

	g_signal_connect ( hgtv.dvb_all_n[12], "pad-added", G_CALLBACK ( helia_gst_tv_pad_decode_added ), hgtv.dvb_all_n[13] );
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

static const gchar * helia_gst_tv_iterate_elements ( GstElement *it_element, struct list_types list_types_all[], guint num )
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

static void helia_gst_tv_rec_ts ()
{
	const gchar *video_parser = helia_gst_tv_iterate_elements ( hgtv.dvb_all_n[4],  list_type_video_n, G_N_ELEMENTS ( list_type_video_n ) );
	const gchar *audio_parser = helia_gst_tv_iterate_elements ( hgtv.dvb_all_n[12], list_type_audio_n, G_N_ELEMENTS ( list_type_audio_n ) );
	const gchar *audio_encode = "avenc_mp2";

	if ( !video_parser && !audio_parser ) return;

	gboolean audio_mpeg = FALSE;
	if ( g_strrstr ( audio_parser, "mpeg" ) ) audio_mpeg = TRUE;

	g_debug ( "helia_gst_rec_ts:: video parser: %s | audio parser / enc: %s", 
			video_parser, audio_mpeg ? audio_parser : audio_encode );

struct dvb_rec_all_list { const gchar *name; } dvb_all_rec_list_n[] =
{
	{ "queue2" }, { video_parser },
	{ "queue2" }, { audio_mpeg ? audio_parser : audio_encode },
	{ "mpegtsmux" }, { "filesink" }
};

	gst_element_set_state ( hgtv.dvbsrc_tv, GST_STATE_PAUSED );

	guint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( hgtv.dvb_rec_all_n ); c++ )
	{
		if ( !dvb_all_rec_list_n[c].name ) continue;

		hgtv.dvb_rec_all_n[c] = gst_element_factory_make ( dvb_all_rec_list_n[c].name, NULL );

		if ( !hgtv.dvb_rec_all_n[c] )
			g_critical ( "dvb_all_list:: element (factory make)  - %s not created. \n", dvb_all_rec_list_n[c].name );

		if ( !hgtv.video_enable && ( c == 0 || c == 1 ) ) continue;

		gst_bin_add ( GST_BIN ( hgtv.dvbsrc_tv ), hgtv.dvb_rec_all_n[c] );

		if ( c == 1 || c == 3 || c == 5 )
			gst_element_link ( hgtv.dvb_rec_all_n[c-1], hgtv.dvb_rec_all_n[c] );
	}

	hgtv.blockpad = gst_element_get_static_pad ( hgtv.dvb_rec_all_n[4], "src" );

	if ( hgtv.video_enable )
	{
		gst_element_link ( hgtv.dvb_all_n[2],     hgtv.dvb_rec_all_n[0] );
		gst_element_link ( hgtv.dvb_rec_all_n[1], hgtv.dvb_rec_all_n[4] );
	}

	gst_element_link ( audio_mpeg ? hgtv.dvb_all_n[10] : hgtv.dvb_all_n[14], hgtv.dvb_rec_all_n[2] );
	gst_element_link ( hgtv.dvb_rec_all_n[3], hgtv.dvb_rec_all_n[4] );

	gchar *date_str = helia_service_get_time_date_str ();
	gchar *file_rec = g_strdup_printf ( "%s/%s_%s.%s", hbgv.rec_dir, hgtv.ch_name, date_str, "m2ts" );

	g_object_set ( hgtv.dvb_rec_all_n[5], "location", file_rec, NULL );

	g_free ( file_rec );
	g_free ( date_str );

	for ( c = 0; c < G_N_ELEMENTS ( hgtv.dvb_rec_all_n ) - 1; c++ )
		if ( !hgtv.video_enable && ( c == 0 || c == 1 ) ) continue;
			gst_element_set_state ( hgtv.dvb_rec_all_n[c], GST_STATE_PAUSED );

	gst_element_set_state ( hgtv.dvbsrc_tv, GST_STATE_PLAYING );
}

static void helia_gst_tv_tsdemux_remove ()
{
	g_debug ( "helia_gst_tsdemux_remove" );

	GstIterator *it = gst_bin_iterate_elements ( GST_BIN ( hgtv.dvbsrc_tv ) );
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

				gst_bin_remove ( GST_BIN ( hgtv.dvbsrc_tv ), element );

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

static void helia_gst_tv_checked_video ( gchar *data )
{
	if ( !g_strrstr ( data, "video-pid" ) || g_strrstr ( data, "video-pid=0" ) )
		hgtv.video_enable = FALSE;
	else
		hgtv.video_enable = TRUE;
}

static void helia_gst_tv_set_tuning_timeout ( GstElement *element )
{
	guint64 timeout = 0;
	g_object_get ( element, "tuning-timeout", &timeout, NULL );
	g_object_set ( element, "tuning-timeout", (guint64)timeout / 5, NULL );
}

static void helia_gst_tv_check_a_f ( GstElement *element )
{
	guint adapter = 0, frontend = 0;
	g_object_get ( element, "adapter",  &adapter,  NULL );
	g_object_get ( element, "frontend", &frontend, NULL );

	helia_get_dvb_info ( NULL, adapter, frontend );
}

static void helia_gst_tv_data_set ( gchar *data )
{
	g_debug ( "helia_gst_tv_data_set:: data %s", data );

	GstElement *element = hgtv.dvb_all_n[0];
	helia_gst_tv_set_tuning_timeout ( element );

	gchar **fields = g_strsplit ( data, ":", 0 );
	guint numfields = g_strv_length ( fields );

	if ( hgtv.ch_name ) g_free ( hgtv.ch_name );
		hgtv.ch_name = g_strdup ( fields[0] );

	guint j = 0;
	for ( j = 1; j < numfields; j++ )
	{
		if ( g_strrstr ( fields[j], "delsys" ) || g_strrstr ( fields[j], "audio-pid" ) || g_strrstr ( fields[j], "video-pid" ) ) continue;

		if ( !g_strrstr ( fields[j], "=" ) ) continue;

		gchar **splits = g_strsplit ( fields[j], "=", 0 );

		g_debug ( "helia_gst_tv_data_set:: gst-param %s | gst-value %s", splits[0], splits[1] );

		if ( g_strrstr ( splits[0], "polarity" ) )
		{
			if ( splits[1][0] == 'v' || splits[1][0] == 'V' || splits[1][0] == '0' )
				g_object_set ( element, "polarity", "V", NULL );
			else
				g_object_set ( element, "polarity", "H", NULL );

			g_strfreev (splits);
			
			continue;
		}

		long dat = atol ( splits[1] );

		if ( g_strrstr ( splits[0], "program-number" ) )
		{
			g_object_set ( hgtv.dvb_all_n[1], "program-number", dat, NULL );
		}
		else if ( g_strrstr ( splits[0], "symbol-rate" ) )
			g_object_set ( element, "symbol-rate", ( dat > 100000) ? dat/1000 : dat, NULL );
		else if ( g_strrstr ( splits[0], "lnb-type" ) )
			helia_set_lnb_low_high_switch ( element, dat );
		else
			g_object_set ( element, splits[0], dat, NULL );

		g_strfreev (splits);
	}

	g_strfreev (fields);

	helia_gst_tv_check_a_f ( element );
}

static GstPadProbeReturn helia_gst_tv_blockpad_probe_event ( GstPad * pad, GstPadProbeInfo * info, gpointer user_data )
{
	if ( hbgv.all_info )
		g_debug ( "helia_gst_tv_blockpad_probe_event:: user_data %s", (gchar *)user_data );

	if ( GST_EVENT_TYPE (GST_PAD_PROBE_INFO_DATA (info)) != GST_EVENT_EOS )
		return GST_PAD_PROBE_PASS;

	gst_pad_remove_probe ( pad, GST_PAD_PROBE_INFO_ID (info) );

	gst_element_set_state ( hgtv.dvb_rec_all_n[5], GST_STATE_NULL );

	if ( hgtv.rec_ses )
	{
		gchar *date_str = helia_service_get_time_date_str ();
		gchar *file_rec = g_strdup_printf ( "%s/%s_%s.%s", hbgv.rec_dir, hgtv.ch_name, date_str, "m2ts" );

		g_object_set ( hgtv.dvb_rec_all_n[5], "location", file_rec, NULL );

		g_free ( file_rec );
		g_free ( date_str );
	}
	else
		g_object_set ( hgtv.dvb_rec_all_n[5], "location", "/dev/null", NULL );

	gst_element_link ( hgtv.dvb_rec_all_n[4], hgtv.dvb_rec_all_n[5] );

	gst_element_set_state ( hgtv.dvb_rec_all_n[5], GST_STATE_PLAYING );

	return GST_PAD_PROBE_DROP;
}

static GstPadProbeReturn helia_gst_tv_blockpad_probe ( GstPad * pad, GstPadProbeInfo * info, gpointer user_data )
{
	GstPad *sinkpad;

	gst_pad_remove_probe ( pad, GST_PAD_PROBE_INFO_ID (info) );

	sinkpad = gst_element_get_static_pad ( hgtv.dvb_rec_all_n[5], "sink" );

	gst_pad_add_probe ( sinkpad, GST_PAD_PROBE_TYPE_BLOCK | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,
						helia_gst_tv_blockpad_probe_event, user_data, NULL );

	gst_pad_send_event ( sinkpad, gst_event_new_eos () );

	gst_object_unref ( sinkpad );

	return GST_PAD_PROBE_OK;
}

void helia_gst_tv_record ()
{
	g_debug ( "helia_gst_tv_record" );

	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv )->current_state != GST_STATE_PLAYING ) return;

	if ( !hgtv.rec_status )
	{
		helia_gst_tv_rec_ts ();
		hgtv.rec_status = !hgtv.rec_status;
		hgtv.rec_ses = TRUE;
	}
	else
	{
		hgtv.rec_ses = !hgtv.rec_ses;

		gst_pad_add_probe ( hgtv.blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM,
		helia_gst_tv_blockpad_probe, "blockpad", NULL );
	}
}

void helia_gst_tv_stop ()
{
	g_debug ( "helia_gst_tv_stop" );

	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv )->current_state != GST_STATE_NULL )
	{
		gst_element_set_state ( hgtv.dvbsrc_tv, GST_STATE_NULL );
		helia_gst_tv_tsdemux_remove ();
		
		hgtv.rec_status = FALSE;
		hgtv.rec_ses = FALSE;

		hgtv.count_audio_track = 0;
		hgtv.set_audio_track   = 0;

		helia_media_tv_set_sgn_snr ( hgtv.dvbsrc_tv, 0, 0, FALSE, FALSE );

		helia_base_update_win ();
	}
}

static gboolean helia_gst_tv_update_win ()
{
	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv )->current_state == GST_STATE_NULL ) return FALSE;

	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv )->current_state == GST_STATE_PLAYING )
	{
		if ( hgtv.video_enable ) return FALSE;

		helia_base_update_win ();
	}
	else
	{
		g_debug ( "No TV GST_STATE_PLAYING" );

		return TRUE;
	}

	return FALSE;
}

void helia_gst_tv_play ( gchar *data, guintptr win_handle )
{
	g_debug ( "helia_gst_tv_play: win_handle_tv %lu", (gulong)win_handle );

	helia_gst_tv_stop ();
	hgtv.win_handle_tv = win_handle;

	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv )->current_state != GST_STATE_PLAYING )
	{
		helia_gst_tv_checked_video ( data );
		helia_gst_tv_tsdemux ();

		g_object_set ( hgtv.dvb_all_n[16], "volume", hgtv.volume_tv, NULL );
		helia_gst_tv_data_set ( data );

		gst_element_set_state ( hgtv.dvbsrc_tv, GST_STATE_PLAYING );

		g_timeout_add ( 250, (GSourceFunc)helia_gst_tv_update_win, NULL );
	}
}

gboolean helia_gst_tv_get_state_play ()
{
	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv  )->current_state == GST_STATE_PLAYING ) return TRUE;

	return FALSE;
}
gboolean helia_gst_tv_get_state_stop ()
{
	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv  )->current_state == GST_STATE_NULL ) return TRUE;

	return FALSE;
}

gboolean helia_gst_tv_draw_logo ()
{
	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv )->current_state == GST_STATE_NULL )
		return TRUE;

	if ( !hgtv.video_enable )
		return TRUE;

	return FALSE;
}

gboolean helia_gst_tv_ega ()
{
	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv  )->current_state != GST_STATE_PLAYING ) return FALSE;

	helia_eqa_win ( hgtv.dvb_all_n[17], helia_base_win_ret (), hbgv.opacity_eq );

	return TRUE;
}
gboolean helia_gst_tv_egv ()
{
	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv  )->current_state != GST_STATE_PLAYING ) return FALSE;

	helia_eqv_win ( hgtv.dvb_all_n[8], helia_base_win_ret (), hbgv.opacity_eq );

	return TRUE;
}

static gboolean helia_gst_tv_mute_get ()
{
	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv )->current_state != GST_STATE_PLAYING ) return TRUE;

	gboolean mute = FALSE;

	g_object_get ( hgtv.dvb_all_n[16], "mute", &mute, NULL );

	return mute;
}
void helia_gst_tv_mute_set ()
{
	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv )->current_state != GST_STATE_PLAYING ) return;

	gboolean mute = FALSE;

	g_object_get ( hgtv.dvb_all_n[16], "mute", &mute, NULL );
	g_object_set ( hgtv.dvb_all_n[16], "mute", !mute, NULL );

	g_debug ( "helia_gst_tv_mute_set:: mute %s", mute ? "TRUE" : "FALSE" );
}
void helia_gst_tv_mute_set_widget ( GtkWidget *widget )
{
	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv )->current_state != GST_STATE_PLAYING ) return;

	gboolean mute = FALSE;

	g_object_get ( hgtv.dvb_all_n[16], "mute", &mute, NULL );
	g_object_set ( hgtv.dvb_all_n[16], "mute", !mute, NULL );

	if ( widget ) gtk_widget_set_sensitive ( GTK_WIDGET ( widget ), mute );

	g_debug ( "helia_gst_tv_mute_set_widget:: mute %s", mute ? "TRUE" : "FALSE" );
}

void helia_gst_tv_volume_changed ( GtkScaleButton *button, gdouble value )
{
	if ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv )->current_state == GST_STATE_PLAYING )
	{
		g_object_set ( hgtv.dvb_all_n[16], "volume", value, NULL );
		
		hgtv.volume_tv = value;
	}

	if ( hbgv.all_info )
		g_debug ( "helia_gst_tv_volume_changed:: value %f | widget name %s", value, gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

void helia_gst_tv_set_text_audio ( GtkButton *button, gboolean text, gboolean create_text, gboolean audio, gboolean create_audio )
{
	gint cur_text = 0, num_text = 0;

	gint cur_audio = hgtv.set_audio_track, num_audio = hgtv.count_audio_track;
	
	gint cur = 0, num = 0;
	
	//gint changed_track_text = 0, changed_track_audio = 0;
	
	if ( text )
	{
		if ( create_text )
		{
			if ( num_text == 0 || GST_ELEMENT_CAST ( hgtv.dvbsrc_tv  )->current_state == GST_STATE_NULL ) { cur = -1; }
			if ( num_text == 1 ) { cur = 0; }
			if ( num_text >  1 ) { cur = cur_text; }
		}
		else
		{
			if ( num_text == 0 || GST_ELEMENT_CAST ( hgtv.dvbsrc_tv  )->current_state == GST_STATE_NULL ) { cur = -1; }
			if ( num_text == 1 ) { cur = 0; }
			
			if ( num_text >  1 )
			{
				if ( cur_text + 1 == num_text ) { cur = 0; } else { cur = cur_text + 1; }				
				// helia_gst_tv_changed_text_track ( cur );
			}
		}

		num = num_text;
	}
	
	if ( audio )
	{
		if ( create_audio )
		{
			if ( num_audio == 0 || GST_ELEMENT_CAST ( hgtv.dvbsrc_tv  )->current_state == GST_STATE_NULL ) { cur = -1; }
			if ( num_audio == 1 ) { cur = 0; }
			if ( num_audio >  1 ) { cur = cur_audio; }
		}
		else
		{
			if ( num_audio == 0 || GST_ELEMENT_CAST ( hgtv.dvbsrc_tv  )->current_state == GST_STATE_NULL ) { cur = -1; }
			if ( num_audio == 1 ) { cur = 0; }
			
			if ( num_audio >  1 )
			{
				if ( cur_audio + 1 == num_audio ) { cur = 0; } else { cur = cur_audio + 1; }
				helia_gst_tv_changed_audio_track ( cur );
			}
		}
		
		num = num_audio;
	}
	
	gchar *text_set = g_strdup_printf ( "%d / %d", cur + 1, num );
		gtk_button_set_label ( button, text_set );
	g_free  ( text_set );	
}

HGTvState helia_gst_tv_state_all ()
{
	HGTvState hgtvs;
	
	hgtvs.count_audio_track = hgtv.count_audio_track;
	hgtvs.set_audio_track   = hgtv.set_audio_track;
	
	hgtvs.volume_tv  = hgtv.volume_tv;
	hgtvs.state_mute = helia_gst_tv_mute_get ();
	
	hgtvs.state_playing = ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv  )->current_state == GST_STATE_PLAYING ) ? TRUE : FALSE;
	hgtvs.state_stop    = ( GST_ELEMENT_CAST ( hgtv.dvbsrc_tv  )->current_state == GST_STATE_NULL    ) ? TRUE : FALSE;
	
	return hgtvs;
}



// Window Info

void helia_gst_tv_info ()
{
	g_print ( "helia_gst_tv_info \n" );
}
