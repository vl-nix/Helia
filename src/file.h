/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#pragma once

#include <gtk/gtk.h>

/* Returns a newly-allocated string holding the result. Free with free() */
char * helia_time_to_str ( void );

/* Returns a newly-allocated string holding the result. Free with free() */
char * helia_uri_get_path ( const char * );

/* Returns a newly-allocated string holding the result. Free with free() */
char * helia_open_dir ( const char *, GtkWindow * );

/* Returns a newly-allocated string holding the result. Free with free() */
char * helia_open_file ( const char *, GtkWindow * );

/* Returns a GSList containing the filenames. Free the returned list with g_slist_free(), and the filenames with free(). */
GSList * helia_open_files ( const char *, GtkWindow * );

/* Returns a newly-allocated string holding the result. Free with free() */
char * helia_save_file ( const char *, const char *, const char *, const char *, GtkWindow * );

void helia_message_dialog ( const char *, const char *, GtkMessageType, GtkWindow * );

