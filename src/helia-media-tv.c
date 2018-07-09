/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia-media-tv.h"
#include "helia-panel-tv.h"
#include "helia-scrollwin.h"
#include "helia-panel-treeview.h"
#include "helia-treeview-tv.h"

#include "helia-gst-tv.h"
#include "helia-convert.h"

#include "helia-base.h"
#include "helia-service.h"
#include "helia-pref.h"

#include <gdk/gdkx.h>
#include <glib/gi18n.h>


typedef struct _HeliaMediaTv HeliaMediaTv;

struct _HeliaMediaTv
{
	gulong win_handle_tv;
	gboolean rec_trig_vis;

	GtkBox *sw_vbox;
	GtkTreeView *treeview_tv;

	GtkLabel *signal_snr;
	GtkProgressBar *bar_sgn, *bar_snr;
};

static HeliaMediaTv hmtv;



void helia_media_tv_stop ()
{
	helia_gst_tv_stop ();
	
	helia_treeview_to_file ( hmtv.treeview_tv, (gchar *)hbgv.ch_conf, TRUE );

	g_debug ( "helia_media_tv_stop" );
}

void helia_media_tv_scan_save ( gchar *name, gchar *data )
{
	helia_treeview_tv_add ( hmtv.treeview_tv, name, data );
	
	g_debug ( "helia_media_tv_scan_save" );
}


// ***** Level: Signal & Snr *****

void helia_media_tv_set_sgn_snr ( GstElement *element, gdouble sgl, gdouble snr, gboolean hlook, gboolean rec_ses )
{
	GtkLabel *label = hmtv.signal_snr;
	GtkProgressBar *barsgn = hmtv.bar_sgn;
	GtkProgressBar *barsnr = hmtv.bar_snr;

	gtk_progress_bar_set_fraction ( barsgn, sgl/100 );
	gtk_progress_bar_set_fraction ( barsnr, snr/100 );

	gchar *texta = g_strdup_printf ( "Level %d%s", (int)sgl, "%" );
	gchar *textb = g_strdup_printf ( "Snr %d%s",   (int)snr, "%" );

	const gchar *format = NULL, *rec = " ● ";

	if ( hmtv.rec_trig_vis ) rec = " ◌ ";

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

	gchar *markup = g_markup_printf_escaped ( format, texta, textb, rec_ses ? rec : "" );
		gtk_label_set_markup ( label, markup );
	g_free ( markup );

	g_free ( texta );
	g_free ( textb );

	hmtv.rec_trig_vis = !hmtv.rec_trig_vis;
	
	//g_debug ( "helia_media_tv_set_sgn_snr" );
}

