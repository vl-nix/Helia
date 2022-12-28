/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "dvb.h"
#include "descr.h"
#include "include.h"
#include "dvb-linux.h"

#include <time.h>
#include <stdlib.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#ifdef GDK_WINDOWING_X11
  #include <gdk/gdkx.h>
#endif
#ifdef GDK_WINDOWING_WIN32
  #include <gdk/gdkwin32.h>
#endif

struct _Dvb
{
	GtkDrawingArea parent_instance;

	GstElement *playdvb;

	GstElement *demux;
	GstElement *dvbsrc;
	GstElement *volume;
	GstElement *queue_audio;

	GstElement *teerec;
	GstElement *recmux;
	GstElement *recdemux;

	GstElement *tee_base;

	Level *level;

	uint16_t sid;
	uint8_t win_count;

	guintptr xid;
	uint src_tm;

	double volume_val;

	char *rec_dir;

	time_t t_hide;

	gboolean record;
	gboolean set_video;
	gboolean first_audio;
};

typedef struct _RetSidLnb RetSidLnb;

struct _RetSidLnb
{
	uint8_t lnb;
	uint16_t sid;

	gboolean lo_found;
};

G_DEFINE_TYPE ( Dvb, dvb, GTK_TYPE_DRAWING_AREA )

static void dvb_multi_destroy ( Dvb * );

static char * dvb_time_to_str ( void )
{
	GDateTime *date = g_date_time_new_now_local ();

	char *str_time = g_date_time_format ( date, "%j-%Y-%H-%M-%S" );

	g_date_time_unref ( date );

	return str_time;
}

static void dvb_message_dialog ( const char *f_error, const char *file_or_info, GtkMessageType mesg_type, Dvb *dvb )
{
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb ) ) );

	GtkMessageDialog *dialog = ( GtkMessageDialog *)gtk_message_dialog_new ( window, GTK_DIALOG_MODAL, 
		mesg_type, GTK_BUTTONS_CLOSE, "%s\n%s", f_error, file_or_info );

	gtk_window_set_icon_name ( GTK_WINDOW (dialog), "helia" );
	gtk_widget_set_opacity ( GTK_WIDGET ( dialog ), gtk_widget_get_opacity ( GTK_WIDGET ( window ) ) );

	gtk_dialog_run     ( GTK_DIALOG ( dialog ) );
	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

static char * dvb_save_dialog ( const char *dir, const char *file, Dvb *dvb )
{
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb ) ) );

	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new ( " ", window, GTK_FILE_CHOOSER_ACTION_SAVE, "gtk-cancel", GTK_RESPONSE_CANCEL, "gtk-save", GTK_RESPONSE_ACCEPT, NULL );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "document-save" );
	gtk_widget_set_opacity ( GTK_WIDGET ( dialog ), gtk_widget_get_opacity ( GTK_WIDGET ( window ) ) );

	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), dir );
	gtk_file_chooser_set_do_overwrite_confirmation ( GTK_FILE_CHOOSER ( dialog ), TRUE );
	gtk_file_chooser_set_current_name   ( GTK_FILE_CHOOSER ( dialog ), file );

	char *filename = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT ) filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return filename;
}

static void dvb_set_stop ( Dvb *dvb )
{
	dvb->record = FALSE;

	gst_element_set_state ( dvb->playdvb, GST_STATE_NULL );

	gtk_widget_queue_draw ( GTK_WIDGET ( dvb ) );

	if ( dvb->level && GTK_IS_WIDGET ( dvb->level ) ) g_signal_emit_by_name ( dvb->level, "level-update", 0, 0, FALSE, FALSE );

	g_signal_emit_by_name ( dvb, "dvb-icon-scan-info", FALSE );
}

static void dvb_set_mute ( Dvb *dvb )
{
	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state != GST_STATE_PLAYING ) return;

	gboolean mute = FALSE;

	g_object_get ( dvb->volume, "mute", &mute, NULL );
	g_object_set ( dvb->volume, "mute", !mute, NULL );
}

static void dvb_set_volume ( double val, Dvb *dvb )
{
	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state != GST_STATE_PLAYING ) return;

	g_object_set ( dvb->volume, "volume", val, NULL );
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

				g_debug ( "%s:: Object remove: %s ", __func__, object_name );

				free ( object_name );
				g_value_reset ( &item );

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

	dvb_change_audio_track ( dvb->demux, dvb->queue_audio, (uint8_t)num );
}

