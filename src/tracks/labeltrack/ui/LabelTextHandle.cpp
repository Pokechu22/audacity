/**********************************************************************

Audacity: A Digital Audio Editor

LabelTextHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "LabelTextHandle.h"
#include "LabelDefaultClickHandle.h"
#include "../../../HitTestResult.h"
#include "../../../LabelTrack.h"
#include "../../../Project.h"
#include "../../../RefreshCode.h"
#include "../../../TrackPanel.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../../ViewInfo.h"

LabelTextHandle::LabelTextHandle()
   : mpLT(NULL)
   , mRect()
   , mLabelTrackStartXPos(-1)
   , mLabelTrackStartYPos(-1)
{
}

LabelTextHandle &LabelTextHandle::Instance()
{
   static LabelTextHandle instance;
   return instance;
}

HitTestResult LabelTextHandle::HitTest(const wxMouseEvent &event, LabelTrack *pLT)
{
   // If Control is down, let the select handle be hit instead
   if (!event.ControlDown() &&
       pLT->OverATextBox(event.m_x, event.m_y) >= 0) {
   // There was no cursor change or status message for mousing over a label text box
      return HitTestResult(
         HitTestPreview(),
         &Instance()
      );
   }

   return HitTestResult();
}

LabelTextHandle::~LabelTextHandle()
{
}

UIHandle::Result LabelTextHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   TrackPanelCell *const pCell = evt.pCell;
   const wxMouseEvent &event = evt.event;
   ViewInfo &viewInfo = pProject->GetViewInfo();

   // Do common click effect
   LabelDefaultClickHandle::DoClick(event, pProject, pCell);

   mpLT = static_cast<LabelTrack*>(pCell);
   mpLT->HandleClick(event, evt.rect, viewInfo, &viewInfo.selectedRegion);
   wxASSERT(mpLT->IsSelected());

   {
      TrackList *const tracks = pProject->GetTracks();

      // IF the user clicked a label, THEN select all other tracks by Label

      TrackListIterator iter(tracks);
      Track *t = iter.First();

      //do nothing if at least one other track is selected
      bool done = false;
      while (!done && t) {
         if (t->GetSelected() && t != mpLT)
            done = true;
         t = iter.Next();
      }

      auto trackPanel = pProject->GetTrackPanel();
      if (!done) {
         //otherwise, select all tracks
         t = iter.First();
         while (t)
         {
            trackPanel->SelectTrack(t, true);
            t = iter.Next();
         }
      }

      // Do this after, for its effect on TrackPanel's memory of last selected
      // track (which affects shift-click actions)
      trackPanel->SelectTrack(mpLT, true);
   }

   return RefreshCode::RefreshCell | RefreshCode::UpdateSelection;
}

UIHandle::Result LabelTextHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;
   Result result = RefreshNone;

   const wxMouseEvent &event = evt.event;
   if(mpLT)
      mpLT->HandleTextDragRelease(event);

   // locate the initial mouse position
   if (event.LeftIsDown()) {
      if (mLabelTrackStartXPos == -1) {
         mLabelTrackStartXPos = event.m_x;
         mLabelTrackStartYPos = event.m_y;

         if (mpLT &&
            (mpLT->getSelectedIndex() != -1) &&
            mpLT->OverTextBox(
            mpLT->GetLabel(mpLT->getSelectedIndex()),
            mLabelTrackStartXPos,
            mLabelTrackStartYPos))
         {
            mLabelTrackStartYPos = -1;
         }
      }
      // if initial mouse position in the text box
      // then only drag text
      if (mLabelTrackStartYPos == -1)
         result |= RefreshCell;
   }

   return result;
}

HitTestPreview LabelTextHandle::Preview
(const TrackPanelMouseEvent &evt, const AudacityProject *pProject)
{
   return HitTestPreview();
}

UIHandle::Result LabelTextHandle::Release
(const TrackPanelMouseEvent &evt, AudacityProject *pProject,
 wxWindow *pParent)
{
   const wxMouseEvent &event = evt.event;
   if (mpLT)
      mpLT->HandleTextDragRelease(event);

   // handle mouse left button up
   if (event.LeftUp()) {
      mLabelTrackStartXPos = -1;
   }
   return RefreshCode::RefreshNone;
}

UIHandle::Result LabelTextHandle::Cancel(AudacityProject *)
{
   return RefreshCode::RefreshAll;
}

void LabelTextHandle::OnProjectChange(AudacityProject *pProject)
{
   if (! pProject->GetTracks()->Contains(mpLT)) {
      mpLT = nullptr;
      mRect = {};
   }

   UIHandle::OnProjectChange(pProject);
}
