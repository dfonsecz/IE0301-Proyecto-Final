#ifndef APPCONFIG_H
#define APPCONFIG_H

class AppConfig {
public:
    gchar *input_file;
    gchar *output_file;
    gchar *report_file;
    gfloat roi_left;
    gfloat roi_top;
    gfloat roi_width;
    gfloat roi_height;
    gint max_time_seconds;
    gchar *mode;
    gint udp_port;
    gchar *udp_host;

    AppConfig();   // solo declaración
};

#endif