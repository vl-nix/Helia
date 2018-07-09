/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef HELIA_PREF_H
#define HELIA_PREF_H


#include <gtk/gtk.h>
#include <gst/gst.h>


void helia_pref_win ( GtkWindow *window );

void helia_pref_read_config ( const gchar *file );
void helia_pref_save_config ( const gchar *file );


#endif // HELIA_PREF_H
