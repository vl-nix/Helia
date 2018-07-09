/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia-media-pl.h"
#include "helia-panel-pl.h"
#include "helia-scrollwin.h"
#include "helia-panel-treeview.h"
#include "helia-treeview-pl.h"

#include "helia-gst-pl.h"

#include "helia-base.h"
#include "helia-service.h"
#include "helia-pref.h"

#include <gdk/gdkx.h>
#include <glib/gi18n.h>


typedef struct _HeliaMediaPl HeliaMediaPl;

struct _HeliaMediaPl
{
	gulong win_handle_pl;

	GtkBox *sw_vbox, *hbox_slider;
	GtkTreeView *treeview_pl;
	GtkTreePath *index_path;
	
	GtkLabel *lab_buf;
	GtkSpinner *spinner;
	
	gboolean next_repeat;

	HMPlSlider sl_base;
};

static HeliaMediaPl hmpl;



void helia_media_pl_stop ()
{
	helia_gst_pl_playback_stop ();

	g_debug ( "helia_media_pl_stop" );
}

void helia_media_pl_set_sensitive ( gboolean sensitive )
{
	gtk_widget_set_sensitive ( GTK_WIDGET ( hmpl.sl_base.slider ), sensitive );
	
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_set_sensitive" );
}


// ***** GtkSpinner *****

static GtkBox * helia_media_pl_create_spinner ()
{
	g_debug ( "helia_media_pl_create_spinner" );
	
	GtkBox *hbox   = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	hmpl.spinner = (GtkSpinner *)gtk_spinner_new ();
	hmpl.lab_buf = (GtkLabel *)gtk_label_new ("");
	gtk_widget_set_halign ( GTK_WIDGET ( hmpl.lab_buf ), GTK_ALIGN_START );

	gtk_box_pack_start ( hbox, GTK_WIDGET ( hmpl.spinner ), FALSE, FALSE, 10 );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( hmpl.lab_buf ), FALSE, FALSE, 10 );

	return hbox;
}

void helia_media_pl_set_spinner ( gchar *text, gboolean spinner_stop )
{
	if ( spinner_stop )
		gtk_spinner_stop ( hmpl.spinner );
	else
		gtk_spinner_start ( hmpl.spinner );

	gtk_label_set_text ( hmpl.lab_buf, text );
	
	if ( hbgv.all_info ) 
		g_debug ( "helia_media_pl_set_spinner" );
}

// ***** GtkSpinner *****



// ***** Player Repeat Next *****

void helia_media_pl_set_path ( GtkTreeModel *model, GtkTreeIter iter )
{
	hmpl.index_path = gtk_tree_model_get_path ( model, &iter );
	
	g_debug ( "helia_media_pl_set_path" );
}

void helia_media_pl_next ()
{
	g_debug ( "helia_media_pl_next" );

	if ( hmpl.next_repeat )
	{
		helia_gst_pl_playback_stop ();
		helia_gst_pl_playback_play ();
		
		return;
	}

	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( hmpl.treeview_pl ) );
	guint ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( !hmpl.index_path || ind < 2 )
	{
		helia_gst_pl_playback_stop ();

		return;
	}

	GtkTreeIter iter;

	if ( gtk_tree_model_get_iter ( model, &iter, hmpl.index_path ) )
	{
		if ( gtk_tree_model_iter_next ( model, &iter ) )
		{
			gchar *file_ch = NULL;

			gtk_tree_model_get ( model, &iter, COL_DATA, &file_ch, -1 );

			hmpl.index_path = gtk_tree_model_get_path ( model, &iter );
			gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( hmpl.treeview_pl ), &iter );
			
			helia_gst_pl_stop_set_play ( file_ch, hmpl.win_handle_pl );

			g_free ( file_ch );
		}
		else
			helia_gst_pl_playback_stop ();
	}
	else
		helia_gst_pl_playback_stop ();
}

