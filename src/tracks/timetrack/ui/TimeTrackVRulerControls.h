/**********************************************************************

Audacity: A Digital Audio Editor

TimeTrackVRulerControls.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_TIME_TRACK_VRULER_CONTROLS__
#define __AUDACITY_TIME_TRACK_VRULER_CONTROLS__

#include "../../ui/TrackVRulerControls.h"

class TimeTrackVRulerControls : public TrackVRulerControls
{
   TimeTrackVRulerControls();
   TimeTrackVRulerControls(const TimeTrackVRulerControls&);
   TimeTrackVRulerControls &operator=(const TimeTrackVRulerControls&);

public:
   static TimeTrackVRulerControls &Instance();
   ~TimeTrackVRulerControls();
};

#endif
