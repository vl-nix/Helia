/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef HELIA_TREEVIEW_TV_H
#define HELIA_TREEVIEW_TV_H


#include "helia-base.h"


// Channels to treeview
void helia_treeview_tv ( const gchar *file, GtkTreeView *tree_view );
void helia_treeview_tv_add ( GtkTreeView *tree_view, gchar *name, gchar *data );


#endif // HELIA_MEDIA_PL_H
