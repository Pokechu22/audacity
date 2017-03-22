/**********************************************************************

Audacity: A Digital Audio Editor

SelectHandle.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "SelectHandle.h"

#include "Scrubbing.h"
#include "TrackControls.h"

#include "../../AColor.h"
#include "../../FreqWindow.h"
#include "../../HitTestResult.h"
#include "../../MixerBoard.h"
#include "../../NumberScale.h"
#include "../../Project.h"
#include "../../RefreshCode.h"
#include "../../Snap.h"
#include "../../TrackPanel.h"
#include "../../TrackPanelMouseEvent.h"
#include "../../ViewInfo.h"
#include "../../WaveTrack.h"
#include "../../commands/Keyboard.h"
#include "../../ondemand/ODManager.h"
#include "../../prefs/SpectrogramSettings.h"
#include "../../toolbars/ToolsToolBar.h"
#include "../../../images/Cursors.h"

#include "../../Experimental.h"

enum {
   //This constant determines the size of the horizontal region (in pixels) around
   //the right and left selection bounds that can be used for horizontal selection adjusting
   //(or, vertical distance around top and bottom bounds in spectrograms,
   // for vertical selection adjusting)
   SELECTION_RESIZE_REGION = 3,

   // Seems 4 is too small to work at the top.  Why?
   FREQ_SNAP_DISTANCE = 10,
};

// #define SPECTRAL_EDITING_ESC_KEY

SelectHandle::SelectHandle()
   : mpTrack(0)
   , mRect()
   , mInitialSelection()
   , mInitialTrackSelection()

   , mSnapManager()
   , mSnapLeft(-1)
   , mSnapRight(-1)

   , mSelStartValid(false)
   , mSelStart(0.0)

   , mSelectionBoundary(0)

   , mFreqSelMode(FREQ_SEL_INVALID)
   , mFreqSelTrack(NULL)
   , mFreqSelPin(-1.0)
   , mFrequencySnapper(new SpectrumAnalyst)

   , mMostRecentX(-1)
   , mMostRecentY(-1)

   , mAutoScrolling(false)

   , mConnectedProject(NULL)
{
}

SelectHandle &SelectHandle::Instance()
{
   static SelectHandle instance;
   return instance;
}

namespace
{
   // If we're in OnDemand mode, we may change the tip.
   void MaySetOnDemandTip(const Track * t, wxString &tip)
   {
      wxASSERT(t);
      //For OD regions, we need to override and display the percent complete for this task.
      //first, make sure it's a wavetrack.
      if (t->GetKind() != Track::Wave)
         return;
      //see if the wavetrack exists in the ODManager (if the ODManager exists)
      if (!ODManager::IsInstanceCreated())
         return;
      //ask the wavetrack for the corresponding tip - it may not change tip, but that's fine.
      ODManager::Instance()->FillTipForWaveTrack(static_cast<const WaveTrack*>(t), tip);
      return;
   }

   /// Converts a frequency to screen y position.
   wxInt64 FrequencyToPosition(const WaveTrack *wt,
      double frequency,
      wxInt64 trackTopEdge,
      int trackHeight)
   {
      const double rate = wt->GetRate();
      const SpectrogramSettings &settings = wt->GetSpectrogramSettings();
      float minFreq, maxFreq;
      wt->GetSpectrumBounds(&minFreq, &maxFreq);
      const NumberScale numberScale(settings.GetScale(minFreq, maxFreq));
      const float p = numberScale.ValueToPosition(frequency);
      return trackTopEdge + wxInt64((1.0 - p) * trackHeight);
   }

   /// Converts a position (mouse Y coordinate) to
   /// frequency, in Hz.
   double PositionToFrequency(const WaveTrack *wt,
      bool maySnap,
      wxInt64 mouseYCoordinate,
      wxInt64 trackTopEdge,
      int trackHeight)
   {
      const double rate = wt->GetRate();

      // Handle snapping
      if (maySnap &&
         mouseYCoordinate - trackTopEdge < FREQ_SNAP_DISTANCE)
         return rate;
      if (maySnap &&
         trackTopEdge + trackHeight - mouseYCoordinate < FREQ_SNAP_DISTANCE)
         return -1;

      const SpectrogramSettings &settings = wt->GetSpectrogramSettings();
      float minFreq, maxFreq;
      wt->GetSpectrumBounds(&minFreq, &maxFreq);
      const NumberScale numberScale(settings.GetScale(minFreq, maxFreq));
      const double p = double(mouseYCoordinate - trackTopEdge) / trackHeight;
      return numberScale.PositionToValue(1.0 - p);
   }

   template<typename T>
   inline void SetIfNotNull(T * pValue, const T Value)
   {
      if (pValue == NULL)
         return;
      *pValue = Value;
   }

   // This returns true if we're a spectral editing track.
   inline bool isSpectralSelectionTrack(const Track *pTrack) {
      if (pTrack &&
         pTrack->GetKind() == Track::Wave) {
         const WaveTrack *const wt = static_cast<const WaveTrack*>(pTrack);
         const SpectrogramSettings &settings = wt->GetSpectrogramSettings();
         const int display = wt->GetDisplay();
         return (display == WaveTrack::Spectrum) && settings.SpectralSelectionEnabled();
      }
      else {
         return false;
      }
   }

   enum SelectionBoundary {
      SBNone,
      SBLeft, SBRight,
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
      SBBottom, SBTop, SBCenter, SBWidth,
#endif
   };

   SelectionBoundary ChooseTimeBoundary
      (const ViewInfo &viewInfo,
      double selend, bool onlyWithinSnapDistance,
      wxInt64 *pPixelDist, double *pPinValue)
   {
      const double t0 = viewInfo.selectedRegion.t0();
      const double t1 = viewInfo.selectedRegion.t1();
      const wxInt64 posS = viewInfo.TimeToPosition(selend);
      const wxInt64 pos0 = viewInfo.TimeToPosition(t0);
      wxInt64 pixelDist = std::abs(posS - pos0);
      bool chooseLeft = true;

      if (viewInfo.selectedRegion.isPoint())
         // Special case when selection is a point, and thus left
         // and right distances are the same
         chooseLeft = (selend < t0);
      else {
         const wxInt64 pos1 = viewInfo.TimeToPosition(t1);
         const wxInt64 rightDist = std::abs(posS - pos1);
         if (rightDist < pixelDist)
            chooseLeft = false, pixelDist = rightDist;
      }

      SetIfNotNull(pPixelDist, pixelDist);

      if (onlyWithinSnapDistance &&
         pixelDist >= SELECTION_RESIZE_REGION) {
         SetIfNotNull(pPinValue, -1.0);
         return SBNone;
      }
      else if (chooseLeft) {
         SetIfNotNull(pPinValue, t1);
         return SBLeft;
      }
      else {
         SetIfNotNull(pPinValue, t0);
         return SBRight;
      }
   }

   SelectionBoundary ChooseBoundary
      (const ViewInfo &viewInfo,
       const wxMouseEvent & event, const Track *pTrack, const wxRect &rect,
       bool mayDragWidth, bool onlyWithinSnapDistance,
       double *pPinValue = NULL)
   {
      // Choose one of four boundaries to adjust, or the center frequency.
      // May choose frequencies only if in a spectrogram view and
      // within the time boundaries.
      // May choose no boundary if onlyWithinSnapDistance is true.
      // Otherwise choose the eligible boundary nearest the mouse click.
      const double selend = viewInfo.PositionToTime(event.m_x, rect.x);
      wxInt64 pixelDist = 0;
      SelectionBoundary boundary =
         ChooseTimeBoundary(viewInfo, selend, onlyWithinSnapDistance,
         &pixelDist, pPinValue);

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
      const double t0 = viewInfo.selectedRegion.t0();
      const double t1 = viewInfo.selectedRegion.t1();
      const double f0 = viewInfo.selectedRegion.f0();
      const double f1 = viewInfo.selectedRegion.f1();
      const double fc = viewInfo.selectedRegion.fc();
      double ratio = 0;

      bool chooseTime = true;
      bool chooseBottom = true;
      bool chooseCenter = false;
      // Consider adjustment of frequencies only if mouse is
      // within the time boundaries
      if (!viewInfo.selectedRegion.isPoint() &&
         t0 <= selend && selend < t1 &&
         isSpectralSelectionTrack(pTrack)) {
         // Spectral selection track is always wave
         const WaveTrack *const wt = static_cast<const WaveTrack*>(pTrack);
         const wxInt64 bottomSel = (f0 >= 0)
            ? FrequencyToPosition(wt, f0, rect.y, rect.height)
            : rect.y + rect.height;
         const wxInt64 topSel = (f1 >= 0)
            ? FrequencyToPosition(wt, f1, rect.y, rect.height)
            : rect.y;
         wxInt64 signedBottomDist = (int)(event.m_y - bottomSel);
         wxInt64 verticalDist = std::abs(signedBottomDist);
         if (bottomSel == topSel)
            // Top and bottom are too close to resolve on screen
            chooseBottom = (signedBottomDist >= 0);
         else {
            const wxInt64 topDist = std::abs((int)(event.m_y - topSel));
            if (topDist < verticalDist)
               chooseBottom = false, verticalDist = topDist;
         }
         if (fc > 0
#ifdef SPECTRAL_EDITING_ESC_KEY
            && mayDragWidth
#endif
            ) {
            const wxInt64 centerSel =
               FrequencyToPosition(wt, fc, rect.y, rect.height);
            const wxInt64 centerDist = abs((int)(event.m_y - centerSel));
            if (centerDist < verticalDist)
               chooseCenter = true, verticalDist = centerDist,
               ratio = f1 / fc;
         }
         if (verticalDist >= 0 &&
            verticalDist < pixelDist) {
            pixelDist = verticalDist;
            chooseTime = false;
         }
      }

      if (!chooseTime) {
         // PRL:  Seems I need a larger tolerance to make snapping work
         // at top of track, not sure why
         if (onlyWithinSnapDistance &&
            pixelDist >= FREQ_SNAP_DISTANCE) {
            SetIfNotNull(pPinValue, -1.0);
            return SBNone;
         }
         else if (chooseCenter) {
            SetIfNotNull(pPinValue, ratio);
            return SBCenter;
         }
         else if (mayDragWidth && fc > 0) {
            SetIfNotNull(pPinValue, fc);
            return SBWidth;
         }
         else if (chooseBottom) {
            SetIfNotNull(pPinValue, f1);
            return SBBottom;
         }
         else {
            SetIfNotNull(pPinValue, f0);
            return SBTop;
         }
      }
      else
#endif
      {
         return boundary;
      }
   }

   wxCursor *SelectCursor()
   {
      static std::unique_ptr<wxCursor> selectCursor(
         ::MakeCursor(wxCURSOR_IBEAM, IBeamCursorXpm, 17, 16)
      );
      return &*selectCursor;
   }

   wxCursor *EnvelopeCursor()
   {
      // This one doubles as the center frequency cursor for spectral selection:
      static std::unique_ptr<wxCursor> envelopeCursor(
         ::MakeCursor(wxCURSOR_ARROW, EnvCursorXpm, 16, 16)
      );
      return &*envelopeCursor;
   }

   void SetTipAndCursorForBoundary
      (SelectionBoundary boundary, bool frequencySnapping,
       wxString &tip, wxCursor *&pCursor)
   {
      static wxCursor adjustLeftSelectionCursor(
         wxCURSOR_POINT_LEFT
      );
      static wxCursor adjustRightSelectionCursor(
         wxCURSOR_POINT_RIGHT
      );

      static std::unique_ptr<wxCursor> bottomFrequencyCursor(
         ::MakeCursor(wxCURSOR_ARROW, BottomFrequencyCursorXpm, 16, 16)
      );
      static std::unique_ptr<wxCursor> topFrequencyCursor(
         ::MakeCursor(wxCURSOR_ARROW, TopFrequencyCursorXpm, 16, 16)
      );
      static std::unique_ptr<wxCursor> bandWidthCursor(
         ::MakeCursor(wxCURSOR_ARROW, BandWidthCursorXpm, 16, 16)
      );

      switch (boundary) {
      case SBNone:
         pCursor = SelectCursor();
         break;
      case SBLeft:
         tip = _("Click and drag to move left selection boundary.");
         pCursor = &adjustLeftSelectionCursor;
         break;
      case SBRight:
         tip = _("Click and drag to move right selection boundary.");
         pCursor = &adjustRightSelectionCursor;
         break;
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
      case SBBottom:
         tip = _("Click and drag to move bottom selection frequency.");
         pCursor = &*bottomFrequencyCursor;
         break;
      case SBTop:
         tip = _("Click and drag to move top selection frequency.");
         pCursor = &*topFrequencyCursor;
         break;
      case SBCenter:
      {
#ifndef SPECTRAL_EDITING_ESC_KEY
         tip =
            frequencySnapping ?
            _("Click and drag to move center selection frequency to a spectral peak.") :
            _("Click and drag to move center selection frequency.");

#else
         shiftDown;

         tip =
            _("Click and drag to move center selection frequency.");

#endif

         pCursor = EnvelopeCursor();
      }
      break;
      case SBWidth:
         tip = _("Click and drag to adjust frequency bandwidth.");
         pCursor = &*bandWidthCursor;
         break;
#endif
      default:
         wxASSERT(false);
      } // switch
      // Falls through the switch if there was no boundary found.
   }
}

HitTestResult SelectHandle::HitTest
(const TrackPanelMouseEvent &evt, const AudacityProject *pProject, const Track *pTrack)
{
   const wxMouseEvent &event = evt.event;
   const wxRect &rect = evt.rect;

   wxCursor *pCursor = SelectCursor();
   const bool bMultiToolMode  = pProject->GetToolsToolBar()->IsDown(multiTool);
   wxString tip;

   //In Multi-tool mode, give multitool prompt if no-special-hit.
   if (bMultiToolMode) {
      // Look up the current key binding for Preferences.
      // (Don't assume it's the default!)
      wxString keyStr
         (pProject->GetCommandManager()->GetKeyFromName(wxT("Preferences")));
      if (keyStr.IsEmpty())
         // No keyboard preference defined for opening Preferences dialog
         /* i18n-hint: These are the names of a menu and a command in that menu */
         keyStr = _("Edit, Preferences...");
      else
         keyStr = KeyStringDisplay(keyStr);
      /* i18n-hint: %s is usually replaced by "Ctrl+P" for Windows/Linux, "Command+," for Mac */
      tip = wxString::Format(
         _("Multi-Tool Mode: %s for Mouse and Keyboard Preferences."),
         keyStr.c_str());
      // Later in this function we may point to some other string instead.
   }

   const ViewInfo &viewInfo = pProject->GetViewInfo();

   //Make sure we are within the selected track
   // Adjusting the selection edges can be turned off in
   // the preferences...
   if (!pTrack->GetSelected() || !viewInfo.bAdjustSelectionEdges)
   {
      MaySetOnDemandTip(pTrack, tip);
      return HitTestResult(
         HitTestPreview(tip, pCursor),
         &Instance()
      );
   }

   {
      wxInt64 leftSel = viewInfo.TimeToPosition(viewInfo.selectedRegion.t0(), rect.x);
      wxInt64 rightSel = viewInfo.TimeToPosition(viewInfo.selectedRegion.t1(), rect.x);
      // Something is wrong if right edge comes before left edge
      wxASSERT(!(rightSel < leftSel));
   }

   const bool bShiftDown = event.ShiftDown();
   const bool bCtrlDown = event.ControlDown();
   const bool bModifierDown = bShiftDown || bCtrlDown;

