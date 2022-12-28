/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "scan.h"
#include "descr.h"
#include "level.h"
#include "convert.h"
#include "dvb-linux.h"

#include <stdlib.h>
#include <gst/gst.h>
#include <linux/dvb/frontend.h>

#define GST_USE_UNSTABLE_API
#include <gst/mpegts/mpegts.h>

#define MAX_PAT 128

/* DVB-T/T2, DVB-S/S2, DVB-C */

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
};

G_DEFINE_TYPE ( Scan, scan, GTK_TYPE_WINDOW )

const struct ScanLabel { uint8_t page; const char *name; } scan_label_n[] =
{
	{ PAGE_SC, "Scanner"  },
	{ PAGE_DT, "DVB-T/T2" },
	{ PAGE_DS, "DVB-S/S2" },
	{ PAGE_DC, "DVB-C"    },
	{ PAGE_AT, "ATSC"     },
	{ PAGE_DM, "DTMB"     },
	{ PAGE_CH, "Channels" }
};

typedef struct _ScanDelSys ScanDelSys;

struct _ScanDelSys
{
	uint8_t descr;
	const char *text;
};

const ScanDelSys scan_delsys_type_n[] =
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

static void scan_stop ( GtkButton *, Scan * );

static void scan_add_treeview ( const char *, const char *, GtkTreeView * );

static void _strip_ch_name ( char *name )
{
	uint8_t i = 0; for ( i = 0; name[i] != '\0'; i++ )
	{
		if ( name[i] == ':' || name[i] == '/' || name[i] == '\'' ) name[i] = ' ';
	}

	g_strstrip ( name );
}

static void mpegts_initialize ( void )
{
	gst_mpegts_initialize ();

	g_type_class_ref ( GST_TYPE_MPEGTS_SECTION_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_SECTION_TABLE_ID );
}

static void mpegts_sdt ( GstMpegtsSection *section, GtkTreeView *tree_view, GstElement *dvbsrc, Scan *scan )
{
	const GstMpegtsSDT *sdt = gst_mpegts_section_get_sdt ( section );

	uint8_t i = 0, c = 0, len = (uint8_t)sdt->services->len;

	GString *gstr_data = g_string_new ( NULL );

	Descr *descr = descr_new ();

	g_signal_emit_by_name ( descr, "descr-get-tp", gstr_data, dvbsrc );

	g_message ( "SDT: %u Services \n", len );

	for ( i = 0; i < len; i++ )
	{
		if ( i >= MAX_PAT ) { g_print ( "MAX_PAT %u: Sdt stop  \n", MAX_PAT ); break; }

		GstMpegtsSDTService *service = g_ptr_array_index ( sdt->services, i );

		g_message ( "  Service id:  %u | %u ( 0x%04x ) ", i+1, service->service_id, service->service_id );

		GPtrArray *descriptors = service->descriptors;

		for ( c = 0; c < descriptors->len; c++ )
		{
			GstMpegtsDescriptor *desc = g_ptr_array_index ( descriptors, c );

			char *service_name, *provider_name;
			GstMpegtsDVBServiceType service_type;

			if ( desc->tag == GST_MTS_DESC_DVB_SERVICE )
			{
				if ( gst_mpegts_descriptor_parse_dvb_service ( desc, &service_type, &service_name, &provider_name ) )
				{
					_strip_ch_name ( service_name );

					GString *gstring = g_string_new ( NULL );
					g_string_append_printf ( gstring, "%s:program-number=%d%s", service_name, service->service_id, gstr_data->str );

					scan_add_treeview ( service_name, gstring->str, tree_view );

					g_message ( "    Service  : %s ", service_name );
					g_message ( "    Provider : %s \n", provider_name );

					free ( service_name  );
					free ( provider_name );
					g_string_free ( gstring, TRUE );
				}
			}
		}
	}

	g_object_unref ( descr );
	g_string_free ( gstr_data, TRUE );

	g_message ( "SDT Done \n" );

	scan_stop ( NULL, scan );
}

