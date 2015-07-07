/**********************************************************************

Audacity: A Digital Audio Editor

NoteTrackVRulerControls.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_NOTE_TRACK_VRULER_CONTROLS__
#define __AUDACITY_NOTE_TRACK_VRULER_CONTROLS__

#include "../../ui/TrackVRulerControls.h"

class NoteTrackVRulerControls : public TrackVRulerControls
{
   NoteTrackVRulerControls();
   NoteTrackVRulerControls(const NoteTrackVRulerControls&);
   NoteTrackVRulerControls &operator=(const NoteTrackVRulerControls&);

public:
   static NoteTrackVRulerControls &Instance();
   ~NoteTrackVRulerControls();

   virtual HitTestResult HitTest
      (const TrackPanelMouseEvent &event,
       const AudacityProject *pProject);
};

#endif