static GObject * dvb_handler_combo_lang ( Dvb *dvb )
{
	GtkComboBoxText *combo_lang = (GtkComboBoxText *) gtk_combo_box_text_new ();

	uint8_t num = dvb_add_audio_track ( combo_lang, dvb->demux );

	gtk_combo_box_set_active ( GTK_COMBO_BOX ( combo_lang ), num );
	g_signal_connect ( combo_lang, "changed", G_CALLBACK ( dvb_combo_lang_changed ), dvb );

	return G_OBJECT ( combo_lang );
}

static uint dvb_handler_getsid ( Dvb *dvb )
{
	return dvb->sid;
}

static gboolean dvb_pad_check_type ( GstPad *pad, const char *type )
{
	gboolean ret = FALSE;

	GstCaps *caps = gst_pad_get_current_caps ( pad );

	if ( !caps ) return FALSE;
	if ( !GST_IS_CAPS ( caps ) ) return FALSE;

	const char *name = gst_structure_get_name ( gst_caps_get_structure ( caps, 0 ) );

	if ( name && g_str_has_prefix ( name, type ) ) ret = TRUE;

	gst_caps_unref (caps);

	return ret;
}

static void dvb_pad_link ( GstPad *pad, GstElement *element, const char *name )
{
	GstPad *pad_sink = gst_element_get_static_pad ( element, "sink" );

	if ( gst_pad_link ( pad, pad_sink ) == GST_PAD_LINK_OK )
		g_debug ( "%s:: linking Ok; %s", __func__, name );
	else
		g_debug ( "%s:: linking Failed; %s", __func__, name );

	gst_object_unref ( pad_sink );
}

static void dvb_add_pad_decode_audio ( G_GNUC_UNUSED GstElement *element, GstPad *pad, GstElement *el )
{
	dvb_pad_link ( pad, el, "decode audio" );
}

static void dvb_add_pad_decode_video ( G_GNUC_UNUSED GstElement *element, GstPad *pad, GstElement *el )
{
	dvb_pad_link ( pad, el, "decode video" );
}

static void dvb_create_elements_audio ( GstPad *pad, Dvb *dvb )
{
	if ( dvb->first_audio ) return;

	const char *names[] = { "queue2", "decodebin", "audioconvert", "volume", "autoaudiosink" };

	GstElement *elements[ G_N_ELEMENTS ( names ) ];

	uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( names ); c++ )
	{
		elements[c] = gst_element_factory_make ( names[c], NULL );

		if ( !elements[c] ) g_critical ( "%s:: element (factory make) - %s not created.", __func__, names[c] );

		gst_bin_add ( GST_BIN ( dvb->playdvb ), elements[c] );

		gst_element_set_state ( elements[c], GST_STATE_PLAYING );

		if (  c == 0 || c == 2 ) continue;

		gst_element_link ( elements[c-1], elements[c] );
	}

	g_signal_connect ( elements[1], "pad-added", G_CALLBACK ( dvb_add_pad_decode_audio ), elements[2] );

	dvb_pad_link ( pad, elements[0], "demux audio" );

	dvb->volume = elements[3];
	dvb->queue_audio = elements[0];

	g_object_set ( dvb->volume, "mute",   FALSE, NULL );
	g_object_set ( dvb->volume, "volume", dvb->volume_val, NULL );

	dvb->first_audio = TRUE;
}

static void dvb_create_elements_video ( GstPad *pad, Dvb *dvb )
{
	const char *names[] = { "queue2", "decodebin", "videoconvert", "autovideosink" };

	GstElement *elements[ G_N_ELEMENTS ( names ) ];

	uint c = 0;
	for ( c = 0; c < G_N_ELEMENTS ( names ); c++ )
	{
		elements[c] = gst_element_factory_make ( names[c], NULL );

		if ( !elements[c] ) g_critical ( "%s:: element (factory make) - %s not created.", __func__, names[c] );

		gst_bin_add ( GST_BIN ( dvb->playdvb ), elements[c] );

		gst_element_set_state ( elements[c], GST_STATE_PLAYING );

		if (  c == 0 || c == 2 ) continue;

		gst_element_link ( elements[c-1], elements[c] );
	}

	g_signal_connect ( elements[1], "pad-added", G_CALLBACK ( dvb_add_pad_decode_video ), elements[2] );

	dvb_pad_link ( pad, elements[0], "demux video" );

	dvb->set_video = TRUE;
}

