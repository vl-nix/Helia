/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "helia-dvb.h"
#include "dvb.h"
#include "scan.h"
#include "file.h"
#include "level.h"
#include "default.h"
#include "settings.h"
#include "enc-prop.h"
#include "tree-view.h"
#include "control-tv.h"

#include <time.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

struct _HeliaDvb
{
	GtkBox parent_instance;

	GtkBox *playlist;
	TreeView *treeview;
	GtkDrawingArea *video;

	Dvb *dvb;
	Level *level;

	ulong xid;
	time_t t_hide;
};

G_DEFINE_TYPE ( HeliaDvb, helia_dvb, GTK_TYPE_BOX )

typedef void ( *fp ) ( HeliaDvb *dvb );

static void helia_dvb_treeview_enc_handler ( G_GNUC_UNUSED TreeView *treeview, HeliaDvb *dvb )
{
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb->video ) ) );

	EncProp *ep = enc_prop_new ();
	g_signal_emit_by_name ( dvb->dvb, "dvb-enc", G_OBJECT ( window ), ep );
}

static void helia_dvb_treeview_scan_append_handler ( G_GNUC_UNUSED Scan *scan, const char *name, const char *data, HeliaDvb *dvb )
{
	g_signal_emit_by_name ( dvb->treeview, "treeview-append-data", name, data );
}

static void helia_dvb_treeview_scan_handler ( G_GNUC_UNUSED TreeView *treeview, G_GNUC_UNUSED GtkButton *button, HeliaDvb *dvb )
{
	GtkWindow *win_base = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb->video ) ) );

	Scan *scan = scan_new ( win_base );
	g_signal_connect ( scan, "scan-append", G_CALLBACK ( helia_dvb_treeview_scan_append_handler ), dvb );

}
static void helia_dvb_treeview_close_handler ( G_GNUC_UNUSED TreeView *treeview, HeliaDvb *dvb )
{
	gtk_widget_set_visible ( GTK_WIDGET ( dvb->playlist ), FALSE );
}

static void helia_dvb_treeview_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, G_GNUC_UNUSED GtkTreeViewColumn *column, HeliaDvb *dvb )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	if ( gtk_tree_model_get_iter ( model, &iter, path ) )
	{
		g_autofree char *data = NULL;

		gtk_tree_model_get ( model, &iter, COL_DATA, &data, -1 );

		g_signal_emit_by_name ( dvb->dvb, "dvb-play", dvb->xid, data );

		gtk_widget_queue_draw ( GTK_WIDGET ( dvb->video ) );
	}
}

static void helia_dvb_add_channels ( HeliaDvb *dvb )
{
	char path[PATH_MAX];
	sprintf ( path, "%s/helia/gtv-channel.conf", g_get_user_config_dir () );

	if ( g_file_test ( path, G_FILE_TEST_EXISTS ) ) g_signal_emit_by_name ( dvb->treeview, "treeview-add-channels", path );
}

static GtkBox * helia_dvb_create_treeview_scroll ( HeliaDvb *dvb )
{
	GtkBox *v_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( v_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( v_box ), TRUE );

	GtkScrolledWindow *sw = (GtkScrolledWindow *)gtk_scrolled_window_new ( NULL, NULL );
	gtk_scrolled_window_set_policy ( sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_widget_set_size_request ( GTK_WIDGET ( sw ), 220, -1 );
	gtk_widget_set_visible ( GTK_WIDGET ( sw ), TRUE );

	const char *title[3] = { "Num", "Channels", "Data" };

	dvb->treeview = tree_view_new ( title );
	gtk_widget_set_visible ( GTK_WIDGET ( dvb->treeview ), TRUE );
	g_signal_connect ( dvb->treeview, "row-activated", G_CALLBACK ( helia_dvb_treeview_row_activated ), dvb );

	gtk_container_add ( GTK_CONTAINER ( sw ), GTK_WIDGET ( dvb->treeview ) );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( sw ), TRUE, TRUE, 0 );

	GtkBox *box_tool = create_treeview_box ( TRUE, dvb->treeview );
	gtk_widget_set_visible ( GTK_WIDGET ( box_tool ), TRUE );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( box_tool ), 5 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( box_tool ), 5 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( box_tool ), 5 );
	gtk_box_pack_end ( v_box, GTK_WIDGET ( box_tool ), FALSE, FALSE, 0 );

	g_signal_connect ( dvb->treeview, "treeview-enc",   G_CALLBACK ( helia_dvb_treeview_enc_handler   ), dvb );
	g_signal_connect ( dvb->treeview, "treeview-repeat-scan",  G_CALLBACK ( helia_dvb_treeview_scan_handler  ), dvb );
	g_signal_connect ( dvb->treeview, "treeview-close", G_CALLBACK ( helia_dvb_treeview_close_handler ), dvb );

	dvb->level = level_new ();
	gtk_widget_set_visible ( GTK_WIDGET ( dvb->level ), TRUE );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( dvb->level ), 5 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( dvb->level ), 5 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( dvb->level ), 5 );

	gtk_box_pack_end ( v_box, GTK_WIDGET ( dvb->level ), FALSE, FALSE, 0 );

	helia_dvb_add_channels ( dvb );

	return v_box;
}

