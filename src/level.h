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

#define LEVEL_TYPE_DVB level_get_type ()

G_DECLARE_FINAL_TYPE ( Level, level, LEVEL, DVB, GtkBox )

Level * level_new ( void );

void level_update ( uint8_t, uint8_t, gboolean, gboolean, uint, uint, Level * );
