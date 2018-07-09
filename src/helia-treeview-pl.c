/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia-treeview-tv.h"
#include "helia-media-pl.h"
#include "helia-service.h"
#include "helia-gst-pl.h"


static void helia_treeview_pl_add ( GtkTreeView *tree_view, gchar *name, gchar *data )
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

	if ( hbgv.play && helia_gst_pl_get_current_state_stop () )
	{
		g_debug ( "helia_treeview_pl_add:: play" );

		helia_media_pl_set_path ( model, iter );
		gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( tree_view ) ), &iter );

		helia_gst_pl_stop_set_play ( data, helia_media_pl_handle_ret () );
		hbgv.play = FALSE;
	}

	//g_debug ( "helia_treeview_pl_add" );
}

static gboolean helia_treeview_pl_filter ( gchar *file_name )
{
	//g_debug ( "helia_treeview_pl_filter" );
	
	gboolean res  = FALSE;
	GError *error = NULL;
	
	GFile *file = g_file_new_for_path ( file_name );
	GFileInfo *file_info = g_file_query_info ( file, "standard::*", 0, NULL, &error );

	const char *content_type = g_file_info_get_content_type ( file_info );

	if ( g_str_has_prefix ( content_type, "audio" ) || g_str_has_prefix ( content_type, "video" ) )
		res =  TRUE;

	gchar *text = g_utf8_strdown ( file_name, -1 );

	if ( g_str_has_suffix ( text, "asx" ) || g_str_has_suffix ( text, "pls" ) )
		res = FALSE;

	//g_debug ( "helia_treeview_pl_filter:: file_name %s | content_type %s", file_name, content_type );

	g_free ( text );

	g_object_unref ( file_info );
	g_object_unref ( file );

	return res;
}

static void helia_treeview_pl_m3u ( GtkTreeView *tree_view, gchar *file )
{
	g_debug ( "helia_treeview_pl_m3u" );
	
	gchar  *contents = NULL;
	GError *err      = NULL;

	if ( g_file_get_contents ( file, &contents, 0, &err ) )
	{
		gchar **lines = g_strsplit ( contents, "\n", 0 );

		gint i; for ( i = 0; lines[i] != NULL; i++ )
		//for ( i = 0; lines[i] != NULL && *lines[i]; i++ )
		{
			if ( g_str_has_prefix ( lines[i], "#EXTINF" ) )
			{
				if ( g_str_has_prefix ( lines[i+1], "/" ) || g_strrstr ( lines[i+1], "://" ) )
				{
					gchar **lines_info = g_strsplit ( lines[i], ",", 0 );
						helia_treeview_pl_add ( tree_view, g_strstrip ( lines_info[1] ), g_strstrip ( lines[i+1] ) );
					g_strfreev ( lines_info );

					i += 1;
				}
			}
			else
			{
				if ( g_str_has_prefix ( lines[i], "/" ) || g_strrstr ( lines[i], "://" ) )
				{
					gchar *name = g_path_get_basename ( lines[i] );
						helia_treeview_pl_add ( tree_view, g_strstrip ( name ), g_strstrip ( lines[i] ) );
					g_free ( name );
				}
			}
		}

		g_strfreev ( lines );
		g_free ( contents );
	}
	else
	{
		g_critical ( "helia_treeview_pl_m3u:: file %s | %s\n", file, err->message );
		g_error_free ( err );
	}
}