static void helia_media_pl_set_repeat ( GtkButton *button )
{
    //gtk_widget_set_opacity ( GTK_WIDGET ( button ), hmpl.next_repeat ? 1.0 : 0.5 );
    
    hmpl.next_repeat = !hmpl.next_repeat;
    
	GdkPixbuf *logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					  hmpl.next_repeat ? "helia-set" : "helia-repeat", 16, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	GtkImage *image = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );

	gtk_button_set_image ( button, GTK_WIDGET ( image ) );
}

// ***** Player Repeat Next *****



// ***** Slider *****

static void helia_media_pl_label_set_text ( GtkLabel *label, gint64 pos_dur, guint digits )
{
	gchar *str   = g_strdup_printf ( "%" GST_TIME_FORMAT, GST_TIME_ARGS ( pos_dur ) );
	gchar *str_l = g_strndup ( str, strlen ( str ) - digits );

	gtk_label_set_text ( label, str_l );

	g_free ( str_l  );
	g_free ( str );
	
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_label_set_text" );
}
void helia_media_pl_pos_dur_text ( gint64 pos, gint64 dur, guint digits )
{
	helia_media_pl_label_set_text ( hmpl.sl_base.lab_pos, pos, digits );
	helia_media_pl_label_set_text ( hmpl.sl_base.lab_dur, dur, digits );
	
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_pos_dur_text" );
}
static void helia_media_pl_pos_text ( gint64 pos, guint digits )
{
	helia_media_pl_label_set_text ( hmpl.sl_base.lab_pos, pos, digits );
	
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_pos_text" );
}
static void helia_media_pl_slider_update ( GtkRange *range, gdouble data )
{
	g_signal_handler_block   ( range, hmpl.sl_base.slider_signal_id );
		gtk_range_set_value  ( range, data );
	g_signal_handler_unblock ( range, hmpl.sl_base.slider_signal_id );
	
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_slider_update" );
}
void helia_media_pl_pos_dur_sliders ( gint64 current, gint64 duration, guint digits )
{
	gdouble slider_max = (gdouble)duration / GST_SECOND;

		gtk_range_set_range ( GTK_RANGE ( hmpl.sl_base.slider ), 0, slider_max );

		helia_media_pl_slider_update ( GTK_RANGE ( hmpl.sl_base.slider ), (gdouble)current / GST_SECOND );

	helia_media_pl_pos_dur_text ( current, duration, digits );
	
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_pos_dur_sliders" );
}
static void helia_media_pl_slider_changed ( GtkRange *range )
{
	gdouble value = gtk_range_get_value ( GTK_RANGE (range) );

		helia_gst_pl_slider_changed ( value );

	helia_media_pl_pos_text ( (gint64)( value * GST_SECOND ), 8 );
	
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_slider_changed" );
}

HMPlSlider helia_media_pl_create_slider ( void (* activate)() )
{
	g_debug ( "helia_media_pl_create_slider" );
	
	HMPlSlider slider_base;
	
	slider_base.lab_pos = (GtkLabel *)gtk_label_new ( "0:00:00" );
	slider_base.lab_dur = (GtkLabel *)gtk_label_new ( "0:00:00" );
	slider_base.slider  = (GtkScale *)gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0, 100, 1 );
	slider_base.slider_signal_id = g_signal_connect ( slider_base.slider, "value-changed", G_CALLBACK ( activate ), NULL );

	gtk_scale_set_draw_value ( slider_base.slider, 0 );
	gtk_range_set_value ( GTK_RANGE ( slider_base.slider ), 0 );

	return slider_base;
}

