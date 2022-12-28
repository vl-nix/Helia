/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "convert.h"

#include <linux/dvb/frontend.h>

typedef struct _CDDescrAll CDDescrAll;

struct _CDDescrAll
{
	uint8_t descr;
	const char *name;
};

const CDDescrAll cdvb_descr_delsys_type_n[] =
{
	{ SYS_UNDEFINED, 	"UNDEFINED" },
	{ SYS_DVBT, 		"DVBT"      },
	{ SYS_DVBT2, 		"DVBT2"     },
	{ SYS_DVBS, 		"DVBS"      },
	{ SYS_DVBS2, 		"DVBS2"     },
	{ SYS_TURBO, 		"TURBO"     },
	{ SYS_DVBC_ANNEX_A, "DVBC/A"    },
	{ SYS_DVBC_ANNEX_B, "DVBC/B"    },
	{ SYS_DVBC_ANNEX_C, "DVBC/C"    },
	{ SYS_ATSC,         "ATSC"      },
	{ SYS_DTMB, 		"DTMB"      },
	{ SYS_ISDBT, 		"ISDBT"	    },
	{ SYS_ISDBS, 		"ISDBS"	    }
};

const CDDescrAll cdvb_descr_inversion_type_n[] =
{
	{ INVERSION_OFF,  "OFF"  },
	{ INVERSION_ON,   "ON"   },
	{ INVERSION_AUTO, "AUTO" }
};

const CDDescrAll cdvb_descr_coderate_type_n[] =
{
	{ FEC_NONE, "NONE" },
	{ FEC_1_2,  "1/2"  },
	{ FEC_2_3,  "2/3"  },
	{ FEC_3_4,  "3/4"  },
	{ FEC_4_5,  "4/5"  },
	{ FEC_5_6,  "5/6"  },
	{ FEC_6_7,  "6/7"  },
	{ FEC_7_8,  "7/8"  },
	{ FEC_8_9,  "8/9"  },
	{ FEC_AUTO, "AUTO" },
	{ FEC_3_5,  "3/5"  },
	{ FEC_9_10, "9/10" },
	{ FEC_2_5,  "2/5"  }
};

const CDDescrAll cdvb_descr_modulation_type_n[] =
{
	{ QPSK,     "QPSK"     },
	{ QAM_16,   "QAM/16"   },
	{ QAM_32,   "QAM/32"   },
	{ QAM_64,   "QAM/64"   },
	{ QAM_128,  "QAM/128"  },
	{ QAM_256,  "QAM/256"  },
	{ QAM_AUTO, "QAM/AUTO" },
	{ VSB_8,    "VSB/8"    },
	{ VSB_16,   "VSB/16"   },
	{ PSK_8,    "PSK/8"    },
	{ APSK_16,  "APSK/16"  },
	{ APSK_32,  "APSK/32"  },
	{ DQPSK,    "DQPSK"    },
	{ QAM_4_NR, "QAM/4_NR" }
};

const CDDescrAll cdvb_descr_transmode_type_n[] =
{
	{ TRANSMISSION_MODE_2K,   "2K"    },
	{ TRANSMISSION_MODE_8K,   "8K"    },
	{ TRANSMISSION_MODE_AUTO, "AUTO"  },
	{ TRANSMISSION_MODE_4K,   "4K"    },
	{ TRANSMISSION_MODE_1K,   "1K"    },
	{ TRANSMISSION_MODE_16K,  "16K"   },
	{ TRANSMISSION_MODE_32K,  "32K"   },
	{ TRANSMISSION_MODE_C1,   "C1"    },
	{ TRANSMISSION_MODE_C3780,"C3780" }
};

const CDDescrAll cdvb_descr_guard_type_n[] =
{
	{ GUARD_INTERVAL_1_32,   "1/32"   },
	{ GUARD_INTERVAL_1_16,   "1/16"   },
	{ GUARD_INTERVAL_1_8,    "1/8"    },
	{ GUARD_INTERVAL_1_4,    "1/4"    },
	{ GUARD_INTERVAL_AUTO,   "AUTO"   },
	{ GUARD_INTERVAL_1_128,  "1/128"  },
	{ GUARD_INTERVAL_19_128, "19/128" },
	{ GUARD_INTERVAL_19_256, "19/256" },
	{ GUARD_INTERVAL_PN420,  "PN420"  },
	{ GUARD_INTERVAL_PN595,  "PN595"  },
	{ GUARD_INTERVAL_PN945,  "PN945"  }
};

