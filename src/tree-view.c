/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "tree-view.h"
#include "button.h"
#include "file.h"

struct _TreeView
{
	GtkTreeView parent_instance;
};

G_DEFINE_TYPE ( TreeView, tree_view, GTK_TYPE_TREE_VIEW )

static const char *title[3];

typedef void ( *ftp ) ( GtkButton *button, TreeView *tree_view );

static void tree_view_to_file ( const char *file, gboolean mp_tv, GtkTreeView *tree_view )
{
	GString *gstring = g_string_new ( ( mp_tv ) ? "#EXTM3U \n" : "# Gtv-Dvb channel format \n" );

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind == 0 ) return;

	gboolean valid;
	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		  valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		char *name = NULL;
		char *data = NULL;

		gtk_tree_model_get ( model, &iter, 1, &name, -1 );
		gtk_tree_model_get ( model, &iter, 2, &data, -1 );

		if ( mp_tv ) g_string_append_printf ( gstring, "#EXTINF:-1,%s\n", name );

		g_string_append_printf ( gstring, "%s\n", data );

		free ( name );
		free ( data );
	}

	GError *err = NULL;

	if ( !g_file_set_contents ( file, gstring->str, -1, &err ) )
	{
		g_critical ( "%s: %s ", __func__, err->message );
		g_error_free ( err );
	}

	g_string_free ( gstring, TRUE );
}

static void tree_view_append ( const char *name, const char *data, GtkTreeView *treeview )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( treeview );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );

	gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter );
	gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter,
				COL_NUM,  ind + 1,
				COL_FLCH, name,
				COL_DATA, data,
				-1 );
}

static void tree_view_add_channels ( const char *file, GtkTreeView *treeview )
{
	char  *contents = NULL;
	GError *err     = NULL;

	if ( g_file_get_contents ( file, &contents, 0, &err ) )
	{
		char **lines = g_strsplit ( contents, "\n", 0 );

		uint i = 0; for ( i = 0; lines[i] != NULL; i++ )
		// for ( i = 0; lines[i] != NULL && *lines[i]; i++ )
		{
			if ( g_str_has_prefix ( lines[i], "#" ) || strlen ( lines[i] ) < 2 ) continue;

			char **data = g_strsplit ( lines[i], ":", 0 );

			tree_view_append ( data[0], lines[i], treeview );

			g_strfreev ( data );
		}

		g_strfreev ( lines );
		free ( contents );
	}
	else
	{
		g_critical ( "%s:: ERROR: %s ", __func__, err->message );
		g_error_free ( err );
	}
}

static gboolean tree_view_media_filter ( const char *file_name, gboolean m3u )
{
	gboolean res = FALSE;
	GError  *err = NULL;

	GFile *file = g_file_new_for_path ( file_name );
	GFileInfo *file_info = g_file_query_info ( file, "standard::*", 0, NULL, &err );

	const char *content_type = g_file_info_get_content_type ( file_info );

	if ( m3u )
	{
		if ( g_str_has_suffix ( content_type, "mpegurl" ) ) res =  TRUE;
	}
	else
	{
		if ( g_str_has_prefix ( content_type, "audio" ) || g_str_has_prefix ( content_type, "video" ) ) res =  TRUE;
	}

	g_object_unref ( file_info );
	g_object_unref ( file );

	if ( err )
	{
		g_critical ( "%s:: ERROR: %s ", __func__, err->message );
		g_error_free ( err );
	}

	return res;
}

static void tree_view_add_m3u ( const char *file, GtkTreeView *treeview )
{
	char  *contents = NULL;
	GError *err     = NULL;

	if ( g_file_get_contents ( file, &contents, 0, &err ) )
	{
		char **lines = g_strsplit ( contents, "\n", 0 );

		uint i = 0; for ( i = 0; lines[i] != NULL; i++ )
		//for ( i = 0; lines[i] != NULL && *lines[i]; i++ )
		{
			if ( g_str_has_prefix ( lines[i], "#EXTM3U" ) || g_str_has_prefix ( lines[i], " " ) || strlen ( lines[i] ) < 4 ) continue;

			if ( g_str_has_prefix ( lines[i], "#EXTINF" ) )
			{
				char **lines_info = g_strsplit ( lines[i], ",", 0 );

					if ( g_str_has_prefix ( lines[i+1], "#EXTGRP" ) ) i++;

					tree_view_append ( g_strstrip ( lines_info[1] ), g_strstrip ( lines[i+1] ), treeview );

				g_strfreev ( lines_info );
				i++;
			}
			else
			{
				if ( g_str_has_prefix ( lines[i], "#" ) || g_str_has_prefix ( lines[i], " " ) || strlen ( lines[i] ) < 4 ) continue;

				char *name = g_path_get_basename ( lines[i] );

				tree_view_append ( g_strstrip ( name ), g_strstrip ( lines[i] ), treeview );

				free ( name );
			}
		}

		g_strfreev ( lines );
		free ( contents );
	}
	else
	{
		g_critical ( "%s:: ERROR: %s ", __func__, err->message );
		g_error_free ( err );
	}
}

