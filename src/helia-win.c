/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "helia-win.h"
#include "helia-dvb.h"
#include "helia-player.h"
#include "button.h"
#include "default.h"
#include "settings.h"

#include <locale.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib/gstdio.h>

struct _HeliaWin
{
	GtkWindow  parent_instance;

	GtkBox *bs_vbox;
	GtkBox *mn_vbox;
	GtkBox *mp_vbox;
	GtkBox *tv_vbox;

	GSettings *setting;
	GtkPopover *popover;

	HeliaDvb *dvb;
	HeliaPlayer *player;

	gboolean first_mp;
	gboolean first_tv;

	uint width;
	uint height;

	uint opacity_p;
	uint opacity_w;
	uint icon_size;

	uint cookie;
	GDBusConnection *connect;
};

G_DEFINE_TYPE ( HeliaWin, helia_win, GTK_TYPE_WINDOW )

static uint helia_power_manager_inhibit ( GDBusConnection *connect )
{
	uint cookie;
	GError *err = NULL;

	GVariant *reply = g_dbus_connection_call_sync ( connect, "org.freedesktop.PowerManagement", "/org/freedesktop/PowerManagement/Inhibit",
		"org.freedesktop.PowerManagement.Inhibit", "Inhibit", g_variant_new ("(ss)", "Helia", "Video" ), G_VARIANT_TYPE ("(u)"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err );

	if ( reply != NULL )
	{
		g_variant_get ( reply, "(u)", &cookie, NULL );
		g_variant_unref ( reply );

		return cookie;
	}

	if ( err )
	{
		g_warning ( "Inhibiting failed %s", err->message );
		g_error_free ( err );
	}

	return 0;
}

static void helia_power_manager_uninhibit ( GDBusConnection *connect, uint cookie )
{
	GError *err = NULL;

	GVariant *reply = g_dbus_connection_call_sync ( connect, "org.freedesktop.PowerManagement", "/org/freedesktop/PowerManagement/Inhibit",
		"org.freedesktop.PowerManagement.Inhibit", "UnInhibit", g_variant_new ("(u)", cookie), NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err );

	if ( err )
	{
		g_warning ( "Uninhibiting failed %s", err->message );
		g_error_free ( err );
	}

	g_variant_unref ( reply );
}

static void helia_power_manager_on ( HeliaWin *win )
{
	if ( win->cookie > 0 ) helia_power_manager_uninhibit ( win->connect, win->cookie );

	win->cookie = 0;
}

static void helia_power_manager_off ( HeliaWin *win )
{
	win->cookie = helia_power_manager_inhibit ( win->connect );
}

static void helia_power_manager ( gboolean power_off, HeliaWin *win )
{
	if ( win->connect )
	{
		if ( power_off )
			helia_power_manager_off ( win );
		else
			helia_power_manager_on  ( win );
	}
}

static GDBusConnection * helia_dbus_init ( void )
{
	GError *err = NULL;

	GDBusConnection *connect = g_bus_get_sync ( G_BUS_TYPE_SESSION, NULL, &err );

	if ( err )
	{
		g_warning ( "%s:: Failed to get session bus: %s", __func__, err->message );
		g_error_free ( err );
	}

	return connect;
}

static void helia_about ( GtkWindow *window )
{
	GtkAboutDialog *dialog = (GtkAboutDialog *)gtk_about_dialog_new ();
	gtk_window_set_transient_for ( GTK_WINDOW ( dialog ), window );
	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), DEF_ICON );
	gtk_widget_set_opacity   ( GTK_WIDGET ( dialog ), gtk_widget_get_opacity ( GTK_WIDGET ( window ) ) );

	const char *authors[] = { "Stepan Perun", " ", NULL };
	const char *artists[] = { "Itzik Gur",    " ", NULL };

	gtk_about_dialog_set_version ( dialog, VERSION );
	gtk_about_dialog_set_authors ( dialog, authors );
	gtk_about_dialog_set_artists ( dialog, artists );
	gtk_about_dialog_set_program_name   ( dialog, "Helia" );
	gtk_about_dialog_set_logo_icon_name ( dialog, "helia-logo" );
	gtk_about_dialog_set_license_type   ( dialog, GTK_LICENSE_GPL_3_0 );
	gtk_about_dialog_set_copyright ( dialog, "Copyright 2022 Helia" );
	gtk_about_dialog_set_website   ( dialog, "https://github.com/vl-nix/helia" );
	gtk_about_dialog_set_comments  ( dialog, "Media Player & IPTV & Digital TV \nDVB-T2/S2/C" );

	gtk_dialog_run ( GTK_DIALOG ( dialog ) );
	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

