#include <iostream>
#include <gst/gst.h>
#include "pipeline/pipeline.hpp"

// Constructor
Pipeline::Pipeline(const AppConfig& config, ROI& r) {
    config = config;
    roi = roi;
    app_timer = g_timer_new();
}

// Destructor
Pipeline::~Pipeline() {
    cleanup();
}

bool Pipeline::init() {
    loop = g_main_loop_new(nullptr, FALSE);
    return create_pipeline();
}

void Pipeline::run() {
    if (!pipeline || !loop) {
        std::cerr << "Pipeline was not initialized."
        return;
    } else {
        std::cout << "Pipeline executing..."
    }
}