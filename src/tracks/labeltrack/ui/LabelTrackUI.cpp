/**********************************************************************

Audacity: A Digital Audio Editor

LabelTrackUI.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../LabelTrack.h"
#include "LabelTrackControls.h"
#include "LabelDefaultClickHandle.h"
#include "LabelTrackVRulerControls.h"
#include "LabelGlyphHandle.h"
#include "LabelTextHandle.h"

#include "../../ui/SelectHandle.h"

#include "../../../HitTestResult.h"
#include "../../../Project.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../../toolbars/ToolsToolBar.h"

HitTestResult LabelTrack::HitTest
(const TrackPanelMouseEvent &evt,
 const AudacityProject *pProject)
{
   HitTestResult result;

   const wxMouseEvent &event = evt.event;

   result = LabelGlyphHandle::HitTest(event, this);
   if (result.preview.cursor)
      return result;

   // Don't lose the refresh result side effect of the glyph
   // hit test.
   unsigned refreshResult = result.preview.refreshCode;

   result = LabelTextHandle::HitTest(event, this);
   // This hit test does not define its own messages or cursor
   UIHandle *const pHandle = result.handle;

   result = Track::HitTest(evt, pProject);
   if (pHandle)
      // Use any cursor or status message change from catchall,
      // But let the text ui handle pass
      result.handle = pHandle;
   else {

      if (!result.handle) {
         // In case of multi tool, default to selection.
         const ToolsToolBar *const pTtb = pProject->GetToolsToolBar();
         if (pTtb->IsDown(multiTool)) {
            if (NULL != (result =
               SelectHandle::HitTest(evt, pProject, this)).preview.cursor) {
               // Side-effect on the toolbar
               //Use the false argument since in multimode we don't
               //want the button indicating which tool is in use to be updated.
               ((ToolsToolBar*)pTtb)->SetCurrentTool(selectTool, false);
            }
         }
      }

      // Attach some extra work to the click action
      LabelDefaultClickHandle::Instance().mpForward = result.handle;
      result.handle = &LabelDefaultClickHandle::Instance();
   }

   result.preview.refreshCode |= refreshResult;
   return result;
}

TrackControls *LabelTrack::GetControls()
{
   return &LabelTrackControls::Instance();
}

TrackVRulerControls *LabelTrack::GetVRulerControls()
{
   return &LabelTrackVRulerControls::Instance();
}
