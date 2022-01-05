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

void helia_info_player ( GtkWindow *, GtkTreeView *, GstElement * );

GtkComboBoxText * helia_info_dvb ( const char *, GtkWindow *, GstElement * );
