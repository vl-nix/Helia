/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "descr.h"
#include "scan.h"

#include <stdlib.h>
#include <gst/gst.h>

#include <linux/dvb/frontend.h>

struct _Descr
{
	GObject parent_instance;
};

G_DEFINE_TYPE ( Descr, descr, G_TYPE_OBJECT )

typedef struct _DvbDescrAll DvbDescrAll;

struct _DvbDescrAll
{
	int descr;
	const char *text;
};

const DvbDescrAll dvb_descr_delsys_type_n[] =
{
	{ SYS_UNDEFINED, 	"Undefined" },
	{ SYS_DVBT, 		"DVB-T" 	},
	{ SYS_DVBT2, 		"DVB-T2"	},
	{ SYS_DVBS, 		"DVB-S"		},
	{ SYS_DVBS2, 		"DVB-S2"	},
	{ SYS_TURBO, 		"TURBO"		},
	{ SYS_DVBC_ANNEX_A, "DVB-C/A" 	},
	{ SYS_DVBC_ANNEX_B, "DVB-C/B" 	},
	{ SYS_DVBC_ANNEX_C, "DVB-C/C"	},
	{ SYS_ATSC, 		"ATSC" 		},
	{ SYS_DTMB, 		"DTMB" 		}
};

const DvbDescrAll dvb_descr_inversion_type_n[] =
{
	{ INVERSION_OFF,  "Off"  },
	{ INVERSION_ON,   "On"   },
	{ INVERSION_AUTO, "Auto" }
};

const DvbDescrAll dvb_descr_coderate_type_n[] =
{
	{ FEC_NONE, "None" },
	{ FEC_1_2,  "1/2"  },
	{ FEC_2_3,  "2/3"  },
	{ FEC_3_4,  "3/4"  },
	{ FEC_4_5,  "4/5"  },
	{ FEC_5_6,  "5/6"  },
	{ FEC_6_7,  "6/7"  },
	{ FEC_7_8,  "7/8"  },
	{ FEC_8_9,  "8/9"  },
	{ FEC_AUTO, "Auto" },
	{ FEC_3_5,  "3/5"  },
	{ FEC_9_10, "9/10" },
	{ FEC_2_5,  "2/5"  }
};

const DvbDescrAll dvb_descr_modulation_type_n[] =
{
	{ QPSK,     "QPSK"     },
	{ QAM_16,   "QAM 16"   },
	{ QAM_32,   "QAM 32"   },
	{ QAM_64,   "QAM 64"   },
	{ QAM_128,  "QAM 128"  },
	{ QAM_256,  "QAM 256"  },
	{ QAM_AUTO, "Auto"     },
	{ VSB_8,    "VSB 8"    },
	{ VSB_16,   "VSB 16"   },
	{ PSK_8,    "PSK 8"    },
	{ APSK_16,  "APSK 16"  },
	{ APSK_32,  "APSK 32"  },
	{ DQPSK,    "DQPSK"    },
	{ QAM_4_NR, "QAM 4 NR" }
};

const DvbDescrAll dvb_descr_transmode_type_n[] =
{
	{ TRANSMISSION_MODE_2K,    "2K"   },
	{ TRANSMISSION_MODE_8K,    "8K"   },
	{ TRANSMISSION_MODE_AUTO,  "Auto" },
	{ TRANSMISSION_MODE_4K,    "4K"   },
	{ TRANSMISSION_MODE_1K,    "1K"   },
	{ TRANSMISSION_MODE_16K,   "16K"  },
	{ TRANSMISSION_MODE_32K,   "32K"  },
	{ TRANSMISSION_MODE_C1,    "C1"   },
	{ TRANSMISSION_MODE_C3780, "C3780"}
};

const DvbDescrAll dvb_descr_guard_type_n[] =
{
	{ GUARD_INTERVAL_1_32,   "32"     },
	{ GUARD_INTERVAL_1_16,   "16"     },
	{ GUARD_INTERVAL_1_8,    "8"      },
	{ GUARD_INTERVAL_1_4,    "4"      },
	{ GUARD_INTERVAL_AUTO,   "Auto"   },
	{ GUARD_INTERVAL_1_128,  "128"    },
	{ GUARD_INTERVAL_19_128, "19/128" },
	{ GUARD_INTERVAL_19_256, "19/256" },
	{ GUARD_INTERVAL_PN420,  "PN 420" },
	{ GUARD_INTERVAL_PN595,  "PN 595" },
	{ GUARD_INTERVAL_PN945,  "PN 945" }
};

const DvbDescrAll dvb_descr_hierarchy_type_n[] =
{
	{ HIERARCHY_NONE, "None" },
	{ HIERARCHY_1,    "1"    },
	{ HIERARCHY_2,    "2"    },
	{ HIERARCHY_4,    "4"    },
	{ HIERARCHY_AUTO, "Auto" }
};

