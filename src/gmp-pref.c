/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/


#include "gmp-pref.h"
#include "gmp-dvb.h"

#include <glib/gi18n.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



static gchar * gmp_pref_get_prop ( const gchar *prop );

void gmp_message_dialog ( gchar *f_error, gchar *file_or_info, GtkMessageType mesg_type )
{
	g_debug ( "gmp_message_dialog" );
	
    GtkMessageDialog *dialog = ( GtkMessageDialog *)gtk_message_dialog_new (
                                 NULL, GTK_DIALOG_MODAL,
                                 mesg_type,   GTK_BUTTONS_CLOSE,
                                 "%s\n %s",   f_error, file_or_info );

    gtk_dialog_run     ( GTK_DIALOG ( dialog ) );
    gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

gchar * gmp_pref_get_time_date_str ()
{
    GDateTime *datetime = g_date_time_new_now_local ();

    gint doy = g_date_time_get_day_of_year ( datetime );
    gint tth = g_date_time_get_hour   ( datetime );
    gint ttm = g_date_time_get_minute ( datetime );
    gint tts = g_date_time_get_second ( datetime );
    
    gchar *dt = g_strdup_printf ( "%d-%d-%d-%d", doy, tth, ttm, tts );

	g_debug ( "gmp_pref_get_time_date_str:: dt %s", dt );
    
    g_date_time_unref ( datetime );

    return dt;
}

void gmp_add_filter ( GtkFileChooserDialog *dialog, const gchar *name, const gchar *filter )
{
    GtkFileFilter *filter_video = gtk_file_filter_new ();
    gtk_file_filter_set_name ( filter_video, name );
    gtk_file_filter_add_pattern ( filter_video, filter );
    gtk_file_chooser_add_filter ( GTK_FILE_CHOOSER ( dialog ), filter_video );
}

gchar * gmp_pref_open_file ( const gchar *path, const gchar *filter_name, const gchar *filter_file )
{
	g_debug ( "gmp_pref_open_file" );

    GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
                    _("Choose file"),  NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                    "gtk-cancel", GTK_RESPONSE_CANCEL,
                    "gtk-open",   GTK_RESPONSE_ACCEPT,
                     NULL );

	if ( filter_name )
		gmp_add_filter ( dialog, filter_name, filter_file );

    gtk_file_chooser_set_current_folder  ( GTK_FILE_CHOOSER ( dialog ), path );
    gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ), FALSE );

    gchar *filename = NULL;

    if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
        filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

    gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

    return filename;
}

void gmp_pref_read_config ( const gchar *file )
{
    guint n = 0;
    gchar *contents;
    GError *err = NULL;

    if ( g_file_get_contents ( file, &contents, 0, &err ) )
    {
        gchar **lines = g_strsplit ( contents, "\n", 0 );

        for ( n = 0; lines[n] != NULL; n++ )
        {
            if ( !g_strrstr ( lines[n], "=" ) ) continue;

            gchar **key_val = g_strsplit ( lines[n], "=", 0 );

                if ( g_strrstr ( lines[n], "theme" ) )
                    g_object_set ( gtk_settings_get_default (), key_val[0], key_val[1], NULL );

				if ( g_strrstr ( lines[n], "opacity-control" ) )
                    opacity_cnt = (double)( atoi ( key_val[1] ) ) / 100;

				if ( g_strrstr ( lines[n], "opacity-equalizer" ) )
                    opacity_eq = (double)( atoi ( key_val[1] ) ) / 100;

				if ( g_strrstr ( lines[n], "opacity-window" ) )
                    opacity_win = (double)( atoi ( key_val[1] ) ) / 100;

				if ( g_strrstr ( lines[n], "resize-icon" ) )
                    resize_icon = atoi ( key_val[1] );

                g_debug ( "gmp_pref_read_config:: Set %s -> %s", key_val[0], key_val[1]);

            g_strfreev ( key_val );
        }

        g_strfreev ( lines );
        g_free ( contents );
    }
    else
    {
        g_critical ( "gmp_pref_read_config:: %s\n", err->message );
        g_error_free ( err );
	}

	g_debug ( "gmp_pref_read_config" );
}

