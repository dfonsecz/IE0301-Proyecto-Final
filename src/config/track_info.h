#ifndef TRACK_INFO_H
#define TRACK_INFO_H

#include <glib.h>

// Estado del objeto
enum ObjectState {
    STATE_OUTSIDE,
    STATE_INSIDE,
    STATE_ALERT
};

// Declaración de la clase TrackInfo
class TrackInfo {
public:
    guint64 track_id;
    ObjectState state;
    GTimer *timer;
    gdouble total_time;
    gdouble entry_timestamp;
    gchar *class_name;
    gboolean alert_triggered;

    TrackInfo();  // Solo se declara
};

#endif // TRACK_INFO_H