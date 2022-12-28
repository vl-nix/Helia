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

#define PREF_TYPE_POPOVER pref_get_type ()

G_DECLARE_FINAL_TYPE ( Pref, pref, PREF, POPOVER, GtkPopover )

Pref * pref_new ( void );