const DvbDescrAll dvb_descr_pilot_type_n[] =
{
	{ PILOT_ON,   "On"   },
	{ PILOT_OFF,  "Off"  },
	{ PILOT_AUTO, "Auto" }
};

const DvbDescrAll dvb_descr_roll_type_n[] =
{
	{ ROLLOFF_35,   "35"   },
	{ ROLLOFF_20,   "20"   },
	{ ROLLOFF_25,   "25"   },
	{ ROLLOFF_AUTO, "Auto" }
};

const DvbDescrAll dvb_descr_polarity_type_n[] =
{
	{ SEC_VOLTAGE_13,  "V / R 13V" },
	{ SEC_VOLTAGE_18,  "H / L 18V" }
	// { SEC_VOLTAGE_OFF, "Off"       }
};

const DvbDescrAll dvb_descr_ileaving_type_n[] =
{
	{ INTERLEAVING_NONE, "None" },
	{ INTERLEAVING_AUTO, "Auto" },
	{ INTERLEAVING_240,  "240"  },
	{ INTERLEAVING_720,  "720"  }
};

const DvbDescrAll dvb_descr_lnb_type_n[] =
{
	{ LNB_UNV, "Universal" },
	{ LNB_DBS, "DBS"       },
	{ LNB_EXT, "Extended"  },
	{ LNB_STD, "Standard"  },
	{ LNB_EHD, "Enhanced"  },
	{ LNB_CBD, "C-Band"    },
	{ LNB_CMT, "C-Mult"    },
	{ LNB_BJ1, "110 BS"    },
	{ LNB_QPH, "QPH031"    },
	{ LNB_BRO, "BrasilSat Oi" },
	{ LNB_MNL, "Manual"    }
};

enum dsqs
{
	LNN = -1,
	LN0 =  0,
	LN1,
	LN2,
	LN3,
	LN4,
	LN5,
	LN6,
	LN7
};

const DvbDescrAll dvb_descr_diseqc_num_n[] =
{
	{ LNN, "None"  },
	{ LN0, "Lnb 1" },
	{ LN1, "Lnb 2" },
	{ LN2, "Lnb 3" },
	{ LN3, "Lnb 4" },
	{ LN4, "Lnb 5" },
	{ LN5, "Lnb 6" },
	{ LN6, "Lnb 7" },
	{ LN7, "Lnb 8" }
};

enum DescrNum
{
	NON,
	INV,
	FEC,
	MDL,
	TRM,
	GRD,
	HRR,
	PLT,
	RLF,
	POL,
	LNB,
	DSC,
	INT,
	NUM
};

struct DvbDescrNumType { enum DescrNum descr; const DvbDescrAll *dvb_descr; uint8_t cdsc; } dvb_descr_n[] =
{
	{ NON, NULL, 0 },
	{ INV, dvb_descr_inversion_type_n,  G_N_ELEMENTS ( dvb_descr_inversion_type_n  ) },
	{ FEC, dvb_descr_coderate_type_n,   G_N_ELEMENTS ( dvb_descr_coderate_type_n   ) },
	{ MDL, dvb_descr_modulation_type_n, G_N_ELEMENTS ( dvb_descr_modulation_type_n ) },
	{ TRM, dvb_descr_transmode_type_n,  G_N_ELEMENTS ( dvb_descr_transmode_type_n  ) },
	{ GRD, dvb_descr_guard_type_n,      G_N_ELEMENTS ( dvb_descr_guard_type_n      ) },
	{ HRR, dvb_descr_hierarchy_type_n,  G_N_ELEMENTS ( dvb_descr_hierarchy_type_n  ) },
	{ PLT, dvb_descr_pilot_type_n,      G_N_ELEMENTS ( dvb_descr_pilot_type_n      ) },
	{ RLF, dvb_descr_roll_type_n,       G_N_ELEMENTS ( dvb_descr_roll_type_n 	   ) },
	{ POL, dvb_descr_polarity_type_n,   G_N_ELEMENTS ( dvb_descr_polarity_type_n   ) },
	{ LNB, dvb_descr_lnb_type_n,        G_N_ELEMENTS ( dvb_descr_lnb_type_n 	   ) },
	{ DSC, dvb_descr_diseqc_num_n,      G_N_ELEMENTS ( dvb_descr_diseqc_num_n      ) },
	{ INT, dvb_descr_ileaving_type_n,   G_N_ELEMENTS ( dvb_descr_ileaving_type_n   ) }
};

typedef struct _DvbTypes DvbTypes;

struct _DvbTypes
{
	const char *name;
	const char *gst_prop;
	enum DescrNum descr;
	int16_t min;
	uint32_t max;
};