static void mpegts_vct ( GstMpegtsSection *section, GtkTreeView *tree_view, GstElement *dvbsrc, Scan *scan )
{
	const GstMpegtsAtscVCT *vct = ( GST_MPEGTS_SECTION_TYPE (section) == GST_MPEGTS_SECTION_ATSC_CVCT ) 
		? gst_mpegts_section_get_atsc_cvct ( section ) : gst_mpegts_section_get_atsc_tvct ( section );

	uint8_t i = 0, len = (uint8_t)vct->sources->len;

	GString *gstr_data = g_string_new ( NULL );

	Descr *descr = descr_new ();

	g_signal_emit_by_name ( descr, "descr-get-tp", gstr_data, dvbsrc );

	g_message ( "VCT: %u Sources \n", len );

	for ( i = 0; i < len; i++ )
	{
		if ( i >= MAX_PAT ) { g_print ( "MAX_PAT %u: Vct stop  \n", MAX_PAT ); break; }

		GstMpegtsAtscVCTSource *source = g_ptr_array_index ( vct->sources, i );

		g_message ( "  Service id:  %u | %u ( 0x%04x ) ", i+1, source->program_number, source->program_number );

		char *name = g_strdup ( source->short_name );

		_strip_ch_name ( name );

		GString *gstring = g_string_new ( NULL );
		g_string_append_printf ( gstring, "%s:program-number=%d%s", name, source->program_number, gstr_data->str );

		scan_add_treeview ( name, gstring->str, tree_view );

		g_message ( "    Service ( short name ) : %s \n", source->short_name );

		free ( name );
		g_string_free ( gstring, TRUE );
	}

	g_object_unref ( descr );
	g_string_free ( gstr_data, TRUE );

	g_message ( "VCT Done \n" );

	scan_stop ( NULL, scan );
}

static void mpegts_parse_section ( GstMessage *message, GtkTreeView *tree_view, GstElement *dvbsrc, Scan *scan )
{
	GstMpegtsSection *section = gst_message_parse_mpegts_section ( message );

	if ( section )
	{
		switch ( GST_MPEGTS_SECTION_TYPE ( section ) )
		{
			case GST_MPEGTS_SECTION_SDT:
				mpegts_sdt ( section, tree_view, dvbsrc, scan );
				break;

			case GST_MPEGTS_SECTION_ATSC_CVCT:
			case GST_MPEGTS_SECTION_ATSC_TVCT:
				mpegts_vct ( section, tree_view, dvbsrc, scan );
				break;

			default:
			break;
		}

		gst_mpegts_section_unref ( section );
	}
}

static void scan_message_dialog ( const char *f_error, const char *file_or_info, GtkMessageType mesg_type, Scan *scan )
{
	GtkMessageDialog *dialog = ( GtkMessageDialog *)gtk_message_dialog_new ( GTK_WINDOW ( scan ), GTK_DIALOG_MODAL, mesg_type, GTK_BUTTONS_CLOSE, "%s\n%s",  f_error, file_or_info );

	gtk_window_set_icon_name ( GTK_WINDOW (dialog), "helia" );
	gtk_widget_set_opacity ( GTK_WIDGET ( dialog ), gtk_widget_get_opacity ( GTK_WIDGET ( scan ) ) );

	gtk_dialog_run     ( GTK_DIALOG ( dialog ) );
	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

static void scan_add_treeview ( const char *ch_name, const char *ch_data, GtkTreeView *tree_view )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );

	gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter );
	gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter, COL_SCAN_NUM, ind + 1, COL_SCAN_CHNL, ch_name, COL_SCAN_DATA, ch_data, -1 );
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

static void scan_stop ( G_GNUC_UNUSED GtkButton *button, Scan *scan )
{
	if ( GST_ELEMENT_CAST ( scan->dvbscan )->current_state == GST_STATE_NULL ) return;

	if ( scan->level && GTK_IS_WIDGET ( scan->level ) ) g_signal_emit_by_name ( scan->level, "level-update", 0, 0, FALSE, FALSE );

	gst_element_set_state ( scan->dvbscan, GST_STATE_NULL );
}

static void scan_start ( G_GNUC_UNUSED GtkButton *button, Scan *scan )
{
	if ( GST_ELEMENT_CAST ( scan->dvbscan )->current_state == GST_STATE_PLAYING ) return;

	gst_element_set_state ( scan->dvbscan, GST_STATE_PLAYING );
}

