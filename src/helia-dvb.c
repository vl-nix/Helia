/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "dvb.h"
#include "scan.h"
#include "level.h"
#include "treeview.h"
#include "helia-dvb.h"
#include "info-dvb-win.h"

#include <stdlib.h>

struct _HeliaDvb
{
	GtkBox parent_instance;

	Dvb *video;
	Level *level;
	TreeDvb *treedvb;

	Pref *pref;

	GtkBox *vbox_video;
	GtkBox *hbox_video_a;
	GtkBox *hbox_video_b;

	GtkButton *button[4];

	GtkWindow *win_base;

	uint8_t win_count;

	gboolean multi_destroy;
};

G_DEFINE_TYPE ( HeliaDvb, helia_dvb, GTK_TYPE_BOX )

typedef void ( *fpd ) ( GtkButton *, HeliaDvb * );

static void helia_dvb_multi_stop ( HeliaDvb * );

static void helia_dvb_pref ( GtkButton *button, HeliaDvb *dvb )
{
	gtk_popover_set_relative_to ( GTK_POPOVER ( dvb->pref ), GTK_WIDGET ( button ) );
	gtk_popover_popup ( GTK_POPOVER ( dvb->pref ) );
}

static void helia_dvb_treeview_scan_append_handler ( G_GNUC_UNUSED Scan *scan, const char *name, const char *data, HeliaDvb *dvb )
{
	g_signal_emit_by_name ( dvb->treedvb, "treeview-dvb-append", name, data );
}

static void helia_dvb_scan ( G_GNUC_UNUSED GtkButton *button, HeliaDvb *dvb )
{
	Scan *scan = scan_new ( dvb->win_base );
	g_signal_connect ( scan, "scan-append", G_CALLBACK ( helia_dvb_treeview_scan_append_handler ), dvb );
}

static void helia_dvb_info ( G_GNUC_UNUSED GtkButton *button, HeliaDvb *dvb )
{
	g_autofree char *data = NULL;
	g_signal_emit_by_name ( dvb->treedvb, "treeview-dvb-get", &data );

	gboolean play = FALSE;
	g_signal_emit_by_name ( dvb->video, "dvb-is-play", &play );

	if ( !play || !data ) { helia_dvb_scan ( NULL, dvb ); return; }

	GObject *obj;
	g_signal_emit_by_name ( dvb->video, "dvb-combo-lang", &obj );

	uint sid = 0;
	g_signal_emit_by_name ( dvb->video, "dvb-get-sid", &sid );

	info_dvb_win_new ( sid, data, obj, dvb->win_base );
}

static void helia_dvb_stop ( G_GNUC_UNUSED GtkButton *button, HeliaDvb *dvb )
{
	helia_dvb_multi_stop ( dvb );
}

static void helia_dvb_record ( G_GNUC_UNUSED GtkButton *button, HeliaDvb *dvb )
{
	g_signal_emit_by_name ( dvb->video, "dvb-rec" );
}

static void helia_dvb_volume_changed ( G_GNUC_UNUSED GtkScaleButton *button, double value, HeliaDvb *dvb )
{
	g_signal_emit_by_name ( dvb->video, "dvb-vol", value );
}

static GtkBox * helia_dvb_create_control ( HeliaDvb *dvb )
{
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 5 );

	GtkVolumeButton *volbutton = (GtkVolumeButton *)gtk_volume_button_new ();
	gtk_widget_set_visible ( GTK_WIDGET ( volbutton ), TRUE );
	gtk_button_set_relief  ( GTK_BUTTON ( volbutton ), GTK_RELIEF_NORMAL );
	gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( volbutton ), 1.0 );
	g_signal_connect ( volbutton, "value-changed", G_CALLBACK ( helia_dvb_volume_changed ), dvb );

	const char *icons[] = { "helia-stop", "helia-record", "helia-display", NULL, "helia-pref" };
	fpd funcs[] = { helia_dvb_stop, helia_dvb_record, helia_dvb_info, NULL, helia_dvb_pref };

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( icons ); c++ )
	{
		if ( icons[c] == NULL ) { gtk_box_pack_start ( h_box, GTK_WIDGET ( volbutton ), TRUE, TRUE, 0 ); continue; }

		dvb->button[c] = (GtkButton *)gtk_button_new_from_icon_name ( icons[c], GTK_ICON_SIZE_MENU );
		gtk_widget_set_visible ( GTK_WIDGET ( dvb->button[c] ), TRUE );
		gtk_box_pack_start ( h_box, GTK_WIDGET ( dvb->button[c] ), TRUE, TRUE, 0 );
		g_signal_connect ( dvb->button[c], "clicked", G_CALLBACK ( funcs[c] ), dvb );
	}

	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	return h_box;
}

