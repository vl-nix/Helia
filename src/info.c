/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "info.h"
#include "default.h"
#include "scan.h"
#include "button.h"

#include <linux/dvb/frontend.h>
#include <gst/pbutils/pbutils.h>

typedef struct _Info Info;

struct _Info
{
	GtkLabel *label_wh;

	uint src_a, src_v;
	GtkLabel *label_audio;
	GtkLabel *label_video;

	GstElement *element;
	GtkTreeView *treeview;

	GstDiscoverer *discoverer;

	char *fps;
	char *audio;
};

/*
static void print_tag_foreach ( const GstTagList *tags, const gchar *tag, gpointer data )
{
	char *str = NULL;
	GValue val = { 0, };

	int depth = GPOINTER_TO_INT (data);

	gst_tag_list_copy_value ( &val, tags, tag );

	if ( G_VALUE_HOLDS_STRING (&val) )
		str = g_value_dup_string (&val);
	else
		str = gst_value_serialize (&val);

	g_print ( "%*s: %s \n", 2*depth, gst_tag_get_nick (tag), str );

	free (str);
	g_value_unset (&val);
}
*/

static void helia_info_discovered_cb ( G_GNUC_UNUSED GstDiscoverer *discoverer, GstDiscovererInfo *info, GError *err, Info *inf )
{
	if ( err )
	{
		g_warning ( "Couldn't get information about '%s': %s", gst_discoverer_info_get_uri (info), err->message );
		return;
	}

	GList *video_streams = gst_discoverer_info_get_video_streams ( info );
	GList *audio_streams = gst_discoverer_info_get_audio_streams ( info );

	if ( video_streams )
	{
		uint width = gst_discoverer_video_info_get_width ( video_streams->data );
		uint height = gst_discoverer_video_info_get_height ( video_streams->data );

		char wh[100];
		sprintf ( wh, "%u × %u", width, height );

		gtk_label_set_text ( inf->label_wh, wh );

		uint fps_n = gst_discoverer_video_info_get_framerate_num   ( video_streams->data );
		uint fps_d = gst_discoverer_video_info_get_framerate_denom ( video_streams->data );

		char fps[100] = { ' ' };
		if ( fps_n > 0 && fps_d > 0 ) { sprintf ( fps, "%u", ( fps_n + fps_d/2 ) / fps_d ); inf->fps = g_strdup ( fps ); }
	}

	if ( audio_streams )
	{
		int num = 0, n_audio = 0, c_audio = 0;
		g_object_get ( inf->element, "n-audio", &n_audio, NULL );
		g_object_get ( inf->element, "current-audio", &c_audio, NULL );

		while ( audio_streams != NULL )
		{
			uint channels = gst_discoverer_audio_info_get_channels ( audio_streams->data );
			uint rate = gst_discoverer_audio_info_get_sample_rate ( audio_streams->data );

			if ( channels )
			{
				const char *str = "Stereo";

				if ( channels > 2 )
					str = "Surround";
				else if ( channels == 1 )
					str = "Mono";

				if ( num == c_audio )
				{
					free ( inf->audio );

					if ( rate )
						inf->audio = g_strdup_printf ( "%s   ( %d KHz )", str, rate / 1000 );
					else
						inf->audio = g_strdup_printf ( "%s", str );
				}
			}
/*
			const GstTagList *tags = gst_discoverer_stream_info_get_tags ( audio_streams->data );

			if ( tags ) gst_tag_list_foreach ( tags, print_tag_foreach, GINT_TO_POINTER (4) );
*/
			num++;
			audio_streams = audio_streams->next;
		}
	}

	gst_discoverer_stream_info_list_free ( video_streams );
	gst_discoverer_stream_info_list_free ( audio_streams );
}

static GstDiscoverer * helia_info_create_discovered ( Info *info )
{
	g_autofree char *uri = NULL;
	g_object_get ( info->element, "current-uri", &uri, NULL );

	GError *err = NULL;
	GstDiscoverer *discoverer = gst_discoverer_new ( 60 * GST_SECOND, &err );

	if ( discoverer == NULL )
	{
		g_warning ( "Could not create discoverer object: %s", err->message );
		g_error_free ( err );

		return NULL;
	}

	g_signal_connect ( discoverer, "discovered", G_CALLBACK ( helia_info_discovered_cb ), info );

	gst_discoverer_start ( discoverer );
	gst_discoverer_discover_uri_async ( discoverer, uri );

	return discoverer;
}

