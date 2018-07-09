/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <stdlib.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include <sys/ioctl.h>
#include <glib/gi18n.h>

#include "helia-descr.h"
#include "helia-scan.h"
#include "helia-mpegts.h"

#include "helia-base.h"
#include "helia-scrollwin.h"

#include "helia-service.h"
#include "helia-media-tv.h"
#include "helia-treeview-tv.h"


typedef struct _HeliaScan HeliaScan;

struct _HeliaScan
{
	GstElement *dvb_scan, *dvbsrc_tune;

	GtkNotebook *notebook;
	GtkTreeView *scan_treeview;

	GtkLabel *label_dvb_name, *all_channels, *scan_sgn_snr;
	GtkProgressBar *bar_scan_sgn, *bar_scan_snr;

	guint DVB_DELSYS, pol_num, lnb_type;
	guint adapter_set, frontend_set;
	guint info_all_ch, info_all_tv, info_all_ro;
};

static HeliaScan heliascan;



static void helia_scan_set_sgn_snr ( GstElement *element, gdouble sgl, gdouble snr, gboolean hlook );


enum page_n
{
	PAGE_SC,
	PAGE_DT,
	PAGE_DS,
	PAGE_DC,
	PAGE_AT,
	PAGE_DM,
	PAGE_IT,
	PAGE_IS,
	PAGE_CH,
	PAGE_NUM
};

const struct HeliaScanLabel { guint page; const gchar *name; } helia_scan_label_n[] =
{
	{ PAGE_SC, N_("Scanner")  },
	{ PAGE_DT, "DVB-T/T2" },
	{ PAGE_DS, "DVB-S/S2" },
	{ PAGE_DC, "DVB-C"    },
	{ PAGE_AT, "ATSC"     },
	{ PAGE_DM, "DTMB"     },
	{ PAGE_IT, "ISDB-T"   },
	{ PAGE_IS, "ISDB-S"   },
	{ PAGE_CH, N_("Channels") }
};

