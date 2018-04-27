/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/


#include "gmp-media.h"
#include "gmp-dvb.h"
#include "gmp-gst-tv.h"
#include "gmp-gst-pl.h"
#include "gmp-scan.h"
#include "gmp-pref.h"

#include <gdk/gdkx.h>
#include <glib/gi18n.h>


struct GmpMedia
{
	gulong win_handle_tv, win_handle_pl;
	gboolean media_tv_pl, rec_trig_vis, show_slider_menu, firstfile, all_info;

	GtkBox *sw_vbox_tv, *sw_vbox_pl, *hbox_base, *hbox_panel;
	GtkTreeView *treeview_tv, *treeview_pl;
	GtkTreePath *gmp_index_path;

	GtkScale *slider_menu, *slider_base, *slider_vol;
	GtkLabel *lab_pos, *lab_dur,*lab_pos_m, *lab_dur_m, *lab_buf;
	GtkSpinner *spinner;	

	GtkLabel *signal_snr;
	GtkProgressBar *bar_sgn, *bar_snr;

	gulong slider_signal_id, slider_signal_id_m, slider_signal_id_vol;
};

struct GmpMedia gmpmedia;


static void gmp_treeview_to_file ( GtkTreeView *tree_view, gchar *filename, gboolean tv_pl );




void gmp_media_stop_all ()
{
	gmp_dvb_stop ();
	gmp_player_playback_stop ();
	gmp_player_base_quit ( TRUE );
	
	g_debug ( "gmp_media_stop_all" );
}

void gmp_media_auto_save ()
{
	gmp_treeview_to_file ( gmpmedia.treeview_tv, (gchar *)ch_conf, TRUE );
	
	g_debug ( "gmp_media_auto_save" );
}


static void gmp_media_buttons ( GtkBox *box, gchar *icon, guint size, void (* activate)(), GtkWidget *widget )
{
	GdkPixbuf *logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
              icon, size, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

  	GtkButton *button = (GtkButton *)gtk_button_new ();
  	GtkImage *image   = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );
 
	gtk_button_set_image ( button, GTK_WIDGET ( image ) );

  	g_signal_connect ( button, "clicked", G_CALLBACK (activate), widget );

  	gtk_box_pack_start ( box, GTK_WIDGET ( button ), TRUE, TRUE, 5 );
}
static GtkButton * gmp_media_ret_button ( GtkBox *box, gchar *icon, guint size )
{
	GtkButton *button = (GtkButton *)gtk_button_new ();
	
	GdkPixbuf *logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
              icon, size, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

  	GtkImage *image   = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );
 
	gtk_button_set_image ( button, GTK_WIDGET ( image ) );

  	gtk_box_pack_start ( box, GTK_WIDGET ( button ), TRUE, TRUE, 5 );
  	
  	return button;
}



// ***** TV: Panel *****

void gmp_set_sgn_snr ( GstElement *element, gdouble sgl, gdouble snr, gboolean hlook, gboolean rec_ses )
{	
	GtkLabel *label = gmpmedia.signal_snr;
	GtkProgressBar *barsgn = gmpmedia.bar_sgn; 
	GtkProgressBar *barsnr = gmpmedia.bar_snr;
	
    gtk_progress_bar_set_fraction ( barsgn, sgl/100 );
    gtk_progress_bar_set_fraction ( barsnr, snr/100 );

    gchar *texta = g_strdup_printf ( "Level %d%s", (int)sgl, "%" );
    gchar *textb = g_strdup_printf ( "Snr %d%s",   (int)snr, "%" );

    const gchar *format = NULL, *rec = " ● ";

	if ( gmpmedia.rec_trig_vis ) rec = " ◌ ";

    gboolean play = TRUE;
    if ( GST_ELEMENT_CAST ( element )->current_state != GST_STATE_PLAYING ) play = FALSE;

        if ( hlook )
            format = "<span>\%s</span><span foreground=\"#00ff00\"> ◉ </span><span>\%s</span><span foreground=\"#ff0000\">  \%s</span>";
        else
            format = "<span>\%s</span><span foreground=\"#ff0000\"> ◉ </span><span>\%s</span><span foreground=\"#ff0000\">  \%s</span>";

        if ( !play )
            format = "<span>\%s</span><span foreground=\"#ff8000\"> ◉ </span><span>\%s</span><span foreground=\"#ff0000\">  \%s</span>";

        if ( sgl == 0 && snr == 0 )
            format = "<span>\%s</span><span foreground=\"#bfbfbf\"> ◉ </span><span>\%s</span><span foreground=\"#ff0000\">  \%s</span>";

        gchar *markup = g_markup_printf_escaped ( format, texta, textb, rec_ses ? rec : "" );
            gtk_label_set_markup ( label, markup );
        g_free ( markup );

    g_free ( texta );
    g_free ( textb );

	gmpmedia.rec_trig_vis = !gmpmedia.rec_trig_vis;
}

static GtkBox * gmp_create_sgn_snr ()
{	
    GtkBox *tbar_dvb = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
    gtk_widget_set_margin_start ( GTK_WIDGET ( tbar_dvb ), 5 );
    gtk_widget_set_margin_end   ( GTK_WIDGET ( tbar_dvb ), 5 );

    gmpmedia.signal_snr = (GtkLabel *)gtk_label_new ( "Level  &  Quality" );
    gtk_box_pack_start ( tbar_dvb, GTK_WIDGET ( gmpmedia.signal_snr  ), FALSE, FALSE, 5 );

    gmpmedia.bar_sgn = (GtkProgressBar *)gtk_progress_bar_new ();
    gmpmedia.bar_snr = (GtkProgressBar *)gtk_progress_bar_new ();
    
    gtk_box_pack_start ( tbar_dvb, GTK_WIDGET ( gmpmedia.bar_sgn  ), FALSE, FALSE, 0 );
    gtk_box_pack_start ( tbar_dvb, GTK_WIDGET ( gmpmedia.bar_snr  ), FALSE, FALSE, 3 );

    return tbar_dvb;
}