static void helia_info_changed_combo_video ( GtkComboBox *combo, Info *info )
{
	g_object_set ( info->element, "current-video", gtk_combo_box_get_active (combo), NULL );
}
static void helia_info_changed_combo_audio ( GtkComboBox *combo, Info *info )
{
	g_object_set ( info->element, "current-audio", gtk_combo_box_get_active (combo), NULL );

	gst_discoverer_stop ( info->discoverer );
	gst_object_unref    ( info->discoverer );

	info->discoverer = helia_info_create_discovered ( info );
}
static void helia_info_changed_combo_text ( GtkComboBox *combo, Info *info )
{
	g_object_set ( info->element, "current-text", gtk_combo_box_get_active (combo), NULL );
}

static gboolean helia_info_get_state_subtitle ( GstElement *element )
{
	enum gst_flags { GST_FLAG_TEXT = (1 << 2) };

	int flags = 0, flags_c = 0;
	g_object_get ( element, "flags", &flags, NULL );

	flags_c = flags;
	flags |= GST_FLAG_TEXT;

	if ( flags_c == flags ) return TRUE;

	return FALSE;
}

static void helia_info_change_state_subtitle ( G_GNUC_UNUSED GtkButton *button, GstElement *element )
{
	enum gst_flags { GST_FLAG_TEXT = (1 << 2) };

	int flags;
	g_object_get ( element, "flags", &flags, NULL );

	if ( helia_info_get_state_subtitle ( element ) ) flags &= ~GST_FLAG_TEXT; else flags |= GST_FLAG_TEXT;

	g_object_set ( element, "flags", flags, NULL );
}
static void helia_info_change_hide_show_subtitle ( GtkButton *button, GtkComboBoxText *combo )
{
	gboolean sensitive = gtk_widget_get_sensitive ( GTK_WIDGET ( combo ) );

	gtk_widget_set_sensitive ( GTK_WIDGET ( combo ), !sensitive );

	if ( helia_check_icon_theme ( "helia-set" ) )
	{
		GtkImage *image = helia_create_image ( ( !sensitive ) ? "helia-set" : "helia-unset", 16 );
		gtk_button_set_image ( button, GTK_WIDGET ( image ) );
	}
}

static char * helia_info_get_str_vat ( GstElement *element, char *get_tag, int n_cur, const char *metadata, const char *metadata_2 )
{
	char *str = NULL, *str_2 = NULL, *ret_str = NULL;

	GstTagList *tags;
	g_signal_emit_by_name ( element, get_tag, n_cur, &tags );

	if ( tags )
	{
		if ( gst_tag_list_get_string ( tags, metadata, &str ) )
		{
			if ( g_strrstr ( str, " (" ) )
			{
				char **lines = g_strsplit ( str, " (", 0 );

				free ( str );
				str = g_strdup ( lines[0] );

				g_strfreev ( lines );
			}

			if ( metadata_2 && gst_tag_list_get_string ( tags, metadata_2, &str_2 ) )
				{ ret_str = g_strdup_printf ( "%s   %s", str_2, str ); free ( str_2 ); }
			else
				ret_str = ( g_str_has_prefix ( get_tag, "get-audio" ) ) ? g_strdup_printf ( "%d   %s", n_cur + 1, str ) : g_strdup_printf ( "%s", str );

			free ( str );
		}
	}

	if ( ret_str == NULL ) ret_str = g_strdup_printf ( "№ %d", n_cur + 1 );

	return ret_str;
}

static char * helia_info_get_int_vat ( GstElement *element, char *get_tag, int n_cur, char *metadata )
{
	char *ret_str = NULL;

	GstTagList *tags;
	g_signal_emit_by_name ( element, get_tag, n_cur, &tags );

	if ( tags )
	{
		uint rate = 0;
		if ( gst_tag_list_get_uint ( tags, metadata, &rate ) ) 
			ret_str = g_strdup_printf ( "%d Kbits/s", rate / 1000 );
	}

	return ret_str;
}

static const char * helia_info_get_str_tag ( GstElement *element, char *get_tag, char *data )
{
	const char *name = NULL;

	GstTagList *tags;
	g_signal_emit_by_name ( element, get_tag, 0, &tags );

	if ( tags )
	{
		const GValue *value = gst_tag_list_get_value_index ( tags, data, 0 );

		if ( value && G_VALUE_HOLDS_STRING (value) )
			name = g_value_get_string ( value );
	}

	return name;
}

