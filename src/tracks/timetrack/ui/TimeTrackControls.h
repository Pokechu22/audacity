/**********************************************************************

Audacity: A Digital Audio Editor

TimeTrackControls.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_TIME_TRACK_CONTROLS__
#define __AUDACITY_TIME_TRACK_CONTROLS__

#include "../../ui/TrackControls.h"

class TimeTrackControls : public TrackControls
{
   TimeTrackControls();
   TimeTrackControls(const TimeTrackControls&);
   TimeTrackControls &operator=(const TimeTrackControls&);

public:
   static TimeTrackControls &Instance();
   ~TimeTrackControls();

   virtual HitTestResult HitTest
      (const TrackPanelMouseEvent &event,
       const AudacityProject *pProject);
};

#endif
