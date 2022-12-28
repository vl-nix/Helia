/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "treeview.h"

#include <stdlib.h>

enum cols_n
{
	COL_NUM,
	COL_FLCH,
	COL_DATA,
	NUM_COLS
};

typedef void ( *fpt ) ( GtkButton *, GtkTreeView * );

static void add_filter ( GtkFileChooserDialog *dialog, const char *name, const char *filter_set )
{
	GtkFileFilter *filter = gtk_file_filter_new ();

	gtk_file_filter_set_name ( filter, name );
	gtk_file_filter_add_pattern ( filter, filter_set );

	gtk_file_chooser_add_filter ( GTK_FILE_CHOOSER ( dialog ), filter );
}

static char * save_dialog ( const char *dir, const char *file, const char *name_filter, const char *filter_set, GtkWindow *window )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new ( " ", window, GTK_FILE_CHOOSER_ACTION_SAVE, "gtk-cancel", GTK_RESPONSE_CANCEL, "gtk-save", GTK_RESPONSE_ACCEPT, NULL );

	add_filter ( dialog, name_filter, filter_set );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "document-save" );
	gtk_widget_set_opacity   ( GTK_WIDGET ( dialog ), gtk_widget_get_opacity ( GTK_WIDGET ( window ) ) );

	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), dir );
	gtk_file_chooser_set_do_overwrite_confirmation ( GTK_FILE_CHOOSER ( dialog ), TRUE );
	gtk_file_chooser_set_current_name   ( GTK_FILE_CHOOSER ( dialog ), file );

	char *filename = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT ) filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return filename;
}

static void treeview_reread ( GtkTreeView *tree_view )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	int row_count = 1;
	gboolean valid;

	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
		  valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter, COL_NUM, row_count++, -1 );
	}
}

static void treeview_up_down ( gboolean up_dw, GtkTreeView *tree_view )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );
	if ( ind < 2 ) return;

	if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter ) )
	{
		GtkTreeIter *iter_c = gtk_tree_iter_copy ( &iter );

		if ( up_dw )
		if ( gtk_tree_model_iter_previous ( model, &iter ) )
			gtk_list_store_move_before ( GTK_LIST_STORE ( model ), iter_c, &iter );

		if ( !up_dw )
		if ( gtk_tree_model_iter_next ( model, &iter ) )
			gtk_list_store_move_after ( GTK_LIST_STORE ( model ), iter_c, &iter );

		gtk_tree_iter_free ( iter_c );

		treeview_reread ( tree_view );
	}
}

static void treeview_remove ( G_GNUC_UNUSED GtkButton *button, GtkTreeView *tree_view )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter ) )
	{
		gtk_list_store_remove ( GTK_LIST_STORE ( model ), &iter );

		treeview_reread ( tree_view );
	}
}

static void treeview_goup ( G_GNUC_UNUSED GtkButton *button, GtkTreeView *tree_view )
{
	treeview_up_down ( TRUE, tree_view );
}

static void treeview_down ( G_GNUC_UNUSED GtkButton *button, GtkTreeView *tree_view )
{
	treeview_up_down ( FALSE, tree_view );
}

static void treeview_clear ( G_GNUC_UNUSED GtkButton *button, GtkTreeView *tree_view )
{
	gtk_list_store_clear ( GTK_LIST_STORE ( gtk_tree_view_get_model ( tree_view ) ) );
}

static void treeview_save ( const char *file, GtkTreeView *tree_view )
{
	GString *gstring = g_string_new ( "# Gtv-Dvb channel format \n" );

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind == 0 ) return;

	gboolean valid;
	for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid; valid = gtk_tree_model_iter_next ( model, &iter ) )
	{
		char *data = NULL;

		gtk_tree_model_get ( model, &iter, 2, &data, -1 );

		g_string_append_printf ( gstring, "%s\n", data );

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

static void treeview_save_dvb ( GtkButton *button, GtkTreeView *treeview )
{
	GtkTreeModel *model = gtk_tree_view_get_model ( treeview );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );

	if ( ind == 0 ) return;

	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( button ) ) );

	g_autofree char *path = save_dialog ( g_get_home_dir (), "gtv-channel.conf", "conf", "*.conf", window );

	if ( path == NULL ) return;

	treeview_save ( path, treeview );
}