static GtkPaned * helia_dvb_create_paned ( TreeDvb *tree, GtkBox *video )
{
	GtkPaned *paned = (GtkPaned *)gtk_paned_new ( GTK_ORIENTATION_HORIZONTAL );

	gtk_paned_add1 ( paned, GTK_WIDGET ( tree  ) );
	gtk_paned_add2 ( paned, GTK_WIDGET ( video ) );

	return paned;
}

static void helia_dvb_drag_in ( G_GNUC_UNUSED GtkBox *box, GdkDragContext *ct, G_GNUC_UNUSED int x, G_GNUC_UNUSED int y,
	GtkSelectionData *s_data, G_GNUC_UNUSED uint info, guint32 time, HeliaDvb *dvb )
{
	char **uris = gtk_selection_data_get_uris ( s_data );

	uint c = 0; for ( c = 0; uris[c] != NULL; c++ )
	{
		char *path = g_filename_from_uri ( uris[c], NULL, NULL );

		if ( path && g_str_has_suffix ( path, "gtv-channel.conf" ) )
			g_signal_emit_by_name ( dvb->treedvb, "treeview-dvb-add", path );

		free ( path );
	}

	g_strfreev ( uris );
	gtk_drag_finish ( ct, TRUE, FALSE, time );
}

static GtkImage * helia_dvb_create_image ( const char *icon, uint16_t size )
{
	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), icon, size, GTK_ICON_LOOKUP_FORCE_SIZE, NULL );

	GtkImage *image   = (GtkImage *)gtk_image_new_from_pixbuf ( pixbuf );
	gtk_image_set_pixel_size ( image, size );

	if ( pixbuf ) g_object_unref ( pixbuf );

	return image;
}

static uint helia_dvb_multi_get_box_w_n ( GtkBox *hbox )
{
	GList *list = gtk_container_get_children ( GTK_CONTAINER ( hbox ) );

	uint nums = g_list_length ( list );

	g_list_free ( list );

	return nums;
}

static gboolean helia_dvb_multi_set_visible ( HeliaDvb *dvb )
{
	uint nums = helia_dvb_multi_get_box_w_n ( dvb->hbox_video_b );

	if ( !nums ) gtk_widget_set_visible ( GTK_WIDGET ( dvb->hbox_video_b ), FALSE );	

	return FALSE;
}

static void helia_dvb_handler_base ( G_GNUC_UNUSED Dvb *d, uint num, HeliaDvb *dvb )
{
	if ( num == 1 ) { gtk_widget_set_visible ( GTK_WIDGET ( dvb->treedvb ), FALSE ); }

	if ( num == 2 ) { gboolean vis = gtk_widget_get_visible ( GTK_WIDGET ( dvb->treedvb ) ); gtk_widget_set_visible ( GTK_WIDGET ( dvb->treedvb ), !vis ); }

	if ( num == 99 ) { dvb->multi_destroy = TRUE; g_timeout_add ( 250, (GSourceFunc)helia_dvb_multi_set_visible, dvb ); }
}

static void helia_dvb_handler_icon ( G_GNUC_UNUSED Dvb *d, gboolean play, HeliaDvb *dvb )
{
	GtkImage *image = helia_dvb_create_image ( ( play ) ? "helia-info" : "helia-display", 16 );
	gtk_button_set_image ( dvb->button[2], GTK_WIDGET ( image ) );
}

static void helia_dvb_handler_play ( G_GNUC_UNUSED TreeDvb *td, const char *data, HeliaDvb *dvb )
{
	if ( dvb->multi_destroy || dvb->win_count ) helia_dvb_multi_stop ( dvb );

	g_signal_emit_by_name ( dvb->video, "dvb-play", data );
}

static void helia_dvb_handler_multi ( G_GNUC_UNUSED TreeDvb *td, const char *data, HeliaDvb *dvb )
{
	if ( dvb->multi_destroy || dvb->win_count >= 3 ) return;

	gboolean play = FALSE;
	g_signal_emit_by_name ( dvb->video, "dvb-is-play", &play );

	if ( !play ) return;

	dvb->win_count++;

	Dvb *video_add = dvb_new ( dvb->win_count, NULL );
	g_signal_connect ( video_add, "dvb-base", G_CALLBACK ( helia_dvb_handler_base ), dvb );

	if ( dvb->win_count < 2 )
	{
		gtk_box_pack_start ( dvb->hbox_video_a, GTK_WIDGET ( video_add ), TRUE, TRUE, 0 );
	}
	else
	{
		gtk_box_pack_start ( dvb->hbox_video_b, GTK_WIDGET ( video_add ), TRUE, TRUE, 0 );
		gtk_widget_set_visible ( GTK_WIDGET ( dvb->hbox_video_b ), TRUE );
	}

	g_signal_emit_by_name ( video_add, "dvb-multi", data, dvb->video );
}