static void dvb_add_pad_demux ( G_GNUC_UNUSED GstElement *element, GstPad *pad, Dvb *dvb )
{
	if ( dvb_pad_check_type ( pad, "audio" ) ) dvb_create_elements_audio ( pad, dvb );
	if ( dvb_pad_check_type ( pad, "video" ) ) dvb_create_elements_video ( pad, dvb );
}

static void dvb_create_demux ( Dvb *dvb )
{
	dvb->teerec = gst_element_factory_make ( "tee",     NULL );
	dvb->demux  = gst_element_factory_make ( "tsdemux", NULL );

	if ( !dvb->teerec || !dvb->demux ) { g_critical ( "%s:: dvbsrc ... - not created.", __func__ ); return; }

	gst_bin_add_many ( GST_BIN ( dvb->playdvb ), dvb->teerec, dvb->demux, NULL );

	gst_element_link_many ( dvb->dvbsrc, dvb->teerec, dvb->demux, NULL );

	g_signal_connect ( dvb->demux, "pad-added", G_CALLBACK ( dvb_add_pad_demux ), dvb );
}

static void dvb_create_bin ( Dvb *dvb )
{
	dvb->dvbsrc = gst_element_factory_make ( "dvbsrc",  NULL );

	if ( !dvb->dvbsrc ) { g_critical ( "%s:: dvbsrc ... - not created.", __func__ ); return; }

	gst_bin_add ( GST_BIN ( dvb->playdvb ), dvb->dvbsrc );

	dvb_create_demux ( dvb );

	gst_element_link ( dvb->dvbsrc, dvb->teerec );
}

static void dvb_create_bin_rm_rec ( Dvb *dvb )
{
	dvb_remove_bin ( dvb->playdvb, "dvbsrc" );

	dvb_create_demux ( dvb );

	g_object_set ( dvb->demux, "program-number", dvb->sid, NULL );
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

static void dvb_typefind_parser ( GstElement *typefind, G_GNUC_UNUSED uint probability, GstCaps *caps, Dvb *dvb )
{
	GstElementFactory *factory = dvb_find_factory ( caps, GST_ELEMENT_FACTORY_TYPE_PARSER );

	GstElement *element = gst_element_factory_create ( factory, NULL );

	gst_bin_add ( GST_BIN ( dvb->playdvb ), element );

	gst_element_link_many ( typefind, element, dvb->recmux, NULL );

	gst_element_set_state ( element, GST_STATE_PLAYING );
}

static void dvb_create_elements_audio_video_rec ( GstPad *pad, const char *name, Dvb *dvb )
{
	GstElement *queue2   = gst_element_factory_make ( "queue2",   NULL );
	GstElement *typefind = gst_element_factory_make ( "typefind", NULL );

	if ( !queue2 || !typefind ) { g_critical ( "%s:: recbin ... - not created.", __func__ ); return; }

	gst_bin_add_many ( GST_BIN ( dvb->playdvb ), queue2, typefind, NULL );

	gst_element_link ( queue2, typefind );

	dvb_pad_link ( pad, queue2, g_str_has_prefix ( name, "audio" ) ? "demux-rec - audio" : "demux-rec - video" );

	g_signal_connect ( typefind, "have-type", G_CALLBACK ( dvb_typefind_parser ), dvb );

	gst_element_set_state ( queue2,   GST_STATE_PLAYING );
	gst_element_set_state ( typefind, GST_STATE_PLAYING );
}

static void dvb_add_pad_demux_rec ( G_GNUC_UNUSED GstElement *element, GstPad *pad, Dvb *dvb )
{
	if ( dvb_pad_check_type ( pad, "audio" ) ) dvb_create_elements_audio_video_rec ( pad, "audio", dvb );
	if ( dvb_pad_check_type ( pad, "video" ) ) dvb_create_elements_audio_video_rec ( pad, "video", dvb );
}

static void dvb_create_rec ( const char *path, Dvb *dvb )
{
	dvb->recmux   = gst_element_factory_make ( "mpegtsmux", NULL );
	dvb->recdemux = gst_element_factory_make ( "tsdemux",   NULL );

	GstElement *filesink = gst_element_factory_make ( "filesink", NULL );

	if ( !dvb->recmux || !dvb->recdemux || !filesink ) { g_critical ( "%s:: recbin ... - not created. ", __func__ ); return; }

	gst_bin_add_many ( GST_BIN ( dvb->playdvb ), dvb->recdemux, dvb->recmux, filesink, NULL );

	g_object_set ( dvb->recdemux, "program-number", dvb->sid, NULL );
	g_object_set ( filesink, "location", path, NULL );

	gst_element_link ( dvb->teerec, dvb->recdemux );
	gst_element_link ( dvb->recmux, filesink );

	gst_element_set_state ( dvb->recdemux, GST_STATE_PLAYING );
	gst_element_set_state ( dvb->recmux, GST_STATE_PLAYING );
	gst_element_set_state ( filesink, GST_STATE_PLAYING );

	g_signal_connect ( dvb->recdemux, "pad-added", G_CALLBACK ( dvb_add_pad_demux_rec ), dvb );
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

	g_autofree char *dvb_name = dvb_get_name ( adapter, frontend );

	g_debug ( "%s:: %s ", __func__, dvb_name );
}

static RetSidLnb dvb_data_set ( const char *data, GstElement *element, GstElement *demux )
{
	RetSidLnb sl;

	sl.lnb = 0;
	sl.sid = 0;
	sl.lo_found = FALSE;

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
			sl.sid = (uint16_t)dat;
			g_object_set ( demux, "program-number", dat, NULL );
		}
		else if ( g_strrstr ( splits[0], "symbol-rate" ) )
		{
			g_object_set ( element, "symbol-rate", ( dat > 1000000 ) ? dat / 1000 : dat, NULL );
		}
		else if ( g_strrstr ( splits[0], "lnb-type" ) )
		{
			sl.lnb = (uint8_t)dat;

			if ( g_strrstr ( data, "lnb-lof" ) ) sl.lo_found = TRUE;

			Descr *descr = descr_new ();

			g_signal_emit_by_name ( descr, "descr-lnbs", (uint)dat, element );

			g_object_unref ( descr );
		}
		else
		{
			g_object_set ( element, splits[0], dat, NULL );
		}

		g_strfreev (splits);
	}

	g_strfreev (fields);

	dvb_rinit ( element );

	return sl;
}