struct _TreeDvb
{
	GtkBox parent_instance;

	GtkTreeView *treeview;
};

G_DEFINE_TYPE ( TreeDvb, treedvb, GTK_TYPE_BOX )

static void treeview_append_dvb ( const char *name, const char *data, GtkTreeView *treeview )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( treeview );

	int ind = gtk_tree_model_iter_n_children ( model, NULL );

	gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter );
	gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter, COL_NUM, ind+1, COL_FLCH, name, COL_DATA, data, -1 );
}

static void treeview_add_channels_dvb ( const char *file, GtkTreeView *treeview )
{
	char  *contents = NULL;
	GError *err     = NULL;

	if ( g_file_get_contents ( file, &contents, 0, &err ) )
	{
		char **lines = g_strsplit ( contents, "\n", 0 );

		uint i = 0; for ( i = 0; lines[i] != NULL; i++ )
		{
			if ( g_str_has_prefix ( lines[i], "#" ) || strlen ( lines[i] ) < 2 ) continue;

			char **data = g_strsplit ( lines[i], ":", 0 );

			treeview_append_dvb ( data[0], lines[i], treeview );

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

static void treeview_row_activated_dvb ( GtkTreeView *tree_view, GtkTreePath *path, G_GNUC_UNUSED GtkTreeViewColumn *column, TreeDvb *treedvb )
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

	if ( gtk_tree_model_get_iter ( model, &iter, path ) )
	{
		g_autofree char *data = NULL;
		gtk_tree_model_get ( model, &iter, COL_DATA, &data, -1 );

		g_signal_emit_by_name ( treedvb, "treeview-dvb-play", data );

		g_debug ( "%s: %s", __func__, data );
	}
}

static gboolean treeview_row_press_event_dvb ( GtkTreeView *tree_view, GdkEventButton *event, TreeDvb *treedvb )
{
	if ( event->button == GDK_BUTTON_SECONDARY )
	{
		GtkTreeIter iter;
		GtkTreeModel *model = gtk_tree_view_get_model ( tree_view );

		if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter ) )
		{
			g_autofree char *data = NULL;
			gtk_tree_model_get ( model, &iter, COL_DATA, &data, -1 );

			g_signal_emit_by_name ( treedvb, "treeview-dvb-multi", data );
		}
	}

	return GDK_EVENT_PROPAGATE;
}

static GtkBox * treeview_create_box_dvb ( TreeDvb *treedvb )
{
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 5 );

	const char *icons[] = { "helia-up", "helia-down", "helia-remove", "helia-clear", "helia-save" };
	fpt funcs[] = { treeview_goup, treeview_down, treeview_remove, treeview_clear, treeview_save_dvb };

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( icons ); c++ )
	{
		GtkButton *button = (GtkButton *)gtk_button_new_from_icon_name ( icons[c], GTK_ICON_SIZE_MENU );
		gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
		g_signal_connect ( button, "clicked", G_CALLBACK ( funcs[c] ), treedvb->treeview );

		gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	}

	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	return h_box;
}

static void treeview_set_dvb ( Level *level, TreeDvb *treedvb )
{
	gtk_widget_set_visible ( GTK_WIDGET ( level ), TRUE );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( level ), 5 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( level ), 5 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( level ), 5 );

	gtk_box_pack_start ( GTK_BOX ( treedvb ), GTK_WIDGET ( level ), FALSE, FALSE, 0 );

	GtkBox *vbox = treeview_create_box_dvb ( treedvb );

	gtk_widget_set_margin_end    ( GTK_WIDGET ( vbox ), 5 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( vbox ), 5 );

	gtk_box_pack_start ( GTK_BOX ( treedvb ), GTK_WIDGET ( vbox ), FALSE, FALSE, 0 );

	char path[PATH_MAX];
	sprintf ( path, "%s/helia/gtv-channel.conf", g_get_user_config_dir () );

	treeview_add_channels_dvb ( path, treedvb->treeview );
}

static void treedvb_handler_add ( TreeDvb *treedvb, const char *data )
{
	treeview_add_channels_dvb ( data, treedvb->treeview );
}

