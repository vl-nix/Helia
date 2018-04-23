/*
 * Copyright 2014 - 2018 Stepan Perun
 * This program is free software.
 * License: Gnu Lesser General Public License
 * http://www.gnu.org/licenses/lgpl.html
*/


#define GST_USE_UNSTABLE_API
#include <gst/mpegts/mpegts.h>

#include "gmp-mpegts.h"
#include "gmp-scan.h"


void gmp_mpegts_initialize ()
{
    gst_mpegts_initialize ();
    g_type_class_ref ( GST_TYPE_MPEGTS_STREAM_TYPE          );
    g_type_class_ref ( GST_TYPE_MPEGTS_DVB_SERVICE_TYPE     );
    //g_type_class_ref ( GST_TYPE_MPEGTS_DESCRIPTOR_TYPE      );
    //g_type_class_ref ( GST_TYPE_MPEGTS_DVB_DESCRIPTOR_TYPE  );
}

const gchar * dvb_mpegts_enum_name ( GType instance_type, gint val )
{
    GEnumValue *env = g_enum_get_value ( G_ENUM_CLASS ( g_type_class_peek ( instance_type ) ), val );

    if ( !env )
        return "Unknown";

    return env->value_nick;
}

void gmp_mpegts_clear_scan ()
{
    
    gtvmpegts.pat_done = FALSE;
    gtvmpegts.pmt_done = FALSE;
    gtvmpegts.sdt_done = FALSE;
    gtvmpegts.vct_done = FALSE;

    gtvmpegts.pat_count = 0;
    gtvmpegts.pmt_count = 0;
    gtvmpegts.sdt_count = 0;
    gtvmpegts.vct_count = 0;

	guint j = 0;
    for ( j = 0; j < MAX_RUN_PAT; j++ )
    {
        dvb_gst_pmt_n[j].pmn_pid = 0;
        dvb_gst_pmt_n[j].apid = 0;
        dvb_gst_pmt_n[j].vpid = 0;

        dvb_gst_pat_n[j].pmn_pid = 0;
        dvb_gst_pat_n[j].nmap = 0;

        dvb_gst_sdt_n[j].pmn_pid = 0;
        dvb_gst_sdt_n[j].name = NULL;

        dvb_gst_vct_n[j].pmn_pid = 0;
        dvb_gst_vct_n[j].name = NULL;
    }
}


static void gmp_mpegts_pat ( GstMpegtsSection *section )
{
    GPtrArray *pat = gst_mpegts_section_get_pat ( section );
    g_debug ( "\nPAT: %d Programs", pat->len );

    guint i = 0;
    for ( i = 0; i < pat->len; i++ )
    {
        if ( i >= MAX_RUN_PAT )
        {
			g_debug ( "MAX %d: PAT scan stop  \n", MAX_RUN_PAT );
			break;
		}
		
        GstMpegtsPatProgram *patp = g_ptr_array_index ( pat, i );

        if ( patp->program_number == 0 ) continue;

        dvb_gst_pat_n[gtvmpegts.pat_count].pmn_pid = patp->program_number;
        dvb_gst_pat_n[gtvmpegts.pat_count].nmap    = patp->network_or_program_map_PID;
        gtvmpegts.pat_count++;

        g_debug ( "     Program number: %6d (0x%04x) | network or pg-map pid: 0x%04x",
            patp->program_number, patp->program_number, patp->network_or_program_map_PID );
    }

    g_ptr_array_unref ( pat );

    gtvmpegts.pat_done = TRUE;
    g_debug ( "PAT Done  \n" );
}

