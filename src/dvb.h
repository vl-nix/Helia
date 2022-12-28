/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#pragma once

#include "level.h"

typedef unsigned int uint;
typedef unsigned long ulong;

#define DVB_TYPE_DRAW dvb_get_type ()

G_DECLARE_FINAL_TYPE ( Dvb, dvb, DVB, DRAW, GtkDrawingArea )

Dvb * dvb_new ( uint8_t, Level * );