#if 0
   // This is a vestige of an idea in the prototype version.
   // Center would snap without mouse button down, click would pin the center
   // and drag width.
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   if ((mFreqSelMode == FREQ_SEL_SNAPPING_CENTER) &&
      isSpectralSelectionTrack(pTrack)) {
      // Not shift-down, but center frequency snapping toggle is on
      tip = _("Click and drag to set frequency bandwidth.");
      pCursor = &*envelopeCursor;
      return;
   }
#endif
#endif

   // If not shift-down and not snapping center, then
   // choose boundaries only in snapping tolerance,
   // and may choose center.
   SelectionBoundary boundary =
      ChooseBoundary(viewInfo, event, pTrack, rect, !bModifierDown, !bModifierDown);

   SetTipAndCursorForBoundary(boundary, !bShiftDown, tip, pCursor);

   MaySetOnDemandTip(pTrack, tip);

   return HitTestResult(
      HitTestPreview(tip, pCursor),
      &Instance()
   );
}

SelectHandle::~SelectHandle()
{
}

namespace {
   // Is the distance between A and B less than D?
   template < class A, class B, class DIST > bool within(A a, B b, DIST d)
   {
      return (a > b - d) && (a < b + d);
   }


   inline double findMaxRatio(double center, double rate)
   {
      const double minFrequency = 1.0;
      const double maxFrequency = (rate / 2.0);
      const double frequency =
         std::min(maxFrequency,
            std::max(minFrequency, center));
      return
         std::min(frequency / minFrequency, maxFrequency / frequency);
   }
}

