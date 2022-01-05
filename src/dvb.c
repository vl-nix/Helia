/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "dvb.h"
#include "file.h"
#include "scan.h"
#include "info.h"
#include "default.h"
#include "settings.h"
#include "enc-prop.h"
#include "helia-eqa.h"
#include "helia-eqv.h"

#include <gst/video/videooverlay.h>

enum prop_dvb
{
	PROP_0,
	PROP_MUTE,
	PROP_VOLUME,
	N_PROPS
};

struct _Dvb
{
	GstObject parent_instance;

	GstElement *playdvb;
	GstElement *dvbsrc;
	GstElement *demux;
	GstElement *volume;
	GstElement *videoblnc;
	GstElement *equalizer;

	GstElement *enc_video;
	GstElement *enc_audio;
	GstElement *enc_muxer;

	ulong xid;
	uint16_t sid;

	uint bitrate_a;
	uint bitrate_v;

	char *data;

	gboolean rec_tv;
	gboolean checked_video;
};

G_DEFINE_TYPE ( Dvb, dvb, GST_TYPE_OBJECT )

typedef struct _DvbSet DvbSet;

struct _DvbSet
{
	GstElement *dvbsrc;
	GstElement *demux;
	GstElement *volume;
	GstElement *videoblnc;
	GstElement *equalizer;
};

static void dvb_set_stop ( Dvb *dvb )
{
	dvb->bitrate_a = 0;
	dvb->bitrate_v = 0;

	gst_element_set_state ( dvb->playdvb, GST_STATE_NULL );
}

static gboolean dvb_pad_check_type ( GstPad *pad, const char *type )
{
	gboolean ret = FALSE;

	GstCaps *caps = gst_pad_get_current_caps ( pad );

	const char *name = gst_structure_get_name ( gst_caps_get_structure ( caps, 0 ) );

	if ( g_str_has_prefix ( name, type ) ) ret = TRUE;

	gst_caps_unref (caps);

	return ret;
}

static void dvb_pad_link ( GstPad *pad, GstElement *element, const char *name )
{
	GstPad *pad_va_sink = gst_element_get_static_pad ( element, "sink" );

	if ( gst_pad_link ( pad, pad_va_sink ) == GST_PAD_LINK_OK )
		gst_object_unref ( pad_va_sink );
	else
		g_debug ( "%s:: linking demux/decode name %s video/audio pad failed ", __func__, name );
}

static void dvb_pad_demux_audio ( G_GNUC_UNUSED GstElement *element, GstPad *pad, GstElement *element_audio )
{
	if ( dvb_pad_check_type ( pad, "audio" ) ) dvb_pad_link ( pad, element_audio, "demux audio" );
}

static void dvb_pad_demux_video ( G_GNUC_UNUSED GstElement *element, GstPad *pad, GstElement *element_video )
{
	if ( dvb_pad_check_type ( pad, "video" ) ) dvb_pad_link ( pad, element_video, "demux video" );
}

static void dvb_pad_decode ( G_GNUC_UNUSED GstElement *element, GstPad *pad, GstElement *element_va )
{
	dvb_pad_link ( pad, element_va, "decode  audio / video" );
}

static DvbSet dvb_create_bin ( GstElement *element, gboolean video_enable )
{
	struct dvb_all_list { const char *name; } dvb_all_list_n[] =
	{
		{ "dvbsrc" }, { "tsdemux"   },
		{ "queue2" }, { "decodebin" }, { "audioconvert" }, { "equalizer-nbands" }, { "volume" }, { "autoaudiosink" },
		{ "queue2" }, { "decodebin" }, { "videoconvert" }, { "videobalance"     }, { "autovideosink" }
	};

	DvbSet dvbset;

	GstElement *elements[ G_N_ELEMENTS ( dvb_all_list_n ) ];

	uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( dvb_all_list_n ); c++ )
	{
		if ( !video_enable && c > 7 ) continue;

		elements[c] = gst_element_factory_make ( dvb_all_list_n[c].name, NULL );

		if ( !elements[c] )
			g_critical ( "%s:: element (factory make) - %s not created. \n", __func__, dvb_all_list_n[c].name );

		if ( c == 2 ) gst_element_set_name ( elements[c], "queue-tee-audio" );

		gst_bin_add ( GST_BIN ( element ), elements[c] );

		if (  c == 0 || c == 2 || c == 4 || c == 8 || c == 10 ) continue;

		gst_element_link ( elements[c-1], elements[c] );
	}

	g_signal_connect ( elements[1], "pad-added", G_CALLBACK ( dvb_pad_demux_audio ), elements[2] );
	if ( video_enable ) g_signal_connect ( elements[1], "pad-added", G_CALLBACK ( dvb_pad_demux_video ), elements[8] );

	g_signal_connect ( elements[3], "pad-added", G_CALLBACK ( dvb_pad_decode ), elements[4] );
	if ( video_enable ) g_signal_connect ( elements[9], "pad-added", G_CALLBACK ( dvb_pad_decode ), elements[10] );

	g_object_set ( elements[6], "volume", VOLUME, NULL );

	dvbset.dvbsrc = elements[0];
	dvbset.demux  = elements[1];
	dvbset.volume = elements[6];
	dvbset.equalizer = elements[5];
	dvbset.videoblnc = elements[11];

	return dvbset;
}

