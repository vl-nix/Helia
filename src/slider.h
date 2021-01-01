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

#define SLIDER_TYPE_PLAYER slider_get_type ()

G_DECLARE_FINAL_TYPE ( Slider, slider, SLIDER, PLAYER, GtkBox )

Slider * slider_new ( void );

void slider_update ( gint64, uint, gint64, uint, gboolean, Slider * );
