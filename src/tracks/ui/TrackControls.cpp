/**********************************************************************

Audacity: A Digital Audio Editor

TrackControls.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../Audacity.h"
#include "TrackControls.h"
#include "TrackButtonHandles.h"
#include "../../HitTestResult.h"
#include "../../TrackPanel.h"
#include "../../TrackPanelMouseEvent.h"

int TrackControls::gCaptureState;

TrackControls::~TrackControls()
{
}

HitTestResult TrackControls::HitTest1
(const TrackPanelMouseEvent &evt,
 const AudacityProject *)
{
   const wxMouseEvent &event = evt.event;
   const wxRect &rect = evt.rect;
   HitTestResult result;

   if (NULL != (result = CloseButtonHandle::HitTest(event, rect)).handle)
      return result;

   // VJ: Check sync-lock icon and the blank area to the left of the minimize
   // button.
   // Have to do it here, because if track is shrunk such that these areas
   // occlude controls,
   // e.g., mute/solo, don't want positive hit tests on the buttons.
   // Only result of doing so is to select the track. Don't care whether isleft.
   const bool bTrackSelClick =
      TrackInfo::TrackSelFunc(GetTrack(), rect, event.m_x, event.m_y);

   if (!bTrackSelClick) {
      if (NULL != (result = MinimizeButtonHandle::HitTest(event, rect)).handle)
         return result;
   }

   return result;
}

HitTestResult TrackControls::HitTest2
(const TrackPanelMouseEvent &,
 const AudacityProject *)
{
   return HitTestResult();
}

HitTestResult TrackControls::HitTest
(const TrackPanelMouseEvent &evt,
 const AudacityProject *pProject)
{
   HitTestResult result = HitTest1(evt, pProject);
   if (!result.handle)
      result = HitTest2(evt, pProject);
   return result;
}

Track *TrackControls::FindTrack()
{
   return GetTrack();
}