UIHandle::Result SelectHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   /// This method gets called when we're handling selection
   /// and the mouse was just clicked.

   using namespace RefreshCode;

   wxMouseEvent &event = evt.event;
   Track *const pTrack = static_cast<Track*>(evt.pCell);
   ViewInfo &viewInfo = pProject->GetViewInfo();

   mMostRecentX = event.m_x;
   mMostRecentY = event.m_y;

   TrackPanel *const trackPanel = pProject->GetTrackPanel();

   if( pTrack->GetKind() == Track::Label &&
       event.LeftDown() &&
       event.ControlDown() ){
      bool bShift = event.ShiftDown();
      bool unsafe = pProject->IsAudioActive();
      trackPanel->HandleListSelection(pTrack, bShift, true, !unsafe);
      // Do not start a drag
      return RefreshAll | Cancelled;
   }

   if (event.LeftDClick() && !event.ShiftDown()) {
      TrackList *const trackList = pProject->GetTracks();

      // Deselect all other tracks and select this one.
      trackPanel->SelectNone();
      trackPanel->SelectTrack(pTrack, true);

      // Default behavior: select whole track
      trackPanel->SelectTrackLength(pTrack);

      // Special case: if we're over a clip in a WaveTrack,
      // select just that clip
      if (pTrack->GetKind() == Track::Wave) {
         WaveTrack *const wt = static_cast<WaveTrack *>(pTrack);
         WaveClip *const selectedClip = wt->GetClipAtX(event.m_x);
         if (selectedClip) {
            viewInfo.selectedRegion.setTimes(
               selectedClip->GetOffset(), selectedClip->GetEndTime());
         }
      }

      pProject->ModifyState(false);

      // Do not start a drag
      return RefreshAll | UpdateSelection | Cancelled;
   }
   else if (!(event.LeftDown() ||
         (event.LeftDClick() && event.CmdDown())))
      return Cancelled;

   mpTrack = pTrack;
   mRect = evt.rect;
   mInitialSelection = viewInfo.selectedRegion;
   mSelectionBoundary = 0;

   // Save initial state of track selections
   mInitialTrackSelection.clear();

   TrackList *const trackList = pProject->GetTracks();

   {
      TrackListIterator iter(trackList);
      for (Track *t = iter.First(); t; t = iter.Next()) {
         const bool isSelected = t->GetSelected();
         mInitialTrackSelection.push_back(isSelected);
      }
   }

   // We create a NEW snap manager in case any snap-points have changed
   mSnapManager.reset(new SnapManager(trackList, &viewInfo));

   mSnapLeft = -1;
   mSnapRight = -1;

   bool bShiftDown = event.ShiftDown();
   bool bCtrlDown = event.ControlDown();

   // I. Shift-click adjusts an existing selection
   if (bShiftDown || bCtrlDown) {
      if (bShiftDown)
         trackPanel->ChangeSelectionOnShiftClick(pTrack);
      if( bCtrlDown ){
         //Commented out bIsSelected toggles, as in Track Control Panel.
         //bool bIsSelected = pTrack->GetSelected();
         //Actual bIsSelected will always add.
         bool bIsSelected = false;
         // Don't toggle away the last selected track.
         if( !bIsSelected || trackPanel->GetSelectedTrackCount() > 1 )
            trackPanel->SelectTrack( pTrack, !bIsSelected, true );
      }

      double value;
      // Shift-click, choose closest boundary
      SelectionBoundary boundary =
         ChooseBoundary(viewInfo, event, mpTrack, mRect, false, false, &value);
      mSelectionBoundary = boundary;
      switch (boundary) {
         case SBLeft:
         case SBRight:
         {
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
            // If drag starts, change time selection only
            // (also exit frequency snapping)
            mFreqSelMode = FREQ_SEL_INVALID;
#endif
            mSelStartValid = true;
            mSelStart = value;
            AdjustSelection(viewInfo, event.m_x, mRect.x, mpTrack);
            break;
         }
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
         case SBBottom:
         case SBTop:
         {
            mFreqSelTrack = static_cast<const WaveTrack*>(mpTrack);
            mFreqSelPin = value;
            mFreqSelMode =
               (boundary == SBBottom)
               ? FREQ_SEL_BOTTOM_FREE : FREQ_SEL_TOP_FREE;

            // Drag frequency only, not time:
            mSelStartValid = false;
            AdjustFreqSelection(viewInfo, event.m_y, mRect.y, mRect.height);
            break;
         }
         case SBCenter:
         {
            const auto wt = static_cast<const WaveTrack*>(pTrack);
            HandleCenterFrequencyClick(viewInfo, true, wt, value);
            break;
         }
#endif
         default:
            wxASSERT(false);
      };

      // For persistence of the selection change:
      pProject->ModifyState(false);

      // Get timer events so we can auto-scroll
      Connect(pProject);

      // Full refresh since the label area may need to indicate
      // newly selected tracks.
      return RefreshAll;
   }

   // II. Unmodified click starts a new selection

   //Make sure you are within the selected track
   bool startNewSelection = true;
   if (mpTrack && mpTrack->GetSelected()) {
      // Adjusting selection edges can be turned off in the
      // preferences now
      if (viewInfo.bAdjustSelectionEdges) {
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
         if (mFreqSelMode == FREQ_SEL_SNAPPING_CENTER &&
            isSpectralSelectionTrack(mpTrack)) {
            // This code is no longer reachable, but it had a place in the
            // spectral selection prototype.  It used to be that you could be
            // in a center-frequency-snapping mode that was not a mouse drag
            // but responded to mouse movements.  Click exited that and dragged
            // width instead.  PRL.

            // Ignore whether we are inside the time selection.
            // Exit center-snapping, start dragging the width.
            mFreqSelMode = FREQ_SEL_PINNED_CENTER;
            mFreqSelTrack = static_cast<WaveTrack*>(mpTrack);
            mFreqSelPin = viewInfo.selectedRegion.fc();
            // Do not adjust time boundaries
            mSelStartValid = false;
            AdjustFreqSelection(viewInfo, event.m_y, mRect.y, mRect.height);
            // For persistence of the selection change:
            pProject->ModifyState(false);
            mSelectionBoundary = SBWidth;
            return UpdateSelection;
         }
         else
#endif
         {
            // Not shift-down, choose boundary only within snapping
            double value;
            SelectionBoundary boundary =
               ChooseBoundary(viewInfo, event, mpTrack, mRect, true, true, &value);
            mSelectionBoundary = boundary;
            switch (boundary) {
            case SBNone:
               // startNewSelection remains true
               break;
            case SBLeft:
            case SBRight:
               startNewSelection = false;
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
               // Disable frequency selection
               mFreqSelMode = FREQ_SEL_INVALID;
#endif
               mSelStartValid = true;
               mSelStart = value;
               break;
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
            case SBBottom:
            case SBTop:
            case SBWidth:
               startNewSelection = false;
               // Disable time selection
               mSelStartValid = false;
               mFreqSelTrack = static_cast<const WaveTrack*>(mpTrack);
               mFreqSelPin = value;
               mFreqSelMode =
                  (boundary == SBWidth) ? FREQ_SEL_PINNED_CENTER :
                  (boundary == SBBottom) ? FREQ_SEL_BOTTOM_FREE :
                  FREQ_SEL_TOP_FREE;
               break;
            case SBCenter:
            {
               const auto wt = static_cast<const WaveTrack*>(pTrack);
               HandleCenterFrequencyClick(viewInfo, false, wt, value);
               startNewSelection = false;
               break;
            }
#endif
            default:
               wxASSERT(false);
            }
         }
      } // bAdjustSelectionEdges
   }

   // III. Common case for starting a new selection

   if (startNewSelection) {
      TrackPanel *const trackPanel = pProject->GetTrackPanel();
      // If we didn't move a selection boundary, start a NEW selection
      trackPanel->SelectNone();
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
      StartFreqSelection (viewInfo, event.m_y, mRect.y, mRect.height, mpTrack);
#endif
      StartSelection(pProject, event.m_x, mRect.x);
      trackPanel->SelectTrack(mpTrack, true);
      trackPanel->SetFocusedTrack(mpTrack);
      //On-Demand: check to see if there is an OD thing associated with this track.
      if (mpTrack->GetKind() == Track::Wave) {
         if(ODManager::IsInstanceCreated())
            ODManager::Instance()->DemandTrackUpdate
               (static_cast<WaveTrack*>(mpTrack),mSelStart);
      }

      Connect(pProject);
   }

   return RefreshAll;
}