static void dvb_remove_bin ( GstElement *pipeline, const char *name )
{
	GstIterator *it = gst_bin_iterate_elements ( GST_BIN ( pipeline ) );
	GValue item = { 0, };
	gboolean done = FALSE;

	while ( !done )
	{
		switch ( gst_iterator_next ( it, &item ) )
		{
			case GST_ITERATOR_OK:
			{
				GstElement *element = GST_ELEMENT ( g_value_get_object (&item) );

				char *object_name = gst_object_get_name ( GST_OBJECT ( element ) );

				if ( name && g_strrstr ( object_name, name ) )
				{
					g_debug ( "%s:: Object Not remove: %s \n", __func__, object_name );
				}
				else
				{
					gst_element_set_state ( element, GST_STATE_NULL );
					gst_bin_remove ( GST_BIN ( pipeline ), element );

					g_debug ( "%s:: Object remove: %s \n", __func__, object_name );
				}

				g_free ( object_name );
				g_value_reset (&item);

				break;
			}

			case GST_ITERATOR_RESYNC:
				gst_iterator_resync (it);
				break;

			case GST_ITERATOR_ERROR:
				done = TRUE;
				break;

			case GST_ITERATOR_DONE:
				done = TRUE;
				break;
		}
	}

	g_value_unset ( &item );
	gst_iterator_free ( it );
}

static GstElement * dvb_iterate_element ( GstElement *it_e, const char *name1, const char *name2 )
{
	GstIterator *it = gst_bin_iterate_recurse ( GST_BIN ( it_e ) );

	GValue item = { 0, };
	gboolean done = FALSE;

	GstElement *element_ret = NULL;

	while ( !done )
	{
		switch ( gst_iterator_next ( it, &item ) )
		{
			case GST_ITERATOR_OK:
			{
				GstElement *element = GST_ELEMENT ( g_value_get_object (&item) );

				char *object_name = gst_object_get_name ( GST_OBJECT ( element ) );

				if ( g_strrstr ( object_name, name1 ) )
				{
					if ( name2 && g_strrstr ( object_name, name2 ) )
						element_ret = element;
					else
						element_ret = element;
				}

				g_free ( object_name );
				g_value_reset (&item);

				break;
			}

			case GST_ITERATOR_RESYNC:
				gst_iterator_resync (it);
				break;

			case GST_ITERATOR_ERROR:
				done = TRUE;
				break;

			case GST_ITERATOR_DONE:
				done = TRUE;
				break;
		}
	}

	g_value_unset ( &item );
	gst_iterator_free ( it );

	return element_ret;
}

static GstElementFactory * dvb_find_factory ( GstCaps *caps, guint64 num )
{
	GList *list, *list_filter;

	static GMutex mutex;

	g_mutex_lock ( &mutex );
		list = gst_element_factory_list_get_elements ( num, GST_RANK_MARGINAL );
		list_filter = gst_element_factory_list_filter ( list, caps, GST_PAD_SINK, gst_caps_is_fixed ( caps ) );
	g_mutex_unlock ( &mutex );

	GstElementFactory *factory = GST_ELEMENT_FACTORY_CAST ( list_filter->data );

	gst_plugin_feature_list_free ( list_filter );
	gst_plugin_feature_list_free ( list );

	return factory;
}

static gboolean dvb_typefind_remove ( const char *name, Dvb *dvb )
{
	GstElement *element = dvb_iterate_element ( dvb->playdvb, name, NULL );

	if ( element )
	{
		gst_element_set_state ( element, GST_STATE_NULL );
		gst_bin_remove ( GST_BIN ( dvb->playdvb ), element );

		return TRUE;
	}

	return FALSE;
}

static void dvb_typefind_parser ( GstElement *typefind, G_GNUC_UNUSED uint probability, GstCaps *caps, Dvb *dvb )
{
	const char *name_caps = gst_structure_get_name ( gst_caps_get_structure ( caps, 0 ) );

	GstElementFactory *factory = dvb_find_factory ( caps, GST_ELEMENT_FACTORY_TYPE_PARSER );

	GstElement *mpegtsmux = dvb_iterate_element ( dvb->playdvb, "mpegtsmux", NULL );

	GstElement *element = gst_element_factory_create ( factory, NULL );

	gboolean remove = FALSE;

	if ( g_str_has_prefix ( name_caps, "audio" ) )
	{
		remove = dvb_typefind_remove ( "parser-audio", dvb );
		gst_element_set_name ( element, "parser-audio" );
	}

	if ( g_str_has_prefix ( name_caps, "video" ) )
	{
		remove = dvb_typefind_remove ( "parser-video", dvb );
		gst_element_set_name ( element, "parser-video" );
	}

	if ( remove == FALSE ) gst_element_unlink ( typefind, mpegtsmux );

	gst_bin_add ( GST_BIN ( dvb->playdvb ), element );

	gst_element_link ( typefind, element );
	gst_element_link ( element, mpegtsmux );

	gst_element_set_state ( element, GST_STATE_PLAYING );
}

