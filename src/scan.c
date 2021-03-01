/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "scan.h"
#include "default.h"
#include "file.h"
#include "level.h"
#include "button.h"

#include "mpegts.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>

/* DVB-T/T2, DVB-S/S2, DVB-C ( A/B/C ), DTMB */

typedef struct _DvbTypes DvbTypes;

struct _DvbTypes
{
	const char *param;
	gboolean descr;
	int8_t min;
	uint32_t max;
};

const DvbTypes dvbt_props_n[] =
{
	{ "Frequency  MHz", FALSE,  0,  5000000  }, // kHz ( 0 - 5  GHz )
	{ "Bandwidth  Hz",  FALSE,  0,  25000000 }, //  Hz ( 0 - 25 MHz )
	{ "Modulation",     TRUE,   0,  0 },
	{ "Inversion",      TRUE,   0,  0 },
	{ "Code Rate HP",   TRUE,   0,  0 },
	{ "Code Rate LP",   TRUE,   0,  0 },
	{ "Transmission",   TRUE,   0,  0 },
	{ "Guard interval", TRUE,   0,  0 },
	{ "Hierarchy",      TRUE,   0,  0 },
	{ "Stream ID",      FALSE, -1,  255 }
};

const DvbTypes dvbs_props_n[] =
{
	{ "Frequency  MHz",   FALSE,  0,  30000000 }, // kHz ( 0 - 30  GHz )
	{ "Symbol rate  kBd", FALSE,  0,  100000   }, // kBd ( 0 - 100 MBd )
	{ "Modulation",       TRUE,   0,  0 },
	{ "Polarity",         TRUE,   0,  0 },
	{ "Inner Fec",        TRUE,   0,  0 },
	{ "Pilot",            TRUE,   0,  0 },
	{ "Rolloff",          TRUE,   0,  0 },
	{ "Stream ID",        FALSE, -1,  255 },
	{ "DiSEqC",           TRUE,   0,  0 },
	{ "LNB",              TRUE,   0,  0 }
};

const DvbTypes dvbc_props_n[] =
{
	{ "Frequency  MHz",   FALSE,  0,  5000000 }, // kHz ( 0 - 5   GHz )
	{ "Symbol rate  kBd", FALSE,  0,  100000  }, // kBd ( 0 - 100 MBd )
	{ "Modulation",       TRUE,   0,  0 },
	{ "Inversion",        TRUE,   0,  0 },
	{ "Inner Fec",        TRUE,   0,  0 }
};

const DvbTypes dtmb_props_n[] =
{
	{ "Frequency  MHz", FALSE,  0,  5000000  }, // kHz ( 0 - 5  GHz )
	{ "Bandwidth  Hz",  FALSE,  0,  25000000 }, //  Hz ( 0 - 25 MHz )
	{ "Modulation",     TRUE,   0,  0 },
	{ "Inversion",      TRUE,   0,  0 },
	{ "Inner Fec",      TRUE,   0,  0 },
	{ "Transmission",   TRUE,   0,  0 },
	{ "Guard interval", TRUE,   0,  0 },
	{ "Interleaving",   TRUE,   0,  0 }
};

const DvbTypes atsc_props_n[] =
{
	{ "Frequency  MHz", FALSE,  0,  5000000  }, // kHz ( 0 - 5  GHz )
	{ "Modulation",     TRUE,   0,  0 }
};

typedef struct _DvbDescrAll DvbDescrAll;

struct _DvbDescrAll
{
	int descr_num;
	const char *dvb_v5_name;
	const char *text_vis;
};

const DvbDescrAll dvb_descr_delsys_type_n[] =
{
	{ SYS_UNDEFINED, 	"UNDEFINED", 		"Undefined" 	},
	{ SYS_DVBT, 		"DVBT",   		"DVB-T" 	},
	{ SYS_DVBT2, 		"DVBT2", 		"DVB-T2"	},
	{ SYS_DVBS, 		"DVBS",   		"DVB-S"		},
	{ SYS_DVBS2, 		"DVBS2",  		"DVB-S2"	},
	{ SYS_TURBO, 		"TURBO", 		"TURBO"		},
	{ SYS_DVBC_ANNEX_A, 	"DVBC/A", 		"DVB-C/A" 	},
	{ SYS_DVBC_ANNEX_B, 	"DVBC/B", 		"DVB-C/B" 	},
	{ SYS_DVBC_ANNEX_C, 	"DVBC/C", 		"DVB-C/C"	},
	{ SYS_DTMB, 		"DTMB", 		"DTMB"		}
};

const DvbDescrAll dvb_descr_inversion_type_n[] =
{
	{ INVERSION_OFF,  "OFF",  "Off"  },
	{ INVERSION_ON,   "ON",   "On"   },
	{ INVERSION_AUTO, "AUTO", "Auto" }
};

const DvbDescrAll dvb_descr_coderate_type_n[] =
{
	{ FEC_NONE, "NONE", "None" },
	{ FEC_1_2,  "1/2",  "1/2"  },
	{ FEC_2_3,  "2/3",  "2/3"  },
	{ FEC_3_4,  "3/4",  "3/4"  },
	{ FEC_4_5,  "4/5",  "4/5"  },
	{ FEC_5_6,  "5/6",  "5/6"  },
	{ FEC_6_7,  "6/7",  "6/7"  },
	{ FEC_7_8,  "7/8",  "7/8"  },
	{ FEC_8_9,  "8/9",  "8/9"  },
	{ FEC_AUTO, "AUTO", "Auto" },
	{ FEC_3_5,  "3/5",  "3/5"  },
	{ FEC_9_10, "9/10", "9/10" },
	{ FEC_2_5,  "2/5",  "2/5"  }
};

const DvbDescrAll dvb_descr_modulation_type_n[] =
{
	{ QPSK,     "QPSK",     "QPSK"     },
	{ QAM_16,   "QAM/16",   "QAM 16"   },
	{ QAM_32,   "QAM/32",   "QAM 32"   },
	{ QAM_64,   "QAM/64",   "QAM 64"   },
	{ QAM_128,  "QAM/128",  "QAM 128"  },
	{ QAM_256,  "QAM/256",  "QAM 256"  },
	{ QAM_AUTO, "QAM/AUTO", "Auto"     },
	{ VSB_8,    "VSB/8",    "VSB 8"    },
	{ VSB_16,   "VSB/16",   "VSB 16"   },
	{ PSK_8,    "PSK/8",    "PSK 8"    },
	{ APSK_16,  "APSK/16",  "APSK 16"  },
	{ APSK_32,  "APSK/32",  "APSK 32"  },
	{ DQPSK,    "DQPSK",    "DQPSK"    },
	{ QAM_4_NR, "QAM/4_NR", "QAM 4 NR" }
};

const DvbDescrAll dvb_descr_transmode_type_n[] =
{
	{ TRANSMISSION_MODE_2K,   "2K",   "2K"   },
	{ TRANSMISSION_MODE_8K,   "8K",   "8K"   },
	{ TRANSMISSION_MODE_AUTO, "AUTO", "Auto" },
	{ TRANSMISSION_MODE_4K,   "4K",   "4K"   },
	{ TRANSMISSION_MODE_1K,   "1K",   "1K"   },
	{ TRANSMISSION_MODE_16K,  "16K",  "16K"  },
	{ TRANSMISSION_MODE_32K,  "32K",  "32K"  },
	{ TRANSMISSION_MODE_C1,   "C1",   "C1"   },
	{ TRANSMISSION_MODE_C3780,"C3780","C3780"}
};