static int _sort_func_list ( gconstpointer a, gconstpointer b )
{
	return g_utf8_collate ( a, b );
}

static void tree_view_slist_sort ( GList *list, GtkTreeView *treeview )
{
	GList *list_sort = g_list_sort ( list, _sort_func_list );

	while ( list_sort != NULL )
	{
		char *data = (char *)list_sort->data;
		char *name = g_path_get_basename ( data );

		tree_view_append ( name, data, treeview );

		list_sort = list_sort->next;
		free ( name );
	}

	g_list_free_full ( list_sort, (GDestroyNotify) g_free );
}

static void tree_view_add_dir ( const char *dir_path, GtkTreeView *treeview )
{
	GList *list = NULL;
	GDir *dir = g_dir_open ( dir_path, 0, NULL );

	if ( dir )
	{
		const char *name = NULL;

		while ( ( name = g_dir_read_name ( dir ) ) != NULL )
		{
			char *path_name = g_strconcat ( dir_path, "/", name, NULL );

			if ( g_file_test ( path_name, G_FILE_TEST_IS_DIR ) )
				tree_view_add_dir ( path_name, treeview ); // Recursion!

			if ( g_file_test ( path_name, G_FILE_TEST_IS_REGULAR ) )
			{
				if ( tree_view_media_filter ( path_name, TRUE ) )
				// if ( g_str_has_suffix ( path_name, "m3u" ) || g_str_has_suffix ( path_name, "M3U" ) )
					tree_view_add_m3u ( path_name, treeview );
				else
				{
					if ( tree_view_media_filter ( path_name, FALSE ) )
						list = g_list_append ( list, g_strdup ( path_name ) );
						// tree_view_append ( name, path_name, treeview );
				}
			}

			free ( path_name );
		}

		g_dir_close ( dir );
	}
	else
	{
		g_critical ( "%s: opening directory %s failed.", __func__, dir_path );
	}

	tree_view_slist_sort ( list, treeview );

	g_list_free_full ( list, (GDestroyNotify) g_free );
}

static void tree_view_add_files ( const char *file, GtkTreeView *treeview )
{
	if ( g_file_test ( file, G_FILE_TEST_IS_DIR ) )
	{
		tree_view_add_dir ( file, treeview);
	}

	if ( g_file_test ( file, G_FILE_TEST_IS_REGULAR ) )
	{
		if ( tree_view_media_filter ( file, TRUE ) )
		// if ( g_str_has_suffix ( file, "m3u" ) || g_str_has_suffix ( file, "M3U" ) )
		{
			tree_view_add_m3u ( file, treeview );
		}
		else
		{
			g_autofree char *name = g_path_get_basename ( file );

			tree_view_append ( name, file, treeview );
		}
	}
}

static void treeview_reread ( GtkTreeView *tree_view )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	int row_count = 1;
	gboolean valid;

	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		  valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter, COL_NUM, row_count++, -1 );
	}
}

static void treeview_up_down ( gboolean up_dw, TreeView *tree_view )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );
	if ( ind < 2 ) return;

	if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( tree_view ) ), NULL, &iter ) )
	{
		GtkTreeIter *iter_c = gtk_tree_iter_copy ( &iter );

		if ( up_dw )
		if ( gtk_tree_model_iter_previous ( model, &iter ) )
			gtk_list_store_move_before ( GTK_LIST_STORE ( model ), iter_c, &iter );

		if ( !up_dw )
		if ( gtk_tree_model_iter_next ( model, &iter ) )
			gtk_list_store_move_after ( GTK_LIST_STORE ( model ), iter_c, &iter );

		gtk_tree_iter_free ( iter_c );

		treeview_reread ( GTK_TREE_VIEW ( tree_view ) );
	}
}

