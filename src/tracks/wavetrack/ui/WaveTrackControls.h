/**********************************************************************

Audacity: A Digital Audio Editor

WavelTrackControls.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_WAVE_TRACK_CONTROLS__
#define __AUDACITY_WAVE_TRACK_CONTROLS__

#include "../../ui/TrackControls.h"

class WaveTrackControls : public TrackControls
{
   WaveTrackControls();
   WaveTrackControls(const WaveTrackControls&);
   WaveTrackControls &operator=(const WaveTrackControls&);

public:
   static WaveTrackControls &Instance();
   ~WaveTrackControls();

   virtual HitTestResult HitTest
      (const TrackPanelMouseEvent &event,
       const AudacityProject *pProject);
};

#endif
