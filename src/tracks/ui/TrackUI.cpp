/**********************************************************************

Audacity: A Digital Audio Editor

TrackUI.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../Track.h"
#include "../../TrackPanelMouseEvent.h"
#include "TrackControls.h"
#include "TrackVRulerControls.h"

#include "../../HitTestResult.h"
#include "../../Project.h"
#include "../../toolbars/ToolsToolBar.h"

#include "EnvelopeHandle.h"
#include "../wavetrack/ui/SampleHandle.h"
#include "ZoomHandle.h"
#include "TimeShiftHandle.h"

HitTestResult Track::HitTest
(const TrackPanelMouseEvent &event,
 const AudacityProject *pProject)
{
   const ToolsToolBar * pTtb = pProject->GetToolsToolBar();
   // Unless in Multimode keep using the current tool.
   const bool isMultiTool = pTtb->IsDown(multiTool);
   if (!isMultiTool) {
      switch (pTtb->GetCurrentTool()) {
      case envelopeTool:
         // Pass "false" for unsafe -- let the tool decide to cancel itself
         return EnvelopeHandle::HitAnywhere(pProject);
      case drawTool:
         return SampleHandle::HitAnywhere(event.event, pProject);
      case zoomTool:
         return ZoomHandle::HitAnywhere(event.event, pProject);
      case slideTool:
         return TimeShiftHandle::HitAnywhere(pProject);

      case selectTool:
      default:
         // cases not yet implemented
         // fallthru
         ;
      }
   }

   // Replicate some of the logic of TrackPanel::DetermineToolToUse
   HitTestResult result;

   if (isMultiTool) {
      int currentTool = -1;
      if (NULL != (result = ZoomHandle::HitTest(event.event, pProject)).preview.cursor)
         currentTool = zoomTool;

      // To do: special hit tests

      if (currentTool >= 0) {
         // Side-effect on the toolbar
         //Use the false argument since in multimode we don't
         //want the button indicating which tool is in use to be updated.
         const_cast<ToolsToolBar*>(pTtb)->SetCurrentTool(currentTool, false);
         return result;
      }
   }

   return result;
}

TrackPanelCell *Track::GetTrackControl()
{
   TrackControls *const result = GetControls();
   result->mpTrack = this;
   return result;
}

TrackPanelCell *Track::GetVRulerControl()
{
   TrackVRulerControls *const result = GetVRulerControls();
   result->mpTrack = this;
   return result;
}