static void gmp_mpegts_pmt ( GstMpegtsSection *section )
{
    if ( gtvmpegts.pmt_count >= MAX_RUN_PAT )
    {
        g_debug ( "MAX %d: PMT scan stop  \n", MAX_RUN_PAT );
        return;
    }

    guint i = 0, c = 0, len = 0;
    gboolean first_audio = TRUE;

    const GstMpegtsPMT *pmt = gst_mpegts_section_get_pmt ( section );
    len = pmt->streams->len;
    dvb_gst_pmt_n[gtvmpegts.pmt_count].pmn_pid = pmt->program_number;

    g_debug ( "PMT: %d  ( %d )", gtvmpegts.pmt_count+1, len );

    g_debug ( "     Program number     : %d (0x%04x)", pmt->program_number, pmt->program_number );
    g_debug ( "     Pcr pid            : %d (0x%04x)", pmt->pcr_pid, pmt->pcr_pid );
    g_debug ( "     %d Streams:", len );

    for ( i = 0; i < len; i++ )
    {
        GstMpegtsPMTStream *stream = g_ptr_array_index ( pmt->streams, i );

        g_debug ( "       pid: %d (0x%04x), stream_type:0x%02x (%s)", stream->pid, stream->pid, stream->stream_type,
            dvb_mpegts_enum_name (GST_TYPE_MPEGTS_STREAM_TYPE, stream->stream_type) );

        const gchar *name_t = dvb_mpegts_enum_name ( GST_TYPE_MPEGTS_STREAM_TYPE, stream->stream_type );

        if ( g_strrstr ( name_t, "video" ) )
			dvb_gst_pmt_n[gtvmpegts.pmt_count].vpid = stream->pid;

        if ( g_strrstr ( name_t, "audio" ) )
        {
            if ( first_audio ) dvb_gst_pmt_n[gtvmpegts.pmt_count].apid = stream->pid;

            first_audio = FALSE;

            gchar *lang = NULL;

                GPtrArray *descriptors = stream->descriptors;
                for ( c = 0; c < descriptors->len; c++ )
                {
                    GstMpegtsDescriptor *desc = g_ptr_array_index ( descriptors, c );

                    GstMpegtsISO639LanguageDescriptor *res;
                    if (  gst_mpegts_descriptor_parse_iso_639_language ( desc, &res )  )
                    {
                        lang = g_strdup ( res->language[0] );
                        gst_mpegts_iso_639_language_descriptor_free (res);
                    }
                }

                g_debug ( "       lang: %s | %d", lang ? lang : "none", stream->pid );

            if ( lang ) g_free ( lang );
        }
    }

    gtvmpegts.pmt_count++;

    if ( gtvmpegts.pat_count > 0 && gtvmpegts.pmt_count == gtvmpegts.pat_count )
    {
        gtvmpegts.pmt_done = TRUE;
        g_debug ( "PMT Done \n" );
    }
}