static GtkBox * helia_media_pl_slider ()
{
	g_debug ( "helia_media_pl_slider" );
	
	hmpl.sl_base = helia_media_pl_create_slider ( helia_media_pl_slider_changed );

	GtkBox *hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( hbox ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( hbox ), 10 );
	gtk_box_set_spacing ( hbox, 5 );

	gtk_box_pack_start ( hbox, GTK_WIDGET ( hmpl.sl_base.lab_pos ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( hmpl.sl_base.slider  ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( hmpl.sl_base.lab_dur ), FALSE, FALSE, 0 );

	gtk_widget_set_size_request ( GTK_WIDGET ( hmpl.sl_base.slider ), 300, -1 );

	return hbox;
}

// ***** Slider *****


void helia_media_pl_edit_hs () 
{
	gtk_widget_get_visible ( GTK_WIDGET ( hmpl.sw_vbox ) ) 
		? gtk_widget_hide ( GTK_WIDGET ( hmpl.sw_vbox ) )
		: gtk_widget_show ( GTK_WIDGET ( hmpl.sw_vbox ) );
}

void helia_media_pl_slider_hs () 
{ 
	gtk_widget_get_visible ( GTK_WIDGET ( hmpl.hbox_slider ) ) 
		? gtk_widget_hide ( GTK_WIDGET ( hmpl.hbox_slider ) ) 
		: gtk_widget_show ( GTK_WIDGET ( hmpl.hbox_slider ) ); 
}


// ***** GdkEventButton *****

static gboolean helia_media_pl_press_event ( GtkDrawingArea *widget, GdkEventButton *event, GtkBox *vbox )
{
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_press_event:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( widget ) ) );

	if ( event->button == 1 && event->type == GDK_2BUTTON_PRESS )
	{
		if ( helia_base_flscr () ) 
			{ gtk_widget_hide ( GTK_WIDGET ( vbox ) ); gtk_widget_hide ( GTK_WIDGET ( hmpl.hbox_slider ) ); }
		// else
			// { gtk_widget_show ( GTK_WIDGET ( vbox ) ); gtk_widget_show ( GTK_WIDGET ( hmpl.hbox_slider ) ); }

		return TRUE;
	}

	if ( event->button == 3 )
	{
		helia_panel_pl ( vbox, hmpl.hbox_slider, hmpl.sl_base.slider, helia_base_win_ret (), hbgv.opacity_cnt, hbgv.resize_icon );

		return TRUE;
	}

	if ( event->button == 2 )
	{
		helia_gst_pl_get_current_state_playing () ? helia_gst_pl_playback_pause () : helia_gst_pl_playback_play ();

		return TRUE;
	}

	return FALSE;
}

static gboolean helia_media_pl_press_event_trw ( GtkTreeView *tree_view, GdkEventButton *event )
{
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_press_event_trw" );

	if ( event->button == 3 )
	{
		helia_panel_treeview ( tree_view, helia_base_win_ret (), hbgv.opacity_cnt, FALSE, hbgv.resize_icon );

		return TRUE;
	}

	return FALSE;
}

static gboolean helia_media_pl_scroll_event ( GtkDrawingArea *widget, GdkEventScroll *evscroll )
{
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_scroll_event:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( widget ) ) );

	if ( helia_gst_pl_get_current_state_stop () ) return TRUE;

	gdouble new_pos = 0, skip = 20, pos = gtk_range_get_value ( GTK_RANGE ( hmpl.sl_base.slider ) ), dur = 0;

	GtkAdjustment *adjustment = gtk_range_get_adjustment ( GTK_RANGE ( hmpl.sl_base.slider ) );
	g_object_get ( adjustment, "upper", &dur, NULL );

	if ( (gint)dur < 1 ) return TRUE;

	if ( evscroll->direction == GDK_SCROLL_DOWN ) new_pos = ( pos > skip ) ? ( pos - skip ) : 0;

	if ( evscroll->direction == GDK_SCROLL_UP   ) new_pos = ( dur > ( pos + skip ) ) ? ( pos + skip ) : dur;

	if ( evscroll->direction == GDK_SCROLL_DOWN || evscroll->direction == GDK_SCROLL_UP )
		gtk_range_set_value  ( GTK_RANGE ( hmpl.sl_base.slider ), new_pos );

	return TRUE;
}

