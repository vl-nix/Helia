/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef HELIA_PANEL_PL_H
#define HELIA_PANEL_PL_H


#include <gtk/gtk.h>


void helia_panel_pl ( GtkBox *vbox, GtkBox *hbox_sl, GtkScale *slider, GtkWindow *parent, gdouble opacity, guint resize_icon );

void helia_panel_pl_pos_dur_text ( gint64 pos, gint64 dur, guint digits );
void helia_panel_pl_pos_dur_slider ( gint64 current, gint64 duration, guint digits );
void helia_panel_pl_set_sensitive ( gboolean sensitive );


#endif // HELIA_PANEL_PL_H