static void treedvb_handler_append ( TreeDvb *treedvb, const char *name, const char *data )
{
	treeview_append_dvb ( name, data, treedvb->treeview );
}

static char * treedvb_handler_get ( TreeDvb *treedvb )
{
	char *data = NULL;

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( treedvb->treeview );

	if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( treedvb->treeview ), NULL, &iter ) )
	{
		gtk_tree_model_get ( model, &iter, COL_DATA, &data, -1 );
	}

	return data;
}

static void treedvb_save ( GtkTreeView *tree_view )
{
	char path[PATH_MAX];
	sprintf ( path, "%s/helia/gtv-channel.conf", g_get_user_config_dir () );

	treeview_save ( path, tree_view );
}

static void treedvb_destroy ( TreeDvb *treedvb )
{
	treedvb_save ( treedvb->treeview );
}

static void treedvb_init ( TreeDvb *treedvb )
{
	GtkBox *v_box = GTK_BOX ( treedvb );
	gtk_orientable_set_orientation ( GTK_ORIENTABLE ( v_box ), GTK_ORIENTATION_VERTICAL );

	gtk_box_set_spacing ( v_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( v_box ), TRUE );

	GtkScrolledWindow *sw = (GtkScrolledWindow *)gtk_scrolled_window_new ( NULL, NULL );
	gtk_scrolled_window_set_policy ( sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	gtk_widget_set_size_request ( GTK_WIDGET ( sw ), 220, -1 );
	gtk_widget_set_visible ( GTK_WIDGET ( sw ), TRUE );

	const char *title[3] = { "Num", "Channel", "Data" };

	GtkListStore *store = (GtkListStore *)gtk_list_store_new ( NUM_COLS, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING );
	treedvb->treeview = (GtkTreeView *)gtk_tree_view_new_with_model ( GTK_TREE_MODEL ( store ) );
	gtk_tree_view_set_search_column ( treedvb->treeview, COL_FLCH );

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( title ); c++ )
	{
		renderer = gtk_cell_renderer_text_new ();

		column = gtk_tree_view_column_new_with_attributes ( title[c], renderer, "text", c, NULL );

		if ( c == COL_DATA ) gtk_tree_view_column_set_visible ( column, FALSE );

		gtk_tree_view_append_column ( treedvb->treeview, column );
	}

	g_object_unref ( G_OBJECT (store) );

	gtk_widget_set_visible ( GTK_WIDGET ( treedvb->treeview ), TRUE );
	g_signal_connect ( treedvb->treeview, "row-activated", G_CALLBACK ( treeview_row_activated_dvb ), treedvb );
	g_signal_connect ( treedvb->treeview, "button-press-event", G_CALLBACK ( treeview_row_press_event_dvb ), treedvb );

	gtk_container_add ( GTK_CONTAINER ( sw ), GTK_WIDGET ( treedvb->treeview ) );
	gtk_box_pack_start ( v_box, GTK_WIDGET ( sw ), TRUE, TRUE, 0 );

	g_signal_connect ( treedvb, "destroy",          G_CALLBACK ( treedvb_destroy     ), NULL );
	g_signal_connect ( treedvb, "treeview-dvb-add", G_CALLBACK ( treedvb_handler_add ), NULL );
	g_signal_connect ( treedvb, "treeview-dvb-get", G_CALLBACK ( treedvb_handler_get ), NULL );
	g_signal_connect ( treedvb, "treeview-dvb-append", G_CALLBACK ( treedvb_handler_append ), NULL );
}

static void treedvb_finalize ( GObject *object )
{
	G_OBJECT_CLASS ( treedvb_parent_class )->finalize ( object );
}

static void treedvb_class_init ( TreeDvbClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = treedvb_finalize;

	g_signal_new ( "treeview-dvb-get",  G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_STRING, 0 );
	g_signal_new ( "treeview-dvb-play", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );
	g_signal_new ( "treeview-dvb-add",  G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );
	g_signal_new ( "treeview-dvb-append", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING );

	g_signal_new ( "treeview-dvb-multi", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING );
}

TreeDvb * treedvb_new ( Level *level )
{
	TreeDvb *treedvb = g_object_new ( TREEDVB_TYPE_BOX, NULL );

	treeview_set_dvb ( level, treedvb );

	return treedvb;
}
