/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#pragma once

#include "helia-app.h"

#define HELIA_TYPE_WIN helia_win_get_type ()

G_DECLARE_FINAL_TYPE ( HeliaWin, helia_win, HELIA, WIN, GtkWindow )

HeliaWin * helia_win_new ( HeliaApp * );