static DvbSet dvb_create_rec_bin ( GstElement *element, gboolean video, Dvb *dvb )
{
	struct dvb_all_list { const char *name; } dvb_all_list_n[] =
	{
		{ "tsdemux" },
		{ "tee"     }, { "queue2"     }, { "decodebin" }, { "audioconvert" }, { "equalizer-nbands" }, { "volume" }, { "autoaudiosink" },
		{ "tee"     }, { "queue2"     }, { "decodebin" }, { "videoconvert" }, { "videobalance"     }, { "autovideosink" },
		{ "queue2"  }, { "typefind"   },
		{ "queue2"  }, { "typefind"   },
		{ "mpegtsmux" }, { "filesink" }
	};

	DvbSet dvbset;

	GstElement *elements[ G_N_ELEMENTS ( dvb_all_list_n ) ];

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( dvb_all_list_n ); c++ )
	{
		if ( !video && ( c > 7 && c < 14 ) ) continue;
		if ( !video && ( c == 16 || c == 17 ) ) continue;

		elements[c] = gst_element_factory_make ( dvb_all_list_n[c].name, NULL );

		if ( !elements[c] )
			g_critical ( "%s:: element (factory make) - %s not created. \n", __func__, dvb_all_list_n[c].name );

		if ( c == 1 ) gst_element_set_name ( elements[c], "queue-tee-audio" );

		gst_bin_add ( GST_BIN ( element ), elements[c] );

		if ( c > 13 ) continue;

		if (  c == 0 || c == 1 || c == 4 || c == 8 || c == 11 ) continue;

		gst_element_link ( elements[c-1], elements[c] );
	}

	g_signal_connect ( elements[0], "pad-added", G_CALLBACK ( dvb_pad_demux_audio ), elements[1] );
	if ( video ) g_signal_connect ( elements[0], "pad-added", G_CALLBACK ( dvb_pad_demux_video ), elements[8] );

	g_signal_connect ( elements[3], "pad-added", G_CALLBACK ( dvb_pad_decode ), elements[4] );
	if ( video ) g_signal_connect ( elements[10], "pad-added", G_CALLBACK ( dvb_pad_decode ), elements[11] );

	gst_element_link_many ( elements[1], elements[14], elements[15], NULL );
	if ( video ) gst_element_link_many ( elements[8], elements[16], elements[17], NULL );

	gst_element_link ( elements[15], elements[18] );
	if ( video ) gst_element_link ( elements[17], elements[18] );

	gst_element_link ( elements[18], elements[19] );

	g_signal_connect ( elements[15], "have-type", G_CALLBACK ( dvb_typefind_parser ), dvb );
	if ( video ) g_signal_connect ( elements[17], "have-type", G_CALLBACK ( dvb_typefind_parser ), dvb );

	g_autofree char *dt = helia_time_to_str ();
	char **lines = g_strsplit ( dvb->data, ":", 0 );

	g_autofree char *rec_dir = NULL;
	GSettings *setting = settings_init ();
	if ( setting ) rec_dir = g_settings_get_string ( setting, "rec-dir" );

	char path[PATH_MAX];

	if ( setting && rec_dir && !g_str_has_prefix ( rec_dir, "none" ) )
		sprintf ( path, "%s/%s-%s.m2ts", rec_dir, lines[0], dt );
	else
		sprintf ( path, "%s/%s-%s.m2ts", g_get_home_dir (), lines[0], dt );

	g_object_set ( elements[19], "location", path, NULL );

	g_strfreev ( lines );
	if ( setting ) g_object_unref ( setting );

	dvbset.demux  = elements[0];
	dvbset.volume = elements[6];
	dvbset.equalizer = elements[5];
	dvbset.videoblnc = elements[12];

	return dvbset;
}

