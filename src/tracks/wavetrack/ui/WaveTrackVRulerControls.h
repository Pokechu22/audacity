/**********************************************************************

Audacity: A Digital Audio Editor

WaveTrackVRulerControls.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_WAVE_TRACK_VRULER_CONTROLS__
#define __AUDACITY_WAVE_TRACK_VRULER_CONTROLS__

#include "../../ui/TrackVRulerControls.h"

class WaveTrackVRulerControls : public TrackVRulerControls
{
   WaveTrackVRulerControls();
   WaveTrackVRulerControls(const WaveTrackVRulerControls&);
   WaveTrackVRulerControls &operator=(const WaveTrackVRulerControls&);

public:
   static WaveTrackVRulerControls &Instance();
   ~WaveTrackVRulerControls();

   virtual HitTestResult HitTest
      (const TrackPanelMouseEvent &event,
       const AudacityProject *);

   virtual unsigned HandleWheelRotation
      (const TrackPanelMouseEvent &event,
       AudacityProject *pProject);
};

#endif
