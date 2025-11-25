/*
 * pipeline.cpp
 * Implementación del pipeline GStreamer
 */

#include "pipeline.hpp"
#include "config/track_info.hpp"
#include "roi/render.h"
#include "report/report.hpp"
#include "gstnvdsmeta.h"
#include <unordered_map>
#include <sys/stat.h>

// Variable global para el contexto del pipeline (usado por callbacks)
static PipelineContext *g_pipeline_ctx = NULL;

// Verifica si un archivo existe
static gboolean file_exists(const gchar *filepath) {
    struct stat buffer;
    return (stat(filepath, &buffer) == 0);
}

void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstElement *parser = GST_ELEMENT(data);
    GstPad *sinkpad = gst_element_get_static_pad(parser, "sink");
    if (!sinkpad) {
        g_printerr("on_pad_added: could not get parser sink pad\n");
        return;
    }

    if (gst_pad_is_linked(sinkpad)) {
        gst_object_unref(sinkpad);
        return;
    }

    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, NULL);
    }
    if (!caps) {
        g_printerr("on_pad_added: could not get caps from demux pad\n");
        gst_object_unref(sinkpad);
        return;
    }

    const GstStructure *str = gst_caps_get_structure(caps, 0);
    const gchar *name = gst_structure_get_name(str);

    if (g_str_has_prefix(name, "video")) {
        if (GST_PAD_LINK_FAILED(gst_pad_link(pad, sinkpad))) {
            g_printerr("Error: Failed to link demuxer to parser\n");
        } else {
            g_print("Linked demuxer pad (%s) to parser\n", name);
        }
    }

    gst_caps_unref(caps);
    gst_object_unref(sinkpad);
}

gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            if (g_pipeline_ctx) {
                generate_report(g_pipeline_ctx->tracker, 
                              g_pipeline_ctx->config->report_file);
            }
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            gst_message_parse_error(msg, &error, &debug);
            g_printerr("ERROR from element %s: %s\n", 
                      GST_OBJECT_NAME(msg->src), error->message);
            if (debug) {
                g_printerr("Debug info: %s\n", debug);
            }
            g_free(debug);
            g_error_free(error);
            g_main_loop_quit(loop);
            break;
        }
        case GST_MESSAGE_WARNING: {
            gchar *debug;
            GError *warning;
            gst_message_parse_warning(msg, &warning, &debug);
            g_printerr("WARNING from element %s: %s\n",
                      GST_OBJECT_NAME(msg->src), warning->message);
            if (debug) {
                g_printerr("Debug info: %s\n", debug);
            }
            g_free(debug);
            g_error_free(warning);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

GstPadProbeReturn osd_sink_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, 
                                            gpointer u_data) {
    GstBuffer *buf = (GstBuffer *)info->data;
    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);
    
    if (!batch_meta || !g_pipeline_ctx) return GST_PAD_PROBE_OK;
    
    TrackerContext *tracker = g_pipeline_ctx->tracker;
    tracker->roi_has_objects = FALSE;
    tracker->roi_has_alerts = FALSE;
    std::unordered_map<guint64, bool> active_tracks;
    
    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame; 
         l_frame = l_frame->next) {
        NvDsFrameMeta *fmeta = (NvDsFrameMeta *)l_frame->data;
        
        if (tracker->source_width == 0) {
            tracker->source_width = fmeta->source_frame_width;
            tracker->source_height = fmeta->source_frame_height;
        }
        
        for (NvDsMetaList *l_obj = fmeta->obj_meta_list; l_obj; l_obj = l_obj->next) {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)l_obj->data;

            if (!obj_meta)
                continue;

            if (obj_meta->obj_label == nullptr ||
                g_strcmp0(obj_meta->obj_label, "car") != 0) {
                continue;  // ignorar todo lo que no sea un carro
            }

            guint64 track_id = obj_meta->object_id;
            active_tracks[track_id] = true;

            tracker_process_object(
                tracker,
                obj_meta,
                fmeta->source_frame_width,
                fmeta->source_frame_height
            );
        }

        
        draw_roi_rect(batch_meta, fmeta, &tracker->roi, 
                     tracker->roi_has_objects, tracker->roi_has_alerts);
    }
    
    return GST_PAD_PROBE_OK;
}

