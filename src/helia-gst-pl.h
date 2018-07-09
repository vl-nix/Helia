/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/

#ifndef HELIA_GST_PL_H
#define HELIA_GST_PL_H


#include <gtk/gtk.h>
#include <gst/gst.h>


typedef struct _HeliaGstPlState HGPlState;

struct _HeliaGstPlState
{
	gdouble volume_pl;
	gint cur_text, num_text;
	gint cur_audio, num_audio;
	gboolean state_mute, state_subtitle;
	gboolean state_playing, state_pause, state_stop;
};

HGPlState helia_gst_pl_state_all ();


gboolean helia_gst_pl_gst_create ();
gboolean helia_gst_pl_draw_logo  ();

void helia_gst_pl_stop_set_play ( gchar *name_file, guintptr win_handle );

gboolean helia_gst_pl_playback_play   ();
gboolean helia_gst_pl_playback_pause  ();
gboolean helia_gst_pl_playback_stop   ();
gboolean helia_gst_pl_playback_record ();

void helia_gst_pl_frame ();
void helia_gst_pl_info  ();

gboolean helia_gst_pl_get_current_state_playing ();
gboolean helia_gst_pl_get_current_state_paused  ();
gboolean helia_gst_pl_get_current_state_stop    ();

void helia_gst_pl_slider_changed ( gdouble value );

gboolean helia_gst_pl_ega ();
gboolean helia_gst_pl_egv ();

void helia_gst_pl_mute_set ();
void helia_gst_pl_mute_set_widget ( GtkWidget *widget );
void helia_gst_pl_volume_changed ( GtkScaleButton *button, gdouble value );

void helia_gst_pl_set_text_audio ( GtkButton *button, gboolean text, gboolean create_text, gboolean audio, gboolean create_audio );
void helia_gst_pl_playbin_change_state_subtitle ();


#endif // HELIA_GST_PL_H
