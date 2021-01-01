/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "enc-prop.h"
#include "default.h"
#include "button.h"
#include "settings.h"

struct _EncProp
{
	GtkWindow parent_instance;

	GtkBox *prop_vbox;

	GstElement *playbin;
	GstElement *enc_video;
	GstElement *enc_audio;
	GstElement *enc_muxer;

	GSettings *setting;

	gboolean mp_tv;
};

G_DEFINE_TYPE ( EncProp, enc_prop, GTK_TYPE_WINDOW )

static void enc_props_set ( GtkEntry *entry, GObject *object )
{
	const char *text = gtk_entry_get_text ( entry );

	const char *prop = gtk_widget_get_name ( GTK_WIDGET ( entry ) );

	GParamSpec *spec = g_object_class_find_property ( G_OBJECT_GET_CLASS (object), prop );

	if ( spec == NULL ) return;

	if ( spec->value_type == G_TYPE_STRING  ) g_object_set ( object, prop, text, NULL );
	if ( spec->value_type == G_TYPE_INT     ) g_object_set ( object, prop, atoi  ( text ), NULL );
	if ( spec->value_type == G_TYPE_UINT    ) g_object_set ( object, prop, atoi  ( text ), NULL );
	if ( spec->value_type == G_TYPE_LONG    ) g_object_set ( object, prop, atol  ( text ), NULL );
	if ( spec->value_type == G_TYPE_ULONG   ) g_object_set ( object, prop, atol  ( text ), NULL );
	if ( spec->value_type == G_TYPE_INT64   ) g_object_set ( object, prop, atoll ( text ), NULL );
	if ( spec->value_type == G_TYPE_UINT64  ) g_object_set ( object, prop, atoll ( text ), NULL );
	if ( spec->value_type == G_TYPE_FLOAT   ) g_object_set ( object, prop, atof  ( text ), NULL );
	if ( spec->value_type == G_TYPE_DOUBLE  ) g_object_set ( object, prop, atof  ( text ), NULL );
	if ( spec->value_type == G_TYPE_ENUM    ) g_object_set ( object, prop, atoll ( text ), NULL );
	if ( spec->value_type == G_TYPE_BOOLEAN ) g_object_set ( object, prop, g_str_has_prefix ( text, "TRUE" ) ? TRUE : FALSE, NULL );

	if ( g_str_has_prefix ( g_type_name ( spec->value_type ), "Gst" ) ) g_object_set ( object, prop, atoll ( text ), NULL );

	g_debug ( "%s:: text: %s | prop: %s | v_type: %s ", __func__, text, prop, g_type_name ( spec->value_type ) );
}

