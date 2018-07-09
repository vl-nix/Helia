/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef HELIA_TREEVIEW_PL_H
#define HELIA_MEDIA_PL_H


#include "helia-base.h"


// Files to treeview
void helia_treeview_pl_files ( GFile **files, gchar **uris, gint n_files, GtkTreeView *tree_view );

void helia_treeview_pl_open_dir   ( GtkButton *button, GtkWidget *tree_view );
void helia_treeview_pl_open_files ( GtkButton *button, GtkWidget *tree_view );


#endif // HELIA_MEDIA_PL_H
