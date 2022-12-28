/*
* Copyright 2022 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#pragma once

#include <gtk/gtk.h>

enum ScanPage
{
	PAGE_SC,
	PAGE_DT,
	PAGE_DS,
	PAGE_DC,
	PAGE_AT,
	PAGE_DM,
	PAGE_CH,
	PAGE_NUM
};

enum Lnbs
{
	LNB_UNV,
	LNB_DBS,
	LNB_EXT,
	LNB_STD,
	LNB_EHD,
	LNB_CBD,
	LNB_CMT,
	LNB_BJ1,
	LNB_QPH,
	LNB_BRO,
	LNB_MNL
};
