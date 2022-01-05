/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "player.h"
#include "file.h"
#include "default.h"
#include "settings.h"
#include "enc-prop.h"
#include "helia-eqa.h"
#include "helia-eqv.h"

#include <time.h>
#include <gst/video/videooverlay.h>

enum prop_player
{
	PROP_0,
	PROP_MUTE,
	PROP_VOLUME,
	N_PROPS
};

struct _Player
{
	GstObject parent_instance;

	GstElement *playbin;
	GstElement *volume;
	GstElement *videoblnc;
	GstElement *equalizer;

	GstElement *enc_video;
	GstElement *enc_audio;
	GstElement *enc_muxer;

	GstElement *pipeline_rec;

	char *rec_in_out[2];

	ulong xid;
	uint slider_timeout;

	time_t t_start;
	gboolean pulse;

	gboolean debug;
	gboolean rec_video;
};

G_DEFINE_TYPE ( Player, player, GST_TYPE_OBJECT )

static gboolean player_slider_refresh ( Player *player )
{
	GstElement *element = player->playbin;
	if ( player->pipeline_rec ) element = player->pipeline_rec;

	if ( GST_ELEMENT_CAST ( element )->current_state == GST_STATE_NULL    ) return TRUE;
	if ( GST_ELEMENT_CAST ( element )->current_state  < GST_STATE_PLAYING ) return TRUE;

	gboolean dur_b = FALSE;
	gint64 duration = 0, current = 0;

	if ( gst_element_query_position ( element, GST_FORMAT_TIME, &current ) )
	{
		if ( gst_element_query_duration ( element, GST_FORMAT_TIME, &duration ) ) dur_b = TRUE;

			if ( dur_b && duration / GST_SECOND > 0 )
			{
				if ( current / GST_SECOND < duration / GST_SECOND )
				{
					g_signal_emit_by_name ( player, "player-slider", current, 8, duration, 10, TRUE );
				}
			}
			else
			{
				g_signal_emit_by_name ( player, "player-slider", current, 8, -1, 10, FALSE );
			}
	}

	return TRUE;
}

static void player_stop_record ( Player *player )
{
	if ( player->pipeline_rec == NULL ) return;

	gst_element_set_state ( player->pipeline_rec, GST_STATE_NULL );

	gst_object_unref ( player->pipeline_rec );

	player->pipeline_rec = NULL;
}

static void player_set_stop ( Player *player )
{
	player_stop_record ( player );

	g_object_set ( player->playbin, "mute", FALSE, NULL );
	gst_element_set_state ( player->playbin, GST_STATE_NULL );

	if ( player->slider_timeout ) g_source_remove ( player->slider_timeout );
	player->slider_timeout = 0;
}

static void player_set_pause ( Player *player )
{
	g_autofree char *uri = NULL;
	g_object_get ( player->playbin, "current-uri", &uri, NULL );

	if ( uri == NULL ) return;

	if ( GST_ELEMENT_CAST ( player->playbin )->current_state == GST_STATE_PLAYING )
		gst_element_set_state ( player->playbin, GST_STATE_PAUSED  );
	else
		gst_element_set_state ( player->playbin, GST_STATE_PLAYING );
}

static void player_stop_set_play ( const char *file, ulong xid, Player *player )
{
	player->xid = xid;

	player_set_stop ( player );

	if ( g_strrstr ( file, "://" ) )
	{
		g_object_set ( player->playbin, "uri", file, NULL );
	}
	else
	{
		g_autofree char *uri = gst_filename_to_uri ( file, NULL );
		g_object_set ( player->playbin, "uri", uri, NULL );
	}

	g_object_set ( player->playbin, "mute", FALSE, NULL );
	gst_element_set_state ( player->playbin, GST_STATE_PLAYING );

	player->slider_timeout = g_timeout_add ( 100, (GSourceFunc)player_slider_refresh, player );
}

static GstBusSyncReply player_sync_handler ( G_GNUC_UNUSED GstBus *bus, GstMessage *message, Player *player )
{
	if ( !gst_is_video_overlay_prepare_window_handle_message ( message ) ) return GST_BUS_PASS;

	if ( player->xid != 0 )
	{
		GstVideoOverlay *xoverlay = GST_VIDEO_OVERLAY ( GST_MESSAGE_SRC ( message ) );
		gst_video_overlay_set_window_handle ( xoverlay, player->xid );

	} else { g_warning ( "Should have obtained window_handle by now!" ); }

	gst_message_unref ( message );

	return GST_BUS_DROP;
}