static void enc_props_win_set ( GObject *object, EncProp *prop )
{
	GtkWindow *window = (GtkWindow *)gtk_window_new ( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title ( window, "" );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_transient_for ( window, GTK_WINDOW ( prop ) );
	gtk_window_set_icon_name ( window, DEF_ICON );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_widget_set_visible ( GTK_WIDGET ( m_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	GtkGrid *grid = (GtkGrid *)gtk_grid_new ();
	gtk_grid_set_column_spacing ( grid, 10 );
	gtk_widget_set_visible ( GTK_WIDGET ( grid ), TRUE );

	gtk_box_pack_start ( m_box, GTK_WIDGET ( grid ), TRUE, TRUE, 5 );

	uint n = 0;
	uint8_t i = 0;

	GParamSpec **specs = g_object_class_list_properties ( G_OBJECT_GET_CLASS (object), &n );

	for ( i = 0; i < n; i++ ) 
	{
		GParamSpec *spec = specs[i];
		GValue value = { 0 };

		g_value_init ( &value, spec->value_type );
		g_object_get_property ( object, spec->name, &value );

		char *str = g_strdup_value_contents ( &value );

		GtkLabel *label = (GtkLabel *)gtk_label_new ( spec->name );
		gtk_widget_set_halign ( GTK_WIDGET ( label ), GTK_ALIGN_START );
		gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );

		gtk_grid_attach ( grid, GTK_WIDGET ( label ), 0, i, 1, 1 );

		GtkEntry *entry = (GtkEntry *)gtk_entry_new ();
		gtk_entry_set_text ( entry, str );
		gtk_widget_set_name ( GTK_WIDGET ( entry ), spec->name );
		gtk_widget_set_visible ( GTK_WIDGET ( entry ), TRUE );

		gboolean editable = ( spec->flags & G_PARAM_WRITABLE ) ? TRUE : FALSE;
		g_object_set ( entry, "editable", editable, NULL );

		g_signal_connect ( entry, "changed", G_CALLBACK ( enc_props_set ), object );

		gtk_grid_attach ( grid, GTK_WIDGET ( entry ), 1, i, 1, 1 );

		g_value_unset ( &value );
		free ( str );
	}

	free ( specs );

	GtkButton *button_close = helia_create_button ( h_box, "helia-exit", "🞬", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( button_close ), TRUE );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_end ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_window_present ( window );

	double opacity = gtk_widget_get_opacity ( GTK_WIDGET ( prop ) );
	gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity );
}

static void enc_prop_set_audio ( G_GNUC_UNUSED GtkButton *button, EncProp *prop )
{
	if ( prop->enc_audio ) enc_props_win_set ( G_OBJECT ( prop->enc_audio ), prop );
}

static void enc_prop_set_video ( G_GNUC_UNUSED GtkButton *button, EncProp *prop )
{
	if ( prop->enc_video ) enc_props_win_set ( G_OBJECT ( prop->enc_video ), prop );
}

static void enc_prop_set_muxer ( G_GNUC_UNUSED GtkButton *button, EncProp *prop )
{
	if ( prop->enc_muxer ) enc_props_win_set ( G_OBJECT ( prop->enc_muxer ), prop );
}



static gboolean enc_prop_check ( const char *text, const char *type )
{
	gboolean ret = FALSE;

	GstElementFactory *element_find = gst_element_factory_find ( text );

	if ( element_find )
	{
		const char *metadata = gst_element_factory_get_metadata ( element_find, GST_ELEMENT_METADATA_KLASS );

		if ( g_strrstr ( metadata, type ) ) ret = TRUE;

		g_debug ( "%s: %s | metadata %s | type %s ", __func__, text, metadata, type );

		gst_object_unref ( element_find );
	}

	return ret;
}

static void enc_prop_enc_audio ( GtkEntry *entry, EncProp *prop )
{
	const char *text = gtk_entry_get_text ( entry );

	if ( gtk_entry_get_text_length ( entry ) < 2 ) return;

	if ( enc_prop_check ( text, "Encoder/Audio" ) ) // GST_ELEMENT_FACTORY_TYPE_AUDIO_ENCODER
	{
		GstElement *enc_audio = gst_element_factory_make ( text, NULL );

		g_signal_emit_by_name ( prop, "enc-prop-set-audio", enc_audio );

		prop->enc_audio = enc_audio;

		if ( prop->setting ) g_settings_set_string ( prop->setting, "encoder-audio", text );

		gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, "helia-ok" );
	}
	else
		gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, "helia-warning" );
}

static void enc_prop_enc_video ( GtkEntry *entry, EncProp *prop )
{
	const char *text = gtk_entry_get_text ( entry );

	if ( gtk_entry_get_text_length ( entry ) < 2 ) return;

	if ( enc_prop_check ( text, "Encoder/Video" ) ) // GST_ELEMENT_FACTORY_TYPE_VIDEO_ENCODER
	{
		GstElement *enc_video = gst_element_factory_make ( text, NULL );

		g_signal_emit_by_name ( prop, "enc-prop-set-video", enc_video );

		prop->enc_video = enc_video;

		if ( prop->setting ) g_settings_set_string ( prop->setting, "encoder-video", text );

		gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, "helia-ok" );
	}
	else
		gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, "helia-warning" );
}

static void enc_prop_enc_muxer ( GtkEntry *entry, EncProp *prop )
{
	const char *text = gtk_entry_get_text ( entry );

	if ( gtk_entry_get_text_length ( entry ) < 2 ) return;

	if ( enc_prop_check ( text, "Muxer" ) ) // GST_ELEMENT_FACTORY_TYPE_MUXER
	{
		GstElement *enc_muxer = gst_element_factory_make ( text, NULL );

		g_signal_emit_by_name ( prop, "enc-prop-set-muxer", enc_muxer );

		prop->enc_muxer = enc_muxer;

		if ( prop->setting ) g_settings_set_string ( prop->setting, "encoder-muxer", text );

		gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, "helia-ok" );
	}
	else
		gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, "helia-warning" );
}

static GtkEntry * enc_prop_create_entry ( const char *set_text, void (*f)(), const char *type, EncProp *prop )
{
	GtkEntry *entry = (GtkEntry *)gtk_entry_new ();
	gtk_entry_set_text ( entry, set_text );
	gtk_widget_set_visible ( GTK_WIDGET ( entry ), TRUE );
	g_signal_connect ( entry, "changed", G_CALLBACK ( f ), prop );

	gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_PRIMARY, "helia-info" );
	gtk_entry_set_icon_tooltip_text ( entry, GTK_ENTRY_ICON_PRIMARY, type );

	if ( enc_prop_check ( set_text, type ) )
		gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, "helia-ok" );
	else
		gtk_entry_set_icon_from_icon_name ( entry, GTK_ENTRY_ICON_SECONDARY, "helia-warning" );

	return entry;
}