struct DvbDescrGstParam { const gchar *name; const gchar *dvb_v5_name; const gchar *gst_param; 
						  const DvbDescrAll *dvb_descr; guint cdsc; } gst_param_dvb_descr_n[] =
{
// descr
{ "Inversion",      "INVERSION",         "inversion",        dvb_descr_inversion_type_n,  G_N_ELEMENTS ( dvb_descr_inversion_type_n  ) },
{ "Code Rate HP",   "CODE_RATE_HP",      "code-rate-hp",     dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Code Rate LP",   "CODE_RATE_LP",      "code-rate-lp",     dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Inner Fec",      "INNER_FEC",         "code-rate-hp",     dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Modulation",     "MODULATION",        "modulation",       dvb_descr_modulation_type_n, G_N_ELEMENTS ( dvb_descr_modulation_type_n ) },
{ "Transmission",   "TRANSMISSION_MODE", "trans-mode",       dvb_descr_transmode_type_n,  G_N_ELEMENTS ( dvb_descr_transmode_type_n  ) },
{ "Guard interval", "GUARD_INTERVAL",    "guard",            dvb_descr_guard_type_n,      G_N_ELEMENTS ( dvb_descr_guard_type_n 	 ) },
{ "Hierarchy",      "HIERARCHY",         "hierarchy",        dvb_descr_hierarchy_type_n,  G_N_ELEMENTS ( dvb_descr_hierarchy_type_n  ) },
{ "Pilot",          "PILOT",             "pilot",            dvb_descr_pilot_type_n,      G_N_ELEMENTS ( dvb_descr_pilot_type_n 	 ) },
{ "Rolloff",        "ROLLOFF",           "rolloff",          dvb_descr_roll_type_n,       G_N_ELEMENTS ( dvb_descr_roll_type_n 		 ) },
{ "Polarity",       "POLARIZATION",      "polarity",         dvb_descr_polarity_type_n,   G_N_ELEMENTS ( dvb_descr_polarity_type_n   ) },
{ "LNB",            "LNB",               "lnb-type",         dvb_descr_lnb_type_n,        G_N_ELEMENTS ( dvb_descr_lnb_type_n 		 ) },
{ "DiSEqC",         "SAT_NUMBER",        "diseqc-source",    dvb_descr_lnb_num_n,         G_N_ELEMENTS ( dvb_descr_lnb_num_n 		 ) },
{ "Interleaving",   "INTERLEAVING",      "interleaving",     dvb_descr_ileaving_type_n,   G_N_ELEMENTS ( dvb_descr_ileaving_type_n 	 ) },

// digits
{ "Frequency  MHz", 	"FREQUENCY",         "frequency",        NULL, 0 },
{ "Bandwidth  Hz",  	"BANDWIDTH_HZ",      "bandwidth-hz",     NULL, 0 },
{ "Symbol rate  kBd", 	"SYMBOL_RATE",       "symbol-rate",      NULL, 0 },
{ "Stream ID",      	"STREAM_ID",         "stream-id",        NULL, 0 },
{ "Service Id",     	"SERVICE_ID",        "program-number",   NULL, 0 },
{ "Audio Pid",      	"AUDIO_PID",         "audio-pid",        NULL, 0 },
{ "Video Pid",      	"VIDEO_PID",         "video-pid",        NULL, 0 },

// ISDB
{ "Layer enabled",      "ISDBT_LAYER_ENABLED",            "isdbt-layer-enabled",            NULL, 0 },
{ "Partial",            "ISDBT_PARTIAL_RECEPTION",        "isdbt-partial-reception",        NULL, 0 },
{ "Sound",              "ISDBT_SOUND_BROADCASTING",       "isdbt-sound-broadcasting",       NULL, 0 },
{ "Subchannel  SB",     "ISDBT_SB_SUBCHANNEL_ID",         "isdbt-sb-subchannel-id",         NULL, 0 },
{ "Segment idx  SB",    "ISDBT_SB_SEGMENT_IDX",           "isdbt-sb-segment-idx",           NULL, 0 },
{ "Segment count  SB",  "ISDBT_SB_SEGMENT_COUNT",         "isdbt-sb-segment-count",         NULL, 0 },
{ "Inner Fec  LA",      "ISDBT_LAYERA_FEC",               "isdbt-layera-fec",               dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Modulation  LA",     "ISDBT_LAYERA_MODULATION",        "isdbt-layera-modulation",        dvb_descr_modulation_type_n, G_N_ELEMENTS ( dvb_descr_modulation_type_n ) },
{ "Segment count  LA",  "ISDBT_LAYERA_SEGMENT_COUNT",     "isdbt-layera-segment-count",     NULL, 0  },
{ "Interleaving  LA",   "ISDBT_LAYERA_TIME_INTERLEAVING", "isdbt-layera-time-interleaving", NULL, 0  },
{ "Inner Fec  LB",      "ISDBT_LAYERB_FEC",               "isdbt-layerb-fec",               dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Modulation  LB",     "ISDBT_LAYERB_MODULATION",        "isdbt-layerb-modulation",        dvb_descr_modulation_type_n, G_N_ELEMENTS ( dvb_descr_modulation_type_n ) },
{ "Segment count  LB",  "ISDBT_LAYERB_SEGMENT_COUNT",     "isdbt-layerb-segment-count",     NULL, 0  },
{ "Interleaving  LB",   "ISDBT_LAYERB_TIME_INTERLEAVING", "isdbt-layerb-time-interleaving", NULL, 0  },
{ "Inner Fec  LC",      "ISDBT_LAYERC_FEC",               "isdbt-layerc-fec",               dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Modulation  LC",     "ISDBT_LAYERC_MODULATION",        "isdbt-layerc-modulation",        dvb_descr_modulation_type_n, G_N_ELEMENTS ( dvb_descr_modulation_type_n ) },
{ "Segment count  LC",  "ISDBT_LAYERC_SEGMENT_COUNT",     "isdbt-layerc-segment-count",     NULL, 0  },
{ "Interleaving  LC",   "ISDBT_LAYERC_TIME_INTERLEAVING", "isdbt-layerc-time-interleaving", NULL, 0  }
};


void helia_set_lnb_low_high_switch ( GstElement *element, gint type_lnb )
{
	g_object_set ( element, "lnb-lof1", lnb_type_lhs_n[type_lnb].low_val,    NULL );
	g_object_set ( element, "lnb-lof2", lnb_type_lhs_n[type_lnb].high_val,   NULL );
	g_object_set ( element, "lnb-slof", lnb_type_lhs_n[type_lnb].switch_val, NULL );
}



static gchar * helia_strip_ch_name ( gchar *name )
{
	guint i = 0;
	for ( i = 0; name[i] != '\0'; i++ )
	{
		if ( name[i] == ':' || name[i] == '[' || name[i] == ']' ) name[i] = ' ';
	}
	return g_strstrip ( name );
}



// ***** Convert DVBv5 -> Dvbsrc *****

static void helia_strip_dvb5_to_dvbsrc ( gchar *section, guint num, guint set_adapter, guint set_frontend )
{
	//g_debug ( "gmp_strip_dvb5_to_dvbsrc:: section: %s", section );

	guint n = 0, z = 0, x = 0;

	gchar **lines = g_strsplit ( section, "\n", 0 );

	GString *gstring = g_string_new ( NULL );

	gchar *ch_name = NULL;

	for ( n = 0; lines[n] != NULL; n++ )
	{
		if ( n == 0 )
		{
			ch_name = helia_strip_ch_name ( lines[n] );
			gstring = g_string_new ( ch_name );
			g_string_append_printf ( gstring, ":adapter=%d:frontend=%d", set_adapter, set_frontend );

			g_print ( "gmp_convert_dvb5:: Channel: %s ( %d ) \n", lines[n], num );
		}

		for ( z = 0; z < G_N_ELEMENTS ( gst_param_dvb_descr_n ); z++ )
		{
			if ( g_strrstr ( lines[n], gst_param_dvb_descr_n[z].dvb_v5_name ) )
			{
				gchar **value_key = g_strsplit ( lines[n], " = ", 0 );
				
				if ( !g_str_has_prefix ( g_strstrip ( value_key[0] ), gst_param_dvb_descr_n[z].dvb_v5_name ) )
				{
					g_strfreev ( value_key );
					continue;
				}

				g_string_append_printf ( gstring, ":%s=", gst_param_dvb_descr_n[z].gst_param );
				
				// g_debug ( " gst_param -> dvb5: %s ( %s | %s )", gst_param_dvb_descr_n[z].gst_param, lines[n], gst_param_dvb_descr_n[z].dvb_v5_name );
				

				if ( g_strrstr ( gst_param_dvb_descr_n[z].dvb_v5_name, "SAT_NUMBER" ) )
				{
					g_string_append ( gstring, value_key[1] );
					
					g_strfreev ( value_key );
					continue;
				}

				if ( gst_param_dvb_descr_n[z].cdsc == 0 )
				{
					g_string_append ( gstring, value_key[1] );
				}
				else
				{
					for ( x = 0; x < gst_param_dvb_descr_n[z].cdsc; x++ )
					{
						if ( g_strrstr ( value_key[1], gst_param_dvb_descr_n[z].dvb_descr[x].dvb_v5_name ) )
						{
							g_string_append_printf ( gstring, "%d", gst_param_dvb_descr_n[z].dvb_descr[x].descr_num );
						}
					}
				}
				
				g_strfreev ( value_key );
			}
		}
	}

	if ( g_strrstr ( gstring->str, "audio-pid" ) || g_str_has_prefix ( gstring->str, "video-pid" ) ) // ignore other
		helia_media_tv_scan_save ( ch_name, gstring->str );

	g_string_free ( gstring, TRUE );

	g_strfreev ( lines );
}

void helia_convert_dvb5 ( gchar *file, guint set_adapter, guint set_frontend )
{
	guint n = 0;
	gchar *contents;
	GError *err = NULL;

	if ( g_file_get_contents ( file, &contents, 0, &err ) )
	{
		gchar **lines = g_strsplit ( contents, "[", 0 );
		guint length = g_strv_length ( lines );

		for ( n = 1; n < length; n++ )
			helia_strip_dvb5_to_dvbsrc ( lines[n], n, set_adapter, set_frontend );

		g_strfreev ( lines );
		g_free ( contents );
	}
	else
	{
		helia_service_message_dialog ( err->message, " ", GTK_MESSAGE_ERROR );

		g_critical ( "helia_convert_dvb5:: %s\n", err->message );
		g_error_free ( err );

		return;
	}
}

// ***** Convert DVBv5 -> Dvbsrc *****



const gchar * helia_get_str_dvb_type ( gint delsys )
{
	const gchar *dvb_type = NULL;

	guint i = 0;
	for ( i = 0; i < G_N_ELEMENTS ( dvb_descr_delsys_type_n ); i++ )
	{
		if ( dvb_descr_delsys_type_n[i].descr_num == delsys )
			dvb_type = dvb_descr_delsys_type_n[i].text_vis;
	}

	return dvb_type;
}
/*
static void gtv_set_delsys ( gint fd, guint del_sys )
{
	struct dtv_property dvb_prop[1];
	struct dtv_properties cmdseq;
	
	dvb_prop[0].cmd = DTV_DELIVERY_SYSTEM;
	dvb_prop[0].u.data = del_sys;
	
	cmdseq.num = 1;
	cmdseq.props = dvb_prop;
			
	if ( ioctl ( fd, FE_SET_PROPERTY, &cmdseq ) == -1 )
		perror ( "ERROR DVBv5 Set delivery system" );
	else
		g_print ( "DVBv5 Set delivery system Ok\n" );
}
*/
void helia_get_dvb_info ( GtkLabel *label_dvb_name, guint adapter, guint frontend )
{
	g_print ( "\nhelia_get_dvb_info:: adapter %d frontend %d \n", adapter, frontend );

	const gchar *dvb_name = _( "Undefined \n" );

	gint flags = O_RDWR;
	guint SYS_DVBC = SYS_DVBC_ANNEX_A;
	guint dtv_api_ver = 0, dtv_del_sys = SYS_UNDEFINED;

	gint fd;
	gchar *fd_name = g_strdup_printf ( "/dev/dvb/adapter%d/frontend%d", adapter, frontend );

	if ( ( fd = g_open ( fd_name, flags ) ) == -1 )
	{
		flags = O_RDONLY;

		if ( ( fd = g_open ( fd_name, flags ) ) == -1 )
		{
			g_critical ( "helia_get_dvb_info: %s %s\n", fd_name, g_strerror ( errno ) );

			g_free  ( fd_name );

			gtk_label_set_text ( label_dvb_name, dvb_name );

			return;
		}
	}

	struct dvb_frontend_info info;

	if ( ( ioctl ( fd, FE_GET_INFO, &info ) ) == -1 )
	{
		perror ( "ioctl FE_GET_INFO " );

		g_close ( fd, NULL );
		g_free  ( fd_name );

		gtk_label_set_text ( label_dvb_name, dvb_name );

		return;
	}
	else
		dvb_name = info.name;

	struct dtv_property dvb_prop[2];
	struct dtv_properties cmdseq;

	dvb_prop[0].cmd = DTV_DELIVERY_SYSTEM;
	dvb_prop[1].cmd = DTV_API_VERSION;

	cmdseq.num = 2;
	cmdseq.props = dvb_prop;

	if ( ( ioctl ( fd, FE_GET_PROPERTY, &cmdseq ) ) == -1 )
	{
		perror ( "ioctl FE_GET_PROPERTY " );

		dtv_api_ver = 0x300;
		gboolean legacy = FALSE;

		switch ( info.type )
		{
			case FE_QPSK:
				legacy = TRUE;
				dtv_del_sys = SYS_DVBS;
				break;

			case FE_OFDM:
				legacy = TRUE;
				dtv_del_sys = SYS_DVBT;
				break;

			case FE_QAM:
				legacy = TRUE;
				dtv_del_sys = SYS_DVBC;
				break;

			case FE_ATSC:
				legacy = TRUE;
				dtv_del_sys = SYS_ATSC;
				break;

			default:
				break;
		}

		if ( legacy )
			g_print ( "DVBv3:  Ok\n" );
		else
			g_critical ( "DVBv3:  Failed\n" );

		if ( !legacy )
		{
			g_close ( fd, NULL );
			g_free  ( fd_name );

			gtk_label_set_text ( label_dvb_name, dvb_name );

			return;
		}
	}
	else
	{
		g_print ( "\nDVBv5:  Ok\n" );

		dtv_del_sys = dvb_prop[0].u.data;
		dtv_api_ver = dvb_prop[1].u.data;
		
		// gtv_set_delsys ( fd, dtv_del_sys );
	}

	g_close ( fd, NULL );
	g_free  ( fd_name );

	const gchar *dvb_type = helia_get_str_dvb_type ( dtv_del_sys );

	if ( label_dvb_name )
	{
		gchar *text = g_strdup_printf ( "%s\n%s", dvb_name, dvb_type ? dvb_type : _( "Undefined" ) );
			gtk_label_set_text ( label_dvb_name, text );
		g_free  ( text );
	}

	g_print ( "DVB device:  %s\n%s  ( %d )\nAPI Version:  %d.%d\nFlag:  %s\n\n", dvb_name, dvb_type ? dvb_type : _( "Undefined" ), 
			dtv_del_sys, dtv_api_ver / 256, dtv_api_ver % 256, ( flags == O_RDWR ) ? "O_RDWR" : "O_RDONLY" );
}



static void helia_scan_msg_all ( GstBus *bus, GstMessage *message )
{
	if ( hbgv.all_info )
		g_debug ( "helia_scan_msg_all:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );

	const GstStructure *structure = gst_message_get_structure ( message );

	if ( structure )
	{
		gint signal, snr;
		gboolean hlook = FALSE;

		if (  gst_structure_get_int ( structure, "signal", &signal )  )
		{
			gst_structure_get_boolean ( structure, "lock", &hlook );
			gst_structure_get_int ( structure, "snr", &snr);

			if ( heliascan.scan_treeview )
				helia_scan_set_sgn_snr ( heliascan.dvb_scan, (signal * 100) / 0xffff, (snr * 100) / 0xffff, hlook );
		}
	}

	helia_mpegts_parse_section ( message );
}

void helia_scan_msg_war ( GstBus *bus, GstMessage *msg )
{
	if ( hbgv.all_info )
		g_debug ( "helia_scan_msg_war:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );

	GError *war = NULL;
	gchar  *dbg = NULL;

	gst_message_parse_warning ( msg, &war, &dbg );
	
	g_warning ( "helia_scan_msg_war:: %s (%s)", war->message, (dbg) ? dbg : "no details" );

	g_error_free ( war );
	g_free ( dbg );
}

static void helia_scan_msg_err ( GstBus *bus, GstMessage *msg )
{
	if ( hbgv.all_info )
		g_debug ( "helia_scan_msg_err:: pending %s", gst_bus_have_pending ( bus ) ? "TRUE" : "FALSE" );

	GError *err = NULL;
	gchar  *dbg = NULL;

	gst_message_parse_error ( msg, &err, &dbg );

	g_critical ( "scan_msg_err:: %s (%s)\n", err->message, (dbg) ? dbg : "no details" );
	helia_service_message_dialog ( err->message, (dbg) ? dbg : " ", GTK_MESSAGE_ERROR );

	g_error_free ( err );
	g_free ( dbg );

	helia_scan_stop ();
}

static void helia_scan_set_tune_timeout ( guint64 time_set )
{
	guint64 timeout = 0, timeout_set = 0, timeout_get = 0, timeout_def = 10000000000;
	g_object_get ( heliascan.dvbsrc_tune, "tuning-timeout", &timeout, NULL );

	timeout_set = timeout_def / 10 * time_set;

	g_object_set ( heliascan.dvbsrc_tune, "tuning-timeout", (guint64)timeout_set, NULL );

	g_object_get ( heliascan.dvbsrc_tune, "tuning-timeout", &timeout_get, NULL );

	g_debug ( "helia_scan_set_tune_timeout: timeout %ld | timeout set %ld", timeout, timeout_get );
}
static void helia_scan_set_tune_def ()
{
	g_object_set ( heliascan.dvbsrc_tune, "bandwidth-hz", 8000000,  NULL );
	g_object_set ( heliascan.dvbsrc_tune, "modulation",   QAM_AUTO, NULL );

	g_debug ( "helia_scan_set_tune_def" );
}

void helia_scan_gst_create ()
{
	helia_mpegts_initialize ();

	GstElement *dvbsrc_parse, *dvbsrc_files;

	heliascan.pol_num  = 0;
	heliascan.lnb_type = 0;
	hbgv.all_info = FALSE;

	heliascan.dvb_scan     = gst_pipeline_new ( "pipeline_scan" );
	heliascan.dvbsrc_tune  = gst_element_factory_make ( "dvbsrc",   NULL );
	dvbsrc_parse = gst_element_factory_make ( "tsparse",  NULL );
	dvbsrc_files = gst_element_factory_make ( "filesink", NULL );

	if ( !heliascan.dvb_scan || !heliascan.dvbsrc_tune || !dvbsrc_parse || !dvbsrc_files )
		g_critical ( "helia_scan_gst_create:: pipeline_scan - not be created.\n" );

	gst_bin_add_many ( GST_BIN ( heliascan.dvb_scan ), heliascan.dvbsrc_tune, dvbsrc_parse, dvbsrc_files, NULL );
	gst_element_link_many ( heliascan.dvbsrc_tune, dvbsrc_parse, dvbsrc_files, NULL );

	g_object_set ( dvbsrc_files, "location", "/dev/null", NULL);

	GstBus *bus_scan = gst_element_get_bus ( heliascan.dvb_scan );
	gst_bus_add_signal_watch ( bus_scan );
	gst_object_unref ( bus_scan );

	g_signal_connect ( bus_scan, "message",          G_CALLBACK ( helia_scan_msg_all ), NULL );
	g_signal_connect ( bus_scan, "message::error",   G_CALLBACK ( helia_scan_msg_err ), NULL );
	g_signal_connect ( bus_scan, "message::warning", G_CALLBACK ( helia_scan_msg_war ), NULL );

	helia_scan_set_tune_timeout ( 5 );
	helia_scan_set_tune_def ();

	g_debug ( "helia_scan_gst_create" );
}



static void helia_scan_get_tp_data ( GString *gstring )
{
	guint c = 0, d = 0;
	gint  d_data = 0, DVBTYPE = 0;

	g_object_get ( heliascan.dvbsrc_tune, "delsys", &DVBTYPE, NULL );

	const gchar *dvb_f[] = { "delsys", "adapter", "frontend" };

	for ( d = 0; d < G_N_ELEMENTS ( dvb_f ); d++ )
	{
		g_object_get ( heliascan.dvbsrc_tune, dvb_f[d], &d_data, NULL );
		g_string_append_printf ( gstring, ":%s=%d", dvb_f[d], d_data );
	}

	if ( DVBTYPE == SYS_DVBT || DVBTYPE == SYS_DVBT2 )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dvbt_props_n ); c++ )
		{
			if ( DVBTYPE == SYS_DVBT )
				if ( g_str_has_prefix ( dvbt_props_n[c].param, "Stream ID" ) ) continue;

			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( dvbt_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DTMB )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dtmb_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( dtmb_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DVBC_ANNEX_A || DVBTYPE == SYS_DVBC_ANNEX_C )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dvbc_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( dvbc_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_ATSC || DVBTYPE == SYS_DVBC_ANNEX_B )
	{
		for ( c = 0; c < G_N_ELEMENTS ( atsc_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( atsc_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DVBS || DVBTYPE == SYS_TURBO || DVBTYPE == SYS_DVBS2 )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dvbs_props_n ); c++ )
		{
			if ( DVBTYPE == SYS_TURBO )
				if ( g_str_has_prefix ( dvbs_props_n[c].param, "Pilot" ) || g_str_has_prefix ( dvbs_props_n[c].param, "Rolloff" ) || g_str_has_prefix ( dvbs_props_n[c].param, "Stream ID" ) ) continue;

			if ( DVBTYPE == SYS_DVBS )
				if ( g_str_has_prefix ( dvbs_props_n[c].param, "Modulation" ) || g_str_has_prefix ( dvbs_props_n[c].param, "Pilot" ) || g_str_has_prefix ( dvbs_props_n[c].param, "Rolloff" ) || g_str_has_prefix ( dvbs_props_n[c].param, "Stream ID" ) ) continue;

			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( dvbs_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					if ( g_str_has_prefix ( "polarity", gst_param_dvb_descr_n[d].gst_param ) )
					{
						gchar *pol = NULL;
						g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[d].gst_param, &pol, NULL );
							g_string_append_printf ( gstring, ":polarity=%s", pol );
						g_free ( pol );

						continue;
					}

					if ( g_str_has_prefix ( "lnb-type", gst_param_dvb_descr_n[d].gst_param ) )
					{
						g_string_append_printf ( gstring, ":%s=%d", "lnb-type", heliascan.lnb_type );
						continue;
					}

					g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_ISDBT )
	{
		for ( c = 0; c < G_N_ELEMENTS ( isdbt_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( isdbt_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}
	
	if ( DVBTYPE == SYS_ISDBS )
	{
		for ( c = 0; c < G_N_ELEMENTS ( isdbs_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( gst_param_dvb_descr_n ); d++ )
			{
				if ( g_str_has_suffix ( isdbs_props_n[c].param, gst_param_dvb_descr_n[d].name ) )
				{
					g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", gst_param_dvb_descr_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}	
}



static void helia_scan_set_all_ch ( gboolean clear, guint all_ch, guint c_tv, guint c_ro )
{
	heliascan.info_all_ch += all_ch;
	heliascan.info_all_tv += c_tv;
	heliascan.info_all_ro += c_ro;

	if ( clear )
	{
		heliascan.info_all_ch = 0;
		heliascan.info_all_tv = 0;
		heliascan.info_all_ro = 0;
	}

	gchar *text = NULL;

	text = g_strdup_printf ( " %s: %d \n %s: %d -- %s: %d -- %s: %d",
				_("All Channels"), heliascan.info_all_ch, _("TV"), heliascan.info_all_tv, _("Radio"), heliascan.info_all_ro, _("Other"), 
				heliascan.info_all_ch - heliascan.info_all_tv - heliascan.info_all_ro );

	gtk_label_set_text ( GTK_LABEL ( heliascan.all_channels ), text );

	g_print ( "%s\n", text );

	g_free ( text );
}

static void helia_scan_set_info_ch ()
{
	guint c = 0, c_tv = 0, c_ro = 0;

	for ( c = 0; c < gtvmpegts.pmt_count; c++ )
	{
		if ( dvb_gst_pmt_n[c].vpid > 0 ) c_tv++;
		if ( dvb_gst_pmt_n[c].apid > 0 && dvb_gst_pmt_n[c].vpid == 0 ) c_ro++;
	}

	helia_scan_set_all_ch ( FALSE, gtvmpegts.pmt_count, c_tv, c_ro );
}

static void helia_scan_channels_save ( GtkButton *button, GtkTreeView *tree_view )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	gboolean valid;
	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		  valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		gchar *name, *data;

		gtk_tree_model_get ( model, &iter, COL_DATA,  &data, -1 );
		gtk_tree_model_get ( model, &iter, COL_TITLE, &name, -1 );

			helia_media_tv_scan_save ( name, data );

		g_free ( name );
		g_free ( data );
	}

	if ( hbgv.all_info )
		g_debug ( "helia_scan_channels_save:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}
static void helia_scan_channels_clear ( GtkButton *button, GtkTreeView *tree_view )
{
	helia_scan_set_all_ch ( TRUE, 0, 0, 0 );
	gtk_label_set_text ( GTK_LABEL ( heliascan.all_channels ), _("All Channels") );

	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );
	gtk_list_store_clear ( GTK_LIST_STORE ( model ) );

	if ( hbgv.all_info )
		g_debug ( "helia_scan_channels_clear:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}
static GtkBox * helia_scan_channels ()
{
	g_debug ( "helia_scan_channels" );

	GtkBox *g_box  = (GtkBox *)gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 0 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 0 );

	heliascan.all_channels = (GtkLabel *)gtk_label_new ( _("All Channels") );
	gtk_widget_set_halign ( GTK_WIDGET ( heliascan.all_channels ), GTK_ALIGN_START );

	heliascan.scan_treeview = (GtkTreeView *)gtk_tree_view_new_with_model ( GTK_TREE_MODEL ( gtk_list_store_new ( 3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING ) ) );

	HBTrwCol trwc_tv_n[] =
	{
		{ "Num", TRUE  }, { _("Channels"), TRUE  }, { "Data", FALSE }
	};

	GtkScrolledWindow *scroll_win = helia_scrollwin ( heliascan.scan_treeview, trwc_tv_n, G_N_ELEMENTS ( trwc_tv_n ), 200 );

	gtk_box_pack_start ( g_box, GTK_WIDGET ( scroll_win ), TRUE, TRUE, 0 );

	GtkBox *lh_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_pack_start ( lh_box, GTK_WIDGET ( heliascan.all_channels ),  FALSE, FALSE, 10 );
	gtk_box_pack_start ( g_box,  GTK_WIDGET ( lh_box ),  FALSE, FALSE, 10 );

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( h_box ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( h_box ), 5 );
	gtk_box_set_spacing ( h_box, 5 );

	helia_service_create_image_button ( h_box, "helia-clear", 16, helia_scan_channels_clear, GTK_WIDGET ( heliascan.scan_treeview ) );
	helia_service_create_image_button ( h_box, "helia-save",  16, helia_scan_channels_save,  GTK_WIDGET ( heliascan.scan_treeview ) );

	gtk_box_pack_start ( g_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

	return g_box;
}



static void helia_scan_set_adapter ( GtkSpinButton *button, GtkLabel *label_dvb_name )
{
	gtk_spin_button_update ( button );
	heliascan.adapter_set = gtk_spin_button_get_value_as_int ( button );

	g_object_set ( heliascan.dvbsrc_tune, "adapter",  heliascan.adapter_set,  NULL );
	helia_get_dvb_info ( label_dvb_name, heliascan.adapter_set, heliascan.frontend_set );

	g_debug ( "helia_scan_set_adapter" );
}
static void helia_scan_set_frontend ( GtkSpinButton *button, GtkLabel *label_dvb_name )
{
	gtk_spin_button_update ( button );
	heliascan.frontend_set = gtk_spin_button_get_value_as_int ( button );

	g_object_set ( heliascan.dvbsrc_tune, "frontend",  heliascan.frontend_set,  NULL );
	helia_get_dvb_info ( label_dvb_name, heliascan.adapter_set, heliascan.frontend_set );

	g_debug ( "helia_scan_set_frontend" );
}

GtkBox * helia_scan_device ( void (* activate_a)(), void (* activate_f)() )
{
	g_debug ( "helia_scan_device" );

	heliascan.adapter_set  = 0;
	heliascan.frontend_set = 0;


	GtkBox *g_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 10 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( GTK_GRID ( grid ), TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	struct data_a { GtkWidget *label; const gchar *ltext; GtkWidget *widget; gchar *etext; guint edat; void (* activate)(); }
	data_a_n[] =
	{
		{ gtk_label_new ( "" ), "DVB Device", NULL, NULL, 0, NULL },
		{ gtk_label_new ( "" ), NULL,         NULL, NULL, 0, NULL },

		{ gtk_label_new ( "" ), "Adapter",  gtk_spin_button_new_with_range ( 0, 16, 1 ), NULL, heliascan.adapter_set,  activate_a },
		{ gtk_label_new ( "" ), "Frontend", gtk_spin_button_new_with_range ( 0, 16, 1 ), NULL, heliascan.frontend_set, activate_f }
	};

	guint d = 0;
	for ( d = 0; d < G_N_ELEMENTS ( data_a_n ); d++ )
	{
		gtk_label_set_label ( GTK_LABEL ( data_a_n[d].label ), data_a_n[d].ltext );
		gtk_widget_set_halign ( GTK_WIDGET ( data_a_n[d].label ), ( d == 0 ) ? GTK_ALIGN_CENTER : GTK_ALIGN_START );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( data_a_n[d].label ), 0, d, ( d == 0 ) ? 2 : 1, 1 );

		if ( d == 0 )
		{
			heliascan.label_dvb_name = GTK_LABEL ( data_a_n[d].label );
			helia_get_dvb_info ( heliascan.label_dvb_name, heliascan.adapter_set, heliascan.frontend_set );

			continue;
		}

		if ( !data_a_n[d].widget ) continue;

		gtk_spin_button_set_value ( GTK_SPIN_BUTTON ( data_a_n[d].widget ), data_a_n[d].edat );
		g_signal_connect ( data_a_n[d].widget, "changed", G_CALLBACK ( data_a_n[d].activate ), heliascan.label_dvb_name );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( data_a_n[d].widget ), 1, d, 1, 1 );
	}

	return g_box;
}





static void helia_scan_read_ch_to_treeview ()
{
	if ( gtvmpegts.pmt_count == 0 )
		return;

	GString *gstr_data = g_string_new ( NULL );

	helia_scan_get_tp_data ( gstr_data );

	g_debug ( "%s", gstr_data->str );

	guint i = 0, c = 0, sdt_vct_count = gtvmpegts.pat_count;

	if ( gtvmpegts.sdt_count ) sdt_vct_count = gtvmpegts.sdt_count;
	if ( gtvmpegts.vct_count ) sdt_vct_count = gtvmpegts.vct_count;

	for ( i = 0; i < gtvmpegts.pmt_count; i++ )
	{
		gchar *ch_name = NULL;

		for ( c = 0; c < sdt_vct_count; c++ )
		{
			if ( dvb_gst_pmt_n[i].pmn_pid == dvb_gst_sdt_n[c].pmn_pid )
				{ ch_name = dvb_gst_sdt_n[c].name; break; }

			if ( dvb_gst_pmt_n[i].pmn_pid == dvb_gst_vct_n[c].pmn_pid )
				{ ch_name = dvb_gst_sdt_n[c].name; break; }
		}

		if ( ch_name )
			ch_name = helia_strip_ch_name ( ch_name );
		else
			ch_name = g_strdup_printf ( "Program-%d", dvb_gst_pmt_n[i].pmn_pid );

		GString *gstring = g_string_new ( ch_name );

		g_string_append_printf ( gstring, ":program-number=%d:video-pid=%d:audio-pid=%d", 
								 dvb_gst_pmt_n[i].pmn_pid, dvb_gst_pmt_n[i].vpid, dvb_gst_pmt_n[i].apid );

		g_string_append_printf ( gstring, "%s", gstr_data->str );

		if ( heliascan.scan_treeview && dvb_gst_pmt_n[i].apid != 0 ) // ignore other
			helia_treeview_tv_add ( heliascan.scan_treeview, ch_name, gstring->str );

		g_print ( "%s \n", ch_name );

		g_debug ( "%s", gstring->str );

		g_free ( ch_name );
		g_string_free ( gstring, TRUE );
	}

	g_string_free ( gstr_data, TRUE );

	if ( heliascan.scan_treeview )
		helia_scan_set_info_ch ();
}





static void helia_scan_start ()
{
	if ( GST_ELEMENT_CAST ( heliascan.dvb_scan )->current_state == GST_STATE_PLAYING )
		return;

	time ( &gtvmpegts.t_start );

	gst_element_set_state ( heliascan.dvb_scan, GST_STATE_PLAYING );
	
	g_debug ( "helia_scan_start" );
}

void helia_scan_stop ()
{
	if ( GST_ELEMENT_CAST ( heliascan.dvb_scan )->current_state == GST_STATE_NULL )
		return;

	gst_element_set_state ( heliascan.dvb_scan, GST_STATE_NULL );

	helia_scan_read_ch_to_treeview ();
	
	helia_mpegts_clear_scan ();

	helia_scan_set_sgn_snr ( heliascan.dvb_scan, 0, 0, FALSE );
	
	g_debug ( "helia_scan_stop" );
}

static void helia_scan_set_sgn_snr ( GstElement *element, gdouble sgl, gdouble snr, gboolean hlook )
{
	GtkLabel *label = heliascan.scan_sgn_snr;
	GtkProgressBar *barsgn = heliascan.bar_scan_sgn;
	GtkProgressBar *barsnr = heliascan.bar_scan_snr;

	gtk_progress_bar_set_fraction ( barsgn, sgl/100 );
	gtk_progress_bar_set_fraction ( barsnr, snr/100 );

	gchar *texta = g_strdup_printf ( "Level %d%s", (int)sgl, "%" );
	gchar *textb = g_strdup_printf ( "Snr %d%s",   (int)snr, "%" );

	const gchar *format = NULL;

	gboolean play = TRUE;
	if ( GST_ELEMENT_CAST ( element )->current_state != GST_STATE_PLAYING ) play = FALSE;

	if ( hlook )
		format = "<span>\%s</span><span foreground=\"#00ff00\"> ◉ </span><span>\%s</span><span foreground=\"#ff0000\">  \%s</span>";
	else
		format = "<span>\%s</span><span foreground=\"#ff0000\"> ◉ </span><span>\%s</span><span foreground=\"#ff0000\">  \%s</span>";

	if ( !play )
		format = "<span>\%s</span><span foreground=\"#ff8000\"> ◉ </span><span>\%s</span><span foreground=\"#ff0000\">  \%s</span>";

	if ( sgl == 0 && snr == 0 )
		format = "<span>\%s</span><span foreground=\"#bfbfbf\"> ◉ </span><span>\%s</span><span foreground=\"#ff0000\">  \%s</span>";

	gchar *markup = g_markup_printf_escaped ( format, texta, textb, "" );
		gtk_label_set_markup ( label, markup );
	g_free ( markup );

	g_free ( texta );
	g_free ( textb );
}
static GtkBox * helia_scan_create_sgn_snr ()
{
	g_debug ( "helia_scan_create_sgn_snr" );

	GtkBox *tbar_dvb = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( tbar_dvb ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( tbar_dvb ), 5 );

	heliascan.scan_sgn_snr = (GtkLabel *)gtk_label_new ( "Level  &  Quality" );
	gtk_box_pack_start ( tbar_dvb, GTK_WIDGET ( heliascan.scan_sgn_snr  ), FALSE, FALSE, 5 );

	heliascan.bar_scan_sgn = (GtkProgressBar *)gtk_progress_bar_new ();
	heliascan.bar_scan_snr = (GtkProgressBar *)gtk_progress_bar_new ();

	gtk_box_pack_start ( tbar_dvb, GTK_WIDGET ( heliascan.bar_scan_sgn  ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( tbar_dvb, GTK_WIDGET ( heliascan.bar_scan_snr  ), FALSE, FALSE, 3 );

	return tbar_dvb;
}


static void helia_scan_create_control_battons ( GtkBox *b_box )
{
	gtk_box_pack_start ( b_box, GTK_WIDGET ( gtk_label_new ( " " ) ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( b_box, GTK_WIDGET ( helia_scan_create_sgn_snr () ), FALSE, FALSE, 0 );

	GtkBox *hb_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( hb_box ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( hb_box ), 5 );
	gtk_box_set_spacing ( hb_box, 5 );

	helia_service_create_image_button ( hb_box, "helia-media-start", 16, helia_scan_start, NULL );
	helia_service_create_image_button ( hb_box, "helia-media-stop",  16, helia_scan_stop,  NULL );

	gtk_box_pack_start ( b_box, GTK_WIDGET ( hb_box ), FALSE, FALSE, 5 );
}



static glong helia_scan_label_freq ( GtkLabel *label_set, glong num )
{
	gint numpage = gtk_notebook_get_current_page ( heliascan.notebook );

	GtkWidget *page = gtk_notebook_get_nth_page ( heliascan.notebook, numpage );
	GtkLabel *label = (GtkLabel *)gtk_notebook_get_tab_label ( heliascan.notebook, page );
	const gchar *name_tab = gtk_label_get_text ( label );

	if ( g_str_has_prefix ( name_tab, "DVB-S" ) || g_str_has_prefix ( name_tab, "ISDB-S" ) )
	{
		if ( num < 30000 )
		{
			num *= 1000;
			gtk_label_set_text ( label_set, "Frequency  MHz" );
		}
		else
			gtk_label_set_text ( label_set, "Frequency  KHz" );
	}
	else
	{
		if ( num < 1000 )
		{
			num *= 1000000;
			gtk_label_set_text ( label_set, "Frequency  MHz" );
		}
		else if ( num < 1000000 )
		{
			num *= 1000;
			gtk_label_set_text ( label_set, "Frequency  KHz" );
		}
	}

	g_debug ( "num = %ld | numpage = %d | %s", num, numpage, name_tab );

	return num;
}

static void helia_scan_changed_spin_all ( GtkSpinButton *button, GtkLabel *label )
{
	gtk_spin_button_update ( button );

	glong num = gtk_spin_button_get_value  ( button );
	const gchar *name = gtk_label_get_text ( label  );

	if ( g_str_has_prefix ( name, "Frequency" ) )
	{
		num = helia_scan_label_freq ( label, num );
		
		g_object_set ( heliascan.dvbsrc_tune, "frequency", num, NULL );

		g_debug ( "name = %s | num = %ld | gst_param = %s", gtk_label_get_text ( label ), 
				   num, "frequency" );
		
		return;
	}
	
	guint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
	{
		if ( g_str_has_suffix ( name, gst_param_dvb_descr_n[c].name ) )
		{
			g_object_set ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[c].gst_param, num, NULL );

			g_debug ( "name = %s | num = %ld | gst_param = %s", gtk_label_get_text ( label ), 
					num, gst_param_dvb_descr_n[c].gst_param );
		}
	}
}
static void helia_scan_changed_combo_all ( GtkComboBox *combo_box, GtkLabel *label )
{
	guint num = gtk_combo_box_get_active ( combo_box );
	const gchar *name = gtk_label_get_text ( label );

	if ( g_str_has_prefix ( name, "LNB" ) )
	{
		heliascan.lnb_type = num;
		
		helia_set_lnb_low_high_switch ( heliascan.dvbsrc_tune, num );
		
		g_debug ( "name %s | set %s: %d ( low %ld | high %ld | switch %ld )", name, lnb_type_lhs_n[num].name, num, 
				  lnb_type_lhs_n[num].low_val, lnb_type_lhs_n[num].high_val, lnb_type_lhs_n[num].switch_val );

		return;
	}

	guint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
	{
		if ( g_str_has_suffix ( name, gst_param_dvb_descr_n[c].name ) )
		{

			if ( g_str_has_prefix ( name, "Polarity" ) )
			{
				heliascan.pol_num = num;

				if ( heliascan.pol_num == 1 || heliascan.pol_num == 3 )
					g_object_set ( heliascan.dvbsrc_tune, "polarity", "V", NULL );
				else
					g_object_set ( heliascan.dvbsrc_tune, "polarity", "H", NULL );
			}
			else
				g_object_set ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[c].gst_param, 
					gst_param_dvb_descr_n[c].dvb_descr[num].descr_num, NULL );
			
			
			g_debug ( "name = %s | num = %d | gst_param = %s | descr_text_vis = %s | descr_num = %d", 
					name, num, gst_param_dvb_descr_n[c].gst_param, 
					gst_param_dvb_descr_n[c].dvb_descr[num].text_vis, 
					gst_param_dvb_descr_n[c].dvb_descr[num].descr_num );
		}
	}
}
static GtkBox * helia_scan_dvb_all  ( guint num, const DvbTypes *dvball, const gchar *type )
{
	g_debug ( "helia_scan_dvb_all:: %s", type );

	GtkBox *g_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 10 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( GTK_GRID ( grid ), TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	GtkLabel *label;
	GtkSpinButton *spinbutton;
	GtkComboBoxText *scan_combo_box;

	
	gboolean freq = FALSE;
	gint d_data = 0, set_freq = 1000000;
	guint d = 0, c = 0, z = 0;
	
	if ( g_str_has_prefix ( type, "DVB-S" ) || g_str_has_prefix ( type, "ISDB-S" ) ) set_freq = 1000;
	
	
	for ( d = 0; d < num; d++ )
	{		
		label = (GtkLabel *)gtk_label_new ( dvball[d].param );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( label ), 0, d, 1, 1 );

		if ( !dvball[d].descr )
		{
			if ( g_str_has_prefix ( dvball[d].param, "Frequency" ) ) freq = TRUE; else freq = FALSE;

			spinbutton = (GtkSpinButton *) gtk_spin_button_new_with_range ( dvball[d].min, dvball[d].max, 1 );
			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( spinbutton ), 1, d, 1, 1 );

			for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
			{
				if ( g_str_has_suffix ( dvball[d].param, gst_param_dvb_descr_n[c].name ) )
				{
					g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[c].gst_param, &d_data, NULL );
					gtk_spin_button_set_value ( spinbutton, freq ? d_data / set_freq : d_data );
				}
			}

			g_signal_connect ( spinbutton, "changed", G_CALLBACK ( helia_scan_changed_spin_all ), label );
		}
		else
		{
			scan_combo_box = (GtkComboBoxText *) gtk_combo_box_text_new ();
			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( scan_combo_box ), 1, d, 1, 1 );

			for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
			{
				if ( g_str_has_suffix ( dvball[d].param, gst_param_dvb_descr_n[c].name ) )
				{
					for ( z = 0; z < gst_param_dvb_descr_n[c].cdsc; z++ )
						gtk_combo_box_text_append_text ( scan_combo_box, gst_param_dvb_descr_n[c].dvb_descr[z].text_vis );

					if ( g_str_has_prefix ( dvball[d].param, "Polarity" ) )
					{
						gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), heliascan.pol_num );
						continue;
					}

					if ( g_str_has_prefix ( dvball[d].param, "LNB" ) )
						d_data = heliascan.lnb_type;
					else
						g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[c].gst_param, &d_data, NULL );

					if ( g_str_has_prefix ( dvball[d].param, "DiSEqC" ) ) d_data = d_data + 1;

					gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), d_data );
				}
			}

			if ( gtk_combo_box_get_active ( GTK_COMBO_BOX ( scan_combo_box ) ) == -1 )
				gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), 0 );

			g_signal_connect ( scan_combo_box, "changed", G_CALLBACK ( helia_scan_changed_combo_all ), label );
		}
	}

	return g_box;
}
static GtkBox * helia_scan_isdb_all  ( guint num, const DvbTypes *dvball, const gchar *type )
{
	g_debug ( "helia_scan_isdb_all:: %s", type );

	GtkBox *g_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 10 );

	GtkGrid *grid;
	GtkNotebook *notebook;
	
	if ( g_str_has_prefix ( type, "ISDB-S" ) )
	{
		grid = (GtkGrid *)gtk_grid_new();
		gtk_grid_set_column_homogeneous ( GTK_GRID ( grid ), TRUE );
		gtk_box_pack_start ( g_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );
	}

	if ( g_str_has_prefix ( type, "ISDB-T" ) )
	{
		notebook = (GtkNotebook *)gtk_notebook_new ();
		gtk_notebook_set_tab_pos ( notebook, GTK_POS_TOP );
		gtk_notebook_set_scrollable ( notebook, TRUE );
	}
	
	struct NameLabel { const gchar *name; const gchar *label; } nl_n[] =
	{
		{ "Frequency",      "Frequency" },
		{ "Layer enabled",  "Layer"     },
		{ "Subchannel  SB", "SB"        },
		{ "Inner Fec  LA",  "Layer A"   },
		{ "Inner Fec  LB",  "Layer B"   },
		{ "Inner Fec  LC",  "Layer C"   }
	};
	
	GtkLabel *label;
	GtkSpinButton *spinbutton;
	GtkComboBoxText *scan_combo_box;

	
	gboolean freq = FALSE;
	gint d_data = 0, set_freq = 1000000;
	guint d = 0, c = 0, z = 0, q = 0;
	
	if ( g_str_has_prefix ( type, "DVB-S" ) || g_str_has_prefix ( type, "ISDB-S" ) ) set_freq = 1000;
	
	
	for ( d = 0; d < num; d++ )
	{
		if ( g_str_has_prefix ( type, "ISDB-T" ) )
		{
			if ( g_str_has_suffix ( dvball[d].param, "Inner Fec  LA" ) )
			{
				gtk_box_pack_start ( g_box, GTK_WIDGET ( notebook ), TRUE, TRUE, 10 );
			
				notebook = (GtkNotebook *)gtk_notebook_new ();
				gtk_notebook_set_tab_pos ( notebook, GTK_POS_TOP );
				gtk_notebook_set_scrollable ( notebook, TRUE );
			}
		  
			for ( q = 0; q < G_N_ELEMENTS ( nl_n ); q++ )
			{
				if ( g_str_has_prefix ( dvball[d].param, nl_n[q].name ) )
				{
					grid = (GtkGrid *)gtk_grid_new();
					gtk_grid_set_column_homogeneous ( GTK_GRID ( grid ), TRUE );
					gtk_notebook_append_page ( notebook, GTK_WIDGET ( grid ),  gtk_label_new ( nl_n[q].label ) );				  
				}
			}
		}

		label = (GtkLabel *)gtk_label_new ( dvball[d].param );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( label ), 0, d, 1, 1 );

		if ( !dvball[d].descr )
		{
			if ( g_str_has_prefix ( dvball[d].param, "Frequency" ) ) freq = TRUE; else freq = FALSE;
			
			spinbutton = (GtkSpinButton *) gtk_spin_button_new_with_range ( dvball[d].min, dvball[d].max, 1 );
			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( spinbutton ), 1, d, 1, 1 );	

			for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
			{
				if ( g_str_has_suffix ( dvball[d].param, gst_param_dvb_descr_n[c].name ) )
				{
					g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[c].gst_param, &d_data, NULL );
					gtk_spin_button_set_value ( spinbutton, freq ? d_data / set_freq : d_data );
				}
			}

			g_signal_connect ( spinbutton, "changed", G_CALLBACK ( helia_scan_changed_spin_all ), label );
		}
		else
		{
			scan_combo_box = (GtkComboBoxText *) gtk_combo_box_text_new ();
			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( scan_combo_box ), 1, d, 1, 1 );

			for ( c = 0; c < G_N_ELEMENTS ( gst_param_dvb_descr_n ); c++ )
			{
				if ( g_str_has_suffix ( dvball[d].param, gst_param_dvb_descr_n[c].name ) )
				{
					for ( z = 0; z < gst_param_dvb_descr_n[c].cdsc; z++ )
						gtk_combo_box_text_append_text ( scan_combo_box, gst_param_dvb_descr_n[c].dvb_descr[z].text_vis );

					if ( g_str_has_prefix ( dvball[d].param, "LNB" ) )
						d_data = heliascan.lnb_type;
					else
						g_object_get ( heliascan.dvbsrc_tune, gst_param_dvb_descr_n[c].gst_param, &d_data, NULL );

					if ( g_str_has_prefix ( dvball[d].param, "DiSEqC" ) ) d_data = d_data + 1;

					gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), d_data );
				}
			}

			if ( gtk_combo_box_get_active ( GTK_COMBO_BOX ( scan_combo_box ) ) == -1 )
				gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), 0 );

			g_signal_connect ( scan_combo_box, "changed", G_CALLBACK ( helia_scan_changed_combo_all ), label );
		}
	}
	
	if ( g_str_has_prefix ( type, "ISDB-T" ) )
		gtk_box_pack_start ( g_box, GTK_WIDGET ( notebook ), TRUE, TRUE, 10 );

	return g_box;
}