static void helia_win_changed_rec ( GtkFileChooserButton *button, HeliaWin *win )
{
	gtk_widget_set_visible ( GTK_WIDGET ( win->popover ), FALSE );

	g_autofree char *path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( button ) );

	if ( path == NULL ) return;

	if ( win->setting ) g_settings_set_string ( win->setting, "rec-dir", path );
}

static void helia_win_changed_theme ( GtkFileChooserButton *button, HeliaWin *win )
{
	gtk_widget_set_visible ( GTK_WIDGET ( win->popover ), FALSE );

	g_autofree char *path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( button ) );

	if ( path == NULL ) return;

	g_autofree char *name = g_path_get_basename ( path );

	g_object_set ( gtk_settings_get_default (), "gtk-theme-name", name, NULL );

	if ( win->setting ) g_settings_set_string ( win->setting, "theme", name );
}

static GtkBox * win_create_chooser_button ( const char *title, const char *icon, const char *path, void ( *f )( GtkFileChooserButton *, HeliaWin * ), HeliaWin *win )
{
	GtkBox *hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( hbox, 3 );
	gtk_widget_set_visible ( GTK_WIDGET ( hbox    ), TRUE );

	GtkFileChooserButton *button = (GtkFileChooserButton *)gtk_file_chooser_button_new ( title, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER );
	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( button ), path );
	g_signal_connect ( button, "file-set", G_CALLBACK ( f ), win );
	gtk_widget_set_visible ( GTK_WIDGET ( button  ), TRUE );

	if ( icon )
	{
		GtkButton *b_image = helia_create_button ( NULL, icon, "⎈", 16 );
		gtk_widget_set_visible ( GTK_WIDGET ( b_image ), TRUE );

		gtk_box_pack_start ( hbox, GTK_WIDGET ( b_image ), FALSE, FALSE, 0 );
	}
	else
		gtk_widget_set_tooltip_text ( GTK_WIDGET ( button ), title );

	gtk_box_pack_start ( hbox, GTK_WIDGET ( button  ), TRUE, TRUE, 0 );

	return hbox;
}

static void helia_win_spinbutton_changed_opacity ( GtkSpinButton *button, HeliaWin *win )
{
	uint opacity = (uint)gtk_spin_button_get_value_as_int ( button );

	if ( win->setting ) g_settings_set_uint ( win->setting, "opacity-panel", opacity );
}

static void helia_win_spinbutton_changed_opacity_win ( GtkSpinButton *button, HeliaWin *win )
{
	uint opacity = (uint)gtk_spin_button_get_value_as_int ( button );

	if ( win->setting ) g_settings_set_uint ( win->setting, "opacity-win", opacity );

	gtk_widget_set_opacity ( GTK_WIDGET ( win ), (double)opacity / 100 );
}

static void helia_win_spinbutton_changed_icon_size ( GtkSpinButton *button, HeliaWin *win )
{
	uint icon_size = (uint)gtk_spin_button_get_value_as_int ( button );

	if ( win->setting ) g_settings_set_uint ( win->setting, "icon-size", icon_size );
}