static DvbSet dvb_create_rec_enc_bin ( GstElement *element, gboolean video, Dvb *dvb )
{
	struct dvb_all_list { const char *name; } dvb_all_list_n[] =
	{
		{ "tsdemux" },
		{ "queue2"  }, { "decodebin" }, { "audioconvert" }, { "tee" }, { "queue2" }, { "equalizer-nbands" }, { "volume" }, { "autoaudiosink" },
		{ "queue2"  }, { "decodebin" }, { "videoconvert" }, { "tee" }, { "queue2" }, { "videobalance"     }, { "autovideosink" }
	};

	DvbSet dvbset;
	GstElement *elements[ G_N_ELEMENTS ( dvb_all_list_n ) ];

	uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( dvb_all_list_n ); c++ )
	{
		if ( !video && ( c > 8 ) ) continue;

		elements[c] = gst_element_factory_make ( dvb_all_list_n[c].name, NULL );

		if ( !elements[c] )
			g_critical ( "%s:: element (factory make) - %s not created. \n", __func__, dvb_all_list_n[c].name );

		if ( c == 1 ) gst_element_set_name ( elements[c], "queue-tee-audio" );

		gst_bin_add ( GST_BIN ( element ), elements[c] );

		if (  c == 0 || c == 1 || c == 3 || c == 9 || c == 11 ) continue;

		gst_element_link ( elements[c-1], elements[c] );
	}

	g_signal_connect ( elements[0], "pad-added", G_CALLBACK ( dvb_pad_demux_audio ), elements[1] );
	if ( video ) g_signal_connect ( elements[0], "pad-added", G_CALLBACK ( dvb_pad_demux_video ), elements[9] );

	g_signal_connect ( elements[2], "pad-added", G_CALLBACK ( dvb_pad_decode ), elements[3] );
	if ( video ) g_signal_connect ( elements[10], "pad-added", G_CALLBACK ( dvb_pad_decode ), elements[11] );

	GstElement *enc_video = g_object_ref ( dvb->enc_video ), *enc_audio = g_object_ref ( dvb->enc_audio ), *enc_muxer = g_object_ref ( dvb->enc_muxer );

	GstElement *filesink = gst_element_factory_make ( "filesink", NULL );
	GstElement *queue2_a = gst_element_factory_make ( "queue2",   NULL );

	gst_bin_add_many ( GST_BIN ( element ), queue2_a, enc_audio, enc_muxer, filesink, NULL );
	gst_element_link_many ( elements[4], queue2_a, enc_audio, enc_muxer, NULL );

	gst_element_link ( enc_muxer, filesink );

	if ( video )
	{
		GstElement *queue2_v = gst_element_factory_make ( "queue2", NULL );

		gst_bin_add_many ( GST_BIN ( element ), queue2_v, enc_video, NULL );
		gst_element_link_many ( elements[12], queue2_v, enc_video, enc_muxer, NULL );
	}

	g_autofree char *dt = helia_time_to_str ();
	char **lines = g_strsplit ( dvb->data, ":", 0 );

	g_autofree char *rec_dir = NULL;
	GSettings *setting = settings_init ();
	if ( setting ) rec_dir =  g_settings_get_string ( setting, "rec-dir" );

	char path[PATH_MAX];

	if ( setting && rec_dir && !g_str_has_prefix ( rec_dir, "none" ) )
		sprintf ( path, "%s/%s-%s", rec_dir, lines[0], dt );
	else
		sprintf ( path, "%s/%s-%s", g_get_home_dir (), lines[0], dt );

	g_object_set ( filesink, "location", path, NULL );

	g_strfreev ( lines );
	if ( setting ) g_object_unref ( setting );

	dvbset.demux  = elements[0];
	dvbset.volume = elements[7];
	dvbset.equalizer = elements[6];
	dvbset.videoblnc = elements[13];

	return dvbset;
}

static GstPadProbeReturn dvb_blockpad_probe ( GstPad *pad, GstPadProbeInfo *info, gpointer data )
{
	Dvb *dvb = (Dvb *) data;

	GSettings *setting = settings_init ();

	gboolean enc_b = FALSE;
	if ( setting ) enc_b = g_settings_get_boolean ( setting, "encoding-tv" );

	if ( setting ) g_object_unref ( setting );

	double value = VOLUME;
	g_object_get ( dvb->volume, "volume", &value, NULL );

	gst_element_set_state ( dvb->playdvb, GST_STATE_PAUSED );
	dvb_remove_bin ( dvb->playdvb, "dvbsrc" );

	DvbSet dvbset;

	if ( enc_b )
		dvbset = dvb_create_rec_enc_bin ( dvb->playdvb, dvb->checked_video, dvb );
	else
		dvbset = dvb_create_rec_bin ( dvb->playdvb, dvb->checked_video, dvb );

	dvb->demux  = dvbset.demux;
	dvb->volume = dvbset.volume;
	dvb->equalizer = dvbset.equalizer;
	dvb->videoblnc = dvbset.videoblnc;

	gst_element_link ( dvb->dvbsrc, dvbset.demux );

	g_object_set ( dvb->volume, "volume", value, NULL );
	g_object_set ( dvbset.demux, "program-number", dvb->sid, NULL );

	gst_pad_remove_probe ( pad, GST_PAD_PROBE_INFO_ID (info) );
	gst_element_set_state ( dvb->playdvb, GST_STATE_PLAYING );

	return GST_PAD_PROBE_OK;
}

static void dvb_combo_append_text ( GtkComboBoxText *combo, char *name, uint8_t num )
{
	if ( g_strrstr ( name, "_0_" ) )
	{
		char **data = g_strsplit ( name, "_0_", 0 );

		uint32_t pid = 0;
		sscanf ( data[1], "%4x", &pid );

		char buf[100];
		sprintf ( buf, "%d  ( 0x%.4X )", pid, pid );

		gtk_combo_box_text_append_text ( combo, buf );

		g_strfreev ( data );
	}
	else
	{
		char buf[100];
		sprintf ( buf, "%.4d", num + 1 );

		gtk_combo_box_text_append_text ( combo, buf );
	}
}

static uint8_t dvb_add_audio_track ( GtkComboBoxText *combo, GstElement *element )
{
	GstIterator *it = gst_element_iterate_src_pads ( element );
	GValue item = { 0, };

	uint8_t i = 0, num = 0;
	gboolean done = FALSE;

	while ( !done )
	{
		switch ( gst_iterator_next ( it, &item ) )
		{
			case GST_ITERATOR_OK:
			{
				GstPad *pad_src = GST_PAD ( g_value_get_object (&item) );

				char *name = gst_object_get_name ( GST_OBJECT ( pad_src ) );

				if ( g_str_has_prefix ( name, "audio" ) )
				{
					if ( gst_pad_is_linked ( pad_src ) ) num = i;

					dvb_combo_append_text ( combo, name, i );
					i++;
				}

				free ( name );
				g_value_reset (&item);

				break;
			}

			case GST_ITERATOR_RESYNC:
				gst_iterator_resync (it);
				break;

			case GST_ITERATOR_ERROR:
				done = TRUE;
				break;

			case GST_ITERATOR_DONE:
				done = TRUE;
				break;
		}
	}

	g_value_unset ( &item );
	gst_iterator_free ( it );

	return num;
}

