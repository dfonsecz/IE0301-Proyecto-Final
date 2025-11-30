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
    
    g_print("Tracker initialized with ROI: x=%.3f, y=%.3f, w=%.3f, h=%.3f\n",
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
    
    // FILTRO: Solo procesar vehículos (class_id 0 = Car, 2 = Bicycle, 5 = Bus, 7 = Truck)
    // Las personas son class_id 1 (Person)
    gboolean is_vehicle = (obj_meta->class_id == 0 ||  // Car
                           obj_meta->class_id == 2 ||  // Bicycle  
                           obj_meta->class_id == 5 ||  // Bus
                           obj_meta->class_id == 7);   // Truck
    
    if (!is_vehicle) {
        // Si es persona u otro objeto, dibujar borde verde y salir
        obj_meta->rect_params.border_color.red = 0.0f;
        obj_meta->rect_params.border_color.green = 1.0f;
        obj_meta->rect_params.border_color.blue = 0.0f;
        obj_meta->rect_params.border_color.alpha = 1.0f;
        obj_meta->rect_params.border_width = 2;
        obj_meta->rect_params.has_bg_color = 0;  // Sin fondo
        return;
    }
    
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
        new_track.alert_start_time = 0.0;  // Inicializar nuevo campo
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
                track_info->alert_start_time = g_timer_elapsed(ctx->app_timer, NULL);
                ctx->total_alerts++;
            }
        }
        
        if (track_info->state == STATE_ALERT) {
            ctx->roi_has_alerts = TRUE;
            
            // Calcular tiempo desde que se activó la alerta
            gdouble time_since_alert = g_timer_elapsed(ctx->app_timer, NULL) - track_info->alert_start_time;
            
            // Debug: imprimir estado
            static gint debug_counter = 0;
            if (debug_counter++ % 30 == 0) {  // Cada ~30 frames
                g_print("ALERT: Vehicle ID %lu - Time in alert: %.1fs - Inside ROI: YES\n", 
                        track_id, time_since_alert);
            }
            
            // Efecto de parpadeo: alternar entre relleno y sin relleno cada 0.3 segundos
            // Durante los primeros 3 segundos (CAMBIA 3.0 POR EL TIEMPO QUE QUIERAS)
            gboolean show_fill = FALSE;
            if (time_since_alert < 3.0) {  // ← LÍNEA PARA MODIFICAR: Duración del parpadeo
                // Parpadeo rápido: on/off cada 0.3 segundos (CAMBIA 0.3 PARA VELOCIDAD)
                gint blink_cycle = (gint)(time_since_alert / 0.3);  // ← LÍNEA PARA MODIFICAR: Velocidad
                show_fill = (blink_cycle % 2 == 0);
            }
            
            // Rosa/Pink para alerta - PERMANECE HASTA QUE SALGA DEL ROI
            obj_meta->rect_params.border_color.red = 1.0f;
            obj_meta->rect_params.border_color.green = 0.41f;
            obj_meta->rect_params.border_color.blue = 0.71f;
            obj_meta->rect_params.border_color.alpha = 1.0f;
            obj_meta->rect_params.border_width = 4;
            
            if (show_fill) {
                // Rellenar el bounding box con rosa semi-transparente
                obj_meta->rect_params.has_bg_color = 1;
                obj_meta->rect_params.bg_color.red = 1.0f;
                obj_meta->rect_params.bg_color.green = 0.41f;
                obj_meta->rect_params.bg_color.blue = 0.71f;
                obj_meta->rect_params.bg_color.alpha = 0.4f;  // 40% transparente
            } else {
                // Solo borde rosa (sin relleno) después del parpadeo
                obj_meta->rect_params.has_bg_color = 0;
            }
        } else {
            // Naranja para vehículo dentro del ROI (aún no ha excedido el tiempo)
            obj_meta->rect_params.border_color.red = 1.0f;
            obj_meta->rect_params.border_color.green = 0.65f;
            obj_meta->rect_params.border_color.blue = 0.0f;
            obj_meta->rect_params.border_color.alpha = 1.0f;
            obj_meta->rect_params.border_width = 3;
            obj_meta->rect_params.has_bg_color = 0;  // Sin fondo
        }
    } else {
        // El vehículo SALIÓ del ROI
        if (track_info->state != STATE_OUTSIDE) {
            track_info->total_time = g_timer_elapsed(track_info->timer, NULL);
            g_timer_stop(track_info->timer);
            track_info->state = STATE_OUTSIDE;
            // Aquí es donde se resetea el estado, ya no está en alerta
        }
        // Verde para vehículo fuera del ROI (vuelve a verde después de salir)
        obj_meta->rect_params.border_color.red = 0.0f;
        obj_meta->rect_params.border_color.green = 1.0f;
        obj_meta->rect_params.border_color.blue = 0.0f;
        obj_meta->rect_params.border_color.alpha = 1.0f;
        obj_meta->rect_params.border_width = 2;
        obj_meta->rect_params.has_bg_color = 0;  // Sin fondo
    }
}

void tracker_destroy(TrackerContext *ctx) {
    for (auto &pair : ctx->tracked_objects) {
        if (pair.second.timer) g_timer_destroy(pair.second.timer);
        if (pair.second.class_name) g_free(pair.second.class_name);
    }
    ctx->tracked_objects.clear();
}