static void scan_channels ( GtkBox *box, Scan *scan )
{
	gtk_widget_set_margin_start ( GTK_WIDGET ( box ), 0 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( box ), 0 );

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
		{ "Num",     "text", COL_SCAN_NUM  },
		{ "Channel", "text", COL_SCAN_CHNL },
		{ "Data",    "text", COL_SCAN_DATA }
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

	gtk_box_pack_start ( box, GTK_WIDGET ( sw ), TRUE, TRUE, 0 );

	GtkBox *hb_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( hb_box ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( hb_box ), 5 );
	gtk_box_set_spacing ( hb_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( hb_box ), TRUE );

	GtkButton *button = (GtkButton *)gtk_button_new_from_icon_name ( "helia-clear", GTK_ICON_SIZE_MENU );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	gtk_box_pack_start ( hb_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_list_store_clear ), GTK_LIST_STORE ( gtk_tree_view_get_model ( scan->treeview ) ) );

	button = (GtkButton *)gtk_button_new_from_icon_name ( "helia-save", GTK_ICON_SIZE_MENU );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	gtk_box_pack_start ( hb_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( scan_save ), scan );

	gtk_box_pack_start ( box, GTK_WIDGET ( hb_box ), FALSE, FALSE, 5 );
}

static void scan_create_control_battons ( GtkBox *box, Scan *scan )
{
	scan->level = level_new ();
	gtk_widget_set_margin_start ( GTK_WIDGET ( scan->level ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( scan->level ), 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( scan->level ), TRUE );

	GtkBox *hb_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( hb_box ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( hb_box ), 5 );
	gtk_box_set_spacing ( hb_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( hb_box ), TRUE );

	GtkButton *button = (GtkButton *)gtk_button_new_from_icon_name ( "helia-play", GTK_ICON_SIZE_MENU );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	gtk_box_pack_start ( hb_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( scan_start ), scan );

	button = (GtkButton *)gtk_button_new_from_icon_name ( "helia-stop", GTK_ICON_SIZE_MENU );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	gtk_box_pack_start ( hb_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( scan_stop ), scan );

	gtk_box_pack_end ( box, GTK_WIDGET ( hb_box ), FALSE, FALSE, 5 );
	gtk_box_pack_end ( box, GTK_WIDGET ( scan->level ), FALSE, FALSE, 0 );
}

static void scan_set_new_device ( Scan *scan )
{
	int frontend = 0, adapter = 0;
	g_object_get ( scan->dvbsrc, "adapter",  &adapter,  NULL );
	g_object_get ( scan->dvbsrc, "frontend", &frontend, NULL );

	g_autofree char *dvb_name = dvb_get_name ( adapter, frontend );
	gtk_label_set_text ( scan->label_device, dvb_name );
}

static void scan_set_adapter ( GtkSpinButton *button, Scan *scan )
{
	if ( !scan->dvbsrc ) { g_critical ( "%s:: dvbsrc == NULL - not set.\n", __func__ ); return; }

	gtk_spin_button_update ( button );

	int adapter_set = gtk_spin_button_get_value_as_int ( button );

	g_object_set ( scan->dvbsrc, "adapter", adapter_set, NULL );

	scan_set_new_device ( scan );
}

static void scan_set_frontend ( GtkSpinButton *button, Scan *scan )
{
	if ( !scan->dvbsrc ) { g_critical ( "%s:: dvbsrc == NULL - not set.\n", __func__ ); return; }

	gtk_spin_button_update ( button );

	int frontend_set = gtk_spin_button_get_value_as_int ( button );

	g_object_set ( scan->dvbsrc, "frontend", frontend_set, NULL );

	scan_set_new_device ( scan );
}

static void scan_set_delsys ( GtkComboBox *combo_box, Scan *scan )
{
	if ( !scan->dvbsrc ) { g_critical ( "%s:: dvbsrc == NULL - not set.\n", __func__ ); return; }

	int num = gtk_combo_box_get_active ( combo_box );

	int delsys_set = scan_delsys_type_n[num].descr;

	g_object_set ( scan->dvbsrc, "delsys", delsys_set, NULL );

	g_debug ( "%s: Set delsys: %s ( %d ) ", __func__, scan_delsys_type_n[num].text, delsys_set );
}

static void scan_convert_dvb5_to_gst ( const char *file, Scan *scan )
{
	Convert *convert = convert_new ();

	char *contents;
	GError *err = NULL;

	uint n = 0;
	int adapter = 0, frontend = 0;

	GstElement *element = scan->dvbsrc;

	g_object_get ( element, "adapter",  &adapter,  NULL );
	g_object_get ( element, "frontend", &frontend, NULL );

	if ( g_file_get_contents ( file, &contents, 0, &err ) )
	{
		char **lines = g_strsplit ( contents, "[", 0 );
		uint length = g_strv_length ( lines );

		for ( n = 1; n < length; n++ )
		{
			GString *gstr_name = g_string_new ( NULL );
			GString *gstr_data = g_string_new ( NULL );

			g_signal_emit_by_name ( convert, "convert", (uint)adapter, (uint)frontend, lines[n], gstr_name, gstr_data );

			g_signal_emit_by_name ( scan, "scan-append", gstr_name->str, gstr_data->str );

			g_string_free ( gstr_name, TRUE );
			g_string_free ( gstr_data, TRUE );
		}

		g_strfreev ( lines );
		g_free ( contents );
	}
	else
	{
		scan_message_dialog ( "", err->message, GTK_MESSAGE_ERROR, scan );
		g_error_free ( err );
	}

	g_object_unref ( convert );
}

static void scan_convert_file ( const char *file, Scan *scan )
{
	if ( file && g_str_has_suffix ( file, "channel.conf" ) )
	{
		if ( g_file_test ( file, G_FILE_TEST_EXISTS ) )
			scan_convert_dvb5_to_gst ( file, scan );
		else
			scan_message_dialog ( file, g_strerror ( errno ), GTK_MESSAGE_ERROR, scan );
	}
	else
	{
		g_warning ( "%s:: No convert %s ", __func__, file );
	}
}

static char * scan_open_file ( const char *path, GtkWindow *window )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new ( 
		" ",  window, GTK_FILE_CHOOSER_ACTION_OPEN, "gtk-cancel", GTK_RESPONSE_CANCEL, "gtk-open", GTK_RESPONSE_ACCEPT, NULL );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "document-open" );
	gtk_widget_set_opacity   ( GTK_WIDGET ( dialog ), gtk_widget_get_opacity ( GTK_WIDGET ( window ) ) );

	gtk_file_chooser_set_current_folder  ( GTK_FILE_CHOOSER ( dialog ), path );
	gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ), FALSE );

	char *filename = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return filename;
}

