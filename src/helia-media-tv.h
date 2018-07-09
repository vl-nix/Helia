/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef HELIA_MEDIA_TV_H
#define HELIA_MEDIA_TV_H


#include "helia-base.h"


void helia_media_tv_win ( GtkBox *box, GdkPixbuf *logo, HBTrwCol sw_col_n[], guint num );

void helia_media_tv_stop ();
void helia_media_tv_scan_save ( gchar *name, gchar *data );
void helia_media_tv_set_sgn_snr ( GstElement *element, gdouble sgl, gdouble snr, gboolean hlook, gboolean rec_ses );

void helia_media_tv_edit_hs ();


#endif // HELIA_MEDIA_TV_H