static GtkBox * helia_win_create_spinbutton ( uint val, uint8_t min, uint32_t max, uint8_t step, const char *text, const char *icon, void ( *f )( GtkSpinButton *, HeliaWin * ), HeliaWin *win )
{
	GtkBox *hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( hbox, 3 );
	gtk_widget_set_visible ( GTK_WIDGET ( hbox    ), TRUE );

	GtkSpinButton *spinbutton = (GtkSpinButton *)gtk_spin_button_new_with_range ( min, max, step );
	gtk_spin_button_set_value ( spinbutton, val );
	g_signal_connect ( spinbutton, "changed", G_CALLBACK ( f ), win );

	gtk_widget_set_visible ( GTK_WIDGET ( spinbutton ), TRUE );

	if ( icon )
	{
		GtkButton *b_image = helia_create_button ( NULL, icon, "⎈", 16 );
		gtk_widget_set_visible ( GTK_WIDGET ( b_image ), TRUE );

		gtk_box_pack_start ( hbox, GTK_WIDGET ( b_image ), FALSE, FALSE, 0 );
	}
	else
	{
		gtk_entry_set_icon_from_icon_name ( GTK_ENTRY ( spinbutton ), GTK_ENTRY_ICON_PRIMARY, "info" );
		gtk_entry_set_icon_tooltip_text   ( GTK_ENTRY ( spinbutton ), GTK_ENTRY_ICON_PRIMARY, text );
	}

	gtk_box_pack_start ( hbox, GTK_WIDGET ( spinbutton  ), TRUE, TRUE, 0 );

	return hbox;
}

static void helia_win_clicked_dark ( G_GNUC_UNUSED GtkButton *button, G_GNUC_UNUSED HeliaWin *win )
{
	gboolean dark = FALSE;
	g_object_get ( gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &dark, NULL );
	g_object_set ( gtk_settings_get_default(), "gtk-application-prefer-dark-theme", !dark, NULL );

	if ( win->setting ) g_settings_set_boolean ( win->setting, "dark", !dark );
}

static void helia_win_clicked_about ( G_GNUC_UNUSED GtkButton *button, HeliaWin *win )
{
	gtk_widget_set_visible ( GTK_WIDGET ( win->popover ), FALSE );

	helia_about ( GTK_WINDOW ( win ) );
}

static void helia_win_clicked_quit ( G_GNUC_UNUSED GtkButton *button, HeliaWin *win )
{
	gtk_widget_destroy ( GTK_WIDGET ( win ) );
}

static GtkButton * win_create_button ( const char *icon_name, const char *u_name, void ( *f )( GtkButton *, HeliaWin * ), HeliaWin *win )
{
	GtkButton *button = helia_create_button ( NULL, icon_name, u_name, 16 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( f ), win );

	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );

	return button;
}