static void scan_convert_set_file ( GtkEntry *entry, GtkEntryIconPosition icon_pos, G_GNUC_UNUSED GdkEventButton *event, Scan *scan )
{
	if ( icon_pos == GTK_ENTRY_ICON_PRIMARY )
	{
		char *file = scan_open_file ( g_get_home_dir (), GTK_WINDOW ( scan ) );

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

	gtk_widget_set_margin_end   ( GTK_WIDGET ( entry ), 10 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( entry ), 10 );

	g_object_set ( entry, "editable", FALSE, NULL );
	gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_PRIMARY, "folder" );
	gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, "helia-convert" );
	g_signal_connect ( entry, "icon-press", G_CALLBACK ( scan_convert_set_file ), scan );

	gtk_box_pack_start ( g_box, GTK_WIDGET ( entry ), FALSE, FALSE, 5 );

	return g_box;
}

static void scan_device ( GtkBox *box, Scan *scan )
{
	int adapter = 0, frontend = 0, delsys = 0;

	if ( scan->dvbsrc ) g_object_get ( scan->dvbsrc, "adapter",  &adapter, NULL );
	if ( scan->dvbsrc ) g_object_get ( scan->dvbsrc, "frontend", &frontend, NULL );

	g_autofree char *dvb_name = dvb_get_name ( adapter, frontend );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( grid, TRUE );
	gtk_grid_set_row_spacing ( grid, 5 );

	gtk_widget_set_margin_start ( GTK_WIDGET ( grid ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( grid ), 10 );

	gtk_widget_set_visible ( GTK_WIDGET ( grid ), TRUE );
	gtk_box_pack_start ( box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	struct DataDevice { const char *text; int value; void (*f)(); } data_n[] =
	{
		{ dvb_name,     0, NULL },
		{ "Adapter",    adapter,  scan_set_adapter  },
		{ "Frontend",   frontend, scan_set_frontend },
		{ "DelSys",     delsys,   NULL }
	};

	uint8_t d = 0, c = 0;
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

	for ( c = 0; c < G_N_ELEMENTS ( scan_delsys_type_n ); c++ )
	{
		gtk_combo_box_text_append_text ( combo_delsys, scan_delsys_type_n[c].text );
	}

	gtk_combo_box_set_active ( GTK_COMBO_BOX ( combo_delsys ), 0 );
	g_signal_connect ( combo_delsys, "changed", G_CALLBACK ( scan_set_delsys ), scan );

	gtk_grid_attach ( grid, GTK_WIDGET ( combo_delsys ), 1, d-1, 1, 1 );

	gtk_box_pack_start ( box, GTK_WIDGET ( scan_convert ( scan ) ), TRUE, TRUE, 10 );

	scan_create_control_battons ( box, scan );
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

			if ( GTK_IS_WIDGET ( scan->level ) ) g_signal_emit_by_name ( scan->level, "level-update", ret_sgl, ret_snr, lock, FALSE );
		}
	}

	mpegts_parse_section ( message, scan->treeview, scan->dvbsrc, scan );
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

	// g_debug ( "scan_set_tune_timeout: timeout %ld | timeout set %ld", timeout, timeout_get );
}