const DvbDescrAll dvb_descr_guard_type_n[] =
{
	{ GUARD_INTERVAL_1_32,   "1/32",   "32"     },
	{ GUARD_INTERVAL_1_16,   "1/16",   "16"     },
	{ GUARD_INTERVAL_1_8,    "1/8",    "8"      },
	{ GUARD_INTERVAL_1_4,    "1/4",    "4"      },
	{ GUARD_INTERVAL_AUTO,   "AUTO",   "Auto"   },
	{ GUARD_INTERVAL_1_128,  "1/128",  "128"    },
	{ GUARD_INTERVAL_19_128, "19/128", "19/128" },
	{ GUARD_INTERVAL_19_256, "19/256", "19/256" },
	{ GUARD_INTERVAL_PN420,  "PN420",  "PN 420" },
	{ GUARD_INTERVAL_PN595,  "PN595",  "PN 595" },
	{ GUARD_INTERVAL_PN945,  "PN945",  "PN 945" }
};

const DvbDescrAll dvb_descr_hierarchy_type_n[] =
{
	{ HIERARCHY_NONE, "NONE", "None" },
	{ HIERARCHY_1,    "1",    "1"    },
	{ HIERARCHY_2,    "2",    "2"    },
	{ HIERARCHY_4,    "4",    "4"    },
	{ HIERARCHY_AUTO, "AUTO", "Auto" }

};

const DvbDescrAll dvb_descr_pilot_type_n[] =
{
	{ PILOT_ON,   "ON",   "On"   },
	{ PILOT_OFF,  "OFF",  "Off"  },
	{ PILOT_AUTO, "AUTO", "Auto" }
};

const DvbDescrAll dvb_descr_roll_type_n[] =
{
	{ ROLLOFF_35,   "35",   "35"   },
	{ ROLLOFF_20,   "20",   "20"   },
	{ ROLLOFF_25,   "25",   "25"   },
	{ ROLLOFF_AUTO, "AUTO", "Auto" }
};

const DvbDescrAll dvb_descr_polarity_type_n[] =
{
	{ SEC_VOLTAGE_18,  "HORIZONTAL", "H  18V" },
	{ SEC_VOLTAGE_13,  "VERTICAL",   "V  13V" },
	{ SEC_VOLTAGE_18,  "LEFT",       "L  18V" },
	{ SEC_VOLTAGE_13,  "RIGHT",      "R  13V" }
	//  { SEC_VOLTAGE_OFF, "OFF",        "Off"    }
};

const DvbDescrAll dvb_descr_ileaving_type_n[] =
{
	{ INTERLEAVING_NONE, "NONE", "None" },
	{ INTERLEAVING_AUTO, "AUTO", "Auto" },
	{ INTERLEAVING_240,  "240",  "240"  },
	{ INTERLEAVING_720,  "720",  "720"  }
};

// LNB ( SAT )

enum lnbs
{
	LNB_UNV,
	LNB_DBS,
	LNB_EXT,
	LNB_STD,
	LNB_EHD,
	LNB_CBD,
	LNB_CMT,
	LNB_BJ1,
	LNB_QPH,
	LNB_BRO,
	LNB_MNL
};

const DvbDescrAll dvb_descr_lnb_type_n[] =
{
	{ LNB_UNV, "UNIVERSAL", "Universal" },
	{ LNB_DBS, "DBS",	"DBS"       },
	{ LNB_EXT, "EXTENDED",  "Extended"  },
	{ LNB_STD, "STANDARD",  "Standard"  },
	{ LNB_EHD, "ENHANCED",  "Enhanced"  },
	{ LNB_CBD, "C-BAND",    "C-Band"    },
	{ LNB_CMT, "C-MULT",    "C-Mult"    },
	{ LNB_BJ1, "110BS",	"110 BS"    },
	{ LNB_QPH, "QPH031",    "QPH031"    },
	{ LNB_BRO, "OI-BRASILSAT", "BrasilSat Oi" },
	{ LNB_MNL, "MANUAL", "Manual" }
};

typedef struct _LnbTypes LnbTypes;

struct _LnbTypes
{
	uint8_t descr_num;
	const char *name;

	uint32_t lo1_val;
	uint32_t lo2_val;
	uint32_t switch_val;
	uint32_t min_val;
	uint32_t max_val;
};

const LnbTypes lnb_type_lhs_n[] =
{
	{ LNB_UNV, "UNIVERSAL", 	9750000,  10600000, 11700000,   10700000, 12700000 },
	{ LNB_DBS, "DBS",		11250000, 0, 0, 		12200000, 12700000 },
	{ LNB_EXT, "EXTENDED",  	9750000,  10600000, 11700000,   10700000, 12750000 },
	{ LNB_STD, "STANDARD",		10000000, 0, 0, 		10945000, 11450000 },
	{ LNB_EHD, "ENHANCED",		9750000,  0, 0, 		10700000, 11700000 },
	{ LNB_CBD, "C-BAND",		5150000,  0, 0, 		3700000,  4200000  },
	{ LNB_CMT, "C-MULT",		5150000,  5750000,  0,		3700000,  4200000  },
	{ LNB_BJ1, "110BS",		10678000, 0, 0, 		11710000, 12751000 },
	{ LNB_QPH, "QPH031",    	10750000, 11250000, 12200000, 	11700000, 12700000 },
	{ LNB_BRO, "OI-BRASILSAT", 	10000000, 10445000, 11800000,   10950000, 12200000 },
	{ LNB_MNL, "MANUAL", 		9750,     0, 0, 0, 25000 } // MHz
};

const DvbDescrAll dvb_descr_diseqc_num_n[] =
{
	{-1, "NONE",    "None"  },
	{ 0, "LNB 1",   "Lnb 1" },
	{ 1, "LNB 2",   "Lnb 2" },
	{ 2, "LNB 3",   "Lnb 3" },
	{ 3, "LNB 4",   "Lnb 4" },
	{ 4, "LNB 5",   "Lnb 5" },
	{ 5, "LNB 6",   "Lnb 6" },
	{ 6, "LNB 7",   "Lnb 7" },
	{ 7, "LNB 8",	"Lnb 8" }
};



enum page_n
{
	PAGE_SC,
	PAGE_DT,
	PAGE_DS,
	PAGE_DC,
	PAGE_DM,
	PAGE_CH,
	PAGE_NUM
};

const struct ScanLabel { uint8_t page; const char *name; } scan_label_n[] =
{
	{ PAGE_SC, "Scanner"  },
	{ PAGE_DT, "DVB-T/T2" },
	{ PAGE_DS, "DVB-S/S2" },
	{ PAGE_DC, "DVB-C"    },
	{ PAGE_DM, "DTMB"     },
	{ PAGE_CH, "Channels" }
};