void gmp_pref_save_config ( const gchar *file )
{
    gchar *gmp_conf_t = gmp_pref_get_prop ( "gtk-theme-name" );
    gchar *gmp_conf_i = gmp_pref_get_prop ( "gtk-icon-theme-name" );

    GString *gstring = g_string_new ( "# Gtv-Dvb conf \n" );

        g_string_append_printf ( gstring, "gtk-theme-name=%s\n",      gmp_conf_t  );
        g_string_append_printf ( gstring, "gtk-icon-theme-name=%s\n", gmp_conf_i  );
		g_string_append_printf ( gstring, "opacity-control=%d\n",     (int)( opacity_cnt * 100 ) );
		g_string_append_printf ( gstring, "opacity-equalizer=%d\n",   (int)( opacity_eq  * 100 ) );
		g_string_append_printf ( gstring, "opacity-window=%d\n",      (int)( opacity_win * 100 ) );
		g_string_append_printf ( gstring, "resize-icon=%d\n",         resize_icon );


		GError *err = NULL;

        if ( !g_file_set_contents ( file, gstring->str, -1, &err ) )
		{
			g_critical ( "gmp_pref_save_config:: %s\n", err->message );
			g_error_free ( err );
		}

    g_string_free ( gstring, TRUE );

    g_free ( gmp_conf_i );
    g_free ( gmp_conf_t );

	g_debug ( "gmp_pref_save_config" );
}

static gchar * gmp_pref_open_dir ( const gchar *path )
{
	g_debug ( "gmp_pref_open_dir" );

    GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
                    _("Choose folder"),  NULL, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                    "gtk-cancel", GTK_RESPONSE_CANCEL,
                    "gtk-apply",  GTK_RESPONSE_ACCEPT,
                     NULL );

    gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), path );

    gchar *dirname = NULL;

    if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
        dirname = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

    gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

    return dirname;
}

static void gmp_pref_set_rec_dir ( GtkEntry *entry )
{
	gchar *path = gmp_pref_open_dir ( g_get_home_dir () );

	if ( path == NULL ) return;

	if ( gmp_rec_dir ) g_free ( gmp_rec_dir );

    gmp_rec_dir = path;

	gtk_entry_set_text ( entry, gmp_rec_dir );

	g_debug ( "gmp_pref_set_rec_dir" );
}

static gchar * gmp_pref_get_prop ( const gchar *prop )
{
	g_debug ( "gmp_pref_get_prop" );

    gchar *name = NULL;
	g_object_get ( gtk_settings_get_default (), prop, &name, NULL );

    return name;
}
static void gmp_pref_set_prop ( const gchar *prop, gchar *path )
{
    gchar *i_file = g_strconcat ( path, "/index.theme", NULL );

        if ( g_file_test ( i_file, G_FILE_TEST_EXISTS ) )
        {
            gchar *name = g_path_get_basename ( path );
                g_object_set ( gtk_settings_get_default (), prop, name, NULL );
            g_free ( name );
        }

    g_free ( i_file );

	g_debug ( "gmp_pref_set_prop" );
}
static void gmp_pref_set_theme ( GtkEntry *entry )
{
    gchar *path = gmp_pref_open_dir ( "/usr/share/themes" );

	if ( path == NULL ) return;

	gmp_pref_set_prop ( "gtk-theme-name", path );

    gchar *name = g_path_get_basename ( path );
    	gtk_entry_set_text ( entry, name );
	g_free ( name );

    g_free ( path );

	g_debug ( "gmp_pref_set_theme" );
}
static void gmp_pref_set_icon ( GtkEntry *entry )
{
    gchar *path = gmp_pref_open_dir ( "/usr/share/icons" );

	if ( path == NULL ) return;

	gmp_pref_set_prop ( "gtk-icon-theme-name", path );

    gchar *name = g_path_get_basename ( path );
    	gtk_entry_set_text ( entry, name );
	g_free ( name );

	g_debug ( "gmp_pref_set_icon" );
}


