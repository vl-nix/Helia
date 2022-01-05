/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "helia-player.h"
#include "file.h"
#include "info.h"
#include "player.h"
#include "slider.h"
#include "button.h"
#include "default.h"
#include "settings.h"
#include "enc-prop.h"
#include "tree-view.h"
#include "control-mp.h"

#include <time.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

struct _HeliaPlayer
{
	GtkBox parent_instance;

	GtkBox *playlist;
	TreeView *treeview;
	GtkDrawingArea *video;

	GtkLabel *label_buf;
	GtkLabel *label_rec;

	Player *player;
	Slider *level;

	ulong xid;
	time_t t_hide;

	gboolean repeat;
};

G_DEFINE_TYPE ( HeliaPlayer, helia_player, GTK_TYPE_BOX )

typedef void ( *fp ) ( HeliaPlayer *player );

static void player_set_stop ( HeliaPlayer *player );

static void player_first_play ( uint c, HeliaPlayer *player )
{
	gboolean play = FALSE;
	g_signal_emit_by_name ( player->player, "player-is-play", TRUE, &play );

	if ( play ) return;

	g_autofree char *path_get = NULL;
	g_signal_emit_by_name ( player->treeview, "treeview-next", NULL, c, &path_get );

	if ( path_get )
	{
		g_signal_emit_by_name ( player->player, "player-play", player->xid, path_get );
	}
}

static void player_next_play ( const char *uri, HeliaPlayer *player )
{
	g_autofree char *uri_set = g_strdup ( uri );

	if ( player->repeat )
	{
		g_signal_emit_by_name ( player->player, "player-play", player->xid, uri );

		return;
	}

	g_autofree char *path_get = NULL;
	g_signal_emit_by_name ( player->treeview, "treeview-next", uri, 0, &path_get );

	if ( path_get )
	{
		g_signal_emit_by_name ( player->player, "player-play", player->xid, path_get );
	}
	else
		player_set_stop ( player );
}

static void helia_player_handler_next ( G_GNUC_UNUSED Player *p, const char *uri, HeliaPlayer *player )
{
	player_next_play ( uri, player );
}


static void helia_player_treeview_enc_handler ( G_GNUC_UNUSED TreeView *treeview, HeliaPlayer *player )
{
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( player->video ) ) );

	EncProp *ep = enc_prop_new ();
	g_signal_emit_by_name ( player->player, "player-enc", G_OBJECT ( window ), ep );
}

static void helia_player_treeview_repeat_handler ( G_GNUC_UNUSED TreeView *treeview, GtkButton *button, HeliaPlayer *player )
{
	player->repeat = !player->repeat;

	GtkImage *image = helia_create_image ( ( player->repeat ) ? "helia-set" : "helia-repeat", 16 );

	gtk_button_set_image ( button, GTK_WIDGET ( image ) );
}

static void helia_player_treeview_close_handler ( G_GNUC_UNUSED TreeView *treeview, HeliaPlayer *player )
{
	gtk_widget_set_visible ( GTK_WIDGET ( player->playlist ), FALSE );
}

static void helia_player_treeview_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, G_GNUC_UNUSED GtkTreeViewColumn *column, HeliaPlayer *player )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	if ( gtk_tree_model_get_iter ( model, &iter, path ) )
	{
		g_autofree char *data = NULL;
		gtk_tree_model_get ( model, &iter, COL_DATA, &data, -1 );

		player_set_stop ( player );
		g_signal_emit_by_name ( player->player, "player-play", player->xid, data );

		gtk_widget_queue_draw ( GTK_WIDGET ( player->video ) );
	}
}

