/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/

#include "helia-panel-treeview.h"
#include "helia-base.h"
#include "helia-service.h"
#include "helia-pref.h"

#include <glib/gi18n.h>



static void helia_treeview_add ( GtkTreeView *tree_view, gchar *name, gchar *data )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
	guint ind = gtk_tree_model_iter_n_children ( model, NULL );

	gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter);
	gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter,
							COL_NUM, ind+1,
							COL_TITLE, name,
							COL_DATA, data,
							-1 );
}
static gint _sort_func ( gconstpointer a, gconstpointer z )
{
	gint res = g_utf8_collate ( a, z );

	if ( !hbgv.sort_az ) res = g_utf8_collate ( z, a );

	return res;
}

static void helia_treeview_slist_sort ( GSList *list, GtkTreeView *tree_view )
{
	GSList *list_sort = g_slist_sort ( list, _sort_func );

	while ( list_sort != NULL )
	{
		gchar *data_file = g_strdup ( list_sort->data );

			gchar **lines = g_strsplit ( data_file, ":@#$%:", 0 );

				helia_treeview_add ( tree_view, lines[0], lines[1] );

			g_strfreev ( lines );

		g_free ( data_file );

		list_sort = list_sort->next;
	}

	g_slist_free ( list_sort );
}
void helia_treeview_sort ( GtkTreeView *tree_view )
{
	g_debug ( "helia_treeview_sort" );

	GSList *list = NULL;

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	gboolean valid;
	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		  valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		gchar *name = NULL, *data = NULL;

			gtk_tree_model_get ( model, &iter, COL_TITLE,  &name, -1 );
			gtk_tree_model_get ( model, &iter, COL_DATA,   &data, -1 );

			list = g_slist_append ( list, g_strconcat ( name, ":@#$%:", data, NULL ) );

		g_free ( name );
		g_free ( data );
	}

	gtk_list_store_clear ( GTK_LIST_STORE ( model ) );

	helia_treeview_slist_sort ( list, tree_view );
	
	g_slist_free ( list );
}

void helia_treeview_to_file ( GtkTreeView *tree_view, gchar *filename, gboolean tv_pl )
{
	g_debug ( "helia_treeview_to_file" );

	GString *gstring;

	if ( tv_pl )
		gstring = g_string_new ( "# Gtv-Dvb channel format \n" );
	else
		gstring = g_string_new ( "#EXTM3U \n" );

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	gboolean valid;
	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		  valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		gchar *data = NULL;
		gchar *name = NULL;

		gtk_tree_model_get ( model, &iter, COL_DATA,  &data, -1 );
		gtk_tree_model_get ( model, &iter, COL_TITLE, &name, -1 );

		if ( !tv_pl )
		{
			g_string_append_printf ( gstring, "#EXTINF:-1,%s\n", name );
		}

		g_string_append_printf ( gstring, "%s\n", data );

		g_free ( name );
		g_free ( data );
	}

	if ( !g_file_set_contents ( filename, gstring->str, -1, NULL ) )
		helia_service_message_dialog ( "Save failed.", filename, GTK_MESSAGE_ERROR );

	g_string_free ( gstring, TRUE );
}

