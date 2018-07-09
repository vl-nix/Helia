/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia-base.h"
#include "helia-service.h"
#include "helia-pref.h"

#include "helia-media-tv.h"
#include "helia-media-pl.h"

#include "helia-gst-pl.h"
#include "helia-gst-tv.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>


typedef struct _HeliaBase HeliaBase;

struct _HeliaBase
{
	GtkApplication *app;
	GtkWindow *main_window;
	GdkPixbuf *logo_image, *tv_logo, *mp_logo;
	GtkBox *mn_vbox, *bs_vbox, *tv_vbox, *mp_vbox;
};

static HeliaBase heliabase;



typedef struct _HeliaBaseAccel HeliaBaseAccel;

struct _HeliaBaseAccel
{ 
	const gchar *name;
	const gchar *accel_key;
	void (* activate);
};

static HeliaBaseAccel hb_accel_n[] =
{
	// Media Player
	{ "mpedit",   "<control>l",   helia_media_pl_edit_hs      },
	{ "mpmute",   "<control>m",   helia_gst_pl_mute_set       },
	{ "mpstop",   "<control>x",   helia_gst_pl_playback_stop  },
	{ "mpinfo",   "<control>i",   helia_gst_pl_info           },

	{ "mpslider", "<control>h",   helia_media_pl_slider_hs    },
	{ "mpplay",   "space",        helia_gst_pl_playback_play  },
	{ "mppause",  "<shift>space", helia_gst_pl_playback_pause },
	{ "mpframe",  "period",       helia_gst_pl_frame          },
		
	// Digital TV
	{ "tvedit",   "<control>l",   helia_media_tv_edit_hs      },
	{ "tvmute",   "<control>m",   helia_gst_tv_mute_set       },
	{ "tvstop",   "<control>x",   helia_gst_tv_stop           },
	{ "tvinfo",   "<control>i",   helia_gst_tv_info           }
};


static void helia_base_app_add_remove_accelerator ( guint start, guint end, gboolean add_remove )
{	
    guint i;
    for ( i = start; i < end; i++ )
    {
        const gchar *accelf[] = { NULL, NULL };

        if ( add_remove ) { accelf[0] = hb_accel_n[i].accel_key; }

			gchar *text = g_strconcat ( "app.", hb_accel_n[i].name, NULL );

				gtk_application_set_accels_for_action ( heliabase.app, text, accelf );
            
            g_free ( text );

        accelf[0] = NULL;
    }
}


static void helia_about_win ( GtkWindow *window, GdkPixbuf *logo )
{
	g_debug ( "helia_about_win" );

	GtkAboutDialog *dialog = (GtkAboutDialog *)gtk_about_dialog_new ();
	gtk_window_set_transient_for ( (GtkWindow *)dialog, window );

	const gchar *authors[]   = { "Stepan Perun",   " ", NULL };
	const gchar *artists[]   = { "Itzik Gur",      " ", NULL };
	const gchar *translators = "Anton Midyukov \nHeimen Stoffels \n";
	const gchar *license     = "This program is free software. \nGNU Lesser General Public License \nwww.gnu.org/licenses/lgpl.html";

	gtk_about_dialog_set_program_name ( dialog, "Helia" );
	gtk_about_dialog_set_version ( dialog, "4.2" );
	gtk_about_dialog_set_license ( dialog, license );
	gtk_about_dialog_set_authors ( dialog, authors );
	gtk_about_dialog_set_artists ( dialog, artists );
	gtk_about_dialog_set_translator_credits ( dialog, translators );
	gtk_about_dialog_set_website ( dialog,   "https://gitlab.com/vl-nix/Helia" );
	gtk_about_dialog_set_copyright ( dialog, "Copyright 2018 Helia  ( Gtv-Dvb )" );
	gtk_about_dialog_set_comments  ( dialog, "Media Player & IPTV & Digital TV \nDVB-T2/S2/C, ATSC, DTMB, ISDB" );

	if ( logo != NULL )
	{
		gtk_about_dialog_set_logo ( dialog, logo );
	}
	else
		gtk_about_dialog_set_logo_icon_name ( dialog, "applications-multimedia" );

	gtk_dialog_run ( GTK_DIALOG (dialog) );

	gtk_widget_destroy ( GTK_WIDGET (dialog) );
}

static void helia_base_set_about ()
{
	helia_about_win ( heliabase.main_window, heliabase.logo_image );

	g_debug ( "helia_base_set_about" );
}

static void helia_base_set_pref ()
{
	helia_pref_win ( heliabase.main_window );

	g_debug ( "helia_base_set_pref" );
}