struct DvbDescrGstParam { const char *name; const char *dvb_v5_name; const char *gst_param; 
	const DvbDescrAll *dvb_descr; uint8_t cdsc; } param_dvb_n[] =
{
// descr
{ "Inversion",      "INVERSION",         "inversion",        dvb_descr_inversion_type_n,  G_N_ELEMENTS ( dvb_descr_inversion_type_n  ) },
{ "Code Rate HP",   "CODE_RATE_HP",      "code-rate-hp",     dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Code Rate LP",   "CODE_RATE_LP",      "code-rate-lp",     dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Inner Fec",      "INNER_FEC",         "code-rate-hp",     dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
{ "Modulation",     "MODULATION",        "modulation",       dvb_descr_modulation_type_n, G_N_ELEMENTS ( dvb_descr_modulation_type_n ) },
{ "Transmission",   "TRANSMISSION_MODE", "trans-mode",       dvb_descr_transmode_type_n,  G_N_ELEMENTS ( dvb_descr_transmode_type_n  ) },
{ "Guard interval", "GUARD_INTERVAL",    "guard",            dvb_descr_guard_type_n,      G_N_ELEMENTS ( dvb_descr_guard_type_n      ) },
{ "Hierarchy",      "HIERARCHY",         "hierarchy",        dvb_descr_hierarchy_type_n,  G_N_ELEMENTS ( dvb_descr_hierarchy_type_n  ) },
{ "Pilot",          "PILOT",             "pilot",            dvb_descr_pilot_type_n,      G_N_ELEMENTS ( dvb_descr_pilot_type_n      ) },
{ "Rolloff",        "ROLLOFF",           "rolloff",          dvb_descr_roll_type_n,       G_N_ELEMENTS ( dvb_descr_roll_type_n 	     ) },
{ "Polarity",       "POLARIZATION",      "polarity",         dvb_descr_polarity_type_n,   G_N_ELEMENTS ( dvb_descr_polarity_type_n   ) },
{ "LNB",            "LNB",               "lnb-type",         dvb_descr_lnb_type_n,        G_N_ELEMENTS ( dvb_descr_lnb_type_n 	     ) },
{ "DiSEqC",         "SAT_NUMBER",        "diseqc-source",    dvb_descr_diseqc_num_n,      G_N_ELEMENTS ( dvb_descr_diseqc_num_n      ) },
{ "Interleaving",   "INTERLEAVING",      "interleaving",     dvb_descr_ileaving_type_n,   G_N_ELEMENTS ( dvb_descr_ileaving_type_n   ) },

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



enum cols_scan_n
{
	COL_SCAN_NUM,
	COL_SCAN_CHNL,
	COL_SCAN_DATA,
	NUM_SCAN_COLS
};

struct _Scan
{
	GtkWindow parent_instance;

	GtkLabel *label_device;
	GtkTreeView *treeview;

	Level *level;

	GstElement *dvbscan;
	GstElement *dvbsrc;

	MpegTs *mpegts;

	uint8_t dvb_type;
	uint8_t lnb_type;

	gboolean debug;
};

G_DEFINE_TYPE ( Scan, scan, GTK_TYPE_WINDOW )

static void helia_convert_dvb5 ( const char *file, Scan *scan );
static uint8_t scan_get_dvb_delsys ( int adapter, int frontend );

static void scan_message_dialog ( const char *f_error, const char *file_or_info, GtkMessageType mesg_type, Scan *scan )
{
	GtkMessageDialog *dialog = ( GtkMessageDialog *)gtk_message_dialog_new (
					GTK_WINDOW ( scan ), GTK_DIALOG_MODAL,
					mesg_type, GTK_BUTTONS_CLOSE,
					"%s\n%s",  f_error, file_or_info );

	gtk_window_set_icon_name ( GTK_WINDOW (dialog), DEF_ICON );

	gtk_dialog_run     ( GTK_DIALOG ( dialog ) );
	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}



void set_lnb_lhs ( GstElement *element, int type_lnb )
{
	if ( type_lnb == LNB_MNL ) return;

	g_object_set ( element, "lnb-lof1", lnb_type_lhs_n[type_lnb].lo1_val,    NULL );
	g_object_set ( element, "lnb-lof2", lnb_type_lhs_n[type_lnb].lo2_val,    NULL );
	g_object_set ( element, "lnb-slof", lnb_type_lhs_n[type_lnb].switch_val, NULL );
}

static const char * scan_get_dvb_type_str ( uint8_t delsys )
{
	const char *dvb_type = "Undefined";

	uint8_t i = 0; for ( i = 0; i < G_N_ELEMENTS ( dvb_descr_delsys_type_n ); i++ )
	{
		if ( dvb_descr_delsys_type_n[i].descr_num == delsys )
			dvb_type = dvb_descr_delsys_type_n[i].text_vis;
	}

	return dvb_type;
}

const char * scan_get_info ( const char *data )
{
	const char *res = NULL;

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( param_dvb_n ); c++ )
	{
		if ( g_str_has_suffix ( data, param_dvb_n[c].gst_param ) )
		{
			res = param_dvb_n[c].name;
			break;
		}
	}

	return res;
}

const char * scan_get_info_descr_vis ( const char *data, int num )
{
	const char *res = NULL;

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( param_dvb_n ); c++ )
	{
		if ( param_dvb_n[c].cdsc == 0 ) continue;

		if ( g_str_has_suffix ( data, param_dvb_n[c].gst_param ) )
		{
			if ( g_str_has_prefix ( data, "diseqc-source" ) ) num += 1;

			res = param_dvb_n[c].dvb_descr[num].text_vis;
			break;
		}
	}

	return res;
}

static uint scan_get_lnb_lhs ( GstElement *element, const char *param )
{
	uint freq = 0;
	g_object_get ( element, param, &freq, NULL );

	return freq / 1000;
}

static void scan_lnb_changed_spin_all ( GtkSpinButton *button, GstElement *element )
{
	gtk_spin_button_update ( button );

	uint freq = (uint)gtk_spin_button_get_value ( button );

	g_object_set ( element, gtk_widget_get_name ( GTK_WIDGET ( button ) ), freq *= 1000, NULL );
}

static void scan_lnb_win ( G_GNUC_UNUSED GtkButton *button, Scan *scan )
{
	if ( scan->lnb_type != LNB_MNL ) return;

	GtkWindow *window = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title ( window, "" );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_icon_name ( window, DEF_ICON );
	gtk_window_set_transient_for ( window, GTK_WINDOW ( scan ) );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_default_size ( window, 400, -1 );

	GstElement *element = scan->dvbsrc;

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( grid, TRUE );
	gtk_grid_set_row_spacing ( grid, 5 );
	gtk_box_pack_start ( m_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	gtk_widget_set_visible ( GTK_WIDGET ( m_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( grid  ), TRUE );

	struct data_a { const char *text; const char *name; uint value; } data_a_n[] =
	{
		{ "LNB LOf1   MHz", "lnb-lof1", scan_get_lnb_lhs ( element, "lnb-lof1" ) },
		{ "LNB LOf2   MHz", "lnb-lof2", scan_get_lnb_lhs ( element, "lnb-lof2" ) },
		{ "LNB Switch MHz", "lnb-slof", scan_get_lnb_lhs ( element, "lnb-slof" ) }
	};

	uint8_t d = 0; for ( d = 0; d < G_N_ELEMENTS ( data_a_n ); d++ )
	{
		GtkLabel *label = (GtkLabel *)gtk_label_new ( data_a_n[d].text );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );

		gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
		gtk_grid_attach ( grid, GTK_WIDGET ( label ), 0, d, 1, 1 );

		GtkSpinButton *spinbutton = (GtkSpinButton *)gtk_spin_button_new_with_range ( lnb_type_lhs_n[LNB_MNL].min_val, lnb_type_lhs_n[LNB_MNL].max_val, 1 );
		gtk_widget_set_name ( GTK_WIDGET ( spinbutton ), data_a_n[d].name );
		gtk_spin_button_set_value ( spinbutton, data_a_n[d].value );
		g_signal_connect ( spinbutton, "changed", G_CALLBACK ( scan_lnb_changed_spin_all ), element );

		gtk_widget_set_visible ( GTK_WIDGET ( spinbutton ), TRUE );
		gtk_grid_attach ( grid, GTK_WIDGET ( spinbutton ), 1, d, 1, 1 );
	}

	GtkButton *button_close = helia_create_button ( NULL, "helia-exit", "🞬", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( button_close ), TRUE );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_end ( m_box, GTK_WIDGET ( button_close ), FALSE, FALSE, 0 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_window_present ( window );
}

static GtkBox * scan_create_lnb_m ( GtkComboBoxText *combo, Scan *scan )
{
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	GtkButton *button = (GtkButton *)gtk_button_new_with_label ( " 🤚 " );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	g_signal_connect ( button, "clicked", G_CALLBACK ( scan_lnb_win ), scan );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( combo  ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );

	return h_box;
}


static void scan_changed_spin_all ( GtkSpinButton *button, Scan *scan )
{
	gtk_spin_button_update ( button );

	GstElement *element = scan->dvbsrc;

	long val = (long)gtk_spin_button_get_value ( button );
	const char *name = gtk_widget_get_name ( GTK_WIDGET ( button ) );

	if ( g_str_has_prefix ( name, "Frequency" ) ) val *= ( val < 1000 ) ? 1000000 : 1000;

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( param_dvb_n ); c++ )
	{
		if ( g_str_has_suffix ( name, param_dvb_n[c].name ) )
		{
			g_object_set ( element, param_dvb_n[c].gst_param, val, NULL );

			if ( scan->debug ) g_message ( "name = %s | val = %ld | gst_param = %s ", name, val, param_dvb_n[c].gst_param );
		}
	}
}
static void scan_changed_combo_all ( GtkComboBox *combo_box, Scan *scan )
{
	int num = gtk_combo_box_get_active ( combo_box );
	const char *name = gtk_widget_get_name ( GTK_WIDGET ( combo_box ) );

	GstElement *element = scan->dvbsrc;

	if ( g_str_has_prefix ( name, "LNB" ) )
	{
		scan->lnb_type = (uint8_t)num;
		set_lnb_lhs ( element, num );

		if ( scan->debug ) g_message ( "name %s | set %s: %d ( low %u | high %u | switch %u )", name, lnb_type_lhs_n[num].name, num, 
			lnb_type_lhs_n[num].lo1_val, lnb_type_lhs_n[num].lo2_val, lnb_type_lhs_n[num].switch_val );

		return;
	}

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( param_dvb_n ); c++ )
	{
		if ( g_str_has_suffix ( name, param_dvb_n[c].name ) )
		{
			if ( g_str_has_prefix ( name, "Polarity" ) )
				g_object_set ( element, "polarity", ( num == 1 || num == 3 ) ? "V" : "H", NULL );
			else
				g_object_set ( element, param_dvb_n[c].gst_param, param_dvb_n[c].dvb_descr[num].descr_num, NULL );

			if ( scan->debug ) g_message ( "name = %s | num = %d | gst_param = %s | descr_text_vis = %s | descr_num = %d ", 
				name, num, param_dvb_n[c].gst_param, param_dvb_n[c].dvb_descr[num].text_vis,  param_dvb_n[c].dvb_descr[num].descr_num );
		}
	}
}

