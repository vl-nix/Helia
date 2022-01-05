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

#define SCAN_TYPE_WIN                    scan_get_type ()

G_DECLARE_FINAL_TYPE ( Scan, scan, SCAN, WIN, GtkWindow )

Scan * scan_new ( GtkWindow *win_base );

void helia_init_dvb ( int, int );

void set_lnb_lhs ( GstElement *, int );

char * scan_get_dvb_info ( int , int );

const char * scan_get_info ( const char * );

const char * scan_get_info_descr_vis ( const char *, int );