static void player_msg_cng ( G_GNUC_UNUSED GstBus *bus, G_GNUC_UNUSED GstMessage *msg, Player *player )
{
	if ( GST_MESSAGE_SRC ( msg ) != GST_OBJECT ( player->playbin ) ) return;

	GstState old_state, new_state;

	gst_message_parse_state_changed ( msg, &old_state, &new_state, NULL );

	switch ( new_state )
	{
		case GST_STATE_NULL:
		case GST_STATE_READY:
			break;

		case GST_STATE_PAUSED:
		{
			g_signal_emit_by_name ( player, "player-power-set", FALSE );
			break;
		}

		case GST_STATE_PLAYING:
		{
			g_signal_emit_by_name ( player, "player-power-set", TRUE );
			break;
		}

		default:
			break;
	}
}

static void player_msg_buf ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Player *player )
{
	int percent;
	gst_message_parse_buffering ( msg, &percent );

	if ( percent == 100 )
	{
		// if ( GST_ELEMENT_CAST ( player->playbin )->current_state == GST_STATE_PAUSED )
			gst_element_set_state ( player->playbin, GST_STATE_PLAYING );

		g_signal_emit_by_name ( player, "player-staus", " ⇄ 0% ", NULL );
	}
	else
	{
		if ( GST_ELEMENT_CAST ( player->playbin )->current_state == GST_STATE_PLAYING )
			gst_element_set_state ( player->playbin, GST_STATE_PAUSED );

		char buf[20];
		sprintf ( buf, " ⇄ %d%% ", percent );

		g_signal_emit_by_name ( player, "player-staus", buf, NULL );
	}
}

static void player_msg_eos ( G_GNUC_UNUSED GstBus *bus, G_GNUC_UNUSED GstMessage *msg, Player *player )
{
	g_autofree char *uri = NULL;
	g_object_get ( player->playbin, "current-uri", &uri, NULL );

	if ( uri && ( g_str_has_suffix ( uri, ".png" ) || g_str_has_suffix ( uri, ".jpg" ) ) ) return;

	g_signal_emit_by_name ( player, "player-next", uri );
}

static void player_msg_err ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Player *player )
{
	GError *err = NULL;
	char   *dbg = NULL;

	gst_message_parse_error ( msg, &err, &dbg );

	g_critical ( "%s: %s (%s)", __func__, err->message, (dbg) ? dbg : "no details" );

	g_signal_emit_by_name ( player, "player-message-dialog", err->message );

	g_error_free ( err );
	g_free ( dbg );

	if ( GST_ELEMENT_CAST ( player->playbin )->current_state != GST_STATE_PLAYING )
		player_set_stop ( player );
}

static void player_enc_create_elements ( Player *player )
{
	GSettings *setting = settings_init ();

	g_autofree char *enc_audio = NULL;
	g_autofree char *enc_video = NULL;
	g_autofree char *enc_muxer = NULL;

	if ( setting ) enc_audio = g_settings_get_string ( setting, "encoder-audio" );
	if ( setting ) enc_video = g_settings_get_string ( setting, "encoder-video" );
	if ( setting ) enc_muxer = g_settings_get_string ( setting, "encoder-muxer" );

	player->enc_audio = gst_element_factory_make ( ( enc_audio ) ? enc_audio : "vorbisenc", NULL );
	player->enc_video = gst_element_factory_make ( ( enc_video ) ? enc_video : "theoraenc", NULL );
	player->enc_muxer = gst_element_factory_make ( ( enc_muxer ) ? enc_muxer : "oggmux",    NULL );

	if ( setting ) g_object_unref ( setting );
}

static GstElement * player_create ( Player *player )
{
	GstElement *playbin = gst_element_factory_make ( "playbin", NULL );

	GstElement *bin_audio, *bin_video, *asink, *vsink;

	vsink     = gst_element_factory_make ( "autovideosink",     NULL );
	asink     = gst_element_factory_make ( "autoaudiosink",     NULL );

	player->videoblnc = gst_element_factory_make ( "videobalance",      NULL );
	player->equalizer = gst_element_factory_make ( "equalizer-nbands",  NULL );

	if ( !playbin )
	{
		g_critical ( "%s: playbin - not all elements could be created.", __func__ );
		return NULL;
	}

	bin_audio = gst_bin_new ( "audio-sink-bin" );
	gst_bin_add_many ( GST_BIN ( bin_audio ), player->equalizer, asink, NULL );
	gst_element_link_many ( player->equalizer, asink, NULL );

	GstPad *pad = gst_element_get_static_pad ( player->equalizer, "sink" );
	gst_element_add_pad ( bin_audio, gst_ghost_pad_new ( "sink", pad ) );
	gst_object_unref ( pad );

	bin_video = gst_bin_new ( "video-sink-bin" );
	gst_bin_add_many ( GST_BIN ( bin_video ), player->videoblnc, vsink, NULL );
	gst_element_link_many ( player->videoblnc, vsink, NULL );

	GstPad *padv = gst_element_get_static_pad ( player->videoblnc, "sink" );
	gst_element_add_pad ( bin_video, gst_ghost_pad_new ( "sink", padv ) );
	gst_object_unref ( padv );

	g_object_set ( playbin, "video-sink", bin_video, NULL );
	g_object_set ( playbin, "audio-sink", bin_audio, NULL );

	g_object_set ( playbin, "volume", VOLUME, NULL );

	GstBus *bus = gst_element_get_bus ( playbin );

	gst_bus_add_signal_watch_full ( bus, G_PRIORITY_DEFAULT );
	gst_bus_set_sync_handler ( bus, (GstBusSyncHandler)player_sync_handler, player, NULL );

	g_signal_connect ( bus, "message::eos",   G_CALLBACK ( player_msg_eos ), player );
	g_signal_connect ( bus, "message::error", G_CALLBACK ( player_msg_err ), player );
	g_signal_connect ( bus, "message::buffering", G_CALLBACK ( player_msg_buf ), player );
	g_signal_connect ( bus, "message::state-changed", G_CALLBACK ( player_msg_cng ), player );

	gst_object_unref ( bus );

	player_enc_create_elements ( player );

	return playbin;
}