UIHandle::Result SelectHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   using namespace RefreshCode;

   ViewInfo &viewInfo = pProject->GetViewInfo();
   const wxMouseEvent &event = evt.event;

   int x = mAutoScrolling ? mMostRecentX : event.m_x;
   int y = mAutoScrolling ? mMostRecentY : event.m_y;
   mMostRecentX = x;
   mMostRecentY = y;

   /// AS: If we're dragging to adjust a selection (or actually,
   ///  if the screen is scrolling while you're selecting), we
   ///  handle it here.

   // Also fuhggeddaboudit if we're not dragging and not autoscrolling.
   if (!event.Dragging() && !mAutoScrolling)
      return RefreshNone;

   if (event.CmdDown()) {
      // Ctrl-drag has no meaning, fuhggeddaboudit
      // JKC YES it has meaning.
      //return RefreshNone;
   }

   // Also fuhggeddaboudit if not in a track.
   if (!mpTrack)
      return RefreshNone;

   // JKC: Logic to prevent a selection smaller than 5 pixels to
   // prevent accidental dragging when selecting.
   // (if user really wants a tiny selection, they should zoom in).
   // Can someone make this value of '5' configurable in
   // preferences?
   enum { minimumSizedSelection = 5 }; //measured in pixels

   // Might be dragging frequency bounds only, test
   if (mSelStartValid) {
      wxInt64 SelStart = viewInfo.TimeToPosition(mSelStart, mRect.x); //cvt time to pixels.
      // Abandon this drag if selecting < 5 pixels.
      if (wxLongLong(SelStart - x).Abs() < minimumSizedSelection)
         return RefreshNone;
   }

   Track *clickedTrack =
      static_cast<CommonTrackPanelCell*>(evt.pCell)->FindTrack();

   // Handle which tracks are selected
   Track *sTrack = mpTrack;
   Track *eTrack = clickedTrack;
   if (eTrack && !event.ControlDown()) {
      auto trackPanel = pProject->GetTrackPanel();
      trackPanel->SelectRangeOfTracks(sTrack, eTrack);
   }

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
#ifndef SPECTRAL_EDITING_ESC_KEY
   if (mFreqSelMode == FREQ_SEL_SNAPPING_CENTER &&
      !viewInfo.selectedRegion.isPoint())
      MoveSnappingFreqSelection(viewInfo, y, mRect.y, mRect.height, mpTrack);
   else
