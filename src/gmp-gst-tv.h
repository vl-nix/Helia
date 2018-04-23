/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/

#ifndef GMP_GST_TV_H
#define GMP_GST_TV_H


#include <gtk/gtk.h>
#include <gst/gst.h>


#define MAX_AUDIO 32

gboolean gmp_dvb_gst_create_tv ();

void gmp_dvb_play ( gchar *data, guintptr win_handle );

void gmp_dvb_record ();
void gmp_dvb_stop   ();

gboolean gmp_dvb_draw_logo ();

gboolean gmp_dvb_ega_tv ();
gboolean gmp_dvb_egv_tv ();
void gmp_dvb_volume_changed_tv ( gdouble value );
void gmp_dvb_mute_set_tv ( GtkWidget *widget );
gboolean gmp_dvb_mute_get_tv ();
gdouble gmp_dvb_get_volume_tv ();

void gmp_dvb_audio_button_tv ( GtkBox *hbox );


#endif // GMP_GST_TV_H
