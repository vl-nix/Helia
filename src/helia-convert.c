/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <glib/gi18n.h>

#include "helia-convert.h"
#include "helia-scan.h"
#include "helia-base.h"
#include "helia-service.h"


static guint adapter_set, frontend_set;


static void helia_convert_set_adapter ( GtkSpinButton *button, GtkLabel *label_dvb_name )
{
	gtk_spin_button_update ( button );
	adapter_set = gtk_spin_button_get_value_as_int ( button );

	helia_get_dvb_info ( label_dvb_name, adapter_set, frontend_set );

	g_debug ( "helia_convert_set_adapter" );
}
static void helia_convert_set_frontend ( GtkSpinButton *button, GtkLabel *label_dvb_name )
{
	gtk_spin_button_update ( button );
	frontend_set = gtk_spin_button_get_value_as_int ( button );

	helia_get_dvb_info ( label_dvb_name, adapter_set, frontend_set );

	g_debug ( "helia_convert_set_frontend" );
}

static void helia_convert_file ( GtkButton *button, GtkEntry *entry )
{
	const gchar *text = gtk_entry_get_text ( entry );

	g_debug ( "helia_convert_file:: file %s", text );

	if ( text && g_str_has_suffix ( text, ".conf" ) )
		helia_convert_dvb5 ( (gchar *)text, adapter_set, frontend_set );
	else
		g_print ( "helia_convert_file:: no convert %s \n ", text );

	gtk_widget_destroy ( GTK_WIDGET ( gtk_widget_get_parent ( gtk_widget_get_parent ( gtk_widget_get_parent (GTK_WIDGET (button)) ) ) ) );
}

static void  helia_convert_set_file ( GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEventButton *event )
{
	g_debug ( "helia_convert_set_file:: icon_pos %d | button %d", icon_pos, event->button );

	gchar *file_name = helia_service_open_file ( g_get_home_dir (), "conf", "*.conf" );

	if ( file_name == NULL ) return;

	gtk_entry_set_text ( entry, file_name );

	g_free ( file_name );
}

void helia_convert_win ( gchar *file )
{
	GtkWindow *window =      (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_transient_for ( window, helia_base_win_ret () );
	gtk_window_set_modal     ( window, TRUE );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_title     ( window, _("Convert") );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	GtkBox *m_box_n = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_pack_start ( m_box_n, GTK_WIDGET ( helia_scan_device ( helia_convert_set_adapter, helia_convert_set_frontend ) ), TRUE, TRUE, 0 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( m_box_n ), TRUE, TRUE, 0 );


	GtkLabel *label = (GtkLabel *)gtk_label_new ( _("Format  DVBv5") );
	gtk_box_pack_start ( m_box_n, GTK_WIDGET ( label ), FALSE, FALSE, 10 );

		GtkEntry *entry = (GtkEntry *)gtk_entry_new ();
		gtk_entry_set_text ( entry, ( file ) ? file : "dvb_channel.conf" );

		gtk_widget_set_margin_start ( GTK_WIDGET ( entry ), 10 );
		gtk_widget_set_margin_end   ( GTK_WIDGET ( entry ), 10 );

		g_object_set ( entry, "editable", FALSE, NULL );
		gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, "folder" );
		g_signal_connect ( entry, "icon-press", G_CALLBACK ( helia_convert_set_file ), NULL );

		gtk_box_pack_start ( m_box_n, GTK_WIDGET ( entry ), FALSE, FALSE, 10 );

	gtk_box_pack_start ( m_box_n, GTK_WIDGET ( gtk_label_new ( " " ) ), TRUE, TRUE, 0 );
	

	GtkButton *button_convert = (GtkButton *)gtk_button_new_from_icon_name ( "helia-ok", GTK_ICON_SIZE_SMALL_TOOLBAR );
	g_signal_connect ( button_convert, "clicked", G_CALLBACK ( helia_convert_file ), entry );
	gtk_box_pack_end ( h_box, GTK_WIDGET ( button_convert ), TRUE, TRUE, 5 );

	GtkButton *button_close = (GtkButton *)gtk_button_new_from_icon_name ( "helia-exit", GTK_ICON_SIZE_SMALL_TOOLBAR );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), GTK_WIDGET ( window ) );	
	gtk_box_pack_end ( h_box, GTK_WIDGET ( button_close ), TRUE, TRUE, 5 );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );
	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 5 );
	gtk_container_add ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_widget_show_all ( GTK_WIDGET ( window ) );

	gtk_widget_set_opacity ( GTK_WIDGET ( window ), hbgv.opacity_win );

	g_debug ( "helia_convert_win" );
}