static void helia_treeview_save ( GtkWidget *tree_view, gboolean media_tv_pl, const gchar *dir, const gchar *file )
{
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
	gint ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind == 0 ) return;

	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
									  _("Choose file"),  helia_base_win_ret (), GTK_FILE_CHOOSER_ACTION_SAVE,
										"gtk-cancel", GTK_RESPONSE_CANCEL,
										"gtk-save",   GTK_RESPONSE_ACCEPT,
										NULL );
	if ( media_tv_pl )
		helia_service_add_filter ( dialog, "conf", "*.conf" );
	else
		helia_service_add_filter ( dialog, "m3u",  "*.m3u"  );

	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), dir );
	gtk_file_chooser_set_do_overwrite_confirmation ( GTK_FILE_CHOOSER ( dialog ), TRUE );
	gtk_file_chooser_set_current_name   ( GTK_FILE_CHOOSER ( dialog ), file );

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
	{
		gchar *filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );
			helia_treeview_to_file ( GTK_TREE_VIEW ( tree_view ), filename, media_tv_pl );
		g_free ( filename );
	}

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}
void helia_treeview_save_tv ( GtkButton *button, GtkWidget *tree_view )
{
	helia_treeview_save ( tree_view, TRUE, hbgv.dir_conf, "gtv-channel.conf" );
	
	gtk_widget_destroy ( GTK_WIDGET ( gtk_widget_get_parent ( gtk_widget_get_parent ( gtk_widget_get_parent (GTK_WIDGET (button)) ) ) ) );

	if ( hbgv.all_info )
		g_debug ( "helia_media_save_tv:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}
void helia_treeview_save_pl ( GtkButton *button, GtkWidget *tree_view )
{
	helia_treeview_save ( tree_view, FALSE, g_get_home_dir (), "playlist-001.m3u" );

	if ( hbgv.all_info )
		g_debug ( "helia_media_save_pl:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}
static void helia_treeview_sort_pl ( GtkButton *button, GtkWidget *tree_view )
{
	helia_treeview_sort ( GTK_TREE_VIEW ( tree_view ) );
	
	hbgv.sort_az = !hbgv.sort_az;

	if ( hbgv.all_info )
		g_debug ( "helia_treeview_sort_pl:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );	
}

static void helia_treeview_clear_click ( GtkButton *button, GtkTreeModel *model )
{
	gtk_list_store_clear ( GTK_LIST_STORE ( model ) );

	g_debug ( "helia_treeview_clear_click:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}
static void helia_treeview_close_click ( GtkButton *button, GtkWindow *window )
{
	gtk_widget_destroy ( GTK_WIDGET ( window ) );

	if ( hbgv.all_info )
		g_debug ( "helia_treeview_close_click:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}
static void helia_treeview_clear ( GtkTreeView *tree_view )
{
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
	guint ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind == 0 ) return;

	GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window, helia_base_win_ret () );
	gtk_window_set_modal     ( window, TRUE );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_title     ( window, _("Clear") );

	gtk_widget_set_size_request ( GTK_WIDGET ( window ), 400, 150 );


	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *i_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	GdkPixbuf *logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					 "helia-warning", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	GtkImage *image   = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );
	gtk_box_pack_start ( i_box, GTK_WIDGET ( image ), TRUE, TRUE, 0 );

	GtkLabel *label = (GtkLabel *)gtk_label_new ( "" );

	gchar *text = g_strdup_printf ( "%d", ind );
	gtk_label_set_text ( label, text );
	g_free  ( text );

	gtk_box_pack_start ( i_box, GTK_WIDGET ( label ), TRUE, TRUE, 10 );

	logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
			"helia-clear", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	image = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );
	gtk_box_pack_start ( i_box, GTK_WIDGET ( image ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( i_box ), TRUE, TRUE, 5 );


	GtkButton *button_clear = (GtkButton *)gtk_button_new_from_icon_name ( "helia-ok", GTK_ICON_SIZE_SMALL_TOOLBAR );
	g_signal_connect ( button_clear, "clicked", G_CALLBACK ( helia_treeview_clear_click ), model  );
	g_signal_connect ( button_clear, "clicked", G_CALLBACK ( helia_treeview_close_click ), window );
	gtk_box_pack_end ( h_box, GTK_WIDGET ( button_clear ), TRUE, TRUE, 5 );

	GtkButton *button_close = (GtkButton *)gtk_button_new_from_icon_name ( "helia-exit", GTK_ICON_SIZE_SMALL_TOOLBAR );
	g_signal_connect ( button_close, "clicked", G_CALLBACK ( helia_treeview_close_click ), window );
	gtk_box_pack_end ( h_box, GTK_WIDGET ( button_close ), TRUE, TRUE, 5 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );


	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), hbgv.opacity_win );

	g_debug ( "helia_treeview_clear" );
}

static void helia_treeview_reread ( GtkTreeView *tree_view )
{
	g_debug ( "helia_treeview_reread" );

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	gint row_count = 1;
	gboolean valid;
	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		  valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter, COL_NUM, row_count++, -1 );
	}
}

static void helia_treeview_up_down ( GtkTreeView *tree_view, gboolean up_dw )
{
	g_debug ( "helia_treeview_up_down" );

	GtkTreeIter iter, iter_c;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
	gint ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind < 2 ) return;

	if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter ) )
	{
		gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter_c );

		if ( up_dw )
		if ( gtk_tree_model_iter_previous ( model, &iter ) )
			gtk_list_store_move_before ( GTK_LIST_STORE ( model ), &iter_c, &iter );

		if ( !up_dw )
		if ( gtk_tree_model_iter_next ( model, &iter ) )
			gtk_list_store_move_after ( GTK_LIST_STORE ( model ), &iter_c, &iter );

		helia_treeview_reread ( tree_view );
	}
	else if ( gtk_tree_model_get_iter_first ( model, &iter ) )
	{
		gtk_tree_selection_select_iter ( gtk_tree_view_get_selection (tree_view), &iter);
	}
}

