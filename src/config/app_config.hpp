/*
 * config.hpp
 * Configuración y estructuras de datos del sistema
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <gst/gst.h>
#include <glib.h>

// Parámetros de la aplicación
struct AppConfig {
    gchar *input_file;
    gchar *output_file;
    gchar *report_file;
    gfloat roi_left;
    gfloat roi_top;
    gfloat roi_width;
    gfloat roi_height;
    gint max_time_seconds;
    gchar *mode;
    gint udp_port;
    gchar *udp_host;
};

// ROI normalizado (0-1)
struct ROIParams {
    float x, y, w, h;
};

// Estados del objeto
enum ObjectState {
    STATE_OUTSIDE,
    STATE_INSIDE,
    STATE_ALERT
};

// Información de seguimiento por objeto
struct TrackInfo {
    guint64 track_id;
    ObjectState state;
    GTimer *timer;
    gdouble total_time;
    gdouble entry_timestamp;
    gchar *class_name;
    gboolean alert_triggered;
};

// Parse argumentos de línea de comandos
gboolean parse_arguments(int argc, char *argv[], AppConfig *config, ROIParams *roi);

#endif // CONFIG_H