#endif
      if (mFreqSelTrack == mpTrack)
         AdjustFreqSelection(viewInfo, y, mRect.y, mRect.height);
#endif

   AdjustSelection(viewInfo, x, mRect.x, clickedTrack);

   // If scrubbing does not use the helper poller thread, then
   // don't refresh at every mouse event, because it slows down seek-scrub.
   // Instead, let OnTimer do it, which is often enough.
   // And even if scrubbing does use the thread, then skipping refresh does not
   // bring that advantage, but it is probably still a good idea anyway.
   return RefreshNone;
}

HitTestPreview SelectHandle::Preview
(const TrackPanelMouseEvent &, const AudacityProject *)
{
   wxString tip;
   wxCursor *pCursor;
   SetTipAndCursorForBoundary
      (SelectionBoundary(mSelectionBoundary),
       (mFreqSelMode == FREQ_SEL_SNAPPING_CENTER),
       tip, pCursor);
   return HitTestPreview(tip, pCursor);
}

UIHandle::Result SelectHandle::Release
(const TrackPanelMouseEvent &, AudacityProject *pProject,
 wxWindow *)
{
   using namespace RefreshCode;
   Disconnect();
   pProject->ModifyState(false);
   if (mSnapLeft != -1 || mSnapRight != -1)
      return RefreshAll;
   else
      return RefreshNone;
}

UIHandle::Result SelectHandle::Cancel(AudacityProject *pProject)
{
   Disconnect();

   TrackListIterator iter(pProject->GetTracks());
   std::vector<bool>::const_iterator
      it = mInitialTrackSelection.begin(),
      end = mInitialTrackSelection.end();
   for (Track *t = iter.First(); t; t = iter.Next()) {
      wxASSERT(it != end);
      t->SetSelected(*it++);
   }
   pProject->GetViewInfo().selectedRegion = mInitialSelection;

   // Refresh mixer board for change of set of selected tracks
   if (MixerBoard* pMixerBoard = pProject->GetMixerBoard())
      pMixerBoard->Refresh();

   return RefreshCode::RefreshAll;
}

void SelectHandle::DrawExtras
(DrawingPass pass, wxDC * dc, const wxRegion &, const wxRect &)
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

void SelectHandle::OnProjectChange(AudacityProject *pProject)
{
   if (! pProject->GetTracks()->Contains(mpTrack)) {
      mpTrack = nullptr;
      mRect = {};
   }

   if (! pProject->GetTracks()->Contains(mFreqSelTrack)) {
      mFreqSelTrack = nullptr;
   }

   UIHandle::OnProjectChange(pProject);
}

void SelectHandle::Connect(AudacityProject *pProject)
{
   mConnectedProject = pProject;
   mConnectedProject->Connect(EVT_TRACK_PANEL_TIMER,
      wxCommandEventHandler(SelectHandle::OnTimer),
      NULL,
      this);
}

