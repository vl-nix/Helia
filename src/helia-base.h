/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#ifndef HELIA_BASE_H
#define HELIA_BASE_H


#include <gtk/gtk.h>
#include <gst/gst.h>


enum COLS
{
	COL_NUM,
	COL_TITLE,
	COL_DATA,
	NUM_COLS
};

typedef struct _HBTrwCol HBTrwCol;

struct _HBTrwCol
{
	const gchar *title;
	gboolean visible;
};

typedef struct _HBGlobalVal HBGlobalVal;

struct _HBGlobalVal
{
	const gchar *dir_conf, *helia_conf, *ch_conf;
	gchar *rec_dir;

	gdouble opacity_cnt, opacity_eq, opacity_win;
	guint resize_icon;

	gboolean play, sort_az, base_quit, all_info;
};

HBGlobalVal hbgv;


void helia_base_win ( GApplication *app, GFile **files, gint n_files );

void helia_base_set_window ();
void helia_base_update_win ();

GtkWindow * helia_base_win_ret ();
gboolean    helia_base_flscr   ();


#endif // HELIA_BASE_H
