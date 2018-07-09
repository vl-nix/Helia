/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef HELIA_GST_TV_H
#define HELIA_GST_TV_H


#include <gtk/gtk.h>
#include <gst/gst.h>


#define MAX_AUDIO 32


typedef struct _HeliaGstTvState HGTvState;

struct _HeliaGstTvState
{
	gdouble volume_tv;
	guint count_audio_track, set_audio_track;
	gboolean state_mute, state_playing, state_stop;
};

HGTvState helia_gst_tv_state_all ();


gboolean helia_gst_tv_gst_create ();
gboolean helia_gst_tv_draw_logo  ();

void helia_gst_tv_play   ( gchar *data, guintptr win_handle );
void helia_gst_tv_stop   ();
void helia_gst_tv_record ();

gboolean helia_gst_tv_get_state_play ();
gboolean helia_gst_tv_get_state_stop ();

gboolean helia_gst_tv_ega ();
gboolean helia_gst_tv_egv ();

void helia_gst_tv_info    ();

void helia_gst_tv_mute_set ();
void helia_gst_tv_mute_set_widget ( GtkWidget *widget );
void helia_gst_tv_volume_changed ( GtkScaleButton *button, gdouble value );

void helia_gst_tv_set_text_audio ( GtkButton *button, gboolean text, gboolean create_text, gboolean audio, gboolean create_audio );


#endif // HELIA_GST_TV_H