static void player_msg_cng_rec ( G_GNUC_UNUSED GstBus *bus, G_GNUC_UNUSED GstMessage *msg, Player *player )
{
	if ( GST_MESSAGE_SRC ( msg ) != GST_OBJECT ( player->pipeline_rec ) ) return;

	GstState old_state, new_state;

	gst_message_parse_state_changed ( msg, &old_state, &new_state, NULL );

	switch ( new_state )
	{
		case GST_STATE_NULL:
		case GST_STATE_READY:
			break;

		case GST_STATE_PAUSED:
		{
			g_signal_emit_by_name ( player, "player-power-set", FALSE );
			break;
		}

		case GST_STATE_PLAYING:
		{
			g_signal_emit_by_name ( player, "player-power-set", TRUE );
			break;
		}

		default:
			break;
	}
}

static void player_msg_eos_rec ( G_GNUC_UNUSED GstBus *bus, G_GNUC_UNUSED GstMessage *msg, Player *player )
{
	player_stop_record ( player );
}

static void player_msg_err_rec ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Player *player )
{
	player_stop_record ( player );

	GError *err = NULL;
	char   *dbg = NULL;

	gst_message_parse_error ( msg, &err, &dbg );

	g_critical ( "%s: %s (%s)", __func__, err->message, (dbg) ? dbg : "no details" );

	g_signal_emit_by_name ( player, "player-message-dialog", err->message );

	g_error_free ( err );
	g_free ( dbg );
}

static void player_pad_add_hls ( GstElement *element, GstPad *pad, GstElement *el )
{
	gst_element_unlink ( element, el );

	GstPad *pad_va_sink = gst_element_get_static_pad ( el, "sink" );

	if ( gst_pad_link ( pad, pad_va_sink ) == GST_PAD_LINK_OK )
		gst_object_unref ( pad_va_sink );
	else
		g_message ( "%s:: linking pad failed ", __func__ );
}
/*
static GstElement * player_create_rec_bin ( gboolean f_hls, const char *uri, const char *rec )
{
	GstElement *pipeline_rec = gst_pipeline_new ( "pipeline-record" );

	GstElement *element_src  = gst_element_make_from_uri ( GST_URI_SRC, uri, NULL, NULL );
	GstElement *element_hq   = gst_element_factory_make  ( ( f_hls ) ? "hlsdemux" : "queue2", NULL );
	GstElement *element_sinc = gst_element_factory_make  ( "filesink", NULL );

	if ( !pipeline_rec || !element_hq || !element_sinc ) return NULL;

	gst_bin_add_many ( GST_BIN ( pipeline_rec ), element_src, element_hq, element_sinc, NULL );

	gst_element_link_many ( element_src, element_hq, NULL );

	if ( found_hls )
		g_signal_connect ( element_hq, "pad-added", G_CALLBACK ( player_pad_add_hls ), element_sinc );
	else
		gst_element_link ( element_hq, element_sinc );

	if ( g_object_class_find_property ( G_OBJECT_GET_CLASS ( element_src ), "location" ) )
		g_object_set ( element_src, "location", uri, NULL );
	else
		g_object_set ( element_src, "uri", uri, NULL );

	g_object_set ( element_sinc, "location", rec, NULL );

	return pipeline_rec;
}
*/

static gboolean player_pad_check_type ( GstPad *pad, const char *type )
{
	gboolean ret = FALSE;

	GstCaps *caps = gst_pad_get_current_caps ( pad );

	const char *name = gst_structure_get_name ( gst_caps_get_structure ( caps, 0 ) );

	if ( g_str_has_prefix ( name, type ) ) ret = TRUE;

	gst_caps_unref (caps);

	return ret;
}