static GtkPopover * helia_win_popover_pref ( HeliaWin *win )
{
	GtkPopover *popover = (GtkPopover *)gtk_popover_new ( NULL );

	GtkBox *vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 5 );
	gtk_box_set_spacing ( vbox, 3 );
	gtk_widget_set_visible ( GTK_WIDGET ( vbox ), TRUE );

	uint opacity_w = 100;
	uint opacity_p = OPACITY;
	uint icon_size = ICON_SIZE;

	if ( win->setting ) opacity_p = win->opacity_p;
	if ( win->setting ) opacity_w = win->opacity_w;
	if ( win->setting ) icon_size = win->icon_size;

	g_autofree char *rec_dir = NULL;
	if ( win->setting ) rec_dir = g_settings_get_string ( win->setting, "rec-dir" );
	if ( rec_dir && g_str_has_prefix ( rec_dir, "none" ) ) { free ( rec_dir ); rec_dir = NULL; }

	gtk_box_pack_start ( vbox, GTK_WIDGET ( helia_win_create_spinbutton ( icon_size, 8,   48, 1, "Icon-size",      "helia-panel",  helia_win_spinbutton_changed_icon_size,   win ) ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( vbox, GTK_WIDGET ( helia_win_create_spinbutton ( opacity_p, 40, 100, 1, "Opacity-Panel",  "helia-panel",  helia_win_spinbutton_changed_opacity,     win ) ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( vbox, GTK_WIDGET ( helia_win_create_spinbutton ( opacity_w, 40, 100, 1, "Opacity-Window", "helia-window", helia_win_spinbutton_changed_opacity_win, win ) ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( vbox, GTK_WIDGET ( win_create_chooser_button ( "Record", "helia-record", ( rec_dir ) ? rec_dir : g_get_home_dir (), helia_win_changed_rec , win ) ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( vbox, GTK_WIDGET ( win_create_chooser_button ( "Theme",  "helia-theme",  "/usr/share/themes/", helia_win_changed_theme, win ) ), FALSE, FALSE, 0 );

	GtkBox *hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( hbox, 3 );
	gtk_widget_set_visible ( GTK_WIDGET ( hbox ), TRUE );

	gtk_box_pack_start ( hbox, GTK_WIDGET ( win_create_button ( "helia-dark",  "⏾", helia_win_clicked_dark,  win ) ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( win_create_button ( "helia-info",  "🛈", helia_win_clicked_about, win ) ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( win_create_button ( "helia-quit",  "⏻", helia_win_clicked_quit,  win ) ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( vbox, GTK_WIDGET ( hbox ), TRUE, TRUE, 0 );

	gtk_container_add ( GTK_CONTAINER ( popover ), GTK_WIDGET ( vbox ) );

	return popover;
}

static void helia_win_clic_popover ( G_GNUC_UNUSED GtkButton *button, HeliaWin *win )
{
	gtk_popover_set_relative_to ( win->popover, GTK_WIDGET ( button ) );
	gtk_popover_popup ( win->popover );
}

static void helia_win_load_pref ( HeliaWin *win )
{
	win->width  = g_settings_get_uint ( win->setting, "width"  );
	win->height = g_settings_get_uint ( win->setting, "height" );

	win->opacity_w = g_settings_get_uint ( win->setting, "opacity-win" );
	win->opacity_p = g_settings_get_uint ( win->setting, "opacity-panel" );
	win->icon_size = g_settings_get_uint ( win->setting, "icon-size" );

	gboolean dark = g_settings_get_boolean ( win->setting, "dark" );
	g_object_set ( gtk_settings_get_default(), "gtk-application-prefer-dark-theme", dark, NULL );

	g_autofree char *theme = g_settings_get_string  ( win->setting, "theme" );
	if ( theme && !g_str_has_prefix ( theme, "none" ) ) g_object_set ( gtk_settings_get_default (), "gtk-theme-name", theme, NULL );
}

static void helia_win_base ( HeliaWin *win )
{
	gtk_widget_set_visible ( GTK_WIDGET ( win->mp_vbox ), FALSE );
	gtk_widget_set_visible ( GTK_WIDGET ( win->tv_vbox ), FALSE );

	gtk_widget_set_visible ( GTK_WIDGET ( win->bs_vbox ), TRUE  );

	gtk_window_set_title ( GTK_WINDOW ( win ), "Helia" );

	helia_power_manager ( FALSE, win );
}

static void helia_window_set_win_mp ( G_GNUC_UNUSED GtkButton *button, HeliaWin *win )
{
	gtk_widget_set_visible ( GTK_WIDGET ( win->bs_vbox ), FALSE  );

	if ( win->first_mp )
	{
		gtk_box_pack_start ( win->mn_vbox, GTK_WIDGET ( win->mp_vbox ), TRUE, TRUE, 0 );
		gtk_widget_set_visible ( GTK_WIDGET ( win->mp_vbox ), TRUE );
		win->first_mp = FALSE;
	}
	else
		gtk_widget_set_visible ( GTK_WIDGET ( win->mp_vbox ), TRUE );

	gtk_window_set_title ( GTK_WINDOW ( win ), "Helia - Media Player");
}

static void helia_window_set_win_tv ( G_GNUC_UNUSED GtkButton *button, HeliaWin *win )
{
	gtk_widget_set_visible ( GTK_WIDGET ( win->bs_vbox ), FALSE  );

	if ( win->first_tv )
	{
		gtk_box_pack_start ( win->mn_vbox, GTK_WIDGET ( win->tv_vbox ), TRUE, TRUE, 0 );
		gtk_widget_set_visible ( GTK_WIDGET ( win->tv_vbox ), TRUE );
		win->first_tv = FALSE;
	}
	else
		gtk_widget_set_visible ( GTK_WIDGET ( win->tv_vbox ), TRUE );

	gtk_window_set_title ( GTK_WINDOW ( win ), "Helia - Digital TV" );
}

static void helia_dvb_clicked_handler ( G_GNUC_UNUSED HeliaDvb *dvb, HeliaWin *win )
{
	helia_win_base ( win );
}

static void helia_player_clicked_handler ( G_GNUC_UNUSED HeliaPlayer *player, HeliaWin *win )
{
	helia_win_base ( win );
}

static void helia_dvb_power_handler ( G_GNUC_UNUSED HeliaDvb *dvb, gboolean power, HeliaWin *win )
{
	helia_power_manager ( power, win );
}

static void helia_player_power_handler ( G_GNUC_UNUSED HeliaPlayer *player, gboolean power, HeliaWin *win )
{
	helia_power_manager ( power, win );
}

static void helia_win_create ( HeliaWin *win )
{
	setlocale ( LC_NUMERIC, "C" );

	win->connect = helia_dbus_init ();

	gtk_icon_theme_add_resource_path ( gtk_icon_theme_get_default (), "/helia" );

	GtkWindow *window = GTK_WINDOW ( win );

	gtk_window_set_title ( window, "Helia");
	gtk_window_set_default_size ( window, (int)win->width, (int)win->height );
	gtk_window_set_icon_name ( window, DEF_ICON );

	win->dvb = helia_dvb_new ();
	win->tv_vbox = GTK_BOX ( win->dvb );
	g_signal_connect ( win->dvb, "power-set", G_CALLBACK ( helia_dvb_power_handler ), win );
	g_signal_connect ( win->dvb, "return-base", G_CALLBACK ( helia_dvb_clicked_handler ), win );

	win->player = helia_player_new ();
	win->mp_vbox = GTK_BOX ( win->player );
	g_signal_connect ( win->player, "power-set", G_CALLBACK ( helia_player_power_handler ), win );
	g_signal_connect ( win->player, "return-base", G_CALLBACK ( helia_player_clicked_handler ), win );

	win->mn_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( win->mn_vbox ), TRUE );

	win->bs_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( win->bs_vbox ), TRUE );

	gtk_container_set_border_width ( GTK_CONTAINER ( win->bs_vbox ), 25 );

	gtk_box_set_spacing ( win->mn_vbox, 10 );
	gtk_box_set_spacing ( win->bs_vbox, 10 );

	GtkBox *bt_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( bt_hbox, 10 );
	gtk_widget_set_visible ( GTK_WIDGET ( bt_hbox ), TRUE );

	gboolean icon_mp = helia_check_icon_theme ( "helia-mp" );

	GtkButton *button = helia_create_button ( bt_hbox, "helia-mp", "🎬", 96 );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_window_set_win_mp ), win );

	if ( !icon_mp )
	{
		GList *list = gtk_container_get_children ( GTK_CONTAINER (button) );
		gtk_label_set_markup ( GTK_LABEL (list->data), "<span font='48'>🎬</span>");
		g_list_free ( list );
	}

	gboolean icon_tv = helia_check_icon_theme ( "helia-tv" );

	button = helia_create_button ( bt_hbox, "helia-tv", "🖵", 96 );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_window_set_win_tv ), win );

	if ( !icon_tv )
	{
		GList *list = gtk_container_get_children ( GTK_CONTAINER (button) );
		gtk_label_set_markup ( GTK_LABEL (list->data), "<span font='48'>🖵</span>");
		g_list_free ( list );
	}

	gtk_box_pack_start ( win->bs_vbox, GTK_WIDGET ( bt_hbox ), TRUE,  TRUE,  0 );

	win->popover = helia_win_popover_pref ( win );

	GtkBox *bc_hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( bc_hbox,  10 );
	gtk_widget_set_visible ( GTK_WIDGET ( bc_hbox ), TRUE );

	button = helia_create_button ( bc_hbox, "helia-base", "🟒", 16 );
	g_signal_connect ( button, "clicked", G_CALLBACK ( helia_win_clic_popover ), win );
	gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );

	gtk_box_pack_start ( win->bs_vbox, GTK_WIDGET ( bc_hbox ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( win->mn_vbox, GTK_WIDGET ( win->bs_vbox ), TRUE,  TRUE,  0 );

	gtk_container_add  ( GTK_CONTAINER ( window ), GTK_WIDGET ( win->mn_vbox ) );
}

static void helia_win_init ( HeliaWin *win )
{
	win->first_mp = TRUE;
	win->first_tv = TRUE;

	win->opacity_w = 100;
	win->opacity_p = OPACITY;
	win->icon_size = ICON_SIZE;

	win->cookie = 0;
	win->width  = WIDTH;
	win->height = HEIGHT;

	char path_dir[PATH_MAX];
	sprintf ( path_dir, "%s/helia", g_get_user_config_dir () );

	if ( !g_file_test ( path_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR ) )
	{
		g_mkdir ( path_dir, 0775 );
		g_print ( "Creating %s directory. \n", path_dir );
	}

	win->setting = settings_init ();

	if ( win->setting ) helia_win_load_pref ( win );

	helia_win_create ( win );
}

static void helia_win_dispose ( GObject *object )
{
	HeliaWin *win = HELIA_WIN ( object );

	GdkWindow *rwin = gtk_widget_get_window ( GTK_WIDGET ( win ) );

	if ( GDK_IS_WINDOW ( rwin ) )
	{
  		int rwidth  = gdk_window_get_width  ( rwin );
  		int rheight = gdk_window_get_height ( rwin );

		uint w = ( rwidth  > 0 ) ? (uint)rwidth  : WIDTH;
		uint h = ( rheight > 0 ) ? (uint)rheight : HEIGHT;

		if ( win->setting ) g_settings_set_uint ( win->setting, "width",  w );
		if ( win->setting ) g_settings_set_uint ( win->setting, "height", h );
	}

	G_OBJECT_CLASS ( helia_win_parent_class )->dispose ( object );
}

static void helia_win_finalize ( GObject *object )
{
	HeliaWin *win = HELIA_WIN ( object );

	if ( win->setting ) g_object_unref ( win->setting );

	G_OBJECT_CLASS ( helia_win_parent_class )->finalize ( object );
}

static void helia_win_class_init ( HeliaWinClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = helia_win_finalize;

	oclass->dispose = helia_win_dispose;
}

static void helia_win_start ( GFile **files, int n_files, HeliaWin *win )
{
	g_autofree char *arg_ch = g_file_get_basename ( files[0] );

	if ( arg_ch && g_str_has_suffix ( arg_ch, "channel" ) )
	{
		g_autofree char *ch = NULL;
		if ( n_files == 2 ) ch = g_file_get_basename ( files[1] );

		helia_window_set_win_tv ( NULL, win );
		helia_dvb_start ( ch, win->dvb );
	}
	else
	{
		g_autofree char *path = g_file_get_path ( files[0] );

		if ( path && g_file_test ( path, G_FILE_TEST_EXISTS ) )
		{
			helia_window_set_win_mp ( NULL, win );
			helia_player_start ( files, n_files, win->player );
		}
	}
}

static void helia_action_dir ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, gpointer data )
{
	HeliaWin *win = data;

	if ( !gtk_widget_get_visible ( GTK_WIDGET ( win->mp_vbox ) ) ) return;

	player_action_accels ( OPEN_DIR, win->player );
}

static void helia_action_files ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, gpointer data )
{
	HeliaWin *win = data;

	if ( !gtk_widget_get_visible ( GTK_WIDGET ( win->mp_vbox ) ) ) return;

	player_action_accels ( OPEN_FILES, win->player );
}

static void helia_action_frame ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, gpointer data )
{
	HeliaWin *win = data;

	if ( !gtk_widget_get_visible ( GTK_WIDGET ( win->mp_vbox ) ) ) return;

	player_action_accels ( FRAME, win->player );
}

static void helia_action_pause ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, gpointer data )
{
	HeliaWin *win = data;

	if ( !gtk_widget_get_visible ( GTK_WIDGET ( win->mp_vbox ) ) ) return;

	player_action_accels ( PAUSE, win->player );
}

static void helia_action_slider ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, gpointer data )
{
	HeliaWin *win = data;

	if ( !gtk_widget_get_visible ( GTK_WIDGET ( win->mp_vbox ) ) ) return;

	player_action_accels ( SLIDER, win->player );
}

static void _app_add_accelerator ( const char *func_name, uint mod_key, uint gdk_key, GtkApplication *app )
{
	g_autofree char *accel_name = gtk_accelerator_name ( gdk_key, mod_key );

	const char *accel_str[] = { accel_name, NULL };

	g_autofree char *text = g_strconcat ( "app.", func_name, NULL );

	gtk_application_set_accels_for_action ( app, text, accel_str );
}

static void helia_action_decor ( G_GNUC_UNUSED GSimpleAction *sl, G_GNUC_UNUSED GVariant *pm, gpointer data )
{
	HeliaWin *win = data;

	gboolean decorated = gtk_window_get_decorated ( GTK_WINDOW ( win ) );
	gtk_window_set_decorated ( GTK_WINDOW ( win ), !decorated );
}

typedef struct _FuncAction FuncAction;

struct _FuncAction
{
	void (*f)();
	const char *func_name;

	uint mod_key;
	uint gdk_key;
};

static void helia_app_action ( HeliaApp *_app, HeliaWin *win )
{
	GtkApplication *app = GTK_APPLICATION ( _app );

	static FuncAction func_action_n[] =
        {
                { helia_action_dir,    "open-dir",    GDK_CONTROL_MASK, GDK_KEY_D },
                { helia_action_files,  "open-files",  GDK_CONTROL_MASK, GDK_KEY_O },
                { helia_action_pause,  "paused",      0, GDK_KEY_space  },
                { helia_action_frame,  "frame",       0, GDK_KEY_period },
                { helia_action_slider, "slider",      GDK_CONTROL_MASK, GDK_KEY_Z },
                { helia_action_decor,  "decorated",   0, GDK_KEY_F8 }
        };

        GActionEntry entries[ G_N_ELEMENTS ( func_action_n ) ];

	uint8_t i = 0; for ( i = 0; i < G_N_ELEMENTS ( func_action_n ); i++ )
	{
		entries[i].name           = func_action_n[i].func_name;
		entries[i].activate       = func_action_n[i].f;
		entries[i].parameter_type = NULL;
		entries[i].state          = NULL;

		_app_add_accelerator ( func_action_n[i].func_name, func_action_n[i].mod_key, func_action_n[i].gdk_key, app );
	}

	g_action_map_add_action_entries ( G_ACTION_MAP ( app ), entries, G_N_ELEMENTS ( entries ), win );
}

HeliaWin * helia_win_new ( GFile **files, int n_files, HeliaApp *app )
{
	HeliaWin *win = g_object_new ( HELIA_TYPE_WIN, "application", app, NULL );

	gtk_window_present ( GTK_WINDOW ( win ) );
	gtk_widget_set_opacity ( GTK_WIDGET ( win ), ( (float)win->opacity_w / 100 ) );

	helia_app_action ( app, win );

	if ( n_files ) helia_win_start ( files, n_files, win );

	return win;
}
