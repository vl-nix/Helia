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

GtkImage * helia_create_image ( const char *, uint16_t  );

gboolean helia_check_icon_theme ( const char * );

GtkButton * helia_create_button ( GtkBox *, const char *, const char *, uint16_t );