static GtkBox * scan_dvb_all ( uint8_t num, const DvbTypes *dvball, G_GNUC_UNUSED const char *type, Scan *scan )
{
	GtkBox *g_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 10 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( grid, TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	gtk_widget_set_visible ( GTK_WIDGET ( g_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( grid  ), TRUE );

	GtkLabel *label;
	GtkSpinButton *spinbutton;
	GtkComboBoxText *scan_combo_box;

	int d_data = 0;
	uint8_t d = 0, c = 0, z = 0;

	for ( d = 0; d < num; d++ )
	{
		label = (GtkLabel *)gtk_label_new ( dvball[d].param );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );

		gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
		gtk_grid_attach ( grid, GTK_WIDGET ( label ), 0, d, 1, 1 );

		if ( !dvball[d].descr )
		{
			spinbutton = (GtkSpinButton *) gtk_spin_button_new_with_range ( dvball[d].min, dvball[d].max, 1 );
			gtk_widget_set_name ( GTK_WIDGET ( spinbutton ), dvball[d].param );

			gtk_widget_set_visible ( GTK_WIDGET ( spinbutton ), TRUE );
			gtk_grid_attach ( grid, GTK_WIDGET ( spinbutton ), 1, d, 1, 1 );

			g_signal_connect ( spinbutton, "changed", G_CALLBACK ( scan_changed_spin_all ), scan );
		}
		else
		{
			scan_combo_box = (GtkComboBoxText *) gtk_combo_box_text_new ();
			gtk_widget_set_name ( GTK_WIDGET ( scan_combo_box ), dvball[d].param );
			gtk_widget_set_visible ( GTK_WIDGET ( scan_combo_box ), TRUE );

			if ( g_str_has_prefix ( dvball[d].param, "LNB" ) )
				gtk_grid_attach ( grid, GTK_WIDGET ( scan_create_lnb_m ( scan_combo_box, scan ) ), 1, d, 1, 1 );
			else
				gtk_grid_attach ( grid, GTK_WIDGET ( scan_combo_box ), 1, d, 1, 1 );

			for ( c = 0; c < G_N_ELEMENTS ( param_dvb_n ); c++ )
			{
				if ( g_str_has_suffix ( dvball[d].param, param_dvb_n[c].name ) )
				{
					for ( z = 0; z < param_dvb_n[c].cdsc; z++ )
						gtk_combo_box_text_append_text ( scan_combo_box, param_dvb_n[c].dvb_descr[z].text_vis );

					if ( g_str_has_prefix ( dvball[d].param, "Polarity" ) || g_str_has_prefix ( dvball[d].param, "LNB" ) ) continue;

					d_data = 0;
					g_object_get ( scan->dvbsrc, param_dvb_n[c].gst_param, &d_data, NULL );

					gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), d_data );
				}
			}

			if ( gtk_combo_box_get_active ( GTK_COMBO_BOX ( scan_combo_box ) ) == -1 )
				 gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), 0 );

			g_signal_connect ( scan_combo_box, "changed", G_CALLBACK ( scan_changed_combo_all ), scan );
		}
	}

	return g_box;
}



static void scan_set_new_adapter ( Scan *scan )
{
	GstElement *element = scan->dvbsrc;

	int frontend = 0, adapter = 0;
	g_object_get ( element, "adapter",  &adapter,  NULL );
	g_object_get ( element, "frontend", &frontend, NULL );

	scan->dvb_type = scan_get_dvb_delsys ( adapter, frontend );

	g_autofree char *dvb_name = scan_get_dvb_info ( adapter, frontend );
	gtk_label_set_text ( scan->label_device, dvb_name );
}

static void scan_set_adapter ( GtkSpinButton *button, Scan *scan )
{
	gtk_spin_button_update ( button );

	int adapter_set = gtk_spin_button_get_value_as_int ( button );

	GstElement *element = scan->dvbsrc;

	g_object_set ( element, "adapter",  adapter_set, NULL );

	scan_set_new_adapter ( scan );
}

static void scan_set_frontend ( GtkSpinButton *button, Scan *scan )
{
	gtk_spin_button_update ( button );

	int frontend_set = gtk_spin_button_get_value_as_int ( button );

	GstElement *element = scan->dvbsrc;

	g_object_set ( element, "frontend",  frontend_set, NULL );

	scan_set_new_adapter ( scan );
}

static void scan_set_delsys ( GtkComboBox *combo_box, Scan *scan )
{
	int num = gtk_combo_box_get_active ( combo_box );

	scan->dvb_type = (uint8_t)dvb_descr_delsys_type_n[num].descr_num;

	GstElement *element = scan->dvbsrc;

	g_object_set ( element, "delsys", scan->dvb_type, NULL );
}

static void scan_convert_file ( const char *file, Scan *scan )
{
	if ( file && g_str_has_suffix ( file, "channel.conf" ) )
	{
		if ( g_file_test ( file, G_FILE_TEST_EXISTS ) )
			helia_convert_dvb5 ( file, scan );
		else
			scan_message_dialog ( file, g_strerror ( errno ), GTK_MESSAGE_ERROR, scan );
	}
	else
	{
		g_warning ( "%s:: no convert %s ", __func__, file );
	}
}

