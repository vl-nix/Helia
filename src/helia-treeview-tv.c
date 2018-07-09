/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia-treeview-tv.h"


void helia_treeview_tv_add ( GtkTreeView *tree_view, gchar *name, gchar *data )
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

	//g_debug ( "helia_treeview_tv_add" );
}

static void helia_treeview_tv_split ( gchar *data, GtkTreeView *tree_view )
{
	gchar **lines = g_strsplit ( data, ":", 0 );

	if ( !g_str_has_prefix ( data, "#" ) )
		helia_treeview_tv_add ( tree_view, lines[0], data );

	g_strfreev ( lines );
	
	//g_debug ( "helia_treeview_tv_split" );
}

void helia_treeview_tv ( const gchar *file, GtkTreeView *tree_view )
{
	guint n = 0;
	gchar *contents;
	GError *err = NULL;

	if ( g_file_get_contents ( file, &contents, 0, &err ) )
	{
		gchar **lines = g_strsplit ( contents, "\n", 0 );

		for ( n = 0; lines[n] != NULL; n++ )
			if ( *lines[n] )
				helia_treeview_tv_split ( lines[n], tree_view );

		g_strfreev ( lines );
		g_free ( contents );
	}
	else
	{
		g_critical ( "Channels to treeview:: %s\n", err->message );
		g_error_free ( err );
	}
	
	g_debug ( "helia_treeview_tv" );
}