static void dvb_play ( Dvb *dvb )
{
	gst_element_set_state ( dvb->playdvb, GST_STATE_PLAYING );

	gtk_widget_queue_draw ( GTK_WIDGET ( dvb ) );

	g_signal_emit_by_name ( dvb, "dvb-icon-scan-info", TRUE );
}

static uint dvb_lnb_get_lhs ( GstElement *element, const char *param )
{
	uint freq = 0;
	g_object_get ( element, param, &freq, NULL );

	return freq / 1000;
}

static void dvb_lnb_changed_spin_all ( GtkSpinButton *button, GstElement *element )
{
	gtk_spin_button_update ( button );

	uint freq = (uint)gtk_spin_button_get_value ( button );

	g_object_set ( element, gtk_widget_get_name ( GTK_WIDGET ( button ) ), freq *= 1000, NULL );
}

static void dvb_lnb_win_quit ( G_GNUC_UNUSED GtkWindow *window, Dvb *dvb )
{
	dvb_play ( dvb );
}

static void dvb_lnb_win ( GstElement *element, Dvb *dvb )
{
	GtkWindow *win_base = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb ) ) );

	GtkWindow *window = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title ( window, "" );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_icon_name ( window, "helia" );
	gtk_window_set_transient_for ( window, win_base );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_default_size ( window, 400, -1 );
	g_signal_connect ( window, "destroy", G_CALLBACK ( dvb_lnb_win_quit ), dvb );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( grid, TRUE );
	gtk_grid_set_row_spacing ( grid, 5 );
	gtk_box_pack_start ( m_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	gtk_widget_set_visible ( GTK_WIDGET ( m_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( grid  ), TRUE );

	struct data_a { const char *text; const char *name; uint value; } data_a_n[] =
	{
		{ "LNB LOf1   MHz", "lnb-lof1", dvb_lnb_get_lhs ( element, "lnb-lof1" ) },
		{ "LNB LOf2   MHz", "lnb-lof2", dvb_lnb_get_lhs ( element, "lnb-lof2" ) },
		{ "LNB Switch MHz", "lnb-slof", dvb_lnb_get_lhs ( element, "lnb-slof" ) }
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
		g_signal_connect ( spinbutton, "changed", G_CALLBACK ( dvb_lnb_changed_spin_all ), element );

		gtk_widget_set_visible ( GTK_WIDGET ( spinbutton ), TRUE );
		gtk_grid_attach ( grid, GTK_WIDGET ( spinbutton ), 1, d, 1, 1 );
	}

	GtkButton *button_close = (GtkButton *)gtk_button_new_from_icon_name ( "helia-play", GTK_ICON_SIZE_MENU );
	gtk_widget_set_visible ( GTK_WIDGET ( button_close ), TRUE );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_end ( m_box, GTK_WIDGET ( button_close ), FALSE, FALSE, 0 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_window_present ( window );
}

static void dvb_stop_set_play ( const char *data, Dvb *dvb )
{
	double value = 1.0;
	if ( dvb->volume ) g_object_get ( dvb->volume, "volume", &value, NULL );

	dvb->volume_val = value;

	dvb_set_stop ( dvb );
	dvb_remove_bin ( dvb->playdvb, NULL );

	dvb->volume = NULL;

	dvb->record = FALSE;
	dvb->set_video = FALSE;
	dvb->first_audio = FALSE;

	dvb_create_bin ( dvb );

	if ( !dvb->dvbsrc ) return;

	RetSidLnb sl = dvb_data_set ( data, dvb->dvbsrc, dvb->demux );
	dvb->sid = sl.sid;

	if ( sl.lnb == LNB_MNL && !sl.lo_found ) { dvb_lnb_win ( dvb->dvbsrc, dvb ); return; }

	dvb_play ( dvb );
}

static void dvb_rec ( Dvb *dvb )
{
	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state == GST_STATE_NULL ) return;

	if ( dvb->record )
	{
		dvb->record = FALSE;
		dvb->first_audio = FALSE;

		gst_element_set_state ( dvb->playdvb, GST_STATE_NULL );

		dvb_create_bin_rm_rec ( dvb );

		gst_element_set_state ( dvb->playdvb, GST_STATE_PLAYING );
	}
	else
	{
		g_autofree char *dt = dvb_time_to_str ();

		char file[PATH_MAX];
		sprintf ( file, "Record-Dvb-%s.m2ts", dt );

		g_autofree char *path = dvb_save_dialog ( dvb->rec_dir, file, dvb );

		if ( path == NULL ) return;

		free ( dvb->rec_dir );
		dvb->rec_dir = g_path_get_dirname ( path );

		dvb_create_rec ( path, dvb );

		dvb->record = TRUE;
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

static void dvb_msg_all ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Dvb *dvb )
{
	const GstStructure *structure = gst_message_get_structure ( msg );

	if ( structure && dvb->level )
	{
		int signal = 0, snr = 0;
		gboolean lock = FALSE;

		if ( gst_structure_get_int ( structure, "signal", &signal ) )
		{
			gst_structure_get_int     ( structure, "snr",  &snr  );
			gst_structure_get_boolean ( structure, "lock", &lock );

			uint8_t ret_snr = (uint8_t)( snr * 100 / 0xffff );
			uint8_t ret_sgl = (uint8_t)( signal * 100 / 0xffff );

			if ( GTK_IS_WIDGET ( dvb->level ) ) g_signal_emit_by_name ( dvb->level, "level-update", ret_sgl, ret_snr, lock, dvb->record );
		}
	}
}

static void dvb_msg_err ( G_GNUC_UNUSED GstBus *bus, GstMessage *msg, Dvb *dvb )
{
	GError *err = NULL;
	char   *dbg = NULL;

	gst_message_parse_error ( msg, &err, &dbg );

	g_critical ( "%s: %s (%s)", __func__, err->message, (dbg) ? dbg : "no details" );

	dvb_message_dialog ( " ", err->message, GTK_MESSAGE_ERROR, dvb );

	g_error_free ( err );
	g_free ( dbg );

	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state != GST_STATE_PLAYING )
		dvb_set_stop ( dvb );
}

static GstElement * dvb_create ( Dvb *dvb )
{
	g_autofree char *name = g_strdup_printf ( "pipeline-dvb-%u", dvb->win_count );

	GstElement *dvbplay = gst_pipeline_new ( ( dvb->win_count ) ? name : "pipeline-dvb" );

	if ( !dvbplay ) { g_critical ( "%s: dvbplay - not created.", __func__ ); return NULL; }

	if ( dvb->win_count ) return dvbplay;

	GstBus *bus = gst_element_get_bus ( dvbplay );

	gst_bus_add_signal_watch_full ( bus, G_PRIORITY_DEFAULT );
	gst_bus_set_sync_handler ( bus, (GstBusSyncHandler)dvb_sync_handler, dvb, NULL );

	g_signal_connect ( bus, "message",        G_CALLBACK ( dvb_msg_all ), dvb );
	g_signal_connect ( bus, "message::error", G_CALLBACK ( dvb_msg_err ), dvb );

	gst_object_unref ( bus );

	return dvbplay;
}

static gboolean dvb_fullscreen ( GtkWindow *window )
{
	GdkWindowState state = gdk_window_get_state ( gtk_widget_get_window ( GTK_WIDGET ( window ) ) );

	if ( state & GDK_WINDOW_STATE_FULLSCREEN )
		{ gtk_window_unfullscreen ( window ); return FALSE; }
	else
		{ gtk_window_fullscreen   ( window ); return TRUE;  }

	return TRUE;
}

static gboolean dvb_video_press_event ( GtkDrawingArea *draw, GdkEventButton *event, Dvb *dvb )
{
	GtkWindow *window_base = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( draw ) ) );

	if ( event->button == GDK_BUTTON_PRIMARY )
	{
		if ( event->type == GDK_2BUTTON_PRESS )
		{
			if ( dvb_fullscreen ( window_base ) ) g_signal_emit_by_name ( dvb, "dvb-base", 1 );
		}
	}
	if ( event->button == GDK_BUTTON_MIDDLE )
	{
		if ( dvb->win_count )
			dvb_multi_destroy ( dvb );
		else
			g_signal_emit_by_name ( dvb, "dvb-base", 2 );
	}

	if ( event->button == GDK_BUTTON_SECONDARY )  dvb_set_mute ( dvb );

	return GDK_EVENT_PROPAGATE;
}