const CDDescrAll cdvb_descr_hierarchy_type_n[] =
{
	{ HIERARCHY_NONE, "NONE" },
	{ HIERARCHY_1,    "1"    },
	{ HIERARCHY_2,    "2"    },
	{ HIERARCHY_4,    "4"    },
	{ HIERARCHY_AUTO, "AUTO" }
};

const CDDescrAll cdvb_descr_pilot_type_n[] =
{
	{ PILOT_ON,   "ON"   },
	{ PILOT_OFF,  "OFF"  },
	{ PILOT_AUTO, "AUTO" }
};

const CDDescrAll cdvb_descr_roll_type_n[] =
{
	{ ROLLOFF_35,   "35"   },
	{ ROLLOFF_20,   "20"   },
	{ ROLLOFF_25,   "25"   },
	{ ROLLOFF_AUTO, "AUTO" }
};

const CDDescrAll cdvb_descr_polarity_type_n[] =
{
	{ SEC_VOLTAGE_18,  "HORIZONTAL" },
	{ SEC_VOLTAGE_13,  "VERTICAL"   },
	{ SEC_VOLTAGE_18,  "LEFT"       },
	{ SEC_VOLTAGE_13,  "RIGHT"      }
	// { SEC_VOLTAGE_OFF, "OFF"        }
};

const CDDescrAll cdvb_descr_ileaving_type_n[] =
{
	{ INTERLEAVING_NONE, "NONE" },
	{ INTERLEAVING_AUTO, "AUTO" },
	{ INTERLEAVING_240,  "240"  },
	{ INTERLEAVING_720,  "720"  }
};

const CDDescrAll cdvb_descr_lnb_type_n[] =
{
	{ LNB_UNV, "UNIVERSAL"    },
	{ LNB_DBS, "DBS"          },
	{ LNB_EXT, "EXTENDED"     },
	{ LNB_STD, "STANDARD"     },
	{ LNB_EHD, "ENHANCED"     },
	{ LNB_CBD, "C-BAND"       },
	{ LNB_CMT, "C-MULT"       },
	{ LNB_BJ1, "110BS"        },
	{ LNB_QPH, "QPH031"       },
	{ LNB_BRO, "OI-BRASILSAT" },
	{ LNB_MNL, "MANUAL"       }
};

struct CDDescrGstProp { const char *name; const char *gst_prop; const CDDescrAll *descrs; uint8_t cdsc; } prop_dvb_n[] =
{
	{ "INVERSION",         "inversion",        cdvb_descr_inversion_type_n,  G_N_ELEMENTS ( cdvb_descr_inversion_type_n  ) },
	{ "CODE_RATE_HP",      "code-rate-hp",     cdvb_descr_coderate_type_n,   G_N_ELEMENTS ( cdvb_descr_coderate_type_n   ) },
	{ "CODE_RATE_LP",      "code-rate-lp",     cdvb_descr_coderate_type_n,   G_N_ELEMENTS ( cdvb_descr_coderate_type_n   ) },
	{ "INNER_FEC",         "code-rate-hp",     cdvb_descr_coderate_type_n,   G_N_ELEMENTS ( cdvb_descr_coderate_type_n   ) },
	{ "MODULATION",        "modulation",       cdvb_descr_modulation_type_n, G_N_ELEMENTS ( cdvb_descr_modulation_type_n ) },
	{ "TRANSMISSION_MODE", "trans-mode",       cdvb_descr_transmode_type_n,  G_N_ELEMENTS ( cdvb_descr_transmode_type_n  ) },
	{ "GUARD_INTERVAL",    "guard",            cdvb_descr_guard_type_n,      G_N_ELEMENTS ( cdvb_descr_guard_type_n      ) },
	{ "HIERARCHY",         "hierarchy",        cdvb_descr_hierarchy_type_n,  G_N_ELEMENTS ( cdvb_descr_hierarchy_type_n  ) },
	{ "PILOT",             "pilot",            cdvb_descr_pilot_type_n,      G_N_ELEMENTS ( cdvb_descr_pilot_type_n      ) },
	{ "ROLLOFF",           "rolloff",          cdvb_descr_roll_type_n,       G_N_ELEMENTS ( cdvb_descr_roll_type_n 	     ) },
	{ "POLARIZATION",      "polarity",         cdvb_descr_polarity_type_n,   G_N_ELEMENTS ( cdvb_descr_polarity_type_n   ) },
	{ "LNB",               "lnb-type",         cdvb_descr_lnb_type_n,        G_N_ELEMENTS ( cdvb_descr_lnb_type_n 	     ) },
	{ "INTERLEAVING",      "interleaving",     cdvb_descr_ileaving_type_n,   G_N_ELEMENTS ( cdvb_descr_ileaving_type_n   ) },
	{ "DELIVERY_SYSTEM",   "delsys",           cdvb_descr_delsys_type_n,     G_N_ELEMENTS ( cdvb_descr_delsys_type_n     ) },

