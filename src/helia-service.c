/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia-service.h"

#include <glib/gi18n.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


GtkButton * helia_service_ret_image_button ( const gchar *icon, guint size )
{
	g_debug ( "helia_service_ret_image_button:: %s", icon );
	
	GdkPixbuf *logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					  icon, size, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	GtkButton *button = (GtkButton *)gtk_button_new ();

	GtkImage *image   = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );
	gtk_button_set_image ( button, GTK_WIDGET ( image ) );

	return button;
}

void helia_service_create_image_button ( GtkBox *box, const gchar *icon, guint size, void (* activate)(), GtkWidget *widget )
{
	GtkButton *button = helia_service_ret_image_button ( icon, size );

	if ( activate ) g_signal_connect ( button, "clicked", G_CALLBACK (activate), widget );

	if ( box ) gtk_box_pack_start ( box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );

	g_debug ( "helia_service_create_image_button:: %s", icon );
}

void helia_service_message_dialog ( gchar *f_error, gchar *file_or_info, GtkMessageType mesg_type )
{
	g_debug ( "helia_service_message_dialog" );

	GtkMessageDialog *dialog = ( GtkMessageDialog *)gtk_message_dialog_new (
								helia_base_win_ret (), GTK_DIALOG_MODAL,
								mesg_type,   GTK_BUTTONS_CLOSE,
								"%s\n %s",   f_error, file_or_info );

	gtk_dialog_run     ( GTK_DIALOG ( dialog ) );
	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

gchar * helia_service_get_time_date_str ()
{
	GDateTime *datetime = g_date_time_new_now_local ();

	gint doy = g_date_time_get_day_of_year ( datetime );
	gint tth = g_date_time_get_hour   ( datetime );
	gint ttm = g_date_time_get_minute ( datetime );
	gint tts = g_date_time_get_second ( datetime );

	gchar *dt = g_strdup_printf ( "%d-%d-%d-%d", doy, tth, ttm, tts );

	g_debug ( "helia_service_get_time_date_str:: dt %s", dt );

	g_date_time_unref ( datetime );

	return dt;
}

void helia_service_add_filter ( GtkFileChooserDialog *dialog, const gchar *name, const gchar *filter )
{
	GtkFileFilter *filter_video = gtk_file_filter_new ();
	gtk_file_filter_set_name ( filter_video, name );
	gtk_file_filter_add_pattern ( filter_video, filter );
	gtk_file_chooser_add_filter ( GTK_FILE_CHOOSER ( dialog ), filter_video );
}

gchar * helia_service_open_file ( const gchar *path, const gchar *filter_name, const gchar *filter_file )
{
	g_debug ( "helia_service_open_file" );

	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
									_("Choose file"),  helia_base_win_ret (), GTK_FILE_CHOOSER_ACTION_OPEN,
									"gtk-cancel", GTK_RESPONSE_CANCEL,
									"gtk-open",   GTK_RESPONSE_ACCEPT,
									NULL );

	if ( filter_name ) helia_service_add_filter ( dialog, filter_name, filter_file );

	gtk_file_chooser_set_current_folder  ( GTK_FILE_CHOOSER ( dialog ), path );
	gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ), FALSE );

	gchar *filename = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return filename;
}

GSList * helia_service_open_files ( const gchar *path, const gchar *filter_name, const gchar *filter_file )
{
	g_debug ( "helia_service_open_files" );

	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
									_("Choose files"),  helia_base_win_ret (), GTK_FILE_CHOOSER_ACTION_OPEN,
									"gtk-cancel", GTK_RESPONSE_CANCEL,
									"gtk-open",   GTK_RESPONSE_ACCEPT,
									NULL );

	if ( filter_name ) helia_service_add_filter ( dialog, filter_name, filter_file );

	gtk_file_chooser_set_current_folder  ( GTK_FILE_CHOOSER ( dialog ), path );
	gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ), TRUE );

	GSList *list = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		list = gtk_file_chooser_get_filenames ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return list;
}

gchar * helia_service_open_dir ( const gchar *path )
{
	g_debug ( "helia_service_open_dir" );

	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
									_("Choose folder"),  helia_base_win_ret (), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
									"gtk-cancel", GTK_RESPONSE_CANCEL,
									"gtk-apply",  GTK_RESPONSE_ACCEPT,
									NULL );

	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), path );

	gchar *dirname = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		dirname = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return dirname;
}
