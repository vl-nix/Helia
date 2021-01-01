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

#define CONTROL_TYPE_TV control_tv_get_type ()

G_DECLARE_FINAL_TYPE ( ControlTv, control_tv, CONTROL, TV, GtkWindow )

ControlTv * control_tv_new ( uint16_t, uint16_t, double, GtkWindow * );