static GtkBox * enc_prop_create_all ( EncProp *prop )
{
	g_autofree char *enc_audio = NULL;
	g_autofree char *enc_video = NULL;
	g_autofree char *enc_muxer = NULL;

	if ( prop->setting ) enc_audio = g_settings_get_string ( prop->setting, "encoder-audio" );
	if ( prop->setting ) enc_video = g_settings_get_string ( prop->setting, "encoder-video" );
	if ( prop->setting ) enc_muxer = g_settings_get_string ( prop->setting, "encoder-muxer" );

	GtkBox *rec_vbox = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( rec_vbox, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( rec_vbox ), TRUE );

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	GtkEntry *audio_enc = enc_prop_create_entry ( ( enc_audio ) ? enc_audio : "vorbisenc", enc_prop_enc_audio, "Encoder/Audio", prop );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( audio_enc ), TRUE, TRUE, 0 );

	GtkButton *audio_prop = helia_create_button ( h_box, "helia-pref", "🛠", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( audio_prop ), TRUE );
	g_signal_connect ( audio_prop, "clicked", G_CALLBACK ( enc_prop_set_audio ), prop );

	gtk_box_pack_start ( rec_vbox, GTK_WIDGET ( h_box ), TRUE, TRUE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	GtkEntry *video_enc = enc_prop_create_entry ( ( enc_video ) ? enc_video : "theoraenc", enc_prop_enc_video, "Encoder/Video", prop );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( video_enc ), TRUE, TRUE, 0 );

	GtkButton *video_prop = helia_create_button ( h_box, "helia-pref", "🛠", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( video_prop ), TRUE );
	g_signal_connect ( video_prop, "clicked", G_CALLBACK ( enc_prop_set_video ), prop );

	gtk_box_pack_start ( rec_vbox, GTK_WIDGET ( h_box ), TRUE, TRUE, 0 );

	h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_set_spacing ( h_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	GtkEntry *muxer_enc = enc_prop_create_entry ( ( enc_muxer ) ? enc_muxer : "oggmux", enc_prop_enc_muxer, "Muxer", prop );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( muxer_enc ), TRUE, TRUE, 0 );

	GtkButton *muxer_prop = helia_create_button ( h_box, "helia-pref", "🛠", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( muxer_prop ), TRUE );
	g_signal_connect ( muxer_prop, "clicked", G_CALLBACK ( enc_prop_set_muxer ), prop );

	gtk_box_pack_start ( rec_vbox, GTK_WIDGET ( h_box ), TRUE, TRUE, 0 );

	return rec_vbox;
}

static void enc_prop_switch_get_state ( GObject *gobject, G_GNUC_UNUSED GParamSpec *pspec, EncProp *prop )
{
	gboolean state = gtk_switch_get_state ( GTK_SWITCH ( gobject ) );

	gtk_widget_set_sensitive ( GTK_WIDGET ( prop->prop_vbox ), state );

	if ( prop->setting ) g_settings_set_boolean ( prop->setting, ( prop->mp_tv ) ? "encoding-iptv" : "encoding-tv", state );
}

static GtkSwitch * enc_prop_create_switch ( gboolean enc_b, EncProp *prop )
{
	GtkSwitch *gswitch = (GtkSwitch *)gtk_switch_new ();
	gtk_switch_set_state ( gswitch, enc_b );
	gtk_widget_set_visible ( GTK_WIDGET ( gswitch ), TRUE );
	g_signal_connect ( gswitch, "notify::active", G_CALLBACK ( enc_prop_switch_get_state ), prop );

	return gswitch;
}

static GtkBox * enc_prop_create ( gboolean enc_b, EncProp *prop )
{
	GtkBox *g_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( g_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( g_box ), TRUE );

	GtkLabel *label = (GtkLabel *)gtk_label_new ( "Encoding" );
	gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( label ), FALSE, FALSE, 5 );

	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	GtkSwitch *gswitch = enc_prop_create_switch ( enc_b, prop );
	gtk_box_pack_start ( h_box, GTK_WIDGET ( gswitch ), TRUE, TRUE, 0 );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 0 );

	prop->prop_vbox = enc_prop_create_all ( prop );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( prop->prop_vbox ), FALSE, FALSE, 0 );

	label = (GtkLabel *)gtk_label_new ( " " );
	gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( label ), FALSE, FALSE, 0 );

	gtk_widget_set_sensitive ( GTK_WIDGET ( prop->prop_vbox ), enc_b );

	return g_box;
}

static void enc_prop_spin_changed_speed ( GtkSpinButton *button, EncProp *prop )
{
	uint64_t speed = (uint64_t)gtk_spin_button_get_value_as_int ( button );

	g_object_set ( prop->playbin, "connection-speed", speed, NULL );
}

