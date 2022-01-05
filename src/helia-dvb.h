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

#define HELIA_TYPE_DVB                     helia_dvb_get_type ()

G_DECLARE_FINAL_TYPE ( HeliaDvb, helia_dvb, HELIA, DVB, GtkBox )

HeliaDvb * helia_dvb_new ( void );

void helia_dvb_start ( const char *, HeliaDvb * );

