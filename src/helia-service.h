/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef HELIA_SERVICE_H
#define HELIA_SERVICE_H


#include "helia-base.h"


GtkButton * helia_service_ret_image_button ( const gchar *icon, guint size );
void helia_service_create_image_button ( GtkBox *box, const gchar *icon, guint size, void (* activate)(), GtkWidget *widget );

void helia_service_message_dialog ( gchar *f_error, gchar *file_or_info, GtkMessageType mesg_type );
gchar * helia_service_get_time_date_str ();

gchar * helia_service_open_dir    ( const gchar *path );
gchar * helia_service_open_file   ( const gchar *path, const gchar *filter_name, const gchar *filter_file );
GSList * helia_service_open_files ( const gchar *path, const gchar *filter_name, const gchar *filter_file );
void helia_service_add_filter     ( GtkFileChooserDialog *dialog, const gchar *name, const gchar *filter  );


#endif // HELIA_SERVICE_H
