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

#include "../../../HitTestResult.h"
#include "../../../TrackPanelMouseEvent.h"

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