static GtkBox * helia_player_create_treeview_scroll ( HeliaPlayer *player )
{
	GtkBox *v_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( v_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( v_box ), TRUE );

	GtkScrolledWindow *sw = (GtkScrolledWindow *)gtk_scrolled_window_new ( NULL, NULL );
	gtk_scrolled_window_set_policy ( sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_widget_set_size_request ( GTK_WIDGET ( sw ), 220, -1 );
	gtk_widget_set_visible ( GTK_WIDGET ( sw ), TRUE );

	const char *title[3] = { "Num", "Files", "Data" };

	player->treeview = tree_view_new ( title );
	gtk_widget_set_visible ( GTK_WIDGET ( player->treeview ), TRUE );
	g_signal_connect ( player->treeview, "row-activated", G_CALLBACK ( helia_player_treeview_row_activated ), player );

	gtk_container_add ( GTK_CONTAINER ( sw ), GTK_WIDGET ( player->treeview ) );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( sw ), TRUE, TRUE, 0 );

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 20 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	player->label_buf = (GtkLabel *)gtk_label_new ( " ⇄ 0% " );
	gtk_widget_set_halign ( GTK_WIDGET ( player->label_buf ), GTK_ALIGN_START );
	gtk_widget_set_visible ( GTK_WIDGET ( player->label_buf ), TRUE );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( player->label_buf ), FALSE, FALSE, 0 );

	player->label_rec = (GtkLabel *)gtk_label_new ( "" );
	gtk_widget_set_halign ( GTK_WIDGET ( player->label_rec ), GTK_ALIGN_START );
	gtk_widget_set_visible ( GTK_WIDGET ( player->label_rec ), TRUE );
	gtk_box_pack_end ( h_box, GTK_WIDGET ( player->label_rec ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( v_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	GtkBox *box_tool = create_treeview_box ( FALSE, player->treeview );
	gtk_widget_set_visible ( GTK_WIDGET ( box_tool ), TRUE );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( box_tool ), 5 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( box_tool ), 5 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( box_tool ), 5 );
	gtk_box_pack_end ( v_box, GTK_WIDGET ( box_tool ), FALSE, FALSE, 0 );

	g_signal_connect ( player->treeview, "treeview-enc",   G_CALLBACK ( helia_player_treeview_enc_handler    ), player );
	g_signal_connect ( player->treeview, "treeview-repeat-scan",  G_CALLBACK ( helia_player_treeview_repeat_handler ), player );
	g_signal_connect ( player->treeview, "treeview-close", G_CALLBACK ( helia_player_treeview_close_handler  ), player );

	return v_box;
}

static void player_set_stop ( HeliaPlayer *player )
{
	g_signal_emit_by_name ( player->player, "player-stop" );

	gtk_label_set_text ( player->label_buf, " ⇄ 0% " );
	gtk_label_set_text ( player->label_rec, "" );

	slider_update ( 0, 10, 0, 10, TRUE,  player->level );
	slider_update ( 0, 10, 0, 10, FALSE, player->level );

	g_signal_emit_by_name ( player, "power-set", FALSE );

	gtk_widget_queue_draw ( GTK_WIDGET ( player->video ) );
}

static void player_set_base ( HeliaPlayer *player )
{
	player_set_stop ( player );

	g_signal_emit_by_name ( player, "return-base" );
}

static void player_set_playlist ( HeliaPlayer *player )
{
	if ( gtk_widget_get_visible ( GTK_WIDGET ( player->playlist ) ) )
		gtk_widget_set_visible ( GTK_WIDGET ( player->playlist ), FALSE );
	else
		gtk_widget_set_visible ( GTK_WIDGET ( player->playlist ), TRUE );
}

static void player_set_eqa ( HeliaPlayer *player )
{
	uint opacity_p = OPACITY;

	GSettings *setting = settings_init ();
	if ( setting ) opacity_p = g_settings_get_uint ( setting, "opacity-panel" );

	double opacity = (float)opacity_p / 100;
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( player->video ) ) );

	g_signal_emit_by_name ( player->player, "player-eqa", G_OBJECT ( window ), opacity );

	if ( setting ) g_object_unref ( setting );
}

static void player_set_eqv ( HeliaPlayer *player )
{
	uint opacity_p = OPACITY;

	GSettings *setting = settings_init ();
	if ( setting ) opacity_p = g_settings_get_uint ( setting, "opacity-panel" );

	double opacity = (float)opacity_p / 100;
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( player->video ) ) );

	g_signal_emit_by_name ( player->player, "player-eqv", G_OBJECT ( window ), opacity );

	if ( setting ) g_object_unref ( setting );
}

static void player_set_mute ( HeliaPlayer *player )
{
	gboolean mute = FALSE;
	g_object_get ( player->player, "mute", &mute, NULL );
	g_object_set ( player->player, "mute", !mute, NULL );
}