static void dvb_show_cursor ( GtkDrawingArea *draw, gboolean show_cursor )
{
	GdkWindow *window = gtk_widget_get_window ( GTK_WIDGET ( draw ) );

	GdkCursor *cursor = gdk_cursor_new_for_display ( gdk_display_get_default (), ( show_cursor ) ? GDK_ARROW : GDK_BLANK_CURSOR );

	gdk_window_set_cursor ( window, cursor );

	g_object_unref (cursor);
}

static gboolean dvb_video_notify_event ( GtkDrawingArea *draw, G_GNUC_UNUSED GdkEventMotion *event, Dvb *dvb )
{
	time ( &dvb->t_hide );

	dvb_show_cursor ( draw, TRUE );

	return GDK_EVENT_STOP;
}

static gboolean dvb_set_cursor ( Dvb *dvb )
{
	if ( !GST_IS_ELEMENT ( dvb->playdvb ) ) { dvb->src_tm = 0; return FALSE; }

	time_t t_cur;
	time ( &t_cur );

	if ( ( t_cur - dvb->t_hide < 3 ) ) return TRUE;

	gboolean show = FALSE;
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb ) ) );

	if ( !gtk_window_is_active ( window ) ) show = TRUE;

	GtkDrawingArea *video = GTK_DRAWING_AREA ( dvb );

	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state != GST_STATE_NULL ) dvb_show_cursor ( video, show );

	return TRUE;
}

