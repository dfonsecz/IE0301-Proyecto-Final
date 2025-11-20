#include "track_info.h"

TrackInfo::TrackInfo()
    : track_id(0),
      state(STATE_OUTSIDE),
      timer(nullptr),
      total_time(0.0),
      entry_timestamp(0.0),
      class_name(nullptr),
      alert_triggered(FALSE)
{
}