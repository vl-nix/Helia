/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "pref.h"

struct _Pref
{
	GtkPopover parent_instance;

	GSettings *setting;
};

G_DEFINE_TYPE ( Pref, pref, GTK_TYPE_POPOVER )

typedef void ( *fpp ) ( GtkButton *, Pref * );

static GSettings * settings_init ( void )
{
	GSettingsSchemaSource *schemasrc = g_settings_schema_source_get_default ();
	GSettingsSchema *schema = g_settings_schema_source_lookup ( schemasrc, "org.gnome.helia", FALSE );

	GSettings *setting = NULL;

	if ( schema == NULL ) g_critical ( "%s:: schema: org.gnome.helia - not installed.", __func__ );
	if ( schema == NULL ) return NULL;

	setting = g_settings_new ( "org.gnome.helia" );

	return setting;
}

static void pref_set_def ( GtkWindow *window, Pref *pref )
{
	if ( pref->setting == NULL )
	{
		g_object_set ( gtk_settings_get_default(), "gtk-application-prefer-dark-theme", TRUE, NULL );

		return;
	}

	uint width   = g_settings_get_uint ( pref->setting, "width"  );
	uint height  = g_settings_get_uint ( pref->setting, "height" );
	// uint opacity = g_settings_get_uint ( pref->setting, "opacity" );

	gtk_window_set_default_size ( window, (int)width, (int)height );

	gboolean dark = g_settings_get_boolean ( pref->setting, "dark" );
	g_object_set ( gtk_settings_get_default(), "gtk-application-prefer-dark-theme", dark, NULL );

	g_autofree char *theme = g_settings_get_string  ( pref->setting, "theme" );
	if ( theme && !g_str_has_prefix ( theme, "none" ) ) g_object_set ( gtk_settings_get_default (), "gtk-theme-name", theme, NULL );
}

static void pref_save ( int w, int h, Pref *pref )
{
	if ( pref->setting )
	{
		gboolean dark = FALSE;
		g_object_get ( gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &dark, NULL );

		g_autofree char *theme = NULL;
		g_object_get ( gtk_settings_get_default (), "gtk-theme-name", &theme, NULL );

		g_settings_set_uint ( pref->setting, "width",  (uint)w );
		g_settings_set_uint ( pref->setting, "height", (uint)h );

		g_settings_set_string  ( pref->setting, "theme", theme );
		g_settings_set_boolean ( pref->setting, "dark",  dark  );
	}
}

static void pref_about ( GtkWindow *window )
{
	GtkAboutDialog *dialog = (GtkAboutDialog *)gtk_about_dialog_new ();
	gtk_window_set_transient_for ( GTK_WINDOW ( dialog ), window );
	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), "helia" );

	const char *authors[] = { "Stepan Perun", " ", NULL };

	gtk_about_dialog_set_version ( dialog, VERSION );
	gtk_about_dialog_set_authors ( dialog, authors );
	gtk_about_dialog_set_program_name   ( dialog, "Helia" );
	gtk_about_dialog_set_logo_icon_name ( dialog, "helia-logo" );
	gtk_about_dialog_set_license_type   ( dialog, GTK_LICENSE_GPL_3_0 );
	gtk_about_dialog_set_copyright ( dialog, "Copyright 2022 Helia" );
	gtk_about_dialog_set_website   ( dialog, "https://github.com/vl-nix/helia" );
	gtk_about_dialog_set_comments  ( dialog, "Digital TV" );

	gtk_dialog_run ( GTK_DIALOG ( dialog ) );
	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

static void pref_spinbutton_changed_opacity_win ( GtkSpinButton *button, Pref *pref )
{
	uint opacity = (uint)gtk_spin_button_get_value_as_int ( button );

	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( pref ) ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), (double)opacity / 100 );
}

static GtkBox * pref_create_spinbutton ( uint val, uint8_t min, uint32_t max, uint8_t step, const char *icon, void ( *f )( GtkSpinButton *, Pref * ), Pref *pref )
{
	GtkBox *hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( hbox, 3 );
	gtk_widget_set_visible ( GTK_WIDGET ( hbox    ), TRUE );

	GtkSpinButton *spinbutton = (GtkSpinButton *)gtk_spin_button_new_with_range ( min, max, step );
	gtk_spin_button_set_value ( spinbutton, val );
	g_signal_connect ( spinbutton, "changed", G_CALLBACK ( f ), pref );

	GtkButton *b_image = (GtkButton *)gtk_button_new_from_icon_name ( icon, GTK_ICON_SIZE_MENU );
	gtk_widget_set_visible ( GTK_WIDGET ( b_image ), TRUE );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( b_image ), FALSE, FALSE, 0 );

	gtk_widget_set_visible ( GTK_WIDGET ( spinbutton ), TRUE );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( spinbutton  ), TRUE, TRUE, 0 );

	return hbox;
}

