/*
 * config.cpp
 * Implementación del parser de argumentos
 */

#include "app_config.hpp"
#include <string.h>

gboolean parse_arguments(int argc, char *argv[], AppConfig *config, ROIParams *roi) {
    // Valores por defecto
    config->roi_width = 0.4f;
    config->roi_height = 0.4f;
    config->roi_left = -1.0f;    // Marca para indicar "no especificado"
    config->roi_top = -1.0f;     // Marca para indicar "no especificado"
    config->max_time_seconds = 5;
    config->report_file = g_strdup("report.txt");
    config->output_file = g_strdup("output.mp4");
    config->mode = g_strdup("video");
    config->udp_port = 5000;
    config->udp_host = g_strdup("127.0.0.1");
    config->input_file = NULL;
    
    gboolean center_roi = FALSE;
    gboolean left_specified = FALSE;
    gboolean top_specified = FALSE;
    
    for (int i = 1; i < argc; i++) {
        if (g_strcmp0(argv[i], "--left") == 0 && i + 1 < argc) {
            config->roi_left = g_strtod(argv[++i], NULL);
            left_specified = TRUE;
        } else if (g_strcmp0(argv[i], "--top") == 0 && i + 1 < argc) {
            config->roi_top = g_strtod(argv[++i], NULL);
            top_specified = TRUE;
        } else if (g_strcmp0(argv[i], "--width") == 0 && i + 1 < argc) {
            config->roi_width = g_strtod(argv[++i], NULL);
        } else if (g_strcmp0(argv[i], "--height") == 0 && i + 1 < argc) {
            config->roi_height = g_strtod(argv[++i], NULL);
        } else if (g_strcmp0(argv[i], "--center") == 0) {
            center_roi = TRUE;
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
        g_printerr("Opciones:\n");
        g_printerr("  --width <0-1>   : Ancho del ROI normalizado (default: 0.4)\n");
        g_printerr("  --height <0-1>  : Alto del ROI normalizado (default: 0.4)\n");
        g_printerr("  --left <0-1>    : Posición X del ROI\n");
        g_printerr("  --top <0-1>     : Posición Y del ROI\n");
        g_printerr("  --center        : Centrar el ROI automáticamente\n");
        g_printerr("  --time <seg>    : Tiempo máximo en ROI (default: 5)\n");
        g_printerr("  --file-name <archivo> : Nombre del archivo de reporte\n");
        g_printerr("  vo-file <salida>: Archivo de video de salida\n");
        return FALSE;
    }
    
    // Aplicar centrado si: --center O no se especificaron left/top
    if (center_roi || (!left_specified && !top_specified)) {
        config->roi_left = (1.0f - config->roi_width) / 2.0f;
        config->roi_top = (1.0f - config->roi_height) / 2.0f;
        g_print("[OK] ROI centrado en (%.3f, %.3f) con tamano (%.3f, %.3f)\n", 
                config->roi_left, config->roi_top, config->roi_width, config->roi_height);
    } else {
        // Usar valores especificados o verificar que sean válidos
        if (config->roi_left < 0.0f) {
            config->roi_left = (1.0f - config->roi_width) / 2.0f;
        }
        if (config->roi_top < 0.0f) {
            config->roi_top = (1.0f - config->roi_height) / 2.0f;
        }
        g_print("[OK] ROI en posicion (%.3f, %.3f) con tamano (%.3f, %.3f)\n",
                config->roi_left, config->roi_top, config->roi_width, config->roi_height);
    }
    
    // Asignar valores finales al ROI
    roi->x = config->roi_left;
    roi->y = config->roi_top;
    roi->w = config->roi_width;
    roi->h = config->roi_height;
    
    // Validación de límites
    if (roi->x < 0.0f) roi->x = 0.0f;
    if (roi->y < 0.0f) roi->y = 0.0f;
    if (roi->x + roi->w > 1.0f) roi->x = 1.0f - roi->w;
    if (roi->y + roi->h > 1.0f) roi->y = 1.0f - roi->h;
    
    g_print("[OK] ROI final asignado: x=%.3f, y=%.3f, w=%.3f, h=%.3f\n",
            roi->x, roi->y, roi->w, roi->h);
    
    return TRUE;
}