static void player_pad_link ( GstPad *pad, GstElement *element, const char *name )
{
	GstPad *pad_va_sink = gst_element_get_static_pad ( element, "sink" );

	if ( gst_pad_link ( pad, pad_va_sink ) == GST_PAD_LINK_OK )
		gst_object_unref ( pad_va_sink );
	else
		g_debug ( "%s:: linking demux/decode name %s video/audio pad failed ", __func__, name );
}

static void player_pad_demux_audio ( G_GNUC_UNUSED GstElement *element, GstPad *pad, GstElement *element_audio )
{
	if ( player_pad_check_type ( pad, "audio" ) ) player_pad_link ( pad, element_audio, "demux audio" );
}

static void player_pad_demux_video ( G_GNUC_UNUSED GstElement *element, GstPad *pad, GstElement *element_video )
{
	if ( player_pad_check_type ( pad, "video" ) ) player_pad_link ( pad, element_video, "demux video" );
}

static void player_pad_decode ( G_GNUC_UNUSED GstElement *element, GstPad *pad, GstElement *element_va )
{
	player_pad_link ( pad, element_va, "decode  audio / video" );
}

static GstElement * player_create_rec_bin ( gboolean f_hls, gboolean video, const char *uri, const char *rec, double volume, Player *player )
{
	GstElement *pipeline_rec = gst_pipeline_new ( "pipeline-record" );

	if ( !pipeline_rec ) return NULL;

	const char *name = ( f_hls ) ? "hlsdemux" : "queue2";

	struct rec_all { const char *name; } rec_all_n[] =
	{
		{ "souphttpsrc" }, { name        }, { "tee"          }, { "queue2"        }, { "decodebin"     },
		{ "queue2"      }, { "decodebin" }, { "audioconvert" }, { "volume"        }, { "autoaudiosink" },
		{ "queue2"      }, { "decodebin" }, { "videoconvert" }, { "autovideosink" },
		{ "queue2"      }, { "filesink"  }
	};

	GstElement *elements[ G_N_ELEMENTS ( rec_all_n ) ];

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( rec_all_n ); c++ )
	{
		if ( !video && ( c > 9 && c < 14 ) ) continue;

		if ( c == 0 )
			elements[c] = gst_element_make_from_uri ( GST_URI_SRC, uri, NULL, NULL );
		else
			elements[c] = gst_element_factory_make  ( rec_all_n[c].name, NULL );

		if ( !elements[c] )
		{
			g_critical ( "%s:: element (factory make) - %s not created. \n", __func__, rec_all_n[c].name );
			return NULL;
		}

		gst_bin_add ( GST_BIN ( pipeline_rec ), elements[c] );

		if (  c == 0 || c == 2 || c == 5 || c == 7 || c == 10 || c == 12 || c == 14 ) continue;

		gst_element_link ( elements[c-1], elements[c] );
	}

	if ( f_hls )
		g_signal_connect ( elements[1], "pad-added", G_CALLBACK ( player_pad_add_hls ), elements[2] );
	else
		gst_element_link ( elements[1], elements[2] );

	g_signal_connect ( elements[4], "pad-added", G_CALLBACK ( player_pad_demux_audio ), elements[5] );
	if ( video ) g_signal_connect ( elements[4], "pad-added", G_CALLBACK ( player_pad_demux_video ), elements[10] );

	g_signal_connect ( elements[6], "pad-added", G_CALLBACK ( player_pad_decode ), elements[7] );
	if ( video ) g_signal_connect ( elements[11], "pad-added", G_CALLBACK ( player_pad_decode ), elements[12] );

	if ( g_object_class_find_property ( G_OBJECT_GET_CLASS ( elements[0] ), "location" ) )
		g_object_set ( elements[0], "location", uri, NULL );
	else
		g_object_set ( elements[0], "uri", uri, NULL );

	gst_element_link ( elements[2], elements[14] );

	g_object_set ( elements[15], "location", rec, NULL );

	player->volume = elements[8];
	g_object_set ( player->volume, "volume", volume, NULL );

	return pipeline_rec;
}

