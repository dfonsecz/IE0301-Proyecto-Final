#include <iostream>
#include <gst/gst.h>
#include "pipeline/pipeline.hpp"

// Constructor
Pipeline::Pipeline(const AppConfig& cfg, ROI& r) {
    config = cfg;
    roi = r;
    app_timer = g_timer_new();
}

// Destructor
Pipeline::~Pipeline() {
    cleanup();
}

// Initialization
bool Pipeline::init() {
    loop = g_main_loop_new(nullptr, FALSE);
    return create_pipeline();
}

// Execute pipeline
void Pipeline::run() {
    if (!pipeline || !loop) {
        std::cerr << "Pipeline was not initialized.";
        return;
    } else {
        std::cout << "Pipeline executing...";
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);
}

// Cleanup
void Pipeline::cleanup() {
    tracked_objects.clear();

    if (app_timer) {
        g_timer_destroy(app_timer);
        app_timer = nullptr;
    }

    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    if (loop) {
        g_main_loop_unref(loop);
        loop = nullptr;
    }
}

// Pipeline creation
bool Pipeline::create_pipeline() {
    pipeline = gst_pipeline_new("secure-roi-pipeline");
    
    if (!pipeline) {
        std::cerr << "Failed to create pipeline\n";
        return false;
    }

    // Create elements
    GstElement *source, *demux, *parser, *decoder, *streammux;
    GstElement *pgie, *tracker, *nvvidconv, *nvosd;
    GstElement *nvvidconv2, *capsfilter, *encoder, *parser2, *mux, *sink;
    GstPad *osd_sink_pad;
    GstBus *bus;

    // Source and decode
    source     = gst_element_factory_make("filesrc",        "file-source");
    demux      = gst_element_factory_make("qtdemux",        "demuxer");
    parser     = gst_element_factory_make("h264parse",      "h264-parser");
    decoder    = gst_element_factory_make("nvv4l2decoder",  "decoder");

    // DeepStream code
    streammux  = gst_element_factory_make("nvstreammux",    "stream-muxer");
    pgie       = gst_element_factory_make("nvinfer",        "primary-infer");
    tracker    = gst_element_factory_make("nvtracker",      "tracker");
    nvvidconv  = gst_element_factory_make("nvvideoconvert", "nvvideo-converter");
    nvosd      = gst_element_factory_make("nvdsosd",        "nv-onscreendisplay");

    // Post-OSD -> encoder -> mp4
    nvvidconv2 = gst_element_factory_make("nvvideoconvert", "post-osd-conv");
    capsfilter = gst_element_factory_make("capsfilter",     "capsfilter");
    encoder    = gst_element_factory_make("nvv4l2h264enc",  "h264-encoder");
    parser2    = gst_element_factory_make("h264parse",      "h264-parser-out");
    mux        = gst_element_factory_make("qtmux",          "mp4-muxer");
    sink       = gst_element_factory_make("filesink",       "file-sink");

    if (!source || !demux || !parser || !decoder || !streammux ||
        !pgie || !tracker || !nvvidconv || !nvosd ||
        !nvvidconv2 || !capsfilter || !encoder || !parser2 || !mux || !sink) {
        g_printerr("Failed to create one or more elements\n");
        return false;
    }
    
    // Configure elements
    g_object_set(source, "location", config.input_file().c_str(), NULL);
    
    g_object_set(streammux,
                "batch-size", 1,
                "width", 1920,
                "height", 1080,
                "batched-push-timeout", 4000000, // uSec
                NULL);
    
    g_object_set(pgie,
                "config-file-path",
                "/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_infer_primary.txt",
                NULL);
    
    g_object_set(tracker,
                "tracker-width", 640,
                "tracker-height", 384,
                "ll-lib-file", "/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so",
                "ll-config-file",
                "/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_tracker_NvDCF_perf.yml",
                NULL);

    g_object_set(encoder,
                 "bitrate", 4000000, // 4 Mbps
                 "preset-level", 1,
                 "insert-sps-pps", true,
                 "iframeinterval", 30,
                 NULL);

    g_object_set(sink,
                "location", config.output_file().c_str(),
                "sync", false, // Don't sync to clock
                "async", false,
                NULL);

    // Caps
    GstCaps *caps = gst_caps_from_string("video/x-raw(memory:NVMM), format=NV12");
    g_object_set(capsfilter, "caps", caps, NULL);
    gst_caps_unref(caps);

    // Add elements to bin
    gst_bin_add_many(GST_BIN(pipeline),
                    source, demux, parser, decoder,
                    streammux, pgie, tracker, nvvidconv, nvosd,
                    nvvidconv2, capsfilter, encoder, parser2, mux, sink,
                    NULL);

    // Linking
    if (!gst_element_link(source, demux)) {
        g_printerr("Failed to link source -> demux\n");
        return false;
    }
    
    g_signal_connect(demux,
                    "pad-added",
                    G_CALLBACK(Pipeline::on_pad_added_callback),
                    parser);

    if (!gst_element_link_many(parser, decoder, NULL)) {
        std::cerr << "Failed to link parser -> decoder\n";
        return false;
    }

    // Request pad
    GstPad *dec_src  = gst_element_get_static_pad(decoder, "src");
    GstPad *mux_sink = gst_element_get_request_pad(streammux, "sink_0");

    if (gst_pad_link(dec_src, mux_sink) != GST_PAD_LINK_OK) {
        std::cerr << "Failed to link decoder -> streammux\n";
        gst_object_unref(dec_src);
        gst_object_unref(mux_sink);
        return false;
    }

    gst_object_unref(dec_src);
    gst_object_unref(mux_sink);

    // Full path to output
    if (!gst_element_link_many(streammux, pgie, tracker, nvvidconv, nvosd,
                               nvvidconv2, capsfilter, encoder, parser2, mux, sink, NULL)) {
        std::cerr << "Failed to link to main pipeline\n";
        return false;
    }

    // Pad probe -> attach callback
    osd_sink_pad = gst_element_get_static_pad(nvosd, "sink");
    gst_pad_add_probe(osd_sink_pad,
                      GST_PAD_PROBE_TYPE_BUFFER,
                      Pipeline::pad_probe_callback,
                      this,
                      NULL);
    gst_object_unref(osd_sink_pad);

    // Bus
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, Pipeline::bus_callback, this);
    gst_object_unref(bus);

    return true;
}