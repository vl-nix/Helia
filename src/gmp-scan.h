/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/


#ifndef GMP_SCAN_H
#define GMP_SCAN_H


#include <gtk/gtk.h>
#include <gst/gst.h>


void gmp_scan_win ();

void gmp_convert_win ( gboolean set_file );

void gmp_scan_gst_create ();

void gmp_get_dvb_info ( gboolean set_label, guint adapter, guint frontend );

void gmp_set_lnb  ( GstElement *element, gint num_lnb );

void gmp_scan_stop_mpeg ();


#endif // GMP_SCAN_H