static void gmp_media_panel_menu_quit_tv ( GtkWidget *window )
{
    gtk_widget_destroy ( GTK_WIDGET ( window ) );
}
static void gmp_media_panel_close_tv ( GtkButton *button, GtkWidget *window )
{
    gmp_media_panel_menu_quit_tv ( GTK_WIDGET ( window ) );

	g_debug ( "gmp_media_panel_close_tv:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_window_tv ( GtkButton *button, GtkWidget *window )
{
	gmp_dvb_stop ();

	gmp_media_panel_menu_quit_tv ( GTK_WIDGET ( window ) );
    gmp_base_set_window ();

	g_debug ( "gmp_media_panel_window_tv:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_scan_tv ( GtkButton *button, GtkWidget *window )
{			
	gmp_media_panel_menu_quit_tv ( GTK_WIDGET ( window ) );

	gmp_scan_win ();

	g_debug ( "gmp_media_panel_scan_tv:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_convert_tv ( GtkButton *button, GtkWidget *window )
{			
	gmp_media_panel_menu_quit_tv ( GTK_WIDGET ( window ) );

	gmp_convert_win ( FALSE );

	g_debug ( "gmp_media_panel_convert_tv:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_scroll_tv ( GtkButton *button, GtkWidget *vbox )
{
	gtk_widget_get_visible ( GTK_WIDGET ( vbox ) ) ? gtk_widget_hide ( GTK_WIDGET ( vbox ) ) : gtk_widget_show ( GTK_WIDGET ( vbox ) );
	
	g_debug ( "gmp_media_panel_scroll_tv:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_ega_tv ( GtkButton *button, GtkWidget *window )
{
	if ( gmp_dvb_ega_tv () ) gmp_media_panel_menu_quit_tv ( GTK_WIDGET ( window ) );

	g_debug ( "gmp_media_panel_ega_tv:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}
static void gmp_media_panel_egv_tv ( GtkButton *button, GtkWidget *window )
{	
	if ( gmp_dvb_egv_tv () ) gmp_media_panel_menu_quit_tv ( GTK_WIDGET ( window ) );

	g_debug ( "gmp_media_panel_egv_tv:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_mute_tv ( GtkButton *button, GtkWidget *widget )
{
	gmp_dvb_mute_set_tv ( widget );

	g_debug ( "gmp_media_panel_mute_tv:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_volume_changed_tv ( GtkScaleButton *button, gdouble value )
{
	gmp_dvb_volume_changed_tv ( value );
	
	g_debug ( "gmp_media_panel_volume_changed_tv:: widget name %s | value %f", gtk_widget_get_name ( GTK_WIDGET ( button ) ), value );
}

static void gmp_media_panel_tv ( GtkBox *vbox )
{
    GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_transient_for ( window, gmp_base_win_ret () );
    gtk_window_set_modal     ( window, TRUE   );
	gtk_window_set_decorated ( window, FALSE  );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );

	GtkVolumeButton *volbutton = (GtkVolumeButton *)gtk_volume_button_new ();
    gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( volbutton ), gmp_dvb_get_volume_tv () );
    g_signal_connect ( volbutton, "value-changed", G_CALLBACK ( gmp_media_panel_volume_changed_tv ), NULL );

	gtk_widget_set_sensitive ( GTK_WIDGET ( volbutton ), !gmp_dvb_mute_get_tv () );
		
		
    GtkBox *m_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );

	GtkBox *b_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
    GtkBox *l_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
    GtkBox *r_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
    
    GtkBox *h_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	GtkBox *hm_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gmp_media_buttons ( h_box, "gmp-tv",     resize_icon, gmp_media_panel_window_tv, GTK_WIDGET ( window ) );
	gmp_media_buttons ( h_box, "gmp-editor", resize_icon, gmp_media_panel_scroll_tv, GTK_WIDGET ( vbox )   );
	gmp_media_buttons ( h_box, "gmp-eqav",   resize_icon, gmp_media_panel_ega_tv,    GTK_WIDGET ( window ) );
	gmp_media_buttons ( h_box, "gmp-eqav",   resize_icon, gmp_media_panel_egv_tv,    GTK_WIDGET ( window ) );
	gmp_media_buttons ( h_box, "gmp-muted",  resize_icon, gmp_media_panel_mute_tv,   GTK_WIDGET ( volbutton ) );

    gtk_box_pack_start ( l_box, GTK_WIDGET ( h_box ), TRUE, TRUE, 5 );

	gmp_media_buttons ( hm_box, "gmp-media-stop",   resize_icon, gmp_dvb_stop,   NULL );
	gmp_media_buttons ( hm_box, "gmp-media-record", resize_icon, gmp_dvb_record, NULL );
	gmp_media_buttons ( hm_box, "gmp-display",      resize_icon, gmp_media_panel_scan_tv,    GTK_WIDGET ( window ) );
	gmp_media_buttons ( hm_box, "gmp-convert",      resize_icon, gmp_media_panel_convert_tv, GTK_WIDGET ( window ) );
	gmp_media_buttons ( hm_box, "gmp-exit",         resize_icon, gmp_media_panel_close_tv,   GTK_WIDGET ( window ) );

	gtk_box_pack_start ( l_box, GTK_WIDGET ( hm_box ), TRUE, TRUE, 5 );
		
	
	gmp_dvb_audio_button_tv ( r_box );
	gtk_box_pack_start ( r_box, GTK_WIDGET ( volbutton ), TRUE, TRUE, 5 );
	

	gtk_box_pack_start ( b_box, GTK_WIDGET ( l_box ), TRUE,  TRUE,  5 );
	gtk_box_pack_end   ( b_box, GTK_WIDGET ( r_box ), FALSE, FALSE, 5 );
	
	gtk_box_pack_start ( m_box, GTK_WIDGET ( b_box ), TRUE,  TRUE,  5 );
		

    gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
    gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
    gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity_cnt );
}

// ***** TV: Panel *****



// ***** Panel PL *****

static GtkBox * gmp_media_spinner_pl ()
{
	GtkBox *hl_box   = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
		
	gmpmedia.spinner = (GtkSpinner *)gtk_spinner_new ();
	gmpmedia.lab_buf = (GtkLabel *)gtk_label_new ("");
	gtk_widget_set_halign ( GTK_WIDGET ( gmpmedia.lab_buf ), GTK_ALIGN_START );
		
	gtk_box_pack_start ( hl_box, GTK_WIDGET ( gmpmedia.spinner ), FALSE, FALSE, 10 );
	gtk_box_pack_start ( hl_box, GTK_WIDGET ( gmpmedia.lab_buf ), FALSE, FALSE, 10 );
	
	return hl_box;
}

void gmp_spinner_label_set ( gchar *text, gboolean spinner_stop )
{
	if ( spinner_stop )
		gtk_spinner_stop ( gmpmedia.spinner );
	else
		gtk_spinner_start ( gmpmedia.spinner );
	
	gtk_label_set_text ( gmpmedia.lab_buf, text );
}

static void gmp_label_set_text ( GtkLabel *label, gint64 pos_dur )
{
    gchar *str   = g_strdup_printf ( "%" GST_TIME_FORMAT, GST_TIME_ARGS ( pos_dur ) );
    gchar *str_l = g_strndup ( str, strlen ( str ) - 8 );

    gtk_label_set_text ( label, str_l );

    g_free ( str_l  );
    g_free ( str );
}
void gmp_pos_dur_text ( gint64 pos, gint64 dur )
{
    gmp_label_set_text ( gmpmedia.lab_pos, pos );
    gmp_label_set_text ( gmpmedia.lab_dur, dur );

	if ( gmpmedia.show_slider_menu )
	{
    	gmp_label_set_text ( gmpmedia.lab_pos_m, pos );
    	gmp_label_set_text ( gmpmedia.lab_dur_m, dur );
	}
}
void gmp_pos_text ( gint64 pos )
{
    gmp_label_set_text ( gmpmedia.lab_pos, pos );

	if ( gmpmedia.show_slider_menu )
    	gmp_label_set_text ( gmpmedia.lab_pos_m, pos );
}

static void gmp_slider_update ( gdouble data )
{
	g_signal_handler_block   ( gmpmedia.slider_base, gmpmedia.slider_signal_id );
        gtk_range_set_value  ( GTK_RANGE ( gmpmedia.slider_base ), data );
    g_signal_handler_unblock ( gmpmedia.slider_base, gmpmedia.slider_signal_id );

	if ( !gmpmedia.show_slider_menu ) return;
	
	g_signal_handler_block   ( gmpmedia.slider_menu, gmpmedia.slider_signal_id_m );
		gtk_range_set_value ( GTK_RANGE ( gmpmedia.slider_menu ), data );
    g_signal_handler_unblock ( gmpmedia.slider_menu, gmpmedia.slider_signal_id_m );
}

void gmp_pos_dur_sliders ( gint64 current, gint64 duration )
{
	gdouble slider_max = (gdouble)duration / GST_SECOND;
				
    gtk_range_set_range ( GTK_RANGE ( gmpmedia.slider_base ), 0, slider_max );

	if ( gmpmedia.show_slider_menu )
		gtk_range_set_range ( GTK_RANGE ( gmpmedia.slider_menu ), 0, slider_max );

	gmp_slider_update ( (gdouble)current / GST_SECOND );	
}

static void gmp_media_panel_new_slider ()
{	
	if ( gmp_player_get_current_state_stop () )
	{
		gmp_media_set_sensitive_pl ( FALSE, FALSE );
		return;
	}

	gint64 current  = gmp_player_query_position ();
	gint64 duration = gmp_player_query_duration ();
				
	if ( duration > 0 )
	{
		gtk_range_set_range ( GTK_RANGE ( gmpmedia.slider_menu ), 0, (gdouble)duration / GST_SECOND );
		gmp_pos_dur_sliders ( current, duration );
		gmp_pos_dur_text ( current, duration );
		
		gmp_media_set_sensitive_pl ( TRUE, !gmp_player_mute_get_pl () );
	}
	else
	{
		if ( current > 0 )
			gmp_pos_dur_text ( current, 0 );
		else
		{
			gmp_pos_dur_text ( 0, 0 );
		}
		
		gmp_media_set_sensitive_pl ( FALSE, !gmp_player_mute_get_pl () );
	}
}


static void gmp_slider_changed_menu ( GtkRange *range )
{
	gdouble value = gtk_range_get_value ( GTK_RANGE (range) );
	
    gmp_player_slider_changed ( value );
    
    g_usleep ( 25000 );
    
	gmp_pos_text ( gmp_player_query_position () );

	g_debug ( "gmp_slider_changed_menu" );
}
static void gmp_slider_changed ( GtkRange *range )
{
	gdouble value = gtk_range_get_value ( GTK_RANGE (range) );
	        
	gmp_player_slider_changed ( value );
	
	g_usleep ( 25000 );
	
	gmp_pos_text ( gmp_player_query_position () );

	g_debug ( "gmp_slider_changed" );
}

static void gmp_slider_changed_vol ( GtkRange *range )
{
	gdouble value = gtk_range_get_value ( GTK_RANGE (range) );

	gmp_player_volume_changed_pl ( value );

	g_debug ( "gmp_slider_changed_vol:: widget name %s | value %f", gtk_widget_get_name ( GTK_WIDGET ( range ) ), value );
}

static GtkScale * gmp_media_slider_vol_pl ()
{	
    gmpmedia.slider_vol = (GtkScale *)gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.01 );
    gtk_scale_set_draw_value ( gmpmedia.slider_vol, 0 );
	gtk_range_set_value ( GTK_RANGE ( gmpmedia.slider_vol ), gmp_player_get_volume_pl () );
    gmpmedia.slider_signal_id_vol = g_signal_connect ( gmpmedia.slider_vol, "value-changed", G_CALLBACK ( gmp_slider_changed_vol ), NULL );
    
	gtk_widget_set_size_request ( GTK_WIDGET ( gmpmedia.slider_vol  ), 100, -1 );

	return gmpmedia.slider_vol;
}
static GtkBox * gmp_media_slider_base_pl ( GtkScale *slider, GtkLabel *lab_pos, GtkLabel *lab_dur, gboolean vol_slider_set )
{
    gtk_scale_set_draw_value ( slider, 0 );
    gtk_range_set_value ( GTK_RANGE ( slider ), 0 );

    GtkBox *hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
        gtk_box_pack_start ( hbox, GTK_WIDGET ( lab_pos ), FALSE, FALSE, 5 );
        gtk_box_pack_start ( hbox, GTK_WIDGET ( slider  ), TRUE,  TRUE,  0 );
        gtk_box_pack_start ( hbox, GTK_WIDGET ( lab_dur ), FALSE, FALSE, 5 );
		
		if ( vol_slider_set )
			gtk_box_pack_start ( hbox, GTK_WIDGET ( gmp_media_slider_vol_pl () ), FALSE, FALSE, 0 );
    
	gtk_widget_set_size_request ( GTK_WIDGET ( slider ), 300, -1 );

	return hbox;
}


static void gmp_media_panel_menu_quit_pl ( GtkWidget *window )
{
    gtk_widget_destroy ( GTK_WIDGET ( window ) );
	gmpmedia.show_slider_menu = FALSE;
}
static void gmp_media_panel_close_pl ( GtkButton *button, GtkWidget *window )
{
    gmp_media_panel_menu_quit_pl ( GTK_WIDGET ( window ) );

	g_debug ( "gmp_media_panel_close_pl:: widget name %s ", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_base_win ( GtkButton *button, GtkWidget *window )
{
	gmp_player_playback_stop ();

	gmp_media_panel_menu_quit_pl ( GTK_WIDGET ( window ) );
    gmp_base_set_window ();

	g_debug ( "gmp_media_panel_base_win:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_scroll_pl ( GtkButton *button, GtkWidget *vbox )
{
	gtk_widget_get_visible ( GTK_WIDGET ( vbox ) ) ? gtk_widget_hide ( GTK_WIDGET ( vbox ) ) : gtk_widget_show ( GTK_WIDGET ( vbox ) );
	
	g_debug ( "gmp_media_panel_scroll_pl:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_ega_pl ( GtkButton *button, GtkWidget *window )
{
	if ( gmp_player_ega_pl () ) gmp_media_panel_menu_quit_pl ( GTK_WIDGET ( window ) );

	g_debug ( "gmp_media_panel_ega_pl:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}
static void gmp_media_panel_egv_pl ( GtkButton *button, GtkWidget *window )
{	
	if ( gmp_player_egv_pl () ) gmp_media_panel_menu_quit_pl ( GTK_WIDGET ( window ) );

	g_debug ( "gmp_media_panel_egv_pl:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_mute_pl ( GtkButton *button, GtkWidget *widget )
{
	gmp_player_mute_set_pl ( widget, GTK_WIDGET ( gmpmedia.slider_vol ) );

	g_debug ( "gmp_media_panel_mute_pl:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_volume_changed_pl ( GtkScaleButton *button, gdouble value )
{
	gmp_player_volume_changed_pl ( value );

	g_signal_handler_block   ( gmpmedia.slider_vol, gmpmedia.slider_signal_id_vol );
		gtk_range_set_value  ( GTK_RANGE ( gmpmedia.slider_vol ), value );
    g_signal_handler_unblock ( gmpmedia.slider_vol, gmpmedia.slider_signal_id_vol );
		
	g_debug ( "gmp_media_panel_volume_changed_pl:: widget name %s | value %f", gtk_widget_get_name ( GTK_WIDGET ( button ) ), value );
}

static void gmp_media_panel_slider_pl ()
{	
	if ( gtk_widget_get_visible ( GTK_WIDGET ( gmpmedia.hbox_base ) ) )
	{
		gtk_widget_hide ( GTK_WIDGET ( gmpmedia.hbox_base ) );		
		gtk_widget_show ( GTK_WIDGET ( gmpmedia.hbox_panel ) );
		
		gmpmedia.show_slider_menu = TRUE;	
		
		gmp_media_panel_new_slider ();
	}	
	else
	{
		gtk_widget_hide ( GTK_WIDGET ( gmpmedia.hbox_panel ) );
		gtk_widget_show ( GTK_WIDGET ( gmpmedia.hbox_base ) );
		
		gtk_range_set_value  ( GTK_RANGE ( gmpmedia.slider_vol ), gmp_player_get_volume_pl () );

		gmpmedia.show_slider_menu = FALSE;	
	}
}

static void gmp_player_start ( GtkButton *button, GtkWidget *widget )
{
	if ( !gmp_player_playback_start () ) return;
	
	gtk_widget_hide ( GTK_WIDGET ( button ) );		
	gtk_widget_show ( GTK_WIDGET ( widget ) );	
}
static void gmp_player_pause ( GtkButton *button, GtkWidget *widget )
{
	if ( !gmp_player_playback_pause () ) return;

	gtk_widget_hide ( GTK_WIDGET ( button ) );		
	gtk_widget_show ( GTK_WIDGET ( widget ) );	
}
static void gmp_player_stoph ( GtkButton *button, GtkWidget *widget )
{
	if ( !gmp_player_playback_stop () ) return;

	gtk_widget_hide ( GTK_WIDGET ( widget ) );
	
	g_debug ( "gmp_player_stoph:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}
static void gmp_player_stops ( GtkButton *button, GtkWidget *widget )
{
	if ( !gmp_player_get_current_state_stop () ) return;
	
	gtk_widget_show ( GTK_WIDGET ( widget ) );	
	
	g_debug ( "gmp_player_stops:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}
static void gmp_player_record ()
{	
	gmp_player_playback_record ();
}

void gmp_media_panel_pl ( GtkBox *vbox )
{
    GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_transient_for ( window, gmp_base_win_ret () );
    gtk_window_set_modal     ( window, TRUE   );
	gtk_window_set_decorated ( window, FALSE  );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );

	GtkVolumeButton *volbutton = (GtkVolumeButton *)gtk_volume_button_new ();
    gtk_scale_button_set_value ( GTK_SCALE_BUTTON ( volbutton ), gmp_player_get_volume_pl () );
    g_signal_connect ( volbutton, "value-changed", G_CALLBACK ( gmp_media_panel_volume_changed_pl ), NULL );

	gtk_widget_set_sensitive ( GTK_WIDGET ( volbutton ), !gmp_player_mute_get_pl () );
		
		
    GtkBox *m_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );

	GtkBox *b_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
    GtkBox *l_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
    GtkBox *r_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
    
    GtkBox *h_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	GtkBox *hm_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gmp_media_buttons ( h_box, "gmp-mp",     resize_icon, gmp_media_panel_base_win,  GTK_WIDGET ( window ) );
	gmp_media_buttons ( h_box, "gmp-editor", resize_icon, gmp_media_panel_scroll_pl, GTK_WIDGET ( vbox )   );
	gmp_media_buttons ( h_box, "gmp-eqav",   resize_icon, gmp_media_panel_ega_pl,    GTK_WIDGET ( window ) );
	gmp_media_buttons ( h_box, "gmp-eqav",   resize_icon, gmp_media_panel_egv_pl,    GTK_WIDGET ( window ) );
	gmp_media_buttons ( h_box, "gmp-muted",  resize_icon, gmp_media_panel_mute_pl,   GTK_WIDGET ( volbutton ) );

    gtk_box_pack_start ( l_box, GTK_WIDGET ( h_box ), TRUE, TRUE, 5 );

	GtkButton *button_start = gmp_media_ret_button ( hm_box, "gmp-media-start",  resize_icon );
	GtkButton *button_pause = gmp_media_ret_button ( hm_box, "gmp-media-pause",  resize_icon );
	GtkButton *button_stop  = gmp_media_ret_button ( hm_box, "gmp-media-stop",   resize_icon );
	GtkButton *button_rec   = gmp_media_ret_button ( hm_box, "gmp-media-record", resize_icon );
	
	g_signal_connect ( button_start, "clicked", G_CALLBACK ( gmp_player_start ),  button_pause );
	g_signal_connect ( button_pause, "clicked", G_CALLBACK ( gmp_player_pause ),  button_start );
	g_signal_connect ( button_stop,  "clicked", G_CALLBACK ( gmp_player_stoph ),  button_pause );
	g_signal_connect ( button_stop,  "clicked", G_CALLBACK ( gmp_player_stops ),  button_start );
	g_signal_connect ( button_rec,   "clicked", G_CALLBACK ( gmp_player_record ), NULL );
	
	gtk_widget_set_sensitive ( GTK_WIDGET ( button_rec ), FALSE ); // 
	
			
	gmp_media_buttons ( hm_box, "gmp-slider",      resize_icon, gmp_media_panel_slider_pl, NULL );
	gmp_media_buttons ( hm_box, "gmp-exit",        resize_icon, gmp_media_panel_close_pl,  GTK_WIDGET ( window ) );

	gtk_box_pack_start ( l_box, GTK_WIDGET ( hm_box ), TRUE, TRUE, 5 );
	
	
	gmp_player_audio_text_buttons ( r_box );
	gtk_box_pack_start ( r_box, GTK_WIDGET ( volbutton ), TRUE, TRUE, 5 );

	gtk_box_pack_start ( b_box, GTK_WIDGET ( l_box ), TRUE,  TRUE,  5 );
	gtk_box_pack_end   ( b_box, GTK_WIDGET ( r_box ), FALSE, FALSE, 5 );
	
	gtk_box_pack_start ( m_box, GTK_WIDGET ( b_box ), TRUE,  TRUE,  5 );
	

    gmpmedia.lab_pos_m = (GtkLabel *)gtk_label_new ( "0:00:00" );
    gmpmedia.lab_dur_m = (GtkLabel *)gtk_label_new ( "0:00:00" );	
	gmpmedia.slider_menu = (GtkScale *)gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0, 100, 1 );
	gmpmedia.slider_signal_id_m = g_signal_connect ( gmpmedia.slider_menu, "value-changed", G_CALLBACK ( gmp_slider_changed_menu ), NULL );

	gmpmedia.hbox_panel = gmp_media_slider_base_pl ( gmpmedia.slider_menu, gmpmedia.lab_pos_m, gmpmedia.lab_dur_m, FALSE );
	
	gtk_box_pack_start ( m_box, GTK_WIDGET ( gmpmedia.hbox_panel ), FALSE,  FALSE,  0 );


    gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
    gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
    gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity_cnt );
	
	gmp_player_get_current_state_playing () ? gtk_widget_hide ( GTK_WIDGET ( button_start ) ) : gtk_widget_hide ( GTK_WIDGET ( button_pause ) );
	

	if ( gtk_widget_get_visible ( GTK_WIDGET ( gmpmedia.hbox_base ) ) )
	{
		gtk_widget_hide ( GTK_WIDGET ( gmpmedia.hbox_panel ) );
		
		gmpmedia.show_slider_menu = FALSE;
	}
	else
		gmpmedia.show_slider_menu = TRUE;


	gmp_media_panel_new_slider ();
}

// ***** Panel PL *****



void gmp_media_set_sensitive_pl ( gboolean sensitive_trc, gboolean sensitive_vol )
{
	//g_debug ( "gmp_media_set_sensitive_pl" );
	
	gtk_widget_set_sensitive ( GTK_WIDGET ( gmpmedia.slider_base ), sensitive_trc );

	if ( gmpmedia.show_slider_menu )
		gtk_widget_set_sensitive ( GTK_WIDGET ( gmpmedia.slider_menu ), sensitive_trc );

	gtk_widget_set_sensitive ( GTK_WIDGET ( gmpmedia.slider_vol ), sensitive_vol );
}

void gmp_media_next_pl ()
{
	g_debug ( "gmp_media_next_pl" );
	
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( gmpmedia.treeview_pl ) );
    guint ind = gtk_tree_model_iter_n_children ( model, NULL );

    if ( !gmpmedia.gmp_index_path || ind < 2 )
    {
		gmp_player_playback_stop (); 
		return;
	}

    GtkTreeIter iter;

    if ( gtk_tree_model_get_iter ( model, &iter, gmpmedia.gmp_index_path ) )
    {
        if ( gtk_tree_model_iter_next ( model, &iter ) )
        {
            gchar *file_ch = NULL;

                gtk_tree_model_get ( model, &iter, COL_DATA, &file_ch, -1 );
                gmpmedia.gmp_index_path = gtk_tree_model_get_path ( model, &iter );
                gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( gmpmedia.treeview_pl ), &iter );
                gmp_player_stop_set_play ( file_ch, gmpmedia.win_handle_pl );

            g_free ( file_ch );
        }
		else
			gmp_player_playback_stop ();
    }
    else
		gmp_player_playback_stop ();
}



// ***** Panel: TreeView *****

static void gmp_media_save ( GtkButton *button, GtkWidget *tree_view )
{
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
    gint ind = gtk_tree_model_iter_n_children ( model, NULL );

    if ( ind > 0 )
    {
        GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
                    _("Choose file"),  NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
                    "gtk-cancel", GTK_RESPONSE_CANCEL,
                    "gtk-save",   GTK_RESPONSE_ACCEPT,
                     NULL );

        gmp_add_filter ( dialog, gmpmedia.media_tv_pl ? "conf" : "m3u", gmpmedia.media_tv_pl ? "*.conf" : "*.m3u" );

		gchar *dir_conf = g_strdup_printf ( "%s/helia", g_get_user_config_dir () );
			gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), gmpmedia.media_tv_pl ? dir_conf : g_get_home_dir () );
		g_free ( dir_conf );
        
        gtk_file_chooser_set_do_overwrite_confirmation ( GTK_FILE_CHOOSER ( dialog ), TRUE    );
        gtk_file_chooser_set_current_name   ( GTK_FILE_CHOOSER ( dialog ), gmpmedia.media_tv_pl ? "gtv-channel.conf" : "playlist-001.m3u" );

        if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
        {
            gchar *filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );
                gmp_treeview_to_file ( GTK_TREE_VIEW ( tree_view ), filename, gmpmedia.media_tv_pl );
            g_free ( filename );
        }

        gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
    }
	
	g_debug ( "gmp_media_save:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}


static void gmp_treeview_clear_click ( GtkButton *button, GtkTreeModel *model )
{
    gtk_list_store_clear ( GTK_LIST_STORE ( model ) );

	g_debug ( "gmp_treeview_clear_click:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );	
}
static void gmp_treeview_close_click ( GtkButton *button, GtkWindow *window )
{
    gtk_widget_destroy ( GTK_WIDGET ( window ) );

	g_debug ( "gmp_treeview_close_click:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );	
}
static void gmp_treeview_clear ( GtkTreeView *tree_view )
{
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
    guint ind = gtk_tree_model_iter_n_children ( model, NULL );

    if ( ind == 0 ) return;

    GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_transient_for ( window, gmp_base_win_ret () );
    gtk_window_set_modal     ( window, TRUE );
    gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
    gtk_window_set_title     ( window, _("Clear") );
    
    gtk_widget_set_size_request ( GTK_WIDGET ( window ), 400, 150 );


    GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
    GtkBox *i_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
    GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	GdkPixbuf *logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
              "gmp-warning", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

  	GtkImage *image   = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );
    gtk_box_pack_start ( i_box, GTK_WIDGET ( image ), TRUE, TRUE, 0 );

	GtkLabel *label = (GtkLabel *)gtk_label_new ( "" );

    gchar *text = g_strdup_printf ( "%d", ind );			
		gtk_label_set_text ( label, text );
    g_free  ( text );
       
    gtk_box_pack_start ( i_box, GTK_WIDGET ( label ), TRUE, TRUE, 10 );

	logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
              "gmp-clear", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

  	image   = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );
    gtk_box_pack_start ( i_box, GTK_WIDGET ( image ), TRUE, TRUE, 0 );
    
    gtk_box_pack_start ( m_box, GTK_WIDGET ( i_box ), TRUE, TRUE, 5 );
    

    GtkButton *button_clear = (GtkButton *)gtk_button_new_from_icon_name ( "gmp-ok", GTK_ICON_SIZE_SMALL_TOOLBAR );
    g_signal_connect ( button_clear, "clicked", G_CALLBACK ( gmp_treeview_clear_click ), model  );
    g_signal_connect ( button_clear, "clicked", G_CALLBACK ( gmp_treeview_close_click ), window );
    gtk_box_pack_end ( h_box, GTK_WIDGET ( button_clear ), TRUE, TRUE, 5 );

    GtkButton *button_close = (GtkButton *)gtk_button_new_from_icon_name ( "gmp-exit", GTK_ICON_SIZE_SMALL_TOOLBAR );
    g_signal_connect ( button_close, "clicked", G_CALLBACK ( gmp_treeview_close_click ), window );
    gtk_box_pack_end ( h_box, GTK_WIDGET ( button_close ), TRUE, TRUE, 5 );

    gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );
    
    
    gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
    gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

    gtk_widget_show_all ( GTK_WIDGET ( window ) );
    
    gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity_win );
    
	g_debug ( "gmp_treeview_clear" );
}

static void gmp_treeview_reread_mini ( GtkTreeView *tree_view )
{
	g_debug ( "gmp_treeview_reread_mini" );
	
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

    gint row_count = 1;
    gboolean valid;
    for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
          valid = gtk_tree_model_iter_next ( model, &iter ) )
    {
        gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter, COL_NUM, row_count++, -1 );
    }
}

static void gmp_treeview_up_down ( GtkTreeView *tree_view, gboolean up_dw )
{
	g_debug ( "gmp_treeview_up_down" );
	
    GtkTreeIter iter, iter_c;
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
    gint ind = gtk_tree_model_iter_n_children ( model, NULL );

    if ( ind < 2 ) return;

    if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter ) )
    {
        gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter_c );

            if ( up_dw )
                if ( gtk_tree_model_iter_previous ( model, &iter ) )
                    gtk_list_store_move_before ( GTK_LIST_STORE ( model ), &iter_c, &iter );

            if ( !up_dw )
                if ( gtk_tree_model_iter_next ( model, &iter ) )
                    gtk_list_store_move_after ( GTK_LIST_STORE ( model ), &iter_c, &iter );

        gmp_treeview_reread_mini ( tree_view );
    }
    else if ( gtk_tree_model_get_iter_first ( model, &iter ) )
    {
        gmpmedia.gmp_index_path = gtk_tree_model_get_path (model, &iter);
        gtk_tree_selection_select_iter ( gtk_tree_view_get_selection (tree_view), &iter);
    }
}

static void gmp_treeview_up_down_s ( GtkTreeView *tree_view, gboolean up_dw )
{
	g_debug ( "gmp_treeview_up_down_s" );
	
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
    gint ind = gtk_tree_model_iter_n_children ( model, NULL );

    if ( ind < 2 ) return;

    if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter ) )
    {
		GtkTreePath *path;
		
            if ( up_dw )
                if ( gtk_tree_model_iter_previous ( model, &iter ) )
                {
                    gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( tree_view ) ), &iter );
					
					path = gtk_tree_model_get_path (model, &iter);
					gtk_tree_view_scroll_to_cell ( tree_view, path, NULL, FALSE, 0, 0 );
				}
				
            if ( !up_dw )
                if ( gtk_tree_model_iter_next ( model, &iter ) )
				{
                    gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( tree_view ) ), &iter );

					path = gtk_tree_model_get_path (model, &iter);
					gtk_tree_view_scroll_to_cell ( tree_view, path, NULL, FALSE, 0, 0 );
				}
    }
    else if ( gtk_tree_model_get_iter_first ( model, &iter ) )
    {
        gmpmedia.gmp_index_path = gtk_tree_model_get_path (model, &iter);
        gtk_tree_selection_select_iter ( gtk_tree_view_get_selection (tree_view), &iter);
    }
}

static void gmp_treeview_remove ( GtkTreeView *tree_view )
{
	gboolean prev_i = FALSE;
    GtkTreeIter iter, iter_prev;
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

    if ( gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter ) )
    {
		gtk_tree_selection_get_selected ( gtk_tree_view_get_selection ( tree_view ), NULL, &iter_prev );
		prev_i = gtk_tree_model_iter_previous ( model, &iter_prev );
		
        gtk_list_store_remove ( GTK_LIST_STORE ( model ), &iter );
        gmp_treeview_reread_mini ( tree_view );
        
        if ( prev_i )
		{
			gtk_tree_selection_select_iter ( gtk_tree_view_get_selection (tree_view), &iter_prev);
		}
		else
		{
			if ( gtk_tree_model_get_iter_first ( model, &iter ) )
				gtk_tree_selection_select_iter ( gtk_tree_view_get_selection (tree_view), &iter);
		}
    }

	g_debug ( "gmp_treeview_remove:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( tree_view ) ) );
}

static void gmp_media_goup  ( GtkButton *button, GtkWidget *tree_view ) 
{ 
	gmp_treeview_up_down ( GTK_TREE_VIEW ( tree_view ), TRUE  ); 

	g_debug ( "gmp_media_goup:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_down_s  ( GtkButton *button, GtkWidget *tree_view ) 
{
	gmp_treeview_up_down_s ( GTK_TREE_VIEW ( tree_view ), FALSE ); 

	g_debug ( "gmp_media_down_s:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_goup_s  ( GtkButton *button, GtkWidget *tree_view ) 
{ 
	gmp_treeview_up_down_s ( GTK_TREE_VIEW ( tree_view ), TRUE  ); 

	g_debug ( "gmp_media_goup_s:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_down  ( GtkButton *button, GtkWidget *tree_view ) 
{
	gmp_treeview_up_down ( GTK_TREE_VIEW ( tree_view ), FALSE ); 

	g_debug ( "gmp_media_down:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_clear ( GtkButton *button, GtkWidget *tree_view ) 
{ 
	gmp_treeview_clear   ( GTK_TREE_VIEW ( tree_view ) );

	g_debug ( "gmp_media_clear:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_remv  ( GtkButton *button, GtkWidget *tree_view ) 
{ 
	gmp_treeview_remove  ( GTK_TREE_VIEW ( tree_view ) );

	g_debug ( "gmp_media_remv:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_media_panel_trw_quit ( GtkWidget *window )
{
    gtk_widget_destroy ( GTK_WIDGET ( window ) );
}
static void gmp_media_trw_close ( GtkButton *button, GtkWidget *window )
{
    gmp_media_panel_trw_quit ( GTK_WIDGET ( window ) );

	g_debug ( "gmp_media_trw_close:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

void gmp_media_panel_tree_view ( GtkTreeView *tree_view )
{
    GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_transient_for ( window, gmp_base_win_ret () );
    gtk_window_set_modal     ( window, TRUE   );
	gtk_window_set_decorated ( window, FALSE  );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	
    GtkBox *m_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
    GtkBox *h_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
    GtkBox *hc_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gmp_media_buttons ( h_box, "gmp-up",     resize_icon, gmp_media_goup,  GTK_WIDGET ( tree_view ) );
	gmp_media_buttons ( h_box, "gmp-down",   resize_icon, gmp_media_down,  GTK_WIDGET ( tree_view ) );	
	gmp_media_buttons ( h_box, "gmp-remove", resize_icon, gmp_media_remv,  GTK_WIDGET ( tree_view ) );
	gmp_media_buttons ( h_box, "gmp-clear",  resize_icon, gmp_media_clear, GTK_WIDGET ( tree_view ) );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), TRUE, TRUE, 5 );
	
	gmp_media_buttons ( hc_box, "gmp-up-select",   resize_icon, gmp_media_goup_s,    GTK_WIDGET ( tree_view ) );
	gmp_media_buttons ( hc_box, "gmp-down-select", resize_icon, gmp_media_down_s,    GTK_WIDGET ( tree_view ) );	
	gmp_media_buttons ( hc_box, "gmp-save",        resize_icon, gmp_media_save,      GTK_WIDGET ( tree_view ) );
	gmp_media_buttons ( hc_box, "gmp-exit",        resize_icon, gmp_media_trw_close, GTK_WIDGET ( window )    );

    gtk_box_pack_start ( m_box, GTK_WIDGET ( hc_box ), TRUE, TRUE, 5 );


    gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
    gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
    gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity_cnt );
}

// ***** Panel: TreeView *****



// ***** TreeView & PlayList *****

static void gmp_media_add_fl_ch ( GtkTreeView *tree_view, gchar *file_ch, gchar *data, gboolean play )
{
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );
    guint ind = gtk_tree_model_iter_n_children ( model, NULL );

    gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter);
    gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter,
                            COL_NUM, ind+1,
                            COL_TITLE, file_ch,
                            COL_DATA, data,
                            -1 );

	if ( play && gmp_player_get_current_state_stop () )
	{
		g_debug ( "gmp_media_add_fl_ch:: play = TRUE" );
		
        gmpmedia.gmp_index_path = gtk_tree_model_get_path ( model, &iter );
        gtk_tree_selection_select_iter ( gtk_tree_view_get_selection ( GTK_TREE_VIEW ( tree_view ) ), &iter );

		gmp_player_stop_set_play ( data, gmpmedia.win_handle_pl );
	}

}

void gmp_str_split_ch_data ( gchar *data )
{
    gchar **lines = g_strsplit ( data, ":", 0 );

        if ( !g_str_has_prefix ( data, "#" ) )
            gmp_media_add_fl_ch ( gmpmedia.treeview_tv, lines[0], data, FALSE );

    g_strfreev ( lines );
}

static void gmp_channels_to_treeview ( const gchar *filename )
{
	g_debug ( "gmp_channels_to_treeview" );
	
    guint n = 0;
    gchar *contents;
    GError *err = NULL;

    if ( g_file_get_contents ( filename, &contents, 0, &err ) )
    {
        gchar **lines = g_strsplit ( contents, "\n", 0 );

        for ( n = 0; lines[n] != NULL; n++ )
            if ( *lines[n] )
                gmp_str_split_ch_data ( lines[n] );

        g_strfreev ( lines );
        g_free ( contents );
    }
    else
    {
        g_critical ( "gmp_channels_to_treeview:: %s\n", err->message );
		g_error_free ( err );
	}
}

static void gmp_treeview_to_file ( GtkTreeView *tree_view, gchar *filename, gboolean tv_pl )
{
	g_debug ( "gmp_treeview_to_file" );
	
	GString *gstring;

	if ( tv_pl )
		gstring = g_string_new ( "# Gtv-Dvb channel format \n" );
	else
		gstring = g_string_new ( "#EXTM3U \n" );

    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

    gboolean valid;
    for ( valid = gtk_tree_model_get_iter_first ( model, &iter ); valid;
          valid = gtk_tree_model_iter_next ( model, &iter ) )
    {
        gchar *data = NULL;
        gchar *name = NULL;

            gtk_tree_model_get ( model, &iter, COL_DATA,  &data, -1 );
            gtk_tree_model_get ( model, &iter, COL_TITLE, &name, -1 );

			if ( !tv_pl )
			{
				g_string_append_printf ( gstring, "#EXTINF:-1,%s\n", name );
			}
			
			g_string_append_printf ( gstring, "%s\n", data );

        g_free ( name );
        g_free ( data );
    }

    if ( !g_file_set_contents ( filename, gstring->str, -1, NULL ) )
		gmp_message_dialog ( "Save failed.", filename, GTK_MESSAGE_ERROR );

    g_string_free ( gstring, TRUE );
}



static void gmp_read_m3u_to_treeview ( GtkTreeView *tree_view, gchar *name_file );

gboolean gmp_checked_filter ( gchar *file_name )
{
    gboolean res = FALSE;
    
	GError *error = NULL;
	GFile *file = g_file_new_for_path ( file_name );
	GFileInfo *file_info = g_file_query_info ( file, "standard::*", 0, NULL, &error );

	const char *content_type = g_file_info_get_content_type (file_info);
    
    if ( g_str_has_prefix ( content_type, "audio" ) || g_str_has_prefix ( content_type, "video" ) )
		res =  TRUE;

	gchar *text = g_utf8_strdown ( file_name, -1 );
	
	if ( g_str_has_suffix ( text, "asx" ) || g_str_has_suffix ( text, "pls" ) )
		res = FALSE;
	
	//g_debug ( "gmp_checked_filter:: file_name %s | content_type %s", file_name, content_type );
	
	g_free ( text );
	

	g_object_unref ( file_info );
	g_object_unref ( file );

    return res;
}

static gint gmp_sort_func ( gconstpointer a, gconstpointer b )
{
	return g_utf8_collate ( a, b );
}

static void gmp_add_playlist_sort ( GtkTreeView *tree_view, GSList *lists )
{
    gboolean set_play = TRUE;

    GSList *list_sort = g_slist_sort ( lists, gmp_sort_func );

    while ( list_sort != NULL )
    {
        gchar *name_file = g_strdup ( list_sort->data );
        gchar *text = g_utf8_strdown ( name_file, -1 );
        
        g_debug ( "gmp_add_playlist_sort:: name_file %s | play = %s", name_file, ( set_play ) ? "TRUE" : "FALSE" );

        if ( g_str_has_suffix ( text, ".m3u" ) )
            gmp_read_m3u_to_treeview ( tree_view, name_file );
        else
        {			
            gchar *name = g_path_get_basename ( name_file );
                gmp_media_add_fl_ch ( tree_view, g_path_get_basename ( name ), name_file, set_play );
            g_free ( name );
		}
		
		g_free ( text );
        g_free ( name_file );

        list_sort = list_sort->next;
        set_play = FALSE;
    }

    g_slist_free ( list_sort );
}

static void gmp_read_m3u_to_treeview ( GtkTreeView *tree_view, gchar *name_file )
{
    gchar  *contents = NULL;
    GError *err      = NULL;
    
    if ( g_file_get_contents ( name_file, &contents, 0, &err ) )
    {
        gchar **lines = g_strsplit ( contents, "\n", 0 );

        gint i; for ( i = 0; lines[i] != NULL; i++ )
        //for ( i = 0; lines[i] != NULL && *lines[i]; i++ )
        {
			if ( g_str_has_prefix ( lines[i], "#EXTINF" ) )
			{				
				if ( g_str_has_prefix ( lines[i+1], "/" ) || g_strrstr ( lines[i+1], "://" ) )
				{
					gchar **lines_info = g_strsplit ( lines[i], ",", 0 );
				
					gmp_media_add_fl_ch ( tree_view, g_strstrip ( lines_info[1] ), g_strstrip ( lines[i+1] ), FALSE );
				
					g_strfreev ( lines_info );
								
					i += 1;
				}
			}
			else
			{
				if ( g_str_has_prefix ( lines[i], "/" ) || g_strrstr ( lines[i], "://" ) )
				{
					gchar *name = g_path_get_basename ( lines[i] );
						gmp_media_add_fl_ch ( tree_view, g_strstrip ( name ), g_strstrip ( lines[i] ), FALSE );
					g_free ( name );
				}				
			}
        }

        g_strfreev ( lines );
        g_free ( contents );
    }
    else
    {
        g_critical ( "gmp_read_m3u_to_treeview:: file %s | %s\n", name_file, err->message );
        g_error_free ( err );
	}
}

static void gmp_read_dir ( GtkTreeView *tree_view, gchar *dir_path )
{	
    GDir *dir = g_dir_open ( dir_path, 0, NULL );
    const gchar *name = NULL;

    GSList *lists = NULL;

    if (!dir)
        g_printerr ( "opening directory %s failed\n", dir_path );
    else
    {
        while ( ( name = g_dir_read_name ( dir ) ) != NULL )
        {
            gchar *path_name = g_strconcat ( dir_path, "/", name, NULL );

            if ( g_file_test ( path_name, G_FILE_TEST_IS_DIR ) )
                gmp_read_dir ( tree_view, path_name ); // Recursion!

            if ( g_file_test ( path_name, G_FILE_TEST_IS_REGULAR ) )
            {
                if ( gmp_checked_filter ( path_name ) )
                    lists = g_slist_append ( lists, g_strdup ( path_name ) );
            }

            name = NULL;
            g_free ( path_name );
        }

        g_dir_close ( dir );

        gmp_add_playlist_sort ( tree_view, lists );

        g_slist_free ( lists );
        name = NULL;
    }
}

static void gmp_add_file_name ( GtkTreeView *tree_view, gchar *file_name )
{
	g_debug ( "gmp_add_file_name" );
	
    if ( g_file_test ( file_name, G_FILE_TEST_IS_DIR ) )
    {
        gmp_read_dir ( tree_view, file_name );
        return;
    }

	if ( !gmp_checked_filter ( file_name ) ) return;

	gchar *text = g_utf8_strdown ( file_name, -1 );

    if ( g_str_has_suffix ( text, ".m3u" ) )
        gmp_read_m3u_to_treeview ( tree_view, file_name );
    else
    {
        GSList *lists = NULL;
        lists = g_slist_append ( lists, g_strdup ( file_name ) );
            gmp_add_playlist_sort ( tree_view, lists );
        g_slist_free ( lists );
    }
    
    g_free ( text );
}


static void gmp_media_drag_in ( GtkDrawingArea *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, guint info, guint32 time, GtkTreeView *tree_view )
{
	const gchar *title = gtk_tree_view_column_get_title ( gtk_tree_view_get_column ( tree_view, 1 ) );

/*
	guchar *text  = gtk_selection_data_get_text ( selection_data );
	
	if ( text )
	{
		gchar *name = g_path_get_basename ( (gchar *)text );
		
			if ( g_strrstr ( (gchar *)text, "://" ) )
				gmp_media_add_fl_ch ( tree_view, name, (gchar *)text, FALSE );
			
			g_debug ( "gmp_media_drag_in:: text %s", text );
		
		g_free ( name );
		g_free ( text );
		
		gtk_drag_finish ( context, TRUE, FALSE, time );
		
		return;
	}
*/

	gchar **uris = gtk_selection_data_get_uris ( selection_data );
	
	guint i = 0; 
	for ( i = 0; uris[i] != NULL; i++ )
    {
        GFile *file = g_file_new_for_uri ( uris[i] );
        gchar *file_name = g_file_get_path ( file );
        
        g_debug ( "title: %s | file: %s", title, file_name );	

		if ( gmpmedia.media_tv_pl )
		{
			if ( g_str_has_suffix ( file_name, "gtv-channel.conf" ) )
			{
				gmp_channels_to_treeview ( ch_conf );
			}
			
			if ( g_str_has_suffix ( file_name, "dvb_channel.conf" ) )
			{
				gmp_drag_cvr = g_strdup ( file_name );			
				gmp_convert_win ( TRUE );
			}
		}
		else
		{	
			//if ( g_str_has_suffix ( file_name, "srt" ) )
				//gmp_player_set_suburi ( file_name );
			//else
				gmp_add_file_name ( tree_view, file_name );
		}
		
        g_free ( file_name );
        g_object_unref ( file );		
    }

	g_strfreev ( uris );

	gtk_drag_finish ( context, TRUE, FALSE, time );

	g_debug ( "gmp_media_drag_in:: widget name %s | x %d | y %d | i %d", gtk_widget_get_name ( GTK_WIDGET ( widget ) ), x, y, info );
}

void gmp_media_add_arg ( GSList *lists )
{
	while ( lists != NULL )
	{
		gchar *name_file = g_strdup ( lists->data );
		
		g_debug ( "gmp_media_add_arg:: name_file %s", name_file );

		if ( g_strrstr ( name_file, "://" ) )
		{
			GFile *file = g_file_new_for_uri ( name_file );
			gchar *file_name = g_file_get_path ( file );

			gmp_add_file_name ( gmpmedia.treeview_pl, file_name );
		
			g_free ( file_name );
			g_object_unref ( file );
		}
		else
			gmp_add_file_name ( gmpmedia.treeview_pl, name_file );
		
		lists = lists->next;
		g_free ( name_file );
	}
}

// ***** TreeView & PlayList *****



// ***** GtkScrolledWindow *****

static void gmp_tree_view_row_activated ( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column )
{
	g_debug ( "gmp_tree_view_row_activated:: column name %s", gtk_tree_view_column_get_title ( column ) );
	
    GtkTreeIter iter;
    GtkTreeModel *model = gtk_tree_view_get_model ( GTK_TREE_VIEW ( tree_view ) );

    if ( gtk_tree_model_get_iter ( model, &iter, path ) )
    {
        gchar *file_ch = NULL;
            gtk_tree_model_get ( model, &iter, COL_DATA, &file_ch, -1 );
            
			if ( !gmpmedia.media_tv_pl )
			{
				gmp_player_stop_set_play ( file_ch, gmpmedia.win_handle_pl );
				gmpmedia.gmp_index_path = gtk_tree_model_get_path ( model, &iter );
			}
			else
				gmp_dvb_play ( file_ch, gmpmedia.win_handle_tv );
			
        g_free ( file_ch );
    }
}

static void gmp_create_columns ( GtkTreeView *tree_view, GtkTreeViewColumn *column, GtkCellRenderer *renderer, const gchar *name, gint column_id, gboolean col_vis )
{
    column = gtk_tree_view_column_new_with_attributes ( name, renderer, "text", column_id, NULL );
    gtk_tree_view_append_column ( tree_view, column );
    gtk_tree_view_column_set_visible ( column, col_vis );
}
static void gmp_add_columns ( GtkTreeView *tree_view, struct trw_columns sw_col_n[], guint num )
{
    GtkTreeViewColumn *column_n[num];
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();

	guint c = 0;
    for ( c = 0; c < num; c++ )
        gmp_create_columns ( tree_view, column_n[c], renderer, sw_col_n[c].title, c, sw_col_n[c].visible );
}

static GtkScrolledWindow * gmp_treeview ( GtkTreeView *tree_view, struct trw_columns sw_col_n[], guint num )
{
    GtkScrolledWindow *scroll_win = (GtkScrolledWindow *)gtk_scrolled_window_new ( NULL, NULL );
    gtk_scrolled_window_set_policy ( scroll_win, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
    gtk_widget_set_size_request ( GTK_WIDGET ( scroll_win ), 200, -1 );

    gtk_tree_view_set_search_column ( GTK_TREE_VIEW ( tree_view ), COL_TITLE );
    gmp_add_columns ( tree_view, sw_col_n, num );

    g_signal_connect ( tree_view, "row-activated", G_CALLBACK ( gmp_tree_view_row_activated ), NULL );
    gtk_container_add ( GTK_CONTAINER ( scroll_win ), GTK_WIDGET ( tree_view ) );

    return scroll_win;
}

// ***** GtkScrolledWindow *****



// ***** GdkEventButton *****

static gboolean gmp_media_press_event_trw ( GtkTreeView *tree_view, GdkEventButton *event )
{
	g_debug ( "gmp_media_press_event_trw" );
	
    if ( event->button == 3 )
    {
		gmp_media_panel_tree_view ( tree_view );

        return TRUE;
    }

    return FALSE;
}

static gboolean gmp_media_press_event ( GtkDrawingArea *widget, GdkEventButton *event, GtkBox *vbox )
{
	g_debug ( "gmp_media_press_event:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( widget ) ) );

    if ( event->button == 1 && event->type == GDK_2BUTTON_PRESS )
    {
        if ( gmp_base_flscr () ) gtk_widget_hide ( GTK_WIDGET ( vbox ) );

        return TRUE;
    }

    if ( event->button == 2 )
    {	
		if ( gmpmedia.media_tv_pl )
		{
			gtk_widget_get_visible ( GTK_WIDGET ( vbox ) ) ? gtk_widget_hide ( GTK_WIDGET ( vbox ) ) : gtk_widget_show ( GTK_WIDGET ( vbox ) );
		}
		else
		{
			gmp_player_get_current_state_playing () ? gmp_player_playback_pause () : gmp_player_playback_start ();
		}

        return TRUE;
    }

    if ( event->button == 3 )
    {
		gmpmedia.media_tv_pl ? gmp_media_panel_tv ( vbox ) : gmp_media_panel_pl ( vbox );

        return TRUE;
    }

    return FALSE;
}

static gboolean gmp_media_scroll_event ( GtkDrawingArea *widget, GdkEventScroll *evscroll )
{
	g_debug ( "gmp_media_scroll_event:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( widget ) ) );
	
	return gmp_player_scroll ( evscroll );

    return FALSE;
}

// ***** GdkEventButton *****



// ***** GtkDrawingArea *****

static void gmp_media_draw_black ( GtkDrawingArea *widget, cairo_t *cr, GdkPixbuf *logo )
{
    GdkRGBA color; color.red = 0; color.green = 0; color.blue = 0; color.alpha = 1.0;

    gint width  = gtk_widget_get_allocated_width  ( GTK_WIDGET ( widget ) );
    gint height = gtk_widget_get_allocated_height ( GTK_WIDGET ( widget ) );

    gint widthl  = gdk_pixbuf_get_width  ( logo );
    gint heightl = gdk_pixbuf_get_height ( logo );

    cairo_rectangle ( cr, 0, 0, width, height );
    gdk_cairo_set_source_rgba ( cr, &color );
    cairo_fill (cr);

    if ( logo != NULL )
    {
        cairo_rectangle ( cr, 0, 0, width, height );
        gdk_cairo_set_source_pixbuf ( cr, logo,
            ( width / 2  ) - ( widthl  / 2 ),
            ( height / 2 ) - ( heightl / 2 ) );

        cairo_fill (cr);
    }
}

static gboolean gmp_media_draw_callback ( GtkDrawingArea *widget, cairo_t *cr, GdkPixbuf *logo )
{
	if ( !gmpmedia.media_tv_pl && gmp_player_draw_logo () )
		{ gmp_media_draw_black ( widget, cr, logo ); return TRUE; }
	
	if ( gmpmedia.media_tv_pl && gmp_dvb_draw_logo () )
		{ gmp_media_draw_black ( widget, cr, logo ); return TRUE; }

    return FALSE;
}


static void gmp_media_video_window_realize ( GtkDrawingArea *draw, gchar *text )
{
    gulong xid = GDK_WINDOW_XID ( gtk_widget_get_window ( GTK_WIDGET ( draw ) ) );

	g_print ( "%s xid: %ld \n", text, xid );	

	if ( g_str_has_prefix ( text, "tv" ) )
		gmpmedia.win_handle_tv = xid;
	else
		gmpmedia.win_handle_pl = xid;

}

// ***** GtkDrawingArea *****



static void gmp_media_hide_box ( GtkButton *button, GtkBox *box )
{
	gtk_widget_hide ( GTK_WIDGET ( box ) );

	g_debug ( "gmp_media_hide_box:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

void gmp_media_update_box_pl ()
{
	g_debug ( "gmp_media_update_box_pl" );
	
	gtk_widget_hide ( GTK_WIDGET ( gmpmedia.sw_vbox_pl ) );
	gtk_widget_hide ( GTK_WIDGET ( gmpmedia.hbox_base ) );

	if ( gmp_player_get_current_state_stop () )
		gmp_media_set_sensitive_pl ( FALSE, FALSE );
}

static void gmp_media_init ()
{
	gmpmedia.win_handle_tv = 0;
	gmpmedia.win_handle_pl = 0;
	gmpmedia.firstfile = FALSE;
	gmpmedia.rec_trig_vis = TRUE;
	gmpmedia.all_info = FALSE;
	
	gmpmedia.slider_signal_id = 0;
	gmpmedia.slider_signal_id_m = 0;
	gmpmedia.slider_signal_id_vol = 0;
}

void gmp_media_win ( GtkBox *box, GdkPixbuf *logo, gboolean set_tv_pl, struct trw_columns sw_col_n[], guint num )
{
	gmp_media_init ();
	
	gchar *text = "tv";
	if ( !set_tv_pl ) text = "pl";

	if ( set_tv_pl )
		gmp_dvb_gst_create_tv ();
	else
		gmp_player_gst_create_pl ();

	GtkBox *sw_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

    GtkDrawingArea *video_window = (GtkDrawingArea *)gtk_drawing_area_new ();
    g_signal_connect ( video_window, "realize", G_CALLBACK ( gmp_media_video_window_realize ), text );
    g_signal_connect ( video_window, "draw",    G_CALLBACK ( gmp_media_draw_callback ), logo );

    gtk_widget_add_events ( GTK_WIDGET ( video_window ), GDK_BUTTON_PRESS_MASK | GDK_SCROLL_MASK  );
    g_signal_connect ( video_window, "button-press-event", G_CALLBACK ( gmp_media_press_event  ), sw_vbox );
	g_signal_connect ( video_window, "scroll-event",       G_CALLBACK ( gmp_media_scroll_event ), NULL );


    GtkListStore *liststore   = (GtkListStore *)gtk_list_store_new ( 3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING );
    GtkTreeView *tree_view = (GtkTreeView *)gtk_tree_view_new_with_model ( GTK_TREE_MODEL ( liststore ) );
	g_signal_connect ( tree_view, "button-press-event", G_CALLBACK ( gmp_media_press_event_trw  ), NULL );

    GtkScrolledWindow *scroll_win = gmp_treeview ( tree_view, sw_col_n, num );
	gtk_box_pack_start ( sw_vbox, GTK_WIDGET ( scroll_win ), TRUE,  TRUE,  0 );

	if ( !set_tv_pl )
	{
		gtk_box_pack_start ( sw_vbox, GTK_WIDGET ( gmp_media_spinner_pl () ), FALSE, FALSE, 2 );
	}
	
	GtkBox *hb_box   = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gmp_media_buttons ( hb_box, "gmp-exit", 16, gmp_media_hide_box,  GTK_WIDGET ( sw_vbox ) );
  	gtk_box_pack_end ( sw_vbox, GTK_WIDGET ( hb_box ), FALSE, FALSE, 5 );


    GtkPaned *hpaned = (GtkPaned *)gtk_paned_new ( GTK_ORIENTATION_HORIZONTAL );
    gtk_paned_add1 ( hpaned, GTK_WIDGET ( sw_vbox )      );
    gtk_paned_add2 ( hpaned, GTK_WIDGET ( video_window ) );


	GtkBox *main_hbox   = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
    
	gtk_box_pack_start ( main_hbox, GTK_WIDGET ( hpaned ),    TRUE,  TRUE,  0 );
	gtk_box_pack_start ( box,       GTK_WIDGET ( main_hbox ), TRUE,  TRUE,  0 );
	
	if ( set_tv_pl )
	{
		gtk_box_pack_start ( sw_vbox, GTK_WIDGET ( gmp_create_sgn_snr () ), FALSE, FALSE, 0 );
	}
	else
	{
		gmpmedia.lab_pos = (GtkLabel *)gtk_label_new ( "0:00:00" );
		gmpmedia.lab_dur = (GtkLabel *)gtk_label_new ( "0:00:00" );	
		gmpmedia.slider_base = (GtkScale *)gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0, 100, 1 );
		gmpmedia.slider_signal_id = g_signal_connect ( gmpmedia.slider_base, "value-changed", G_CALLBACK ( gmp_slider_changed ), NULL );
		
		gmpmedia.hbox_base = gmp_media_slider_base_pl ( gmpmedia.slider_base, gmpmedia.lab_pos, gmpmedia.lab_dur, TRUE );
		
		gtk_box_pack_start ( box, GTK_WIDGET ( gmpmedia.hbox_base ), FALSE, FALSE, 0 );	
	}

    gtk_drag_dest_set ( GTK_WIDGET ( video_window ), GTK_DEST_DEFAULT_ALL, NULL, 0, GDK_ACTION_COPY );
    gtk_drag_dest_add_uri_targets  ( GTK_WIDGET ( video_window ) );
    //gtk_drag_dest_add_text_targets ( GTK_WIDGET ( video_window ) );
    g_signal_connect  ( video_window, "drag-data-received", G_CALLBACK ( gmp_media_drag_in ), tree_view );
	
	if ( set_tv_pl )
	{
		gmpmedia.treeview_tv = tree_view;
		gmpmedia.sw_vbox_tv = sw_vbox;
		
		gmp_get_dvb_info ( FALSE, 0, 0 );
		
		if ( g_file_test ( ch_conf, G_FILE_TEST_EXISTS ) )
			gmp_channels_to_treeview ( ch_conf );
	}	
	else
	{
		gmpmedia.treeview_pl = tree_view;
		gmpmedia.sw_vbox_pl = sw_vbox;
	}
	
	g_debug ( "gmp_media_win" );
}

void gmp_media_set_tv ()
{
	gmpmedia.media_tv_pl = TRUE;

	g_debug ( "gmp_media_set_tv:: gmpmedia.media_tv_pl = TRUE" );
}

void gmp_media_set_mp ()
{
	gmpmedia.media_tv_pl = FALSE;

	g_debug ( "gmp_media_set_mp:: gmpmedia.media_tv_pl = FALSE" );
}