static void gmp_pref_quit ( GtkWindow *window )
{
    gtk_widget_destroy ( GTK_WIDGET ( window ) );

	g_debug ( "gmp_pref_quit" );
}
static void gmp_pref_close ( GtkButton *button, GtkWindow *window )
{
    gmp_pref_quit ( window );

	g_debug ( "gmp_pref_close:: widget name %s", gtk_widget_get_name ( GTK_WIDGET ( button ) ) );
}

static void gmp_changed_opacity_cnt ( GtkRange *range )
{
    opacity_cnt = gtk_range_get_value ( range );

	g_debug ( "gmp_changed_opacity_cnt" );
}
static void gmp_changed_opacity_eq ( GtkRange *range )
{
    opacity_eq = gtk_range_get_value ( range );

	g_debug ( "gmp_changed_opacity_eq" );
}
static void gmp_changed_opacity_win ( GtkRange *range, gpointer data )
{
    opacity_win = gtk_range_get_value ( range );
    gtk_widget_set_opacity ( GTK_WIDGET ( data ), opacity_win );

	g_debug ( "gmp_changed_opacity_win" );
}
static void gmp_changed_opacity_base_win ( GtkRange *range, gpointer data )
{
    gdouble opacity = gtk_range_get_value ( range );
    gtk_widget_set_opacity ( GTK_WIDGET ( data ), opacity );

	g_debug ( "gmp_changed_opacity_base_win" );
}
static void gmp_changed_resize_icon ( GtkRange *range )
{
    resize_icon = (guint)gtk_range_get_value ( range );

	g_debug ( "gmp_changed_resize_icon" );
}

