/*
 * report.h
 * Generación de reportes de análisis
 */

#ifndef REPORT_H
#define REPORT_H

#include <glib.h>
#include "config/track_info.h"

// Genera el reporte final con estadísticas
void generate_report(const TrackerContext *ctx, const gchar *report_file);

#endif // REPORT_H