/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#pragma once

#include "pref.h"

typedef unsigned int uint;

#define HELIA_TYPE_DVB helia_dvb_get_type ()

G_DECLARE_FINAL_TYPE ( HeliaDvb, helia_dvb, HELIA, DVB, GtkBox )

HeliaDvb * helia_dvb_new ( Pref *, GtkWindow * );