static void treeview_remove ( G_GNUC_UNUSED GtkButton *button, TreeView *tree_view )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

	if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( tree_view ) ), NULL, &iter ) )
	{
		gtk_list_store_remove ( GTK_LIST_STORE ( model ), &iter );

		treeview_reread ( GTK_TREE_VIEW ( tree_view ) );
	}
}

static void treeview_goup ( G_GNUC_UNUSED GtkButton *button, TreeView *tree_view )
{
	treeview_up_down ( TRUE, tree_view );
}

static void treeview_down ( G_GNUC_UNUSED GtkButton *button, TreeView *tree_view )
{
	treeview_up_down ( FALSE, tree_view );
}

static void treeview_clear ( G_GNUC_UNUSED GtkButton *button, TreeView *tree_view )
{
	gtk_list_store_clear ( GTK_LIST_STORE ( gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) ) ) );
}

static void treeview_enc ( G_GNUC_UNUSED GtkButton *button, TreeView *treeview )
{
	g_signal_emit_by_name ( treeview, "treeview-enc" );
}
static void treeview_scan ( GtkButton *button, TreeView *treeview )
{
	g_signal_emit_by_name ( treeview, "treeview-repeat-scan", button );
}

static void treeview_close ( G_GNUC_UNUSED GtkButton *button, TreeView *treeview )
{
	g_signal_emit_by_name ( treeview, "treeview-close" );
}

static void treeview_save_mp ( G_GNUC_UNUSED GtkButton *button, TreeView *treeview )
{
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( treeview ) );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind == 0 ) return;

	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( button ) ) );

	g_autofree char *path = helia_save_file ( g_get_home_dir (), "playlist-001.m3u", "m3u", "*.m3u", window );

	if ( path == NULL ) return;

	tree_view_to_file ( path, TRUE, GTK_TREE_VIEW ( treeview ) );
}

static void treeview_save_tv ( G_GNUC_UNUSED GtkButton *button, TreeView *treeview )
{
	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( treeview ) );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind == 0 ) return;

	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( button ) ) );

	g_autofree char *path = helia_save_file ( g_get_home_dir (), "gtv-channel.conf", "conf", "*.conf", window );

	if ( path == NULL ) return;

	tree_view_to_file ( path, FALSE, GTK_TREE_VIEW ( treeview ) );
}

GtkBox * create_treeview_box ( gboolean tv, TreeView *treeview )
{
	GtkBox *v_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( v_box, 5 );

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 5 );

	uint8_t BN = 4;
	const char *name_a[]  = { "⬆", "⬇", "➖", "🗑" };
	const char *icons_a[] = { "helia-up", "helia-down", "helia-remove", "helia-clear" };
	ftp funcs_a[] = { treeview_goup, treeview_down, treeview_remove, treeview_clear };

	uint8_t c = 0; for ( c = 0; c < BN; c++ )
	{
		GtkButton *button = helia_create_button ( h_box, icons_a[c], name_a[c], 16 );
		gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
		g_signal_connect ( button, "clicked", G_CALLBACK ( funcs_a[c] ), treeview );
	}

	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( h_box ), TRUE, TRUE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 5 );

	const char *name_b[] = { "🛠", ( tv ) ? "📡" : "🔁", "🖴", "🞬" };
	const char *icons_b[] = { "helia-pref", ( tv ) ?  "helia-display" : "helia-repeat", "helia-save", "helia-exit" };
	ftp funcs_b[] = { treeview_enc, treeview_scan, ( tv ) ? treeview_save_tv : treeview_save_mp, treeview_close };

	for ( c = 0; c < BN; c++ )
	{
		GtkButton *button = helia_create_button ( h_box, icons_b[c], name_b[c], 16 );
		gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
		g_signal_connect ( button, "clicked", G_CALLBACK ( funcs_b[c] ), treeview );
	}

	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( h_box ), TRUE, TRUE, 0 );

	return v_box;
}

static void treeview_handler_add_files ( TreeView *treeview, const char *path )
{
	tree_view_add_files ( path, GTK_TREE_VIEW ( treeview ) );
}

static void treeview_handler_add_channels ( TreeView *treeview, const char *path )
{
	tree_view_add_channels ( path, GTK_TREE_VIEW ( treeview ) );
}

