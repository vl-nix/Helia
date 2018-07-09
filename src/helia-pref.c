/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia-pref.h"
#include "helia-base.h"
#include "helia-service.h"

#include <glib/gi18n.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



static gchar * helia_pref_get_prop ( const gchar *prop );

void helia_pref_read_config ( const gchar *file )
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
				hbgv.opacity_cnt = (double)( atoi ( key_val[1] ) ) / 100;

			if ( g_strrstr ( lines[n], "opacity-equalizer" ) )
				hbgv.opacity_eq = (double)( atoi ( key_val[1] ) ) / 100;

			if ( g_strrstr ( lines[n], "opacity-window" ) )
				hbgv.opacity_win = (double)( atoi ( key_val[1] ) ) / 100;

			if ( g_strrstr ( lines[n], "resize-icon" ) )
				hbgv.resize_icon = atoi ( key_val[1] );

			// g_debug ( "helia_pref_read_config:: Set %s -> %s", key_val[0], key_val[1]);

			g_strfreev ( key_val );
		}

		g_strfreev ( lines );
		g_free ( contents );
	}
	else
	{
		g_critical ( "helia_pref_read_config:: %s\n", err->message );
		g_error_free ( err );
	}

	g_debug ( "helia_pref_read_config" );
}

void helia_pref_save_config ( const gchar *file )
{
	gchar *helia_conf_t = helia_pref_get_prop ( "gtk-theme-name" );
	gchar *helia_conf_i = helia_pref_get_prop ( "gtk-icon-theme-name" );

	GString *gstring = g_string_new ( "# Gtv-Dvb conf \n" );

	g_string_append_printf ( gstring, "gtk-theme-name=%s\n",      helia_conf_t  );
	g_string_append_printf ( gstring, "gtk-icon-theme-name=%s\n", helia_conf_i  );
	g_string_append_printf ( gstring, "opacity-control=%d\n",     (int)( hbgv.opacity_cnt * 100 ) );
	g_string_append_printf ( gstring, "opacity-equalizer=%d\n",   (int)( hbgv.opacity_eq  * 100 ) );
	g_string_append_printf ( gstring, "opacity-window=%d\n",      (int)( hbgv.opacity_win * 100 ) );
	g_string_append_printf ( gstring, "resize-icon=%d\n",         hbgv.resize_icon );


	GError *err = NULL;

	if ( !g_file_set_contents ( file, gstring->str, -1, &err ) )
	{
		g_critical ( "helia_pref_save_config:: %s\n", err->message );
		g_error_free ( err );
	}

	g_string_free ( gstring, TRUE );

	g_free ( helia_conf_i );
	g_free ( helia_conf_t );

	g_debug ( "helia_pref_save_config" );
}


static void helia_pref_set_rec_dir ( GtkEntry *entry )
{
	gchar *path = helia_service_open_dir ( g_get_home_dir () );

	if ( path == NULL ) return;

	if ( hbgv.rec_dir ) g_free ( hbgv.rec_dir );

	hbgv.rec_dir = path;

	gtk_entry_set_text ( entry, hbgv.rec_dir );

	g_debug ( "helia_pref_set_rec_dir" );
}