const DvbTypes dvbs_props_n[] =
{
	{ "Frequency  MHz",   "frequency",     NON,  0,  30000000 },
	{ "Symbol rate  kBd", "symbol-rate",   NON,  0,  100000   },
	{ "Modulation",       "modulation",    MDL,  0,  0 },
	{ "Polarity",         "polarity",      POL,  0,  0 },
	{ "Inner Fec",        "code-rate-hp",  FEC,  0,  0 },
	{ "Pilot",            "pilot",         PLT,  0,  0 },
	{ "Rolloff",          "rolloff",       RLF,  0,  0 },
	{ "Stream Id",        "stream-id",     NON, -1,  255 },
	{ "DiSEqC",           "diseqc-source", DSC,  0,  0 },
	{ "Lnb",              "lnb-type",      LNB,  0,  0 }
};

const DvbTypes dvbt_props_n[] =
{
	{ "Frequency  MHz", "frequency",    NON,  0,  5000000  },
	{ "Bandwidth  Hz",  "bandwidth-hz", NON,  0,  25000000 },
	{ "Modulation",     "modulation",   MDL,  0,  0 },
	{ "Inversion",      "inversion",    INV,  0,  0 },
	{ "Code Rate HP",   "code-rate-hp", FEC,  0,  0 },
	{ "Code Rate LP",   "code-rate-lp", FEC,  0,  0 },
	{ "Transmission",   "trans-mode",   TRM,  0,  0 },
	{ "Guard interval", "guard",        GRD,  0,  0 },
	{ "Hierarchy",      "hierarchy",    HRR,  0,  0 },
	{ "Stream Id",      "stream-id",    NON, -1,  255 }
};

const DvbTypes dvbc_props_n[] =
{
	{ "Frequency  MHz",   "frequency",    NON,  0,  5000000 },
	{ "Symbol rate  kBd", "symbol-rate",  NON,  0,  100000  },
	{ "Modulation",       "modulation",   MDL,  0,  0 },
	{ "Inversion",        "inversion",    INV,  0,  0 },
	{ "Inner Fec",        "code-rate-hp", FEC,  0,  0 }
};

const DvbTypes atsc_props_n[] =
{
	{ "Frequency  MHz", "frequency",  NON,  0,  5000000 },
	{ "Modulation",     "modulation", MDL,  0,  0 }
};

const DvbTypes dtmb_props_n[] =
{
	{ "Frequency  MHz", "frequency",    NON,  0,  5000000 },
	{ "Bandwidth  Hz",  "symbol-rate",  NON,  0,  100000  },
	{ "Modulation",     "modulation",   MDL,  0,  0 },
	{ "Inversion",      "inversion",    INV,  0,  0 },
	{ "Inner Fec",      "code-rate-hp", FEC,  0,  0 },
	{ "Transmission",   "trans-mode",   TRM,  0,  0 },
	{ "Guard interval", "guard",        GRD,  0,  0 },
	{ "Interleaving",   "interleaving", INT,  0,  0 }
};

typedef struct _LnbTypes LnbTypes;

struct _LnbTypes
{
	uint8_t descr;
	const char *name;

	uint32_t lo1_val;
	uint32_t lo2_val;
	uint32_t switch_val;
};

const LnbTypes lnb_type_lhs_n[] =
{
	{ LNB_UNV, "Universal", 	9750000,  10600000, 11700000 },
	{ LNB_DBS, "DBS",			11250000, 0,        0 	     },
	{ LNB_EXT, "Extended",  	9750000,  10600000, 11700000 },
	{ LNB_STD, "Standard",		10000000, 0,        0, 		 },
	{ LNB_EHD, "Enhanced",		9750000,  0,        0, 		 },
	{ LNB_CBD, "C-Band",		5150000,  0,        0, 		 },
	{ LNB_CMT, "C-Mult",		5150000,  5750000,  0,	     },
	{ LNB_BJ1, "110 BS",		10678000, 0,        0, 		 },
	{ LNB_QPH, "QPH031",    	10750000, 11250000, 12200000 },
	{ LNB_BRO, "BrasilSat Oi", 	10000000, 10445000, 11800000 },
	{ LNB_MNL, "Manual", 		9750,     0,        0,       }
};

static void descr_lnb_set_lhs ( uint type_lnb, GstElement *element )
{
	g_object_set ( element, "lnb-lof1", lnb_type_lhs_n[type_lnb].lo1_val,    NULL );
	g_object_set ( element, "lnb-lof2", lnb_type_lhs_n[type_lnb].lo2_val,    NULL );
	g_object_set ( element, "lnb-slof", lnb_type_lhs_n[type_lnb].switch_val, NULL );

	g_debug ( "%s:: Set: %s ", __func__, lnb_type_lhs_n[type_lnb].name );
}

