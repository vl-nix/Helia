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

#define ENCPROP_TYPE_WINDOW enc_prop_get_type ()

G_DECLARE_FINAL_TYPE ( EncProp, enc_prop, ENCPROP, WINDOW, GtkWindow )

EncProp * enc_prop_new (void);

void enc_prop_set_run ( GtkWindow *, GstElement *, GstElement *, GstElement *, GstElement *, gboolean, EncProp * );