gboolean pipeline_create(PipelineContext *ctx) {
    g_pipeline_ctx = ctx;
    
    // Verificar archivo de entrada
    if (!file_exists(ctx->config->input_file)) {
        g_printerr("ERROR: Input file not found: %s\n", ctx->config->input_file);
        return FALSE;
    }
    
    g_print("Input file verified: %s\n", ctx->config->input_file);
    
    // Detectar modo UDP
    gboolean use_udp = (g_strcmp0(ctx->config->mode, "udp") == 0);
    
    GstElement *source, *demux, *parser, *decoder, *streammux;
    GstElement *pgie, *tracker_elem, *nvvidconv, *nvosd;
    GstElement *nvvidconv2, *capsfilter, *encoder, *parser2, *mux, *sink;
    GstPad *osd_sink_pad;
    GstBus *bus;

    ctx->pipeline = gst_pipeline_new("secure-roi-pipeline");
    if (!ctx->pipeline) {
        g_printerr("Failed to create pipeline\n");
        return FALSE;
    }

    /* Source & decode */
    source     = gst_element_factory_make("filesrc",        "file-source");
    demux      = gst_element_factory_make("qtdemux",        "demuxer");
    parser     = gst_element_factory_make("h264parse",      "h264-parser");
    decoder    = gst_element_factory_make("nvv4l2decoder",  "decoder");

    /* DeepStream core */
    streammux  = gst_element_factory_make("nvstreammux",    "stream-muxer");
    pgie       = gst_element_factory_make("nvinfer",        "primary-infer");
    tracker_elem = gst_element_factory_make("nvtracker",    "tracker");
    nvvidconv  = gst_element_factory_make("nvvideoconvert", "nvvideo-converter");
    nvosd      = gst_element_factory_make("nvdsosd",        "nv-onscreendisplay");

    /* Post-OSD -> encoder */
    nvvidconv2 = gst_element_factory_make("nvvideoconvert", "post-osd-conv");
    capsfilter = gst_element_factory_make("capsfilter",     "capsfilter");
    encoder    = gst_element_factory_make("nvv4l2h264enc",  "h264-encoder");
    parser2    = gst_element_factory_make("h264parse",      "h264-parser-out");
    
    if (use_udp) {
        // Modo UDP: RTP payloader + UDP sink
        mux = gst_element_factory_make("rtph264pay", "rtp-payloader");
        sink = gst_element_factory_make("udpsink", "udp-sink");
        g_print("Mode: UDP STREAMING\n");
    } else {
        // Modo archivo: MP4 muxer + file sink
        mux = gst_element_factory_make("qtmux", "mp4-muxer");
        sink = gst_element_factory_make("filesink", "file-sink");
        g_print("Mode: FILE OUTPUT\n");
    }

    if (!source || !demux || !parser || !decoder || !streammux ||
        !pgie || !tracker_elem || !nvvidconv || !nvosd ||
        !nvvidconv2 || !capsfilter || !encoder || !parser2 || !mux || !sink) {
        g_printerr("Failed to create one or more elements\n");
        return FALSE;
    }

    g_print("All GStreamer elements created successfully\n");

    /* Configure elements */
    g_object_set(G_OBJECT(source), "location", ctx->config->input_file, NULL);
    g_print("Configured source: %s\n", ctx->config->input_file);

    // Usar la resolución detectada del video
    g_object_set(G_OBJECT(streammux),
                 "batch-size", 1,
                 "width", ctx->stream_width,
                 "height", ctx->stream_height,
                 "batched-push-timeout", 4000000,
                 "live-source", 0,
                 NULL);
    g_print("Configured streammux: %dx%d\n", ctx->stream_width, ctx->stream_height);

    // Configurar PGIE
    const gchar *pgie_config = "/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_infer_primary.txt";
    if (!file_exists(pgie_config)) {
        pgie_config = "/opt/nvidia/deepstream/deepstream-6.0/samples/configs/deepstream-app/config_infer_primary.txt";
    }
    g_object_set(G_OBJECT(pgie), "config-file-path", pgie_config, NULL);
    g_print("Configured PGIE\n");

    // Configurar tracker
    const gchar *tracker_config = "/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_tracker_NvDCF_perf.yml";
    const gchar *tracker_lib = "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so";
    if (!file_exists(tracker_config)) {
        tracker_config = "/opt/nvidia/deepstream/deepstream-6.0/samples/configs/deepstream-app/config_tracker_NvDCF_perf.yml";
    }
    if (!file_exists(tracker_lib)) {
        tracker_lib = "/opt/nvidia/deepstream/deepstream-6.0/lib/libnvds_nvmultiobjecttracker.so";
    }
    g_object_set(G_OBJECT(tracker_elem),
                 "tracker-width", 640,
                 "tracker-height", 384,
                 "ll-lib-file", tracker_lib,
                 "ll-config-file", tracker_config,
                 NULL);
    g_print("Configured tracker\n");

    // Configurar encoder
    g_object_set(G_OBJECT(encoder),
                 "bitrate", 4000000,
                 "preset-level", 1,
                 "insert-sps-pps", TRUE,
                 "iframeinterval", 30,
                 NULL);
    g_print("Configured encoder\n");

    // Configurar sink según modo
    if (use_udp) {
        g_object_set(G_OBJECT(mux),
                     "config-interval", 1,
                     "pt", 96,
                     NULL);
        g_object_set(G_OBJECT(sink),
                     "host", ctx->config->udp_host,
                     "port", ctx->config->udp_port,
                     "async", FALSE,
                     "sync", FALSE,
                     NULL);
        g_print("Configured UDP sink: %s:%d\n", ctx->config->udp_host, ctx->config->udp_port);
    } else {
        g_object_set(G_OBJECT(sink),
                     "location", ctx->config->output_file,
                     "sync", FALSE,
                     "async", FALSE,
                     NULL);
        g_print("Configured file sink: %s\n", ctx->config->output_file);
    }

    GstCaps *caps = gst_caps_from_string("video/x-raw(memory:NVMM), format=NV12");
    g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    /* Add all elements to the bin */
    gst_bin_add_many(GST_BIN(ctx->pipeline),
                     source, demux, parser, decoder,
                     streammux, pgie, tracker_elem,
                     nvvidconv, nvosd,
                     nvvidconv2, capsfilter, encoder, parser2, mux, sink,
                     NULL);
    g_print("All elements added to pipeline\n");

    /* Link elements */
    if (!gst_element_link(source, demux)) {
        g_printerr("Failed to link source -> demux\n");
        return FALSE;
    }
    g_print("Linked: source -> demux\n");

    g_signal_connect(demux, "pad-added", G_CALLBACK(on_pad_added), parser);

    if (!gst_element_link_many(parser, decoder, NULL)) {
        g_printerr("Failed to link parser -> decoder\n");
        return FALSE;
    }
    g_print("Linked: parser -> decoder\n");

    GstPad *decoder_src = gst_element_get_static_pad(decoder, "src");
    GstPad *mux_sink = gst_element_get_request_pad(streammux, "sink_0");
    if (gst_pad_link(decoder_src, mux_sink) != GST_PAD_LINK_OK) {
        g_printerr("Failed to link decoder -> streammux\n");
        gst_object_unref(decoder_src);
        gst_object_unref(mux_sink);
        return FALSE;
    }
    gst_object_unref(decoder_src);
    gst_object_unref(mux_sink);
    g_print("Linked: decoder -> streammux\n");

    if (!gst_element_link_many(streammux, pgie, tracker_elem,
                               nvvidconv, nvosd,
                               nvvidconv2, capsfilter,
                               encoder, parser2, mux, sink, NULL)) {
        g_printerr("Failed to link main pipeline\n");
        return FALSE;
    }
    
    if (use_udp) {
        g_print("Linked: streammux -> ... -> rtph264pay -> udpsink\n");
    } else {
        g_print("Linked: streammux -> ... -> qtmux -> filesink\n");
    }

    /* OSD pad probe */
    osd_sink_pad = gst_element_get_static_pad(nvosd, "sink");
    if (!osd_sink_pad) {
        g_printerr("Failed to get osd sink pad\n");
        return FALSE;
    }
    gst_pad_add_probe(osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      osd_sink_pad_buffer_probe, NULL, NULL);
    gst_object_unref(osd_sink_pad);
    g_print("OSD pad probe added\n");

    /* Bus */
    bus = gst_pipeline_get_bus(GST_PIPELINE(ctx->pipeline));
    gst_bus_add_watch(bus, bus_call, ctx->loop);
    gst_object_unref(bus);
    g_print("Bus configured\n");

    g_print("Pipeline created successfully!\n");
    return TRUE;
}