static void dvb_video_draw_black ( GtkDrawingArea *area, cairo_t *cr, const char *name, uint16_t w, uint16_t h )
{
	GdkRGBA color; color.red = 0; color.green = 0; color.blue = 0; color.alpha = 1.0;

	int width = gtk_widget_get_allocated_width  ( GTK_WIDGET ( area ) );
	int heigh = gtk_widget_get_allocated_height ( GTK_WIDGET ( area ) );

	cairo_rectangle ( cr, 0, 0, width, heigh );
	gdk_cairo_set_source_rgba ( cr, &color );
	cairo_fill (cr);

	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), name, w, GTK_ICON_LOOKUP_FORCE_SIZE, NULL );

	if ( pixbuf != NULL )
	{
		cairo_rectangle ( cr, 0, 0, width, heigh );
		gdk_cairo_set_source_pixbuf ( cr, pixbuf, ( width / 2  ) - ( w  / 2 ), ( heigh / 2 ) - ( h / 2 ) );
		cairo_fill (cr);
	}

	if ( pixbuf ) g_object_unref ( pixbuf );
}

static gboolean dvb_video_draw ( GtkDrawingArea *area, cairo_t *cr, Dvb *dvb )
{
	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state == GST_STATE_NULL || !dvb->set_video )
		dvb_video_draw_black ( area, cr, "helia-logo", 96, 96 );

	return FALSE;
}

