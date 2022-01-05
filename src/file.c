/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "file.h"
#include "default.h"

/* Returns a newly-allocated string holding the result. Free with free() */
char * helia_time_to_str ( void )
{
	GDateTime *date = g_date_time_new_now_local ();

	char *str_time = g_date_time_format ( date, "%j-%Y-%H%M%S" );

	g_date_time_unref ( date );

	return str_time;
}

/* Returns a newly-allocated string holding the result. Free with free() */
char * helia_uri_get_path ( const char *uri )
{
	GFile *file = g_file_new_for_uri ( uri );

	char *path = g_file_get_path ( file );

	g_object_unref ( file );

	return path;
}

void helia_message_dialog ( const char *f_error, const char *file_or_info, GtkMessageType mesg_type, GtkWindow *window )
{
	GtkMessageDialog *dialog = ( GtkMessageDialog *)gtk_message_dialog_new (
					window,    GTK_DIALOG_MODAL,
					mesg_type, GTK_BUTTONS_CLOSE,
					"%s\n%s",  f_error, file_or_info );

	gtk_window_set_icon_name ( GTK_WINDOW (dialog), DEF_ICON );

	gtk_dialog_run     ( GTK_DIALOG ( dialog ) );
	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

static void helia_dialod_add_filter ( GtkFileChooserDialog *dialog, const char *name, const char *filter_set )
{
	GtkFileFilter *filter = gtk_file_filter_new ();

	gtk_file_filter_set_name ( filter, name );

	if ( filter_set )
		gtk_file_filter_add_pattern ( filter, filter_set );
	else
	{
		GList *list = g_content_types_get_registered ();

		while ( list != NULL )
		{
			char *type = (char *)list->data;

			if ( g_str_has_prefix ( type, "audio" ) || g_str_has_prefix ( type, "video" ) )
				gtk_file_filter_add_mime_type ( filter, type );

			list = list->next;
		}

		g_list_free_full ( list, (GDestroyNotify) g_free );
	}

	gtk_file_chooser_add_filter ( GTK_FILE_CHOOSER ( dialog ), filter );
}

/* Returns a newly-allocated string holding the result. Free with free() */
char * helia_open_file ( const char *path, GtkWindow *window )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
					" ",  window, GTK_FILE_CHOOSER_ACTION_OPEN,
					"gtk-cancel", GTK_RESPONSE_CANCEL,
					"gtk-open",   GTK_RESPONSE_ACCEPT,
					NULL );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "document-open" );
	gtk_widget_set_opacity   ( GTK_WIDGET ( dialog ), gtk_widget_get_opacity ( GTK_WIDGET ( window ) ) );

	gtk_file_chooser_set_current_folder  ( GTK_FILE_CHOOSER ( dialog ), path );
	gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ), FALSE );

	char *filename = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return filename;
}

/* Returns a GSList containing the filenames. Free the returned list with g_slist_free(), and the filenames with g_free(). */
GSList * helia_open_files ( const char *path, GtkWindow *window )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
					" ",  window, GTK_FILE_CHOOSER_ACTION_OPEN,
					"gtk-cancel", GTK_RESPONSE_CANCEL,
					"gtk-open",   GTK_RESPONSE_ACCEPT,
					NULL );

	helia_dialod_add_filter ( dialog, "Media", NULL );
	helia_dialod_add_filter ( dialog, "All",  "*.*" );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "document-open" );
	gtk_widget_set_opacity   ( GTK_WIDGET ( dialog ), gtk_widget_get_opacity ( GTK_WIDGET ( window ) ) );

	gtk_file_chooser_set_current_folder  ( GTK_FILE_CHOOSER ( dialog ), path );
	gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ), TRUE );

	GSList *files = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		files = gtk_file_chooser_get_filenames ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return files;
}

/* Returns a newly-allocated string holding the result. Free with free() */
char * helia_open_dir ( const char *path, GtkWindow *window )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
					" ",  window, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					"gtk-cancel", GTK_RESPONSE_CANCEL,
					"gtk-apply",  GTK_RESPONSE_ACCEPT,
					NULL );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "folder-open" );
	gtk_widget_set_opacity   ( GTK_WIDGET ( dialog ), gtk_widget_get_opacity ( GTK_WIDGET ( window ) ) );

	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), path );

	char *dirname = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		dirname = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return dirname;
}

/* Returns a newly-allocated string holding the result. Free with free() */
char * helia_save_file ( const char *dir, const char *file, const char *name_filter, const char *filter_set, GtkWindow *window )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
					" ", window,   GTK_FILE_CHOOSER_ACTION_SAVE,
					"gtk-cancel",  GTK_RESPONSE_CANCEL,
					"gtk-save",    GTK_RESPONSE_ACCEPT,
					NULL );

	helia_dialod_add_filter ( dialog, name_filter, filter_set );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "document-save" );
	gtk_widget_set_opacity   ( GTK_WIDGET ( dialog ), gtk_widget_get_opacity ( GTK_WIDGET ( window ) ) );

	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), dir );
	gtk_file_chooser_set_do_overwrite_confirmation ( GTK_FILE_CHOOSER ( dialog ), TRUE );
	gtk_file_chooser_set_current_name   ( GTK_FILE_CHOOSER ( dialog ), file );

	char *filename = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return filename;
}

