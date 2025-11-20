#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include <gst/gst.h>
#include <unordered_map>
#include "config/app_config.hpp"
#include "config/track_info.hpp"
#include "roi/roi.hpp"

class Pipeline {
public:
    // Constructor
    Pipeline(const AppConfig& config, ROI& roi);
    
    // Destructor
    ~Pipeline();

    // Public methods
    bool init();
    void run();

private:
    // Pipeline state
    GstElement *pipeline;
    GMainLoop *loop;

    // Logical state
    AppConfig config;
    ROI roi;
    std::unordered_map<guint64, TrackInfo> tracked_objects;
    
    // Timing
    GTimer *app_timer;

    // Metrics
    guint total_detected;
    guint total_alerts;
    gint source_width;
    gint source_height;
    gboolean roi_has_objects;
    gboolean roi_has_alerts;

private:
    // Private methods
    bool create_pipeline();
    void cleanup();
    void handle_bus_message(GstMessage *msg);
    GstPadProbeReturn handle_pad_probe(GstPadProbeInfo *info);
    
    // Static callbacks (for Gstreamer)
    static GstPadProbeReturn pad_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer udata);
    static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data);
    static void on_pad_added_callback(GstElement *element, GstPad *pad, gpointer data);
};

#endif // PIPELINE_HPP