static void treeview_handler_append ( TreeView *treeview, const char *name, const char *data )
{
	tree_view_append ( name, data, GTK_TREE_VIEW ( treeview ) );
}

static char * treeview_handler_next ( TreeView *treeview, const char *uri, uint num )
{
	char *path = NULL;

	GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( treeview ) );

	uint count = 0;
	GtkTreeIter iter;
	gboolean valid, break_f = FALSE;

	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		char *data = NULL;
		gtk_tree_model_get ( model, &iter, COL_DATA, &data, -1 );

		if ( uri == NULL )
		{
			if ( count == num )
			{
				path = g_strdup ( data );
				gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( treeview ) ), &iter );

				break_f = TRUE;
			}
		}
		else
		{
			char *next = helia_uri_get_path ( uri );

			if ( g_strrstr ( next, data ) )
			{
				if ( gtk_tree_model_iter_next ( model, &iter ) )
				{
					char *data2 = NULL;
					gtk_tree_model_get ( model, &iter, COL_DATA, &data2, -1 );

					path = g_strdup ( data2 );

					gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( treeview ) ), &iter );
					free ( data2 );

				}

				break_f = TRUE;
			}

			free ( next );
		}

		count++;
		free ( data );
		if ( break_f ) break;
	}

	return path;
}

static void treeview_autosave_channels ( GtkTreeView *tree_view )
{
	GtkTreeViewColumn *column = gtk_tree_view_get_column ( tree_view, COL_FLCH );

	const char *title = gtk_tree_view_column_get_title ( column );

	if ( !g_str_has_prefix ( title, "Channels" ) ) return;

	char path[PATH_MAX];
	sprintf ( path, "%s/helia/gtv-channel.conf", g_get_user_config_dir () );

	tree_view_to_file ( path, FALSE, tree_view );
}

static void treeview_destroy ( GtkTreeView *tree_view )
{
	treeview_autosave_channels ( tree_view );
}

static void tree_view_init ( TreeView *view )
{
	GtkListStore *store = (GtkListStore *)gtk_list_store_new ( NUM_COLS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING );

	GtkTreeView *treeview = GTK_TREE_VIEW ( view );

	gtk_tree_view_set_model ( treeview, GTK_TREE_MODEL ( store ) );
	gtk_tree_view_set_search_column ( treeview, COL_FLCH );

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	uint8_t c = 0; for ( c = 0; c < NUM_COLS; c++ )
	{
		renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes ( title[c], renderer, "text", c, NULL );
		if ( c == COL_DATA ) gtk_tree_view_column_set_visible ( column, FALSE );
		gtk_tree_view_append_column ( treeview, column );
	}

	g_object_unref ( G_OBJECT (store) );

	g_signal_connect ( view, "treeview-add-files",    G_CALLBACK ( treeview_handler_add_files    ), NULL );
	g_signal_connect ( view, "treeview-add-channels", G_CALLBACK ( treeview_handler_add_channels ), NULL );
	g_signal_connect ( view, "treeview-append-data",  G_CALLBACK ( treeview_handler_append       ), NULL );
	g_signal_connect ( view, "treeview-next",         G_CALLBACK ( treeview_handler_next         ), NULL );

	g_signal_connect ( treeview, "destroy", G_CALLBACK ( treeview_destroy ), NULL );
}

static void tree_view_finalize ( GObject *object )
{
	G_OBJECT_CLASS ( tree_view_parent_class )->finalize (object);
}

static void tree_view_class_init ( TreeViewClass *class )
{
	GObjectClass *oclass   = G_OBJECT_CLASS ( class );

	oclass->finalize = tree_view_finalize;

	g_signal_new ( "treeview-add-files", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );

	g_signal_new ( "treeview-add-channels", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );

	g_signal_new ( "treeview-append-data", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING );

	g_signal_new ( "treeview-enc", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "treeview-repeat-scan", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_OBJECT );

	g_signal_new ( "treeview-close", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 0 );

	g_signal_new ( "treeview-next", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_STRING, 2, G_TYPE_STRING, G_TYPE_UINT );
}

TreeView * tree_view_new ( const char *title_abc[] )
{
	title[COL_NUM]  = title_abc[COL_NUM];
	title[COL_FLCH] = title_abc[COL_FLCH];
	title[COL_DATA] = title_abc[COL_DATA];

	return g_object_new ( TREE_TYPE_VIEW, NULL );
}
