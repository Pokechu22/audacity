/**********************************************************************

Audacity: A Digital Audio Editor

TimeShiftHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../Audacity.h"
#include "TimeShiftHandle.h"

#include "TrackControls.h"
#include "../../AColor.h"
#include "../../HitTestResult.h"
#include "../../Project.h"
#include "../../RefreshCode.h"
#include "../../Snap.h"
#include "../../TrackPanelMouseEvent.h"
#include "../../toolbars/ToolsToolBar.h"
#include "../../UndoManager.h"
#include "../../WaveTrack.h"
#include "../../../images/Cursors.h"

TimeShiftHandle::TimeShiftHandle()
   : mCapturedTrack(0)
   , mRect()
   , mCapturedClip(0)
   , mCapturedClipArray()
   , mTrackExclusions()
   , mCapturedClipIsSelection(false)
   , mHSlideAmount(0.0)
   , mDidSlideVertically(false)
   , mSlideUpDownOnly(false)
   , mSnapPreferRightEdge(false)
   , mMouseClickX(0)
   , mSnapManager()
   , mSnapLeft(0)
   , mSnapRight(0)
{
}

TimeShiftHandle &TimeShiftHandle::Instance()
{
   static TimeShiftHandle instance;
   return instance;
}

HitTestPreview TimeShiftHandle::HitPreview
(const AudacityProject *pProject, bool unsafe)
{
   static std::unique_ptr<wxCursor> disabledCursor(
      ::MakeCursor(wxCURSOR_NO_ENTRY, DisabledCursorXpm, 16, 16)
   );
   static std::unique_ptr<wxCursor> slideCursor(
      MakeCursor(wxCURSOR_SIZEWE, TimeCursorXpm, 16, 16)
   );
   const ToolsToolBar *const ttb = pProject->GetToolsToolBar();
   return HitTestPreview(
      ttb->GetMessageForTool(slideTool),
      (unsafe
       ? &*disabledCursor
       : &*slideCursor)
   );
}

HitTestResult TimeShiftHandle::HitAnywhere(const AudacityProject *pProject)
{
   // After all that, it still may be unsafe to drag.
   // Even if so, make an informative cursor change from default to "banned."
   const bool unsafe = pProject->IsAudioActive();
   return HitTestResult(
      HitPreview(pProject, unsafe),
      (unsafe
       ? NULL
       : &Instance())
   );
}

HitTestResult TimeShiftHandle::HitTest
   (const wxMouseEvent & event, const wxRect &rect, const AudacityProject *pProject)
{
   /// method that tells us if the mouse event landed on a
   /// time-slider that allows us to time shift the sequence.
   /// (Those are the two "grips" drawn at left and right edges for multi tool mode.)

   // Perhaps we should delegate this to TrackArtist as only TrackArtist
   // knows what the real sizes are??

   // The drag Handle width includes border, width and a little extra margin.
   const int adjustedDragHandleWidth = 14;
   // The hotspot for the cursor isn't at its centre.  Adjust for this.
   const int hotspotOffset = 5;

   // We are doing an approximate test here - is the mouse in the right or left border?
   if (!(event.m_x + hotspotOffset < rect.x + adjustedDragHandleWidth ||
       event.m_x + hotspotOffset >= rect.x + rect.width - adjustedDragHandleWidth))
      return HitTestResult();

   return HitAnywhere(pProject);
}

TimeShiftHandle::~TimeShiftHandle()
{
}

namespace
{
   // Adds a track's clips to mCapturedClipArray within a specified time
   void AddClipsToCaptured
      (TrackClipArray &capturedClipArray, Track *pTrack, double t0, double t1)
   {
      if (pTrack->GetKind() == Track::Wave)
      {
         for(const auto &clip: static_cast<WaveTrack*>(pTrack)->GetClips())
         {
            if (!clip->AfterClip(t0) && !clip->BeforeClip(t1))
            {
               // Avoid getting clips that were already captured
               bool newClip = true;
               for (unsigned int ii = 0; newClip && ii < capturedClipArray.size(); ++ii)
                  newClip = (capturedClipArray[ii].clip != clip.get());
               if (newClip)
                  capturedClipArray.push_back(TrackClip(pTrack, clip.get()));
            }
         }
      }
      else
      {
         // This handles label tracks rather heavy-handedly -- it would be nice to
         // treat individual labels like clips

         // Avoid adding a track twice
         bool newClip = true;
         for (unsigned int ii = 0; newClip && ii < capturedClipArray.size(); ++ii)
            newClip = (capturedClipArray[ii].track != pTrack);
         if (newClip) {
#ifdef USE_MIDI
            // do not add NoteTrack if the data is outside of time bounds
            if (pTrack->GetKind() == Track::Note) {
               if (pTrack->GetEndTime() < t0 || pTrack->GetStartTime() > t1)
                  return;
            }
#endif
            capturedClipArray.push_back(TrackClip(pTrack, NULL));
         }
      }
   }

   // Helper for the above, adds a track's clips to mCapturedClipArray (eliminates
   // duplication of this logic)
   void AddClipsToCaptured
      (TrackClipArray &capturedClipArray,
       const ViewInfo &viewInfo, Track *pTrack, bool withinSelection)
   {
      if (withinSelection)
         AddClipsToCaptured(capturedClipArray, pTrack,
            viewInfo.selectedRegion.t0(), viewInfo.selectedRegion.t1());
      else
         AddClipsToCaptured(capturedClipArray, pTrack,
            pTrack->GetStartTime(), pTrack->GetEndTime());
   }
}

namespace {
   // Don't count right channels.
   WaveTrack *NthAudioTrack(TrackList &list, int nn)
   {
      if (nn >= 0) {
         TrackListOfKindIterator iter(Track::Wave, &list);
         Track *pTrack = iter.First();
         while (pTrack && nn--)
            pTrack = iter.Next(true);
         return static_cast<WaveTrack*>(pTrack);
      }

      return NULL;
   }

   // Don't count right channels.
   int TrackPosition(TrackList &list, Track *pFindTrack)
   {
      Track *const partner = pFindTrack->GetLink();
      TrackListOfKindIterator iter(Track::Wave, &list);
      int nn = 0;
      for (Track *pTrack = iter.First(); pTrack; pTrack = iter.Next(true), ++nn) {
         if (pTrack == pFindTrack ||
             pTrack == partner)
            return nn;
      }
      return -1;
   }

   WaveClip *FindClipAtTime(WaveTrack *pTrack, double time)
   {
      if (pTrack) {
         // WaveClip::GetClipAtX doesn't work unless the clip is on the screen and can return bad info otherwise
         // instead calculate the time manually
         double rate = pTrack->GetRate();
         auto s0 = (sampleCount)(time * rate + 0.5);

         if (s0 >= 0)
            return pTrack->GetClipAtSample(s0);
      }

      return 0;
   }
}

UIHandle::Result TimeShiftHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   const wxMouseEvent &event = evt.event;
   const wxRect &rect = evt.rect;
   const ViewInfo &viewInfo = pProject->GetViewInfo();

   Track *const pTrack = static_cast<Track*>(evt.pCell);

   using namespace RefreshCode;

   const bool unsafe = pProject->IsAudioActive();
   if (unsafe)
      return Cancelled;

   TrackList *const trackList = pProject->GetTracks();

   mHSlideAmount = 0.0;
   mDidSlideVertically = false;

   mTrackExclusions.clear();

   ToolsToolBar *const ttb = pProject->GetToolsToolBar();
   const bool multiToolModeActive = (ttb && ttb->IsDown(multiTool));

   const double clickTime =
      viewInfo.PositionToTime(event.m_x, rect.x);
   mCapturedClipIsSelection =
      (pTrack->GetSelected() &&
       clickTime > viewInfo.selectedRegion.t0() &&
       clickTime < viewInfo.selectedRegion.t1());

   WaveTrack *wt = pTrack->GetKind() == Track::Wave
      ? static_cast<WaveTrack*>(pTrack) : nullptr;

   if ((wt
#ifdef USE_MIDI
      || pTrack->GetKind() == Track::Note
#endif
      ) && !event.ShiftDown())
   {
#ifdef USE_MIDI
      if (!wt)
         mCapturedClip = nullptr;
      else
#endif
      {
         mCapturedClip = wt->GetClipAtX(event.m_x);
         if (mCapturedClip == NULL)
            return Cancelled;
      }
      // The captured clip is the focus, but we need to create a list
      // of all clips that have to move, also...

      mCapturedClipArray.clear();

      // First, if click was in selection, capture selected clips; otherwise
      // just the clicked-on clip
      if (mCapturedClipIsSelection) {
         TrackListIterator iter(trackList);
         for (Track *pTrack1 = iter.First(); pTrack1; pTrack1 = iter.Next()) {
            if (pTrack1->GetSelected()) {
               AddClipsToCaptured(mCapturedClipArray, viewInfo, pTrack1, true);
               if (pTrack1->GetKind() != Track::Wave)
                  mTrackExclusions.push_back(pTrack1);
            }
         }
      }
      else {
         mCapturedClipArray.push_back(TrackClip(pTrack, mCapturedClip));

         // Check for stereo partner
         Track *const partner = pTrack->GetLink();
         WaveTrack *wt;
         if (mCapturedClip &&
             // Assume linked track is wave or null
             nullptr != (wt = static_cast<WaveTrack*>(partner))) {
            WaveClip *const clip =
            FindClipAtTime(wt, viewInfo.PositionToTime(event.m_x, rect.x));
            if (clip)
               mCapturedClipArray.push_back(TrackClip(partner, clip));
         }
      }

      // Now, if sync-lock is enabled, capture any clip that's linked to a
      // captured clip.
      if (pProject->IsSyncLocked()) {
         // AWD: mCapturedClipArray expands as the loop runs, so newly-added
         // clips are considered (the effect is like recursion and terminates
         // because AddClipsToCaptured doesn't add duplicate clips); to remove
         // this behavior just store the array size beforehand.
         for (unsigned int ii = 0; ii < mCapturedClipArray.size(); ++ii) {
            // Capture based on tracks that have clips -- that means we
            // don't capture based on links to label tracks for now (until
            // we can treat individual labels as clips)
            WaveClip *const clip = mCapturedClipArray[ii].clip;
            if (clip) {
               const double startTime = clip->GetStartTime();
               const double endTime = clip->GetEndTime();
               // Iterate over sync-lock group tracks.
               SyncLockedTracksIterator git(trackList);
               for (Track *pTrack1 = git.StartWith(mCapturedClipArray[ii].track); pTrack1;
                  pTrack1 = git.Next()) {
                  AddClipsToCaptured(mCapturedClipArray, pTrack1, startTime, endTime);
                  if (pTrack1->GetKind() != Track::Wave)
                     mTrackExclusions.push_back(pTrack1);
               }
            }
#ifdef USE_MIDI
            // Capture additional clips from NoteTracks
            Track *const nt = mCapturedClipArray[ii].track;
            if (nt->GetKind() == Track::Note) {
               const double startTime = nt->GetStartTime();
               const double endTime = nt->GetEndTime();
               // Iterate over sync-lock group tracks.
               SyncLockedTracksIterator git(trackList);
               for (Track *pTrack1 = git.StartWith(nt); pTrack1; pTrack1 = git.Next()) {
                  AddClipsToCaptured(mCapturedClipArray, pTrack1, startTime, endTime);
                  if (pTrack1->GetKind() != Track::Wave)
                     mTrackExclusions.push_back(pTrack1);
               }
            }
#endif
         }
      }
   }
   else {
      // Shift was down, or track was not Wave or Note
      mCapturedClip = NULL;
      mCapturedClipArray.clear();
   }

   mSlideUpDownOnly = event.CmdDown() && !multiToolModeActive;
   mCapturedTrack = pTrack;
   mRect = rect;
   mMouseClickX = event.m_x;
   const double selStart = viewInfo.PositionToTime(event.m_x, mRect.x);
   mSnapManager.reset(new SnapManager(trackList,
      &viewInfo,
      &mCapturedClipArray,
      &mTrackExclusions,
      true // don't snap to time
   ));
   mSnapLeft = -1;
   mSnapRight = -1;
   mSnapPreferRightEdge =
      mCapturedClip &&
      (fabs(selStart - mCapturedClip->GetEndTime()) <
       fabs(selStart - mCapturedClip->GetStartTime()));

   return RefreshNone;
}

UIHandle::Result TimeShiftHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   const wxMouseEvent &event = evt.event;
   ViewInfo &viewInfo = pProject->GetViewInfo();

   // We may switch pTrack to its stereo partner below
   Track *pTrack = dynamic_cast<Track*>(evt.pCell);

   // Uncommenting this permits drag to continue to work even over the controls area
   /*
   pTrack = static_cast<CommonTrackPanelCell*>(evt.pCell)->FindTrack();
   */

   if (!pTrack) {
      // Allow sliding if the pointer is not over any track, but only if x is
      // within the bounds of the tracks area.
      if (event.m_x >= mRect.GetX() &&
         event.m_x < mRect.GetX() + mRect.GetWidth())
          pTrack = mCapturedTrack;
   }

   if (!pTrack)
      return RefreshCode::RefreshNone;

   using namespace RefreshCode;
   const bool unsafe = pProject->IsAudioActive();
   if (unsafe) {
      this->Cancel(pProject);
      return RefreshAll | Cancelled;
   }

   TrackList *const trackList = pProject->GetTracks();

   // GM: DoSlide now implementing snap-to
   // samples functionality based on sample rate.

   // Start by undoing the current slide amount; everything
   // happens relative to the original horizontal position of
   // each clip...