static void helia_info_get_title_artist ( GstElement *element, GtkEntry *entry_title )
{
	char buf[1024];
	const char *title_new = NULL;

	const char *title = helia_info_get_str_tag ( element, "get-video-tags", GST_TAG_TITLE  );

	if ( !title ) title = helia_info_get_str_tag ( element, "get-audio-tags", GST_TAG_TITLE  );

	const char *artist = helia_info_get_str_tag ( element, "get-audio-tags", GST_TAG_ARTIST );

	if ( title && artist )
	{
		sprintf ( buf, "%s -- %s", title, artist );
		title_new = buf;
	}
	else
	{
		if ( title  ) title_new = title;
		if ( artist ) title_new = artist;
	}

	gtk_entry_set_text ( entry_title, ( title_new ) ? title_new : "None" );
}

static gboolean helia_info_update_bitrate_video ( Info *info )
{
	int c_video;
	g_object_get ( info->element, "current-video", &c_video, NULL );

	g_autofree char *bitrate_video = helia_info_get_int_vat ( info->element, "get-video-tags", c_video, GST_TAG_BITRATE );

	if ( !bitrate_video && !info->fps ) return TRUE;

	char label[256] = { ' ' };

	if ( bitrate_video && info->fps )
		sprintf ( label, "%s   ---   Fps %s", bitrate_video, info->fps );
	else if ( bitrate_video )
		sprintf ( label, "%s", bitrate_video );
	else if ( info->fps )
		sprintf ( label, "Fps %s", info->fps );

	gtk_label_set_text ( info->label_video, label );

	return TRUE;
}

static void helia_info_bitrate_video ( GtkLabel *label, Info *info )
{
	info->label_video = label;

	info->src_v = g_timeout_add ( 250, (GSourceFunc)helia_info_update_bitrate_video, info );
}

static gboolean helia_info_update_bitrate_audio ( Info *info )
{
	int c_audio;
	g_object_get ( info->element, "current-audio", &c_audio, NULL );

	g_autofree char *bitrate_audio = helia_info_get_int_vat ( info->element, "get-audio-tags", c_audio, GST_TAG_BITRATE );

	if ( !bitrate_audio && !info->audio ) return TRUE;

	char label[256] = { ' ' };

	if ( bitrate_audio && info->audio )
		sprintf ( label, "%s   ---   %s", bitrate_audio, info->audio );
	else if ( bitrate_audio )
		sprintf ( label, "%s", bitrate_audio );
	else if ( info->audio )
		sprintf ( label, "%s", info->audio );


	gtk_label_set_text ( info->label_audio, label );

	return TRUE;
}

static void helia_info_bitrate_audio ( GtkLabel *label, Info *info )
{
	info->label_audio = label;

	info->src_a = g_timeout_add ( 250, (GSourceFunc)helia_info_update_bitrate_audio, info );
}

static void helia_info_title_save ( const char *title, const char *uri, Info *info )
{
	gboolean valid;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( info->treeview );

	if ( gtk_tree_model_iter_n_children ( model, NULL ) == 0 ) return;

	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		char *data;
		gtk_tree_model_get ( model, &iter, 2,  &data, -1 );

		if ( g_str_has_suffix ( uri, data ) )
		{
			gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter, 1, title, -1 );
			break;
		}

		free ( data );
	}
}

static void helia_info_entry_title_save ( GtkEntry *entry, GtkEntryIconPosition icon_pos, G_GNUC_UNUSED GdkEvent *event, Info *info )
{
	const char *title = gtk_entry_get_text ( entry );

	g_autofree char *uri = NULL;
	g_object_get ( info->element, "current-uri", &uri, NULL );

	if ( icon_pos == GTK_ENTRY_ICON_SECONDARY )
		if ( uri && title && gtk_entry_get_text_length ( entry ) > 0 ) helia_info_title_save ( title, uri, info );
}