static void helia_media_pl_tree_view_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column )
{
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_tree_view_row_activated:: column name %s", gtk_tree_view_column_get_title ( column ) );

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	if ( gtk_tree_model_get_iter ( model, &iter, path ) )
	{
		gchar *file_ch = NULL;
		gtk_tree_model_get ( model, &iter, COL_DATA, &file_ch, -1 );

			helia_gst_pl_stop_set_play ( file_ch, hmpl.win_handle_pl );
			
			hmpl.index_path = gtk_tree_model_get_path ( model, &iter );

			g_debug ( "File: %s", file_ch );

		g_free ( file_ch );
	}
}

// ***** GdkEventButton *****



// ***** GtkDrawingArea *****

static void helia_media_pl_draw_black ( GtkDrawingArea *widget, cairo_t *cr, GdkPixbuf *logo )
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
		g_debug ( "helia_media_pl_draw_black" );
}

static gboolean helia_media_pl_draw_callback ( GtkDrawingArea *widget, cairo_t *cr, GdkPixbuf *logo )
{
	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_draw_callback" );

	if ( helia_gst_pl_draw_logo () ) { helia_media_pl_draw_black ( widget, cr, logo ); return TRUE; }

	return FALSE;
}


static void helia_media_pl_video_window_realize ( GtkDrawingArea *draw )
{
	hmpl.win_handle_pl = GDK_WINDOW_XID ( gtk_widget_get_window ( GTK_WIDGET ( draw ) ) );

	g_print ( "Pl xid: %ld \n", hmpl.win_handle_pl );
}
gulong helia_media_pl_handle_ret ()
{
	return hmpl.win_handle_pl;
}

// ***** GtkDrawingArea *****



// ***** Drag And Drop & Arg *****

static void helia_media_pl_drag_in ( GtkDrawingArea *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint info, guint32 time, GtkTreeView *tree_view )
{
	gchar **uris = gtk_selection_data_get_uris ( selection_data );

	guint c = 0;
	for ( c = 0; uris[c] != NULL; c++ ) ;;

	helia_treeview_pl_files ( NULL, uris, c, tree_view );

	g_strfreev ( uris );

	gtk_drag_finish ( context, TRUE, FALSE, time );

	if ( hbgv.all_info )
		g_debug ( "helia_media_pl_drag_in:: widget name %s | x %d | y %d | i %d", gtk_widget_get_name ( GTK_WIDGET ( widget ) ), x, y, info );
}


void helia_media_pl_add_arg ( GFile **files, gint n_files )
{
	helia_treeview_pl_files ( files, NULL, n_files, hmpl.treeview_pl );
	
	g_debug ( "helia_media_pl_add_arg" );
}

// ***** Drag And Drop & Arg *****



static void helia_media_pl_init ()
{
	hmpl.win_handle_pl = 0;
	hmpl.next_repeat = FALSE;

	helia_gst_pl_gst_create ();

	g_debug ( "helia_media_pl_init" );
}

