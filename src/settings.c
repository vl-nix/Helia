/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "settings.h"

GSettings * settings_init ( void )
{
	GSettingsSchemaSource *schemasrc = g_settings_schema_source_get_default ();
	GSettingsSchema *schema = g_settings_schema_source_lookup ( schemasrc, "org.gnome.helia", FALSE );

	GSettings *setting = NULL;

	if ( schema == NULL ) g_critical ( "%s:: schema: org.gnome.helia - not installed.", __func__ );
	if ( schema == NULL ) return NULL;

	setting = g_settings_new ( "org.gnome.helia" );

	return setting;
}
