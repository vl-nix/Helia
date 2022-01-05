/*
* Copyright 2020 Stepan Perun
* This program is free software.
*
* License: Gnu General Public License GPL-3
* file:///usr/share/common-licenses/GPL-3
* http://www.gnu.org/licenses/gpl-3.0.html
*/

#include "mpegts.h"

#define GST_USE_UNSTABLE_API
#include <gst/mpegts/mpegts.h>

void mpegts_initialize ( void )
{
	gst_mpegts_initialize ();

	g_type_class_ref ( GST_TYPE_MPEGTS_SECTION_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_SECTION_TABLE_ID );
	g_type_class_ref ( GST_TYPE_MPEGTS_RUNNING_STATUS );
	g_type_class_ref ( GST_TYPE_MPEGTS_DESCRIPTOR_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_DVB_DESCRIPTOR_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_MISC_DESCRIPTOR_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_ISO639_AUDIO_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_DVB_SERVICE_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_DVB_TELETEXT_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_STREAM_TYPE );
	g_type_class_ref ( GST_TYPE_MPEGTS_SECTION_DVB_TABLE_ID );
	g_type_class_ref ( GST_TYPE_MPEGTS_COMPONENT_STREAM_CONTENT );
}

void mpegts_clear ( MpegTs *mpegts )
{
	mpegts->pat_done = FALSE;
	mpegts->pmt_done = FALSE;
	mpegts->sdt_done = FALSE;

	mpegts->pat_count = 0;
	mpegts->pmt_count = 0;
	mpegts->sdt_count = 0;

	uint8_t j = 0; for ( j = 0; j < MAX_RUN_PAT; j++ )
	{
		mpegts->pids[j].pat_sid = 0;
		mpegts->pids[j].pmt_sid = 0;
		mpegts->pids[j].sdt_sid = 0;

		mpegts->pids[j].pmt_apid = 0;
		mpegts->pids[j].pmt_vpid = 0;

		mpegts->pids[j].ch_name = NULL;
	}
}

static const char * mpegts_enum_name ( GType instance_type, int val )
{
	GEnumValue *en = g_enum_get_value ( G_ENUM_CLASS ( g_type_class_peek ( instance_type ) ), val );

	if ( en == NULL ) return "Unknown";

	return en->value_nick;
}

static void mpegts_pat ( GstMpegtsSection *section, MpegTs *mpegts )
{
	GPtrArray *pat = gst_mpegts_section_get_pat ( section );

	if ( mpegts->debug ) g_message ( "PAT: %u Programs ", pat->len );

	uint8_t i = 0; for ( i = 0; i < pat->len; i++ )
	{
		if ( i >= MAX_RUN_PAT )
		{
			g_print ( "MAX %u: PAT scan stop  \n", MAX_RUN_PAT );
			break;
		}

		GstMpegtsPatProgram *patp = g_ptr_array_index ( pat, i );

		if ( patp->program_number == 0 ) continue;

		mpegts->pids[mpegts->pat_count].pat_sid = patp->program_number;
		mpegts->pat_count++;

		if ( mpegts->debug ) g_message ( "     Program number: %u (0x%04x) | network or pg-map pid: 0x%04x ", 
				patp->program_number, patp->program_number, patp->network_or_program_map_PID );
	}

	g_ptr_array_unref ( pat );

	mpegts->pat_done = TRUE;
	if ( mpegts->debug ) g_message ( "PAT Done: pat_count %u \n", mpegts->pat_count );
}

static void mpegts_pmt ( GstMpegtsSection *section, MpegTs *mpegts )
{
	if ( mpegts->pmt_count >= MAX_RUN_PAT )
	{
		g_print ( "MAX %u: PMT scan stop  \n", MAX_RUN_PAT );
		return;
	}

	uint8_t i = 0, len = 0;
	gboolean first_audio = TRUE;

	const GstMpegtsPMT *pmt = gst_mpegts_section_get_pmt ( section );
	len = (uint8_t)pmt->streams->len;

	mpegts->pids[mpegts->pmt_count].pmt_sid = pmt->program_number;

	if ( mpegts->debug ) g_message ( "PMT: %u  ( %u )", mpegts->pmt_count + 1, len );

	if ( mpegts->debug ) g_message ( "     Program number     : %u (0x%04x) ", pmt->program_number, pmt->program_number );
	if ( mpegts->debug ) g_message ( "     Pcr pid            : %u (0x%04x) ", pmt->pcr_pid, pmt->pcr_pid );
	if ( mpegts->debug ) g_message ( "     %u Streams: ", len );

	for ( i = 0; i < len; i++ )
	{
		GstMpegtsPMTStream *stream = g_ptr_array_index ( pmt->streams, i );

		if ( mpegts->debug ) g_message ( "       pid: %u (0x%04x), stream_type:0x%02x (%s) ", stream->pid, stream->pid, stream->stream_type,
			mpegts_enum_name (GST_TYPE_MPEGTS_STREAM_TYPE, stream->stream_type) );

		const char *name_t = mpegts_enum_name ( GST_TYPE_MPEGTS_STREAM_TYPE, stream->stream_type );

		if ( g_strrstr ( name_t, "video" ) )
			mpegts->pids[mpegts->pmt_count].pmt_vpid = stream->pid;

		if ( g_strrstr ( name_t, "audio" ) )
		{
			if ( first_audio )
				mpegts->pids[mpegts->pmt_count].pmt_apid = stream->pid;

			first_audio = FALSE;
		}
	}

	mpegts->pmt_count++;

	if ( mpegts->pat_count > 0 && mpegts->pat_count == mpegts->pmt_count )
	{
		mpegts->pmt_done = TRUE;
		if ( mpegts->debug ) g_message ( "PMT Done: pmt_count %u \n", mpegts->pmt_count );
	}
}

