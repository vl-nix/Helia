/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#pragma once

#include <gtk/gtk.h>
#include <gst/gst.h>

enum player_accel
{
	PAUSE,
	FRAME,
	SLIDER,
	OPEN_DIR,
	OPEN_FILES
};

#define HELIA_TYPE_PLAYER                        helia_player_get_type ()

G_DECLARE_FINAL_TYPE ( HeliaPlayer, helia_player, HELIA, PLAYER, GtkBox )

HeliaPlayer * helia_player_new ( void );

void player_action_accels ( enum player_accel, HeliaPlayer * );

void helia_player_start ( GFile **, int, HeliaPlayer * );