void helia_base_update_win ()
{
	gtk_widget_queue_draw ( GTK_WIDGET ( heliabase.main_window ) );

	g_debug ( "helia_base_update_win" );
}

GtkWindow * helia_base_win_ret ()
{
	g_debug ( "helia_base_win_ret" );

	return heliabase.main_window;
}

gboolean helia_base_flscr ()
{
	g_debug ( "helia_base_flscr" );

	GdkWindowState state = gdk_window_get_state ( gtk_widget_get_window ( GTK_WIDGET ( heliabase.main_window ) ) );

	if ( state & GDK_WINDOW_STATE_FULLSCREEN )
		{ gtk_window_unfullscreen ( heliabase.main_window ); gtk_widget_queue_draw ( GTK_WIDGET ( heliabase.main_window ) ); return FALSE; }
	else
		{ gtk_window_fullscreen   ( heliabase.main_window ); gtk_widget_queue_draw ( GTK_WIDGET ( heliabase.main_window ) ); return TRUE;  }

	return TRUE;
}

void helia_base_set_window ()
{
	gtk_window_set_title ( heliabase.main_window, "Media Player & Digital TV");

	gtk_widget_hide ( GTK_WIDGET ( heliabase.mp_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( heliabase.tv_vbox ) );
	gtk_widget_show ( GTK_WIDGET ( heliabase.bs_vbox ) );
	
	helia_base_app_add_remove_accelerator ( 0, 12, FALSE );

	g_debug ( "helia_base_set_window" );
}

static void helia_base_set_tv ()
{
	gtk_window_set_icon  ( heliabase.main_window, heliabase.tv_logo );
	gtk_window_set_title ( heliabase.main_window, "Digital TV");

	gtk_widget_hide ( GTK_WIDGET ( heliabase.bs_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( heliabase.mp_vbox ) );
	gtk_widget_show ( GTK_WIDGET ( heliabase.tv_vbox ) );
	
	helia_base_app_add_remove_accelerator ( 8, 12, TRUE );

	g_debug ( "helia_base_set_tv" );
}

static void helia_base_set_mp ()
{
	gtk_window_set_icon  ( heliabase.main_window, heliabase.mp_logo );
	gtk_window_set_title ( heliabase.main_window, "Media Player");

	gtk_widget_hide ( GTK_WIDGET ( heliabase.bs_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( heliabase.tv_vbox ) );
	gtk_widget_show ( GTK_WIDGET ( heliabase.mp_vbox ) );
	
	helia_base_app_add_remove_accelerator ( 0, 8, TRUE );

	g_debug ( "helia_base_set_mp" );
}

static void helia_base_init ()
{
	gchar *dir_conf_old = g_strdup_printf ( "%s/gtv",   g_get_user_config_dir () );
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

	hbgv.base_quit = FALSE;
	hbgv.play      = TRUE;
	hbgv.sort_az   = TRUE;
	hbgv.all_info  = FALSE;

	hbgv.opacity_cnt = 0.85;
	hbgv.opacity_eq  = 0.85;
	hbgv.opacity_win = 1.0;
	hbgv.resize_icon = 28;

	hbgv.dir_conf   = g_strconcat ( g_get_user_config_dir (), "/helia",                  NULL );
	hbgv.helia_conf = g_strconcat ( g_get_user_config_dir (), "/helia/gtv.conf",         NULL );
	hbgv.ch_conf    = g_strconcat ( g_get_user_config_dir (), "/helia/gtv-channel.conf", NULL );

	hbgv.rec_dir    = g_strdup ( g_get_home_dir () );

	if ( g_file_test ( hbgv.helia_conf, G_FILE_TEST_EXISTS ) )
		helia_pref_read_config ( hbgv.helia_conf );


	gtk_icon_theme_add_resource_path ( gtk_icon_theme_get_default (), "/helia/icons" );
	gtk_icon_theme_add_resource_path ( gtk_icon_theme_get_default (), "/helia/data/icons" );

	heliabase.logo_image = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
						 "helia-logo", 512, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	heliabase.tv_logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), 
					  "helia-tv", 256, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	heliabase.mp_logo = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (),	
					  "helia-mp", 256, GTK_ICON_LOOKUP_USE_BUILTIN, NULL );

	g_debug ( "helia_base_init:: %s", g_get_prgname () );
}


static void helia_base_create_gaction_entry ( GtkApplication *app )
{	
    GActionEntry entries[ G_N_ELEMENTS ( hb_accel_n ) ];

    guint i; for ( i = 0; i < G_N_ELEMENTS ( hb_accel_n ); i++ )
    {
        entries[i].name           = hb_accel_n[i].name;
        entries[i].activate       = hb_accel_n[i].activate;
        entries[i].parameter_type = NULL;
        entries[i].state          = NULL;
    }

    g_action_map_add_action_entries ( G_ACTION_MAP ( app ), entries, G_N_ELEMENTS ( entries ), NULL );
}


static void helia_base_quit ( /*GtkWindow *window*/ )
{
	hbgv.base_quit = TRUE;
	
	helia_media_tv_stop ();

	helia_media_pl_stop ();

	helia_pref_save_config ( hbgv.helia_conf );

	gtk_widget_destroy ( GTK_WIDGET ( heliabase.main_window ) );

	g_debug ( "helia_base_quit" );
}

void helia_base_win ( GApplication *app, GFile **files, gint n_files )
{
	heliabase.app = GTK_APPLICATION ( app );
	
	helia_base_init ();
	helia_base_create_gaction_entry ( GTK_APPLICATION ( app ) );

	heliabase.main_window = (GtkWindow *)gtk_application_window_new ( GTK_APPLICATION ( app ) );
	gtk_window_set_title ( heliabase.main_window, "Media Player & Digital TV");
	gtk_window_set_default_size ( heliabase.main_window, 900, 400 );
	g_signal_connect ( heliabase.main_window, "destroy", G_CALLBACK ( helia_base_quit ), NULL );

	gtk_window_set_icon ( heliabase.main_window, heliabase.tv_logo );


	heliabase.mn_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	heliabase.bs_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	heliabase.tv_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	heliabase.mp_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );

	gtk_container_set_border_width ( GTK_CONTAINER ( heliabase.bs_vbox ), 25 );

	gtk_box_set_spacing ( heliabase.mn_vbox, 10 );
	gtk_box_set_spacing ( heliabase.bs_vbox, 10 );


	GtkBox *bt_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( bt_hbox,  10 );

	helia_service_create_image_button ( bt_hbox, "helia-mp", 256, helia_base_set_mp, NULL );
	helia_service_create_image_button ( bt_hbox, "helia-tv", 256, helia_base_set_tv, NULL );

	gtk_box_pack_start ( heliabase.bs_vbox, GTK_WIDGET ( bt_hbox ), TRUE,  TRUE,  0 );


	GtkBox *bc_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( bc_hbox,  10 );

	helia_service_create_image_button ( bc_hbox, "helia-pref",     48, helia_base_set_pref,  NULL );
	helia_service_create_image_button ( bc_hbox, "helia-about",    48, helia_base_set_about, NULL );
	helia_service_create_image_button ( bc_hbox, "helia-shutdown", 48, helia_base_quit,      NULL );

	gtk_box_pack_start ( heliabase.bs_vbox, GTK_WIDGET ( bc_hbox ), FALSE,  FALSE, 0 );


	HBTrwCol trwc_tv_n[] =
	{
		{ "Num",         TRUE  }, 
		{ _("Channels"), TRUE  }, 
		{ "Data",        FALSE }
	};

	helia_media_tv_win ( heliabase.tv_vbox, gdk_pixbuf_flip ( heliabase.tv_logo, TRUE ), trwc_tv_n, G_N_ELEMENTS ( trwc_tv_n ) );

	HBTrwCol trwc_pl_n[] =
	{
		{ "Num",      TRUE  }, 
		{ _("Files"), TRUE  }, 
		{ "Full Uri", FALSE }
	};

	helia_media_pl_win ( heliabase.mp_vbox, heliabase.mp_logo, trwc_pl_n, G_N_ELEMENTS ( trwc_pl_n ) );


	gtk_box_pack_start ( heliabase.mn_vbox, GTK_WIDGET ( heliabase.bs_vbox ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( heliabase.mn_vbox, GTK_WIDGET ( heliabase.tv_vbox ), TRUE,  TRUE,  0 );
	gtk_box_pack_start ( heliabase.mn_vbox, GTK_WIDGET ( heliabase.mp_vbox ), TRUE,  TRUE,  0 );


	gtk_container_add ( GTK_CONTAINER ( heliabase.main_window ), GTK_WIDGET ( heliabase.mn_vbox ) );
	gtk_widget_show_all ( GTK_WIDGET ( heliabase.main_window ) );

	gtk_widget_hide ( GTK_WIDGET ( heliabase.tv_vbox ) );
	gtk_widget_hide ( GTK_WIDGET ( heliabase.mp_vbox ) );

	gtk_window_resize ( heliabase.main_window, 900, 400 );

	if ( n_files > 0 ) { helia_base_set_mp (); helia_media_pl_add_arg ( files, n_files ); }

	gtk_widget_set_opacity ( GTK_WIDGET ( heliabase.main_window ), hbgv.opacity_win );

	g_debug ( "helia_base_win" );
}
