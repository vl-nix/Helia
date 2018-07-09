/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef HELIA_MEDIA_PL_H
#define HELIA_MEDIA_PL_H


#include "helia-base.h"


typedef struct _HMPlSlider HMPlSlider;

struct _HMPlSlider
{
	GtkScale *slider;
	GtkLabel *lab_pos, *lab_dur;

	gulong slider_signal_id;
};


void helia_media_pl_win ( GtkBox *box, GdkPixbuf *logo, HBTrwCol sw_col_n[], guint num );

gulong helia_media_pl_handle_ret ();
void helia_media_pl_stop ();
void helia_media_pl_add_arg ( GFile **files, gint n_files );

HMPlSlider helia_media_pl_create_slider ( void (* activate)() );

void helia_media_pl_pos_dur_text    ( gint64 pos, gint64 dur, guint digits );
void helia_media_pl_pos_dur_sliders ( gint64 pos, gint64 dur, guint digits );
void helia_media_pl_set_sensitive ( gboolean sensitive );

void helia_media_pl_next ();
void helia_media_pl_set_path    ( GtkTreeModel *model, GtkTreeIter iter );
void helia_media_pl_set_spinner ( gchar *text, gboolean spinner_stop    );

void helia_media_pl_edit_hs ();
void helia_media_pl_slider_hs ();


#endif // HELIA_MEDIA_PL_H