static GtkBox * helia_media_tv_create_sgn_snr ()
{
	g_debug ( "helia_media_tv_create_sgn_snr" );
	
	GtkBox *vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( vbox ), 5 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( vbox ), 5 );

	hmtv.signal_snr = (GtkLabel *)gtk_label_new ( "Level  &  Quality" );
	gtk_box_pack_start ( vbox, GTK_WIDGET ( hmtv.signal_snr  ), FALSE, FALSE, 5 );

	hmtv.bar_sgn = (GtkProgressBar *)gtk_progress_bar_new ();
	hmtv.bar_snr = (GtkProgressBar *)gtk_progress_bar_new ();

	gtk_box_pack_start ( vbox, GTK_WIDGET ( hmtv.bar_sgn  ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( vbox, GTK_WIDGET ( hmtv.bar_snr  ), FALSE, FALSE, 3 );

	return vbox;
}

// ***** Level: Signal & Snr *****


void helia_media_tv_edit_hs () 
{
	gtk_widget_get_visible ( GTK_WIDGET ( hmtv.sw_vbox ) ) 
		? gtk_widget_hide ( GTK_WIDGET ( hmtv.sw_vbox ) )
		: gtk_widget_show ( GTK_WIDGET ( hmtv.sw_vbox ) );
}


// ***** GdkEventButton *****

static gboolean helia_media_tv_press_event ( GtkDrawingArea *widget, GdkEventButton *event, GtkBox *vbox )
{
	if ( hbgv.all_info )
		g_debug ( "helia_media_tv_press_event:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( widget ) ) );

	if ( event->button == 1 && event->type == GDK_2BUTTON_PRESS )
	{
		if ( helia_base_flscr () ) 
			gtk_widget_hide ( GTK_WIDGET ( vbox ) );
		else
			gtk_widget_show ( GTK_WIDGET ( vbox ) );

		return TRUE;
	}

	if ( event->button == 3 )
	{
		helia_panel_tv ( vbox, helia_base_win_ret (), hbgv.opacity_cnt, hbgv.resize_icon );		

		return TRUE;
	}

	if ( event->button == 2 )
	{
		gtk_widget_get_visible ( GTK_WIDGET ( vbox ) ) ? gtk_widget_hide ( GTK_WIDGET ( vbox ) ) : gtk_widget_show ( GTK_WIDGET ( vbox ) );

		return TRUE;
	}

	return FALSE;
}

static gboolean helia_media_tv_press_event_trw ( GtkTreeView *tree_view, GdkEventButton *event )
{
	if ( hbgv.all_info )
		g_debug ( "helia_media_tv_press_event_trw" );

	if ( event->button == 3 )
	{
		helia_panel_treeview ( tree_view, helia_base_win_ret (), hbgv.opacity_cnt, TRUE, hbgv.resize_icon );

		return TRUE;
	}

	return FALSE;
}

static void helia_media_tv_tree_view_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column )
{
	if ( hbgv.all_info )
		g_debug ( "helia_media_tv_tree_view_row_activated:: column name %s", gtk_tree_view_column_get_title ( column ) );

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	if ( gtk_tree_model_get_iter ( model, &iter, path ) )
	{
		gchar *file_ch = NULL;

			gtk_tree_model_get ( model, &iter, COL_DATA, &file_ch, -1 );

			helia_gst_tv_play ( file_ch, hmtv.win_handle_tv );
			
			g_debug ( "helia_media_tv_tree_view_row_activated:: channel %s", file_ch );

		g_free ( file_ch );
	}
}

// ***** GdkEventButton *****



// ***** GtkDrawingArea *****

static void helia_media_tv_draw_black ( GtkDrawingArea *widget, cairo_t *cr, GdkPixbuf *logo )
{
	GdkRGBA color; color.red = 0; color.green = 0; color.blue = 0; color.alpha = 1.0;

	gint width  = gtk_widget_get_allocated_width  ( GTK_WIDGET ( widget ) );
	gint height = gtk_widget_get_allocated_height ( GTK_WIDGET ( widget ) );

	cairo_rectangle ( cr, 0, 0, width, height );
	gdk_cairo_set_source_rgba ( cr, &color );
	cairo_fill (cr);

	if ( logo != NULL )
	{
		gint widthl  = gdk_pixbuf_get_width  ( logo );
		gint heightl = gdk_pixbuf_get_height ( logo );

		cairo_rectangle ( cr, 0, 0, width, height );
				
		if ( widthl > width || heightl > height )
		{
			guint out_size = width - 10;
			
			if ( width > height ) out_size = height - 10;
			
			gdk_cairo_set_source_pixbuf ( cr, gdk_pixbuf_scale_simple ( logo, out_size, out_size, GDK_INTERP_TILES ),
				( width / 2  ) - ( out_size  / 2 ), ( height / 2 ) - ( out_size / 2 ) );
		}
		else
			gdk_cairo_set_source_pixbuf ( cr, logo,
				( width / 2  ) - ( widthl  / 2 ), ( height / 2 ) - ( heightl / 2 ) );

		cairo_fill (cr);
	}
	
	if ( hbgv.all_info )
		g_debug ( "helia_media_tv_draw_black" );
}

static gboolean helia_media_tv_draw_callback ( GtkDrawingArea *widget, cairo_t *cr, GdkPixbuf *logo )
{
	if ( hbgv.all_info )
		g_debug ( "helia_media_tv_draw_callback" );
	
	if ( helia_gst_tv_draw_logo () ) { helia_media_tv_draw_black ( widget, cr, logo ); return TRUE; }

	return FALSE;
}


static void helia_media_tv_video_window_realize ( GtkDrawingArea *draw )
{
	hmtv.win_handle_tv = GDK_WINDOW_XID ( gtk_widget_get_window ( GTK_WIDGET ( draw ) ) );

	g_print ( "Tv xid: %ld \n", hmtv.win_handle_tv );
}

// ***** GtkDrawingArea *****



// ***** Drag And Drop *****

static void helia_media_tv_drag_in ( GtkDrawingArea *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint info, guint32 time, GtkTreeView *tree_view )
{
	gchar **uris = gtk_selection_data_get_uris ( selection_data );

	guint i = 0;
	for ( i = 0; uris[i] != NULL; i++ )
	{
		GFile *file = g_file_new_for_uri ( uris[i] );
		gchar *file_name = g_file_get_path ( file );

			if ( g_str_has_suffix ( file_name, "gtv-channel.conf" ) )
				helia_treeview_tv ( file_name, tree_view );

			if ( g_str_has_suffix ( file_name, "dvb_channel.conf" ) )
				helia_convert_win ( file_name );

			g_debug ( "helia_media_tv_drag_in:: file %s", file_name );

		g_free ( file_name );
		g_object_unref ( file );
	}

	g_strfreev ( uris );

	gtk_drag_finish ( context, TRUE, FALSE, time );

	if ( hbgv.all_info )
		g_debug ( "helia_media_tv_drag_in:: widget name %s | x %d | y %d | i %d", gtk_widget_get_name ( GTK_WIDGET ( widget ) ), x, y, info );
}

// ***** Drag And Drop *****



static void helia_media_tv_init ()
{
	hmtv.win_handle_tv = 0;
	hmtv.rec_trig_vis = TRUE;

	helia_gst_tv_gst_create ();

	g_debug ( "helia_media_tv_init" );
}

void helia_media_tv_win ( GtkBox *box, GdkPixbuf *logo, HBTrwCol sw_col_n[], guint num )
{
	helia_media_tv_init ();

	hmtv.sw_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	GtkDrawingArea *video_window = (GtkDrawingArea *)gtk_drawing_area_new ();
	g_signal_connect ( video_window, "realize", G_CALLBACK ( helia_media_tv_video_window_realize ), NULL );
	g_signal_connect ( video_window, "draw",    G_CALLBACK ( helia_media_tv_draw_callback ), logo );

	gtk_widget_add_events ( GTK_WIDGET ( video_window ), GDK_BUTTON_PRESS_MASK  );
	g_signal_connect ( video_window, "button-press-event", G_CALLBACK ( helia_media_tv_press_event  ), hmtv.sw_vbox );


	GtkListStore *liststore   = (GtkListStore *)gtk_list_store_new ( 3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING );
	hmtv.treeview_tv = (GtkTreeView *)gtk_tree_view_new_with_model ( GTK_TREE_MODEL ( liststore ) );
	g_signal_connect ( hmtv.treeview_tv, "button-press-event", G_CALLBACK ( helia_media_tv_press_event_trw  ), NULL );
	g_signal_connect ( hmtv.treeview_tv, "row-activated",      G_CALLBACK ( helia_media_tv_tree_view_row_activated ), NULL );

	GtkScrolledWindow *scroll_win = helia_scrollwin ( hmtv.treeview_tv, sw_col_n, num, 200 );
	gtk_box_pack_start ( hmtv.sw_vbox, GTK_WIDGET ( scroll_win ), TRUE,  TRUE,  0 );


	GtkBox *hb_box   = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	GtkButton *button = helia_service_ret_image_button ( "helia-exit", 16 );
	g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_hide ), GTK_WIDGET ( hmtv.sw_vbox ) );

	gtk_box_pack_start ( hb_box,  GTK_WIDGET ( button ), TRUE,  TRUE,  5 );
	gtk_box_pack_end   ( hmtv.sw_vbox, GTK_WIDGET ( hb_box ), FALSE, FALSE, 5 );


	GtkPaned *hpaned = (GtkPaned *)gtk_paned_new ( GTK_ORIENTATION_HORIZONTAL );
	gtk_paned_add1 ( hpaned, GTK_WIDGET ( hmtv.sw_vbox )      );
	gtk_paned_add2 ( hpaned, GTK_WIDGET ( video_window ) );


	GtkBox *main_hbox   = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_box_pack_start ( main_hbox, GTK_WIDGET ( hpaned ),    TRUE,  TRUE,  0 );
	gtk_box_pack_start ( box,       GTK_WIDGET ( main_hbox ), TRUE,  TRUE,  0 );

	gtk_box_pack_start ( hmtv.sw_vbox, GTK_WIDGET ( helia_media_tv_create_sgn_snr () ), FALSE, FALSE, 0 );


	gtk_drag_dest_set ( GTK_WIDGET ( video_window ), GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY );
	gtk_drag_dest_add_uri_targets  ( GTK_WIDGET ( video_window ) );
	g_signal_connect  ( video_window, "drag-data-received", G_CALLBACK ( helia_media_tv_drag_in ), hmtv.treeview_tv );
	
	
	if ( g_file_test ( hbgv.ch_conf, G_FILE_TEST_EXISTS ) )
		helia_treeview_tv ( hbgv.ch_conf, hmtv.treeview_tv );
	
	g_debug ( "helia_media_tv_win" );
}