static void dvb_change_audio_track ( GstElement *e_unlink, GstElement *e_link, uint8_t num )
{
	GstIterator *it = gst_element_iterate_src_pads ( e_unlink );
	GValue item = { 0, };

	uint8_t i = 0;
	gboolean done = FALSE;

	GstPad *pad_link = NULL;
	GstPad *pad_sink = gst_element_get_static_pad ( e_link, "sink" );

	while ( !done )
	{
		switch ( gst_iterator_next ( it, &item ) )
		{
			case GST_ITERATOR_OK:
			{
				GstPad *pad_src = GST_PAD ( g_value_get_object (&item) );

				char *name = gst_object_get_name ( GST_OBJECT ( pad_src ) );

				if ( g_str_has_prefix ( name, "audio" ) )
				{
					if ( gst_pad_is_linked ( pad_src ) )
					{
						if ( gst_pad_unlink ( pad_src, pad_sink ) )
							g_debug ( "%s: unlink Ok ", __func__ );
						else
							g_warning ( "%s: unlink Failed ", __func__ );
					}
					else
						if ( i == num ) pad_link = pad_src;

					i++;
				}

				free ( name );
				g_value_reset (&item);

				break;
			}

			case GST_ITERATOR_RESYNC:
				gst_iterator_resync (it);
				break;

			case GST_ITERATOR_ERROR:
				done = TRUE;
				break;

			case GST_ITERATOR_DONE:
				done = TRUE;
				break;
		}
	}

	if ( gst_pad_link ( pad_link, pad_sink ) == GST_PAD_LINK_OK )
		g_debug ( "%s: link Ok ", __func__ );
	else
		g_warning ( "%s: link Failed ", __func__ );

	gst_object_unref ( pad_sink );

	g_value_unset ( &item );
	gst_iterator_free ( it );
}

static void dvb_combo_lang_changed ( GtkComboBox *combo, Dvb *dvb )
{
	int num = gtk_combo_box_get_active ( GTK_COMBO_BOX ( combo ) );

	GstElement *e_link = dvb_iterate_element ( dvb->playdvb, "queue-tee-audio", NULL );

	if ( e_link ) dvb_change_audio_track ( dvb->demux, e_link, (uint8_t)num );
}

static void dvb_run_info ( GtkWindow *window, Dvb *dvb )
{
	GtkComboBoxText *combo_lang = helia_info_dvb ( dvb->data, window, dvb->dvbsrc );

	if ( combo_lang )
	{
		uint8_t num = dvb_add_audio_track ( combo_lang, dvb->demux );

		gtk_combo_box_set_active ( GTK_COMBO_BOX ( combo_lang ), num );
		g_signal_connect ( combo_lang, "changed", G_CALLBACK ( dvb_combo_lang_changed ), dvb );
	}
}

static void dvb_set_tuning_timeout ( GstElement *element )
{
	guint64 timeout = 0;
	g_object_get ( element, "tuning-timeout", &timeout, NULL );
	g_object_set ( element, "tuning-timeout", (guint64)timeout / 4, NULL );
}

static void dvb_rinit ( GstElement *element )
{
	int adapter = 0, frontend = 0;
	g_object_get ( element, "adapter",  &adapter,  NULL );
	g_object_get ( element, "frontend", &frontend, NULL );

	helia_init_dvb ( adapter, frontend );
}

static void dvb_data_set ( const char *data, GstElement *element, GstElement *demux, Dvb *dvb )
{
	dvb_set_tuning_timeout ( element );

	char **fields = g_strsplit ( data, ":", 0 );
	uint j = 0, numfields = g_strv_length ( fields );

	for ( j = 1; j < numfields; j++ )
	{
		if ( g_strrstr ( fields[j], "audio-pid" ) || g_strrstr ( fields[j], "video-pid" ) ) continue;

		if ( !g_strrstr ( fields[j], "=" ) ) continue;

		char **splits = g_strsplit ( fields[j], "=", 0 );

		g_debug ( "%s: gst-param %s | gst-value %s ", __func__, splits[0], splits[1] );

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
			dvb->sid = (uint16_t)dat;
			g_object_set ( demux, "program-number", dat, NULL );
		}
		else if ( g_strrstr ( splits[0], "symbol-rate" ) )
		{
			g_object_set ( element, "symbol-rate", ( dat > 100000) ? dat / 1000 : dat, NULL );
		}
		else if ( g_strrstr ( splits[0], "lnb-type" ) )
		{
			set_lnb_lhs ( element, (int)dat );
		}
		else
		{
			g_object_set ( element, splits[0], dat, NULL );
		}

		g_strfreev (splits);
	}

	g_strfreev (fields);

	dvb_rinit ( element );
}

static gboolean dvb_checked_video ( const char *data )
{
	gboolean video_enable = TRUE;

	if ( !g_strrstr ( data, "video-pid" ) || g_strrstr ( data, "video-pid=0" ) ) video_enable = FALSE;

	return video_enable;
}