static void pref_changed_theme ( GtkFileChooserButton *button, Pref *pref )
{
	gtk_widget_set_visible ( GTK_WIDGET ( pref ), FALSE );

	g_autofree char *path = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( button ) );

	if ( path == NULL ) return;

	g_autofree char *name = g_path_get_basename ( path );

	g_object_set ( gtk_settings_get_default (), "gtk-theme-name", name, NULL );
}

static GtkBox * pref_create_chooser_button ( const char *title, const char *icon, const char *path, void ( *f )( GtkFileChooserButton *, Pref * ), Pref *pref )
{
	GtkBox *hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( hbox, 3 );
	gtk_widget_set_visible ( GTK_WIDGET ( hbox    ), TRUE );

	GtkFileChooserButton *button = (GtkFileChooserButton *)gtk_file_chooser_button_new ( title, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER );
	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( button ), path );
	g_signal_connect ( button, "file-set", G_CALLBACK ( f ), pref );
	gtk_widget_set_visible ( GTK_WIDGET ( button  ), TRUE );

	GtkButton *b_image = (GtkButton *)gtk_button_new_from_icon_name ( icon, GTK_ICON_SIZE_MENU );
	gtk_widget_set_visible ( GTK_WIDGET ( b_image ), TRUE );
	gtk_box_pack_start ( hbox, GTK_WIDGET ( b_image ), FALSE, FALSE, 0 );

	gtk_box_pack_start ( hbox, GTK_WIDGET ( button  ), TRUE, TRUE, 0 );

	return hbox;
}

static void pref_clicked_dark ( G_GNUC_UNUSED GtkButton *button, G_GNUC_UNUSED Pref *pref )
{
	gboolean dark = FALSE;
	g_object_get ( gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &dark, NULL );
	g_object_set ( gtk_settings_get_default(), "gtk-application-prefer-dark-theme", !dark, NULL );
}

static void pref_clicked_about ( G_GNUC_UNUSED GtkButton *button, Pref *pref )
{
	gtk_widget_set_visible ( GTK_WIDGET ( pref ), FALSE );

	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( pref ) ) );

	pref_about ( window );
}

static void pref_clicked_quit ( G_GNUC_UNUSED GtkButton *button, Pref *pref )
{
	GtkWindow *window = GTK_WINDOW ( gtk_widget_get_toplevel ( GTK_WIDGET ( pref ) ) );

	gtk_window_close ( window );
}

static void pref_handler_defl ( Pref *pref, GtkWindow *window )
{
	pref_set_def ( window, pref );
}

static void pref_handler_save ( Pref *pref, int w, int h )
{
	pref_save ( w, h, pref );
}

static void pref_init ( Pref *pref )
{
	pref->setting = settings_init ();

	GtkPopover *popover = GTK_POPOVER ( pref );

	GtkBox *vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 5 );
	gtk_box_set_spacing ( vbox, 3 );
	gtk_widget_set_visible ( GTK_WIDGET ( vbox ), TRUE );

	gtk_box_pack_start ( vbox, GTK_WIDGET ( pref_create_spinbutton     ( 100, 40, 100, 1, "helia-window", pref_spinbutton_changed_opacity_win, pref ) ), FALSE, FALSE, 0 );
	gtk_box_pack_start ( vbox, GTK_WIDGET ( pref_create_chooser_button ( "Theme", "helia-theme", "/usr/share/themes/", pref_changed_theme, pref ) ), FALSE, FALSE, 0 );

	GtkBox *hbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( hbox, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( hbox ), TRUE );

	const char *icons[] = { "helia-dark", "helia-info", "helia-quit" };
	fpp funcs[] = { pref_clicked_dark, pref_clicked_about, pref_clicked_quit };

	uint8_t c = 0; for ( c = 0; c < G_N_ELEMENTS ( icons ); c++ )
	{
		GtkButton *button = (GtkButton *)gtk_button_new_from_icon_name ( icons[c], GTK_ICON_SIZE_MENU );
		gtk_widget_set_visible ( GTK_WIDGET ( button ), TRUE );
		g_signal_connect ( button, "clicked", G_CALLBACK ( funcs[c] ), pref );

		gtk_box_pack_start ( hbox, GTK_WIDGET ( button ), TRUE, TRUE, 0 );
	}

	gtk_box_pack_start ( vbox, GTK_WIDGET ( hbox ), TRUE, TRUE, 0 );

	gtk_container_add ( GTK_CONTAINER ( popover ), GTK_WIDGET ( vbox ) );

	g_signal_connect ( pref, "pref-defl", G_CALLBACK ( pref_handler_defl ), NULL );
	g_signal_connect ( pref, "pref-save", G_CALLBACK ( pref_handler_save ), NULL );
}

static void pref_finalize ( GObject *object )
{
	Pref *pref = PREF_POPOVER ( object );

	if ( pref->setting ) g_object_unref ( pref->setting );

	G_OBJECT_CLASS ( pref_parent_class )->finalize ( object );
}

static void pref_class_init ( PrefClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = pref_finalize;

	g_signal_new ( "pref-defl", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER );
	g_signal_new ( "pref-save", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT );
}

Pref * pref_new ( void )
{
	return g_object_new ( PREF_TYPE_POPOVER, NULL );
}
