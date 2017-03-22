/**********************************************************************

Audacity: A Digital Audio Editor

CutlineHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../Audacity.h"
#include "CutlineHandle.h"

#include "../../../MemoryX.h"

#include "../../../HitTestResult.h"
#include "../../../Project.h"
#include "../../../RefreshCode.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../../UndoManager.h"
#include "../../../WaveTrack.h"
#include "../../../WaveTrackLocation.h"
#include "../../../../images/Cursors.h"

CutlineHandle::CutlineHandle()
   : mOperation(Merge)
   , mStartTime(0.0)
   , mEndTime(0.0)
   , mbCutline(false)
{
}

CutlineHandle &CutlineHandle::Instance()
{
   static CutlineHandle instance;
   return instance;
}

HitTestPreview CutlineHandle::HitPreview(bool cutline, bool unsafe)
{
   static std::unique_ptr<wxCursor> disabledCursor(
      ::MakeCursor(wxCURSOR_NO_ENTRY, DisabledCursorXpm, 16, 16)
   );
   static std::unique_ptr<wxCursor> arrowCursor(
      new wxCursor(wxCURSOR_ARROW)
   );
   return HitTestPreview(
      cutline
      ? _("Left-Click to expand, Right-Click to remove")
      : _("Left-Click to join clips"),
      (unsafe
      ? &*disabledCursor
      : &*arrowCursor)
   );
}

HitTestResult CutlineHandle::HitAnywhere(const AudacityProject *pProject, bool cutline)
{
   const bool unsafe = pProject->IsAudioActive();
   return HitTestResult(
      HitPreview(cutline, unsafe),
      (unsafe
      ? NULL
      : &Instance())
   );
}

namespace
{
   int FindMergeLine(WaveTrack *track, double time)
   {
      const double tolerance = 0.5 / track->GetRate();
      int ii = 0;
      for (const auto loc: track->GetCachedLocations()) {
         if (loc.typ == WaveTrackLocation::locationMergePoint &&
             fabs(time - loc.pos) < tolerance)
            return ii;
         ++ii;
      }
      return -1;
   }
   
   bool IsOverCutline
      (const ViewInfo &viewInfo, WaveTrack * track,
       const wxRect &rect, const wxMouseEvent &event,
       WaveTrackLocation *pCapturedTrackLocation)
   {
      for (auto loc: track->GetCachedLocations())
      {
         const double x = viewInfo.TimeToPosition(loc.pos);
         if (x >= 0 && x < rect.width)
         {
            wxRect locRect;
            locRect.x = (int)(rect.x + x) - 5;
            locRect.width = 11;
            locRect.y = rect.y;
            locRect.height = rect.height;
            if (locRect.Contains(event.m_x, event.m_y))
            {
               if (pCapturedTrackLocation)
                  *pCapturedTrackLocation = loc;
               return true;
            }
         }
      }

      return false;
   }
}

HitTestResult CutlineHandle::HitTest
(const wxMouseEvent &event, const wxRect &rect, const AudacityProject *pProject, Track *pTrack)
{
   const ViewInfo &viewInfo = pProject->GetViewInfo();
   /// method that tells us if the mouse event landed on an
   /// editable Cutline
   if (pTrack->GetKind() != Track::Wave)
      return HitTestResult();

   WaveTrack *wavetrack = static_cast<WaveTrack*>(pTrack);
   WaveTrackLocation location;
   if (!IsOverCutline(viewInfo, wavetrack, rect, event, &location))
      return HitTestResult();

   return HitAnywhere(pProject, location.typ == WaveTrackLocation::locationCutLine);
}

CutlineHandle::~CutlineHandle()
{
}

UIHandle::Result CutlineHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   const wxMouseEvent &event = evt.event;
   ViewInfo &viewInfo = pProject->GetViewInfo();
   Track *const pTrack = static_cast<Track*>(evt.pCell);

   // Can affect the track by merging clips, expanding a cutline, or
   // deleting a cutline.
   // All the change is done at button-down.  Button-up just commits the undo item.
   using namespace RefreshCode;

   /// Someone has just clicked the mouse.  What do we do?
   const bool unsafe = pProject->IsAudioActive();
   if (unsafe)
      return Cancelled;

   WaveTrackLocation capturedTrackLocation;

   WaveTrack *wavetrack = static_cast<WaveTrack*>(pTrack);
   if (!IsOverCutline(viewInfo, wavetrack, evt.rect, event, &capturedTrackLocation))
      return Cancelled;
   mbCutline = (capturedTrackLocation.typ == WaveTrackLocation::locationCutLine);

   // FIXME: Disable this and return true when CutLines aren't showing?
   // (Don't use gPrefs-> for the fix as registry access is slow).

   // Cutline data changed on either branch, so refresh the track display.
   UIHandle::Result result = RefreshCell;
   // Assume linked track is wave or null
   const auto linked = static_cast<WaveTrack*>(wavetrack->GetLink());

   if (event.LeftDown())
   {
      if (capturedTrackLocation.typ == WaveTrackLocation::locationCutLine)
      {
         mOperation = Expand;
         mStartTime = viewInfo.selectedRegion.t0();
         mEndTime = viewInfo.selectedRegion.t1();

         // When user presses left button on cut line, expand the line again
         double cutlineStart = 0, cutlineEnd = 0;

         if (!wavetrack->ExpandCutLine(capturedTrackLocation.pos, &cutlineStart, &cutlineEnd))
            return Cancelled;

         if (linked) {
            // Expand the cutline in the opposite channel if it is present.

            // PRL:  Do NOT report that the event is not handled if the other
            // channel doesn't also have a cutline to expand at the same time.
            // Just ignore the return.  Bug1310.

            /* bool success = */
            linked->ExpandCutLine(capturedTrackLocation.pos);
         }

         viewInfo.selectedRegion.setTimes(cutlineStart, cutlineEnd);
         result |= UpdateSelection;
      }
      else if (capturedTrackLocation.typ == WaveTrackLocation::locationMergePoint) {
         const double pos = capturedTrackLocation.pos;
         if (!wavetrack->MergeClips(capturedTrackLocation.clipidx1, capturedTrackLocation.clipidx2))
            return Cancelled;

         if (linked) {
            // Don't assume correspondence of merge points across channels!
            int idx = FindMergeLine(linked, pos);
            if (idx >= 0) {
               WaveTrack::Location location = linked->GetCachedLocations()[idx];
               if (!linked->MergeClips(location.clipidx1, location.clipidx2)) {
                  this->Cancel(pProject);
                  return Cancelled;
               }
            }
         }

         mOperation = Merge;
      }
   }
   else if (event.RightDown())
   {
      if (!wavetrack->RemoveCutLine(capturedTrackLocation.pos))
         return Cancelled;

      if (linked && !linked->RemoveCutLine(capturedTrackLocation.pos)) {
         this->Cancel(pProject);
         return Cancelled;
      }

      mOperation = Remove;
   }
   else
      result = RefreshNone;

   return result;
}

