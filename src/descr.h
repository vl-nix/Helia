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

#define DESCR_TYPE_OBJECT descr_get_type ()

G_DECLARE_FINAL_TYPE ( Descr, descr, DESCR, OBJECT, GObject )

Descr * descr_new ( void );