static void dvb_set_stop ( HeliaDvb *dvb )
{
	g_signal_emit_by_name ( dvb->dvb, "dvb-stop" );

	g_signal_emit_by_name ( dvb->dvb, "dvb-level", 0, 0, FALSE, FALSE, 0, 0 );

	gtk_widget_queue_draw ( GTK_WIDGET ( dvb->video ) );

	g_signal_emit_by_name ( dvb, "power-set", FALSE );

}

static void dvb_set_base ( HeliaDvb *dvb )
{
	dvb_set_stop ( dvb );

	g_signal_emit_by_name ( dvb, "return-base" );
}

static void dvb_set_playlist ( HeliaDvb *dvb )
{
	if ( gtk_widget_get_visible ( GTK_WIDGET ( dvb->playlist ) ) )
		gtk_widget_set_visible ( GTK_WIDGET ( dvb->playlist ), FALSE );
	else
		gtk_widget_set_visible ( GTK_WIDGET ( dvb->playlist ), TRUE );
}

static void dvb_set_eqa ( HeliaDvb *dvb )
{
	uint opacity_p = OPACITY;

	GSettings *setting = settings_init ();
	if ( setting ) opacity_p = g_settings_get_uint ( setting, "opacity-panel" );

	double opacity = (float)opacity_p / 100;
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb->video ) ) );

	g_signal_emit_by_name ( dvb->dvb, "dvb-eqa", G_OBJECT ( window ), opacity );

	if ( setting ) g_object_unref ( setting );
}

static void dvb_set_eqv ( HeliaDvb *dvb )
{
	uint opacity_p = OPACITY;

	GSettings *setting = settings_init ();
	if ( setting ) opacity_p = g_settings_get_uint ( setting, "opacity-panel" );

	double opacity = (float)opacity_p / 100;
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb->video ) ) );

	g_signal_emit_by_name ( dvb->dvb, "dvb-eqv", G_OBJECT ( window ), opacity );

	if ( setting ) g_object_unref ( setting );
}

static void dvb_set_mute ( HeliaDvb *dvb )
{
	gboolean mute = FALSE;
	g_object_get ( dvb->dvb, "mute", &mute, NULL );
	g_object_set ( dvb->dvb, "mute", !mute, NULL );
}

static void dvb_set_rec ( HeliaDvb *dvb )
{
	gboolean ret = FALSE;
	g_signal_emit_by_name ( dvb->dvb, "dvb-is-play", &ret );

	if ( ret ) g_signal_emit_by_name ( dvb->dvb, "dvb-rec" );
}

static void dvb_set_info ( HeliaDvb *dvb )
{
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb->video ) ) );

	g_signal_emit_by_name ( dvb->dvb, "dvb-info", G_OBJECT ( window ) );
}

static void dvb_set_scan ( HeliaDvb *dvb )
{
	GtkWindow *win_base = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb->video ) ) );

	Scan *scan = scan_new ( win_base );
	g_signal_connect ( scan, "scan-append", G_CALLBACK ( helia_dvb_treeview_scan_append_handler ), dvb );
}

