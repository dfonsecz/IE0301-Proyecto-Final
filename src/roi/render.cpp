/*
 * render.cpp
 * Implementación del renderizado OSD
 */

#include "render.h"

void set_color(NvOSD_ColorParams &c, float r, float g, float b, float a) {
    c.red = r; 
    c.green = g; 
    c.blue = b; 
    c.alpha = a;
}

void draw_roi_rect(NvDsBatchMeta *batch_meta, NvDsFrameMeta *fmeta,
                   const ROIParams *roi, gboolean any_inside, gboolean any_alert) {
    NvDsDisplayMeta *disp_meta = nvds_acquire_display_meta_from_pool(batch_meta);
    disp_meta->num_rects = 1;
    
    NvOSD_RectParams &r = disp_meta->rect_params[0];
    
    // IMPORTANTE: Usar source_frame_width/height que es la resolución
    // después del escalado por streammux (lo que se ve en pantalla)
    gint roi_left_px = (gint)(roi->x * fmeta->source_frame_width);
    gint roi_top_px = (gint)(roi->y * fmeta->source_frame_height);
    gint roi_width_px = (gint)(roi->w * fmeta->source_frame_width);
    gint roi_height_px = (gint)(roi->h * fmeta->source_frame_height);
    
    // Debug: imprimir primera vez
    static gboolean first_time = TRUE;
    if (first_time) {
        g_print("Frame resolution: %dx%d\n", fmeta->source_frame_width, fmeta->source_frame_height);
        g_print("ROI normalized: x=%.3f, y=%.3f, w=%.3f, h=%.3f\n", 
                roi->x, roi->y, roi->w, roi->h);
        g_print("ROI pixels: left=%d, top=%d, width=%d, height=%d\n",
                roi_left_px, roi_top_px, roi_width_px, roi_height_px);
        first_time = FALSE;
    }
    
    r.left = roi_left_px;
    r.top = roi_top_px;
    r.width = roi_width_px;
    r.height = roi_height_px;
    r.border_width = 4;
    
    if (any_alert) {
        r.has_bg_color = 1;
        // Rosa/Pink: RGB(255, 105, 180) normalizado = (1.0, 0.41, 0.71)
        set_color(r.bg_color, 1.0f, 0.41f, 0.71f, 0.4f);
        set_color(r.border_color, 1.0f, 0.41f, 0.71f);
    } else if (any_inside) {
        r.has_bg_color = 1;
        set_color(r.bg_color, 1.0f, 0.65f, 0.0f, 0.3f);
        set_color(r.border_color, 1.0f, 0.65f, 0.0f);
    } else {
        r.has_bg_color = 0;
        set_color(r.border_color, 0.0f, 1.0f, 0.0f);
    }
    
    nvds_add_display_meta_to_frame(fmeta, disp_meta);
}
