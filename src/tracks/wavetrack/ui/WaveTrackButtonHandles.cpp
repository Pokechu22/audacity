/**********************************************************************

Audacity: A Digital Audio Editor

WaveTrackButtonHandles.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "WaveTrackButtonHandles.h"

#include "../../../HitTestResult.h"
#include "../../../Project.h"
#include "../../../RefreshCode.h"
#include "../../../Track.h"
#include "../../../TrackPanel.h"
#include "../../../TrackPanelMouseEvent.h"

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
   if (mpTrack && (mpTrack->GetKind() == Track::Wave))
      pProject->DoTrackMute(mpTrack, event.ShiftDown());

   return RefreshCode::RefreshNone;
}

HitTestResult MuteButtonHandle::HitTest
(const wxMouseEvent &event, const wxRect &rect, const AudacityProject *pProject)
{
   wxRect buttonRect;
   TrackInfo::GetMuteSoloRect(rect, buttonRect, false,
      !pProject->IsSoloNone());

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
   if (mpTrack && (mpTrack->GetKind() == Track::Wave))
      pProject->DoTrackSolo(mpTrack, event.ShiftDown());

   return RefreshCode::RefreshNone;
}

HitTestResult SoloButtonHandle::HitTest
(const wxMouseEvent &event, const wxRect &rect, const AudacityProject *pProject)
{
   wxRect buttonRect;
   TrackInfo::GetMuteSoloRect(rect, buttonRect, true,
      !pProject->IsSoloNone());

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
