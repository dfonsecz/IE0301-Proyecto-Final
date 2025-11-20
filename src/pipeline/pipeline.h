/*
 * pipeline.h
 * Construcción y gestión del pipeline GStreamer
 */

#ifndef PIPELINE_H
#define PIPELINE_H

#include <gst/gst.h>
#include <glib.h>
#include "config/app_config.h"
#include "config/track_info.h"

// Contexto del pipeline
struct PipelineContext {
    GstElement *pipeline;
    GMainLoop *loop;
    TrackerContext *tracker;
    AppConfig *config;
};

// Crea el pipeline completo
gboolean pipeline_create(PipelineContext *ctx);

// Callback para pad dinámico del demuxer
void on_pad_added(GstElement *element, GstPad *pad, gpointer data);

// Bus callback para mensajes
gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data);

// Pad probe para procesamiento de metadatos
GstPadProbeReturn osd_sink_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, 
                                            gpointer u_data);

#endif // PIPELINE_H