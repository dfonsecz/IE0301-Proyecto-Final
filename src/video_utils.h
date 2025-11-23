/*
 * video_utils.h
 * Utilidades para obtener información de videos
 * Universidad de Costa Rica - IE0301
 */

#ifndef VIDEO_UTILS_H
#define VIDEO_UTILS_H

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

// Estructura para almacenar información del video
typedef struct {
    gint width;
    gint height;
    gint fps_num;
    gint fps_den;
    gdouble duration;
    gboolean valid;
} VideoInfo;

// Obtiene información del video usando GstDiscoverer
gboolean get_video_info(const gchar *filepath, VideoInfo *info);

#endif // VIDEO_UTILS_H