static void scan_convert_set_file ( GtkEntry *entry, G_GNUC_UNUSED GtkEntryIconPosition icon_pos, G_GNUC_UNUSED GdkEventButton *event, Scan *scan )
{
	if ( icon_pos == GTK_ENTRY_ICON_PRIMARY )
	{
		char *file = helia_open_file ( g_get_home_dir (), GTK_WINDOW ( scan ) );

		if ( file == NULL ) return;

		gtk_entry_set_text ( entry, file );

		free ( file );
	}

	if ( icon_pos == GTK_ENTRY_ICON_SECONDARY )
	{
		const char *file = gtk_entry_get_text ( entry );

		scan_convert_file ( file, scan );
	}
}

static GtkBox * scan_convert ( Scan *scan )
{
	GtkBox *g_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( g_box ), TRUE );

	GtkLabel *label = (GtkLabel *)gtk_label_new ( "DVBv5   ⇨  GstDvbSrc" );
	gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( label ), FALSE, FALSE, 5 );

	GtkEntry *entry = (GtkEntry *)gtk_entry_new ();
	gtk_entry_set_text ( entry, "dvb_channel.conf" );
	gtk_widget_set_visible ( GTK_WIDGET ( entry ), TRUE );

	const char *icon = helia_check_icon_theme ( "helia-convert" ) ? "helia-convert" : "reload";

	g_object_set ( entry, "editable", FALSE, NULL );
	gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_PRIMARY, "folder" );
	gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, icon );
	g_signal_connect ( entry, "icon-press", G_CALLBACK ( scan_convert_set_file ), scan );

	gtk_box_pack_start ( g_box, GTK_WIDGET ( entry ), FALSE, FALSE, 5 );

	return g_box;
}

static GtkBox * scan_device ( Scan *scan )
{
	GstElement *element = scan->dvbsrc;

	int adapter = 0, frontend = 0;
	g_object_get ( element, "adapter",  &adapter, NULL );
	g_object_get ( element, "frontend", &frontend, NULL );

	scan->dvb_type = scan_get_dvb_delsys ( adapter, frontend );
	g_autofree char *dvb_name = scan_get_dvb_info ( adapter, frontend );

	GtkBox *g_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 10 );
	gtk_widget_set_visible ( GTK_WIDGET ( g_box ), TRUE );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( grid, TRUE );
	gtk_grid_set_row_spacing ( grid, 5 );

	gtk_widget_set_visible ( GTK_WIDGET ( grid ), TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	struct DataDevice { const char *text; int value; void (*f)(); } data_n[] =
	{
		{ dvb_name,     0, NULL },
		{ "Adapter",    adapter,  	scan_set_adapter  },
		{ "Frontend",   frontend, 	scan_set_frontend },
		{ "DelSys",     0, 		scan_set_delsys   }
	};

	uint8_t d = 0, c = 0, num = 0;
	for ( d = 0; d < G_N_ELEMENTS ( data_n ); d++ )
	{
		GtkLabel *label = (GtkLabel *)gtk_label_new ( data_n[d].text );
		gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), ( d == 0 ) ? GTK_ALIGN_CENTER : GTK_ALIGN_START );
		gtk_grid_attach ( grid, GTK_WIDGET ( label ), 0, d, ( d == 0 ) ? 2 : 1, 1 );

		if ( d == 0 ) scan->label_device = label;
		if ( d == 0 || d == 3 ) continue;

		GtkSpinButton *spinbutton = (GtkSpinButton *)gtk_spin_button_new_with_range ( 0, 16, 1 );
		gtk_spin_button_set_value ( spinbutton, data_n[d].value );
		g_signal_connect ( spinbutton, "changed", G_CALLBACK ( data_n[d].f ), scan );

		gtk_widget_set_visible ( GTK_WIDGET ( spinbutton ), TRUE );
		gtk_grid_attach ( grid, GTK_WIDGET ( spinbutton ), 1, d, 1, 1 );
	}

	GtkComboBoxText *combo_delsys = (GtkComboBoxText *) gtk_combo_box_text_new ();
	gtk_widget_set_visible ( GTK_WIDGET ( combo_delsys ), TRUE );

	for ( c = 0; c < G_N_ELEMENTS ( dvb_descr_delsys_type_n ); c++ )
	{
		if ( g_str_has_suffix ( scan_get_dvb_type_str ( scan->dvb_type ), dvb_descr_delsys_type_n[c].text_vis ) ) num = c;

		gtk_combo_box_text_append_text ( combo_delsys, dvb_descr_delsys_type_n[c].text_vis );
	}

	gtk_combo_box_set_active ( GTK_COMBO_BOX ( combo_delsys ), num );

	gtk_grid_attach ( grid, GTK_WIDGET ( combo_delsys ), 1, 3, 1, 1 );

	gtk_box_pack_start ( g_box, GTK_WIDGET ( scan_convert ( scan ) ), TRUE, TRUE, 10 );

	return g_box;
}



static void scan_save ( G_GNUC_UNUSED GtkButton *button, Scan *scan )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( scan->treeview );

	gboolean valid;
	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		char *name = NULL, *data = NULL;

		gtk_tree_model_get ( model, &iter, COL_SCAN_DATA, &data, -1 );
		gtk_tree_model_get ( model, &iter, COL_SCAN_CHNL, &name, -1 );

		g_signal_emit_by_name ( scan, "scan-append", name, data );

		free ( name );
		free ( data );
	}
}

static GtkBox * scan_channels ( Scan *scan )
{
	GtkBox *g_box  = (GtkBox *)gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 0 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( g_box ), TRUE );

	GtkScrolledWindow *sw = (GtkScrolledWindow *)gtk_scrolled_window_new ( NULL, NULL );
	gtk_scrolled_window_set_policy ( sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_widget_set_size_request ( GTK_WIDGET ( sw ), 220, -1 );
	gtk_widget_set_visible ( GTK_WIDGET ( sw ), TRUE );

	GtkListStore *store = (GtkListStore *)gtk_list_store_new ( 3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING );

	scan->treeview = (GtkTreeView *)gtk_tree_view_new_with_model ( GTK_TREE_MODEL ( store ) );
	gtk_widget_set_visible ( GTK_WIDGET ( scan->treeview ), TRUE );

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	struct Column { const char *name; const char *type; uint8_t num; } column_n[] =
	{
		{ "Num",      "text", COL_SCAN_NUM  },
		{ "Channels", "text", COL_SCAN_CHNL },
		{ "Data",     "text", COL_SCAN_DATA }
	};

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( column_n ); c++ )
	{
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes ( column_n[c].name, renderer, column_n[c].type, column_n[c].num, NULL );

		if ( c == COL_SCAN_DATA ) gtk_tree_view_column_set_visible ( column, FALSE );

		gtk_tree_view_append_column ( scan->treeview, column );
	}

	gtk_container_add ( GTK_CONTAINER ( sw ), GTK_WIDGET ( scan->treeview ) );
	g_object_unref ( G_OBJECT (store) );

	gtk_box_pack_start ( g_box, GTK_WIDGET ( sw ), TRUE, TRUE, 0 );

	GtkBox *hb_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( hb_box ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( hb_box ), 5 );
	gtk_box_set_spacing ( hb_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( hb_box ), TRUE );

	GtkButton *button = helia_create_button ( hb_box, "helia-clear", "🗑", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_list_store_clear ), GTK_LIST_STORE ( gtk_tree_view_get_model ( scan->treeview ) ) );

	button = helia_create_button ( hb_box, "helia-save", "🖴", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	g_signal_connect ( button, "clicked", G_CALLBACK ( scan_save ), scan );

	gtk_widget_set_visible ( GTK_WIDGET ( g_box ), TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( hb_box ), FALSE, FALSE, 5 );

	return g_box;
}