	{ "FREQUENCY",         "frequency",        NULL, 0 },
	{ "BANDWIDTH_HZ",      "bandwidth-hz",     NULL, 0 },
	{ "SYMBOL_RATE",       "symbol-rate",      NULL, 0 },
	{ "SAT_NUMBER",        "diseqc-source",    NULL, 0 },
	{ "STREAM_ID",         "stream-id",        NULL, 0 },
	{ "SERVICE_ID",        "program-number",   NULL, 0 },
	// { "AUDIO_PID",         "audio-pid",        NULL, 0 },
	// { "VIDEO_PID",         "video-pid",        NULL, 0 },

	{ "ISDBT_LAYER_ENABLED",            "isdbt-layer-enabled",            NULL, 0 },
	{ "ISDBT_PARTIAL_RECEPTION",        "isdbt-partial-reception",        NULL, 0 },
	{ "ISDBT_SOUND_BROADCASTING",       "isdbt-sound-broadcasting",       NULL, 0 },
	{ "ISDBT_SB_SUBCHANNEL_ID",         "isdbt-sb-subchannel-id",         NULL, 0 },
	{ "ISDBT_SB_SEGMENT_IDX",           "isdbt-sb-segment-idx",           NULL, 0 },
	{ "ISDBT_SB_SEGMENT_COUNT",         "isdbt-sb-segment-count",         NULL, 0 },
	{ "ISDBT_LAYERA_FEC",               "isdbt-layera-fec",               cdvb_descr_coderate_type_n,   G_N_ELEMENTS ( cdvb_descr_coderate_type_n   ) },
	{ "ISDBT_LAYERA_MODULATION",        "isdbt-layera-modulation",        cdvb_descr_modulation_type_n, G_N_ELEMENTS ( cdvb_descr_modulation_type_n ) },
	{ "ISDBT_LAYERA_SEGMENT_COUNT",     "isdbt-layera-segment-count",     NULL, 0  },
	{ "ISDBT_LAYERA_TIME_INTERLEAVING", "isdbt-layera-time-interleaving", NULL, 0  },
	{ "ISDBT_LAYERB_FEC",               "isdbt-layerb-fec",               cdvb_descr_coderate_type_n,   G_N_ELEMENTS ( cdvb_descr_coderate_type_n   ) },
	{ "ISDBT_LAYERB_MODULATION",        "isdbt-layerb-modulation",        cdvb_descr_modulation_type_n, G_N_ELEMENTS ( cdvb_descr_modulation_type_n ) },
	{ "ISDBT_LAYERB_SEGMENT_COUNT",     "isdbt-layerb-segment-count",     NULL, 0  },
	{ "ISDBT_LAYERB_TIME_INTERLEAVING", "isdbt-layerb-time-interleaving", NULL, 0  },
	{ "ISDBT_LAYERC_FEC",               "isdbt-layerc-fec",               cdvb_descr_coderate_type_n,   G_N_ELEMENTS ( cdvb_descr_coderate_type_n   ) },
	{ "ISDBT_LAYERC_MODULATION",        "isdbt-layerc-modulation",        cdvb_descr_modulation_type_n, G_N_ELEMENTS ( cdvb_descr_modulation_type_n ) },
	{ "ISDBT_LAYERC_SEGMENT_COUNT",     "isdbt-layerc-segment-count",     NULL, 0  },
	{ "ISDBT_LAYERC_TIME_INTERLEAVING", "isdbt-layerc-time-interleaving", NULL, 0  }
};