static void helia_treeview_up_down_s ( GtkTreeView *tree_view, gboolean up_dw )
{
	g_debug ( "helia_treeview_up_down_s" );

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
	gint ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind < 2 ) return;

	if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter ) )
	{
		GtkTreePath *path;

		if ( up_dw )
		if ( gtk_tree_model_iter_previous ( model, &iter ) )
		{
			gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( tree_view ) ), &iter );

			path = gtk_tree_model_get_path (model, &iter);
			gtk_tree_view_scroll_to_cell ( tree_view, path, NULL, FALSE, 0, 0 );
		}

		if ( !up_dw )
		if ( gtk_tree_model_iter_next ( model, &iter ) )
		{
			gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( tree_view ) ), &iter );

			path = gtk_tree_model_get_path (model, &iter);
			gtk_tree_view_scroll_to_cell ( tree_view, path, NULL, FALSE, 0, 0 );
		}
	}
	else if ( gtk_tree_model_get_iter_first ( model, &iter ) )
	{
		gtk_tree_selection_select_iter ( gtk_tree_view_get_selection (tree_view), &iter);
	}
}

static void helia_treeview_remove ( GtkTreeView *tree_view )
{
	gboolean prev_i = FALSE;
	GtkTreeIter iter, iter_prev;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter ) )
	{
		gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter_prev );
		prev_i = gtk_tree_model_iter_previous ( model, &iter_prev );

		gtk_list_store_remove ( GTK_LIST_STORE ( model ), &iter );
		helia_treeview_reread ( tree_view );

		if ( prev_i )
		{
			gtk_tree_selection_select_iter ( gtk_tree_view_get_selection (tree_view), &iter_prev );
		}
		else
		{
			if ( gtk_tree_model_get_iter_first ( model, &iter ) )
				gtk_tree_selection_select_iter ( gtk_tree_view_get_selection (tree_view), &iter );
		}
	}

	if ( hbgv.all_info )
		g_debug ( "helia_treeview_remove:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( tree_view ) ) );
}

