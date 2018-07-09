/*
* Copyright 2014 - 2018 Stepan Perun
* This program is free software.
* License: Gnu Lesser General Public License
* http://www.gnu.org/licenses/lgpl.html
*/


#include "helia.h"

#include <glib/gi18n.h>


int main ( int argc, char *argv[] )
{
	gst_init ( NULL, NULL );

    bindtextdomain ( "helia", "/usr/share/locale/" );
	bind_textdomain_codeset ( "helia", "UTF-8" );
	textdomain ( "helia" );

	Helia *helia = helia_new ();
	
	int status = g_application_run ( G_APPLICATION (helia), argc, argv );

	g_object_unref (helia);

	return status;
}
