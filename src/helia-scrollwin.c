/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia-scrollwin.h"


static void helia_scrollwin_create_columns ( GtkTreeView *tree_view, GtkTreeViewColumn *column, GtkCellRenderer *renderer, const gchar *name, gint column_id, gboolean col_vis )
{
	column = gtk_tree_view_column_new_with_attributes ( name, renderer, "text", column_id, NULL );
	gtk_tree_view_append_column ( tree_view, column );
	gtk_tree_view_column_set_visible ( column, col_vis );
	
	g_debug ( "helia_scrollwin_create_columns" );
}
static void helia_scrollwin_add_columns ( GtkTreeView *tree_view, HBTrwCol sw_col_n[], guint num )
{
	GtkTreeViewColumn *column_n[num];
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

	guint c = 0;
	for ( c = 0; c < num; c++ )
		helia_scrollwin_create_columns ( tree_view, column_n[c], renderer, sw_col_n[c].title, c, sw_col_n[c].visible );

	g_debug ( "helia_scrollwin_add_columns" );
}

GtkScrolledWindow * helia_scrollwin ( GtkTreeView *tree_view, HBTrwCol sw_col_n[], guint num, guint size_request )
{
	g_debug ( "helia_scrollwin" );
	
	GtkScrolledWindow *scroll_win = (GtkScrolledWindow *)gtk_scrolled_window_new ( NULL, NULL );
	gtk_scrolled_window_set_policy ( scroll_win, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_widget_set_size_request ( GTK_WIDGET ( scroll_win ), size_request, -1 );

	gtk_tree_view_set_search_column ( GTK_TREE_VIEW ( tree_view ), COL_TITLE );
	helia_scrollwin_add_columns ( tree_view, sw_col_n, num );

	gtk_container_add ( GTK_CONTAINER ( scroll_win ), GTK_WIDGET ( tree_view ) );

	return scroll_win;
}
