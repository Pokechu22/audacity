/**********************************************************************

Audacity: A Digital Audio Editor

NoteTrackUI.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../NoteTrack.h"
#include "NoteTrackControls.h"
#include "NoteTrackVRulerControls.h"

#include "../../../HitTestResult.h"
#include "../../../Project.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../ui/SelectHandle.h"
#include "StretchHandle.h"
#include "../../../toolbars/ToolsToolBar.h"

HitTestResult NoteTrack::HitTest
(const TrackPanelMouseEvent &event,
 const AudacityProject *pProject)
{
   const ToolsToolBar *const pTtb = pProject->GetToolsToolBar();

   // Eligible for stretch?
   HitTestResult result1;
#ifdef USE_MIDI
   result1 = StretchHandle::HitTest(event, pProject, this);
#endif

   // But some other non-select tool like zoom may take priority.
   HitTestResult result = Track::HitTest(event, pProject);
   if (result.preview.cursor &&
       !(result1.preview.cursor && pTtb->GetCurrentTool() == selectTool))
      return result;

   if (pTtb->IsDown(multiTool)) {
      // Side-effect on the toolbar
      //Use the false argument since in multimode we don't
      //want the button indicating which tool is in use to be updated.
      ((ToolsToolBar*)pTtb)->SetCurrentTool(selectTool, false);

      // Default to selection
      if (!result1.preview.cursor &&
          NULL != (result =
         SelectHandle::HitTest(event, pProject, this)).preview.cursor) {
         return result;
      }
   }

   // Do stretch!
   if (result1.preview.cursor)
      return result1;

   return result;
}

TrackControls *NoteTrack::GetControls()
{
   return &NoteTrackControls::Instance();
}

TrackVRulerControls *NoteTrack::GetVRulerControls()
{
   return &NoteTrackVRulerControls::Instance();
}
