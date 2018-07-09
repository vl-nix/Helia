/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia.h"
#include "helia-base.h"


G_DEFINE_TYPE ( Helia, helia, GTK_TYPE_APPLICATION );


static void helia_activate ( GApplication *app )
{
	g_debug ( "helia_activate" );

	helia_base_win ( app, NULL, 0 );
}

static void helia_open ( GApplication *app, GFile **files, gint n_files, const gchar *hint )
{
	g_debug ( "helia_open:: hint %s", hint );

	helia_base_win ( app, files, n_files );
}

static void helia_init ( Helia *object )
{
	object->program = "helia";

	g_set_prgname ( object->program );

	g_debug ( "Init %s", object->program );
}

static void helia_finalize ( GObject *object )
{
	g_debug ( "helia_finalize" );

	G_OBJECT_CLASS (helia_parent_class)->finalize (object);
}

static void helia_class_init ( HeliaClass *klass )
{
	G_APPLICATION_CLASS (klass)->activate = helia_activate;
	G_APPLICATION_CLASS (klass)->open = helia_open;

	G_OBJECT_CLASS (klass)->finalize = helia_finalize;
}

Helia * helia_new (void)
{
	return g_object_new ( helia_get_type (), // "Helia", "org.gnome.helia",
						  "flags", G_APPLICATION_HANDLES_OPEN, NULL );
}