void SelectHandle::Disconnect()
{
   if (mConnectedProject)
      mConnectedProject->Disconnect(EVT_TRACK_PANEL_TIMER,
         wxCommandEventHandler(SelectHandle::OnTimer),
         NULL,
         this);
   mConnectedProject = NULL;

   mpTrack = 0;
   mFreqSelTrack = nullptr;

   mSnapManager.reset(NULL);

   mFreqSelMode = FREQ_SEL_INVALID;
}

void SelectHandle::OnTimer(wxCommandEvent &event)
{
   event.Skip();

   // AS: If the user is dragging the mouse and there is a track that
   //  has captured the mouse, then scroll the screen, as necessary.

   ///  We check on each timer tick to see if we need to scroll.
   ///  Scrolling is handled by mListener, which is an interface
   ///  to the window TrackPanel is embedded in.

   // DM: If we're "autoscrolling" (which means that we're scrolling
   //  because the user dragged from inside to outside the window,
   //  not because the user clicked in the scroll bar), then
   //  the selection code needs to be handled slightly differently.
   //  We set this flag ("mAutoScrolling") to tell the selecting
   //  code that we didn't get here as a result of a mouse event,
   //  and therefore it should ignore the mouseEvent parameter,
   //  and instead use the last known mouse position.  Setting
   //  this flag also causes the Mac to redraw immediately rather
   //  than waiting for the next update event; this makes scrolling
   //  smoother on MacOS 9.

   AudacityProject *const project = mConnectedProject;
   if (mMostRecentX >= mRect.x + mRect.width) {
      mAutoScrolling = true;
      project->TP_ScrollRight();
   }
   else if (mMostRecentX < mRect.x) {
      mAutoScrolling = true;
      project->TP_ScrollLeft();
   }
   else {
      // Bug1387:  enable autoscroll during drag, if the pointer is at either extreme x
      // coordinate of the screen, even if that is still within the track area.

      int xx = mMostRecentX, yy = 0;
      project->GetTrackPanel()->ClientToScreen(&xx, &yy);
      if (xx == 0) {
         mAutoScrolling = true;
         project->TP_ScrollLeft();
      }
      else {
         int width, height;
         ::wxDisplaySize(&width, &height);
         if (xx == width - 1) {
            mAutoScrolling = true;
            project->TP_ScrollRight();
         }
      }
   }

   if (mAutoScrolling && mpTrack) {
      // AS: To keep the selection working properly as we scroll,
      //  we fake a mouse event (remember, this method is called
      //  from a timer tick).

      // AS: For some reason, GCC won't let us pass this directly.
      wxMouseEvent evt(wxEVT_MOTION);
      Drag(TrackPanelMouseEvent(evt, mRect, mpTrack), project);
      mAutoScrolling = false;
      mConnectedProject->GetTrackPanel()->Refresh(false);
   }
}

/// Reset our selection markers.
void SelectHandle::StartSelection
   (AudacityProject *pProject, int mouseXCoordinate, int trackLeftEdge)
{
   ViewInfo &viewInfo = pProject->GetViewInfo();
   mSelStartValid = true;
   mSelStart = std::max(0.0, viewInfo.PositionToTime(mouseXCoordinate, trackLeftEdge));

   double s = mSelStart;

   if (mSnapManager.get()) {
      mSnapLeft = -1;
      mSnapRight = -1;
      bool snappedPoint, snappedTime;
      if (mSnapManager->Snap(mpTrack, mSelStart, false,
         &s, &snappedPoint, &snappedTime)) {
         if (snappedPoint)
            mSnapLeft = viewInfo.TimeToPosition(s, trackLeftEdge);
      }
   }

   viewInfo.selectedRegion.setTimes(s, s);

   // SonifyBeginModifyState();
   pProject->ModifyState(false);
   // SonifyEndModifyState();
}

/// Extend or contract the existing selection
void SelectHandle::AdjustSelection
(ViewInfo &viewInfo, int mouseXCoordinate, int trackLeftEdge,
 Track *pTrack)
{
   if (!mSelStartValid)
      // Must be dragging frequency bounds only.
      return;

   const double selend =
      std::max(0.0, viewInfo.PositionToTime(mouseXCoordinate, trackLeftEdge));
   double origSel0, origSel1;
   double sel0, sel1;

   if (pTrack == NULL && mpTrack != NULL)
      pTrack = mpTrack;

   if (mSelStart < selend) {
      sel0 = mSelStart;
      sel1 = selend;
   }
   else {
      sel1 = mSelStart;
      sel0 = selend;
   }

   origSel0 = sel0;
   origSel1 = sel1;

   if (mSnapManager.get()) {
      mSnapLeft = -1;
      mSnapRight = -1;
      bool snappedPoint, snappedTime;
      if (mpTrack &&
          mSnapManager->Snap(mpTrack, sel0, false,
                             &sel0, &snappedPoint, &snappedTime)) {
         if (snappedPoint)
            mSnapLeft = viewInfo.TimeToPosition(sel0, trackLeftEdge);
      }
      if (mpTrack &&
          mSnapManager->Snap(mpTrack, sel1, true,
                             &sel1, &snappedPoint, &snappedTime)) {
         if (snappedPoint)
            mSnapRight = viewInfo.TimeToPosition(sel1, trackLeftEdge);
      }

      // Check if selection endpoints are too close together to snap (unless
      // using snap-to-time -- then we always accept the snap results)
      if (mSnapLeft >= 0 && mSnapRight >= 0 && mSnapRight - mSnapLeft < 3 &&
            !snappedTime) {
         sel0 = origSel0;
         sel1 = origSel1;
         mSnapLeft = -1;
         mSnapRight = -1;
      }
   }

   viewInfo.selectedRegion.setTimes(sel0, sel1);

   //On-Demand: check to see if there is an OD thing associated with this track.  If so we want to update the focal point for the task.
   if (pTrack && (pTrack->GetKind() == Track::Wave) && ODManager::IsInstanceCreated())
      ODManager::Instance()->DemandTrackUpdate
         (static_cast<WaveTrack*>(pTrack),sel0); //sel0 is sometimes less than mSelStart
}