static void player_set_pause ( HeliaPlayer *player )
{
	g_signal_emit_by_name ( player->player, "player-pause" );
}

static void player_set_rec ( HeliaPlayer *player )
{
	gtk_label_set_text ( player->label_buf, " ⇄ 0% " );
	gtk_label_set_text ( player->label_rec, "" );

	slider_update ( 0, 8, 0, 10, TRUE,  player->level );
	slider_update ( 0, 8, 0, 10, FALSE, player->level );

	G_GNUC_UNUSED gboolean status = FALSE;
	g_signal_emit_by_name ( player->player, "player-rec", &status );
}

static void player_set_info ( HeliaPlayer *player )
{
	gboolean play = FALSE;
	g_signal_emit_by_name ( player->player, "player-is-play", FALSE, &play );

	if ( !play ) return;

	GObject *object = NULL;
	g_signal_emit_by_name ( player->player, "player-element", &object );

	GstElement *playbin = GST_ELEMENT ( object );

	GtkTreeView *tree_view = GTK_TREE_VIEW ( player->treeview );

	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( player->video ) ) );

	helia_info_player ( window, tree_view, playbin );
}

static void helia_player_clicked_handler_cmp ( G_GNUC_UNUSED ControlMp *cmp, uint8_t num, HeliaPlayer *player )
{
	fp funcs[] = { player_set_base,  player_set_playlist, player_set_eqa, player_set_eqv,  player_set_mute,
		player_set_pause, player_set_stop,     player_set_rec, player_set_info, NULL };

	if ( funcs[num] ) funcs[num] ( player );
}

static void helia_player_handler_set_vol_ctv ( G_GNUC_UNUSED ControlMp *cmp, double value, HeliaPlayer *player )
{
	g_object_set ( player->player, "volume", value, NULL );
}

static gboolean helia_player_handler_is_play ( G_GNUC_UNUSED ControlMp *cmp, gboolean rec_status, HeliaPlayer *player )
{
	gboolean play = FALSE;
	g_signal_emit_by_name ( player->player, "player-is-play", rec_status, &play );

	return play;
}

static gboolean helia_player_video_fullscreen ( GtkWindow *window )
{
	GdkWindowState state = gdk_window_get_state ( gtk_widget_get_window ( GTK_WIDGET ( window ) ) );

	if ( state & GDK_WINDOW_STATE_FULLSCREEN )
		{ gtk_window_unfullscreen ( window ); return FALSE; }
	else
		{ gtk_window_fullscreen   ( window ); return TRUE;  }

	return TRUE;
}

static gboolean helia_player_video_press_event ( GtkDrawingArea *draw, GdkEventButton *event, HeliaPlayer *player )
{
	GtkWindow *window_base = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( draw ) ) );

	if ( event->button == 1 )
	{
		if ( event->type == GDK_2BUTTON_PRESS )
		{
			if ( helia_player_video_fullscreen ( GTK_WINDOW ( window_base ) ) )
			{
				gtk_widget_set_visible ( GTK_WIDGET ( player->playlist ), FALSE );
				gtk_widget_set_visible ( GTK_WIDGET ( player->level    ), FALSE );
			}
		}
	}

	if ( event->button == 2 )
	{
		if ( gtk_widget_get_visible ( GTK_WIDGET ( player->playlist ) ) )
		{
			gtk_widget_set_visible ( GTK_WIDGET ( player->playlist ), FALSE );
			gtk_widget_set_visible ( GTK_WIDGET ( player->level    ), FALSE );
		}
		else
		{
			gtk_widget_set_visible ( GTK_WIDGET ( player->playlist ), TRUE );
			gtk_widget_set_visible ( GTK_WIDGET ( player->level    ), TRUE );
		}
	}

	if ( event->button == 3 )
	{
		uint opacity_p = OPACITY;
		uint icon_size = ICON_SIZE;

		GSettings *setting = settings_init ();
		if ( setting ) opacity_p = g_settings_get_uint ( setting, "opacity-panel" );
		if ( setting ) icon_size = g_settings_get_uint ( setting, "icon-size" );

		gboolean play = FALSE;
		g_signal_emit_by_name ( player->player, "player-is-play", TRUE, &play );

		double volume = 0; // VOLUME;
		g_object_get ( player->player, "volume", &volume, NULL );

		ControlMp *cmp = control_mp_new ( (uint16_t)opacity_p, (uint16_t)icon_size, volume, play, window_base );
		g_signal_connect ( cmp, "player-click-num",   G_CALLBACK ( helia_player_clicked_handler_cmp ), player );
		g_signal_connect ( cmp, "player-set-volume",  G_CALLBACK ( helia_player_handler_set_vol_ctv ), player );
		g_signal_connect ( cmp, "cmp-player-is-play", G_CALLBACK ( helia_player_handler_is_play     ), player );

		if ( setting ) g_object_unref ( setting );
	}

	return TRUE;
}