static void dvb_stop_set_play ( const char *data, ulong xid, Dvb *dvb )
{
	dvb->xid = xid;

	free ( dvb->data );
	dvb->data = g_strdup ( data );

	double value = VOLUME;
	if ( dvb->volume ) g_object_get ( dvb->volume, "volume", &value, NULL );

	dvb->rec_tv = FALSE;
	dvb_set_stop ( dvb );

	dvb_remove_bin ( dvb->playdvb, NULL );

	dvb->checked_video = dvb_checked_video ( data );

	DvbSet dvbset;
	dvbset = dvb_create_bin ( dvb->playdvb, dvb->checked_video );

	dvb->dvbsrc = dvbset.dvbsrc;
	dvb->demux  = dvbset.demux;
	dvb->equalizer = dvbset.equalizer;
	dvb->videoblnc = dvbset.videoblnc;

	dvb->volume = dvbset.volume;
	dvb_data_set ( data, dvbset.dvbsrc, dvbset.demux, dvb );

	g_object_set ( dvb->volume, "mute",   FALSE, NULL );
	g_object_set ( dvb->volume, "volume", value, NULL );

	gst_element_set_state ( dvb->playdvb, GST_STATE_PLAYING );
}

static void dvb_record ( Dvb *dvb )
{
	if ( dvb->rec_tv )
	{
		dvb->rec_tv = FALSE;

		g_autofree char *data = g_strdup ( dvb->data );
		dvb_stop_set_play ( data, dvb->xid, dvb );
	}
	else
	{
		GstPad *blockpad = gst_element_get_static_pad ( dvb->dvbsrc, "src" );

		gst_pad_add_probe ( blockpad, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM, dvb_blockpad_probe, dvb, NULL );

		gst_object_unref ( blockpad );
		dvb->rec_tv = TRUE;
	}
}

static GstBusSyncReply dvb_sync_handler ( G_GNUC_UNUSED GstBus *bus, GstMessage *message, Dvb *dvb )
{
	if ( !gst_is_video_overlay_prepare_window_handle_message ( message ) ) return GST_BUS_PASS;

	if ( dvb->xid != 0 )
	{
		GstVideoOverlay *xoverlay = GST_VIDEO_OVERLAY ( GST_MESSAGE_SRC ( message ) );
		gst_video_overlay_set_window_handle ( xoverlay, dvb->xid );

	} else { g_warning ( "Should have obtained window_handle by now!" ); }

	gst_message_unref ( message );

	return GST_BUS_DROP;
}

static void dvb_msg_cng ( G_GNUC_UNUSED GstBus *bus, G_GNUC_UNUSED GstMessage *msg, Dvb *dvb )
{
	if ( GST_MESSAGE_SRC ( msg ) != GST_OBJECT ( dvb->playdvb ) ) return;

	GstState old_state, new_state;
	gst_message_parse_state_changed ( msg, &old_state, &new_state, NULL );

	switch ( new_state )
	{
		case GST_STATE_NULL:
		case GST_STATE_READY:
			break;

		case GST_STATE_PAUSED:
		{
			g_signal_emit_by_name ( dvb, "dvb-power-set", FALSE );
			break;
		}

		case GST_STATE_PLAYING:
		{
			if ( dvb->checked_video ) g_signal_emit_by_name ( dvb, "dvb-power-set", TRUE );
			break;
		}

		default:
			break;
	}
}

static void dvb_msg_all ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Dvb *dvb )
{
	const GstStructure *structure = gst_message_get_structure ( msg );

	if ( structure )
	{
		switch ( GST_MESSAGE_TYPE ( msg ) )
		{
			case GST_MESSAGE_TAG:
			{
				GstTagList *tags = NULL;
				gst_message_parse_tag ( msg, &tags );

				if ( tags )
				{
					char *check_a = NULL;

					if ( gst_tag_list_get_string ( tags, "audio-codec", &check_a ) )
					{
						uint bitrate_a = 0;
						if ( !gst_tag_list_get_uint ( tags, "nominal-bitrate", &bitrate_a ) )
							gst_tag_list_get_uint ( tags, "bitrate", &bitrate_a );

						dvb->bitrate_a = bitrate_a / 1000;
					}

					free ( check_a );

					char *check_v = NULL;

					if ( gst_tag_list_get_string ( tags, "video-codec", &check_v ) )
					{
						uint bitrate_v = 0;
						gst_tag_list_get_uint ( tags, "bitrate", &bitrate_v );

						dvb->bitrate_v = bitrate_v / 1000;
					}

					free ( check_v );
				}

				gst_tag_list_unref ( tags );
				break;
			}

			default:
				break;
		}

		int signal = 0, snr = 0;
		gboolean lock = FALSE;

		if (  gst_structure_get_int ( structure, "signal", &signal )  )
		{
			gst_structure_get_int     ( structure, "snr",  &snr  );
			gst_structure_get_boolean ( structure, "lock", &lock );

			uint8_t ret_sgl = (uint8_t)(signal*100/0xffff);
			uint8_t ret_snr = (uint8_t)(snr*100/0xffff);

			g_signal_emit_by_name ( dvb, "dvb-level", ret_sgl, ret_snr, lock, dvb->rec_tv, dvb->bitrate_a, dvb->bitrate_v );
		}
	}
}