static GtkBox * helia_scan_all_box ( guint i )
{
	g_debug ( "helia_scan_all_box" );

	GtkBox *only_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	if ( i == PAGE_SC )  { return helia_scan_device   ( helia_scan_set_adapter, helia_scan_set_frontend ); }
	if ( i == PAGE_CH )  { return helia_scan_channels (); }
	if ( i == PAGE_DT )  { return helia_scan_dvb_all  ( G_N_ELEMENTS ( dvbt_props_n  ), dvbt_props_n,  "DVB-T"  ); }
	if ( i == PAGE_DS )  { return helia_scan_dvb_all  ( G_N_ELEMENTS ( dvbs_props_n  ), dvbs_props_n,  "DVB-S"  ); }
	if ( i == PAGE_DC )  { return helia_scan_dvb_all  ( G_N_ELEMENTS ( dvbc_props_n  ), dvbc_props_n,  "DVB-C"  ); }
	if ( i == PAGE_AT )  { return helia_scan_dvb_all  ( G_N_ELEMENTS ( atsc_props_n  ), atsc_props_n,  "ATSC"   ); }
	if ( i == PAGE_DM )  { return helia_scan_dvb_all  ( G_N_ELEMENTS ( dtmb_props_n  ), dtmb_props_n,  "DTMB"   ); }
	if ( i == PAGE_IT )  { return helia_scan_isdb_all ( G_N_ELEMENTS ( isdbt_props_n ), isdbt_props_n, "ISDB-T" ); }
	if ( i == PAGE_IS )  { return helia_scan_isdb_all ( G_N_ELEMENTS ( isdbs_props_n ), isdbs_props_n, "ISDB-S" ); }

	return only_box;
}


