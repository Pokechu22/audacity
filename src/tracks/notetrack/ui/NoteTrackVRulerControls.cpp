/**********************************************************************

Audacity: A Digital Audio Editor

NoteTrackVRulerControls.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../Audacity.h"
#include "NoteTrackVRulerControls.h"

#include "../../../HitTestResult.h"
#include "../../../NoteTrack.h"
#include "../../../Project.h"
#include "../../../RefreshCode.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../../UIHandle.h"
#include "../../../../images/Cursors.h"

namespace
{

   bool IsDragZooming(int zoomStart, int zoomEnd)
   {
      const int DragThreshold = 3;// Anything over 3 pixels is a drag, else a click.
      return (abs(zoomEnd - zoomStart) > DragThreshold);
   }

}

///////////////////////////////////////////////////////////////////////////////

class NoteTrackVZoomHandle : public UIHandle
{
   NoteTrackVZoomHandle();
   NoteTrackVZoomHandle(const NoteTrackVZoomHandle&);
   NoteTrackVZoomHandle &operator=(const NoteTrackVZoomHandle&);
   static NoteTrackVZoomHandle& Instance();
   static HitTestPreview HitPreview(const wxMouseEvent &event);

public:
   static HitTestResult HitTest(const wxMouseEvent &event);

   virtual ~NoteTrackVZoomHandle();

   virtual Result Click
      (const TrackPanelMouseEvent &event, AudacityProject *pProject);

   virtual Result Drag
      (const TrackPanelMouseEvent &event, AudacityProject *pProject);

   virtual HitTestPreview Preview
      (const TrackPanelMouseEvent &event, const AudacityProject *pProject);

   virtual Result Release
      (const TrackPanelMouseEvent &event, AudacityProject *pProject,
       wxWindow *pParent);

   virtual Result Cancel(AudacityProject *pProject);

   virtual void DrawExtras
      (DrawingPass pass,
      wxDC * dc, const wxRegion &updateRegion, const wxRect &panelRect);

   void OnProjectChange(AudacityProject *pProject) override;

private:
   NoteTrack *mpTrack;

   int mZoomStart, mZoomEnd;
   wxRect mRect;
};

NoteTrackVZoomHandle::NoteTrackVZoomHandle()
   : mpTrack(NULL), mZoomStart(0), mZoomEnd(0), mRect()
{
}

NoteTrackVZoomHandle &NoteTrackVZoomHandle::Instance()
{
   static NoteTrackVZoomHandle instance;
   return instance;
}

HitTestPreview NoteTrackVZoomHandle::HitPreview(const wxMouseEvent &event)
{
   static std::unique_ptr<wxCursor> zoomInCursor(
      ::MakeCursor(wxCURSOR_MAGNIFIER, ZoomInCursorXpm, 19, 15)
   );
   static std::unique_ptr<wxCursor> zoomOutCursor(
      ::MakeCursor(wxCURSOR_MAGNIFIER, ZoomOutCursorXpm, 19, 15)
   );
   return HitTestPreview(
      _("Click to verticaly zoom in, Shift-click to zoom out, Drag to create a particular zoom region."),
      (event.ShiftDown() ? &*zoomOutCursor : &*zoomInCursor)
   );
}

HitTestResult NoteTrackVZoomHandle::HitTest(const wxMouseEvent &event)
{
   return HitTestResult(HitPreview(event), &Instance());
}

NoteTrackVZoomHandle::~NoteTrackVZoomHandle()
{
}

UIHandle::Result NoteTrackVZoomHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   mpTrack = static_cast<NoteTrack*>(
      static_cast<NoteTrackVRulerControls*>(evt.pCell)->GetTrack()
   );
   mRect = evt.rect;

   const wxMouseEvent &event = evt.event;
   mZoomStart = event.m_y;
   mZoomEnd = event.m_y;

   // change note track to zoom like audio track
   //          mpTrack->StartVScroll();

   return RefreshCode::RefreshNone;
}

UIHandle::Result NoteTrackVZoomHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   const wxMouseEvent &event = evt.event;
   mZoomEnd = event.m_y;
   using namespace RefreshCode;
   if (IsDragZooming(mZoomStart, mZoomEnd)) {
      // changed Note track to work like audio track
      //         mpTrack->VScroll(mZoomStart, mZoomEnd);
      return RefreshAll;
   }
   return RefreshNone;
}

HitTestPreview NoteTrackVZoomHandle::Preview
(const TrackPanelMouseEvent &evt, const AudacityProject *pProject)
{
   return HitPreview(evt.event);
}

UIHandle::Result NoteTrackVZoomHandle::Release
(const TrackPanelMouseEvent &evt, AudacityProject *pProject,
 wxWindow *pParent)
{
   using namespace RefreshCode;
   if (!mpTrack)
      return RefreshNone;

   const wxMouseEvent &event = evt.event;
   if (IsDragZooming(mZoomStart, mZoomEnd)) {
      mpTrack->ZoomTo(mZoomStart, mZoomEnd);
   }
   else if (event.ShiftDown() || event.RightUp()) {
      mpTrack->ZoomOut(mZoomEnd);
   }
   else {
      mpTrack->ZoomIn(mZoomEnd);
   }

   // TODO:  shift-right click as in audio track?

   mZoomEnd = mZoomStart = 0;
   pProject->ModifyState(true);

   return RefreshAll;
}

UIHandle::Result NoteTrackVZoomHandle::Cancel(AudacityProject *pProject)
{
   // Cancel is implemented!  And there is no initial state to restore,
   // so just return a code.
   return RefreshCode::RefreshAll;
}

void NoteTrackVZoomHandle::DrawExtras
(DrawingPass pass, wxDC * dc, const wxRegion &, const wxRect &panelRect)
{
   if (pass != UIHandle::Cells)
      return;

   if (!IsDragZooming(mZoomStart, mZoomEnd))
      return;

   wxRect rect;

   dc->SetBrush(*wxTRANSPARENT_BRUSH);
   dc->SetPen(*wxBLACK_DASHED_PEN);

   // We don't have access to this function.  It makes some small adjustment
   // to the total width that we can get from panelRect.
   // int width;
   // GetTracksUsableArea(&width, &height);

   rect.y = mZoomStart;
   rect.x = mRect.x;
   //   rect.width = width + mRect.width;
   rect.width = panelRect.width - (mRect.x - panelRect.x);
   rect.height = mZoomEnd - mZoomStart;

   dc->DrawRectangle(rect);
}

void NoteTrackVZoomHandle::OnProjectChange(AudacityProject *pProject)
{
   if (! pProject->GetTracks()->Contains(mpTrack)) {
      mpTrack = nullptr;
      mRect = {};
   }

   UIHandle::OnProjectChange(pProject);
}

///////////////////////////////////////////////////////////////////////////////
NoteTrackVRulerControls::NoteTrackVRulerControls()
   : TrackVRulerControls()
{
}

NoteTrackVRulerControls &NoteTrackVRulerControls::Instance()
{
   static NoteTrackVRulerControls instance;
   return instance;
}

NoteTrackVRulerControls::~NoteTrackVRulerControls()
{
}

HitTestResult NoteTrackVRulerControls::HitTest
(const TrackPanelMouseEvent &evt,
 const AudacityProject *)
{
#ifdef USE_MIDI
   return NoteTrackVZoomHandle::HitTest(evt.event);
#else
   return HitTestResult();
#endif
}