static GtkBox * scan_all_box ( uint8_t i, Scan *scan )
{
	GtkBox *only_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	if ( i == PAGE_SC )  { return scan_device   ( scan ); }
	if ( i == PAGE_CH )  { return scan_channels ( scan ); }
	if ( i == PAGE_DT )  { return scan_dvb_all  ( G_N_ELEMENTS ( dvbt_props_n  ), dvbt_props_n, "DVB-T", scan ); }
	if ( i == PAGE_DS )  { return scan_dvb_all  ( G_N_ELEMENTS ( dvbs_props_n  ), dvbs_props_n, "DVB-S", scan ); }
	if ( i == PAGE_DC )  { return scan_dvb_all  ( G_N_ELEMENTS ( dvbc_props_n  ), dvbc_props_n, "DVB-C", scan ); }
	if ( i == PAGE_DM )  { return scan_dvb_all  ( G_N_ELEMENTS ( dtmb_props_n  ), dtmb_props_n, "DTMB",  scan ); }

	return only_box;
}

static void scan_get_tp_data ( GString *gstring, Scan *scan )
{
	GstElement *element = scan->dvbsrc;

	uint8_t c = 0, d = 0, l = 0;
	int DVBTYPE = 0, d_data = 0, delsys = 0;

	g_object_get ( element, "delsys", &delsys, NULL );
	g_debug ( "%s: Gst delsys: %s ( %d ) ", __func__, scan_get_dvb_type_str ( (uint8_t)delsys ), delsys );

	// DVBTYPE = scan->dvb_type;
	DVBTYPE = delsys;

	const char *dvb_f[] = { "delsys", "adapter", "frontend" };

	for ( d = 0; d < G_N_ELEMENTS ( dvb_f ); d++ )
	{
		g_object_get ( element, dvb_f[d], &d_data, NULL );
		g_string_append_printf ( gstring, ":%s=%d", dvb_f[d], d_data );
	}

	if ( DVBTYPE == SYS_DVBT || DVBTYPE == SYS_DVBT2 )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dvbt_props_n ); c++ )
		{
			if ( DVBTYPE == SYS_DVBT )
				if ( g_str_has_prefix ( dvbt_props_n[c].param, "Stream ID" ) ) continue;

			for ( d = 0; d < G_N_ELEMENTS ( param_dvb_n ); d++ )
			{
				if ( g_str_has_suffix ( dvbt_props_n[c].param, param_dvb_n[d].name ) )
				{
					g_object_get ( element, param_dvb_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", param_dvb_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DTMB )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dtmb_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( param_dvb_n ); d++ )
			{
				if ( g_str_has_suffix ( dtmb_props_n[c].param, param_dvb_n[d].name ) )
				{
					g_object_get ( element, param_dvb_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", param_dvb_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DVBC_ANNEX_A || DVBTYPE == SYS_DVBC_ANNEX_C )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dvbc_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( param_dvb_n ); d++ )
			{
				if ( g_str_has_suffix ( dvbc_props_n[c].param, param_dvb_n[d].name ) )
				{
					g_object_get ( element, param_dvb_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", param_dvb_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DVBC_ANNEX_B )
	{
		for ( c = 0; c < G_N_ELEMENTS ( atsc_props_n ); c++ )
		{
			for ( d = 0; d < G_N_ELEMENTS ( param_dvb_n ); d++ )
			{
				if ( g_str_has_suffix ( atsc_props_n[c].param, param_dvb_n[d].name ) )
				{
					g_object_get ( element, param_dvb_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", param_dvb_n[d].gst_param, d_data );

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

			for ( d = 0; d < G_N_ELEMENTS ( param_dvb_n ); d++ )
			{
				if ( g_str_has_suffix ( dvbs_props_n[c].param, param_dvb_n[d].name ) )
				{
					if ( g_str_has_prefix ( "polarity", param_dvb_n[d].gst_param ) )
					{
						char *pol = NULL;
						g_object_get ( element, param_dvb_n[d].gst_param, &pol, NULL );

						g_string_append_printf ( gstring, ":polarity=%s", pol );

						free ( pol );

						continue;
					}

					if ( g_str_has_prefix ( "lnb-type", param_dvb_n[d].gst_param ) )
					{
						g_string_append_printf ( gstring, ":%s=%d", "lnb-type", scan->lnb_type );

						if ( scan->lnb_type == LNB_MNL )
						{
							const char *lnbf_gst[] = { "lnb-lof1", "lnb-lof2", "lnb-slof" };

							for ( l = 0; l < G_N_ELEMENTS ( lnbf_gst ); l++ )
							{
								g_object_get ( element, lnbf_gst[l], &d_data, NULL );
								g_string_append_printf ( gstring, ":%s=%d", lnbf_gst[l], d_data );
							}
						}

						continue;
					}

					g_object_get ( element, param_dvb_n[d].gst_param, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", param_dvb_n[d].gst_param, d_data );

					break;
				}
			}
		}
	}
}

static char * _strip_ch_name ( char *name )
{
	uint8_t i = 0; for ( i = 0; name[i] != '\0'; i++ )
	{
		if ( name[i] == ':' ) name[i] = ' ';
	}

	return g_strstrip ( name );
}

static void scan_set_data_to_treeview ( const char *ch_name, const char *ch_data, GtkTreeView *tree_view )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );

	gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter );
	gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter,
				COL_SCAN_NUM, ind + 1,
				COL_SCAN_CHNL, ch_name,
				COL_SCAN_DATA, ch_data,
				-1 );
}

static void scan_read_ch_to_treeview ( Scan *scan )
{
	if ( scan->mpegts->pmt_count == 0 ) return;

	GString *gstr_data = g_string_new ( NULL );

	scan_get_tp_data ( gstr_data, scan );

	uint8_t i = 0, c = 0;

	for ( i = 0; i < scan->mpegts->pmt_count; i++ )
	{
		char *ch_name = NULL;

		for ( c = 0; c < scan->mpegts->sdt_count; c++ )
			if ( scan->mpegts->pids[i].pmt_sid == scan->mpegts->pids[c].sdt_sid )
				{ ch_name = scan->mpegts->pids[c].ch_name; break; }

		GString *gstring = g_string_new ( NULL );

		if ( ch_name )
			ch_name = _strip_ch_name ( ch_name );
		else
			ch_name = g_strdup_printf ( "Program-%d", scan->mpegts->pids[i].pmt_sid );

		g_string_append_printf ( gstring, "%s:program-number=%d:video-pid=%d:audio-pid=%d%s", 
					ch_name,
					scan->mpegts->pids[i].pmt_sid, 
					scan->mpegts->pids[i].pmt_vpid,
					scan->mpegts->pids[i].pmt_apid, 
					gstr_data->str );

		gboolean play = FALSE;
		if ( GST_ELEMENT_CAST ( scan->dvbscan )->current_state == GST_STATE_PLAYING ) play = TRUE;

		if ( play && scan->mpegts->pids[i].pmt_apid != 0 ) // ignore other
			scan_set_data_to_treeview ( ch_name, gstring->str, scan->treeview );

		g_string_free ( gstring, TRUE );
		free ( ch_name );
	}

	g_string_free ( gstr_data, TRUE );
}

static void scan_stop ( G_GNUC_UNUSED GtkButton *button, Scan *scan )
{
	if ( GST_ELEMENT_CAST ( scan->dvbscan )->current_state == GST_STATE_NULL ) return;

	level_update ( 0, 0, FALSE, FALSE, 0, 0, scan->level );

	scan_read_ch_to_treeview ( scan );

	gst_element_set_state ( scan->dvbscan, GST_STATE_NULL );
}

static gboolean scan_start_time ( Scan *scan )
{
	if ( GST_ELEMENT_CAST ( scan->dvbscan )->current_state == GST_STATE_NULL ) return FALSE;

	if ( scan->mpegts->pat_done && scan->mpegts->pmt_done && scan->mpegts->sdt_done )
	{
		scan_stop ( NULL, scan );
		return FALSE;
	}

	return TRUE;
}

static void scan_start ( G_GNUC_UNUSED GtkButton *button, Scan *scan )
{
	if ( GST_ELEMENT_CAST ( scan->dvbscan )->current_state == GST_STATE_PLAYING ) return;

	mpegts_clear ( scan->mpegts );

	gst_element_set_state ( scan->dvbscan, GST_STATE_PLAYING );

	g_timeout_add_seconds ( 1, (GSourceFunc)scan_start_time, scan );
}

static void scan_create_control_battons ( GtkBox *box, Scan *scan )
{
	scan->level = level_new ();
	gtk_widget_set_margin_start ( GTK_WIDGET ( scan->level ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( scan->level ), 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( scan->level ), TRUE );
	gtk_box_pack_start ( box, GTK_WIDGET ( scan->level ), FALSE, FALSE, 0 );

	GtkBox *hb_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( hb_box ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( hb_box ), 5 );
	gtk_box_set_spacing ( hb_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( hb_box ), TRUE );

	GtkButton *button = helia_create_button ( hb_box, "helia-play", "⏵", 16 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( scan_start ), scan );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );

	button = helia_create_button ( hb_box, "helia-stop", "⏹", 16 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( scan_stop ), scan );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );

	gtk_box_pack_start ( box, GTK_WIDGET ( hb_box ), FALSE, FALSE, 5 );
}

static void scan_msg_all ( G_GNUC_UNUSED GstBus *bus, GstMessage *message, Scan *scan )
{
	if ( GST_ELEMENT_CAST ( scan->dvbscan )->current_state != GST_STATE_PLAYING ) return;

	const GstStructure *structure = gst_message_get_structure ( message );

	if ( structure )
	{
		int signal, snr;
		gboolean lock = FALSE;

		if (  gst_structure_get_int ( structure, "signal", &signal )  )
		{
			gst_structure_get_boolean ( structure, "lock", &lock );
			gst_structure_get_int ( structure, "snr", &snr);

			uint8_t ret_sgl = (uint8_t)(signal*100/0xffff);
			uint8_t ret_snr = (uint8_t)(snr*100/0xffff);

			level_update ( ret_sgl, ret_snr, lock, FALSE, 0, 0, scan->level );
		}
	}

	mpegts_parse_section ( message, scan->mpegts );
}

static void scan_msg_err ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Scan *scan )
{
	scan_stop ( NULL, scan );

	GError *err = NULL;
	char  *dbg = NULL;

	gst_message_parse_error ( msg, &err, &dbg );

	g_critical ( "%s:: %s (%s)\n", __func__, err->message, (dbg) ? dbg : "no details" );

	scan_message_dialog ( "", err->message, GTK_MESSAGE_ERROR, scan );

	g_error_free ( err );
	g_free ( dbg );
}

static void scan_set_tune_timeout ( GstElement *element, guint64 time_set )
{
	guint64 timeout = 0, timeout_set = 0, timeout_get = 0, timeout_def = 10000000000;

	g_object_get ( element, "tuning-timeout", &timeout, NULL );

	timeout_set = timeout_def / 10 * time_set;

	g_object_set ( element, "tuning-timeout", (guint64)timeout_set, NULL );

	g_object_get ( element, "tuning-timeout", &timeout_get, NULL );

	g_debug ( "scan_set_tune_timeout: timeout %ld | timeout set %ld", timeout, timeout_get );
}

static void scan_create ( Scan *scan )
{
	mpegts_initialize ();

	scan->mpegts = g_new0 ( MpegTs, 1 );
	scan->mpegts->debug = scan->debug;

	GstElement *tsparse, *filesink;

	scan->dvbscan = gst_pipeline_new ( "pipeline-scan" );
	scan->dvbsrc  = gst_element_factory_make ( "dvbsrc",   NULL );
	tsparse       = gst_element_factory_make ( "tsparse",  NULL );
	filesink      = gst_element_factory_make ( "fakesink", NULL );

	if ( !scan->dvbscan || !scan->dvbsrc || !tsparse || !filesink )
		g_critical ( "%s:: pipeline scan - not be created.\n", __func__ );

	gst_bin_add_many ( GST_BIN ( scan->dvbscan ), scan->dvbsrc, tsparse, filesink, NULL );
	gst_element_link_many ( scan->dvbsrc, tsparse, filesink, NULL );

	GstBus *bus_scan = gst_element_get_bus ( scan->dvbscan );
	gst_bus_add_signal_watch ( bus_scan );

	g_signal_connect ( bus_scan, "message",        G_CALLBACK ( scan_msg_all ), scan );
	g_signal_connect ( bus_scan, "message::error", G_CALLBACK ( scan_msg_err ), scan );

	gst_object_unref ( bus_scan );

	scan_set_tune_timeout ( scan->dvbsrc, 5 );
}

static void scan_window_quit ( G_GNUC_UNUSED GtkWindow *window, Scan *scan )
{
	gst_element_set_state ( scan->dvbscan, GST_STATE_NULL );

	g_object_unref ( scan->dvbscan );

	free ( scan->mpegts );
}

static void scan_init ( Scan *scan )
{
	scan->debug = ( g_getenv ( "DVB_DEBUG" ) ) ? TRUE : FALSE;
	scan_create ( scan );

	GtkWindow *window = GTK_WINDOW ( scan );

	gtk_window_set_title ( window, "" );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_icon_name ( window, DEF_ICON );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_default_size ( window, 300, 400 );
	g_signal_connect ( window, "destroy", G_CALLBACK ( scan_window_quit ), scan );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_widget_set_visible ( GTK_WIDGET ( m_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	GtkNotebook *notebook = (GtkNotebook *)gtk_notebook_new ();
	gtk_notebook_set_scrollable ( notebook, TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( notebook ), TRUE );

	gtk_widget_set_margin_top    ( GTK_WIDGET ( notebook ), 5 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( notebook ), 5 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( notebook ), 5 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( notebook ), 5 );

	GtkBox *m_box_n[PAGE_NUM];

	uint8_t j = 0; for ( j = 0; j < PAGE_NUM; j++ )
	{
		m_box_n[j] = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
		gtk_widget_set_visible ( GTK_WIDGET ( m_box_n[j] ), TRUE );

		gtk_box_pack_start ( m_box_n[j], GTK_WIDGET ( scan_all_box ( j, scan ) ), TRUE, TRUE, 0 );

		gtk_notebook_append_page ( notebook, GTK_WIDGET ( m_box_n[j] ), gtk_label_new ( scan_label_n[j].name ) );

		if ( j == PAGE_SC ) scan_create_control_battons ( m_box_n[PAGE_SC], scan );
	}

	gtk_notebook_set_tab_pos ( notebook, GTK_POS_TOP );
	gtk_box_pack_start ( m_box, GTK_WIDGET (notebook), TRUE, TRUE, 0 );

	GtkButton *button = helia_create_button ( NULL, "helia-exit", "🞬", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE,  TRUE,  5 );
	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box  ), FALSE, FALSE, 5 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
}

static void scan_finalize ( GObject *object )
{
	G_OBJECT_CLASS (scan_parent_class)->finalize (object);
}

static void scan_class_init ( ScanClass *class )
{
	G_OBJECT_CLASS (class)->finalize = scan_finalize;

	g_signal_new ( "scan-append", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING );
}

Scan * scan_new ( GtkWindow *win_base )
{
	Scan *scan = g_object_new ( SCAN_TYPE_WIN, NULL );

	GtkWindow *window = GTK_WINDOW ( scan );

	gtk_window_set_transient_for ( window, win_base );
	gtk_window_present ( window );

	double opacity = gtk_widget_get_opacity ( GTK_WIDGET ( win_base ) );
	gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity );

	return scan;
}



static uint32_t scan_get_dvb_delsys_fd ( int fd, struct dvb_frontend_info info )
{
	uint32_t dtv_del_sys = SYS_UNDEFINED, SYS_DVBC = SYS_DVBC_ANNEX_A, dtv_api_ver = 0;

	struct dtv_property dvb_prop[2];
	struct dtv_properties cmdseq;

	dvb_prop[0].cmd = DTV_DELIVERY_SYSTEM;
	dvb_prop[1].cmd = DTV_API_VERSION;

	cmdseq.num = 2;
	cmdseq.props = dvb_prop;

	if ( ( ioctl ( fd, FE_GET_PROPERTY, &cmdseq ) ) == -1 )
	{
		perror ( "scan_get_dvb_delsys_fd: ioctl FE_GET_PROPERTY " );

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
			g_debug ( "DVBv3  Ok " );
		else
			g_critical ( "DVBv3  Failed \n" );
	}
	else
	{
		g_debug ( "DVBv5  Ok " );

		dtv_del_sys = dvb_prop[0].u.data;
		dtv_api_ver = dvb_prop[1].u.data;
	}

	g_debug ( "DVB DTV_DELIVERY_SYSTEM: %d | DVB API Version: %d.%d ", dtv_del_sys, dtv_api_ver / 256, dtv_api_ver % 256 );

	return dtv_del_sys;
}

static uint8_t scan_get_dvb_delsys ( int adapter, int frontend )
{
	uint8_t dtv_delsys = SYS_UNDEFINED;

	int fd = 0, flags = O_RDWR;

	char path[80];
	sprintf ( path, "/dev/dvb/adapter%d/frontend%d", adapter, frontend );

	if ( ( fd = open ( path, flags ) ) == -1 )
	{
		flags = O_RDONLY;

		if ( ( fd = open ( path, flags ) ) == -1 )
		{
			g_critical ( "%s: %s %s \n", __func__, path, g_strerror ( errno ) );
			return dtv_delsys;
		}
	}

	struct dvb_frontend_info info;

	if ( ( ioctl ( fd, FE_GET_INFO, &info ) ) == -1 )
		perror ( "scan_get_dvb_delsys: ioctl FE_GET_INFO " );
	else
		dtv_delsys = (uint8_t)scan_get_dvb_delsys_fd ( fd, info );

	close ( fd );

	return dtv_delsys;
}

char * scan_get_dvb_info ( int adapter, int frontend )
{
	int fd = 0, flags = O_RDWR;

	char path[80];
	sprintf ( path, "/dev/dvb/adapter%d/frontend%d", adapter, frontend );

	if ( ( fd = open ( path, flags ) ) == -1 )
	{
		flags = O_RDONLY;

		if ( ( fd = open ( path, flags ) ) == -1 )
		{
			g_critical ( "%s: %s %s \n", __func__, path, g_strerror ( errno ) );

			return g_strdup ( "Undefined" );
		}
	}

	struct dvb_frontend_info info;

	if ( ( ioctl ( fd, FE_GET_INFO, &info ) ) == -1 )
	{
		perror ( "scan_get_dvb_info: ioctl FE_GET_INFO " );

		close ( fd );

		return g_strdup ( "Undefined" );
	}

	g_debug ( "DVB device: %s ( %s ) ", info.name, path );

	close ( fd );

	return g_strdup ( info.name );
}

void helia_init_dvb ( int adapter, int frontend )
{
	char path[80];
	sprintf ( path, "/dev/dvb/adapter%d/frontend%d", adapter, frontend );

	if ( g_file_test ( path, G_FILE_TEST_EXISTS ) )
	{
		g_autofree char *dvb_name = scan_get_dvb_info ( adapter, frontend );

		uint8_t dvb_type = scan_get_dvb_delsys ( adapter, frontend );

		g_debug ( "%s ( %s ) | %s ", dvb_name, scan_get_dvb_type_str ( dvb_type ), path );
	}
}



// Convert  DVBv5 ⇨ GstDvbSrc

static char * _strip_ch_name_convert ( char *name )
{
	uint i = 0;
	for ( i = 0; name[i] != '\0'; i++ )
	{
		if ( name[i] == ':' || name[i] == '[' || name[i] == ']' ) name[i] = ' ';
	}
	return g_strstrip ( name );
}

static void helia_read_dvb5_data ( const char *ch_data, uint num, int adapter, int frontend, int delsys, Scan *scan )
{
	uint n = 0, z = 0, x = 0;

	char **data = g_strsplit ( ch_data, "\n", 0 );

	if ( data[0] != NULL && *data[0] )
	{
		GString *gstring = g_string_new ( _strip_ch_name_convert ( data[0] ) );
		g_string_append_printf ( gstring, ":delsys=%d:adapter=%d:frontend=%d", delsys, adapter, frontend );

		g_debug ( "Channel ( %d ): %s ", num, data[0] );

		for ( n = 1; data[n] != NULL && *data[n]; n++ )
		{
			char **value_key = g_strsplit ( data[n], " = ", 0 );

			for ( z = 0; z < G_N_ELEMENTS ( param_dvb_n ); z++ )
			{
				if ( g_strrstr ( param_dvb_n[z].dvb_v5_name, g_strstrip ( value_key[0] ) ) )
				{
					g_string_append_printf ( gstring, ":%s=", param_dvb_n[z].gst_param );

					if ( param_dvb_n[z].cdsc == 0 || g_strrstr ( value_key[0], "SAT_NUMBER" ) )
					{
						g_string_append ( gstring, value_key[1] );
					}
					else
					{
						for ( x = 0; x < param_dvb_n[z].cdsc; x++ )
							if ( g_strrstr ( value_key[1], param_dvb_n[z].dvb_descr[x].dvb_v5_name ) )
								g_string_append_printf ( gstring, "%d", param_dvb_n[z].dvb_descr[x].descr_num );
					}

					// g_debug ( "  %s = %s ", param_dvb_n[z].gst_param, value_key[1] );
				}
			}

			g_strfreev ( value_key );
		}

		if ( g_strrstr ( gstring->str, "audio-pid" ) ) // ignore other
			g_signal_emit_by_name ( scan, "scan-append", data[0], gstring->str );

		g_string_free ( gstring, TRUE );
	}

	g_strfreev ( data );
}

static void helia_convert_dvb5 ( const char *file, Scan *scan )
{
	char *contents;
	GError *err = NULL;

	uint n = 0;
	int adapter = 0, frontend = 0, delsys = 0;
	GstElement *element = scan->dvbsrc;

	g_object_get ( element, "adapter",  &adapter,  NULL );
	g_object_get ( element, "frontend", &frontend, NULL );

	delsys = scan_get_dvb_delsys ( adapter, frontend );

	if ( g_file_get_contents ( file, &contents, 0, &err ) )
	{
		char **lines = g_strsplit ( contents, "[", 0 );
		uint length = g_strv_length ( lines );

		for ( n = 1; n < length; n++ )
			helia_read_dvb5_data ( lines[n], n, adapter, frontend, delsys, scan );

		g_strfreev ( lines );
		g_free ( contents );
	}
	else
	{
		scan_message_dialog ( "", err->message, GTK_MESSAGE_ERROR, scan );
		g_error_free ( err );

		return;
	}
}