static uint descr_lnb_get_type ( GstElement *element )
{
	uint ret = LNB_MNL, freq_l1 = 0, freq_l2 = 0, freq_sw = 0;

	g_object_get ( element, "lnb-lof1", &freq_l1, "lnb-lof2", &freq_l2, "lnb-slof", &freq_sw, NULL );

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( lnb_type_lhs_n ); c++ )
	{
		if ( lnb_type_lhs_n[c].lo1_val == freq_l1 && lnb_type_lhs_n[c].lo2_val == freq_l2 && lnb_type_lhs_n[c].switch_val ) { ret = lnb_type_lhs_n[c].descr; break; }
	}

	g_debug ( "%s:: Type %s, %u, %u, %u ", __func__, lnb_type_lhs_n[ret].name, freq_l1, freq_l2, freq_sw );

	return ret;
}

static uint descr_lnb_get_lhs ( GstElement *element, const char *param )
{
	uint freq = 0;
	g_object_get ( element, param, &freq, NULL );

	return freq / 1000;
}

static void descr_lnb_changed_spin_all ( GtkSpinButton *button, GstElement *element )
{
	gtk_spin_button_update ( button );

	uint freq = (uint)gtk_spin_button_get_value ( button );

	g_object_set ( element, gtk_widget_get_name ( GTK_WIDGET ( button ) ), freq *= 1000, NULL );
}

static void descr_lnb_win ( GstElement *element, GtkWindow *win_base )
{
	GtkWindow *window = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title ( window, "" );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_icon_name ( window, "helia" );
	gtk_window_set_transient_for ( window, win_base );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_default_size ( window, 400, -1 );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( grid, TRUE );
	gtk_grid_set_row_spacing ( grid, 5 );
	gtk_box_pack_start ( m_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	gtk_widget_set_visible ( GTK_WIDGET ( m_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( grid  ), TRUE );

	struct data_a { const char *text; const char *name; uint value; } data_a_n[] =
	{
		{ "LNB LOf1   MHz", "lnb-lof1", descr_lnb_get_lhs ( element, "lnb-lof1" ) },
		{ "LNB LOf2   MHz", "lnb-lof2", descr_lnb_get_lhs ( element, "lnb-lof2" ) },
		{ "LNB Switch MHz", "lnb-slof", descr_lnb_get_lhs ( element, "lnb-slof" ) }
	};

	uint8_t d = 0; for ( d = 0; d < G_N_ELEMENTS ( data_a_n ); d++ )
	{
		GtkLabel *label = (GtkLabel *)gtk_label_new ( data_a_n[d].text );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );

		gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
		gtk_grid_attach ( grid, GTK_WIDGET ( label ), 0, d, 1, 1 );

		GtkSpinButton *spinbutton = (GtkSpinButton *)gtk_spin_button_new_with_range ( 0, 25000, 1 );
		gtk_widget_set_name ( GTK_WIDGET ( spinbutton ), data_a_n[d].name );
		gtk_spin_button_set_value ( spinbutton, data_a_n[d].value );
		g_signal_connect ( spinbutton, "changed", G_CALLBACK ( descr_lnb_changed_spin_all ), element );

		gtk_widget_set_visible ( GTK_WIDGET ( spinbutton ), TRUE );
		gtk_grid_attach ( grid, GTK_WIDGET ( spinbutton ), 1, d, 1, 1 );
	}

	GtkButton *button_close = (GtkButton *)gtk_button_new_from_icon_name ( "helia-close", GTK_ICON_SIZE_MENU );
	gtk_widget_set_visible ( GTK_WIDGET ( button_close ), TRUE );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_end ( m_box, GTK_WIDGET ( button_close ), FALSE, FALSE, 0 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_window_present ( window );
}

static void descr_handler_lnbs ( G_GNUC_UNUSED Descr *descr, uint num, gpointer data )
{
	GstElement *dvbsrc = data;

	if ( num == LNB_MNL ) return;

	descr_lnb_set_lhs ( num, dvbsrc );
}

static void descr_scan_changed_spin_all ( GtkSpinButton *button, GstElement *dvbsrc )
{
	gtk_spin_button_update ( button );

	long val = (long)gtk_spin_button_get_value ( button );
	const char *prop = gtk_widget_get_name ( GTK_WIDGET ( button ) );

	if ( g_str_has_prefix ( prop, "frequency" ) ) val *= ( val < 1000 ) ? 1000000 : 1000;

	g_object_set ( dvbsrc, prop, val, NULL );

	g_debug ( "%s:: prop = %s | val = %ld ", __func__, prop, val );
}

static void descr_scan_changed_combo_all ( GtkComboBox *combo_box, GstElement *dvbsrc )
{
	int set_num = gtk_combo_box_get_active ( combo_box );
	const char *str_id = gtk_combo_box_get_active_id ( combo_box );
	const char *prop = gtk_widget_get_name ( GTK_WIDGET ( combo_box ) );

	uint8_t dsr_n = (uint8_t)atoi ( str_id );

	if ( dvb_descr_n[dsr_n].descr == LNB )
	{
		GtkWindow *win_base = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( combo_box ) ) );

		if ( lnb_type_lhs_n[set_num].descr == LNB_MNL ) descr_lnb_win ( dvbsrc, win_base ); else descr_lnb_set_lhs ( ( uint8_t )set_num, dvbsrc ); 

		return;
	}

	if ( dvb_descr_n[dsr_n].descr == POL )
		g_object_set ( dvbsrc, prop, ( set_num == 0 ) ? "V" : "H", NULL );
	else
		g_object_set ( dvbsrc, prop, dvb_descr_n[dsr_n].dvb_descr[set_num].descr, NULL );

	g_debug ( "%s:: prop = %s | set_num = %d | set = %s ( %d ) ", __func__, prop, set_num, dvb_descr_n[dsr_n].dvb_descr[set_num].text, dvb_descr_n[dsr_n].dvb_descr[set_num].descr );
}

