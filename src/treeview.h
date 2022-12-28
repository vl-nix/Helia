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

#define TREEDVB_TYPE_BOX treedvb_get_type ()

G_DECLARE_FINAL_TYPE ( TreeDvb, treedvb, TREEDVB, BOX, GtkBox )

TreeDvb * treedvb_new ( Level * );
