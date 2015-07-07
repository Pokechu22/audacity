/**********************************************************************

Audacity: A Digital Audio Editor

LabelTrackControls.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_LABEL_TRACK_CONTROLS__
#define __AUDACITY_LABEL_TRACK_CONTROLS__

#include "../../ui/TrackControls.h"

class LabelTrackControls : public TrackControls
{
   LabelTrackControls();
   LabelTrackControls(const LabelTrackControls&);
   LabelTrackControls &operator=(const LabelTrackControls&);

public:
   static LabelTrackControls &Instance();
   ~LabelTrackControls();

   virtual HitTestResult HitTest
      (const TrackPanelMouseEvent &event,
       const AudacityProject *pProject);
};

#endif
