/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#pragma once

#include "include.h"

typedef unsigned int uint;

#define SCAN_TYPE_WIN scan_get_type ()

G_DECLARE_FINAL_TYPE ( Scan, scan, SCAN, WIN, GtkWindow )

Scan * scan_new ( GtkWindow * );