static void dvb_msg_err ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Dvb *dvb )
{
	GError *err = NULL;
	char   *dbg = NULL;

	gst_message_parse_error ( msg, &err, &dbg );

	g_critical ( "%s: %s (%s)", __func__, err->message, (dbg) ? dbg : "no details" );

	g_signal_emit_by_name ( dvb, "dvb-message-dialog", err->message );

	g_error_free ( err );
	g_free ( dbg );

	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state != GST_STATE_PLAYING )
		dvb_set_stop ( dvb );
}

static void dvb_enc_create_elements ( Dvb *dvb )
{
	GSettings *setting = settings_init ();

	g_autofree char *enc_audio = NULL;
	g_autofree char *enc_video = NULL;
	g_autofree char *enc_muxer = NULL;

	if ( setting ) enc_audio = g_settings_get_string ( setting, "encoder-audio" );
	if ( setting ) enc_video = g_settings_get_string ( setting, "encoder-video" );
	if ( setting ) enc_muxer = g_settings_get_string ( setting, "encoder-muxer" );

	dvb->enc_audio = gst_element_factory_make ( ( enc_audio ) ? enc_audio : "vorbisenc", NULL );
	dvb->enc_video = gst_element_factory_make ( ( enc_video ) ? enc_video : "theoraenc", NULL );
	dvb->enc_muxer = gst_element_factory_make ( ( enc_muxer ) ? enc_muxer : "oggmux",    NULL );

	if ( setting ) g_object_unref ( setting );
}

static GstElement * dvb_create ( Dvb *dvb )
{
	GstElement *dvbplay = gst_pipeline_new ( "pipeline" );

	if ( !dvbplay )
	{
		g_critical ( "%s: dvbplay - not created.", __func__ );
		return NULL;
	}

	GstBus *bus = gst_element_get_bus ( dvbplay );

	gst_bus_add_signal_watch_full ( bus, G_PRIORITY_DEFAULT );
	gst_bus_set_sync_handler ( bus, (GstBusSyncHandler)dvb_sync_handler, dvb, NULL );

	g_signal_connect ( bus, "message",        G_CALLBACK ( dvb_msg_all ), dvb );
	g_signal_connect ( bus, "message::error", G_CALLBACK ( dvb_msg_err ), dvb );
	g_signal_connect ( bus, "message::state-changed", G_CALLBACK ( dvb_msg_cng ), dvb );

	gst_object_unref (bus);

	dvb_enc_create_elements ( dvb );

	return dvbplay;
}

static void dvb_handler_stop ( Dvb *dvb )
{
	dvb_set_stop ( dvb );
}

static void dvb_handler_rec ( Dvb *dvb )
{
	dvb_record ( dvb );
}

static void dvb_handler_eqa ( Dvb *dvb, GObject *obj, double opacity )
{
	GtkWindow *window = GTK_WINDOW ( obj );

	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state == GST_STATE_PLAYING )
		helia_eqa_win ( opacity, window, dvb->equalizer );
}

static void dvb_handler_eqv ( Dvb *dvb, GObject *obj, double opacity )
{
	GtkWindow *window = GTK_WINDOW ( obj );

	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state == GST_STATE_PLAYING )
		helia_eqv_win ( opacity, window, dvb->videoblnc );
}

static void dvb_handler_info ( Dvb *dvb, GObject *obj )
{
	GtkWindow *window = GTK_WINDOW ( obj );

	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state == GST_STATE_PLAYING )
		dvb_run_info ( window, dvb );
}

static void dvb_handler_play ( Dvb *dvb, ulong xid, const char *data )
{
	dvb_stop_set_play ( data, xid, dvb );
}

static gboolean dvb_handler_video ( Dvb *dvb )
{
	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state < GST_STATE_PLAYING || !dvb->checked_video ) return TRUE;

	return FALSE;
}

static gboolean dvb_handler_isplay ( Dvb *dvb )
{
	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state == GST_STATE_PLAYING ) return TRUE;

	return FALSE;
}

static void dvb_enc_prop_set_audio_handler ( G_GNUC_UNUSED EncProp *ep, GObject *object, Dvb *dvb )
{
	gst_object_unref ( dvb->enc_audio );

	dvb->enc_audio = GST_ELEMENT ( object );
}
static void dvb_enc_prop_set_video_handler ( G_GNUC_UNUSED EncProp *ep, GObject *object, Dvb *dvb )
{
	gst_object_unref ( dvb->enc_video );

	dvb->enc_video = GST_ELEMENT ( object );
}
static void dvb_enc_prop_set_muxer_handler ( G_GNUC_UNUSED EncProp *ep, GObject *object, Dvb *dvb )
{
	gst_object_unref ( dvb->enc_muxer );

	dvb->enc_muxer = GST_ELEMENT ( object );
}

static void dvb_handler_enc ( Dvb *dvb, GstObject *obj, gpointer data )
{
	EncProp *ep = data;
	GtkWindow *window = GTK_WINDOW ( obj );

	enc_prop_set_run ( window, dvb->playdvb, dvb->enc_video, dvb->enc_audio, dvb->enc_muxer, FALSE, ep );

	g_signal_connect ( ep, "enc-prop-set-audio", G_CALLBACK ( dvb_enc_prop_set_audio_handler ), dvb );
	g_signal_connect ( ep, "enc-prop-set-video", G_CALLBACK ( dvb_enc_prop_set_video_handler ), dvb );
	g_signal_connect ( ep, "enc-prop-set-muxer", G_CALLBACK ( dvb_enc_prop_set_muxer_handler ), dvb );
}