static GstElement * player_create_rec_enc_bin ( gboolean f_hls, gboolean video, const char *uri, const char *rec, double volume, Player *player )
{
	GstElement *pipeline_rec = gst_pipeline_new ( "pipeline-record" );

	if ( !pipeline_rec ) return NULL;

	const char *name = ( f_hls ) ? "hlsdemux" : "queue2";

	struct rec_all { const char *name; } rec_all_n[] =
	{
		{ "souphttpsrc" }, { name        }, { "decodebin" },
		{ "queue2"      }, { "decodebin" }, { "audioconvert" }, { "tee" }, { "queue2" }, { "volume"        }, { "autoaudiosink" },
		{ "queue2"      }, { "decodebin" }, { "videoconvert" }, { "tee" }, { "queue2" }, { "autovideosink" },
	};

	GstElement *elements[ G_N_ELEMENTS ( rec_all_n ) ];

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( rec_all_n ); c++ )
	{
		if ( !video && ( c > 9 ) ) continue;

		if ( c == 0 )
			elements[c] = gst_element_make_from_uri ( GST_URI_SRC, uri, NULL, NULL );
		else
			elements[c] = gst_element_factory_make  ( rec_all_n[c].name, NULL );

		if ( !elements[c] )
		{
			g_critical ( "%s:: element (factory make) - %s not created. \n", __func__, rec_all_n[c].name );
			return NULL;
		}

		gst_bin_add ( GST_BIN ( pipeline_rec ), elements[c] );

		if (  c == 0 || c == 2 || c == 3 || c == 5 || c == 10 || c == 12 ) continue;

		gst_element_link ( elements[c-1], elements[c] );
	}

	if ( f_hls )
		g_signal_connect ( elements[1], "pad-added", G_CALLBACK ( player_pad_add_hls ), elements[2] );
	else
		gst_element_link ( elements[1], elements[2] );

	g_signal_connect ( elements[2], "pad-added", G_CALLBACK ( player_pad_demux_audio ), elements[3] );
	if ( video ) g_signal_connect ( elements[2], "pad-added", G_CALLBACK ( player_pad_demux_video ), elements[10] );

	g_signal_connect ( elements[4], "pad-added", G_CALLBACK ( player_pad_decode ), elements[5] );
	if ( video ) g_signal_connect ( elements[11], "pad-added", G_CALLBACK ( player_pad_decode ), elements[12] );

	if ( g_object_class_find_property ( G_OBJECT_GET_CLASS ( elements[0] ), "location" ) )
		g_object_set ( elements[0], "location", uri, NULL );
	else
		g_object_set ( elements[0], "uri", uri, NULL );

	GstElement *enc_video = g_object_ref ( player->enc_video ), *enc_audio = g_object_ref ( player->enc_audio ), *enc_muxer = g_object_ref ( player->enc_muxer );

	GstElement *filesink = gst_element_factory_make ( "filesink", NULL );
	GstElement *queue2_a = gst_element_factory_make ( "queue2",   NULL );

	gst_bin_add_many ( GST_BIN ( pipeline_rec ), queue2_a, enc_audio, enc_muxer, filesink, NULL );
	gst_element_link_many ( elements[6], queue2_a, enc_audio, enc_muxer, NULL );

	gst_element_link ( enc_muxer, filesink );

	if ( video )
	{
		GstElement *queue2_v = gst_element_factory_make ( "queue2", NULL );

		gst_bin_add_many ( GST_BIN ( pipeline_rec ), queue2_v, enc_video, NULL );
		gst_element_link_many ( elements[13], queue2_v, enc_video, enc_muxer, NULL );
	}

	g_object_set ( filesink, "location", rec, NULL );

	player->volume = elements[8];
	g_object_set ( player->volume, "volume", volume, NULL );

	return pipeline_rec;
}

static gboolean player_update_record ( Player *player )
{
	if ( player->pipeline_rec == NULL ) return FALSE;

	GFile *file = g_file_new_for_path ( player->rec_in_out[1] );
	GFileInfo *file_info = g_file_query_info ( file, "standard::*", 0, NULL, NULL );

	uint64_t dsize = ( file_info ) ? g_file_info_get_attribute_uint64 ( file_info, "standard::size" ) : 0;

	g_autofree char *str_size = g_format_size ( dsize );

	const char *rec_cl = ( player->pulse ) ? "ff0000" :"2b2222";

	time_t t_cur;
	time ( &t_cur );

	if ( ( t_cur > player->t_start ) ) { time ( &player->t_start ); player->pulse = !player->pulse; }

	g_autofree char *markup = g_markup_printf_escaped ( "<span foreground=\"#%s\">   ◉   </span>%s", rec_cl, str_size );

	g_signal_emit_by_name ( player, "player-staus", NULL, markup );

	if ( file_info ) g_object_unref ( file_info );
	g_object_unref ( file );

	player_slider_refresh ( player );

	return TRUE;
}