UIHandle::Result CutlineHandle::Drag
(const TrackPanelMouseEvent &, AudacityProject *)
{
   return RefreshCode::RefreshNone;
}

HitTestPreview CutlineHandle::Preview
(const TrackPanelMouseEvent &, const AudacityProject *)
{
   return HitPreview(mbCutline, false);
}

UIHandle::Result CutlineHandle::Release
(const TrackPanelMouseEvent &, AudacityProject *pProject, wxWindow *)
{
   UIHandle::Result result = RefreshCode::RefreshNone;

   // Only now commit the result to the undo stack
   AudacityProject *const project = pProject;
   switch (mOperation) {
   default:
      wxASSERT(false);
   case Merge:
      project->PushState(_("Merged Clips"), _("Merge"), UndoPush::CONSOLIDATE);
      break;
   case Expand:
      project->PushState(_("Expanded Cut Line"), _("Expand"));
      result |= RefreshCode::UpdateSelection;
      break;
   case Remove:
      project->PushState(_("Removed Cut Line"), _("Remove"));
      break;
   }

   // Nothing to do for the display
   return result;
}

UIHandle::Result CutlineHandle::Cancel(AudacityProject *pProject)
{
   using namespace RefreshCode;
   UIHandle::Result result = RefreshCell;
   if (mOperation == Expand) {
      AudacityProject *const project = pProject;
      project->SetSel0(mStartTime);
      project->SetSel1(mEndTime);
      result |= RefreshCode::UpdateSelection;
   }
   pProject->RollbackState();
   return result;
}
