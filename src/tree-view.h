/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#pragma once

#include <gtk/gtk.h>

enum cols_n
{
	COL_NUM,
	COL_FLCH,
	COL_DATA,
	NUM_COLS
};

typedef struct _Column Column;

struct _Column
{
	const char *name;
	const char *type;
	uint8_t num;
};

#define TREE_TYPE_VIEW                          tree_view_get_type ()

G_DECLARE_FINAL_TYPE ( TreeView, tree_view, TREE, VIEW, GtkTreeView )

TreeView * tree_view_new ( const char *title[] );

GtkBox * create_treeview_box ( gboolean, TreeView * );


