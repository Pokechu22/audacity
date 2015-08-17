/**********************************************************************

Audacity: A Digital Audio Editor

LabelDefaultClickHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "LabelDefaultClickHandle.h"
#include "../../../HitTestResult.h"
#include "../../../LabelTrack.h"
#include "../../../Project.h"
#include "../../../RefreshCode.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../../ViewInfo.h"

LabelDefaultClickHandle::LabelDefaultClickHandle()
   : mpForward(NULL)
{
}

LabelDefaultClickHandle &LabelDefaultClickHandle::Instance()
{
   static LabelDefaultClickHandle instance;
   return instance;
}

LabelDefaultClickHandle::~LabelDefaultClickHandle()
{
}

void LabelDefaultClickHandle::DoClick
(const wxMouseEvent &event, AudacityProject *pProject, TrackPanelCell *pCell)
{
   LabelTrack *pLT = static_cast<LabelTrack*>(pCell);

   if (event.LeftDown())
   {
      pLT->SetSelectedIndex(pLT->OverATextBox(event.m_x, event.m_y));

      TrackList *const tracks = pProject->GetTracks();
      TrackListIterator iter(tracks);
      Track *n = iter.First();

      while (n) {
         if (n->GetKind() == Track::Label && pCell != n) {
            LabelTrack *const lt = static_cast<LabelTrack*>(n);
            lt->ResetFlags();
            lt->Unselect();
         }
         n = iter.Next();
      }
   }
}

UIHandle::Result LabelDefaultClickHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;

   DoClick(evt.event, pProject, evt.pCell);

   if (mpForward)
      return mpForward->Click(evt, pProject);
   else {
      // No drag or release follows
      // But redraw to show the change of text box selection status
      return Cancelled | RefreshAll;
   }
}

UIHandle::Result LabelDefaultClickHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   if (mpForward)
      mpForward->Drag(evt, pProject);
   return RefreshCode::RefreshAll;
}

HitTestPreview LabelDefaultClickHandle::Preview
(const TrackPanelMouseEvent &evt, const AudacityProject *pProject)
{
   if (mpForward)
      mpForward->Preview(evt, pProject);
   return HitTestPreview();
}

UIHandle::Result LabelDefaultClickHandle::Release
(const TrackPanelMouseEvent &evt, AudacityProject *pProject,
 wxWindow *pParent)
{
   if (mpForward)
      return mpForward->Release(evt, pProject, pParent);
   return RefreshCode::RefreshNone;
}

UIHandle::Result LabelDefaultClickHandle::Cancel(AudacityProject *pProject)
{
   if (mpForward)
      return mpForward->Cancel(pProject);
   return RefreshCode::RefreshNone;
}

void LabelDefaultClickHandle::DrawExtras
(DrawingPass pass,
 wxDC * dc, const wxRegion &updateRegion, const wxRect &panelRect)
{
   if (mpForward)
      mpForward->DrawExtras(pass, dc, updateRegion, panelRect);
}

bool LabelDefaultClickHandle::StopsOnKeystroke()
{
   if (mpForward)
      return mpForward->StopsOnKeystroke();
   else
      return UIHandle::StopsOnKeystroke();
}

void LabelDefaultClickHandle::OnProjectChange(AudacityProject *pProject)
{
   if (mpForward)
      mpForward->OnProjectChange(pProject);
   else
      UIHandle::OnProjectChange(pProject);
}
