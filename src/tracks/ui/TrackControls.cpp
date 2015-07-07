/**********************************************************************

Audacity: A Digital Audio Editor

TrackControls.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../Audacity.h"
#include "TrackControls.h"
#include "../../HitTestResult.h"

TrackControls::~TrackControls()
{
}

HitTestResult TrackControls::HitTest1
(const TrackPanelMouseEvent &,
 const AudacityProject *)
{
   return HitTestResult();
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