static GtkBox * helia_info_mp ( GstElement *element, Info *info )
{
	GtkBox *v_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( v_box, 5 );
	gtk_widget_set_margin_top    ( GTK_WIDGET ( v_box ), 10 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( v_box ), 10 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( v_box ), 10 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( v_box ), 10 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_row_homogeneous    ( grid, TRUE );
	gtk_grid_set_column_homogeneous ( grid, TRUE );

	gtk_widget_set_visible ( GTK_WIDGET ( v_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( grid  ), TRUE );

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	GtkEntry *entry_title = (GtkEntry *) gtk_entry_new ();
	helia_info_get_title_artist ( element, entry_title );
	gtk_widget_set_visible ( GTK_WIDGET ( entry_title ), TRUE );

	const char *icon = helia_check_icon_theme ( "helia-save" ) ? "helia-save" : "document-save";
	gtk_entry_set_icon_from_icon_name ( entry_title, GTK_ENTRY_ICON_SECONDARY, icon );
	g_signal_connect ( entry_title, "icon-press", G_CALLBACK ( helia_info_entry_title_save ), info );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( entry_title ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	g_autofree char *uri = NULL;
	g_object_get ( element, "current-uri", &uri, NULL );

	GtkEntry *entry_fl = (GtkEntry *) gtk_entry_new ();
	g_object_set ( entry_fl, "editable", FALSE, NULL );
	gtk_entry_set_text ( entry_fl, ( uri ) ? uri : "None" );
	gtk_widget_set_visible ( GTK_WIDGET ( entry_fl ), TRUE );

	gtk_box_pack_start ( h_box, GTK_WIDGET ( entry_fl ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( h_box    ), FALSE, FALSE, 0 );

	info->label_wh = (GtkLabel *)gtk_label_new ( " " );
	gtk_widget_set_visible ( GTK_WIDGET ( info->label_wh ), TRUE );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( info->label_wh ), FALSE, FALSE, 5 );

	int n_video = 0, n_audio = 0, n_text = 0, c_video = 0, c_audio = 0, c_text = 0;
	g_object_get ( element, "n-video", &n_video, NULL );
	g_object_get ( element, "n-audio", &n_audio, NULL );
	g_object_get ( element, "n-text",  &n_text,  NULL );

	g_object_get ( element, "current-video", &c_video, NULL );
	g_object_get ( element, "current-audio", &c_audio, NULL );
	g_object_get ( element, "current-text",  &c_text,  NULL );

	g_autofree char *bitrate_video = helia_info_get_int_vat ( element, "get-video-tags", c_video, GST_TAG_BITRATE );
	g_autofree char *bitrate_audio = helia_info_get_int_vat ( element, "get-audio-tags", c_audio, GST_TAG_BITRATE );

	const char *icon_a = helia_check_icon_theme ( "helia-audio" ) ? "helia-audio" : "audio-x-generic";
	const char *icon_v = helia_check_icon_theme ( "helia-video" ) ? "helia-video" : "video-x-generic";
	const char *icon_s = helia_check_icon_theme ( "helia-subtitles" ) ? "helia-subtitles" : "text-x-generic";

	struct data { const char *name; int n_avt; int c_avt; const char *info; void (*f)(); } data_n[] =
	{
		{ icon_v, n_video, c_video, NULL, helia_info_changed_combo_video },
		{ " ",   	0,       0,       bitrate_video, helia_info_bitrate_video },
		{ icon_a, n_audio, c_audio, NULL, helia_info_changed_combo_audio },
		{ " ",  	0,       0,       bitrate_audio, helia_info_bitrate_audio },
		{ icon_s, n_text,  c_text,  NULL, helia_info_changed_combo_text  }
	};

	gboolean status_subtitle = helia_info_get_state_subtitle ( element );

	uint8_t c = 0, i = 0;
	for ( c = 0; c < G_N_ELEMENTS (data_n); c++ )
	{
		GtkImage *image = helia_create_image ( data_n[c].name, 48 );
		gtk_widget_set_halign ( GTK_WIDGET ( image ), GTK_ALIGN_START );
		gtk_widget_set_visible ( GTK_WIDGET ( image ), TRUE );
		gtk_grid_attach ( grid, GTK_WIDGET ( image ), 0, c, 1, 2 );

		if ( c == 0 || c == 2 || c == 4 )
		{
			GtkComboBoxText *combo = (GtkComboBoxText *)gtk_combo_box_text_new ();
			gtk_widget_set_visible ( GTK_WIDGET ( combo ), TRUE );

			for ( i = 0; i < data_n[c].n_avt; i++ )
			{
				char *teg_info = NULL;

				if ( c == 0 ) teg_info = helia_info_get_str_vat ( element, "get-video-tags", i, GST_TAG_VIDEO_CODEC,   NULL );
				if ( c == 2 ) teg_info = helia_info_get_str_vat ( element, "get-audio-tags", i, GST_TAG_AUDIO_CODEC,   GST_TAG_LANGUAGE_CODE );
				if ( c == 4 ) teg_info = helia_info_get_str_vat ( element, "get-text-tags",  i, GST_TAG_LANGUAGE_CODE, NULL );

				gtk_combo_box_text_append_text ( combo, teg_info );

				free ( teg_info );
			}

			gtk_combo_box_set_active ( GTK_COMBO_BOX ( combo ), data_n[c].c_avt );

			g_signal_connect ( combo, "changed", G_CALLBACK ( data_n[c].f ), info );

			if ( c == 4 && data_n[c].n_avt > 0 )
			{
				h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
				gtk_box_set_spacing ( h_box, 5 );
				gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

				GtkButton *button = helia_create_button ( NULL, ( status_subtitle ) ? "helia-set" : "helia-unset", "●", 16 ); 
				gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
				g_signal_connect ( button, "clicked", G_CALLBACK ( helia_info_change_state_subtitle     ), element );
				g_signal_connect ( button, "clicked", G_CALLBACK ( helia_info_change_hide_show_subtitle ), combo );

				gtk_box_pack_start ( h_box, GTK_WIDGET ( combo  ), TRUE, TRUE, 0 );
				gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );

				gtk_grid_attach ( grid, GTK_WIDGET ( h_box ), 1, c, 1, 1 );
				gtk_widget_set_sensitive ( GTK_WIDGET ( combo ), status_subtitle );
			}
			else
				gtk_grid_attach ( grid, GTK_WIDGET ( combo ), 1, c, 1, 1 );
		}
		else
		{
			GtkLabel *label = (GtkLabel *)gtk_label_new ( ( data_n[c].info ) ? data_n[c].info : " " );
			gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );
			gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
			gtk_grid_attach ( grid, GTK_WIDGET ( label ), 1, c, 1, 1 );

			data_n[c].f ( label, info );
		}
	}

	gtk_box_pack_start ( v_box, GTK_WIDGET ( grid ), TRUE, TRUE, 0 );

	return v_box;
}

static void helia_info_quit ( G_GNUC_UNUSED GtkWindow *win, Info *info )
{
	g_source_remove ( info->src_v );
	g_source_remove ( info->src_a );

	gst_discoverer_stop ( info->discoverer );
	gst_object_unref    ( info->discoverer );

	free ( info->audio );
	free ( info->fps );
	free ( info );
	info = NULL;
}

void helia_info_player ( GtkWindow *win_base, GtkTreeView *treeview, GstElement *element )
{
	Info *info = g_new0 ( Info, 1 );
	info->element = element;
	info->treeview = treeview;
	info->discoverer = helia_info_create_discovered ( info );

	info->fps = NULL;
	info->audio = NULL;

	GtkWindow *window = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title ( window, "" );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_transient_for ( window, win_base );
	gtk_window_set_icon_name ( window, DEF_ICON );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );

	gtk_window_set_default_size ( window, 500, -1 );
	g_signal_connect ( window, "destroy", G_CALLBACK ( helia_info_quit ), info );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_widget_set_visible ( GTK_WIDGET ( m_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( helia_info_mp ( element, info ) ), FALSE, FALSE, 0 );

	GtkButton *button_close = helia_create_button ( h_box, "helia-exit", "🞬", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( button_close ), TRUE );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_end ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_window_present ( window );

	double opacity = gtk_widget_get_opacity ( GTK_WIDGET ( win_base ) );
	gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity );
}



