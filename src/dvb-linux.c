/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "dvb-linux.h"

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/dvb/frontend.h>

inline char * dvb_get_name ( int adapter, int frontend )
{
	char *name = NULL;

	int fd = 0;

	char path[80];
	sprintf ( path, "/dev/dvb/adapter%d/frontend%d", adapter, frontend );

	if ( ( fd = open ( path, O_RDWR ) ) == -1 )
	{
		if ( ( fd = open ( path, O_RDONLY ) ) == -1 ) g_warning ( "%s: %s %s \n", __func__, path, g_strerror ( errno ) );
	}

	if ( fd != -1 )
	{
		struct dvb_frontend_info info;

		if ( ( ioctl ( fd, FE_GET_INFO, &info ) ) == -1 )
			perror ( "dvb_get_name: ioctl FE_GET_INFO " );
		else
			name = g_strdup ( info.name );

		close ( fd );
	}

	if ( name == NULL ) name = g_strdup ( "Undefined" );

	g_debug ( "DVB device: %s ( %s ) ", name, path );

	return name;
}
