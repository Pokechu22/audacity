/**********************************************************************

Audacity: A Digital Audio Editor

WaveTrackUI.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../WaveTrack.h"
#include "WaveTrackControls.h"
#include "WaveTrackVRulerControls.h"

#include "../../../HitTestResult.h"
#include "../../../Project.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../../toolbars/ToolsToolBar.h"

#include "CutlineHandle.h"
#include "../../ui/SelectHandle.h"
#include "../../ui/EnvelopeHandle.h"
#include "SampleHandle.h"
#include "../../ui/TimeShiftHandle.h"

HitTestResult WaveTrack::HitTest
(const TrackPanelMouseEvent &event,
 const AudacityProject *pProject)
{
   // FIXME: Should similar logic apply to NoteTrack (#if defined(USE_MIDI)) ?
   // From here on the order in which we hit test determines
   // which tool takes priority in the rare cases where it
   // could be more than one.

   // This hit was always tested first no matter which tool:
   HitTestResult result = CutlineHandle::HitTest(event.event, event.rect, pProject, this);
   if (result.preview.cursor)
      return result;

   result = Track::HitTest(event, pProject);
   if (result.preview.cursor)
      return result;

   const ToolsToolBar *const pTtb = pProject->GetToolsToolBar();
   if (pTtb->IsDown(multiTool)) {
      // Replicate some of the logic of TrackPanel::DetermineToolToUse
      int currentTool = -1;
      if (event.event.CmdDown()) {
         // msmeyer: If control is down, slide single clip
         // msmeyer: If control and shift are down, slide all clips
         currentTool = slideTool;
         result = TimeShiftHandle::HitAnywhere(pProject);
      }
      else if (NULL !=
         (result = EnvelopeHandle::WaveTrackHitTest(event.event, event.rect, pProject, this))
         .preview.cursor)
         currentTool = envelopeTool;
      else if (NULL != (result =
         TimeShiftHandle::HitTest(event.event, event.rect, pProject)).preview.cursor)
         currentTool = slideTool;
      else if (NULL != (result =
                        SampleHandle::HitTest(event.event, event.rect, pProject, this)).preview.cursor)
         currentTool = drawTool;
      else if (NULL != (result =
         SelectHandle::HitTest(event, pProject, this)).preview.cursor)
         // default of all other hit tests
         currentTool = selectTool;

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

TrackControls *WaveTrack::GetControls()
{
   return &WaveTrackControls::Instance();
}

TrackVRulerControls *WaveTrack::GetVRulerControls()
{
   return &WaveTrackVRulerControls::Instance();
}