static void helia_panel_treeview_down_s  ( GtkButton *button, GtkWidget *tree_view )
{
	helia_treeview_up_down_s ( GTK_TREE_VIEW ( tree_view ), FALSE );

	if ( hbgv.all_info )
		g_debug ( "helia_panel_treeview_down_s:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void helia_panel_treeview_goup_s  ( GtkButton *button, GtkWidget *tree_view )
{
	helia_treeview_up_down_s ( GTK_TREE_VIEW ( tree_view ), TRUE  );

	if ( hbgv.all_info )
		g_debug ( "helia_panel_treeview_goup_s:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void helia_panel_treeview_goup  ( GtkButton *button, GtkWidget *tree_view )
{
	helia_treeview_up_down ( GTK_TREE_VIEW ( tree_view ), TRUE  );

	if ( hbgv.all_info )
		g_debug ( "helia_panel_treeview_goup:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void helia_panel_treeview_down  ( GtkButton *button, GtkWidget *tree_view )
{
	helia_treeview_up_down ( GTK_TREE_VIEW ( tree_view ), FALSE );

	if ( hbgv.all_info )
		g_debug ( "helia_panel_treeview_down:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void helia_panel_treeview_clear ( GtkButton *button, GtkWidget *tree_view )
{
	helia_treeview_clear   ( GTK_TREE_VIEW ( tree_view ) );
	
	gtk_widget_destroy ( GTK_WIDGET ( gtk_widget_get_parent ( gtk_widget_get_parent ( gtk_widget_get_parent (GTK_WIDGET (button)) ) ) ) );

	if ( hbgv.all_info )
		g_debug ( "helia_panel_treeview_clear:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void helia_panel_treeview_remv  ( GtkButton *button, GtkWidget *tree_view )
{
	helia_treeview_remove  ( GTK_TREE_VIEW ( tree_view ) );

	if ( hbgv.all_info )
		g_debug ( "helia_panel_treeview_remv:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void helia_panel_treeview_quit ( GtkWidget *window )
{
	gtk_widget_destroy ( GTK_WIDGET ( window ) );
}
static void helia_panel_treeview_close ( GtkButton *button, GtkWidget *window )
{
	helia_panel_treeview_quit ( GTK_WIDGET ( window ) );

	if ( hbgv.all_info )
		g_debug ( "helia_panel_treeview_close:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

void helia_panel_treeview ( GtkTreeView *tree_view, GtkWindow *parent, gdouble opacity, gboolean media_tv_pl, guint resize_icon )
{
	GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window, parent );
	gtk_window_set_modal     ( window, TRUE   );
	gtk_window_set_decorated ( window, FALSE  );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );

	GtkBox *m_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	GtkBox *hc_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_box_set_spacing ( m_box,  5 );
	gtk_box_set_spacing ( h_box,  5 );
	gtk_box_set_spacing ( hc_box, 5 );


	const struct PanelTrwNameIcon { const gchar *name; void (* activate)(); GtkWidget *widget; } NameIcon_n[] =
	{
		{ "helia-up", 			helia_panel_treeview_goup,  GTK_WIDGET ( tree_view ) },
		{ "helia-down", 		helia_panel_treeview_down,  GTK_WIDGET ( tree_view ) },
		{ "helia-remove", 		helia_panel_treeview_remv,  GTK_WIDGET ( tree_view ) },
		{ "helia-clear", 		helia_panel_treeview_clear, GTK_WIDGET ( tree_view ) },

		{ "helia-up-select", 	helia_panel_treeview_goup_s, GTK_WIDGET ( tree_view ) },
		{ "helia-down-select", 	helia_panel_treeview_down_s, GTK_WIDGET ( tree_view ) },
		{ "helia-save", 		helia_treeview_save_tv,		 GTK_WIDGET ( tree_view ) },
		{ "helia-sort", 		helia_treeview_sort_pl, 	 GTK_WIDGET ( tree_view ) },
		{ "helia-exit", 		helia_panel_treeview_close,  GTK_WIDGET ( window    ) }
	};

	guint i = 0;
	for ( i = 0; i < G_N_ELEMENTS ( NameIcon_n ); i++ )
	{
		if ( media_tv_pl ) 
			{ if ( g_str_has_suffix ( NameIcon_n[i].name, "helia-sort" ) ) continue; }
		else
			{ if ( g_str_has_suffix ( NameIcon_n[i].name, "helia-save" ) ) continue; }

		GtkButton *button = helia_service_ret_image_button ( NameIcon_n[i].name, resize_icon );
		g_signal_connect ( button, "clicked", G_CALLBACK ( NameIcon_n[i].activate ), NameIcon_n[i].widget );

		gtk_box_pack_start ( ( i < 4 ) ? h_box : hc_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	}

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box  ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( m_box, GTK_WIDGET ( hc_box ), TRUE, TRUE, 0 );


	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
	gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity );
	
	g_debug ( "helia_panel_treeview" );
}