static void dvb_init ( Dvb *dvb )
{
	dvb->enc_video = NULL;
	dvb->enc_audio = NULL;
	dvb->enc_muxer = NULL;

	dvb->data   = NULL;
	dvb->volume = NULL;

	dvb->bitrate_a = 0;
	dvb->bitrate_v = 0;

	dvb->playdvb = dvb_create ( dvb );

	g_signal_connect ( dvb, "dvb-rec",     G_CALLBACK ( dvb_handler_rec    ), NULL );
	g_signal_connect ( dvb, "dvb-eqa",     G_CALLBACK ( dvb_handler_eqa    ), NULL );
	g_signal_connect ( dvb, "dvb-eqv",     G_CALLBACK ( dvb_handler_eqv    ), NULL );
	g_signal_connect ( dvb, "dvb-info",    G_CALLBACK ( dvb_handler_info   ), NULL );
	g_signal_connect ( dvb, "dvb-play",    G_CALLBACK ( dvb_handler_play   ), NULL );
	g_signal_connect ( dvb, "dvb-stop",    G_CALLBACK ( dvb_handler_stop   ), NULL );
	g_signal_connect ( dvb, "dvb-video",   G_CALLBACK ( dvb_handler_video  ), NULL );
	g_signal_connect ( dvb, "dvb-is-play", G_CALLBACK ( dvb_handler_isplay ), NULL );
	g_signal_connect ( dvb, "dvb-enc",     G_CALLBACK ( dvb_handler_enc    ), NULL );
}

static void dvb_get_property ( GObject *object, uint id, GValue *value, GParamSpec *pspec )
{
	Dvb *dvb = DVB_OBJECT ( object );

	gboolean play = FALSE;
	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state == GST_STATE_PLAYING ) play = TRUE;

	double volume = 0;
	gboolean mute = FALSE;

	if ( play ) g_object_get ( dvb->volume, "volume", &volume, NULL );
	if ( play ) g_object_get ( dvb->volume, "mute",   &mute,   NULL );

	switch ( id )
	{
		case PROP_VOLUME:
			if ( play ) g_value_set_double ( value, volume );
			break;

		case PROP_MUTE:
			if ( play ) g_value_set_boolean ( value, mute );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, id, pspec );
			break;
	}
}

static void dvb_set_property ( GObject *object, uint id, const GValue *value, GParamSpec *pspec )
{
	Dvb *dvb = DVB_OBJECT ( object );

	gboolean play = FALSE;
	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state == GST_STATE_PLAYING ) play = TRUE;

	double volume = 0;
	gboolean mute = FALSE;

	switch ( id )
	{
		case PROP_VOLUME:
			volume = g_value_get_double (value);
			if ( play ) g_object_set ( dvb->volume, "volume", volume, NULL );
			break;

		case PROP_MUTE:
			mute = g_value_get_boolean (value);
			if ( play ) g_object_set ( dvb->volume, "mute", mute, NULL );
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, id, pspec );
			break;
	}
}

static void dvb_finalize ( GObject *object )
{
	Dvb *dvb = DVB_OBJECT ( object );

	gst_object_unref ( dvb->enc_audio );
	gst_object_unref ( dvb->enc_video );
	gst_object_unref ( dvb->enc_muxer );

	gst_object_unref ( dvb->playdvb );

	free ( dvb->data );

	G_OBJECT_CLASS (dvb_parent_class)->finalize (object);
}

static void dvb_class_init ( DvbClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = dvb_finalize;

	g_signal_new ( "dvb-play", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_ULONG, G_TYPE_STRING );

	g_signal_new ( "dvb-rec", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "dvb-stop", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "dvb-video", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_BOOLEAN, 0 );

	g_signal_new ( "dvb-is-play", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_BOOLEAN, 0 );

	g_signal_new ( "dvb-level", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 6, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_UINT, G_TYPE_UINT );

	g_signal_new ( "dvb-eqa", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_DOUBLE );

	g_signal_new ( "dvb-eqv", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_DOUBLE );

	g_signal_new ( "dvb-info", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_OBJECT );

	g_signal_new ( "dvb-message-dialog", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );

	g_signal_new ( "dvb-enc", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_OBJECT, G_TYPE_POINTER );

	g_signal_new ( "dvb-power-set", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN );

	oclass->get_property = dvb_get_property;
	oclass->set_property = dvb_set_property;

	g_object_class_install_property ( oclass, PROP_MUTE,   g_param_spec_boolean ( "mute",   NULL, NULL,      FALSE, G_PARAM_READWRITE ) );
	g_object_class_install_property ( oclass, PROP_VOLUME, g_param_spec_double  ( "volume", NULL, NULL, 0, 1, 0.75, G_PARAM_READWRITE ) );
}

Dvb * dvb_new ( void )
{
	Dvb *dvb = g_object_new ( DVB_TYPE_OBJECT, NULL );

	return dvb;
}
