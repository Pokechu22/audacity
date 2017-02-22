/**********************************************************************

Audacity: A Digital Audio Editor

MuteSoloButtonHandles.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "MuteSoloButtonHandles.h"

#include "../../Audacity.h"
#include "../../Experimental.h"

#include "../../HitTestResult.h"
#include "../../Project.h"
#include "../../RefreshCode.h"
#include "../../Track.h"
#include "../../TrackPanel.h"
#include "../../TrackPanelMouseEvent.h"

MuteButtonHandle::MuteButtonHandle()
   : ButtonHandle(TrackPanel::IsMuting)
{
}

MuteButtonHandle::~MuteButtonHandle()
{
}

MuteButtonHandle &MuteButtonHandle::Instance()
{
   static MuteButtonHandle instance;
   return instance;
}

UIHandle::Result MuteButtonHandle::CommitChanges
(const wxMouseEvent &event, AudacityProject *pProject, wxWindow *)
{
#ifdef EXPERIMENTAL_MIDI_OUT
   if (mpTrack && (mpTrack->GetKind() == Track::Wave || mpTrack->GetKind() == Track::Note))
#else
   if (mpTrack && (mpTrack->GetKind() == Track::Wave))
#endif
      pProject->DoTrackMute(mpTrack, event.ShiftDown());

   return RefreshCode::RefreshNone;
}

HitTestResult MuteButtonHandle::HitTest
(const wxMouseEvent &event, const wxRect &rect, const AudacityProject *pProject, int trackKind)
{
   wxRect buttonRect;
   TrackInfo::GetMuteSoloRect(rect, buttonRect, false, !pProject->IsSoloNone()
#ifdef EXPERIMENTAL_MIDI_OUT
      , trackKind);
#else
      );
#endif

   if (buttonRect.Contains(event.m_x, event.m_y)) {
      Instance().mRect = buttonRect;
      return HitTestResult(
         Preview(),
         &Instance()
         );
   }
   else
      return HitTestResult();
}

////////////////////////////////////////////////////////////////////////////////

SoloButtonHandle::SoloButtonHandle()
   : ButtonHandle(TrackPanel::IsSoloing)
{
}

SoloButtonHandle::~SoloButtonHandle()
{
}

SoloButtonHandle &SoloButtonHandle::Instance()
{
   static SoloButtonHandle instance;
   return instance;
}

UIHandle::Result SoloButtonHandle::CommitChanges
(const wxMouseEvent &event, AudacityProject *pProject, wxWindow *pParent)
{
#ifdef EXPERIMENTAL_MIDI_OUT
   if (mpTrack && (mpTrack->GetKind() == Track::Wave || mpTrack->GetKind() == Track::Note))
#else
   if (mpTrack && (mpTrack->GetKind() == Track::Wave))
#endif
      pProject->DoTrackSolo(mpTrack, event.ShiftDown());

   return RefreshCode::RefreshNone;
}

HitTestResult SoloButtonHandle::HitTest
(const wxMouseEvent &event, const wxRect &rect, const AudacityProject *pProject, int trackKind)
{
   wxRect buttonRect;
   TrackInfo::GetMuteSoloRect(rect, buttonRect, true, !pProject->IsSoloNone()
#ifdef EXPERIMENTAL_MIDI_OUT
      , trackKind);
#else
      );
#endif

   if (buttonRect.Contains(event.m_x, event.m_y)) {
      Instance().mRect = buttonRect;
      return HitTestResult(
         Preview(),
         &Instance()
         );
   }
   else
      return HitTestResult();
}