static void helia_player_video_draw_black ( GtkDrawingArea *widget, cairo_t *cr, const char *name, uint16_t size )
{
	GdkRGBA color; color.red = 0; color.green = 0; color.blue = 0; color.alpha = 1.0;

	int width = gtk_widget_get_allocated_width  ( GTK_WIDGET ( widget ) );
	int heigh = gtk_widget_get_allocated_height ( GTK_WIDGET ( widget ) );

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

static gboolean helia_player_video_draw_check ( HeliaPlayer *player )
{
	gboolean ret = FALSE;
	g_signal_emit_by_name ( player->player, "player-video", &ret );

	return ret;
}

static gboolean helia_player_video_draw ( GtkDrawingArea *widget, cairo_t *cr, HeliaPlayer *player )
{
	if ( helia_player_video_draw_check ( player ) ) helia_player_video_draw_black ( widget, cr, "helia-mp", 96 );

	return FALSE;
}

static void helia_player_video_realize ( GtkDrawingArea *draw, HeliaPlayer *player )
{
	player->xid = GDK_WINDOW_XID ( gtk_widget_get_window ( GTK_WIDGET ( draw ) ) );
}

static GtkPaned * helia_player_create_paned ( GtkBox *playlist, GtkDrawingArea *video )
{
	GtkPaned *paned = (GtkPaned *)gtk_paned_new ( GTK_ORIENTATION_HORIZONTAL );
	gtk_paned_add1 ( paned, GTK_WIDGET ( playlist ) );
	gtk_paned_add2 ( paned, GTK_WIDGET ( video    ) );

	return paned;
}

static uint helia_player_get_model_iter_n ( HeliaPlayer *player )
{
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( player->treeview ) );

	int indx = gtk_tree_model_iter_n_children ( model, NULL );

	return (uint)indx;
}

static void helia_player_video_drag_in ( G_GNUC_UNUSED GtkDrawingArea *draw, GdkDragContext *ct, G_GNUC_UNUSED int x, G_GNUC_UNUSED int y,
	GtkSelectionData *s_data, G_GNUC_UNUSED uint info, guint32 time, HeliaPlayer *player )
{
	uint indx = helia_player_get_model_iter_n ( player );

	char **uris = gtk_selection_data_get_uris ( s_data );

	uint c = 0; for ( c = 0; uris[c] != NULL; c++ )
	{
		char *path = helia_uri_get_path ( uris[c] );

		g_signal_emit_by_name ( player->treeview, "treeview-add-files", path );

		free ( path );
	}

	g_strfreev ( uris );
	gtk_drag_finish ( ct, TRUE, FALSE, time );

	player_first_play ( indx, player );
}

static void helia_player_show_cursor ( GtkDrawingArea *draw, gboolean show_cursor )
{
	GdkWindow *window = gtk_widget_get_window ( GTK_WIDGET ( draw ) );

	GdkCursor *cursor = gdk_cursor_new_for_display ( gdk_display_get_default (), ( show_cursor ) ? GDK_ARROW : GDK_BLANK_CURSOR );

	gdk_window_set_cursor ( window, cursor );

	g_object_unref (cursor);
}

static gboolean helia_player_video_notify_event ( GtkDrawingArea *draw, G_GNUC_UNUSED GdkEventMotion *event, HeliaPlayer *player )
{
	time ( &player->t_hide );

	helia_player_show_cursor ( draw, TRUE );

	return GDK_EVENT_STOP;
}