void SelectHandle::StartFreqSelection(ViewInfo &viewInfo,
   int mouseYCoordinate, int trackTopEdge,
   int trackHeight, Track *pTrack)
{
   mFreqSelTrack = NULL;
   mFreqSelMode = FREQ_SEL_INVALID;
   mFreqSelPin = SelectedRegion::UndefinedFrequency;

   if (isSpectralSelectionTrack(pTrack)) {
      // Spectral selection track is always wave
      mFreqSelTrack = static_cast<WaveTrack*>(pTrack);
      mFreqSelMode = FREQ_SEL_FREE;
      mFreqSelPin =
         PositionToFrequency(mFreqSelTrack, false, mouseYCoordinate,
         trackTopEdge, trackHeight);
      viewInfo.selectedRegion.setFrequencies(mFreqSelPin, mFreqSelPin);
   }
}

void SelectHandle::AdjustFreqSelection(ViewInfo &viewInfo,
   int mouseYCoordinate, int trackTopEdge,
   int trackHeight)
{
   if (mFreqSelMode == FREQ_SEL_INVALID ||
       mFreqSelMode == FREQ_SEL_SNAPPING_CENTER)
      return;

   // Extension happens only when dragging in the same track in which we
   // started, and that is of a spectrogram display type.

   const WaveTrack *const wt = mFreqSelTrack;
   const double rate =  wt->GetRate();
   const double frequency =
      PositionToFrequency(wt, true, mouseYCoordinate,
         trackTopEdge, trackHeight);

   // Dragging center?
   if (mFreqSelMode == FREQ_SEL_DRAG_CENTER) {
      if (frequency == rate || frequency < 1.0)
         // snapped to top or bottom
         viewInfo.selectedRegion.setFrequencies(
            SelectedRegion::UndefinedFrequency,
            SelectedRegion::UndefinedFrequency);
      else {
         // mFreqSelPin holds the ratio of top to center
         const double maxRatio = findMaxRatio(frequency, rate);
         const double ratio = std::min(maxRatio, mFreqSelPin);
         viewInfo.selectedRegion.setFrequencies(
            frequency / ratio, frequency * ratio);
      }
   }
   else if (mFreqSelMode == FREQ_SEL_PINNED_CENTER) {
      if (mFreqSelPin >= 0) {
         // Change both upper and lower edges leaving centre where it is.
         if (frequency == rate || frequency < 1.0)
            // snapped to top or bottom
            viewInfo.selectedRegion.setFrequencies(
               SelectedRegion::UndefinedFrequency,
               SelectedRegion::UndefinedFrequency);
         else {
            // Given center and mouse position, find ratio of the larger to the
            // smaller, limit that to the frequency scale bounds, and adjust
            // top and bottom accordingly.
            const double maxRatio = findMaxRatio(mFreqSelPin, rate);
            double ratio = frequency / mFreqSelPin;
            if (ratio < 1.0)
               ratio = 1.0 / ratio;
            ratio = std::min(maxRatio, ratio);
            viewInfo.selectedRegion.setFrequencies(
               mFreqSelPin / ratio, mFreqSelPin * ratio);
         }
      }
   }
   else {
      // Dragging of upper or lower.
      const bool bottomDefined =
         !(mFreqSelMode == FREQ_SEL_TOP_FREE && mFreqSelPin < 0);
      const bool topDefined =
         !(mFreqSelMode == FREQ_SEL_BOTTOM_FREE && mFreqSelPin < 0);
      if (!bottomDefined || (topDefined && mFreqSelPin < frequency)) {
         // Adjust top
         if (frequency == rate)
            // snapped high; upper frequency is undefined
            viewInfo.selectedRegion.setF1(SelectedRegion::UndefinedFrequency);
         else
            viewInfo.selectedRegion.setF1(std::max(1.0, frequency));

         viewInfo.selectedRegion.setF0(mFreqSelPin);
      }
      else {
         // Adjust bottom
         if (frequency < 1.0)
            // snapped low; lower frequency is undefined
            viewInfo.selectedRegion.setF0(SelectedRegion::UndefinedFrequency);
         else
            viewInfo.selectedRegion.setF0(std::min(rate / 2.0, frequency));

         viewInfo.selectedRegion.setF1(mFreqSelPin);
      }
   }
}

void SelectHandle::HandleCenterFrequencyClick
(const ViewInfo &viewInfo, bool shiftDown, const WaveTrack *pTrack, double value)
{
   if (shiftDown) {
      // Disable time selection
      mSelStartValid = false;
      mFreqSelTrack = pTrack;
      mFreqSelPin = value;
      mFreqSelMode = FREQ_SEL_DRAG_CENTER;
   }
   else {
#ifndef SPECTRAL_EDITING_ESC_KEY
      // Start center snapping
      // Turn center snapping on (the only way to do this)
      mFreqSelMode = FREQ_SEL_SNAPPING_CENTER;
      // Disable time selection
      mSelStartValid = false;
      StartSnappingFreqSelection(viewInfo, pTrack);
#endif
   }
}

void SelectHandle::StartSnappingFreqSelection
   (const ViewInfo &viewInfo, const WaveTrack *pTrack)
{
   static const size_t minLength = 8;

   const double rate = pTrack->GetRate();

   // Grab samples, just for this track, at these times
   std::vector<float> frequencySnappingData;
   const auto start =
      pTrack->TimeToLongSamples(viewInfo.selectedRegion.t0());
   const auto end =
      pTrack->TimeToLongSamples(viewInfo.selectedRegion.t1());
   const auto length =
      std::min(frequencySnappingData.max_size(),
         limitSampleBufferSize(10485760, // as in FreqWindow.cpp
            end - start));
   const auto effectiveLength = std::max(minLength, length);
   frequencySnappingData.resize(effectiveLength, 0.0f);
   pTrack->Get(
      reinterpret_cast<samplePtr>(&frequencySnappingData[0]),
      floatSample, start, length, fillZero,
      // Don't try to cope with exceptions, just read zeroes instead.
      false);

   // Use same settings as are now used for spectrogram display,
   // except, shrink the window as needed so we get some answers

   const SpectrogramSettings &settings = pTrack->GetSpectrogramSettings();
   auto windowSize = settings.GetFFTLength();

   while(windowSize > effectiveLength)
      windowSize >>= 1;
   const int windowType = settings.windowType;

   mFrequencySnapper->Calculate(
      SpectrumAnalyst::Spectrum, windowType, windowSize, rate,
      &frequencySnappingData[0], length);

   // We can now throw away the sample data but we keep the spectrum.
}