static gboolean helia_treeview_pl_check_m3u ( GtkTreeView *tree_view, gchar *file )
{
	gboolean res = FALSE;
	
	gchar *text = g_utf8_strdown ( file, -1 );
			
	if ( g_str_has_suffix ( text, ".m3u" ) )
	{
		helia_treeview_pl_m3u ( tree_view, file );
		
		res = TRUE;
	}	
		
	g_free ( text );
	
	g_debug ( "helia_treeview_pl_check_m3u" );
	
	return res;
}
/*
static gint _sort_func ( gconstpointer a, gconstpointer b )
{
	return g_utf8_collate ( a, b );
}

static void helia_treeview_pl_slist_sort ( GSList *list, GtkTreeView *tree_view )
{
	GSList *list_sort = g_slist_sort ( list, _sort_func );

	while ( list_sort != NULL )
	{
		gchar *name_file = g_strdup ( list_sort->data );
		gchar *text = g_utf8_strdown ( name_file, -1 );

		// g_debug ( "helia_treeview_pl_slist_sort:: name_file %s", name_file );

		if ( g_str_has_suffix ( text, ".m3u" ) )
			helia_treeview_pl_m3u ( tree_view, name_file );
		else
		{
			if ( helia_treeview_pl_filter ( name_file ) )
			{
				gchar *name = g_path_get_basename ( name_file );
					helia_treeview_pl_add ( tree_view, name, name_file );
				g_free ( name );
			}		
		}

		g_free ( text );
		g_free ( name_file );

		list_sort = list_sort->next;
	}

	g_slist_free ( list_sort );
	
	g_debug ( "helia_treeview_pl_slist_sort" );
}
*/
static void helia_treeview_pl_dir ( GtkTreeView *tree_view, gchar *dir_path )
{
	GDir *dir = g_dir_open ( dir_path, 0, NULL );
	const gchar *name = NULL;

	if ( !dir )
		g_printerr ( "opening directory %s failed\n", dir_path );
	else
	{
		while ( ( name = g_dir_read_name ( dir ) ) != NULL )
		{
			gchar *path_name = g_strconcat ( dir_path, "/", name, NULL );

			if ( g_file_test ( path_name, G_FILE_TEST_IS_DIR ) )
				helia_treeview_pl_dir ( tree_view, path_name ); // Recursion!

			if ( g_file_test ( path_name, G_FILE_TEST_IS_REGULAR ) )
			{
				if ( !helia_treeview_pl_check_m3u ( tree_view, path_name ) )
				{
					if ( helia_treeview_pl_filter ( path_name ) )
					{
						gchar *name = g_path_get_basename ( path_name );
							helia_treeview_pl_add ( tree_view, name, path_name );
						g_free ( name );
					}
				}
			}

			name = NULL;
			g_free ( path_name );
		}

		g_dir_close ( dir );
		name = NULL;
	}
	
	g_debug ( "helia_treeview_pl_dir" );
}

static void helia_treeview_pl_file ( gchar *file, GtkTreeView *tree_view )
{
	g_debug ( "helia_treeview_pl_file" );

	if ( g_file_test ( file, G_FILE_TEST_IS_REGULAR ) )
	{			
		if ( !helia_treeview_pl_check_m3u ( tree_view, file ) )
		{
			if ( helia_treeview_pl_filter ( file ) )
			{
				gchar *name = g_path_get_basename ( file );
					helia_treeview_pl_add ( tree_view, name, file );
				g_free ( name );
			}
		}
	}
	
	if ( g_file_test ( file, G_FILE_TEST_IS_DIR ) )
	{
		helia_treeview_pl_dir ( tree_view, file );
		return;
	}	
}

void helia_treeview_pl_files ( GFile **files, gchar **uris, gint n_files, GtkTreeView *tree_view )
{
	g_debug ( "helia_treeview_pl_files" );

	hbgv.play = TRUE;

	gint i = 0;

	for ( i = 0; i < n_files; i++ )
	{
		GFile *file;

		if ( uris  ) file = g_file_new_for_uri ( uris[i] );
		if ( files ) file = files[i];

		gchar *name_file = g_file_get_path ( file );

			if ( name_file )
			{
				if ( g_file_test ( name_file, G_FILE_TEST_EXISTS ) )
					helia_treeview_pl_file ( name_file, tree_view );
				else
					g_warning ( "helia_treeview_pl_files:: file not exists %s \n", name_file );
			}

		g_free ( name_file );

		if ( uris ) g_object_unref ( file );
	}	
}

void helia_treeview_pl_open_dir ( GtkButton *button, GtkWidget *tree_view )
{
	hbgv.play = TRUE;

	gchar *path = helia_service_open_dir ( g_get_home_dir () );
	
		if ( path == NULL ) return;

		helia_treeview_pl_dir ( GTK_TREE_VIEW ( tree_view ), path );

	g_free ( path );

	if ( hbgv.all_info )
		g_debug ( "helia_treeview_pl_open_dir:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

void helia_treeview_pl_open_files ( GtkButton *button, GtkWidget *tree_view )
{
	hbgv.play = TRUE;

	GSList *files = helia_service_open_files ( g_get_home_dir (), "All", "*" );
	
	if ( files == NULL ) return;

	while ( files != NULL )
	{
		gchar *name_file = g_strdup ( files->data );

		if ( !helia_treeview_pl_check_m3u ( GTK_TREE_VIEW ( tree_view ), name_file ) )
		{
			gchar *name = g_path_get_basename ( name_file );
				helia_treeview_pl_add ( GTK_TREE_VIEW ( tree_view ), name, name_file );
			g_free ( name );
		}
		
		g_free ( name_file );

		files = files->next;
	}

	g_slist_free ( files );

	if ( hbgv.all_info )
		g_debug ( "helia_treeview_pl_open_files:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}
