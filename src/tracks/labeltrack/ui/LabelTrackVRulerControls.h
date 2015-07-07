/**********************************************************************

Audacity: A Digital Audio Editor

LabelTrackVRulerControls.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_LABEL_TRACK_VRULER_CONTROLS__
#define __AUDACITY_LABEL_TRACK_VRULER_CONTROLS__

#include "../../ui/TrackVRulerControls.h"

class LabelTrackVRulerControls : public TrackVRulerControls
{
   LabelTrackVRulerControls();
   LabelTrackVRulerControls(const LabelTrackVRulerControls&);
   LabelTrackVRulerControls &operator=(const LabelTrackVRulerControls&);

public:
   static LabelTrackVRulerControls &Instance();
   ~LabelTrackVRulerControls();
};

#endif