static void descr_scan_set_data ( uint8_t num, const DvbTypes *dvb_types, GtkBox *v_box, GstElement *dvbsrc )
{
	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( grid, TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( grid  ), TRUE );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	gtk_widget_set_margin_start ( GTK_WIDGET ( grid ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( grid ), 10 );

	GtkLabel *label;
	GtkSpinButton *spinbutton;
	GtkComboBoxText *scan_combo_box;

	uint8_t d = 0, c = 0, z = 0;

	for ( d = 0; d < num; d++ )
	{
		label = (GtkLabel *)gtk_label_new ( dvb_types[d].name );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );

		gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
		gtk_grid_attach ( grid, GTK_WIDGET ( label ), 0, d, 1, 1 );

		if ( !dvb_types[d].descr )
		{
			spinbutton = (GtkSpinButton *) gtk_spin_button_new_with_range ( dvb_types[d].min, dvb_types[d].max, 1 );
			gtk_widget_set_name ( GTK_WIDGET ( spinbutton ), dvb_types[d].gst_prop );

			gtk_widget_set_visible ( GTK_WIDGET ( spinbutton ), TRUE );
			gtk_grid_attach ( grid, GTK_WIDGET ( spinbutton ), 1, d, 1, 1 );

			if ( dvbsrc ) g_signal_connect ( spinbutton, "changed", G_CALLBACK ( descr_scan_changed_spin_all ), dvbsrc );
		}
		else
		{
			scan_combo_box = (GtkComboBoxText *) gtk_combo_box_text_new ();
			gtk_widget_set_name ( GTK_WIDGET ( scan_combo_box ), dvb_types[d].gst_prop );
			gtk_widget_set_visible ( GTK_WIDGET ( scan_combo_box ), TRUE );

			gtk_grid_attach ( grid, GTK_WIDGET ( scan_combo_box ), 1, d, 1, 1 );

			for ( c = 0; c < NUM; c++ )
			{
				if ( dvb_types[d].descr == dvb_descr_n[c].descr )
				{
					char buf[5];
					sprintf ( buf, "%u", c );

					for ( z = 0; z < dvb_descr_n[c].cdsc; z++ ) gtk_combo_box_text_append ( scan_combo_box, buf, dvb_descr_n[c].dvb_descr[z].text );

					if ( dvb_types[d].descr == POL ) { gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), 1 ); continue; }
					if ( dvb_types[d].descr == LNB ) { gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), 0 ); continue; }

					int d_data = 0;
					if ( dvbsrc ) g_object_get ( dvbsrc, dvb_types[d].gst_prop, &d_data, NULL );

					gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), d_data );
				}
			}

			if ( gtk_combo_box_get_active ( GTK_COMBO_BOX ( scan_combo_box ) ) == -1 ) gtk_combo_box_set_active ( GTK_COMBO_BOX ( scan_combo_box ), 0 );

			if ( dvbsrc ) g_signal_connect ( scan_combo_box, "changed", G_CALLBACK ( descr_scan_changed_combo_all ), dvbsrc );
		}
	}
}

static void descr_handler_scan ( G_GNUC_UNUSED Descr *descr, uint page, GObject *obj, gpointer data )
{
	GstElement *dvbsrc = data;
	GtkBox *v_box = GTK_BOX ( obj );

	if ( page == PAGE_DT ) descr_scan_set_data ( G_N_ELEMENTS ( dvbt_props_n ), dvbt_props_n, v_box, dvbsrc );
	if ( page == PAGE_DS ) descr_scan_set_data ( G_N_ELEMENTS ( dvbs_props_n ), dvbs_props_n, v_box, dvbsrc );
	if ( page == PAGE_DC ) descr_scan_set_data ( G_N_ELEMENTS ( dvbc_props_n ), dvbc_props_n, v_box, dvbsrc );
	if ( page == PAGE_AT ) descr_scan_set_data ( G_N_ELEMENTS ( atsc_props_n ), atsc_props_n, v_box, dvbsrc );
	if ( page == PAGE_DM ) descr_scan_set_data ( G_N_ELEMENTS ( dtmb_props_n ), dtmb_props_n, v_box, dvbsrc );
}