static void scan_create ( Scan *scan )
{
	mpegts_initialize ();

	GstElement *tsparse, *filesink;

	scan->dvbscan = gst_pipeline_new ( "pipeline-scan" );
	scan->dvbsrc  = gst_element_factory_make ( "dvbsrc",   NULL );
	tsparse       = gst_element_factory_make ( "tsparse",  NULL );
	filesink      = gst_element_factory_make ( "fakesink", NULL );

	if ( !scan->dvbscan || !scan->dvbsrc || !tsparse || !filesink ) { g_critical ( "%s:: pipeline scan - not be created.\n", __func__ ); return; }

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

	gst_object_unref ( scan->dvbscan );
}

static void scan_init ( Scan *scan )
{
	scan_create ( scan );

	GtkWindow *window = GTK_WINDOW ( scan );

	gtk_window_set_title ( window, "" );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_icon_name ( window, "helia" );
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

	Descr *descr = descr_new ();

	uint8_t j = 0; for ( j = 0; j < PAGE_NUM; j++ )
	{
		GtkBox *m_box_n = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
		gtk_widget_set_visible ( GTK_WIDGET ( m_box_n ), TRUE );

		if ( j == PAGE_SC ) scan_device   ( m_box_n, scan );
		if ( j == PAGE_CH ) scan_channels ( m_box_n, scan );

		if ( j == PAGE_DT || j == PAGE_DS || j == PAGE_DC || j == PAGE_AT || j == PAGE_DM )
			g_signal_emit_by_name ( descr, "descr-scan", j, G_OBJECT ( m_box_n ), scan->dvbsrc );

		gtk_notebook_append_page ( notebook, GTK_WIDGET ( m_box_n ), gtk_label_new ( scan_label_n[j].name ) );
	}

	g_object_unref ( descr );

	gtk_notebook_set_tab_pos ( notebook, GTK_POS_TOP );
	gtk_box_pack_start ( m_box, GTK_WIDGET (notebook), TRUE, TRUE, 0 );

	GtkButton *button = (GtkButton *)gtk_button_new_from_icon_name ( "helia-close", GTK_ICON_SIZE_MENU );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE,  TRUE,  5 );
	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box  ), FALSE, FALSE, 5 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
}

static void scan_finalize ( GObject *object )
{
	G_OBJECT_CLASS ( scan_parent_class )->finalize ( object );
}

static void scan_class_init ( ScanClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = scan_finalize;

	g_signal_new ( "scan-append", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING );
}

Scan * scan_new ( GtkWindow *base_win )
{
	Scan *scan = g_object_new ( SCAN_TYPE_WIN, "transient-for", base_win, NULL );

	gtk_window_present ( GTK_WINDOW ( scan ) );

	double opacity = gtk_widget_get_opacity ( GTK_WIDGET ( base_win ) );
	gtk_widget_set_opacity ( GTK_WIDGET ( scan ), opacity );

	return scan;
}
