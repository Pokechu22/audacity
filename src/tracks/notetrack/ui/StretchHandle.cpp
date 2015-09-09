/**********************************************************************

Audacity: A Digital Audio Editor

StretchHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../Audacity.h"
#include "StretchHandle.h"

#include "../../../HitTestResult.h"
#include "../../../NoteTrack.h"
#include "../../../Project.h"
#include "../../../RefreshCode.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../../UndoManager.h"
#include "../../../images/Cursors.h"

#include <algorithm>

StretchHandle::StretchHandle()
   : mpTrack(NULL)
   , mpBackup()
   , mSelStart(0.0)
   , mLeftEdge(-1)
   , mStretchMode(stretchCenter)
   , mStretched(false)
   , mStretchSel0(-1.0)
   , mStretchSel1(-1.0)
   , mStretchStart(0.0)
   , mStretchLeftBeats(-1.0)
   , mStretchRightBeats(-1.0)
   , mOrigSel0(-1.0), mOrigSel1(-1.0)
{
}

StretchHandle &StretchHandle::Instance()
{
   static StretchHandle instance;
   return instance;
}

HitTestPreview StretchHandle::HitPreview(StretchEnum stretchMode, bool unsafe)
{
   static std::unique_ptr<wxCursor> disabledCursor(
      ::MakeCursor(wxCURSOR_NO_ENTRY, DisabledCursorXpm, 16, 16)
   );
   static std::unique_ptr<wxCursor> stretchLeftCursor(
      ::MakeCursor(wxCURSOR_BULLSEYE, StretchLeftCursorXpm, 16, 16)
   );
   static std::unique_ptr<wxCursor> stretchRightCursor(
      ::MakeCursor(wxCURSOR_BULLSEYE, StretchRightCursorXpm, 16, 16)
   );
   static std::unique_ptr<wxCursor> stretchCursor(
      ::MakeCursor(wxCURSOR_BULLSEYE, StretchCursorXpm, 16, 16)
   );

   if (unsafe) {
      return HitTestPreview(_(""), &*disabledCursor);
   }
   else {
      wxCursor *pCursor = NULL;
      switch (stretchMode) {
      default:
         wxASSERT(false);
      case stretchLeft:
         pCursor = &*stretchLeftCursor; break;
      case stretchCenter:
         pCursor = &*stretchCursor; break;
      case stretchRight:
         pCursor = &*stretchRightCursor; break;
      }
      return HitTestPreview(
         _("Click and drag to stretch selected region."),
         pCursor
      );
   }
}

HitTestResult StretchHandle::HitTest
(const TrackPanelMouseEvent &evt, const AudacityProject *pProject, NoteTrack *pTrack,
 double &stretchSel0, double &stretchSel1, double &selStart, double &stretchStart,
 double &stretchLeftBeats, double &stretchRightBeats,
 StretchEnum &stretchMode)
{
   const wxMouseEvent &event = evt.event;

   // later, we may want a different policy, but for now, stretch is
   // selected when the cursor is near the center of the track and
   // within the selection

   const ViewInfo &viewInfo = pProject->GetViewInfo();
   const bool unsafe = pProject->IsAudioActive();
   if (!pTrack || !pTrack->GetSelected() || pTrack->GetKind() != Track::Note)
      return HitTestResult();

   const wxRect &rect = evt.rect;
   int center = rect.y + rect.height / 2;
   int distance = abs(event.m_y - center);
   const int yTolerance = 10;
   wxInt64 leftSel = viewInfo.TimeToPosition(viewInfo.selectedRegion.t0(), rect.x);
   wxInt64 rightSel = viewInfo.TimeToPosition(viewInfo.selectedRegion.t1(), rect.x);
   // Something is wrong if right edge comes before left edge
   wxASSERT(!(rightSel < leftSel));
   if (!(leftSel <= event.m_x && event.m_x <= rightSel &&
         distance < yTolerance))
      return HitTestResult();

   // find nearest beat to sel0, sel1
   static const double minPeriod = 0.05; // minimum beat period
   double qBeat0, qBeat1;
   double centerBeat = 0.0f;
   stretchSel0 = pTrack->NearestBeatTime(viewInfo.selectedRegion.t0(), &qBeat0);
   stretchSel1 = pTrack->NearestBeatTime(viewInfo.selectedRegion.t1(), &qBeat1);

   // If there is not (almost) a beat to stretch that is slower
   // than 20 beats per second, don't stretch
   if (within(qBeat0, qBeat1, 0.9) ||
      (stretchSel1 - stretchSel0) / (qBeat1 - qBeat0) < minPeriod)
      return HitTestResult();

   selStart = std::max(0.0, viewInfo.PositionToTime(event.m_x, rect.x));
   stretchStart = pTrack->NearestBeatTime(selStart, &centerBeat);
   bool startNewSelection = true;
   if (within(qBeat0, centerBeat, 0.1)) {
      selStart = viewInfo.selectedRegion.t1();
      startNewSelection = false;
   }
   else if (within(qBeat1, centerBeat, 0.1)) {
      selStart = viewInfo.selectedRegion.t0();
      startNewSelection = false;
   }

   if (startNewSelection) {
      stretchMode = stretchCenter;
      stretchLeftBeats = qBeat1 - centerBeat;
      stretchRightBeats = centerBeat - qBeat0;
   }
   else if (viewInfo.selectedRegion.t1() == selStart) {
      // note that at this point, mSelStart is at the opposite
      // end of the selection from the cursor. If the cursor is
      // over sel0, then mSelStart is at sel1.
      stretchMode = stretchLeft;
      stretchLeftBeats = 0;
      stretchRightBeats = qBeat1 - qBeat0;
   }
   else {
      stretchMode = stretchRight;
      stretchLeftBeats = qBeat1 - qBeat0;
      stretchRightBeats = 0;
   }

   return HitTestResult(
      HitPreview(stretchMode, unsafe),
      &Instance()
   );
}

void StretchHandle::Duplicate()
{
   // Cope with the peculiarities of note track duplication,
   // once to serialize, again to deserialize
   std::unique_ptr<Track> pTemp(mpTrack->Duplicate());
   mpBackup.reset( static_cast<NoteTrack*>(pTemp->Duplicate().release()) );
}

HitTestResult StretchHandle::HitTest
(const TrackPanelMouseEvent &evt, const AudacityProject *pProject, NoteTrack *pTrack)
{
   double stretchSel0, stretchSel1, selStart, stretchStart,
      stretchLeftBeats, stretchRightBeats;
   StretchEnum stretchMode;
   return HitTest
      (evt, pProject, pTrack,
       stretchSel0, stretchSel1, selStart, stretchStart,
       stretchLeftBeats, stretchRightBeats,
       stretchMode);
}

StretchHandle::~StretchHandle()
{
}

UIHandle::Result StretchHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;
   const bool unsafe = pProject->IsAudioActive();
   const wxMouseEvent &event = evt.event;

   if (unsafe ||
       event.LeftDClick() ||
       !event.LeftDown() ||
       evt.pCell == NULL)
      return Cancelled;


   mLeftEdge = evt.rect.GetLeft();
   mpTrack = static_cast<NoteTrack*>(evt.pCell);
   Duplicate();
   ViewInfo &viewInfo = pProject->GetViewInfo();
   mOrigSel0 = viewInfo.selectedRegion.t0();
   mOrigSel1 = viewInfo.selectedRegion.t1();

   // We must have hit if we got here, but repeat some
   // calculations that set members
   HitTest
      (evt, pProject, mpTrack,
       mStretchSel0, mStretchSel1, mSelStart, mStretchStart,
       mStretchLeftBeats, mStretchRightBeats,
       mStretchMode);

   viewInfo.selectedRegion.setTimes(mStretchSel0, mStretchSel1);
   mStretched = false;

   // Full refresh since the label area may need to indicate
   // newly selected tracks. (I'm really not sure if the label area
   // needs to be refreshed or how to just refresh non-label areas.-RBD)

   return RefreshAll | UpdateSelection;
}

UIHandle::Result StretchHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;
   const bool unsafe = pProject->IsAudioActive();
   if (unsafe) {
      this->Cancel(pProject);
      return RefreshAll | Cancelled;
   }

   const wxMouseEvent &event = evt.event;
   const int x = event.m_x;

   Track *clickedTrack =
      static_cast<CommonTrackPanelCell*>(evt.pCell)->FindTrack();

   if (clickedTrack == NULL && mpTrack != NULL)
      clickedTrack = mpTrack;
   Stretch(pProject, x, mLeftEdge, clickedTrack);
   return RefreshAll;
}

HitTestPreview StretchHandle::Preview
(const TrackPanelMouseEvent &, const AudacityProject *pProject)
{
   const bool unsafe = pProject->IsAudioActive();
   return HitPreview(mStretchMode, unsafe);
}

UIHandle::Result StretchHandle::Release
(const TrackPanelMouseEvent &, AudacityProject *pProject,
 wxWindow *)
{
   using namespace RefreshCode;
   mpBackup.reset(NULL);

   const bool unsafe = pProject->IsAudioActive();
   if (unsafe) {
      this->Cancel(pProject);
      return RefreshAll | Cancelled;
   }
   /* i18n-hint: (noun) The track that is used for MIDI notes which can be
   dragged to change their duration.*/
   pProject->PushState(_("Stretch Note Track"),
      /* i18n-hint: In the history list, indicates a MIDI note has
      been dragged to change its duration (stretch it). Using either past
      or present tense is fine here.  If unsure, go for whichever is
      shorter.*/
      _("Stretch"),
      UndoPush::CONSOLIDATE | UndoPush::AUTOSAVE);
   return RefreshAll;
}