static gchar * helia_pref_get_prop ( const gchar *prop )
{
	g_debug ( "helia_pref_get_prop" );

	gchar *name = NULL;
	g_object_get ( gtk_settings_get_default (), prop, &name, NULL );

	return name;
}
static void helia_pref_set_prop ( const gchar *prop, gchar *path )
{
	gchar *i_file = g_strconcat ( path, "/index.theme", NULL );

	if ( g_file_test ( i_file, G_FILE_TEST_EXISTS ) )
	{
		gchar *name = g_path_get_basename ( path );
			g_object_set ( gtk_settings_get_default (), prop, name, NULL );
		g_free ( name );
	}

	g_free ( i_file );

	g_debug ( "helia_pref_set_prop" );
}
static void helia_pref_set_theme ( GtkEntry *entry )
{
	gchar *path = helia_service_open_dir ( "/usr/share/themes" );

	if ( path == NULL ) return;

	helia_pref_set_prop ( "gtk-theme-name", path );

	gchar *name = g_path_get_basename ( path );
		gtk_entry_set_text ( entry, name );
	g_free ( name );

	g_free ( path );

	g_debug ( "helia_pref_set_theme" );
}
static void helia_pref_set_icon ( GtkEntry *entry )
{
	gchar *path = helia_service_open_dir ( "/usr/share/icons" );

	if ( path == NULL ) return;

	helia_pref_set_prop ( "gtk-icon-theme-name", path );

	gchar *name = g_path_get_basename ( path );
		gtk_entry_set_text ( entry, name );
	g_free ( name );

	g_debug ( "helia_pref_set_icon" );
}


static void helia_pref_changed_opacity_cnt ( GtkRange *range )
{
	hbgv.opacity_cnt = gtk_range_get_value ( range );

	g_debug ( "helia_pref_changed_opacity_cnt" );
}
static void helia_pref_changed_opacity_eq ( GtkRange *range )
{
	hbgv.opacity_eq = gtk_range_get_value ( range );

	g_debug ( "helia_pref_changed_opacity_eq" );
}
static void helia_pref_changed_opacity_win ( GtkRange *range, gpointer data )
{
	hbgv.opacity_win = gtk_range_get_value ( range );
	gtk_widget_set_opacity ( GTK_WIDGET ( data ), hbgv.opacity_win );

	g_debug ( "helia_pref_changed_opacity_win" );
}
static void helia_pref_changed_opacity_base_win ( GtkRange *range, gpointer data )
{
	gdouble opacity = gtk_range_get_value ( range );
	gtk_widget_set_opacity ( GTK_WIDGET ( data ), opacity );

	g_debug ( "helia_pref_changed_opacity_base_win" );
}
static void helia_pref_changed_resize_icon ( GtkRange *range )
{
	hbgv.resize_icon = (guint)gtk_range_get_value ( range );

	g_debug ( "helia_pref_changed_resize_icon" );
}

static GtkBox * helia_pref_set_data ( GtkWindow *window, GtkWindow *base_window )
{
	g_debug ( "helia_pref_set_data" );

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
		{ "helia-theme",      gtk_entry_new  (), helia_pref_get_prop ( "gtk-theme-name"      ), helia_pref_set_theme,   TRUE, 0 },
		{ "helia-icons",      gtk_entry_new  (), helia_pref_get_prop ( "gtk-icon-theme-name" ), helia_pref_set_icon,    TRUE, 0 },
		{ "helia-folder-rec", gtk_entry_new  (), hbgv.rec_dir,                                  helia_pref_set_rec_dir, TRUE, 0 },

		{ " ", gtk_label_new (" "), NULL, NULL, FALSE, 0 },
		{ "helia-panel",   gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0.4, 1.0, 0.01 ), NULL, helia_pref_changed_opacity_cnt, FALSE, hbgv.opacity_cnt },
		{ "helia-eqa",     gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0.4, 1.0, 0.01 ), NULL, helia_pref_changed_opacity_eq,  FALSE, hbgv.opacity_eq  },
		{ "helia-display", gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 0.4, 1.0, 0.01 ), NULL, helia_pref_changed_opacity_win, FALSE, hbgv.opacity_win },

		{ " ", gtk_label_new (" "), NULL, NULL, FALSE, 0 },
		{ "helia-resize",  gtk_scale_new_with_range ( GTK_ORIENTATION_HORIZONTAL, 16,  48,  1    ), NULL, helia_pref_changed_resize_icon, FALSE, hbgv.resize_icon },
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
				g_signal_connect ( data_a_n[d].widget, "value-changed", G_CALLBACK ( data_a_n[d].activate ),                window );
				g_signal_connect ( data_a_n[d].widget, "value-changed", G_CALLBACK ( helia_pref_changed_opacity_base_win ), base_window );
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