static void mpegts_sdt ( GstMpegtsSection *section, MpegTs *mpegts )
{
	if ( mpegts->sdt_done ) return;

	if ( mpegts->sdt_count >= MAX_RUN_PAT )
	{
		g_print ( "MAX %u: SDT scan stop  \n", MAX_RUN_PAT );
		return;
	}

	uint8_t i = 0, z = 0, c = 0, len = 0;

	const GstMpegtsSDT *sdt = gst_mpegts_section_get_sdt ( section );

	len = (uint8_t)sdt->services->len;
	if ( mpegts->debug ) g_message ( "Services: %u  ( %u ) ", mpegts->sdt_count + 1, len );

	for ( i = 0; i < len; i++ )
	{
		if ( i >= MAX_RUN_PAT ) break;

		GstMpegtsSDTService *service = g_ptr_array_index ( sdt->services, i );

		mpegts->pids[mpegts->sdt_count].ch_name = NULL;
		mpegts->pids[mpegts->sdt_count].sdt_sid = service->service_id;

		gboolean get_descr = FALSE;

		if ( mpegts->pat_done )
		{
			for ( z = 0; z < mpegts->pat_count; z++ )
				if ( mpegts->pids[z].pat_sid == service->service_id )
					{  get_descr = TRUE; break; }
		}

		if ( !get_descr ) continue;

		if ( mpegts->debug ) g_message ( "  Service id:  %u | %u  ( 0x%04x ) ", mpegts->sdt_count + 1, service->service_id, service->service_id );

		GPtrArray *descriptors = service->descriptors;
		for ( c = 0; c < descriptors->len; c++ )
		{
			GstMpegtsDescriptor *desc = g_ptr_array_index ( descriptors, c );

			char *service_name, *provider_name;
			GstMpegtsDVBServiceType service_type;

			if ( desc->tag == GST_MTS_DESC_DVB_SERVICE )
			{
				if ( gst_mpegts_descriptor_parse_dvb_service ( desc, &service_type, &service_name, &provider_name ) )
				{
					mpegts->pids[mpegts->sdt_count].ch_name = g_strdup ( service_name );

					if ( mpegts->debug ) g_message ( "    Service Descriptor, type:0x%02x (%s) ",
						service_type, mpegts_enum_name (GST_TYPE_MPEGTS_DVB_SERVICE_TYPE, service_type) );
					if ( mpegts->debug ) g_message ( "    Service  (name) : %s ", service_name  );
					if ( mpegts->debug ) g_message ( "    Provider (name) : %s \n", provider_name );

					free ( service_name  );
					free ( provider_name );
				}
			}

		}

		mpegts->sdt_count++;
	}

	if ( mpegts->pat_count > 0 && mpegts->pat_count == mpegts->sdt_count )
	{
		mpegts->sdt_done = TRUE;
		if ( mpegts->debug ) g_message ( "SDT Done: sdt_count %u \n", mpegts->sdt_count );
	}
}

void mpegts_parse_section ( GstMessage *message, MpegTs *mpegts )
{
	GstMpegtsSection *section = gst_message_parse_mpegts_section ( message );

	if ( section )
	{
		switch ( GST_MPEGTS_SECTION_TYPE ( section ) )
		{
			case GST_MPEGTS_SECTION_PAT:
				mpegts_pat ( section, mpegts );
				break;

			case GST_MPEGTS_SECTION_PMT:
				mpegts_pmt ( section, mpegts );
				break;

			case GST_MPEGTS_SECTION_SDT:
				mpegts_sdt ( section, mpegts );
				break;

			default:
			break;
		}

		gst_mpegts_section_unref ( section );
	}
}