UIHandle::Result StretchHandle::Cancel(AudacityProject *pProject)
{
   mpBackup.reset(NULL);
   if (mStretched)
      pProject->RollbackState();
   mStretched = false;
   pProject->GetViewInfo().selectedRegion.setTimes(mOrigSel0, mOrigSel1);
   return RefreshCode::RefreshNone;
}

void StretchHandle::Stretch(AudacityProject *pProject, int mouseXCoordinate, int trackLeftEdge,
   Track *pTrack)
{
   ViewInfo &viewInfo = pProject->GetViewInfo();
   TrackList *const trackList = pProject->GetTracks();

   if (mStretched) { // Undo stretch and redo it with NEW mouse coordinates
      // Drag handling was not originally implemented with Undo in mind --
      // there are saved pointers to tracks that are not supposed to change.
      // Undo will change tracks, so convert to index
      // values, then look them up after the Undo
      TrackListIterator iter(trackList);
      int pTrackIndex = pTrack->GetIndex();
      int capturedTrackIndex =
         (mpTrack ? mpTrack->GetIndex() : 0);

      {
         std::unique_ptr<NoteTrack> pTemp(std::move(mpBackup));
         pTemp->SwapSequence(*mpTrack);
         Duplicate();
      }

      // Undo brings us back to the pre-click state, but we want to
      // quantize selected region to integer beat boundaries. These
      // were saved in mStretchSel[12] variables:
      viewInfo.selectedRegion.setTimes(mStretchSel0, mStretchSel1);

      mStretched = false;
      int index = 0;
      for (Track *t = iter.First(trackList); t; t = iter.Next()) {
         if (index == pTrackIndex)
            pTrack = t;
         if (mpTrack && index == capturedTrackIndex) {
            if (mpTrack->GetKind() == Track::Note)
               mpTrack = static_cast<NoteTrack*>(t);
            else
               mpTrack = NULL;
         }
         index++;
      }
   }

   if (pTrack == NULL && mpTrack != NULL)
      pTrack = mpTrack;

   if (!pTrack || pTrack->GetKind() != Track::Note) {
      return;
   }

   NoteTrack *pNt = static_cast<NoteTrack *>(pTrack);
   double moveto =
      std::max(0.0, viewInfo.PositionToTime(mouseXCoordinate, trackLeftEdge));

   // check to make sure tempo is not higher than 20 beats per second
   // (In principle, tempo can be higher, but not infinity.)
   double minPeriod = 0.05; // minimum beat period
   double qBeat0, qBeat1;
   pNt->NearestBeatTime(viewInfo.selectedRegion.t0(), &qBeat0); // get beat
   pNt->NearestBeatTime(viewInfo.selectedRegion.t1(), &qBeat1);

   // We could be moving 3 things: left edge, right edge, a point between
   switch (mStretchMode) {
   case stretchLeft: {
      // make sure target duration is not too short
      double dur = viewInfo.selectedRegion.t1() - moveto;
      if (dur < mStretchRightBeats * minPeriod) {
         dur = mStretchRightBeats * minPeriod;
         moveto = viewInfo.selectedRegion.t1() - dur;
      }
      if (pNt->StretchRegion(mStretchSel0, mStretchSel1, dur)) {
         pNt->SetOffset(pNt->GetOffset() + moveto - mStretchSel0);
         viewInfo.selectedRegion.setT0(moveto);
      }
      break;
   }
   case stretchRight: {
      // make sure target duration is not too short
      double dur = moveto - viewInfo.selectedRegion.t0();
      if (dur < mStretchLeftBeats * minPeriod) {
         dur = mStretchLeftBeats * minPeriod;
         moveto = mStretchSel0 + dur;
      }
      if (pNt->StretchRegion(mStretchSel0, mStretchSel1, dur)) {
         viewInfo.selectedRegion.setT1(moveto);
      }
      break;
   }
   case stretchCenter: {
      // make sure both left and right target durations are not too short
      double left_dur = moveto - viewInfo.selectedRegion.t0();
      double right_dur = viewInfo.selectedRegion.t1() - moveto;
      double centerBeat;
      pNt->NearestBeatTime(mSelStart, &centerBeat);
      if (left_dur < mStretchLeftBeats * minPeriod) {
         left_dur = mStretchLeftBeats * minPeriod;
      }
      if (right_dur < mStretchRightBeats * minPeriod) {
         right_dur = mStretchRightBeats * minPeriod;
      }
      pNt->StretchRegion(mStretchStart, mStretchSel1, right_dur);
      pNt->StretchRegion(mStretchSel0, mStretchStart, left_dur);
      break;
   }
   default:
      wxASSERT(false);
      break;
   }
   mStretched = true;
}
