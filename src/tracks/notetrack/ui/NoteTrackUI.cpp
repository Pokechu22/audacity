/**********************************************************************

Audacity: A Digital Audio Editor

NoteTrackUI.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../NoteTrack.h"
#include "NoteTrackControls.h"
#include "NoteTrackVRulerControls.h"

#include "../../../HitTestResult.h"

HitTestResult NoteTrack::HitTest
(const TrackPanelMouseEvent &event,
 const AudacityProject *pProject)
{
   return Track::HitTest(event, pProject);
}

TrackControls *NoteTrack::GetControls()
{
   return &NoteTrackControls::Instance();
}

TrackVRulerControls *NoteTrack::GetVRulerControls()
{
   return &NoteTrackVRulerControls::Instance();
}
