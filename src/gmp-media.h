/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/


#ifndef GMP_MEDIA_H
#define GMP_MEDIA_H


#include <gtk/gtk.h>
#include <gst/gst.h>


enum COLS 
{ 
	COL_NUM, 
	COL_TITLE, 
	COL_DATA, 
	NUM_COLS
};

struct trw_columns 
{
	const gchar *title;
	gboolean visible; 
};

void gmp_media_win ( GtkBox *box, GdkPixbuf *logo, gboolean set_tv_pl, struct trw_columns sw_col_n[], guint num );
void gmp_media_auto_save ();

void gmp_set_sgn_snr ( GstElement *element, gdouble sgl, gdouble snr, gboolean hlook, gboolean rec_ses );
void gmp_str_split_ch_data ( gchar *data );
void gmp_media_add_arg ( GSList *lists_argv );
void gmp_media_update_box_pl ();

void gmp_media_set_tv   ();
void gmp_media_set_mp   ();
void gmp_media_stop_all ();
void gmp_media_next_pl  ();
gboolean gmp_checked_filter ( gchar *file_name );
void gmp_media_set_sensitive_pl ( gboolean sensitive_trc, gboolean sensitive_vol );

void gmp_pos_dur_text ( gint64 pos, gint64 dur );
void gmp_pos_text ( gint64 pos );
void gmp_pos_dur_sliders ( gint64 current, gint64 duration );
void gmp_spinner_label_set ( gchar *text, gboolean spinner_stop );


#endif // GMP_MEDIA_H