static void helia_dvb_clicked_handler_ctv ( G_GNUC_UNUSED ControlTv *ctv, uint8_t num, HeliaDvb *dvb )
{
	fp funcs[] =  { dvb_set_base, dvb_set_playlist, dvb_set_eqa,  dvb_set_eqv,  dvb_set_mute,
			dvb_set_stop, dvb_set_rec,      dvb_set_scan, dvb_set_info, NULL };

	if ( funcs[num] ) funcs[num] ( dvb );
}

static void helia_dvb_handler_set_vol_ctv ( G_GNUC_UNUSED ControlTv *ctv, double value, HeliaDvb *dvb )
{
	g_object_set ( dvb->dvb, "volume", value, NULL );
}


static gboolean helia_dvb_video_fullscreen ( GtkWindow *window )
{
	GdkWindowState state = gdk_window_get_state ( gtk_widget_get_window ( GTK_WIDGET ( window ) ) );

	if ( state & GDK_WINDOW_STATE_FULLSCREEN )
		{ gtk_window_unfullscreen ( window ); return FALSE; }
	else
		{ gtk_window_fullscreen   ( window ); return TRUE;  }

	return TRUE;
}

static gboolean helia_dvb_video_press_event ( GtkDrawingArea *draw, GdkEventButton *event, HeliaDvb *dvb )
{
	GtkWindow *window_base = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( draw ) ) );

	if ( event->button == 1 )
	{
		if ( event->type == GDK_2BUTTON_PRESS )
		{
			if ( helia_dvb_video_fullscreen ( GTK_WINDOW ( window_base ) ) )
			{
				gtk_widget_set_visible ( GTK_WIDGET ( dvb->playlist ), FALSE );
			}
		}
	}

	if ( event->button == 2 )
	{
		if ( gtk_widget_get_visible ( GTK_WIDGET ( dvb->playlist ) ) )
		{
			gtk_widget_set_visible ( GTK_WIDGET ( dvb->playlist ), FALSE );
		}
		else
		{
			gtk_widget_set_visible ( GTK_WIDGET ( dvb->playlist ), TRUE );
		}
	}

	if ( event->button == 3 )
	{
		uint opacity_p = OPACITY;
		uint icon_size = ICON_SIZE;

		GSettings *setting = settings_init ();
		if ( setting ) opacity_p = g_settings_get_uint ( setting, "opacity-panel" );
		if ( setting ) icon_size = g_settings_get_uint ( setting, "icon-size" );

		double volume = 0; // VOLUME;
		g_object_get ( dvb->dvb, "volume", &volume, NULL );

		ControlTv *ctv = control_tv_new ( (uint16_t)opacity_p, (uint16_t)icon_size, volume, window_base );
		g_signal_connect ( ctv, "dvb-click-num",  G_CALLBACK ( helia_dvb_clicked_handler_ctv ), dvb );
		g_signal_connect ( ctv, "dvb-set-volume", G_CALLBACK ( helia_dvb_handler_set_vol_ctv ), dvb );

		if ( setting ) g_object_unref ( setting );
	}

	return TRUE;
}

static void helia_dvb_video_draw_black ( GtkDrawingArea *area, cairo_t *cr, const char *name, uint16_t size )
{
	GdkRGBA color; color.red = 0; color.green = 0; color.blue = 0; color.alpha = 1.0;

	int width = gtk_widget_get_allocated_width  ( GTK_WIDGET ( area ) );
	int heigh = gtk_widget_get_allocated_height ( GTK_WIDGET ( area ) );

	cairo_rectangle ( cr, 0, 0, width, heigh );
	gdk_cairo_set_source_rgba ( cr, &color );
	cairo_fill (cr);

	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), name, size, GTK_ICON_LOOKUP_FORCE_SIZE, NULL );

	if ( pixbuf != NULL )
	{
		cairo_rectangle ( cr, 0, 0, width, heigh );
		gdk_cairo_set_source_pixbuf ( cr, pixbuf, ( width / 2  ) - ( size  / 2 ), ( heigh / 2 ) - ( size / 2 ) );
		cairo_fill (cr);
	}

	if ( pixbuf ) g_object_unref ( pixbuf );
}

static gboolean helia_dvb_video_draw_check ( HeliaDvb *dvb )
{
	gboolean ret = FALSE;
	g_signal_emit_by_name ( dvb->dvb, "dvb-video", &ret );

	return ret;
}

