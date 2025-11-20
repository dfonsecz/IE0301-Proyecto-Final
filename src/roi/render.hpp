/*
 * render.h
 * Renderizado de ROI y overlay en pantalla
 */

#ifndef RENDER_H
#define RENDER_H

#include <gst/gst.h>
#include <glib.h>
#include "config.h"
#include "gstnvdsmeta.h"

// Utilidad para asignar colores
void set_color(NvOSD_ColorParams &c, float r, float g, float b, float a = 1.0f);

// Dibuja el rectángulo del ROI
void draw_roi_rect(NvDsBatchMeta *batch_meta, NvDsFrameMeta *fmeta,
                   const ROIParams *roi, gboolean any_inside, gboolean any_alert);

#endif // RENDER_H