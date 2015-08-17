/**********************************************************************

Audacity: A Digital Audio Editor

LabelGlyphHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "LabelGlyphHandle.h"
#include "LabelDefaultClickHandle.h"
#include "../../../HitTestResult.h"
#include "../../../LabelTrack.h"
#include "../../../Project.h"
#include "../../../RefreshCode.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../../UndoManager.h"
#include "../../../ViewInfo.h"

#include "../../../MemoryX.h"

#include <wx/cursor.h>
#include <wx/translation.h>

LabelGlyphHandle::LabelGlyphHandle()
   : mpLT(NULL)
   , mRect()
{
}

LabelGlyphHandle &LabelGlyphHandle::Instance()
{
   static LabelGlyphHandle instance;
   return instance;
}

HitTestPreview LabelGlyphHandle::HitPreview
   (bool hitCenter, unsigned refreshResult)
{
   static std::unique_ptr<wxCursor> arrowCursor(
      new wxCursor(wxCURSOR_ARROW)
   );
   return HitTestPreview(
      (hitCenter
         ? _("Drag one or more label boundaries.")
         : _("Drag label boundary.")),
      &*arrowCursor,
      refreshResult
   );
}

HitTestResult LabelGlyphHandle::HitTest
(const wxMouseEvent &event, LabelTrack *pLT)
{
   using namespace RefreshCode;
   unsigned refreshResult = RefreshNone;

   int edge = pLT->OverGlyph(event.m_x, event.m_y);

   //KLUDGE: We refresh the whole Label track when the icon hovered over
   //changes colouration.  Inefficient.
   edge += pLT->mbHitCenter ? 4 : 0;
   if (edge != pLT->mOldEdge)
   {
      pLT->mOldEdge = edge;
      refreshResult |= RefreshCell;
   }

   // IF edge!=0 THEN we've set the cursor and we're done.
   // signal this by setting the tip.
   if (edge != 0)
   {
      return HitTestResult(
         HitPreview(pLT->mbHitCenter, refreshResult),
         &Instance()
      );
   }
   else {
      HitTestPreview preview;
      preview.refreshCode = refreshResult;
      return HitTestResult(
         preview,
         NULL
      );
   }
}

LabelGlyphHandle::~LabelGlyphHandle()
{
}

UIHandle::Result LabelGlyphHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   TrackPanelCell *const pCell = evt.pCell;
   const wxMouseEvent &event = evt.event;
   const wxRect &rect = evt.rect;

   // Do common click effect
   LabelDefaultClickHandle::DoClick(event, pProject, pCell);

   mpLT = static_cast<LabelTrack*>(pCell);
   mRect = rect;

   ViewInfo &viewInfo = pProject->GetViewInfo();
   mpLT->HandleClick(event, rect, viewInfo, &viewInfo.selectedRegion);

   if (mpLT->IsAdjustingLabel())
   {
      // redraw the track.
      return RefreshCode::RefreshCell;
   }
   else
      // The positive hit test should have ensured otherwise
      wxASSERT(false);

   // handle shift+ctrl down
   /*if (event.ShiftDown()) { // && event.ControlDown()) {
      lTrack->SetHighlightedByKey(true);
      Refresh(false);
      return;
   }*/


   return RefreshCode::RefreshNone;
}

UIHandle::Result LabelGlyphHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   const wxMouseEvent &event = evt.event;
   ViewInfo &viewInfo = pProject->GetViewInfo();
   mpLT->HandleGlyphDragRelease(event, mRect, viewInfo, &viewInfo.selectedRegion);

   // Refresh all so that the change of selection is redrawn in all tracks
   return RefreshCode::RefreshAll | RefreshCode::DrawOverlays;
}

HitTestPreview LabelGlyphHandle::Preview
(const TrackPanelMouseEvent &evt, const AudacityProject *pProject)
{
   return HitPreview(mpLT->mbHitCenter, 0);
}

UIHandle::Result LabelGlyphHandle::Release
(const TrackPanelMouseEvent &evt, AudacityProject *pProject,
 wxWindow *pParent)
{
   const wxMouseEvent &event = evt.event;
   ViewInfo &viewInfo = pProject->GetViewInfo();
   if (mpLT->HandleGlyphDragRelease(event, mRect, viewInfo, &viewInfo.selectedRegion)) {
      pProject->PushState(_("Modified Label"),
         _("Label Edit"),
         UndoPush::CONSOLIDATE);
   }

   // Refresh all so that the change of selection is redrawn in all tracks
   return RefreshCode::RefreshAll | RefreshCode::DrawOverlays;
}

UIHandle::Result LabelGlyphHandle::Cancel(AudacityProject *)
{
   return RefreshCode::RefreshAll;
}