static gboolean helia_dvb_video_draw ( GtkDrawingArea *area, cairo_t *cr, HeliaDvb *dvb )
{
	if ( helia_dvb_video_draw_check ( dvb ) ) helia_dvb_video_draw_black ( area, cr, "helia-tv", 96 );

	return FALSE;
}

static void helia_dvb_video_realize ( GtkDrawingArea *draw, HeliaDvb *dvb )
{
	dvb->xid = GDK_WINDOW_XID ( gtk_widget_get_window ( GTK_WIDGET ( draw ) ) );
}

static GtkPaned * helia_dvb_create_paned ( GtkBox *playlist, GtkDrawingArea *video )
{
	GtkPaned *paned = (GtkPaned *)gtk_paned_new ( GTK_ORIENTATION_HORIZONTAL );
	gtk_paned_add1 ( paned, GTK_WIDGET ( playlist ) );
	gtk_paned_add2 ( paned, GTK_WIDGET ( video    ) );

	return paned;
}

static void helia_dvb_show_cursor ( GtkDrawingArea *draw, gboolean show_cursor )
{
	GdkWindow *window = gtk_widget_get_window ( GTK_WIDGET ( draw ) );

	GdkCursor *cursor = gdk_cursor_new_for_display ( gdk_display_get_default (), ( show_cursor ) ? GDK_ARROW : GDK_BLANK_CURSOR );

	gdk_window_set_cursor ( window, cursor );

	g_object_unref (cursor);
}

static gboolean helia_dvb_video_notify_event ( GtkDrawingArea *draw, G_GNUC_UNUSED GdkEventMotion *event, HeliaDvb *dvb )
{
	time ( &dvb->t_hide );

	helia_dvb_show_cursor ( draw, TRUE );

	return GDK_EVENT_STOP;
}

static void helia_dvb_video_hide_cursor ( HeliaDvb *dvb )
{
	time_t t_cur;
	time ( &t_cur );

	if ( ( t_cur - dvb->t_hide < 2 ) ) return;

	gboolean show = FALSE;
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb->video ) ) );

	if ( !gtk_window_is_active ( window ) ) helia_dvb_show_cursor ( dvb->video, TRUE ); else helia_dvb_show_cursor ( dvb->video, show );
}

static void helia_dvb_handler_level ( G_GNUC_UNUSED Dvb *d, uint sgl, uint snr, gboolean lock, gboolean rec, uint bitrate_a, uint bitrate_v, HeliaDvb *dvb )
{
	helia_dvb_video_hide_cursor ( dvb );

	level_update ( (uint8_t)sgl, (uint8_t)snr, lock, rec, bitrate_a, bitrate_v, dvb->level );
}

static void helia_dvb_handler_message_dialog ( G_GNUC_UNUSED Dvb *d, const char *message, HeliaDvb *dvb )
{
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( dvb->video ) ) );

	helia_message_dialog ( "", message, GTK_MESSAGE_ERROR, window );
}

static void helia_dvb_handler_power ( G_GNUC_UNUSED Dvb *d, gboolean power, HeliaDvb *dvb )
{
	g_signal_emit_by_name ( dvb, "power-set", power );
}

static void helia_dvb_video_drag_in ( G_GNUC_UNUSED GtkDrawingArea *draw, GdkDragContext *ct, G_GNUC_UNUSED int x, G_GNUC_UNUSED int y,
	GtkSelectionData *s_data, G_GNUC_UNUSED uint info, guint32 time, HeliaDvb *dvb )
{
	char **uris = gtk_selection_data_get_uris ( s_data );

	uint c = 0; for ( c = 0; uris[c] != NULL; c++ )
	{
		char *path = helia_uri_get_path ( uris[c] );

		if ( path && g_str_has_suffix ( path, "gtv-channel.conf" ) )
			g_signal_emit_by_name ( dvb->treeview, "treeview-add-channels", path );

		free ( path );
	}

	g_strfreev ( uris );
	gtk_drag_finish ( ct, TRUE, FALSE, time );
}

