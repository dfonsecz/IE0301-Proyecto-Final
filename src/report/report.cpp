/*
 * report.cpp
 * Implementación de generación de reportes
 */

#include "report.h"
#include <fstream>
#include <iomanip>

void generate_report(const TrackerContext *ctx, const gchar *report_file) {
    std::ofstream report(report_file);
    if (!report.is_open()) {
        g_printerr("Error: No se pudo crear el reporte\n");
        return;
    }
    
    gint roi_left = (gint)(ctx->roi.x * ctx->source_width);
    gint roi_top = (gint)(ctx->roi.y * ctx->source_height);
    gint roi_width = (gint)(ctx->roi.w * ctx->source_width);
    gint roi_height = (gint)(ctx->roi.h * ctx->source_height);
    
    report << "ROI: left: " << roi_left << " top: " << roi_top 
           << " width: " << roi_width << " height: " << roi_height << "\n";
    report << "Max time: " << ctx->max_time_seconds << "s\n";
    report << "Detected: " << ctx->total_detected << " (" 
           << ctx->total_alerts << ")\n";
    
    for (const auto &pair : ctx->tracked_objects) {
        const TrackInfo &info = pair.second;
        gdouble time_in_roi = (info.state != STATE_OUTSIDE) ? 
                              g_timer_elapsed(info.timer, NULL) : info.total_time;
        
        if (time_in_roi > 0.1) {
            gint minutes = (gint)(info.entry_timestamp / 60);
            gint seconds = (gint)(info.entry_timestamp) % 60;
            report << minutes << ":" << std::setfill('0') << std::setw(2) << seconds 
                   << " " << (info.class_name ? info.class_name : "object")
                   << " time " << (gint)time_in_roi << "s";
            if (info.alert_triggered) report << " alert";
            report << "\n";
        }
    }
    
    report.close();
    g_print("Reporte generado: %s\n", report_file);
}