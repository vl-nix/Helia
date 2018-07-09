/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef HELIA_SCAN_H
#define HELIA_SCAN_H


#include <gtk/gtk.h>
#include <gst/gst.h>


void helia_scan_win ();

void helia_scan_gst_create ();

void helia_set_lnb_low_high_switch ( GstElement *element, gint type_lnb );

void helia_get_dvb_info ( GtkLabel *label_dvb_name, guint adapter, guint frontend );
GtkBox * helia_scan_device ( void (* activate_a)(), void (* activate_f)() );

void helia_scan_stop ();

void helia_convert_dvb5 ( gchar *file, guint set_adapter, guint set_frontend );


#endif // HELIA_SCAN_H