static void gmp_mpegts_sdt ( GstMpegtsSection *section )
{
    if ( gtvmpegts.sdt_count >= MAX_RUN_PAT )
    {
        g_debug ( "MAX %d: SDT scan stop  \n", MAX_RUN_PAT );
        return;
    }

    guint i = 0, z = 0, c = 0, len = 0;

    const GstMpegtsSDT *sdt = gst_mpegts_section_get_sdt ( section );

    len = sdt->services->len;
    g_debug ( "Services: %d  ( %d )", gtvmpegts.sdt_count+1, len );

    for ( i = 0; i < len; i++ )
    {
		if ( i >= MAX_RUN_PAT ) break;
		
        GstMpegtsSDTService *service = g_ptr_array_index ( sdt->services, i );

        dvb_gst_sdt_n[gtvmpegts.sdt_count].name = NULL;
        dvb_gst_sdt_n[gtvmpegts.sdt_count].pmn_pid = service->service_id;

        gboolean get_descr = FALSE;

        if ( gtvmpegts.pat_done )
        {
            for ( z = 0; z < gtvmpegts.pat_count; z++ )
                if ( dvb_gst_pat_n[z].pmn_pid == service->service_id )
                    {  get_descr = TRUE; break; }
        }

        if ( !get_descr ) continue;

        g_debug ( "     Service id: %d  %d 0x%04x", gtvmpegts.sdt_count+1, service->service_id, service->service_id );

        GPtrArray *descriptors = service->descriptors;
        for ( c = 0; c < descriptors->len; c++ )
        {
            GstMpegtsDescriptor *desc = g_ptr_array_index ( descriptors, c );

            gchar *service_name, *provider_name;
            GstMpegtsDVBServiceType service_type;

            if ( desc->tag == GST_MTS_DESC_DVB_SERVICE )
            {
                if ( gst_mpegts_descriptor_parse_dvb_service ( desc, &service_type, &service_name, &provider_name ) )
                {
                    dvb_gst_sdt_n[gtvmpegts.sdt_count].name = g_strdup ( service_name );

                    g_debug ( "   Service Descriptor, type:0x%02x (%s)",
                        service_type, dvb_mpegts_enum_name (GST_TYPE_MPEGTS_DVB_SERVICE_TYPE, service_type) );
                    g_debug ( "      Service  (name) : %s", service_name  );
                    g_debug ( "      Provider (name) : %s", provider_name );

                    g_free ( service_name  );
                    g_free ( provider_name );
                }
            }
        }

        if ( dvb_gst_sdt_n[gtvmpegts.sdt_count].name == NULL )
            dvb_gst_sdt_n[gtvmpegts.sdt_count].name = g_strdup_printf ( "Program-%d", service->service_id );

        gtvmpegts.sdt_count++;
    }

    if ( gtvmpegts.pat_count > 0 && gtvmpegts.sdt_count == gtvmpegts.pat_count )
    {
        gtvmpegts.sdt_done = TRUE;
        g_debug ( "SDT Done \n" );
    }
}
static void gmp_mpegts_vct ( GstMpegtsSection *section )
{
    const GstMpegtsAtscVCT *vct;

    if ( GST_MPEGTS_SECTION_TYPE (section) == GST_MPEGTS_SECTION_ATSC_CVCT )
        vct = gst_mpegts_section_get_atsc_cvct ( section );
    else
        vct = gst_mpegts_section_get_atsc_tvct ( section );

    g_assert (vct);
    g_debug ( "VCT" );

    g_debug ( "     transport_stream_id : 0x%04x", vct->transport_stream_id );
    g_debug ( "     protocol_version    : %u", vct->protocol_version );
    g_debug ( "     %d Sources:", vct->sources->len );

    guint i;
    for ( i = 0; i < vct->sources->len; i++ )
    {
        if ( i >= MAX_RUN_PAT )
        {
			g_debug ( "MAX %d: VCT scan stop  \n", MAX_RUN_PAT );
			break;
		}
		
        GstMpegtsAtscVCTSource *source = g_ptr_array_index ( vct->sources, i );

        dvb_gst_vct_n[i].pmn_pid = source->program_number;
        dvb_gst_vct_n[i].name    = g_strdup ( source->short_name );
        
        g_debug ( "     Service id: %d  %d 0x%04x", gtvmpegts.vct_count+1, source->program_number, source->program_number );
        g_debug ( "     Service  (name) : %s", source->short_name );
        
        gtvmpegts.vct_count++;
    }

	gtvmpegts.vct_done = TRUE;
    g_debug ( "VCT Done \n" );
}

void gmp_mpegts_stop ()
{	
    if ( gtvmpegts.pat_done && gtvmpegts.pmt_done && ( gtvmpegts.sdt_done || gtvmpegts.vct_done ) )
		gmp_scan_stop_mpeg ();

    time ( &gtvmpegts.t_cur );
    if ( ( gtvmpegts.t_cur - gtvmpegts.t_start ) >= 11 )
    {
        g_print ( "Warning. Time stop %ld (sec) \n", gtvmpegts.t_cur - gtvmpegts.t_start );
        gmp_scan_stop_mpeg ();
    }
}

void gmp_mpegts_parse_section ( GstMessage *message )
{
    GstMpegtsSection *section = gst_message_parse_mpegts_section ( message );
    
    if ( section )
    {
        switch ( GST_MPEGTS_SECTION_TYPE ( section ) )
        {
            case GST_MPEGTS_SECTION_PAT:
                gmp_mpegts_pat ( section );
                break;

            case GST_MPEGTS_SECTION_PMT:
                gmp_mpegts_pmt ( section );
                break;

            case GST_MPEGTS_SECTION_SDT:
                gmp_mpegts_sdt ( section );
                break;

            case GST_MPEGTS_SECTION_ATSC_CVCT:
            case GST_MPEGTS_SECTION_ATSC_TVCT:
                gmp_mpegts_vct ( section );

            default:
                break;
        }
        
        gst_mpegts_section_unref ( section );
    }
    
    gmp_mpegts_stop ();
}
