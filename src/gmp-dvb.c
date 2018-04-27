/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/


#include "gmp-dvb.h"
#include "gmp-media.h"
#include "gmp-pref.h"

#include <locale.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>


struct GmpBase
{
	GtkWindow *main_window;
	GdkPixbuf *logo_image, *tv_logo, *mp_logo;
	GtkBox *mn_vbox, *bs_vbox, *tv_vbox, *mp_vbox;
	
	GSList *lists_argv;
};

struct GmpBase gmpbase;



static void gmp_about_win ( GtkWindow *window, GdkPixbuf *logo )
{
	g_debug ( "gmp_about_win" );

    GtkAboutDialog *dialog = (GtkAboutDialog *)gtk_about_dialog_new ();
    gtk_window_set_transient_for ( (GtkWindow *)dialog, window );

    const gchar *authors[]   = { "Stepan Perun", " ", NULL };
    const gchar *artists[]   = { "Itzik Gur", "Stepan Perun", " ", NULL };
	const gchar *translators = " \n";
    const gchar *license     = "This program is free software. \nGNU Lesser General Public License \nwww.gnu.org/licenses/lgpl.html";

    gtk_about_dialog_set_program_name ( dialog, "Helia" );
    gtk_about_dialog_set_version ( dialog, "2.5" );
    gtk_about_dialog_set_license ( dialog, license );
	gtk_about_dialog_set_authors ( dialog, authors );
	gtk_about_dialog_set_artists ( dialog, artists );
	gtk_about_dialog_set_translator_credits ( dialog, translators );
    gtk_about_dialog_set_website ( dialog,   "https://github.com/vl-nix/helia" );
    gtk_about_dialog_set_copyright ( dialog, "Copyright 2018 Helia  ( Gtv-Dvb )" );
    gtk_about_dialog_set_comments  ( dialog, "Media Player & IPTV & Digital TV \nDVB-T2/S2/C, ATSC, DTMB" );

    if ( logo != NULL )
    {	
        gtk_about_dialog_set_logo ( dialog, logo );
    }
	else
        gtk_about_dialog_set_logo_icon_name ( dialog, "applications-multimedia" );

    gtk_dialog_run ( GTK_DIALOG (dialog) );

    gtk_widget_destroy ( GTK_WIDGET (dialog) );
}

static void gmp_base_set_about ()
{
	gmp_about_win ( gmpbase.main_window, gmpbase.logo_image );

	g_debug ( "gmp_set_about" );
}

static void gmp_base_set_pref ()
{
	gmp_pref_win ( gmpbase.main_window );

	g_debug ( "gmp_base_set_pref" );
}

void gmp_base_update_win ()
{
	gtk_widget_queue_draw ( GTK_WIDGET ( gmpbase.main_window ) );

	g_debug ( "gmp_base_update_win" );
}

GtkWindow * gmp_base_win_ret ()
{
	g_debug ( "gmp_base_win_ret" );
	
	return gmpbase.main_window;
}

gboolean gmp_base_flscr ()
{
	g_debug ( "gmp_base_flscr" );

    GdkWindowState state = gdk_window_get_state ( gtk_widget_get_window ( GTK_WIDGET ( gmpbase.main_window ) ) );

    if ( state & GDK_WINDOW_STATE_FULLSCREEN )
        { gtk_window_unfullscreen ( gmpbase.main_window ); return FALSE; }
    else
        { gtk_window_fullscreen   ( gmpbase.main_window ); return TRUE; }

	return TRUE;
}