static void helia_player_video_hide_cursor ( HeliaPlayer *player )
{
	time_t t_cur;
	time ( &t_cur );

	if ( ( t_cur - player->t_hide < 2 ) ) return;

	gboolean show = FALSE;
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( player->video ) ) );

	if ( !gtk_window_is_active ( window ) ) helia_player_show_cursor ( player->video, TRUE ); else helia_player_show_cursor ( player->video, show );
}

static void helia_player_handler_staus ( G_GNUC_UNUSED Player *p, const char *buf, const char *rec, HeliaPlayer *player )
{
	if ( buf ) gtk_label_set_text   ( player->label_buf, buf );
	if ( rec ) gtk_label_set_markup ( player->label_rec, rec );
}

static void helia_player_handler_slider ( G_GNUC_UNUSED Player *p, gint64 pos, uint d_pos, gint64 dur, uint d_dur, gboolean sensitive, HeliaPlayer *player )
{
	helia_player_video_hide_cursor ( player );

	slider_update ( pos, d_pos, dur, d_dur, sensitive, player->level );
}

static void helia_player_handler_slider_seek ( G_GNUC_UNUSED Slider *l, double val, HeliaPlayer *player )
{
	g_signal_emit_by_name ( player->player, "player-seek", val );
}

static gboolean helia_player_video_scroll_event ( G_GNUC_UNUSED GtkDrawingArea *widget, GdkEventScroll *evscroll, HeliaPlayer *player )
{
	gboolean up_dwn = TRUE;
	if ( evscroll->direction == GDK_SCROLL_DOWN ) up_dwn = FALSE;
	if ( evscroll->direction == GDK_SCROLL_UP   ) up_dwn = TRUE;

	if ( evscroll->direction == GDK_SCROLL_DOWN || evscroll->direction == GDK_SCROLL_UP )
		g_signal_emit_by_name ( player->player, "player-scrool", up_dwn );

	return TRUE;
}

static void helia_player_handler_message_dialog ( G_GNUC_UNUSED Player *p, const char *message, HeliaPlayer *player )
{
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( player->video ) ) );

	helia_message_dialog ( "", message, GTK_MESSAGE_ERROR, window );
}

static void helia_player_handler_power ( G_GNUC_UNUSED Player *p, gboolean power, HeliaPlayer *player )
{
	g_signal_emit_by_name ( player, "power-set", power );
}

static void helia_player_open_dir ( HeliaPlayer *player )
{
        GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( player->video ) ) );

	g_autofree char *path = helia_open_dir ( g_get_home_dir (), window );

	if ( path == NULL ) return;

	uint indx = helia_player_get_model_iter_n ( player );

	g_signal_emit_by_name ( player->treeview, "treeview-add-files", path );

	player_first_play ( indx, player );
}

static void helia_player_open_files ( HeliaPlayer *player )
{
        GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( player->video ) ) );

	GSList *files = helia_open_files ( g_get_home_dir (), window );

	if ( files == NULL ) return;

	uint indx = helia_player_get_model_iter_n ( player );

	while ( files != NULL )
	{
                char *path = (char *)files->data;
		g_signal_emit_by_name ( player->treeview, "treeview-add-files", path );

                files = files->next;
	}

	g_slist_free_full ( files, (GDestroyNotify) g_free );

	player_first_play ( indx, player );
}

static void helia_player_set_slider ( HeliaPlayer *player )
{
	gboolean vis = gtk_widget_get_visible ( GTK_WIDGET ( player->level ) );
	gtk_widget_set_visible ( GTK_WIDGET ( player->level ), !vis );
}

void player_action_accels ( enum player_accel num, HeliaPlayer *player )
{
	if ( num == OPEN_DIR   ) helia_player_open_dir   ( player );
	if ( num == OPEN_FILES ) helia_player_open_files ( player );

	if ( num == SLIDER ) helia_player_set_slider ( player );

	if ( num == PAUSE  ) g_signal_emit_by_name ( player->player, "player-pause" );
	if ( num == FRAME  ) g_signal_emit_by_name ( player->player, "player-frame" );
}