static void dvb_video_realize ( GtkDrawingArea *draw, Dvb *dvb )
{
#ifdef GDK_WINDOWING_X11

    dvb->xid = GDK_WINDOW_XID ( gtk_widget_get_window ( GTK_WIDGET ( draw ) ) );

#endif
#ifdef GDK_WINDOWING_WIN32

    HWND wnd = GDK_WINDOW_HWND ( gtk_widget_get_window ( GTK_WIDGET ( draw ) ) );
    dvb->xid = (guintptr) wnd;

#endif
}

static void dvb_create_video ( Dvb *dvb )
{
	GtkDrawingArea *video = GTK_DRAWING_AREA ( dvb );

	gtk_widget_set_visible ( GTK_WIDGET ( video ), TRUE );
	gtk_widget_set_events ( GTK_WIDGET ( video ), GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK );

	g_signal_connect ( video, "draw",    G_CALLBACK ( dvb_video_draw    ), dvb );
	g_signal_connect ( video, "realize", G_CALLBACK ( dvb_video_realize ), dvb );

	g_signal_connect ( video, "button-press-event",  G_CALLBACK ( dvb_video_press_event  ), dvb );
	g_signal_connect ( video, "motion-notify-event", G_CALLBACK ( dvb_video_notify_event ), dvb );
}

static gboolean dvb_handler_isplay ( Dvb *dvb )
{
	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state == GST_STATE_PLAYING ) return TRUE;

	return FALSE;
}

static void dvb_handler_play ( Dvb *dvb, const char *data )
{
	dvb_stop_set_play ( data, dvb );
}

static void dvb_handler_stop ( Dvb *dvb )
{
	dvb_set_stop ( dvb );
}

static void dvb_handler_rec ( Dvb *dvb )
{
	dvb_rec ( dvb );
}

static void dvb_handler_mute ( Dvb *dvb )
{
	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state != GST_STATE_NULL ) dvb_set_mute ( dvb );
}

static void dvb_handler_vol ( Dvb *dvb, double val )
{
	if ( GST_ELEMENT_CAST ( dvb->playdvb )->current_state != GST_STATE_NULL ) dvb_set_volume ( val, dvb );
}

static uint16_t dvb_get_sid ( const char *data )
{
	uint16_t ret = 0;

	char **fields = g_strsplit ( data, ":", 0 );
	uint j = 0, numfields = g_strv_length ( fields );

	for ( j = 1; j < numfields; j++ )
	{
		if ( g_strrstr ( fields[j], "program-number" ) )
		{
			char **splits = g_strsplit ( fields[j], "=", 0 );

			g_debug ( "%s: gst-param %s | gst-value %s ", __func__, splits[0], splits[1] );

			ret = (uint16_t)atoi ( splits[1] );

			g_strfreev ( splits );
		}
	}

	g_strfreev ( fields );

	return ret;
}

static void dvb_create_bin_multi ( uint16_t sid, Dvb *dvb )
{
	GstElement *queue2 = gst_element_factory_make ( "queue2",  NULL );
	dvb->demux  = gst_element_factory_make ( "tsdemux", NULL );

	if ( !queue2 || !dvb->demux ) { g_critical ( "%s:: tsdemux ... - not created.", __func__ ); return; }

	gst_bin_add_many ( GST_BIN ( dvb->playdvb ), queue2, dvb->demux, NULL );

	gst_element_link_many ( queue2, dvb->demux, NULL );

	g_object_set ( dvb->demux, "program-number", sid, NULL );

	GstPad *pad_host = gst_element_get_static_pad ( queue2, "sink" );
	gst_element_add_pad ( dvb->playdvb, gst_ghost_pad_new ( "sink", pad_host ) );
	gst_object_unref ( pad_host );

	g_signal_connect ( dvb->demux, "pad-added", G_CALLBACK ( dvb_add_pad_demux ), dvb );
}