void helia_media_pl_win ( GtkBox *box, GdkPixbuf *logo, HBTrwCol sw_col_n[], guint num )
{
	helia_media_pl_init ();

	hmpl.sw_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	GtkDrawingArea *video_window = (GtkDrawingArea *)gtk_drawing_area_new ();
	g_signal_connect ( video_window, "realize", G_CALLBACK ( helia_media_pl_video_window_realize ), NULL );
	g_signal_connect ( video_window, "draw",    G_CALLBACK ( helia_media_pl_draw_callback ), logo );

	gtk_widget_add_events ( GTK_WIDGET ( video_window ), GDK_BUTTON_PRESS_MASK | GDK_SCROLL_MASK  );
	g_signal_connect ( video_window, "button-press-event", G_CALLBACK ( helia_media_pl_press_event  ), hmpl.sw_vbox );
	g_signal_connect ( video_window, "scroll-event",       G_CALLBACK ( helia_media_pl_scroll_event ), NULL );


	GtkListStore *liststore   = (GtkListStore *)gtk_list_store_new ( 3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING );
	hmpl.treeview_pl = (GtkTreeView *)gtk_tree_view_new_with_model ( GTK_TREE_MODEL ( liststore ) );
	g_signal_connect ( hmpl.treeview_pl, "button-press-event", G_CALLBACK ( helia_media_pl_press_event_trw  ), NULL );
	g_signal_connect ( hmpl.treeview_pl, "row-activated",      G_CALLBACK ( helia_media_pl_tree_view_row_activated ), NULL );

	GtkScrolledWindow *scroll_win = helia_scrollwin ( hmpl.treeview_pl, sw_col_n, num, 200 );
	gtk_box_pack_start ( hmpl.sw_vbox, GTK_WIDGET ( scroll_win ), TRUE,  TRUE,  0 );

	gtk_box_pack_start ( hmpl.sw_vbox, GTK_WIDGET ( helia_media_pl_create_spinner () ), FALSE, FALSE, 2 );

	GtkBox *hb_box   = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( hb_box ), 5 );
	gtk_box_set_spacing ( hb_box, 5 );
	
	GtkButton *button = helia_service_ret_image_button ( "helia-add", 16 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_treeview_pl_open_files ), GTK_WIDGET ( hmpl.treeview_pl ) );
	gtk_box_pack_start ( hb_box,  GTK_WIDGET ( button ), TRUE,  TRUE,  0 );

	button = helia_service_ret_image_button ( "helia-add-folder", 16 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_treeview_pl_open_dir ), GTK_WIDGET ( hmpl.treeview_pl ) );	
	gtk_box_pack_start ( hb_box,  GTK_WIDGET ( button ), TRUE, TRUE, 0 );

	button = helia_service_ret_image_button ( "helia-save", 16 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_treeview_save_pl ), GTK_WIDGET ( hmpl.treeview_pl ) );	
	gtk_box_pack_start ( hb_box,  GTK_WIDGET ( button ), TRUE, TRUE, 0 );

	button = helia_service_ret_image_button ( hmpl.next_repeat ? "helia-set" : "helia-repeat", 16 );
	//gtk_widget_set_opacity ( GTK_WIDGET ( button ), hmpl.next_repeat ? 1.0 : 0.5 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_media_pl_set_repeat ), NULL );	
	gtk_box_pack_start ( hb_box,  GTK_WIDGET ( button ), TRUE, TRUE, 0 );

	button = helia_service_ret_image_button ( "helia-exit", 16 );
	g_signal_connect_swapped ( button, "clicked", G_CALLBACK ( gtk_widget_hide ), GTK_WIDGET ( hmpl.sw_vbox ) );	
	gtk_box_pack_start ( hb_box,  GTK_WIDGET ( button ), TRUE, TRUE, 0 );

	gtk_widget_set_margin_end ( GTK_WIDGET ( hb_box ), 5 );
	gtk_box_pack_end   ( hmpl.sw_vbox, GTK_WIDGET ( hb_box ), FALSE, FALSE, 0 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( hmpl.sw_vbox ), 5 );


	GtkPaned *hpaned = (GtkPaned *)gtk_paned_new ( GTK_ORIENTATION_HORIZONTAL );
	gtk_paned_add1 ( hpaned, GTK_WIDGET ( hmpl.sw_vbox )      );
	gtk_paned_add2 ( hpaned, GTK_WIDGET ( video_window ) );


	GtkBox *main_hbox   = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_box_pack_start ( main_hbox, GTK_WIDGET ( hpaned ),    TRUE,  TRUE,  0 );
	gtk_box_pack_start ( box,       GTK_WIDGET ( main_hbox ), TRUE,  TRUE,  0 );


	hmpl.hbox_slider = helia_media_pl_slider ();
	gtk_box_pack_start ( box, GTK_WIDGET ( hmpl.hbox_slider ), FALSE, FALSE, 0 );

	helia_media_pl_set_sensitive ( FALSE );


	gtk_drag_dest_set ( GTK_WIDGET ( video_window ), GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY );
	gtk_drag_dest_add_uri_targets  ( GTK_WIDGET ( video_window ) );
	g_signal_connect  ( video_window, "drag-data-received", G_CALLBACK ( helia_media_pl_drag_in ), hmpl.treeview_pl );

	g_debug ( "helia_media_pl_win" );
}