static GtkBox * gmp_pref_set_data ( GtkWindow *window, GtkWindow *base_window )
{
	g_debug ( "gmp_pref_set_data" );

    GtkBox *g_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
    gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 10 );
    gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 10 );

    GtkGrid *grid = (GtkGrid *)gtk_grid_new();
    gtk_grid_set_column_homogeneous ( GTK_GRID ( grid ), TRUE );
    gtk_box_pack_start ( g_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

    struct data_a { gchar *image; GtkWidget *widget; gchar *etext; void (* activate); gboolean icon_set; gdouble opacity; }
    data_a_n[] =
    {
		{ " ", gtk_label_new (" "), NULL, NULL, FALSE, 0 },
        { "gmp-theme",      gtk_entry_new  (), gmp_pref_get_prop ( "gtk-theme-name"      ), gmp_pref_set_theme,   TRUE, 0 },
        { "gmp-icons",      gtk_entry_new  (), gmp_pref_get_prop ( "gtk-icon-theme-name" ), gmp_pref_set_icon,    TRUE, 0 },
		{ "gmp-folder-rec", gtk_entry_new  (), gmp_rec_dir,                                 gmp_pref_set_rec_dir, TRUE, 0 },
		
		{ " ", gtk_label_new (" "), NULL, NULL, FALSE, 0 },
		{ "gmp-panel",   gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0.4, 1.0, 0.01 ), NULL, gmp_changed_opacity_cnt, FALSE, opacity_cnt },
		{ "gmp-eqav",    gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0.4, 1.0, 0.01 ), NULL, gmp_changed_opacity_eq,  FALSE, opacity_eq  },
		{ "gmp-display", gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0.4, 1.0, 0.01 ), NULL, gmp_changed_opacity_win, FALSE, opacity_win },

		{ " ", gtk_label_new (" "), NULL, NULL, FALSE, 0 },
		{ "gmp-resize",  gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 16,  48,  1    ), NULL, gmp_changed_resize_icon, FALSE, resize_icon },
		{ " ", gtk_label_new (" "), NULL, NULL, FALSE, 0 }
    };

    guint d = 0;
    for ( d = 0; d < G_N_ELEMENTS ( data_a_n ); d++ )
    {
		if ( d == 0 || d == 4 || d == 8 || d == 10 )
		{
			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( data_a_n[d].widget ), 0, d, 1, 1 );
			continue;
		}
		
		if ( d == 5 || d == 6 || d == 7 || d == 9 )
		{
			GtkImage *image = (GtkImage  *)gtk_image_new_from_icon_name ( data_a_n[d].image, GTK_ICON_SIZE_DIALOG );
			
			gtk_range_set_value ( GTK_RANGE ( data_a_n[d].widget ), data_a_n[d].opacity );
			
			if ( d == 7 )
			{
				g_signal_connect ( data_a_n[d].widget, "value-changed", G_CALLBACK ( data_a_n[d].activate ),              window );
				g_signal_connect ( data_a_n[d].widget, "value-changed", G_CALLBACK ( gmp_changed_opacity_base_win ), base_window );
			}
			else
				g_signal_connect ( data_a_n[d].widget, "value-changed", G_CALLBACK ( data_a_n[d].activate ), NULL );
				
			GtkBox *h_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
			gtk_box_pack_start ( h_box, GTK_WIDGET ( image ), FALSE, FALSE, 0 );
			gtk_box_pack_start ( h_box, GTK_WIDGET ( data_a_n[d].widget ), TRUE, TRUE, 5 );
			
			gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( h_box ), 0, d, 2, 1 );
			
			continue;
		}
		
		GtkImage *image = (GtkImage  *)gtk_image_new_from_icon_name ( data_a_n[d].image, GTK_ICON_SIZE_DIALOG );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( image ), 0, d, 1, 1 );
		gtk_widget_set_halign ( GTK_WIDGET ( image ), GTK_ALIGN_START );

        if ( data_a_n[d].etext )
        {
            gtk_entry_set_text ( GTK_ENTRY ( data_a_n[d].widget ), data_a_n[d].etext );
            gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( data_a_n[d].widget ), 1, d, 1, 1 );
        }

        if ( data_a_n[d].icon_set )
        {
            g_object_set ( data_a_n[d].widget, "editable", FALSE, NULL );
            gtk_entry_set_icon_from_icon_name ( GTK_ENTRY ( data_a_n[d].widget ), GTK_ENTRY_ICON_SECONDARY, "folder" );
            g_signal_connect ( data_a_n[d].widget, "icon-press", G_CALLBACK ( data_a_n[d].activate ), NULL );
        }
    }

    return g_box;
}

void gmp_pref_win ( GtkWindow *base_window )
{
    GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
    gtk_window_set_transient_for ( window, gmp_base_win_ret () );
    gtk_window_set_modal     ( window, TRUE );
    gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
    gtk_window_set_title     ( window, _("Preferences" ) );
	//g_signal_connect         ( window, "destroy", G_CALLBACK ( gmp_pref_quit ), NULL );

	GdkPixbuf *icon = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
                      "gmp-pref", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	gtk_window_set_icon ( window, icon );


    GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
    GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

    GtkBox *main_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

    gtk_box_pack_start ( main_box, GTK_WIDGET ( gmp_pref_set_data ( window, base_window ) ), TRUE, TRUE, 0 );

    gtk_box_pack_start ( m_box, GTK_WIDGET ( main_box ), TRUE, TRUE, 0 );

    GtkButton *button_close = (GtkButton *)gtk_button_new_from_icon_name ( "gmp-exit", GTK_ICON_SIZE_BUTTON );
    g_signal_connect ( button_close, "clicked", G_CALLBACK ( gmp_pref_close ), window );
    gtk_box_pack_end ( h_box, GTK_WIDGET ( button_close ), TRUE, TRUE, 10 );

    gtk_box_pack_end ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

    gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
    gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
    gtk_widget_show_all ( GTK_WIDGET ( window ) );
    
    gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity_win );

	g_debug ( "gmp_pref_win" );
}
