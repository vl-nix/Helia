/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/

#ifndef GMP_GST_PL_H
#define GMP_GST_PL_H


#include <gtk/gtk.h>
#include <gst/gst.h>


gboolean gmp_player_gst_create_pl ();

void gmp_player_stop_set_play ( gchar *name_file, guintptr win_handle );

gboolean gmp_player_playback_start  ();
gboolean gmp_player_playback_pause  ();
gboolean gmp_player_playback_stop   ();
gboolean gmp_player_playback_record ();
void gmp_player_base_quit ( gboolean base_quit );

gboolean gmp_player_draw_logo ();

gboolean gmp_player_ega_pl ();
gboolean gmp_player_egv_pl ();
void gmp_player_volume_changed_pl ( gdouble value );
void gmp_player_mute_set_pl ( GtkWidget *widget, GtkWidget *widget_v );
gboolean gmp_player_mute_get_pl ();
gdouble gmp_player_get_volume_pl ();
void gmp_player_set_suburi ( gchar *name_file );

gboolean gmp_player_scroll ( GdkEventScroll *evscroll );
void gmp_player_slider_changed ( gdouble value );

gboolean gmp_player_get_current_state_playing ();
gboolean gmp_player_get_current_state_paused ();
gboolean gmp_player_get_current_state_stop ();

gint64 gmp_player_query_duration ();
gint64 gmp_player_query_position ();

void gmp_player_audio_text_buttons ( GtkBox *hbox );


#endif // GMP_GST_PL_H