void SelectHandle::MoveSnappingFreqSelection
   (ViewInfo &viewInfo, int mouseYCoordinate,
    int trackTopEdge,
    int trackHeight, Track *pTrack)
{
   if (pTrack &&
      pTrack->GetSelected() &&
      isSpectralSelectionTrack(pTrack)) {
      // Spectral selection track is always wave
      WaveTrack *const wt = static_cast<WaveTrack*>(pTrack);
      // PRL:
      // What would happen if center snapping selection began in one spectrogram track,
      // then continues inside another?  We do not then recalculate
      // the spectrum (as was done in StartSnappingFreqSelection)
      // but snap according to the peaks in the old track.

      // But if we always supply the original clicked track here that doesn't matter.
      const double rate = wt->GetRate();
      const double frequency =
         PositionToFrequency(wt, false, mouseYCoordinate,
         trackTopEdge, trackHeight);
      const double snappedFrequency =
         mFrequencySnapper->FindPeak(frequency, NULL);
      const double maxRatio = findMaxRatio(snappedFrequency, rate);
      double ratio = 2.0; // An arbitrary octave on each side, at most
      {
         const double f0 = viewInfo.selectedRegion.f0();
         const double f1 = viewInfo.selectedRegion.f1();
         if (f1 >= f0 && f0 >= 0)
            // Preserve already chosen ratio instead
            ratio = sqrt(f1 / f0);
      }
      ratio = std::min(ratio, maxRatio);

      mFreqSelPin = snappedFrequency;
      viewInfo.selectedRegion.setFrequencies(
         snappedFrequency / ratio, snappedFrequency * ratio);

      // A change here would affect what AdjustFreqSelection() does
      // in the prototype version where you switch from moving center to
      // dragging width with a click.  No effect now.
      mFreqSelTrack = wt;

      // SelectNone();
      // SelectTrack(pTrack, true);
      //SetFocusedTrack(pTrack);
   }
}

void SelectHandle::SnapCenterOnce
   (ViewInfo &viewInfo, const WaveTrack *pTrack, bool up)
{
   const SpectrogramSettings &settings = pTrack->GetSpectrogramSettings();
   const auto windowSize = settings.GetFFTLength();
   const double rate = pTrack->GetRate();
   const double nyq = rate / 2.0;
   const double binFrequency = rate / windowSize;

   double f1 = viewInfo.selectedRegion.f1();
   double centerFrequency = viewInfo.selectedRegion.fc();
   if (centerFrequency <= 0) {
      centerFrequency = up ? binFrequency : nyq;
      f1 = centerFrequency * sqrt(2.0);
   }

   double ratio = f1 / centerFrequency;
   const int originalBin = floor(0.5 + centerFrequency / binFrequency);
   const int limitingBin = up ? floor(0.5 + nyq / binFrequency) : 1;

   // This is crude and wasteful, doing the FFT each time the command is called.
   // It would be better to cache the data, but then invalidation of the cache would
   // need doing in all places that change the time selection.
   StartSnappingFreqSelection(viewInfo, pTrack);
   double snappedFrequency = centerFrequency;
   int bin = originalBin;
   if (up) {
      while (snappedFrequency <= centerFrequency &&
         bin < limitingBin)
         snappedFrequency = mFrequencySnapper->FindPeak(++bin * binFrequency, NULL);
   }
   else {
      while (snappedFrequency >= centerFrequency &&
         bin > limitingBin)
         snappedFrequency = mFrequencySnapper->FindPeak(--bin * binFrequency, NULL);
   }

   const double maxRatio = findMaxRatio(snappedFrequency, rate);
   ratio = std::min(ratio, maxRatio);

   viewInfo.selectedRegion.setFrequencies
      (snappedFrequency / ratio, snappedFrequency * ratio);
}

#if 0
// unused
void SelectHandle::ResetFreqSelectionPin
   (const ViewInfo &viewInfo, double hintFrequency, bool logF)
{
   switch (mFreqSelMode) {
   case FREQ_SEL_INVALID:
   case FREQ_SEL_SNAPPING_CENTER:
      mFreqSelPin = -1.0;
      break;

   case FREQ_SEL_PINNED_CENTER:
      mFreqSelPin = viewInfo.selectedRegion.fc();
      break;

   case FREQ_SEL_DRAG_CENTER:
   {
      // Re-pin the width
      const double f0 = viewInfo.selectedRegion.f0();
      const double f1 = viewInfo.selectedRegion.f1();
      if (f0 >= 0 && f1 >= 0)
         mFreqSelPin = sqrt(f1 / f0);
      else
         mFreqSelPin = -1.0;
   }
   break;

   case FREQ_SEL_FREE:
      // Pin which?  Farther from the hint which is the presumed
      // mouse position.
   {
      // If this function finds use again, the following should be
      // generalized using NumberScale

      const double f0 = viewInfo.selectedRegion.f0();
      const double f1 = viewInfo.selectedRegion.f1();
      if (logF) {
         if (f1 < 0)
            mFreqSelPin = f0;
         else {
            const double logf1 = log(std::max(1.0, f1));
            const double logf0 = log(std::max(1.0, f0));
            const double logHint = log(std::max(1.0, hintFrequency));
            if (std::abs(logHint - logf1) < std::abs(logHint - logf0))
               mFreqSelPin = f0;
            else
               mFreqSelPin = f1;
         }
      }
      else {
         if (f1 < 0 ||
            std::abs(hintFrequency - f1) < std::abs(hintFrequency - f0))
            mFreqSelPin = f0;
         else
            mFreqSelPin = f1;
      }
   }
   break;

   case FREQ_SEL_TOP_FREE:
      mFreqSelPin = viewInfo.selectedRegion.f0();
      break;

   case FREQ_SEL_BOTTOM_FREE:
      mFreqSelPin = viewInfo.selectedRegion.f1();
      break;

   default:
      wxASSERT(false);
   }
}
#endif
