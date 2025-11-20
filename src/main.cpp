/*
 * main.cpp
 * Sistema de Vigilancia con ROI para Jetson Nano
 * Detecta vehículos/personas y monitorea tiempo en ROI
 * Universidad de Costa Rica - IE0301
 */

#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include "config/app_config.h"
#include "config/track_info.h"
#include "pipeline/pipeline.h"

static void cleanup(PipelineContext *ctx, TrackerContext *tracker, 
                   AppConfig *config, GTimer *app_timer) {
    tracker_destroy(tracker);
    
    if (app_timer) g_timer_destroy(app_timer);
    
    if (ctx->pipeline) {
        gst_element_set_state(ctx->pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(ctx->pipeline));
    }
    
    g_free(config->input_file);
    g_free(config->output_file);
    g_free(config->report_file);
    g_free(config->mode);
    g_free(config->udp_host);
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    
    AppConfig config;
    ROIParams roi;
    TrackerContext tracker;
    PipelineContext pipeline_ctx;
    GTimer *app_timer = g_timer_new();
    
    if (!parse_arguments(argc, argv, &config, &roi)) {
        if (app_timer) g_timer_destroy(app_timer);
        return -1;
    }
    
    g_print("=== Sistema de Vigilancia ROI ===\n");
    g_print("Input: %s\n", config.input_file);
    g_print("ROI: [%.2f, %.2f, %.2f, %.2f]\n", roi.x, roi.y, roi.w, roi.h);
    g_print("Max time: %d s\n", config.max_time_seconds);
    
    // Inicializar tracker
    tracker_init(&tracker, &roi, config.max_time_seconds, app_timer);
    
    // Inicializar contexto del pipeline
    pipeline_ctx.loop = g_main_loop_new(NULL, FALSE);
    pipeline_ctx.tracker = &tracker;
    pipeline_ctx.config = &config;
    pipeline_ctx.pipeline = NULL;
    
    if (!pipeline_create(&pipeline_ctx)) {
        g_printerr("Failed to create pipeline\n");
        cleanup(&pipeline_ctx, &tracker, &config, app_timer);
        g_main_loop_unref(pipeline_ctx.loop);
        return -1;
    }
    
    g_print("Starting pipeline...\n");
    gst_element_set_state(pipeline_ctx.pipeline, GST_STATE_PLAYING);
    g_main_loop_run(pipeline_ctx.loop);
    
    g_print("Cleaning up...\n");
    cleanup(&pipeline_ctx, &tracker, &config, app_timer);
    g_main_loop_unref(pipeline_ctx.loop);
    
    return 0;
}