struct _Convert
{
	GObject parent_instance;
};

G_DEFINE_TYPE ( Convert, convert, G_TYPE_OBJECT )

static void convert_strip_ch_name ( char *name )
{
	uint i = 0; for ( i = 0; name[i] != '\0'; i++ )
	{
		if ( name[i] == ':' || name[i] == '/' || name[i] == '\'' || name[i] == '[' || name[i] == ']' ) name[i] = ' ';
	}

	g_strstrip ( name );
}

static uint8_t convert_get_delsys ( uint length, char **lines )
{
	uint8_t ret = SYS_UNDEFINED;

	uint i = 0; for ( i = 0; i < length; i++ )
	{
		if ( g_strrstr ( lines[i], "DELIVERY_SYSTEM" ) )
		{
			char **value_key = g_strsplit ( lines[i], " = ", 0 );

			uint d = 0; for ( d = 0; d < G_N_ELEMENTS ( cdvb_descr_delsys_type_n ); d++ )
			{
				if ( g_str_has_suffix ( value_key[1], cdvb_descr_delsys_type_n[d].name ) )
					{ ret = cdvb_descr_delsys_type_n[d].descr; break; }
			}

			g_strfreev ( value_key );
		}
	}
	return ret;
}

static void convert_handler ( G_GNUC_UNUSED Convert *convert, uint adapter, uint frontend, const char *data, gpointer gpstr_name, gpointer gpstr_data )
{
	uint n = 0, z = 0, x = 0;

	GString *gstr_name = gpstr_name;
	GString *gstr_data = gpstr_data;

	char **lines = g_strsplit ( data, "\n", 0 );
	uint length = g_strv_length ( lines );

	uint8_t delsys = convert_get_delsys ( length, lines );

	convert_strip_ch_name ( lines[0] );
	char *channel = lines[0];

	if ( delsys == SYS_UNDEFINED ) g_warning ( "%s: DelSys: SYS_UNDEFINED ", __func__ );

	g_string_append_printf ( gstr_name, "%s", channel );
	g_string_append_printf ( gstr_data, "%s:delsys=%d:adapter=%d:frontend=%d", channel, delsys, adapter, frontend );

	for ( n = 1; n < length; n++ )
	{
		gboolean f_prop = FALSE;

		if ( lines[n] && g_strrstr ( lines[n], " = " ) ) f_prop = TRUE;

		if ( !f_prop ) continue;

		char **value_key = g_strsplit ( lines[n], " = ", 0 );

		for ( z = 0; z < G_N_ELEMENTS ( prop_dvb_n ); z++ )
		{
			if ( g_strrstr ( value_key[0], prop_dvb_n[z].name ) )
			{
				g_string_append_printf ( gstr_data, ":%s=", prop_dvb_n[z].gst_prop );

				if ( prop_dvb_n[z].cdsc == 0 )
				{
					g_string_append ( gstr_data, value_key[1] );
				}
				else
				{
					gboolean found = FALSE;

					for ( x = 0; x < prop_dvb_n[z].cdsc; x++ )
						if ( g_strrstr ( value_key[1], prop_dvb_n[z].descrs[x].name ) )
							{ g_string_append_printf ( gstr_data, "%d", prop_dvb_n[z].descrs[x].descr ); found = TRUE; break; }

					if ( !found && g_strrstr ( value_key[0], "LNB" ) ) g_string_append_printf ( gstr_data, "%u", LNB_MNL );

					if ( !found ) g_warning ( "%s: %s | Not Set: %s = %s ", __func__, channel, value_key[0], value_key[1] );
				}

				g_debug ( "  %s = %s ", prop_dvb_n[z].gst_prop, value_key[1] );
			}
		}

		g_strfreev ( value_key );
	}

	g_strfreev ( lines );
}

static void convert_init ( Convert *convert )
{
	g_signal_connect ( convert, "convert", G_CALLBACK ( convert_handler ), NULL );
}

static void convert_finalize ( GObject *object )
{
	G_OBJECT_CLASS ( convert_parent_class )->finalize ( object );
}

static void convert_class_init ( ConvertClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = convert_finalize;

	g_signal_new ( "convert", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 5, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER );
}

Convert * convert_new ( void )
{
	Convert *convert = g_object_new ( CONVERT_TYPE_OBJECT, NULL );

	return convert;
}
