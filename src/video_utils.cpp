/*
 * video_utils.cpp
 * Implementación de utilidades de video
 * Universidad de Costa Rica - IE0301
 */

#include "video_utils.h"
#include <gst/pbutils/pbutils.h>

gboolean get_video_info(const gchar *filepath, VideoInfo *info) {
    GError *error = NULL;
    GstDiscoverer *discoverer;
    GstDiscovererInfo *disc_info;
    gchar *uri;
    
    // Inicializar estructura
    info->width = 0;
    info->height = 0;
    info->fps_num = 0;
    info->fps_den = 0;
    info->duration = 0.0;
    info->valid = FALSE;
    
    // Crear discoverer
    discoverer = gst_discoverer_new(5 * GST_SECOND, &error);
    if (!discoverer) {
        g_printerr("Error creating discoverer: %s\n", error->message);
        g_error_free(error);
        return FALSE;
    }
    
    // Convertir filepath a URI
    if (!g_str_has_prefix(filepath, "file://")) {
        gchar *absolute_path = g_filename_to_uri(filepath, NULL, NULL);
        if (!absolute_path) {
            // Intentar con ruta relativa/absoluta directa
            if (g_path_is_absolute(filepath)) {
                uri = g_strdup_printf("file://%s", filepath);
            } else {
                gchar *cwd = g_get_current_dir();
                uri = g_strdup_printf("file://%s/%s", cwd, filepath);
                g_free(cwd);
            }
        } else {
            uri = absolute_path;
        }
    } else {
        uri = g_strdup(filepath);
    }
    
    g_print("Discovering video info: %s\n", uri);
    
    // Descubrir información
    disc_info = gst_discoverer_discover_uri(discoverer, uri, &error);
    if (!disc_info) {
        g_printerr("Error discovering URI: %s\n", error->message);
        g_error_free(error);
        g_free(uri);
        g_object_unref(discoverer);
        return FALSE;
    }
    
    // Verificar resultado
    GstDiscovererResult result = gst_discoverer_info_get_result(disc_info);
    if (result != GST_DISCOVERER_OK) {
        const gchar *result_str = "Unknown error";
        switch (result) {
            case GST_DISCOVERER_URI_INVALID: result_str = "Invalid URI"; break;
            case GST_DISCOVERER_ERROR: result_str = "Discovery error"; break;
            case GST_DISCOVERER_TIMEOUT: result_str = "Discovery timeout"; break;
            case GST_DISCOVERER_BUSY: result_str = "Discoverer busy"; break;
            case GST_DISCOVERER_MISSING_PLUGINS: result_str = "Missing plugins"; break;
            default: break;
        }
        g_printerr("Discovery failed: %s (code: %d)\n", result_str, result);
        gst_discoverer_info_unref(disc_info);
        g_free(uri);
        g_object_unref(discoverer);
        return FALSE;
    }
    
    // Obtener duración
    info->duration = gst_discoverer_info_get_duration(disc_info) / (gdouble)GST_SECOND;
    
    // Obtener streams de video
    GList *video_streams = gst_discoverer_info_get_video_streams(disc_info);
    if (video_streams) {
        GstDiscovererStreamInfo *stream_info = (GstDiscovererStreamInfo *)video_streams->data;
        
        if (GST_IS_DISCOVERER_VIDEO_INFO(stream_info)) {
            GstDiscovererVideoInfo *video_info = GST_DISCOVERER_VIDEO_INFO(stream_info);
            
            info->width = gst_discoverer_video_info_get_width(video_info);
            info->height = gst_discoverer_video_info_get_height(video_info);
            info->fps_num = gst_discoverer_video_info_get_framerate_num(video_info);
            info->fps_den = gst_discoverer_video_info_get_framerate_denom(video_info);
            info->valid = TRUE;
            
            g_print("[OK] Video resolution: %dx%d @ %d/%d fps (%.2fs)\n",
                    info->width, info->height, 
                    info->fps_num, info->fps_den,
                    info->duration);
        }
        
        gst_discoverer_stream_info_list_free(video_streams);
    } else {
        g_printerr("No video streams found\n");
    }
    
    // Limpiar
    gst_discoverer_info_unref(disc_info);
    g_free(uri);
    g_object_unref(discoverer);
    
    return info->valid;
}
