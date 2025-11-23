/*
 * tracker.hpp
 * Sistema de tracking de objetos y ROI
 */

#ifndef TRACKER_H
#define TRACKER_H

#include <gst/gst.h>
#include <glib.h>
#include <unordered_map>
#include "app_config.hpp"
#include "gstnvdsmeta.h"

// Contexto del tracker
struct TrackerContext {
    std::unordered_map<guint64, TrackInfo> tracked_objects;
    ROIParams roi;
    gint max_time_seconds;
    GTimer *app_timer;
    guint total_detected;
    guint total_alerts;
    gint source_width;
    gint source_height;
    gboolean roi_has_objects;
    gboolean roi_has_alerts;
};

// Inicializa el contexto del tracker
void tracker_init(TrackerContext *ctx, const ROIParams *roi, gint max_time, GTimer *app_timer);

// Verifica si un bbox está dentro del ROI
gboolean is_bbox_in_roi(NvOSD_RectParams *bbox, const ROIParams *roi, 
                        gint frame_width, gint frame_height);

// Procesa un objeto detectado
void tracker_process_object(TrackerContext *ctx, NvDsObjectMeta *obj_meta,
                           gint frame_width, gint frame_height);

// Limpia objetos inactivos
void tracker_cleanup_inactive(TrackerContext *ctx, 
                             const std::unordered_map<guint64, bool> &active_tracks);

// Libera recursos del tracker
void tracker_destroy(TrackerContext *ctx);

#endif // TRACKER_H
