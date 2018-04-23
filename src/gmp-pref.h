/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/


#ifndef GMP_PREF_H
#define GMP_PREF_H


#include <gtk/gtk.h>
#include <gst/gst.h>


void gmp_pref_win ( GtkWindow *window );

void gmp_pref_read_config ( const gchar *file );
void gmp_pref_save_config ( const gchar *file );

void gmp_message_dialog ( gchar *f_error, gchar *file_or_info, GtkMessageType mesg_type );
gchar * gmp_pref_get_time_date_str ();
gchar * gmp_pref_open_file ( const gchar *path, const gchar *filter_name, const gchar *filter_file );
void gmp_add_filter ( GtkFileChooserDialog *dialog, const gchar *name, const gchar *filter );

const gchar *gmp_dvb_conf, *ch_conf;
gchar *gmp_drag_cvr, *gmp_rec_dir;
gdouble opacity_cnt, opacity_eq, opacity_win;
guint resize_icon;


#endif // GMP_PREF_H
