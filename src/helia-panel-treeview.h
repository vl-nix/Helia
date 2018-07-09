/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef HELIA_PANEL_TREEVIEW_H
#define HELIA_PANEL_TREEVIEW_H


#include <gtk/gtk.h>


void helia_panel_treeview ( GtkTreeView *tree_view, GtkWindow *parent, gdouble opacity, gboolean media_tv_pl, guint resize_icon );

void helia_treeview_to_file ( GtkTreeView *tree_view, gchar *filename, gboolean tv_pl );

void helia_treeview_save_pl ( GtkButton *button, GtkWidget *tree_view );
void helia_treeview_save_tv ( GtkButton *button, GtkWidget *tree_view );


#endif // HELIA_PANEL_TREEVIEW_H