static void dvb_multi_destroy ( Dvb *dvb )
{
	gst_element_unlink ( dvb->tee_base, dvb->playdvb );

	gst_element_set_state ( dvb->playdvb, GST_STATE_NULL );

	GstElement *element = GST_ELEMENT ( gst_element_get_parent ( dvb->tee_base ) );

	gst_bin_remove ( GST_BIN ( element ), dvb->playdvb );

	g_signal_emit_by_name ( dvb, "dvb-base", 99 );

	gtk_widget_destroy ( GTK_WIDGET ( dvb ) );
}

static void dvb_handler_multi ( Dvb *dvb, const char *data, gpointer base_dvb )
{
	Dvb *dvb_base = base_dvb;

	dvb_base->xid = dvb->xid;

	dvb->volume_val = 1.0;

	dvb->volume = NULL;

	dvb->record = FALSE;
	dvb->set_video = FALSE;
	dvb->first_audio = FALSE;

	dvb->tee_base = dvb_base->teerec;

	uint16_t sid = dvb_get_sid ( data );
	dvb_create_bin_multi ( sid, dvb );

	gst_element_set_state ( dvb->playdvb, GST_STATE_PLAYING );

	gst_bin_add ( GST_BIN ( dvb_base->playdvb ), dvb->playdvb );
	gst_element_link ( dvb_base->teerec, dvb->playdvb );
}

static void dvb_init ( Dvb *dvb )
{
	dvb->sid = 0;
	dvb->xid = 0;

	dvb->dvbsrc = NULL;
	dvb->volume = NULL;
	dvb->record = FALSE;

	dvb->rec_dir = g_strdup ( g_get_home_dir () );

	dvb_create_video ( dvb );

	g_signal_connect ( dvb, "dvb-rec",  G_CALLBACK ( dvb_handler_rec  ), NULL );
	g_signal_connect ( dvb, "dvb-play", G_CALLBACK ( dvb_handler_play ), NULL );
	g_signal_connect ( dvb, "dvb-stop", G_CALLBACK ( dvb_handler_stop ), NULL );
	g_signal_connect ( dvb, "dvb-mute", G_CALLBACK ( dvb_handler_mute ), NULL );
	g_signal_connect ( dvb, "dvb-vol",  G_CALLBACK ( dvb_handler_vol  ), NULL );

	g_signal_connect ( dvb, "dvb-get-sid", G_CALLBACK ( dvb_handler_getsid ), NULL );
	g_signal_connect ( dvb, "dvb-is-play", G_CALLBACK ( dvb_handler_isplay ), NULL );
	g_signal_connect ( dvb, "dvb-combo-lang", G_CALLBACK ( dvb_handler_combo_lang ), NULL );

	g_signal_connect ( dvb, "dvb-multi", G_CALLBACK ( dvb_handler_multi ), NULL );

	dvb->src_tm = g_timeout_add_seconds ( 1, (GSourceFunc)dvb_set_cursor, dvb );
}

static void dvb_finalize ( GObject *object )
{
	Dvb *dvb = DVB_DRAW ( object );

	free ( dvb->rec_dir );

	if ( dvb->src_tm ) g_source_remove ( dvb->src_tm );

	if ( !dvb->win_count && GST_IS_ELEMENT ( dvb->playdvb ) )
	{
		gst_element_set_state ( dvb->playdvb, GST_STATE_NULL );
		gst_object_unref ( dvb->playdvb );
	}

	G_OBJECT_CLASS (dvb_parent_class)->finalize (object);
}

static void dvb_class_init ( DvbClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = dvb_finalize;

	g_signal_new ( "dvb-rec",  G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0 );
	g_signal_new ( "dvb-stop", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0 );
	g_signal_new ( "dvb-mute", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0 );
	g_signal_new ( "dvb-play", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );
	g_signal_new ( "dvb-vol",  G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_DOUBLE );
	g_signal_new ( "dvb-base", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_UINT   );

	g_signal_new ( "dvb-get-sid",    G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_UINT,    0 );
	g_signal_new ( "dvb-combo-lang", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_OBJECT,  0 );
	g_signal_new ( "dvb-is-play",    G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_BOOLEAN, 0 );
	g_signal_new ( "dvb-icon-scan-info", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN );

	g_signal_new ( "dvb-multi", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_POINTER );
}

Dvb * dvb_new ( uint8_t win_count, Level *level )
{
	Dvb *dvb = g_object_new ( DVB_TYPE_DRAW, NULL );

	dvb->level = level;

	dvb->win_count = win_count;

	dvb->playdvb = dvb_create ( dvb );

	return dvb;
}