#ifdef USE_MIDI
   if (mCapturedClipArray.size())
#else
   if (mCapturedClip)
#endif
   {
      for (unsigned ii = 0; ii < mCapturedClipArray.size(); ++ii) {
         if (mCapturedClipArray[ii].clip)
            mCapturedClipArray[ii].clip->Offset(-mHSlideAmount);
         else
            mCapturedClipArray[ii].track->Offset(-mHSlideAmount);
      }
   }
   else {
      // Was a shift-click
      mCapturedTrack->Offset(-mHSlideAmount);
      Track *const link = mCapturedTrack->GetLink();
      if (link)
         link->Offset(-mHSlideAmount);
   }

   if (mCapturedClipIsSelection) {
      // Slide the selection, too
      viewInfo.selectedRegion.move(-mHSlideAmount);
   }
   mHSlideAmount = 0.0;

   double desiredSlideAmount;
   if (mSlideUpDownOnly) {
      desiredSlideAmount = 0.0;
   }
   else {
      desiredSlideAmount =
         viewInfo.PositionToTime(event.m_x) -
         viewInfo.PositionToTime(mMouseClickX);
      bool trySnap = false;
      double clipLeft = 0, clipRight = 0;
#ifdef USE_MIDI
      if (pTrack->GetKind() == Track::Wave) {
         WaveTrack *const mtw = static_cast<WaveTrack*>(pTrack);
         const double rate = mtw->GetRate();
         // set it to a sample point
         desiredSlideAmount = rint(desiredSlideAmount * rate) / rate;
      }

      // Adjust desiredSlideAmount using SnapManager
      if (mSnapManager.get() && mCapturedClipArray.size()) {
         trySnap = true;
         if (mCapturedClip) {
            clipLeft = mCapturedClip->GetStartTime() + desiredSlideAmount;
            clipRight = mCapturedClip->GetEndTime() + desiredSlideAmount;
         }
         else {
            clipLeft = mCapturedTrack->GetStartTime() + desiredSlideAmount;
            clipRight = mCapturedTrack->GetEndTime() + desiredSlideAmount;
         }
      }
#else
      {
         trySnap = true;
         if (pTrack->GetKind() == Track::Wave) {
            const double rate = pTrack->GetRate();
            // set it to a sample point
            desiredSlideAmount = rint(desiredSlideAmount * rate) / rate;
         }
         if (mSnapManager && mCapturedClip) {
            clipLeft = mCapturedClip->GetStartTime() + desiredSlideAmount;
            clipRight = mCapturedClip->GetEndTime() + desiredSlideAmount;
         }
      }
#endif
      if (trySnap)
      {
         double newClipLeft = clipLeft;
         double newClipRight = clipRight;

         bool dummy1, dummy2;
         mSnapManager->Snap(mCapturedTrack, clipLeft, false, &newClipLeft,
            &dummy1, &dummy2);
         mSnapManager->Snap(mCapturedTrack, clipRight, false, &newClipRight,
            &dummy1, &dummy2);

         // Only one of them is allowed to snap
         if (newClipLeft != clipLeft && newClipRight != clipRight) {
            // Un-snap the un-preferred edge
            if (mSnapPreferRightEdge)
               newClipLeft = clipLeft;
            else
               newClipRight = clipRight;
         }

         // Take whichever one snapped (if any) and compute the NEW desiredSlideAmount
         mSnapLeft = -1;
         mSnapRight = -1;
         if (newClipLeft != clipLeft) {
            const double difference = (newClipLeft - clipLeft);
            desiredSlideAmount += difference;
            mSnapLeft = viewInfo.TimeToPosition(newClipLeft, mRect.x);
         }
         else if (newClipRight != clipRight) {
            const double difference = (newClipRight - clipRight);
            desiredSlideAmount += difference;
            mSnapRight = viewInfo.TimeToPosition(newClipRight, mRect.x);
         }
      }
   }

   // Scroll during vertical drag.
   // EnsureVisible(pTrack); //vvv Gale says this has problems on Linux, per bug 393 thread. Revert for 2.0.2.
   bool slidVertically = false;

   // If the mouse is over a track that isn't the captured track,
   // decide which tracks the captured clips should go to.
   if (mCapturedClip &&
       pTrack != mCapturedTrack &&
       pTrack->GetKind() == Track::Wave
       /* && !mCapturedClipIsSelection*/)
   {
      const int diff =
         TrackPosition(*trackList, pTrack) -
         TrackPosition(*trackList, mCapturedTrack);
      for (unsigned ii = 0, nn = mCapturedClipArray.size(); ii < nn; ++ii) {
         TrackClip &trackClip = mCapturedClipArray[ii];
         if (trackClip.clip) {
            // Move all clips up or down by an equal count of audio tracks.
            Track *const pSrcTrack = trackClip.track;
            auto pDstTrack = NthAudioTrack(*trackList,
               diff + TrackPosition(*trackList, pSrcTrack));
            // Can only move mono to mono, or left to left, or right to right
            // And that must be so for each captured clip
            bool stereo = (pSrcTrack->GetLink() != 0);
            if (pDstTrack && stereo && !pSrcTrack->GetLinked())
               // Assume linked track is wave or null
               pDstTrack = static_cast<WaveTrack*>(pDstTrack->GetLink());
            bool ok = pDstTrack &&
            (stereo == (pDstTrack->GetLink() != 0)) &&
            (!stereo || (pSrcTrack->GetLinked() == pDstTrack->GetLinked()));
            if (ok)
               trackClip.dstTrack = pDstTrack;
            else
               return RefreshAll;
         }
      }

      // Having passed that test, remove clips temporarily from their
      // tracks, so moving clips don't interfere with each other
      // when we call CanInsertClip()
      for (unsigned ii = 0, nn = mCapturedClipArray.size(); ii < nn;  ++ii) {
         TrackClip &trackClip = mCapturedClipArray[ii];
         WaveClip *const pSrcClip = trackClip.clip;
         if (pSrcClip)
            trackClip.holder =
               // Assume track is wave because it has a clip
               static_cast<WaveTrack*>(trackClip.track)->
                  RemoveAndReturnClip(pSrcClip);
      }

      // Now check that the move is possible
      bool ok = true;
      for (unsigned ii = 0, nn = mCapturedClipArray.size(); ok && ii < nn; ++ii) {
         TrackClip &trackClip = mCapturedClipArray[ii];
         WaveClip *const pSrcClip = trackClip.clip;
         if (pSrcClip)
            ok = trackClip.dstTrack->CanInsertClip(pSrcClip);
      }

      if (!ok) {
         // Failure -- put clips back where they were
         for (unsigned ii = 0, nn = mCapturedClipArray.size(); ii < nn;  ++ii) {
            TrackClip &trackClip = mCapturedClipArray[ii];
            WaveClip *const pSrcClip = trackClip.clip;
            if (pSrcClip)
               // Assume track is wave because it has a clip
                  static_cast<WaveTrack*>(trackClip.track)->
                     AddClip(std::move(trackClip.holder));
         }
         return RefreshAll;
      }
      else {
         // Do the vertical moves of clips
         for (unsigned ii = 0, nn = mCapturedClipArray.size(); ii < nn; ++ii) {
            TrackClip &trackClip = mCapturedClipArray[ii];
            WaveClip *const pSrcClip = trackClip.clip;
            if (pSrcClip) {
               const auto dstTrack = trackClip.dstTrack;
               dstTrack->AddClip(std::move(trackClip.holder));
            }
         }

         mCapturedTrack = pTrack;
         mDidSlideVertically = true;

         // Make the offset permanent; start from a "clean slate"
         mMouseClickX = event.m_x;
      }

      // Not done yet, check for horizontal movement.
      slidVertically = true;
   }

   if (desiredSlideAmount == 0.0)
      return RefreshAll;

   mHSlideAmount = desiredSlideAmount;