static void player_record ( Player *player )
{
	if ( player->pipeline_rec == NULL )
	{
		free ( player->rec_in_out[0] );
		free ( player->rec_in_out[1] );

		g_autofree char *uri = NULL;
		g_object_get ( player->playbin, "current-uri", &uri, NULL );

		if ( !uri ) return;

		int n_video = 0;
		g_object_get ( player->playbin, "n-video", &n_video, NULL );

		g_autofree char *rec_dir = NULL;
		g_autofree char *dt = helia_time_to_str ();

		gboolean enc_b = FALSE;
		GSettings *setting = settings_init ();
		if ( setting ) rec_dir = g_settings_get_string ( setting, "rec-dir" );
		if ( setting ) enc_b   = g_settings_get_boolean ( setting, "encoding-iptv" );

		char path[PATH_MAX];

		if ( setting && rec_dir && !g_str_has_prefix ( rec_dir, "none" ) )
			sprintf ( path, "%s/Record-iptv-%s", rec_dir, dt );
		else
			sprintf ( path, "%s/Record-iptv-%s", g_get_home_dir (), dt );

		gboolean hls = FALSE;
		if ( uri && g_strrstr ( uri, ".m3u8" ) ) hls = TRUE;

		double volume = VOLUME;
		g_object_get ( player->playbin, "volume", &volume, NULL );

		player_set_stop ( player );

		player->rec_video = ( n_video > 0 ) ? TRUE : FALSE;

		if ( enc_b )
			player->pipeline_rec = player_create_rec_enc_bin ( hls, player->rec_video, uri, path, volume, player );
		else
			player->pipeline_rec = player_create_rec_bin ( hls, player->rec_video, uri, path, volume, player );

		if ( player->pipeline_rec == NULL ) return;

		GstBus *bus = gst_element_get_bus ( player->pipeline_rec );
		gst_bus_add_signal_watch_full ( bus, G_PRIORITY_DEFAULT );
		gst_bus_set_sync_handler ( bus, (GstBusSyncHandler)player_sync_handler, player, NULL );

		g_signal_connect ( bus, "message::eos",   G_CALLBACK ( player_msg_eos_rec ), player );
		g_signal_connect ( bus, "message::error", G_CALLBACK ( player_msg_err_rec ), player );
		g_signal_connect ( bus, "message::state-changed", G_CALLBACK ( player_msg_cng_rec ), player );

		gst_object_unref ( bus );
		if ( setting ) g_object_unref ( setting );

		player->rec_in_out[0] = g_strdup ( uri  );
		player->rec_in_out[1] = g_strdup ( path );

		gst_element_set_state ( player->pipeline_rec, GST_STATE_PLAYING );

		g_timeout_add ( 250, (GSourceFunc)player_update_record, player );
	}
	else
	{
		player_stop_record ( player );

		if ( player->rec_in_out[0] )
			player_stop_set_play ( player->rec_in_out[0], player->xid, player );
		else
			player_set_stop ( player );
	}
}

static gboolean player_set_rec ( Player *player )
{
	gboolean ret = TRUE;

	player_record ( player );

	if ( player->pipeline_rec == NULL ) ret = FALSE;

	return ret;
}

static void player_scroll_new_pos ( gint64 set_pos, gboolean up_dwn, Player *player )
{
	gboolean dur_b = FALSE;
	gint64 current = 0, duration = 0, new_pos = 0, skip = (gint64)( set_pos * GST_SECOND );

	if ( gst_element_query_position ( player->playbin, GST_FORMAT_TIME, &current ) )
	{
		if ( gst_element_query_duration ( player->playbin, GST_FORMAT_TIME, &duration ) ) dur_b = TRUE;

		if ( !dur_b || duration / GST_SECOND < 1 ) return;

		if ( up_dwn ) new_pos = ( duration > ( current + skip ) ) ? ( current + skip ) : duration;

		if ( !up_dwn ) new_pos = ( current > skip ) ? ( current - skip ) : 0;

		gst_element_seek_simple ( player->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, new_pos );

		g_signal_emit_by_name ( player, "player-slider", new_pos, 8, duration, 10, TRUE );
	}
}

static void player_step_pos ( guint64 am, Player *player )
{
	gst_element_send_event ( player->playbin, gst_event_new_step ( GST_FORMAT_BUFFERS, am, 1.0, TRUE, FALSE ) );

	gint64 current = 0;

	if ( gst_element_query_position ( player->playbin, GST_FORMAT_TIME, &current ) )
	{
		g_signal_emit_by_name ( player, "player-slider", current, 7, -1, 10, FALSE );
	}
}

static void player_step_frame ( Player *player )
{
	if ( GST_ELEMENT_CAST ( player->playbin )->current_state == GST_STATE_NULL ) return;

	int n_video = 0;
	g_object_get ( player->playbin, "n-video", &n_video, NULL );

	if ( n_video == 0 ) return;

	if ( GST_ELEMENT_CAST ( player->playbin )->current_state == GST_STATE_PLAYING )
		gst_element_set_state ( player->playbin, GST_STATE_PAUSED );

	player_step_pos ( 1, player );
}

static void player_handler_play ( Player *player, ulong xid, const char *data )
{
	player_stop_set_play ( data, xid, player );
}

static gboolean player_handler_rec ( Player *player )
{
	return player_set_rec ( player );
}

static void player_handler_eqa ( Player *player, GstObject *obj, double opacity )
{
	GtkWindow *window = GTK_WINDOW ( obj );

	if ( GST_ELEMENT_CAST ( player->playbin )->current_state == GST_STATE_PLAYING )
		helia_eqa_win ( opacity, window, player->equalizer );
}

