/*
 * config.cpp
 * Implementación del parser de argumentos
 */

#include "config.h"
#include <string.h>

gboolean parse_arguments(int argc, char *argv[], AppConfig *config, ROIParams *roi) {
    // Valores por defecto
    config->roi_left = 0.3f;
    config->roi_top = 0.3f;
    config->roi_width = 0.4f;
    config->roi_height = 0.4f;
    config->max_time_seconds = 5;
    config->report_file = g_strdup("report.txt");
    config->output_file = g_strdup("output.mp4");
    config->mode = g_strdup("video");
    config->udp_port = 5000;
    config->udp_host = g_strdup("127.0.0.1");
    config->input_file = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (g_strcmp0(argv[i], "--left") == 0 && i + 1 < argc) {
            config->roi_left = g_strtod(argv[++i], NULL);
        } else if (g_strcmp0(argv[i], "--top") == 0 && i + 1 < argc) {
            config->roi_top = g_strtod(argv[++i], NULL);
        } else if (g_strcmp0(argv[i], "--width") == 0 && i + 1 < argc) {
            config->roi_width = g_strtod(argv[++i], NULL);
        } else if (g_strcmp0(argv[i], "--height") == 0 && i + 1 < argc) {
            config->roi_height = g_strtod(argv[++i], NULL);
        } else if (g_strcmp0(argv[i], "--time") == 0 && i + 1 < argc) {
            config->max_time_seconds = atoi(argv[++i]);
        } else if (g_strcmp0(argv[i], "--file-name") == 0 && i + 1 < argc) {
            g_free(config->report_file);
            config->report_file = g_strdup(argv[++i]);
        } else if (g_strcmp0(argv[i], "vi-file") == 0 && i + 1 < argc) {
            config->input_file = g_strdup(argv[++i]);
        } else if (g_strcmp0(argv[i], "vo-file") == 0 && i + 1 < argc) {
            g_free(config->output_file);
            config->output_file = g_strdup(argv[++i]);
        } else if (g_strcmp0(argv[i], "--mode") == 0 && i + 1 < argc) {
            g_free(config->mode);
            config->mode = g_strdup(argv[++i]);
        }
    }
    
    if (!config->input_file) {
        g_printerr("Error: Debe especificar vi-file <input>\n");
        g_printerr("Uso: %s vi-file <input.mp4> [opciones]\n", argv[0]);
        return FALSE;
    }
    
    roi->x = config->roi_left;
    roi->y = config->roi_top;
    roi->w = config->roi_width;
    roi->h = config->roi_height;
    
    return TRUE;
}