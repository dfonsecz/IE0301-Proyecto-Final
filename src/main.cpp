#include <gst/gst.h>
#include "roi_probe.hpp"
#include "config_parser.hpp"

/*
@brief Callback function to handle messages from GStreamer bus.
@param bus GStreamer bus.
@param msg GStreamer message.
@param data User data (GMainLoop pointer).
@return TRUE to continue receiving messages.
*/
static gboolean busCall(GstBust *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *) data;
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS: {
            std::cout << "End of stream" << std::endl;
            g_main_loop_quit(loop);
            break;
        }
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            gst_message_parse_error(msg, &error, &debug);
            g_printerr("Error: %s\n", error->message);
            g_error_free(error);
            g_free(debug);
            g_main_loop_quit(loop);
            break;
        }
        default: break;
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    gst_int(&argc, &argv);

    // Load configuration
    AppCfg cfg;
    if (!load_app_ini("configs/app.ini", cfg)) {
        g_printerr("Error loading app.ini\n");
        return -1;
    }

    // Create elements
    GstElement *pipeline, *source, *demux, *h264parse, *nvv412decoder, *nvvideoconvert, *nvstreammux, *nvinfer, *nvtracker, *nvdsosd, *tee;
    
    pipeline = gst_pipeline_new("ds-pipeline");

    source   = gst_element_factory_make("filesrc", "src");
    demux    = gst_element_factory_make("qtdemux", "demux");
    parse    = gst_element_factory_make("h264parse", "parse");
    decoder  = gst_element_factory_make("nvv412decoder", "decoder");
    conv     = gst_element_factory_make("nvvideoconvert", "conv");
    mux      = gst_element_factory_make("nvstreammux", "mux");
    infer    = gst_element_factory_make("nvinfer", "pgie");
    tracker  = gst_element_factory_make("nvtracker", "tracker");
    osd      = gst_element_factory_make("nvdsosd", "osd");
    tee      = gst_element_factory_make("tee", "tee");

    // Rama display
    GstElement *queue_display, *sink_display;

    queue_display = gst_element_factory_make("queue", "qdisp");
    sink_display  = gst_element_factory_make("nveglglessink", "disp");

    // MP4
    GstElement *queue_mp4, *enc_mp4, *parse_mp4, *mux_mp4, *sink_mp4;

    queue_mp4 = gst_element_factory_make("queue", "qmp4");
    enc_mp4   = gst_element_factory_make("nvv4l2h264enc", "enc_mp4");
    parse_mp4 = gst_element_factory_make("h264parse", "parse_mp4");
    mux_mp4   = gst_element_factory_make("qtmux", "mux_mp4");
    sink_mp4  = gst_element_factory_make("filesink", "out_mp4");

    GstElement *queue_udp, *pay_udp, *sink_udp;

    queue_udp = gst_element_factory_make("queue", "qudp");
    pay_udp   = gst_element_factory_make("rtph264pay", "pay");
    sink_udp  = gst_element_factory_make("udpsink", "udps");

    // Element properties
    // ...

    // Add to pipeline
    gst_bin_add_many(GST_BIN(pipeline),
                    src, demux, parse, decoder, conv, mux, pgie, tracker, osd, tee,
                    queue_display, sink_display,
                    queue_mp4, enc_mp4, parse_mp4, mux_mp4, sink_mp4,
                    queue_udp, pay_udp, sink_udp,
                    NULL
    );

    // Static linking
    gst_element_link(src, demux);
    gst_element_link_many(parse, decoder, conv, NULL);
    gst_element_link_many(mux, infer, tracker, osd, tee, NULL);

    // Display branches
    gst_element_link_many(tee, queue_display, sink_display, NULL);
    gst_element_link_many(tee, queue_mp4, enc_mp4, parse_mp4, mux_mp4, sink_mp4, NULL);
    gst_element_link_many(tee, queue_udp, pay_udp, sink_udp, NULL);

    // Dynamic linking
    g_signal_connect(demux, "pad-added",
        G_CALLBACK(+[](GstElement* d, GstPad* pad, gpointer data){
            GstElement *parse = (GstElement*)data;
            GstPad *sinkpad = gst_element_get_static_pad(parse, "sink");
            gst_pad_link(pad, sinkpad);
            gst_object_unref(sinkpad);
        }), parse);
    
    // Configure mux entry
    GstPad *sinkpad = gst_element_get_request_pad(mux, "sink_0");
    GstPad *srcpad  = gst_element_get_static_pad(conv, "src");
    gst_pad_link(srcpad, sinkpad);
    gst_object_unref(sinkpad);
    gst_object_unref(srcpad);

    // Install pad-probe
    //set_roi_params(cfg.roi);  // valores desde config (C)

    GstPad *probe_pad = gst_element_get_static_pad(tracker, "src");
    gst_pad_add_probe(probe_pad, GST_PAD_PROBE_TYPE_BUFFER,
                      roi_probe_callback, NULL, NULL);
    gst_object_unref(probe_pad);

    // Correr main loop
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, bus_cb, loop);
    gst_object_unref(bus);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    // Limpieza
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}