static void helia_scan_quit ( GtkWindow *window )
{
	helia_scan_stop ();
	
	helia_scan_set_all_ch ( TRUE, 0, 0, 0 );

	gtk_widget_destroy ( GTK_WIDGET ( window ) );

	heliascan.scan_treeview = NULL;

	g_debug ( "helia_scan_quit" );
}

static void helia_scan_close ( GtkButton *button, GtkWindow *window )
{
	helia_scan_quit ( window );

	g_debug ( "helia_scan_close:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

void helia_scan_win ()
{
	GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window, helia_base_win_ret () );
	gtk_window_set_title     ( window, _("Scanner") );
	gtk_window_set_modal     ( window, TRUE );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	g_signal_connect         ( window, "destroy", G_CALLBACK ( helia_scan_quit ), NULL );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	heliascan.notebook = (GtkNotebook *)gtk_notebook_new ();
	gtk_notebook_set_scrollable ( heliascan.notebook, TRUE );

	gtk_widget_set_margin_top    ( GTK_WIDGET ( heliascan.notebook ), 5 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( heliascan.notebook ), 5 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( heliascan.notebook ), 5 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( heliascan.notebook ), 5 );

	GtkBox *m_box_n[PAGE_NUM];

	guint j = 0;
	for ( j = 0; j < PAGE_NUM; j++ )
	{
		m_box_n[j] = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
		gtk_box_pack_start ( m_box_n[j], GTK_WIDGET ( helia_scan_all_box ( j ) ), TRUE, TRUE, 0 );
		gtk_notebook_append_page ( heliascan.notebook, GTK_WIDGET ( m_box_n[j] ),  gtk_label_new ( _(helia_scan_label_n[j].name) ) );

		if ( j == PAGE_SC )
			helia_scan_create_control_battons ( m_box_n[PAGE_SC] );
	}

	gtk_notebook_set_tab_pos ( heliascan.notebook, GTK_POS_TOP );
	gtk_box_pack_start ( m_box, GTK_WIDGET (heliascan.notebook), TRUE, TRUE, 0 );

	GtkButton *button = helia_service_ret_image_button ( "helia-exit", 16 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_scan_close ), GTK_WIDGET ( window ) );	
	gtk_box_pack_start ( h_box,  GTK_WIDGET ( button ), TRUE, TRUE, 5 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );
	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), hbgv.opacity_win );

	g_debug ( "helia_scan_win" );
}
