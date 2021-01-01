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
#include <gst/gst.h>

#define CONTROL_TYPE_MP control_mp_get_type ()

G_DECLARE_FINAL_TYPE ( ControlMp, control_mp, CONTROL, MP, GtkWindow )

ControlMp * control_mp_new ( uint16_t, uint16_t, double, gboolean, GtkWindow * );