static void player_handler_eqv ( Player *player, GstObject *obj, double opacity )
{
	GtkWindow *window = GTK_WINDOW ( obj );

	if ( GST_ELEMENT_CAST ( player->playbin )->current_state == GST_STATE_PLAYING )
		helia_eqv_win ( opacity, window, player->videoblnc );
}

static void player_handler_stop ( Player *player )
{
	player_set_stop ( player );
}

static void player_handler_pause ( Player *player )
{
	player_set_pause ( player );
}

static void player_handler_frame ( Player *player )
{
	player_step_frame ( player );
}

static void player_handler_seek ( Player *player, double value )
{
	if ( GST_ELEMENT_CAST ( player->playbin )->current_state == GST_STATE_NULL ) return;

	gst_element_seek_simple ( player->playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, (gint64)( value * GST_SECOND ) );
}

static void player_handler_scrool ( Player *player, gboolean up_dwn )
{
	if ( GST_ELEMENT_CAST ( player->playbin )->current_state == GST_STATE_NULL ) return;

	gint64 skip = 20;
	player_scroll_new_pos ( skip, up_dwn, player );
}

static gboolean player_handler_video ( Player *player )
{
	if ( player->pipeline_rec && player->rec_video ) return FALSE;

	if ( GST_ELEMENT_CAST ( player->playbin )->current_state < GST_STATE_PAUSED ) return TRUE;

	int n_video = 0;
	g_object_get ( player->playbin, "n-video", &n_video, NULL );

	if ( !n_video ) return TRUE;

	return FALSE;
}

static gboolean player_handler_isplay ( Player *player, gboolean rec_status )
{
	if ( GST_ELEMENT_CAST ( player->playbin )->current_state == GST_STATE_PLAYING ) return TRUE;

	if ( rec_status && player->pipeline_rec && GST_ELEMENT_CAST ( player->pipeline_rec )->current_state == GST_STATE_PLAYING ) return TRUE;

	return FALSE;
}

static GObject * player_handler_element ( Player *player )
{
	return G_OBJECT ( player->playbin );
}

static void player_enc_prop_set_audio_handler ( G_GNUC_UNUSED EncProp *ep, GObject *object, Player *player )
{
	gst_object_unref ( player->enc_audio );

	player->enc_audio = GST_ELEMENT ( object );
}
static void player_enc_prop_set_video_handler ( G_GNUC_UNUSED EncProp *ep, GObject *object, Player *player )
{
	gst_object_unref ( player->enc_video );

	player->enc_video = GST_ELEMENT ( object );
}
static void player_enc_prop_set_muxer_handler ( G_GNUC_UNUSED EncProp *ep, GObject *object, Player *player )
{
	gst_object_unref ( player->enc_muxer );

	player->enc_muxer = GST_ELEMENT ( object );
}

static void player_handler_enc ( Player *player, GstObject *obj, gpointer data )
{
	EncProp *ep = data;
	GtkWindow *window = GTK_WINDOW ( obj );

	enc_prop_set_run ( window, player->playbin, player->enc_video, player->enc_audio, player->enc_muxer, TRUE, ep );

	g_signal_connect ( ep, "enc-prop-set-audio", G_CALLBACK ( player_enc_prop_set_audio_handler ), player );
	g_signal_connect ( ep, "enc-prop-set-video", G_CALLBACK ( player_enc_prop_set_video_handler ), player );
	g_signal_connect ( ep, "enc-prop-set-muxer", G_CALLBACK ( player_enc_prop_set_muxer_handler ), player );
}

static void player_init ( Player *player )
{
	player->enc_video = NULL;
	player->enc_audio = NULL;
	player->enc_muxer = NULL;

	player->rec_in_out[0] = NULL;
	player->rec_in_out[1] = NULL;

	player->slider_timeout = 0;

	player->debug = ( g_getenv ( "DVB_DEBUG" ) ) ? TRUE : FALSE;

	player->playbin = player_create ( player );

	g_signal_connect ( player, "player-rec",     G_CALLBACK ( player_handler_rec    ), NULL );
	g_signal_connect ( player, "player-eqa",     G_CALLBACK ( player_handler_eqa    ), NULL );
	g_signal_connect ( player, "player-eqv",     G_CALLBACK ( player_handler_eqv    ), NULL );
	g_signal_connect ( player, "player-play",    G_CALLBACK ( player_handler_play   ), NULL );
	g_signal_connect ( player, "player-stop",    G_CALLBACK ( player_handler_stop   ), NULL );
	g_signal_connect ( player, "player-frame",   G_CALLBACK ( player_handler_frame  ), NULL );
	g_signal_connect ( player, "player-pause",   G_CALLBACK ( player_handler_pause  ), NULL );
	g_signal_connect ( player, "player-video",   G_CALLBACK ( player_handler_video  ), NULL );
	g_signal_connect ( player, "player-seek",    G_CALLBACK ( player_handler_seek   ), NULL );
	g_signal_connect ( player, "player-scrool",  G_CALLBACK ( player_handler_scrool ), NULL );
	g_signal_connect ( player, "player-is-play", G_CALLBACK ( player_handler_isplay ), NULL );
	g_signal_connect ( player, "player-enc",     G_CALLBACK ( player_handler_enc    ), NULL );
	g_signal_connect ( player, "player-element", G_CALLBACK ( player_handler_element ), NULL );
}

