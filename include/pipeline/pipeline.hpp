#ifdef PIPELINE_HPP
#define PIPELINE_HPP

#include <unordered_map>
#include <gst/gst.h>
#include "config/app_config.hpp"
#include "config/track_info.hpp"
#include "roi/roi.hpp"

class Pipeline {
private:
    GstElement *pipeline;
    GMainLoop *loop;
    AppConfig config;
    ROIParams roi;
    std::unordered_map<guint64, TrackInfo> tracked_objects;
    GTimer *app_timer;
    guint total_detected;
    guint total_alerts;
    gint source_width;
    gint source_height;
    gboolean roi_has_objects;
    gboolean roi_has_alerts;
};

#endif // PIPELINE_HPP