#ifdef USE_MIDI
   if (mCapturedClipArray.size())
#else
   if (mCapturedClip)
#endif
   {
      double allowed;
      double initialAllowed;
      const double safeBigDistance = 1000 + 2.0 * (trackList->GetEndTime() -
         trackList->GetStartTime());

      do { // loop to compute allowed, does not actually move anything yet
         initialAllowed = mHSlideAmount;

         for (unsigned ii = 0; ii < mCapturedClipArray.size(); ++ii) {
            WaveClip *const clip = mCapturedClipArray[ii].clip;
            if (clip) { // only audio clips are used to compute allowed
               WaveTrack *const track =
                  static_cast<WaveTrack *>(mCapturedClipArray[ii].track);
               // Move all other selected clips totally out of the way
               // temporarily because they're all moving together and
               // we want to find out if OTHER clips are in the way,
               // not one of the moving ones
               for (unsigned jj = 0; jj < mCapturedClipArray.size(); ++jj) {
                  WaveClip *const clip2 = mCapturedClipArray[jj].clip;
                  if (clip2 && clip2 != clip)
                     clip2->Offset(-safeBigDistance);
               }

               if (track->CanOffsetClip(clip, mHSlideAmount, &allowed)) {
                  if (mHSlideAmount != allowed) {
                     mHSlideAmount = allowed;
                     mSnapLeft = mSnapRight = -1; // see bug 1067
                  }
               }
               else {
                  mHSlideAmount = 0.0;
                  mSnapLeft = mSnapRight = -1; // see bug 1067
               }

               for (unsigned jj = 0; jj < mCapturedClipArray.size(); ++jj) {
                  WaveClip *const clip2 = mCapturedClipArray[jj].clip;
                  if (clip2 && clip2 != clip)
                     clip2->Offset(safeBigDistance);
               }
            }
         }
      } while (mHSlideAmount != initialAllowed);

      if (mHSlideAmount != 0.0) { // finally, here is where clips are moved
         unsigned int ii;
         for (ii = 0; ii < mCapturedClipArray.size(); ++ii) {
            Track *const track = mCapturedClipArray[ii].track;
            WaveClip *const clip = mCapturedClipArray[ii].clip;
            if (clip)
               clip->Offset(mHSlideAmount);
            else
               track->Offset(mHSlideAmount);
         }
      }
   }
   else {
      // For Shift key down, or
      // For non wavetracks, specifically label tracks ...
      // Or for shift-(ctrl-)drag which moves all clips of a track together
      mCapturedTrack->Offset(mHSlideAmount);
      Track *const link = mCapturedTrack->GetLink();
      if (link)
         link->Offset(mHSlideAmount);
   }

   if (mCapturedClipIsSelection) {
      // Slide the selection, too
      viewInfo.selectedRegion.move(mHSlideAmount);
   }


   if (slidVertically) {
      // NEW origin
      mHSlideAmount = 0;
   }

   return RefreshAll;
}