static void player_get_property ( GObject *object, uint id, GValue *value, GParamSpec *pspec )
{
	Player *player = PLAYER_OBJECT ( object );

	GstElement *volume = NULL;
	if ( GST_ELEMENT_CAST ( player->playbin )->current_state > GST_STATE_NULL ) volume = player->playbin;
	if ( player->pipeline_rec && GST_ELEMENT_CAST ( player->pipeline_rec )->current_state > GST_STATE_NULL ) volume = player->volume;

	double val = 0;
	gboolean mute = FALSE;

	if ( volume ) g_object_get ( volume, "volume", &val, NULL );
	if ( volume ) g_object_get ( volume, "mute",   &mute,   NULL );

	switch ( id )
	{
		case PROP_VOLUME:
			if ( volume ) g_value_set_double ( value, val );
			break;

		case PROP_MUTE:
			if ( volume ) g_value_set_boolean ( value, mute );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, id, pspec );
			break;
	}
}

static void player_set_property ( GObject *object, uint id, const GValue *value, GParamSpec *pspec )
{
	Player *player = PLAYER_OBJECT ( object );

	GstElement *volume = NULL;
	if ( GST_ELEMENT_CAST ( player->playbin )->current_state > GST_STATE_NULL ) volume = player->playbin;
	if ( player->pipeline_rec && GST_ELEMENT_CAST ( player->pipeline_rec )->current_state > GST_STATE_NULL ) volume = player->volume;

	double val = 0;
	gboolean mute = FALSE;

	switch ( id )
	{
		case PROP_VOLUME:
			val = g_value_get_double (value);
			if ( volume ) g_object_set ( volume, "volume", val, NULL );
			break;

		case PROP_MUTE:
			mute = g_value_get_boolean (value);
			if ( volume ) g_object_set ( volume, "mute", mute, NULL );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, id, pspec );
			break;
	}
}

static void player_finalize ( GObject *object )
{
	Player *player = PLAYER_OBJECT ( object );

	gst_object_unref ( player->enc_audio );
	gst_object_unref ( player->enc_video );
	gst_object_unref ( player->enc_muxer );

	gst_object_unref ( player->playbin );

	free ( player->rec_in_out[0] );
	free ( player->rec_in_out[1] );

	G_OBJECT_CLASS (player_parent_class)->finalize (object);
}

static void player_class_init ( PlayerClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = player_finalize;

	g_signal_new ( "player-stop", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "player-rec", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_BOOLEAN, 0 );

	g_signal_new ( "player-frame", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "player-pause", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "player-play", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_ULONG, G_TYPE_STRING );

	g_signal_new ( "player-next", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );

	g_signal_new ( "player-video", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_BOOLEAN, 0 );

	g_signal_new ( "player-is-play", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_BOOLEAN, 1, G_TYPE_BOOLEAN );

	g_signal_new ( "player-slider", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 5, G_TYPE_INT64, G_TYPE_UINT, G_TYPE_INT64, G_TYPE_UINT, G_TYPE_BOOLEAN );

	g_signal_new ( "player-seek", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_DOUBLE );

	g_signal_new ( "player-scrool", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN );

	g_signal_new ( "player-staus", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING );

	g_signal_new ( "player-eqa", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_DOUBLE );

	g_signal_new ( "player-eqv", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_DOUBLE );

	g_signal_new ( "player-message-dialog", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );

	g_signal_new ( "player-element", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_OBJECT, 0 );

	g_signal_new ( "player-enc", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_POINTER );

	g_signal_new ( "player-power-set", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN );

	oclass->get_property = player_get_property;
	oclass->set_property = player_set_property;

	g_object_class_install_property ( oclass, PROP_MUTE,   g_param_spec_boolean ( "mute",   NULL, NULL,      FALSE, G_PARAM_READWRITE ) );
	g_object_class_install_property ( oclass, PROP_VOLUME, g_param_spec_double  ( "volume", NULL, NULL, 0, 1, 0.75, G_PARAM_READWRITE ) );
}

Player * player_new ( void )
{
	return g_object_new ( PLAYER_TYPE_OBJECT, NULL );
}