static GtkBox * helia_accels_win_set_data ()
{
	g_debug ( "helia_accels_win_set_data" );

	GtkBox *g_box  = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_margin_start ( GTK_WIDGET ( g_box ), 10 );
	gtk_widget_set_margin_end   ( GTK_WIDGET ( g_box ), 10 );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new();
	gtk_grid_set_column_homogeneous ( GTK_GRID ( grid ), TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( grid ), TRUE, TRUE, 10 );

	const struct data_a { const gchar *image; const gchar *accel; } data_a_n[] =
	{
		{ "helia-editor",		"Ctrl + L" },
		{ "helia-muted", 		"Ctrl + M" },
		{ "helia-media-stop", 	"Ctrl + X" },
		{ "helia-about", 	    "Ctrl + I" },
		{ NULL, NULL },
		{ "helia-media-start",  "␣"        },
		{ "helia-media-pause",  "␣"        },
		{ "helia-frame", 		"."        },
		{ "helia-slider", 		"Ctrl + H" },
	};

	guint d = 0;
	for ( d = 0; d < G_N_ELEMENTS ( data_a_n ); d++ )
	{
		if ( ! data_a_n[d].image )
		{
			gtk_grid_attach ( GTK_GRID ( grid ), gtk_label_new ( "" ), 0, d, 1, 1 );
			continue;
		}
		
		GtkImage *image = (GtkImage  *)gtk_image_new_from_icon_name ( data_a_n[d].image, GTK_ICON_SIZE_DIALOG );
		gtk_widget_set_halign ( GTK_WIDGET ( image ), GTK_ALIGN_START );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( image ), 0, d, 1, 1 );
		
		GtkLabel *label = (GtkLabel *)gtk_label_new ( data_a_n[d].accel );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_END );
		gtk_grid_attach ( GTK_GRID ( grid ), GTK_WIDGET ( label ), 1, d, 1, 1 );
	}

	return g_box;
}

void helia_pref_win ( GtkWindow *base_window )
{
	GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window, helia_base_win_ret () );
	gtk_window_set_modal     ( window, TRUE );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_title     ( window, _("Preferences") );

	GdkPixbuf *icon = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					 "helia-pref", 48, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	gtk_window_set_icon ( window, icon );


	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	GtkBox *main_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	GtkNotebook *notebook = (GtkNotebook *)gtk_notebook_new ();
	gtk_notebook_set_tab_pos ( notebook, GTK_POS_TOP );
	gtk_notebook_set_scrollable ( notebook, TRUE );

	gtk_widget_set_margin_top    ( GTK_WIDGET ( notebook ), 5 );
	gtk_widget_set_margin_bottom ( GTK_WIDGET ( notebook ), 5 );
	gtk_widget_set_margin_start  ( GTK_WIDGET ( notebook ), 5 );
	gtk_widget_set_margin_end    ( GTK_WIDGET ( notebook ), 5 );
	
	gtk_notebook_append_page ( notebook, GTK_WIDGET ( helia_pref_set_data ( window, base_window ) ),  gtk_label_new ( _("General" ) ) );
	gtk_notebook_append_page ( notebook, GTK_WIDGET ( helia_accels_win_set_data () ),                 gtk_label_new ( _("Shortcut") ) );
	
	gtk_box_pack_start ( main_box, GTK_WIDGET (notebook), TRUE, TRUE, 0 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( main_box ), TRUE, TRUE, 0 );

	GtkButton *button_close = (GtkButton *)gtk_button_new_from_icon_name ( "helia-exit", GTK_ICON_SIZE_BUTTON );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );
	gtk_box_pack_end ( h_box, GTK_WIDGET ( button_close ), TRUE, TRUE, 5 );

	gtk_box_pack_end ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
	gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );
	gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), hbgv.opacity_win );

	g_debug ( "helia_pref_win" );
}