void gmp_base_set_window ()
{
	gtk_window_set_title ( gmpbase.main_window, "Media Player & Digital TV");

	gtk_widget_hide ( GTK_WIDGET ( gmpbase.mp_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( gmpbase.tv_vbox ) );
	gtk_widget_show ( GTK_WIDGET ( gmpbase.bs_vbox ) );

	g_debug ( "gmp_base_set_window" );
}

static void gmp_base_set_tv ()
{
	gtk_window_set_icon  ( gmpbase.main_window, gmpbase.tv_logo );
	gtk_window_set_title ( gmpbase.main_window, "Digital TV");

	gtk_widget_hide ( GTK_WIDGET ( gmpbase.bs_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( gmpbase.mp_vbox ) );
	gtk_widget_show ( GTK_WIDGET ( gmpbase.tv_vbox ) );

	gmp_media_set_tv ();

	g_debug ( "gmp_base_set_tv" );
}

static void gmp_base_set_mp ()
{
	gtk_window_set_icon  ( gmpbase.main_window, gmpbase.mp_logo );
	gtk_window_set_title ( gmpbase.main_window, "Media Player");

	gtk_widget_hide ( GTK_WIDGET ( gmpbase.bs_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( gmpbase.tv_vbox ) );
	gtk_widget_show ( GTK_WIDGET ( gmpbase.mp_vbox ) );

	gmp_media_set_mp ();

	g_debug ( "gmp_base_set_mp" );
}

static void gmp_base_button ( GtkBox *box, gchar *icon, guint size, void (* activate)() )
{
	GdkPixbuf *logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
              icon, size, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

  	GtkButton *button = (GtkButton *)gtk_button_new ();
  	GtkImage *image   = (GtkImage  *)gtk_image_new_from_pixbuf ( logo );
 
	gtk_button_set_image ( button, GTK_WIDGET ( image ) );

  	g_signal_connect ( button, "clicked", G_CALLBACK (activate), NULL );

  	gtk_box_pack_start ( box, GTK_WIDGET ( button ), TRUE, TRUE, 5 );

	g_debug ( "gmp_base_button" );
}

static void gmp_base_init ()
{
	gmp_rec_dir = g_strdup ( g_get_home_dir () );

    gchar *dir_conf_old = g_strdup_printf ( "%s/gtv", g_get_user_config_dir () );
    gchar *dir_conf_new = g_strdup_printf ( "%s/helia", g_get_user_config_dir () );

    if ( g_file_test ( dir_conf_old, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR ) )
    {
		g_rename ( dir_conf_old, dir_conf_new );
		g_print ( "Renames %s directory. \n", dir_conf_new );
    }
    else
    {
		if ( !g_file_test ( dir_conf_new, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR ) )
		{
			g_mkdir ( dir_conf_new, 0777 );
			g_print ( "Creating %s directory. \n", dir_conf_new );		
		}
	}

	g_free ( dir_conf_new );
    g_free ( dir_conf_old );


	opacity_cnt = 0.85;
	opacity_eq  = 0.85;
	opacity_win = 1.0;
	resize_icon = 28;
	
    gmp_dvb_conf = g_strconcat ( g_get_user_config_dir (), "/helia/gtv.conf",         NULL );
	ch_conf      = g_strconcat ( g_get_user_config_dir (), "/helia/gtv-channel.conf", NULL );

    if ( g_file_test ( gmp_dvb_conf, G_FILE_TEST_EXISTS ) )
		gmp_pref_read_config ( gmp_dvb_conf );


	gtk_icon_theme_add_resource_path ( gtk_icon_theme_get_default (), "/helia/res/icons" );

	gmpbase.logo_image = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
              "gmp-helia", 512, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	gmpbase.tv_logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
              "gmp-tv", 256, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	gmpbase.mp_logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),
              "gmp-mp", 256, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	g_debug ( "gmp_base_init" );
}

static void gmp_base_quit ( /*GtkWindow *window*/ )
{
	gmp_media_stop_all ();

	gmp_pref_save_config ( gmp_dvb_conf );
	
	gmp_media_auto_save ();

	gtk_widget_destroy ( GTK_WIDGET ( gmpbase.main_window ) );

	g_debug ( "gmp_base_quit" );
}

static void gmp_base_win ( GtkApplication *app )
{
	gmp_base_init ();

	gmpbase.main_window = (GtkWindow *)gtk_application_window_new (app);
  	gtk_window_set_title ( gmpbase.main_window, "Media Player & Digital TV");
  	gtk_window_set_default_size ( gmpbase.main_window, 900, 400 );
	g_signal_connect ( gmpbase.main_window, "destroy", G_CALLBACK ( gmp_base_quit ), NULL );

	gtk_window_set_icon ( gmpbase.main_window, gmpbase.tv_logo );


  	gmpbase.mn_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gmpbase.bs_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
  	gmpbase.tv_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gmpbase.mp_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	gtk_container_set_border_width ( GTK_CONTAINER ( gmpbase.bs_vbox ), 25 );


  	GtkBox *bt_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	
	gmp_base_button ( bt_hbox, "gmp-mp", 256, *(gmp_base_set_mp) );
	gmp_base_button ( bt_hbox, "gmp-tv", 256, *(gmp_base_set_tv) );

  	gtk_box_pack_start ( gmpbase.bs_vbox, GTK_WIDGET ( bt_hbox ), TRUE,  TRUE,  5 );


  	GtkBox *bc_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gmp_base_button ( bc_hbox, "gmp-pref",     48, *(gmp_base_set_pref)  );
	gmp_base_button ( bc_hbox, "gmp-about",    48, *(gmp_base_set_about) );
	gmp_base_button ( bc_hbox, "gmp-shutdown", 48, *(gmp_base_quit)      );

	gtk_box_pack_start ( gmpbase.bs_vbox, GTK_WIDGET ( bc_hbox ), FALSE,  FALSE,  5 );


	struct trw_columns trw_cols_tv_n[] =
	{
		{ "Num", TRUE  }, { _("Channels"), TRUE  }, { "Data", FALSE }
	};

	gmp_media_win ( gmpbase.tv_vbox, gdk_pixbuf_flip ( gmpbase.tv_logo, TRUE ), TRUE, trw_cols_tv_n, G_N_ELEMENTS ( trw_cols_tv_n ) );


	struct trw_columns trw_cols_pl_n[] =
	{
		{ "Num", TRUE  }, { _("Files"), TRUE  }, { "Full Uri", FALSE }
	};

	gmp_media_win ( gmpbase.mp_vbox, gmpbase.mp_logo, FALSE, trw_cols_pl_n, G_N_ELEMENTS ( trw_cols_pl_n ) );


	gtk_box_pack_start ( gmpbase.mn_vbox, GTK_WIDGET ( gmpbase.bs_vbox ), TRUE,  TRUE,  0 );	
	gtk_box_pack_start ( gmpbase.mn_vbox, GTK_WIDGET ( gmpbase.tv_vbox ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( gmpbase.mn_vbox, GTK_WIDGET ( gmpbase.mp_vbox ), TRUE,  TRUE,  0 );


  	gtk_container_add ( GTK_CONTAINER ( gmpbase.main_window ), GTK_WIDGET (gmpbase. mn_vbox ) );
  	gtk_widget_show_all ( GTK_WIDGET ( gmpbase.main_window ) );

	gtk_widget_hide ( GTK_WIDGET ( gmpbase.tv_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( gmpbase.mp_vbox ) );

	gtk_window_resize ( gmpbase.main_window, 900, 400 );
	
	if ( gmpbase.lists_argv != NULL ) 
	{
    	gmp_base_set_mp ();
        gmp_media_add_arg ( gmpbase.lists_argv );
        g_slist_free ( gmpbase.lists_argv );
	}
	
	gmp_media_update_box_pl ();
	
	gtk_widget_set_opacity ( GTK_WIDGET ( gmpbase.main_window ), opacity_win );

	g_debug ( "gmp_base_win" );
}

static void gmp_base_get_arg ( int argc, char **argv )
{
	gmpbase.lists_argv = NULL;
	
	if ( argc == 1 ) return;
	
	gint n = 0;
	for ( n = 1; n < argc; n++ )
	{
		g_debug ( "gmp_base_get_arg:: argv %s \n", argv[n] );
        gmpbase.lists_argv = g_slist_append ( gmpbase.lists_argv, g_strdup ( argv[n] ) );
	}
}

static void gmp_set_locale ()
{
    setlocale ( LC_ALL, "" );
    bindtextdomain ( "helia", "/usr/share/locale/" );
    textdomain ( "helia" );
}

int main ( int argc, char **argv )
{
	gst_init ( NULL, NULL );
	
	gmp_set_locale ();

	gmp_base_get_arg ( argc, argv );

    GtkApplication *app = gtk_application_new ( NULL, G_APPLICATION_FLAGS_NONE );
    g_signal_connect ( app, "activate", G_CALLBACK ( gmp_base_win ),  NULL );

    int status = g_application_run ( G_APPLICATION ( app ), 0, NULL );
    g_object_unref ( app );

    return status;
}