static void helia_player_init ( HeliaPlayer *player )
{
	player->repeat = FALSE;

	GtkBox *box = GTK_BOX ( player );
	gtk_orientable_set_orientation ( GTK_ORIENTABLE ( box ), GTK_ORIENTATION_VERTICAL );
	gtk_box_set_spacing ( box, 3 );

	player->playlist = helia_player_create_treeview_scroll ( player );

	player->video = (GtkDrawingArea *)gtk_drawing_area_new ();
	gtk_widget_set_visible ( GTK_WIDGET ( player->video ), TRUE );
	gtk_widget_set_events ( GTK_WIDGET ( player->video ), GDK_BUTTON_PRESS_MASK | GDK_SCROLL_MASK | GDK_POINTER_MOTION_MASK );

	g_signal_connect ( player->video, "draw", G_CALLBACK ( helia_player_video_draw ), player );
	g_signal_connect ( player->video, "realize", G_CALLBACK ( helia_player_video_realize ), player );

	g_signal_connect ( player->video, "scroll-event",        G_CALLBACK ( helia_player_video_scroll_event ), player );
	g_signal_connect ( player->video, "button-press-event",  G_CALLBACK ( helia_player_video_press_event  ), player );
	g_signal_connect ( player->video, "motion-notify-event", G_CALLBACK ( helia_player_video_notify_event ), player );

	GtkPaned *paned = helia_player_create_paned ( player->playlist, player->video );
	gtk_widget_set_visible ( GTK_WIDGET ( paned ), TRUE );

	gtk_box_pack_start ( box, GTK_WIDGET ( paned ), TRUE, TRUE, 0 );

	player->level = slider_new ();
	g_signal_connect ( player->level, "slider-seek", G_CALLBACK ( helia_player_handler_slider_seek ), player );
	gtk_widget_set_visible ( GTK_WIDGET ( player->level ), TRUE );
	gtk_box_pack_end ( box, GTK_WIDGET ( player->level ), FALSE, FALSE, 0 );

	player->player = player_new ();
	g_signal_connect ( player->player, "player-staus",  G_CALLBACK ( helia_player_handler_staus  ), player );
	g_signal_connect ( player->player, "player-slider", G_CALLBACK ( helia_player_handler_slider ), player );
	g_signal_connect ( player->player, "player-next",   G_CALLBACK ( helia_player_handler_next   ), player );
	g_signal_connect ( player->player, "player-power-set", G_CALLBACK ( helia_player_handler_power ), player );
	g_signal_connect ( player->player, "player-message-dialog", G_CALLBACK ( helia_player_handler_message_dialog ), player );

	gtk_drag_dest_set ( GTK_WIDGET ( player->video ), GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY );
	gtk_drag_dest_add_uri_targets  ( GTK_WIDGET ( player->video ) );
	g_signal_connect  ( player->video, "drag-data-received", G_CALLBACK ( helia_player_video_drag_in ), player );
}

static void helia_player_destroy ( GtkWidget *object )
{
	HeliaPlayer *player = HELIA_PLAYER ( object );

	g_signal_emit_by_name ( player->player, "player-stop" );

	GTK_WIDGET_CLASS (helia_player_parent_class)->destroy ( object );
}

static void helia_player_finalize ( GObject *object )
{
	HeliaPlayer *player = HELIA_PLAYER ( object );

	g_object_unref ( player->player );

	G_OBJECT_CLASS (helia_player_parent_class)->finalize (object);
}

static void helia_player_class_init ( HeliaPlayerClass *class )
{
	GObjectClass *oclass   = G_OBJECT_CLASS ( class );
	GtkWidgetClass *wclass = GTK_WIDGET_CLASS ( class );

	oclass->finalize = helia_player_finalize;

	wclass->destroy = helia_player_destroy;

	g_signal_new ( "return-base", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "power-set", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_BOOLEAN );
}

void helia_player_start ( GFile **files, int n_files, HeliaPlayer *player )
{
	int c = 0; for ( c = 0; c < n_files; c++ )
	{
		char *path = g_file_get_path ( files[c] );

		g_signal_emit_by_name ( player->treeview, "treeview-add-files", path );

		free ( path );
	}

	player_first_play ( 0, player );
}

HeliaPlayer * helia_player_new ( void )
{
	return g_object_new ( HELIA_TYPE_PLAYER, NULL );
}
