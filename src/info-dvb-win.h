/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#pragma once

#include <gtk/gtk.h>

typedef unsigned int uint;

#define INFODVB_TYPE_WIN info_dvb_win_get_type ()

G_DECLARE_FINAL_TYPE ( InfoDvbWin, info_dvb_win, INFODVB, WIN, GtkWindow )

InfoDvbWin * info_dvb_win_new ( uint, const char *, GObject *, GtkWindow * );
