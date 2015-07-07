/**********************************************************************

Audacity: A Digital Audio Editor

NoteTrackControls.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_NOTE_TRACK_CONTROLS__
#define __AUDACITY_NOTE_TRACK_CONTROLS__

#include "../../ui/TrackControls.h"

class NoteTrackControls : public TrackControls
{
   NoteTrackControls();
   NoteTrackControls(const NoteTrackControls&);
   NoteTrackControls &operator=(const NoteTrackControls&);

public:
   static NoteTrackControls &Instance();
   ~NoteTrackControls();

   virtual HitTestResult HitTest
      (const TrackPanelMouseEvent &event,
       const AudacityProject *pProject);
};

#endif