static void helia_dvb_multi_all_close ( GtkBox *hbox )
{
	GList *list = gtk_container_get_children ( GTK_CONTAINER ( hbox ) );

	while ( list != NULL )
	{
		gtk_widget_destroy ( GTK_WIDGET ( list->data ) );

		list = list->next;
	}

	g_list_free ( list );
}

static void helia_dvb_multi_stop ( HeliaDvb *dvb )
{
	g_signal_emit_by_name ( dvb->video, "dvb-stop" );

	dvb->win_count = 0;

	gboolean vis = gtk_widget_get_visible ( GTK_WIDGET ( dvb->hbox_video_b ) );

	if ( vis )
	{
		helia_dvb_multi_all_close ( dvb->hbox_video_b );
		gtk_widget_set_visible ( GTK_WIDGET ( dvb->hbox_video_b ), FALSE );
	}

	helia_dvb_multi_all_close ( dvb->hbox_video_a );

	dvb->video = dvb_new ( dvb->win_count, dvb->level );
	g_signal_connect ( dvb->video, "dvb-base", G_CALLBACK ( helia_dvb_handler_base ), dvb );
	g_signal_connect ( dvb->video, "dvb-icon-scan-info", G_CALLBACK ( helia_dvb_handler_icon ), dvb );

	gtk_box_pack_start ( dvb->hbox_video_a, GTK_WIDGET ( dvb->video ), TRUE, TRUE, 0 );

	dvb->multi_destroy = FALSE;
}

static void helia_dvb_init ( HeliaDvb *dvb )
{
	GtkBox *box = GTK_BOX ( dvb );
	gtk_orientable_set_orientation ( GTK_ORIENTABLE ( box ), GTK_ORIENTATION_VERTICAL );

	dvb->win_count = 0;
	dvb->multi_destroy = FALSE;

	dvb->level = level_new ();

	dvb->video = dvb_new ( dvb->win_count, dvb->level );
	g_signal_connect ( dvb->video, "dvb-base", G_CALLBACK ( helia_dvb_handler_base ), dvb );
	g_signal_connect ( dvb->video, "dvb-icon-scan-info", G_CALLBACK ( helia_dvb_handler_icon ), dvb );

	dvb->treedvb = treedvb_new ( dvb->level );
	g_signal_connect ( dvb->treedvb, "treeview-dvb-play",  G_CALLBACK ( helia_dvb_handler_play  ), dvb );
	g_signal_connect ( dvb->treedvb, "treeview-dvb-multi", G_CALLBACK ( helia_dvb_handler_multi ), dvb );

	dvb->vbox_video = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( dvb->vbox_video ), TRUE );

	dvb->hbox_video_a = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( dvb->hbox_video_a ), TRUE );

	dvb->hbox_video_b = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( dvb->hbox_video_b ), FALSE );

	gtk_box_pack_start ( dvb->hbox_video_a, GTK_WIDGET ( dvb->video      ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( dvb->vbox_video, GTK_WIDGET ( dvb->hbox_video_a ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( dvb->vbox_video, GTK_WIDGET ( dvb->hbox_video_b ), TRUE, TRUE, 0 );

	GtkBox *control = helia_dvb_create_control ( dvb );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( control ), 5 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( control ), 5 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( control ), 5 );

	gtk_box_pack_end ( GTK_BOX ( dvb->treedvb ), GTK_WIDGET ( control ), FALSE, FALSE, 0 );

	GtkPaned *paned = helia_dvb_create_paned ( dvb->treedvb, dvb->vbox_video );
	gtk_widget_set_visible ( GTK_WIDGET ( paned ), TRUE );
	gtk_box_pack_start ( box, GTK_WIDGET ( paned ), TRUE, TRUE, 0 );

	gtk_drag_dest_set ( GTK_WIDGET ( box ), GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY );
	gtk_drag_dest_add_uri_targets  ( GTK_WIDGET ( box ) );
	g_signal_connect  ( box, "drag-data-received", G_CALLBACK ( helia_dvb_drag_in ), dvb );
}

static void helia_dvb_finalize ( GObject *object )
{
	G_OBJECT_CLASS ( helia_dvb_parent_class )->finalize ( object );
}

static void helia_dvb_class_init ( HeliaDvbClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS ( class );

	oclass->finalize = helia_dvb_finalize;
}

HeliaDvb * helia_dvb_new ( Pref *pref, GtkWindow *window )
{
	HeliaDvb *dvb = g_object_new ( HELIA_TYPE_DVB, NULL );

	dvb->pref = pref;
	dvb->win_base = window;

	return dvb;
}