static void descr_handler_get_tp ( G_GNUC_UNUSED Descr *descr, gpointer string, gpointer data )
{
	GstElement *element = data;
	GString *gstring = string;

	uint8_t c = 0, d = 0;
	int DVBTYPE = 0, d_data = 0, delsys = 0;

	g_object_get ( element, "delsys", &delsys, NULL );

	g_debug ( "%s: delsys: %d ", __func__, delsys );

	DVBTYPE = delsys;

	const char *dvb_f[] = { "delsys", "adapter", "frontend" };

	for ( d = 0; d < G_N_ELEMENTS ( dvb_f ); d++ )
	{
		g_object_get ( element, dvb_f[d], &d_data, NULL );
		g_string_append_printf ( gstring, ":%s=%d", dvb_f[d], d_data );
	}

	if ( DVBTYPE == SYS_DVBS || DVBTYPE == SYS_TURBO || DVBTYPE == SYS_DVBS2 )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dvbs_props_n ); c++ )
		{
			if ( DVBTYPE == SYS_TURBO )
				if ( dvbs_props_n[c].descr == PLT || dvbs_props_n[c].descr == RLF || g_str_has_prefix ( dvbs_props_n[c].name, "Stream Id" ) ) continue;

			if ( DVBTYPE == SYS_DVBS )
				if ( dvbs_props_n[c].descr == MDL || dvbs_props_n[c].descr == PLT || dvbs_props_n[c].descr == RLF || g_str_has_prefix ( dvbs_props_n[c].name, "Stream Id" ) ) continue;

			for ( d = 0; d < NUM; d++ )
			{
				if ( dvbs_props_n[c].descr == dvb_descr_n[d].descr )
				{
					if ( dvb_descr_n[d].descr == POL )
					{
						char *pol = NULL;
						g_object_get ( element, dvbs_props_n[c].gst_prop, &pol, NULL );
						g_string_append_printf ( gstring, ":polarity=%s", pol );
						free ( pol );

						continue;
					}

					if ( dvb_descr_n[d].descr == LNB )
					{
						uint lnb_num = descr_lnb_get_type ( element );

						g_string_append_printf ( gstring, ":%s=%d", "lnb-type", lnb_num );

						if ( lnb_num == LNB_MNL )
						{
							const char *lnbf_gst[] = { "lnb-lof1", "lnb-lof2", "lnb-slof" };

							uint8_t l = 0; for ( l = 0; l < G_N_ELEMENTS ( lnbf_gst ); l++ )
							{
								g_object_get ( element, lnbf_gst[l], &d_data, NULL );
								g_string_append_printf ( gstring, ":%s=%d", lnbf_gst[l], d_data );
							}
						}

						continue;
					}

					g_object_get ( element, dvbs_props_n[c].gst_prop, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", dvbs_props_n[c].gst_prop, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DVBT || DVBTYPE == SYS_DVBT2 )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dvbt_props_n ); c++ )
		{
			if ( DVBTYPE == SYS_DVBT )
				if ( g_str_has_prefix ( dvbt_props_n[c].name, "Stream Id" ) ) continue;

			for ( d = 0; d < NUM; d++ )
			{
				if ( dvbt_props_n[c].descr == dvb_descr_n[d].descr )
				{
					g_object_get ( element, dvbt_props_n[d].gst_prop, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", dvbt_props_n[d].gst_prop, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DVBC_ANNEX_A || DVBTYPE == SYS_DVBC_ANNEX_C )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dvbc_props_n ); c++ )
		{
			for ( d = 0; d < NUM; d++ )
			{
				if ( dvbc_props_n[c].descr == dvb_descr_n[d].descr )
				{
					g_object_get ( element, dvbc_props_n[d].gst_prop, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", dvbc_props_n[d].gst_prop, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_ATSC || DVBTYPE == SYS_DVBC_ANNEX_B )
	{
		for ( c = 0; c < G_N_ELEMENTS ( atsc_props_n ); c++ )
		{
			for ( d = 0; d < NUM; d++ )
			{
				if ( atsc_props_n[c].descr == dvb_descr_n[d].descr )
				{
					g_object_get ( element, atsc_props_n[d].gst_prop, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", atsc_props_n[d].gst_prop, d_data );

					break;
				}
			}
		}
	}

	if ( DVBTYPE == SYS_DTMB )
	{
		for ( c = 0; c < G_N_ELEMENTS ( dtmb_props_n ); c++ )
		{
			for ( d = 0; d < NUM; d++ )
			{
				if ( dtmb_props_n[c].descr == dvb_descr_n[d].descr )
				{
					g_object_get ( element, dtmb_props_n[d].gst_prop, &d_data, NULL );
					g_string_append_printf ( gstring, ":%s=%d", dtmb_props_n[d].gst_prop, d_data );

					break;
				}
			}
		}
	}
}

static GtkLabel * descr_info_create_label ( const char *text )
{
	GtkLabel *label = (GtkLabel *)gtk_label_new ( text );
	gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );
	gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );

	return label;
}

static void descr_info_set_data ( uint sid, const char *data, const DvbTypes *dvball, uint num, uint delsys, GtkBox *v_box, GtkComboBoxText *combo_lang )
{
	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_row_spacing ( grid, 5 );
	gtk_grid_set_column_homogeneous ( grid, TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( grid  ), TRUE );

	char **line = g_strsplit ( data, ":", 0 );
	uint c = 0, d = 0, z = 0, j = 0, n = 0, n_splits = g_strv_length ( line );

	GtkEntry *entry_ch = (GtkEntry *) gtk_entry_new ();
	g_object_set ( entry_ch, "editable", FALSE, NULL );
	gtk_entry_set_text ( entry_ch, line[0] );
	gtk_widget_set_visible ( GTK_WIDGET ( entry_ch  ), TRUE );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( entry_ch ), FALSE, FALSE, 0 );

	gboolean visible_combo = FALSE;
	gtk_box_pack_start ( v_box, GTK_WIDGET ( combo_lang ), FALSE, FALSE, 0 );

	for ( j = 1; j < n_splits; j++ )
	{
		const char *data_1[] = { "Service Id", "Audio Pid", "Video Pid" };
		const char *data_2[] = { "program-number", "audio-pid", "video-pid" };

		uint8_t f = 0; for ( f = 0; f < G_N_ELEMENTS ( data_1 ); f++ )
		{
			if ( g_strrstr ( line[j], data_2[f] ) )
			{
				GtkLabel *label = descr_info_create_label ( data_1[f] );
				gtk_grid_attach ( grid, GTK_WIDGET ( label ), 0, (int)n, 1, 1 );

				char **splits = g_strsplit ( line[j], "=", 0 );

				label = descr_info_create_label ( splits[1] );
				gtk_grid_attach ( grid, GTK_WIDGET ( label ), 1, (int)n++, 1, 1 );

				uint pn = ( uint ) atoi ( splits[1] );
				if ( f == 0 && pn == sid ) visible_combo = TRUE;

		 		g_strfreev ( splits );

				break;
			}
		}
	}

	gtk_widget_set_visible ( GTK_WIDGET ( combo_lang ), visible_combo );

	if ( delsys == SYS_UNDEFINED || dvball == NULL )
	{
		gtk_box_pack_start ( v_box, GTK_WIDGET ( grid ), TRUE, TRUE, 0 );

		g_strfreev ( line );

		return;
	}

	for ( c = 0; c < num; c++ )
	{
		GtkLabel *label = descr_info_create_label ( dvball[c].name );
		gtk_grid_attach ( grid, GTK_WIDGET ( label ), 0, (int)( c + n ), 1, 1 );

		for ( j = 1; j < n_splits; j++ )
		{
			if ( !g_strrstr ( line[j], dvball[c].gst_prop ) ) continue;

			char **splits = g_strsplit ( line[j], "=", 0 );

			GtkLabel *label_set = descr_info_create_label ( splits[1] );
			gtk_grid_attach ( grid, GTK_WIDGET ( label_set ), 1, (int)( c + n ), 1, 1 );

			long dat = atol ( splits[1] );

			if ( !dvball[c].descr )
			{
				if ( g_str_has_prefix ( splits[0], "frequency" ) )
				{
					if ( delsys == SYS_DVBS || delsys == SYS_DVBS2 || delsys == SYS_TURBO )
						dat = dat / 1000;
					else
						dat = dat / 1000000;

					char buf[100];
					sprintf ( buf, "%ld", dat );

					gtk_label_set_text ( label_set, buf );
				}

				if ( g_str_has_prefix ( splits[0], "symbol-rate" ) && dat > 1000000 )
				{
					dat = dat / 1000;

					char buf[100];
					sprintf ( buf, "%ld", dat );

					gtk_label_set_text ( label_set, buf );
				}
			}

			if ( dvball[c].descr )
			{
				for ( d = 0; d < NUM; d++ )
				{
					if ( dvball[c].descr == POL )
					{
						if ( splits[1][0] == 'v' || splits[1][0] == 'V' || splits[1][0] == '0' )
							gtk_label_set_text ( label_set, "V" );
						else
							gtk_label_set_text ( label_set, "H" );
					}
					else if ( dvball[c].descr == dvb_descr_n[d].descr )
					{
						for ( z = 0; z < dvb_descr_n[d].cdsc; z++ )
						{
							if ( dat == dvb_descr_n[d].dvb_descr[z].descr )
								{ gtk_label_set_text ( label_set, dvb_descr_n[d].dvb_descr[z].text ); break; }
						}
					}
				}
			}

			g_debug ( "%s: %s -- %s", __func__, dvball[c].name, line[j] );

			g_strfreev ( splits );
		}
	}

	g_strfreev ( line );

	gtk_box_pack_start ( v_box, GTK_WIDGET ( grid ), TRUE, TRUE, 0 );
}

static uint descr_info_get_delsys ( const char *data )
{
	uint ret = SYS_UNDEFINED, d = 0;

	for ( d = 0; d < G_N_ELEMENTS ( dvb_descr_delsys_type_n ); d++ )
	{
		char delsys_prop[25];
		sprintf ( delsys_prop, "delsys=%d", dvb_descr_delsys_type_n[d].descr );

		if ( g_strrstr ( data, delsys_prop ) )
		{
			ret = (uint)dvb_descr_delsys_type_n[d].descr;

			g_debug ( "%s: %d -- %s", __func__, dvb_descr_delsys_type_n[d].descr, dvb_descr_delsys_type_n[d].text );

			break;
		}
	}

	return ret;
}

static void descr_handler_info ( G_GNUC_UNUSED Descr *descr, uint sid, const char *data, GObject *obj_box, GObject *obj_combo )
{
	GtkBox *v_box = GTK_BOX ( obj_box );
	GtkComboBoxText *combo_lang = GTK_COMBO_BOX_TEXT ( obj_combo );

	uint delsys = descr_info_get_delsys ( data );

	if ( delsys == SYS_UNDEFINED )                                          descr_info_set_data ( sid, data, NULL, 0, delsys, v_box, combo_lang );

	if ( delsys == SYS_DTMB )                                               descr_info_set_data ( sid, data, atsc_props_n, G_N_ELEMENTS ( atsc_props_n ), delsys, v_box, combo_lang );
	if ( delsys == SYS_DVBT || delsys == SYS_DVBT2 )                        descr_info_set_data ( sid, data, dvbt_props_n, G_N_ELEMENTS ( dvbt_props_n ), delsys, v_box, combo_lang );
	if ( delsys == SYS_ATSC || delsys == SYS_DVBC_ANNEX_B )                 descr_info_set_data ( sid, data, dtmb_props_n, G_N_ELEMENTS ( dtmb_props_n ), delsys, v_box, combo_lang );
	if ( delsys == SYS_DVBC_ANNEX_A || delsys == SYS_DVBC_ANNEX_C )         descr_info_set_data ( sid, data, dvbc_props_n, G_N_ELEMENTS ( dvbc_props_n ), delsys, v_box, combo_lang );
	if ( delsys == SYS_DVBS || delsys == SYS_DVBS2 || delsys == SYS_TURBO ) descr_info_set_data ( sid, data, dvbs_props_n, G_N_ELEMENTS ( dvbs_props_n ), delsys, v_box, combo_lang );
}

static void descr_init ( Descr *descr )
{
	g_signal_connect ( descr, "descr-info",   G_CALLBACK ( descr_handler_info   ), NULL );
	g_signal_connect ( descr, "descr-scan",   G_CALLBACK ( descr_handler_scan   ), NULL );
	g_signal_connect ( descr, "descr-lnbs",   G_CALLBACK ( descr_handler_lnbs   ), NULL );
	g_signal_connect ( descr, "descr-get-tp", G_CALLBACK ( descr_handler_get_tp ), NULL );
}

static void descr_finalize ( GObject *object )
{
	G_OBJECT_CLASS ( descr_parent_class )->finalize ( object );
}

static void descr_class_init ( DescrClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = descr_finalize;

	g_signal_new ( "descr-lnbs",   G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_UINT,    G_TYPE_POINTER );
	g_signal_new ( "descr-get-tp", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_POINTER );
	g_signal_new ( "descr-scan",   G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 3, G_TYPE_UINT,    G_TYPE_OBJECT, G_TYPE_POINTER );
	g_signal_new ( "descr-info",   G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 4, G_TYPE_UINT,    G_TYPE_STRING, G_TYPE_OBJECT, G_TYPE_OBJECT );
}

Descr * descr_new ( void )
{
	Descr *descr = g_object_new ( DESCR_TYPE_OBJECT, NULL );

	return descr;
}
