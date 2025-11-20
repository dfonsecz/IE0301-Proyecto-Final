# include "render.hpp"

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
    // Configura el rectángulo del ROI
    NvOSD_RectParams &r = disp_meta->rect_params[0];
    r.left = (gint)(roi->x * fmeta->source_frame_width);
    r.top = (gint)(roi->y * fmeta->source_frame_height);
    r.width = (gint)(roi->w * fmeta->source_frame_width);
    r.height = (gint)(roi->h * fmeta->source_frame_height);
    r.border_width = 4;
    
    // Color del rectángulo según estado
    if (any_alert) {
        r.has_bg_color = 1;
        set_color(r.bg_color, 1.0f, 0.0f, 0.0f, 0.3f);
        set_color(r.border_color, 1.0f, 0.0f, 0.0f);
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