static GtkBox * enc_prop_playbin_create ( uint64_t value, EncProp *prop )
{
	GtkBox *g_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_box_set_spacing ( g_box, 5 );
	gtk_widget_set_visible ( GTK_WIDGET ( g_box ), TRUE );

	GtkLabel *label = (GtkLabel *)gtk_label_new ( "Network speed ( kbps )" );
	gtk_widget_set_visible ( GTK_WIDGET ( label ), TRUE );
	gtk_box_pack_start ( g_box, GTK_WIDGET ( label ), FALSE, FALSE, 5 );

	GtkSpinButton *spin_button = (GtkSpinButton *)gtk_spin_button_new_with_range ( 0, 18446744073709, 1 );
	gtk_spin_button_set_value ( spin_button, (double)value );
	gtk_widget_set_visible ( GTK_WIDGET ( spin_button ), TRUE );
	g_signal_connect ( spin_button, "changed", G_CALLBACK ( enc_prop_spin_changed_speed ), prop );

	gtk_box_pack_start ( g_box, GTK_WIDGET ( spin_button ), FALSE, FALSE, 5 );

	return g_box;
}

static void enc_prop_quit ( G_GNUC_UNUSED GtkWindow *window, EncProp *prop )
{
	if ( prop->setting ) g_object_unref ( prop->setting );
}

void enc_prop_set_run ( GtkWindow *win_base, GstElement *playbin, GstElement *enc_video, GstElement *enc_audio, GstElement *enc_muxer, gboolean mp_tv, EncProp *prop )
{
	prop->playbin   = playbin;
	prop->enc_audio = enc_audio;
	prop->enc_video = enc_video;
	prop->enc_muxer = enc_muxer;

	prop->mp_tv = mp_tv;

	gboolean enc_b = FALSE;
	if ( prop->setting ) enc_b = g_settings_get_boolean ( prop->setting, ( mp_tv ) ? "encoding-iptv" : "encoding-tv" );

	GtkWindow *window = GTK_WINDOW ( prop );

	gtk_window_set_title ( window, "" );
	gtk_window_set_modal ( window, TRUE );
	gtk_window_set_transient_for ( window, win_base );
	gtk_window_set_icon_name ( window, DEF_ICON );
	gtk_window_set_position  ( window, GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_window_set_default_size ( window, 400, -1 );
	g_signal_connect ( window, "destroy", G_CALLBACK ( enc_prop_quit ), prop );

	GtkBox *m_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_VERTICAL,   0 );
	GtkBox *h_box = (GtkBox *)gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );

	gtk_widget_set_visible ( GTK_WIDGET ( m_box ), TRUE );
	gtk_widget_set_visible ( GTK_WIDGET ( h_box ), TRUE );

	if ( g_object_class_find_property ( G_OBJECT_GET_CLASS ( playbin ), "connection-speed" ) )
	{
		uint64_t value = 0;
		g_object_get ( playbin, "connection-speed", &value, NULL );
		gtk_box_pack_start ( m_box, GTK_WIDGET ( enc_prop_playbin_create ( value, prop ) ), FALSE, FALSE, 0 );
	}

	gtk_box_pack_start ( m_box, GTK_WIDGET ( enc_prop_create ( enc_b, prop ) ), FALSE, FALSE, 0 );

	GtkButton *button_close = helia_create_button ( h_box, "helia-exit", "🞬", 16 );
	gtk_widget_set_visible ( GTK_WIDGET ( button_close ), TRUE );
	g_signal_connect_swapped ( button_close, "clicked", G_CALLBACK ( gtk_widget_destroy ), window );

	gtk_box_pack_end ( m_box, GTK_WIDGET ( h_box ), FALSE, FALSE, 5 );

	gtk_container_set_border_width ( GTK_CONTAINER ( m_box ), 10 );
	gtk_container_add   ( GTK_CONTAINER ( window ), GTK_WIDGET ( m_box ) );

	gtk_window_present ( window );

	double opacity = gtk_widget_get_opacity ( GTK_WIDGET ( win_base ) );
	gtk_widget_set_opacity ( GTK_WIDGET ( window ), opacity );
}

static void enc_prop_init ( EncProp *prop )
{
	prop->setting = settings_init ();
}

static void enc_prop_finalize ( GObject *object )
{
	G_OBJECT_CLASS (enc_prop_parent_class)->finalize (object);
}

static void enc_prop_class_init ( EncPropClass *class )
{
	G_OBJECT_CLASS (class)->finalize = enc_prop_finalize;

	g_signal_new ( "enc-prop-set-audio", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_OBJECT );

	g_signal_new ( "enc-prop-set-video", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_OBJECT );

	g_signal_new ( "enc-prop-set-muxer", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_LAST,
		0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_OBJECT );
}

EncProp * enc_prop_new ( void )
{
	return g_object_new ( ENCPROP_TYPE_WINDOW, NULL );
}