static void helia_dvb_init ( HeliaDvb *dvb )
{
	GtkBox *box = GTK_BOX ( dvb );
	gtk_orientable_set_orientation ( GTK_ORIENTABLE ( box ), GTK_ORIENTATION_VERTICAL );
	gtk_box_set_spacing ( box, 3 );

	dvb->playlist = helia_dvb_create_treeview_scroll ( dvb );

	dvb->video = (GtkDrawingArea *)gtk_drawing_area_new ();
	gtk_widget_set_visible ( GTK_WIDGET ( dvb->video ), TRUE );
	gtk_widget_set_events ( GTK_WIDGET ( dvb->video ), GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK );

	g_signal_connect ( dvb->video, "draw", G_CALLBACK ( helia_dvb_video_draw ), dvb );
	g_signal_connect ( dvb->video, "realize", G_CALLBACK ( helia_dvb_video_realize ), dvb );
	g_signal_connect ( dvb->video, "button-press-event",  G_CALLBACK ( helia_dvb_video_press_event  ), dvb );
	g_signal_connect ( dvb->video, "motion-notify-event", G_CALLBACK ( helia_dvb_video_notify_event ), dvb );

	gtk_drag_dest_set ( GTK_WIDGET ( dvb->video ), GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY );
	gtk_drag_dest_add_uri_targets  ( GTK_WIDGET ( dvb->video ) );
	g_signal_connect  ( dvb->video, "drag-data-received", G_CALLBACK ( helia_dvb_video_drag_in ), dvb );

	GtkPaned *paned = helia_dvb_create_paned ( dvb->playlist, dvb->video );
	gtk_widget_set_visible ( GTK_WIDGET ( paned ), TRUE );

	gtk_box_pack_start ( box, GTK_WIDGET ( paned ), TRUE, TRUE, 0 );

	dvb->dvb = dvb_new ();
	g_signal_connect ( dvb->dvb, "dvb-level", G_CALLBACK ( helia_dvb_handler_level ), dvb );
	g_signal_connect ( dvb->dvb, "dvb-power-set", G_CALLBACK ( helia_dvb_handler_power ), dvb );
	g_signal_connect ( dvb->dvb, "dvb-message-dialog", G_CALLBACK ( helia_dvb_handler_message_dialog ), dvb );
}

static void helia_dvb_destroy ( GtkWidget *object )
{
	HeliaDvb *dvb = HELIA_DVB ( object );

	g_signal_emit_by_name ( dvb->dvb, "dvb-stop" );

	GTK_WIDGET_CLASS ( helia_dvb_parent_class )->destroy ( object );
}

static void helia_dvb_finalize ( GObject *object )
{
	HeliaDvb *dvb = HELIA_DVB ( object );

	g_object_unref ( dvb->dvb );

	G_OBJECT_CLASS ( helia_dvb_parent_class )->finalize (object);
}

static void helia_dvb_class_init ( HeliaDvbClass *class )
{
	GObjectClass *oclass   = G_OBJECT_CLASS ( class );
	GtkWidgetClass *wclass = GTK_WIDGET_CLASS ( class );

	oclass->finalize = helia_dvb_finalize;

	wclass->destroy = helia_dvb_destroy;

	g_signal_new ( "return-base", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "power-set", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN );
}

void helia_dvb_start ( const char *channel, HeliaDvb *dvb )
{
	GtkTreeIter iter;
	gboolean valid = FALSE;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( dvb->treeview ) );

	if ( channel )
	{
		for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
			valid = gtk_tree_model_iter_next ( model, &iter ) )
		{
			char *data = NULL;
			gtk_tree_model_get ( model, &iter, COL_DATA, &data, -1 );

			if ( g_str_has_prefix ( data, channel ) )
			{
				g_signal_emit_by_name ( dvb->dvb, "dvb-play", dvb->xid, data );

				gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( dvb->treeview ) ), &iter );

				free ( data );
				return;
			}

			free ( data );
		}
	}

	if ( gtk_tree_model_get_iter_first ( model, &iter ) )
	{
		g_autofree char *data = NULL;
		gtk_tree_model_get ( model, &iter, COL_DATA, &data, -1 );

		g_signal_emit_by_name ( dvb->dvb, "dvb-play", dvb->xid, data );

		gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( dvb->treeview ) ), &iter );
	}
}

HeliaDvb * helia_dvb_new ( void )
{
	return g_object_new ( HELIA_TYPE_DVB, NULL );
}
