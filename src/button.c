/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "button.h"

gboolean helia_check_icon_theme ( const char *name_icon )
{
	gboolean ret = FALSE;

	char **icons = g_resources_enumerate_children ( "/helia", G_RESOURCE_LOOKUP_FLAGS_NONE, NULL );

	if ( icons == NULL ) return ret;

	uint j = 0, numfields = g_strv_length ( icons );

	for ( j = 0; j < numfields; j++ )
	{
		if ( g_strrstr ( icons[j], name_icon ) ) { ret = TRUE; break; }
	}

	g_strfreev ( icons );

	return ret;
}

GtkImage * helia_create_image ( const char *icon, uint16_t size )
{
	GdkPixbuf *pixbuf = gtk_icon_theme_load_icon ( gtk_icon_theme_get_default (), icon, size, GTK_ICON_LOOKUP_FORCE_SIZE, NULL );

	GtkImage *image   = (GtkImage *)gtk_image_new_from_pixbuf ( pixbuf );
	gtk_image_set_pixel_size ( image, size );

	if ( pixbuf ) g_object_unref ( pixbuf );

	return image;
}

static GtkButton * helia_set_image_button ( const char *icon, uint16_t size )
{
	GtkButton *button = (GtkButton *)gtk_button_new ();

	GtkImage *image   = helia_create_image ( icon, size );

	gtk_button_set_image ( button, GTK_WIDGET ( image ) );

	return button;
}

GtkButton * helia_create_button ( GtkBox *h_box, const char *name, const char *icon_u, uint16_t icon_size )
{
	GtkButton *button;

	if ( helia_check_icon_theme ( name ) )
		button = helia_set_image_button ( name, icon_size );
	else
		button = (GtkButton *)gtk_button_new_with_label ( icon_u );

	if ( h_box ) gtk_box_pack_start ( h_box, GTK_WIDGET ( button ), TRUE, TRUE, 0 );

	return button;
}

