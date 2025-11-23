/*
 * tracker.cpp
 * Implementación del sistema de tracking
 */

#include "track_info.hpp"
#include <string.h>

void tracker_init(TrackerContext *ctx, const ROIParams *roi, gint max_time, GTimer *app_timer) {
    ctx->roi = *roi;
    ctx->max_time_seconds = max_time;
    ctx->app_timer = app_timer;
    ctx->total_detected = 0;
    ctx->total_alerts = 0;
    ctx->source_width = 0;
    ctx->source_height = 0;
    ctx->roi_has_objects = FALSE;
    ctx->roi_has_alerts = FALSE;
    ctx->tracked_objects.clear();
    
    g_print(" Tracker inicializado con ROI: x=%.3f, y=%.3f, w=%.3f, h=%.3f\n",
            ctx->roi.x, ctx->roi.y, ctx->roi.w, ctx->roi.h);
}

gboolean is_bbox_in_roi(NvOSD_RectParams *bbox, const ROIParams *roi, 
                        gint frame_width, gint frame_height) {
    float cx = (bbox->left + bbox->width / 2.0f) / frame_width;
    float cy = (bbox->top + bbox->height / 2.0f) / frame_height;
    return (cx >= roi->x && cx <= (roi->x + roi->w) && 
            cy >= roi->y && cy <= (roi->y + roi->h));
}

void tracker_process_object(TrackerContext *ctx, NvDsObjectMeta *obj_meta,
                           gint frame_width, gint frame_height) {
    if (!obj_meta) return;
    if (obj_meta->class_id != 0 && obj_meta->class_id != 2) return;
    
    guint64 track_id = obj_meta->object_id;
    gboolean inside_roi = is_bbox_in_roi(&obj_meta->rect_params, &ctx->roi,
                                         frame_width, frame_height);
    
    TrackInfo *track_info;
    auto it = ctx->tracked_objects.find(track_id);
    
    if (it == ctx->tracked_objects.end()) {
        TrackInfo new_track;
        new_track.track_id = track_id;
        new_track.state = STATE_OUTSIDE;
        new_track.timer = g_timer_new();
        new_track.total_time = 0.0;
        new_track.entry_timestamp = 0.0;
        new_track.class_name = g_strdup(obj_meta->obj_label);
        new_track.alert_triggered = FALSE;
        ctx->tracked_objects[track_id] = new_track;
        track_info = &ctx->tracked_objects[track_id];
        ctx->total_detected++;
    } else {
        track_info = &it->second;
    }
    
    if (inside_roi) {
        ctx->roi_has_objects = TRUE;
        
        if (track_info->state == STATE_OUTSIDE) {
            track_info->state = STATE_INSIDE;
            g_timer_start(track_info->timer);
            track_info->entry_timestamp = g_timer_elapsed(ctx->app_timer, NULL);
        } else if (track_info->state == STATE_INSIDE) {
            gdouble elapsed = g_timer_elapsed(track_info->timer, NULL);
            if (elapsed >= ctx->max_time_seconds) {
                track_info->state = STATE_ALERT;
                track_info->alert_triggered = TRUE;
                ctx->total_alerts++;
            }
        }
        
        if (track_info->state == STATE_ALERT) {
            ctx->roi_has_alerts = TRUE;
            // Rosa/Pink para alerta
            obj_meta->rect_params.border_color.red = 1.0f;
            obj_meta->rect_params.border_color.green = 0.41f;
            obj_meta->rect_params.border_color.blue = 0.71f;
            obj_meta->rect_params.border_color.alpha = 1.0f;
            obj_meta->rect_params.border_width = 4;
        } else {
            obj_meta->rect_params.border_color.red = 1.0f;
            obj_meta->rect_params.border_color.green = 0.65f;
            obj_meta->rect_params.border_color.blue = 0.0f;
            obj_meta->rect_params.border_color.alpha = 1.0f;
            obj_meta->rect_params.border_width = 3;
        }
    } else {
        if (track_info->state != STATE_OUTSIDE) {
            track_info->total_time = g_timer_elapsed(track_info->timer, NULL);
            g_timer_stop(track_info->timer);
            track_info->state = STATE_OUTSIDE;
        }
        obj_meta->rect_params.border_color.red = 0.0f;
        obj_meta->rect_params.border_color.green = 1.0f;
        obj_meta->rect_params.border_color.blue = 0.0f;
        obj_meta->rect_params.border_color.alpha = 1.0f;
        obj_meta->rect_params.border_width = 2;
    }
}

void tracker_destroy(TrackerContext *ctx) {
    for (auto &pair : ctx->tracked_objects) {
        if (pair.second.timer) g_timer_destroy(pair.second.timer);
        if (pair.second.class_name) g_free(pair.second.class_name);
    }
    ctx->tracked_objects.clear();
}