HitTestPreview TimeShiftHandle::Preview
(const TrackPanelMouseEvent &, const AudacityProject *pProject)
{
   return HitPreview(pProject, false);
}

UIHandle::Result TimeShiftHandle::Release
(const TrackPanelMouseEvent &, AudacityProject *pProject,
 wxWindow *)
{
   using namespace RefreshCode;
   const bool unsafe = pProject->IsAudioActive();
   if (unsafe)
      return this->Cancel(pProject);

   Result result = RefreshNone;

   mCapturedTrack = NULL;
   mSnapManager.reset(NULL);
   mCapturedClipArray.clear();

   // Do not draw yellow lines
   if (mSnapLeft != -1 || mSnapRight != -1) {
      mSnapLeft = mSnapRight = -1;
      result |= RefreshAll;
   }

   if (!mDidSlideVertically && mHSlideAmount == 0)
      return result;
   
   for (size_t ii = 0; ii < mCapturedClipArray.size(); ++ii)
   {
      TrackClip &trackClip = mCapturedClipArray[ii];
      WaveClip* pWaveClip = trackClip.clip;
      // Note that per TrackPanel::AddClipsToCaptured(Track *t, double t0, double t1),
      // in the non-WaveTrack case, the code adds a NULL clip to mCapturedClipArray,
      // so we have to check for that any time we're going to deref it.
      // Previous code did not check it here, and that caused bug 367 crash.
      if (pWaveClip &&
         trackClip.track != trackClip.origTrack)
      {
         // Now that user has dropped the clip into a different track,
         // make sure the sample rate matches the destination track (mCapturedTrack).
         // Assume the clip was dropped in a wave track
         pWaveClip->Resample
            (static_cast<WaveTrack*>(trackClip.track)->GetRate());
         pWaveClip->MarkChanged();
      }
   }

   wxString msg;
   bool consolidate;
   if (mDidSlideVertically) {
      msg.Printf(_("Moved clips to another track"));
      consolidate = false;
   }
   else {
      wxString direction = mHSlideAmount > 0 ?
         /* i18n-hint: a direction as in left or right.*/
         _("right") :
         /* i18n-hint: a direction as in left or right.*/
         _("left");
      /* i18n-hint: %s is a direction like left or right */
      msg.Printf(_("Time shifted tracks/clips %s %.02f seconds"),
         direction.c_str(), fabs(mHSlideAmount));
      consolidate = true;
   }
   pProject->PushState(msg, _("Time-Shift"),
      consolidate ? (UndoPush::CONSOLIDATE) : (UndoPush::AUTOSAVE));

   return result | FixScrollbars;
}

UIHandle::Result TimeShiftHandle::Cancel(AudacityProject *pProject)
{
   pProject->RollbackState();
   mCapturedTrack = NULL;
   mSnapManager.reset(NULL);
   mCapturedClipArray.clear();
   return RefreshCode::RefreshAll;
}

void TimeShiftHandle::DrawExtras
(DrawingPass pass,
 wxDC * dc, const wxRegion &, const wxRect &)
{
   if (pass == Panel) {
      // Draw snap guidelines if we have any
      if (mSnapManager.get() && (mSnapLeft >= 0 || mSnapRight >= 0)) {
         AColor::SnapGuidePen(dc);
         if (mSnapLeft >= 0) {
            AColor::Line(*dc, (int)mSnapLeft, 0, mSnapLeft, 30000);
         }
         if (mSnapRight >= 0) {
            AColor::Line(*dc, (int)mSnapRight, 0, mSnapRight, 30000);
         }
      }
   }
}