static GtkBox * helia_info_tv ( const char *data, GstElement *element, GtkComboBoxText *combo_lang )
{
	GtkBox *v_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( v_box, 5 );
	gtk_widget_set_margin_top   ( GTK_WIDGET ( v_box ), 10 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( v_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( v_box ), 10 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_row_homogeneous    ( grid, TRUE );
	gtk_grid_set_column_homogeneous ( grid, TRUE );

	gtk_widget_set_visible ( GTK_WIDGET ( v_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( grid  ), TRUE );

	int adapter = 0, frontend = 0, delsys = 0;
	g_object_get ( element, "adapter",  &adapter,  NULL );
	g_object_get ( element, "frontend", &frontend, NULL );
	g_object_get ( element, "delsys",   &delsys,   NULL );

	g_autofree char *dvb_name = scan_get_dvb_info ( adapter, frontend );

	GtkLabel *label = (GtkLabel *)gtk_label_new ( dvb_name );
	gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( label ), FALSE, FALSE, 0 );

	char **fields = g_strsplit ( data, ":", 0 );
	uint numfields = g_strv_length ( fields );

	GtkEntry *entry_ch = (GtkEntry *) gtk_entry_new ();
	g_object_set ( entry_ch, "editable", FALSE, NULL );
	gtk_entry_set_text ( entry_ch, fields[0] );
	gtk_widget_set_visible ( GTK_WIDGET ( entry_ch  ), TRUE );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( entry_ch ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( v_box, GTK_WIDGET ( combo_lang ), FALSE, FALSE, 0 );

	uint8_t j = 0; for ( j = 1; j < numfields; j++ )
	{
		if ( g_strrstr ( fields[j], "delsys" ) || g_strrstr ( fields[j], "adapter" ) || g_strrstr ( fields[j], "frontend" ) ) continue;

		if ( !g_strrstr ( fields[j], "=" ) ) continue;

		char **splits = g_strsplit ( fields[j], "=", 0 );

		const char *set = scan_get_info ( splits[0] );

		if ( g_strrstr ( splits[0], "code-rate-hp" ) )
		{
			if ( delsys == SYS_DVBT || delsys == SYS_DVBT2 ) ; else set = "Inner Fec";
		}

		if ( g_str_has_prefix ( splits[0], "lnb-lof1" ) ) set = "   LO1  MHz";
		if ( g_str_has_prefix ( splits[0], "lnb-lof2" ) ) set = "   LO2  MHz";
		if ( g_str_has_prefix ( splits[0], "lnb-slof" ) ) set = "   Switch  MHz";

		GtkLabel *label = (GtkLabel *)gtk_label_new ( set );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );

		gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
		gtk_grid_attach ( grid, GTK_WIDGET ( label ), 0, j+1, 1, 1 );

		const char *set_v = scan_get_info_descr_vis ( splits[0], atoi ( splits[1] ) );

		if ( g_strrstr ( splits[0], "polarity" ) ) set_v = splits[1];

		if ( g_str_has_prefix ( splits[0], "frequency" ) || g_str_has_prefix ( splits[0], "lnb-lo" ) || g_str_has_prefix ( splits[0], "lnb-sl" ) )
		{
			long dat = atol ( splits[1] );

			if ( delsys == SYS_DVBS || delsys == SYS_TURBO || delsys == SYS_DVBS2 || delsys == SYS_ISDBS )
				dat = dat / 1000;
			else
				dat = dat / 1000000;

			char buf[100];
			sprintf ( buf, "%ld", dat );

			label = (GtkLabel *)gtk_label_new ( buf );
		}
		else
			label = (GtkLabel *)gtk_label_new ( ( set_v ) ? set_v : splits[1] );

		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );

		gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
		gtk_grid_attach ( grid, GTK_WIDGET ( label ), 1, j+1, 1, 1 );

		g_strfreev (splits);
	}

	g_strfreev (fields);

	gtk_box_pack_start ( v_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	return v_box;
}

GtkComboBoxText * helia_info_dvb ( const char *data, GtkWindow *win_base, GstElement *element )
{
	if ( !data ) return NULL;

	GtkWindow *window = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title ( window, "" );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_transient_for ( window, win_base );
	gtk_window_set_icon_name ( window, DEF_ICON );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );

	gtk_window_set_default_size ( window, 400, -1 );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_widget_set_visible ( GTK_WIDGET ( m_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	GtkComboBoxText *combo_lang = (GtkComboBoxText *)gtk_combo_box_text_new ();
	gtk_widget_set_visible ( GTK_WIDGET ( combo_lang ), TRUE );
	gtk_box_pack_start ( m_box, GTK_WIDGET ( helia_info_tv ( data, element, combo_lang ) ), FALSE, FALSE, 0 );

	GtkButton *button_close = helia_create_button ( h_box, "helia-exit", "🞬", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( button_close ), TRUE );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_end ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_window_present ( window );

	double opacity = gtk_widget_get_opacity ( GTK_WIDGET ( win_base ) );
	gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity );

	return combo_lang;
}
