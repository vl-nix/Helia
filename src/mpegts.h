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

#define MAX_RUN_PAT 128

typedef struct _PatPmtSdt PatPmtSdt;

struct _PatPmtSdt
{
	uint16_t pat_sid, pmt_sid, sdt_sid;
	uint16_t pmt_vpid, pmt_apid;

	char *ch_name;
};

typedef struct _MpegTs MpegTs;

struct _MpegTs
{
	gboolean pat_done, pmt_done, sdt_done;
	uint8_t pat_count, pmt_count, sdt_count;

	PatPmtSdt pids[MAX_RUN_PAT];

	gboolean debug;
};


void mpegts_initialize ( void );

void mpegts_clear ( MpegTs * );

void mpegts_parse_section ( GstMessage *, MpegTs * );

