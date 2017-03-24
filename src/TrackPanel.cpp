/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackPanel.cpp

  Dominic Mazzoni
  and lots of other contributors

  Implements TrackPanel and TrackInfo.

********************************************************************//*!

\todo
  Refactoring of the TrackPanel, possibly as described
  in \ref TrackPanelRefactor

*//*****************************************************************//*!

\file TrackPanel.cpp
\brief
  Implements TrackPanel and TrackInfo.

  TrackPanel.cpp is currently some of the worst code in Audacity.
  It's not really unreadable, there's just way too much stuff in this
  one file.  Rather than apply a quick fix, the long-term plan
  is to create a GUITrack class that knows how to draw itself
  and handle events.  Then this class just helps coordinate
  between tracks.

  Plans under discussion are described in \ref TrackPanelRefactor

*//********************************************************************/

// Documentation: Rather than have a lengthy \todo section, having
// a \todo a \file and a \page in EXACTLY that order gets Doxygen to
// put the following lengthy description of refactoring on a NEW page
// and link to it from the docs.

/*****************************************************************//**

\class TrackPanel
\brief
  The TrackPanel class coordinates updates and operations on the
  main part of the screen which contains multiple tracks.

  It uses many other classes, but in particular it uses the
  TrackInfo class to draw the controls area on the left of a track,
  and the TrackArtist class to draw the actual waveforms.

  Note that in some of the older code here, e.g., GetLabelWidth(),
  "Label" means the TrackInfo plus the vertical ruler.
  Confusing relative to LabelTrack labels.

  The TrackPanel manages multiple tracks and their TrackInfos.

  Note that with stereo tracks there will be one TrackInfo
  being used by two wavetracks.

*//*****************************************************************//**

\class TrackInfo
\brief
  The TrackInfo is shown to the side of a track
  It has the menus, pan and gain controls displayed in it.
  So "Info" is somewhat a misnomer. Should possibly be "TrackControls".

  TrackPanel and not TrackInfo takes care of the functionality for
  each of the buttons in that panel.

  In its current implementation TrackInfo is not derived from a
  wxWindow.  Following the original coding style, it has
  been coded as a 'flyweight' class, which is passed
  state as needed, except for the array of gains and pans.

  If we'd instead coded it as a wxWindow, we would have an instance
  of this class for each instance displayed.

*//**************************************************************//**

\class TrackPanelListener
\brief A now badly named class which is used to give access to a
subset of the TrackPanel methods from all over the place.

*//**************************************************************//**

\class TrackList
\brief A list of TrackListNode items.

*//**************************************************************//**

\class TrackListIterator
\brief An iterator for a TrackList.

*//**************************************************************//**

\class TrackListNode
\brief Used by TrackList, points to a Track.

*//**************************************************************//**

\class TrackPanel::AudacityTimer
\brief Timer class dedicated to infomring the TrackPanel that it
is time to refresh some aspect of the screen.

*//*****************************************************************//**

\page TrackPanelRefactor Track Panel Refactor
\brief Planned refactoring of TrackPanel.cpp

 - Move menus from current TrackPanel into TrackInfo.
 - Convert TrackInfo from 'flyweight' to heavyweight.
 - Split GuiStereoTrack and GuiWaveTrack out from TrackPanel.

  JKC: Incremental refactoring started April/2003

  Possibly aiming for Gui classes something like this - it's under
  discussion:

<pre>
   +----------------------------------------------------+
   |      AdornedRulerPanel                             |
   +----------------------------------------------------+
   +----------------------------------------------------+
   |+------------+ +-----------------------------------+|
   ||            | | (L)  GuiWaveTrack                 ||
   || TrackInfo  | +-----------------------------------+|
   ||            | +-----------------------------------+|
   ||            | | (R)  GuiWaveTrack                 ||
   |+------------+ +-----------------------------------+|
   +-------- GuiStereoTrack ----------------------------+
   +----------------------------------------------------+
   |+------------+ +-----------------------------------+|
   ||            | | (L)  GuiWaveTrack                 ||
   || TrackInfo  | +-----------------------------------+|
   ||            | +-----------------------------------+|
   ||            | | (R)  GuiWaveTrack                 ||
   |+------------+ +-----------------------------------+|
   +-------- GuiStereoTrack ----------------------------+
</pre>

  With the whole lot sitting in a TrackPanel which forwards
  events to the sub objects.

  The GuiStereoTrack class will do the special logic for
  Stereo channel grouping.

  The precise names of the classes are subject to revision.
  Have deliberately not created NEW files for the NEW classes
  such as AdornedRulerPanel and TrackInfo - yet.

*//*****************************************************************/

#include "Audacity.h"
#include "Experimental.h"
#include "TrackPanel.h"
#include "Project.h"
#include "TrackPanelCell.h"
#include "TrackPanelCellIterator.h"
#include "TrackPanelMouseEvent.h"
#include "TrackPanelResizeHandle.h"
//#define DEBUG_DRAW_TIMING 1

#include "AColor.h"
#include "AllThemeResources.h"
#include "AudioIO.h"
#include "float_cast.h"
#include "MixerBoard.h"

#include "Prefs.h"
#include "RefreshCode.h"
#include "TrackArtist.h"
#include "TrackPanelAx.h"
#include "UndoManager.h"
#include "UIHandle.h"
#include "HitTestResult.h"
#include "WaveTrack.h"
#include "NoteTrack.h"

#include "toolbars/ControlToolBar.h"
#include "toolbars/ToolsToolBar.h"

//This loads the appropriate set of cursors, depending on platform.
#include "../images/Cursors.h"

#include "widgets/ASlider.h"
#include "widgets/Ruler.h"
#include <algorithm>

DEFINE_EVENT_TYPE(EVT_TRACK_PANEL_TIMER)

/*

This is a diagram of TrackPanel's division of one (non-stereo) track rectangle.
Total height equals Track::GetHeight()'s value.  Total width is the wxWindow's width.
Each charater that is not . represents one pixel.

Inset space of this track, and top inset of the next track, are used to draw the focus highlight.

Top inset of the right channel of a stereo track, and bottom shadow line of the
left channel, are used for the channel separator.

TrackInfo::GetTrackInfoWidth() == GetVRulerOffset()
counts columns from the left edge up to and including controls, and is a constant.

GetVRulerWidth() is variable -- all tracks have the same ruler width at any time,
but that width may be adjusted when tracks change their vertical scales.

GetLabelWidth() counts columns up to and including the VRuler.
GetLeftOffset() is yet one more -- it counts the "one pixel" column.

FindCell() for label or vruler returns a rectangle up to and including the One Pixel column,
but OMITS left and top insets

FindCell() for track returns a rectangle with x == GetLeftOffset(), and INCLUDES
right and top insets

+--------------- ... ------ ... --------------------- ...       ... -------------+
| Top Inset                                                                      |
|                                                                                |
|  +------------ ... ------ ... --------------------- ...       ... ----------+  |
| L|+-Border---- ... ------ ... --------------------- ...       ... -Border-+ |R |
| e||+---------- ... -++--- ... -+++----------------- ...       ... -------+| |i |
| f|B|                ||         |||                                       |BS|g |
| t|o| Controls       || V       |O|  The good stuff                       |oh|h |
|  |r|                || R       |n|                                       |ra|t |
| I|d|                || u       |e|                                       |dd|  |
| n|e|                || l       | |                                       |eo|I |
| s|r|                || e       |P|                                       |rw|n |
| e|||                || r       |i|                                       ||||s |
| t|||                ||         |x|                                       ||||e |
|  |||                ||         |e|                                       ||||t |
|  |||                ||         |l|                                       ||||  |
|  |||                ||         |||                                       ||||  |

.  ...                ..         ...                                       ....  .
.  ...                ..         ...                                       ....  .
.  ...                ..         ...                                       ....  .

|  |||                ||         |||                                       ||||  |
|  ||+----------     -++--  ... -+++----------------- ...       ... -------+|||  |
|  |+-Border---- ... -----  ... --------------------- ...       ... -Border-+||  |
|  |  Shadow---- ... -----  ... --------------------- ...       ... --Shadow-+|  |
*/

// Is the distance between A and B less than D?
template < class A, class B, class DIST > bool within(A a, B b, DIST d)
{
   return (a > b - d) && (a < b + d);
}

BEGIN_EVENT_TABLE(TrackPanel, OverlayPanel)
    EVT_MOUSE_EVENTS(TrackPanel::OnMouseEvent)
    EVT_MOUSE_CAPTURE_LOST(TrackPanel::OnCaptureLost)
    EVT_COMMAND(wxID_ANY, EVT_CAPTURE_KEY, TrackPanel::OnCaptureKey)
    EVT_KEY_DOWN(TrackPanel::OnKeyDown)
    EVT_KEY_UP(TrackPanel::OnKeyUp)
    EVT_CHAR(TrackPanel::OnChar)
    EVT_PAINT(TrackPanel::OnPaint)
    EVT_SET_FOCUS(TrackPanel::OnSetFocus)
    EVT_KILL_FOCUS(TrackPanel::OnKillFocus)
    EVT_CONTEXT_MENU(TrackPanel::OnContextMenu)

    EVT_TIMER(wxID_ANY, TrackPanel::OnTimer)
END_EVENT_TABLE()

/// Makes a cursor from an XPM, uses CursorId as a fallback.
/// TODO:  Move this function to some other source file for reuse elsewhere.
std::unique_ptr<wxCursor> MakeCursor( int WXUNUSED(CursorId), const char * pXpm[36],  int HotX, int HotY )
{
#ifdef CURSORS_SIZE32
   const int HotAdjust =0;
#else
   const int HotAdjust =8;
#endif

   wxImage Image = wxImage(wxBitmap(pXpm).ConvertToImage());
   Image.SetMaskColour(255,0,0);
   Image.SetMask();// Enable mask.

   Image.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_X, HotX-HotAdjust );
   Image.SetOption( wxIMAGE_OPTION_CUR_HOTSPOT_Y, HotY-HotAdjust );
   return std::make_unique<wxCursor>( Image );
}



// Don't warn us about using 'this' in the base member initializer list.
#ifndef __WXGTK__ //Get rid if this pragma for gtk
#pragma warning( disable: 4355 )
#endif
TrackPanel::TrackPanel(wxWindow * parent, wxWindowID id,
                       const wxPoint & pos,
                       const wxSize & size,
                       const std::shared_ptr<TrackList> &tracks,
                       ViewInfo * viewInfo,
                       TrackPanelListener * listener,
                       AdornedRulerPanel * ruler)
   : OverlayPanel(parent, id, pos, size, wxWANTS_CHARS | wxNO_BORDER),
     mTrackInfo(this),
     mListener(listener),
     mTracks(tracks),
     mViewInfo(viewInfo),
     mRuler(ruler),
     mTrackArtist(nullptr),
     mRefreshBacking(false),
     vrulerSize(36,0)
     , mpClickedTrack(NULL)
     , mUIHandle(NULL)
     , mpBackground(NULL)
#ifndef __WXGTK__   //Get rid if this pragma for gtk
#pragma warning( default: 4355 )
#endif
{
   SetLabel(_("Track Panel"));
   SetName(_("Track Panel"));
   SetBackgroundStyle(wxBG_STYLE_PAINT);

   {
      auto pAx = std::make_unique <TrackPanelAx>( this );
#if wxUSE_ACCESSIBILITY
      // wxWidgets owns the accessible object
      SetAccessible(mAx = pAx.release());
#else
      // wxWidgets does not own the object, but we need to retain it
      mAx = std::move(pAx);
#endif
   }

   mCircularTrackNavigation = false;


   mRedrawAfterStop = false;

   mTrackArtist = std::make_unique<TrackArtist>();
   mTrackArtist->SetInset(1, kTopMargin, kRightMargin, kBottomMargin);

   mTimeCount = 0;
   mTimer.parent = this;
   // Timer is started after the window is visible
   GetProject()->Bind(wxEVT_IDLE, &TrackPanel::OnIdle, this);

   //More initializations, some of these for no other reason than
   //to prevent runtime memory check warnings
   mPrevWidth = -1;
   mPrevHeight = -1;

   // Register for tracklist updates
   mTracks->Connect(EVT_TRACKLIST_RESIZED,
                    wxCommandEventHandler(TrackPanel::OnTrackListResized),
                    NULL,
                    this);
   mTracks->Connect(EVT_TRACKLIST_UPDATED,
                    wxCommandEventHandler(TrackPanel::OnTrackListUpdated),
                    NULL,
                    this);

#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   mLastF0 = mLastF1 = SelectedRegion::UndefinedFrequency;
#endif
}


TrackPanel::~TrackPanel()
{
   mTimer.Stop();

   // Unregister for tracklist updates
   mTracks->Disconnect(EVT_TRACKLIST_UPDATED,
                       wxCommandEventHandler(TrackPanel::OnTrackListUpdated),
                       NULL,
                       this);
   mTracks->Disconnect(EVT_TRACKLIST_RESIZED,
                       wxCommandEventHandler(TrackPanel::OnTrackListResized),
                       NULL,
                       this);

   // This can happen if a label is being edited and the user presses
   // ALT+F4 or Command+Q
   if (HasCapture())
      ReleaseMouse();
}

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
void TrackPanel::UpdateVirtualStereoOrder()
{
   TrackListOfKindIterator iter(Track::Wave, GetTracks());
   Track *t;
   int temp;

   for (t = iter.First(); t; t = iter.Next()) {
      const auto wt = static_cast<WaveTrack*>(t);
      if(t->GetChannel() == Track::MonoChannel){

         if(WaveTrack::mMonoAsVirtualStereo && wt->GetPan() != 0){
            temp = wt->GetHeight();
            wt->SetHeight(temp*wt->GetVirtualTrackPercentage());
            wt->SetHeight(temp - wt->GetHeight(),true);
         }else if(!WaveTrack::mMonoAsVirtualStereo && wt->GetPan() != 0){
            wt->SetHeight(wt->GetHeight() + wt->GetHeight(true));
         }
      }
   }
   t = iter.First();
   if(t){
      t->ReorderList(false);
   }
}
#endif

void TrackPanel::UpdatePrefs()
{
   gPrefs->Read(wxT("/GUI/AutoScroll"), &mViewInfo->bUpdateTrackIndicator,
      true);
   gPrefs->Read(wxT("/GUI/CircularTrackNavigation"), &mCircularTrackNavigation,
      false);
   gPrefs->Read(wxT("/GUI/Solo"), &mSoloPref, wxT("Simple"));

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   bool temp = WaveTrack::mMonoAsVirtualStereo;
   gPrefs->Read(wxT("/GUI/MonoAsVirtualStereo"), &WaveTrack::mMonoAsVirtualStereo,
      false);

   if(WaveTrack::mMonoAsVirtualStereo != temp)
      UpdateVirtualStereoOrder();
#endif

   mViewInfo->UpdatePrefs();

   if (mTrackArtist) {
      mTrackArtist->UpdatePrefs();
   }

   // All vertical rulers must be recalculated since the minimum and maximum
   // frequences may have been changed.
   UpdateVRulers();

   mTrackInfo.UpdatePrefs();

   Refresh();
}

void TrackPanel::SelectNone()
{
   TrackListIterator iter(GetTracks());
   Track *t = iter.First();
   while (t) {
      SelectTrack(t, false, false);
      t = iter.Next();
   }
}

// Set selection length to the length of a track -- but if sync-lock is turned
// on, use the largest possible selection in the sync-lock group.
// If it's a stereo track, do the same for the stereo channels.
void TrackPanel::SelectTrackLength(Track *t)
{
   AudacityProject *p = GetActiveProject();
   SyncLockedTracksIterator it(GetTracks());
   Track *t1 = it.StartWith(t);
   double minOffset = t->GetOffset();
   double maxEnd = t->GetEndTime();

   // If we have a sync-lock group and sync-lock linking is on,
   // check the sync-lock group tracks.
   if (p->IsSyncLocked() && t1 != NULL)
   {
      for ( ; t1; t1 = it.Next())
      {
         if (t1->GetOffset() < minOffset)
            minOffset = t1->GetOffset();
         if (t1->GetEndTime() > maxEnd)
            maxEnd = t1->GetEndTime();
      }
   }
   else
   {
      // Otherwise, check for a stereo pair
      t1 = t->GetLink();
      if (t1)
      {
         if (t1->GetOffset() < minOffset)
            minOffset = t1->GetOffset();
         if (t1->GetEndTime() > maxEnd)
            maxEnd = t1->GetEndTime();
      }
   }

   // PRL: double click or click on track control.
   // should this select all frequencies too?  I think not.
   mViewInfo->selectedRegion.setTimes(minOffset, maxEnd);
}

void TrackPanel::GetTracksUsableArea(int *width, int *height) const
{
   GetSize(width, height);
   if (width) {
      *width -= GetLeftOffset();
      *width -= kRightMargin;
      *width = std::max(0, *width);
   }
}

/// Gets the pointer to the AudacityProject that
/// goes with this track panel.
AudacityProject * TrackPanel::GetProject() const
{
   //JKC casting away constness here.
   //Do it in two stages in case 'this' is not a wxWindow.
   //when the compiler will flag the error.
   wxWindow const * const pConstWind = this;
   wxWindow * pWind=(wxWindow*)pConstWind;
#ifdef EXPERIMENTAL_NOTEBOOK
   pWind = pWind->GetParent(); //Page
   wxASSERT( pWind );
   pWind = pWind->GetParent(); //Notebook
   wxASSERT( pWind );
#endif
   pWind = pWind->GetParent(); //MainPanel
   wxASSERT( pWind );
   pWind = pWind->GetParent(); //Project
   wxASSERT( pWind );
   return (AudacityProject*)pWind;
}

void TrackPanel::OnIdle(wxIdleEvent& event)
{
   // The window must be ready when the timer fires (#1401)
   if (IsShownOnScreen())
   {
      mTimer.Start(kTimerInterval, FALSE);

      // Timer is started, we don't need the event anymore
      GetProject()->Unbind(wxEVT_IDLE, &TrackPanel::OnIdle, this);
   }
   else
   {
      // Get another idle event, wx only guarantees we get one
      // event after "some other normal events occur"
      event.RequestMore();
   }
}

/// AS: This gets called on our wx timer events.
void TrackPanel::OnTimer(wxTimerEvent& )
{
#ifdef __WXMAC__
   // Unfortunate part of fix for bug 1431
   // Without this, the toolbars hide only every other time that you press
   // the yellow title bar button.  For some reason, not every press sends
   // us a deactivate event for the application.
   {
      auto project = GetProject();
      if (project->IsIconized())
         project->MacShowUndockedToolbars(false);
   }
#endif

   mTimeCount++;

   AudacityProject *const p = GetProject();

   // Check whether we were playing or recording, but the stream has stopped.
   if (p->GetAudioIOToken()>0 && !IsAudioActive())
   {
      //the stream may have been started up after this one finished (by some other project)
      //in that case reset the buttons don't stop the stream
      p->GetControlToolBar()->StopPlaying(!gAudioIO->IsStreamActive());
   }

   // Next, check to see if we were playing or recording
   // audio, but now Audio I/O is completely finished.
   if (p->GetAudioIOToken()>0 &&
         !gAudioIO->IsAudioTokenActive(p->GetAudioIOToken()))
   {
      p->FixScrollbars();
      p->SetAudioIOToken(0);
      p->RedrawProject();

      mRedrawAfterStop = false;

      //ANSWER-ME: Was DisplaySelection added to solve a repaint problem?
      DisplaySelection();
   }
   if (mLastDrawnSelectedRegion != mViewInfo->selectedRegion) {
      UpdateSelectionDisplay();
   }

   // Notify listeners for timer ticks
   {
      wxCommandEvent e(EVT_TRACK_PANEL_TIMER);
      p->GetEventHandler()->ProcessEvent(e);
   }

   DrawOverlays(false);
   mRuler->DrawOverlays(false);

   if(IsAudioActive() && gAudioIO->GetNumCaptureChannels()) {

      // Periodically update the display while recording

      if (!mRedrawAfterStop) {
         mRedrawAfterStop = true;
         MakeParentRedrawScrollbars();
         mListener->TP_ScrollUpDown( 99999999 );
         Refresh( false );
      }
      else {
         if ((mTimeCount % 5) == 0) {
            // Must tell OnPaint() to recreate the backing bitmap
            // since we've not done a full refresh.
            mRefreshBacking = true;
            Refresh( false );
         }
      }
   }
   if(mTimeCount > 1000)
      mTimeCount = 0;
}

double TrackPanel::GetScreenEndTime() const
{
   int width;
   GetTracksUsableArea(&width, NULL);
   return mViewInfo->PositionToTime(width, 0, true);
}

/// AS: OnPaint( ) is called during the normal course of
///  completing a repaint operation.
void TrackPanel::OnPaint(wxPaintEvent & /* event */)
{
   mLastDrawnSelectedRegion = mViewInfo->selectedRegion;

#if DEBUG_DRAW_TIMING
   wxStopWatch sw;
#endif

   {
      wxPaintDC dc(this);

      // Retrieve the damage rectangle
      wxRect box = GetUpdateRegion().GetBox();

      // Recreate the backing bitmap if we have a full refresh
      // (See TrackPanel::Refresh())
      if (mRefreshBacking || (box == GetRect()))
      {
         // Reset (should a mutex be used???)
         mRefreshBacking = false;

         // Redraw the backing bitmap
         DrawTracks(&GetBackingDCForRepaint());

         // Copy it to the display
         DisplayBitmap(dc);
      }
      else
      {
         // Copy full, possibly clipped, damage rectangle
         RepairBitmap(dc, box.x, box.y, box.width, box.height);
      }

      // Done with the clipped DC

      // Drawing now goes directly to the client area.
      // DrawOverlays() may need to draw outside the clipped region.
      // (Used to make a NEW, separate wxClientDC, but that risks flashing
      // problems on Mac.)
      dc.DestroyClippingRegion();
      DrawOverlays(true, &dc);
   }

#if DEBUG_DRAW_TIMING
   sw.Pause();
   wxLogDebug(wxT("Total: %ld milliseconds"), sw.Time());
   wxPrintf(wxT("Total: %ld milliseconds\n"), sw.Time());
#endif
}

void TrackPanel::MakeParentModifyState(bool bWantsAutoSave)
{
   mListener->TP_ModifyState(bWantsAutoSave);
}

void TrackPanel::MakeParentRedrawScrollbars()
{
   mListener->TP_RedrawScrollbars();
}

void TrackPanel::HandleInterruptedDrag()
{
   bool sendEvent = false;

   if (mUIHandle)
      sendEvent = mUIHandle->StopsOnKeystroke();

   if (sendEvent) {
      // The bogus id isn't used anywhere, but may help with debugging.
      // as this is sending a bogus mouse up.  The mouse button is still actually down
      // and may go up again.
      const int idBogusUp = 2;
      wxMouseEvent evt { wxEVT_LEFT_UP };
      evt.SetId( idBogusUp );
      evt.SetPosition(this->ScreenToClient(::wxGetMousePosition()));
      this->ProcessEvent(evt);
   }
}

namespace
{
   void ProcessUIHandleResult
      (TrackPanel *panel, AdornedRulerPanel *ruler,
       Track *pClickedTrack, Track *pLatestTrack,
       UIHandle::Result refreshResult)
   {
      // TODO:  make a finer distinction between refreshing the track control area,
      // and the waveform area.  As it is, redraw both whenever you must redraw either.

      using namespace RefreshCode;

      panel->UpdateViewIfNoTracks();

      if (refreshResult & DestroyedCell) {
         // Beware stale pointer!
         if (pLatestTrack == pClickedTrack)
            pLatestTrack = NULL;
         pClickedTrack = NULL;
      }

      if (pClickedTrack && (refreshResult & UpdateVRuler))
         panel->UpdateVRuler(pClickedTrack);

      if (refreshResult & DrawOverlays) {
         panel->DrawOverlays(false);
         ruler->DrawOverlays(false);
      }

      // Refresh all if told to do so, or if told to refresh a track that
      // is not known.
      const bool refreshAll =
         (    (refreshResult & RefreshAll)
          || ((refreshResult & RefreshCell) && !pClickedTrack)
          || ((refreshResult & RefreshLatestCell) && !pLatestTrack));

      if (refreshAll)
         panel->Refresh(false);
      else {
         if (refreshResult & RefreshCell)
            panel->RefreshTrack(pClickedTrack);
         if (refreshResult & RefreshLatestCell)
            panel->RefreshTrack(pLatestTrack);
      }

      if (refreshResult & FixScrollbars)
         panel->MakeParentRedrawScrollbars();

      if (refreshResult & Resize)
         panel->GetListener()->TP_HandleResize();

      // This flag is superfluouos if you do full refresh,
      // because TrackPanel::Refresh() does this too
      if (refreshResult & UpdateSelection) {
         panel->DisplaySelection();

         {
            // Formerly in TrackPanel::UpdateSelectionDisplay():

            // Make sure the ruler follows suit.
            // mRuler->DrawSelection();

            // ... but that too is superfluous it does nothing but refres
            // the ruler, while DisplaySelection calls TP_DisplaySelection which
            // also always refreshes the ruler.
         }
      }

      if ((refreshResult & EnsureVisible) && pClickedTrack)
         panel->EnsureVisible(pClickedTrack);
   }
}

void TrackPanel::CancelDragging()
{
   if (mUIHandle) {
      UIHandle::Result refreshResult = mUIHandle->Cancel(GetProject());
      {
         // TODO: avoid dangling pointers to mpClickedTrack
         // when the undo stack management of the typical Cancel override
         // causes it to relocate.  That is implement some means to
         // re-fetch the track according to its position in the list.
         mpClickedTrack = NULL;
      }
      ProcessUIHandleResult(this, mRuler, mpClickedTrack, NULL, refreshResult);
      mpClickedTrack = NULL;
      mUIHandle = NULL;
      if (HasCapture())
         ReleaseMouse();
      wxMouseEvent dummy;
      HandleCursor(dummy);
   }
}

bool TrackPanel::HandleEscapeKey(bool down)
{
   if (!down)
      return false;

   mLastPickedTrack = mInitialLastPickedTrack;

   if (mUIHandle) {
      // UIHANDLE CANCEL
      CancelDragging();
      return true;
   }
   
   // Not escaping from a mouse drag
   return false;
}

void TrackPanel::HandleAltKey(bool down)
{
   mLastMouseEvent.m_altDown = down;
   HandleCursorForLastMouseEvent();
}

void TrackPanel::HandleShiftKey(bool down)
{
   mLastMouseEvent.m_shiftDown = down;
   HandleCursorForLastMouseEvent();
}

void TrackPanel::HandleControlKey(bool down)
{
   mLastMouseEvent.m_controlDown = down;
   HandleCursorForLastMouseEvent();
}

void TrackPanel::HandlePageUpKey()
{
   mListener->TP_ScrollWindow(2 * mViewInfo->h - GetScreenEndTime());
}

void TrackPanel::HandlePageDownKey()
{
   mListener->TP_ScrollWindow(GetScreenEndTime());
}

void TrackPanel::HandleCursorForLastMouseEvent()
{
   HandleCursor(mLastMouseEvent);
}

MixerBoard* TrackPanel::GetMixerBoard()
{
   AudacityProject *p = GetProject();
   wxASSERT(p);
   return p->GetMixerBoard();
}

bool TrackPanel::IsAudioActive()
{
   AudacityProject *p = GetProject();
   return p->IsAudioActive();
}


///  TrackPanel::HandleCursor( ) sets the cursor drawn at the mouse location.
///  As this procedure checks which region the mouse is over, it is
///  appropriate to establish the message in the status bar.
void TrackPanel::HandleCursor(wxMouseEvent & event)
{
   mLastMouseEvent = event;

   const auto foundCell = FindCell( event.m_x, event.m_y );
   if (foundCell.pTrack == NULL) return;
   auto &track = foundCell.pTrack;
   auto &inner = foundCell.inner;
   auto &pCell = foundCell.pCell;
   auto &pTrack = foundCell.pTrack;
   wxCursor *pCursor = NULL;

   wxString tip;

   // Are we within the vertical resize area?
   // (Note: add bottom border thickness back to inner rectangle)

   // tip may still be NULL at this point, in which case we go on looking.
   if (within(event.m_y, inner.GetBottom() + kBorderThickness, TRACK_RESIZE_REGION))
   {
      HitTestPreview preview
         (TrackPanelResizeHandle::HitPreview(
            (foundCell.type != CellType::Label) && pTrack->GetLinked()));
      tip = preview.message;
      wxCursor *const pCursor = preview.cursor;
      if (pCursor)
         SetCursor(*pCursor);
   }

   if (pCursor == NULL && tip == wxString()) {
      HitTestResult hitTest(pCell->HitTest
         (TrackPanelMouseEvent{ event, inner, pCell}, GetProject()));
      tip = hitTest.preview.message;
      ProcessUIHandleResult(this, mRuler, pTrack, pTrack, hitTest.preview.refreshCode);
      pCursor = hitTest.preview.cursor;
      if (pCursor)
         SetCursor(*pCursor);
   }

   if (pCursor != NULL || tip != wxString())
      mListener->TP_DisplayStatusMessage(tip);
}

void TrackPanel::UpdateSelectionDisplay()
{
   // Full refresh since the label area may need to indicate
   // newly selected tracks.
   Refresh(false);

   // Make sure the ruler follows suit.
   mRuler->DrawSelection();

   // As well as the SelectionBar.
   DisplaySelection();
}

void TrackPanel::UpdateAccessibility()
{
   if (mAx)
      mAx->Updated();
}

#ifdef EXPERIMENTAL_SPECTRAL_EDITING

void TrackPanel::SelectTrack(Track *pTrack, bool selected, bool updateLastPicked)
{
   bool wasCorrect = (selected == pTrack->GetSelected());

   mTracks->Select(pTrack, selected);
   if (updateLastPicked)
      mLastPickedTrack = pTrack;

//Thw older code below avoids an anchor on an unselected track.

   /*
   if (selected) {
      // This handles the case of linked tracks, selecting all channels
      mTracks->Select(pTrack, true);
      if (updateLastPicked)
         mLastPickedTrack = pTrack;
   }
   else {
      mTracks->Select(pTrack, false);
      if (updateLastPicked && pTrack == mLastPickedTrack)
         mLastPickedTrack = nullptr;
   }
*/

   // Update mixer board, but only as needed so it does not flicker.
   if (!wasCorrect) {
      MixerBoard* pMixerBoard = this->GetMixerBoard();
      if (pMixerBoard && (pTrack->GetKind() == Track::Wave))
         pMixerBoard->RefreshTrackCluster(static_cast<WaveTrack*>(pTrack));
   }
}

// Counts tracks, counting stereo tracks as one track.
size_t TrackPanel::GetTrackCount() const
{
   size_t count = 0;

   TrackListConstIterator iter(GetTracks());
   for (auto t = iter.First(); t; t = iter.Next()) {
      count +=  1;
      if( t->GetLinked() ){
         t = iter.Next();
         if( !t )
            break;
      }
   }
   return count;
}

// Counts selected tracks, counting stereo tracks as one track.
size_t TrackPanel::GetSelectedTrackCount() const
{
   size_t count = 0;

   TrackListConstIterator iter(GetTracks());
   for (auto t = iter.First(); t; t = iter.Next()) {
      count +=  t->GetSelected() ? 1:0;
      if( t->GetLinked() ){
         t = iter.Next();
         if( !t )
            break;
      }
   }
   return count;
}

void TrackPanel::SelectRangeOfTracks(Track *sTrack, Track *eTrack)
{
   if (eTrack) {
      // Swap the track pointers if needed
      if (eTrack->GetIndex() < sTrack->GetIndex())
         std::swap(sTrack, eTrack);

      TrackListIterator iter(GetTracks());
      sTrack = iter.StartWith(sTrack);
      do {
         SelectTrack(sTrack, true, false);
         if (sTrack == eTrack) {
            break;
         }

         sTrack = iter.Next();
      } while (sTrack);
   }
}

void TrackPanel::ChangeSelectionOnShiftClick(Track * pTrack){

   // Optional: Track already selected?  Nothing to do.
   // If we enable this, Shift-Click behaves like click in this case.
   //if( pTrack->GetSelected() )
   //   return;

   // Find first and last selected track.
   Track* pFirst = nullptr;
   Track* pLast = nullptr;
   // We will either extend from the first or from the last.
   Track* pExtendFrom= nullptr;

   if( mLastPickedTrack ){
      pExtendFrom = mLastPickedTrack;
   }
   else
   {
      TrackListIterator iter(GetTracks());
      for (Track *t = iter.First(); t; t = iter.Next()) {
         const bool isSelected = t->GetSelected();
         // Record first and last selected.
         if( isSelected ){
            if( !pFirst )
               pFirst = t;
            pLast = t;
         }
         // If our track is at or after the first, extend from the first.
         if( t == pTrack ){
            pExtendFrom = pFirst;
         }
      }
      // Our track was earlier than the first.  Extend from the last.
      if( !pExtendFrom )
         pExtendFrom = pLast;
   }

   SelectNone();
   if( pExtendFrom )
      SelectRangeOfTracks(pTrack, pExtendFrom);
   else
      SelectTrack( pTrack, true );
   mLastPickedTrack = pExtendFrom;
}

void TrackPanel::ToggleSpectralSelection()
{
   SelectedRegion &region = mViewInfo->selectedRegion;
   const double f0 = region.f0();
   const double f1 = region.f1();
   const bool haveSpectralSelection =
      !(f0 == SelectedRegion::UndefinedFrequency &&
        f1 == SelectedRegion::UndefinedFrequency);
   if (haveSpectralSelection)
   {
      mLastF0 = f0;
      mLastF1 = f1;
      region.setFrequencies
         (SelectedRegion::UndefinedFrequency, SelectedRegion::UndefinedFrequency);
   }
   else
      region.setFrequencies(mLastF0, mLastF1);
}
#endif

void TrackPanel::MessageForScreenReader(const wxString& message)
{
   if (mAx)
      mAx->MessageForScreenReader(message);
}

/// Determines if the a modal tool is active
bool TrackPanel::IsMouseCaptured()
{
   return mUIHandle != NULL;
}

void TrackPanel::UpdateViewIfNoTracks()
{
   if (mTracks->IsEmpty())
   {
      // BG: There are no more tracks on screen
      //BG: Set zoom to normal
      mViewInfo->SetZoom(ZoomInfo::GetDefaultZoom());

      //STM: Set selection to 0,0
      //PRL: and default the rest of the selection information
      mViewInfo->selectedRegion = SelectedRegion();

      // PRL:  Following causes the time ruler to align 0 with left edge.
      // Bug 972
      mViewInfo->h = 0;

      mListener->TP_RedrawScrollbars();
      mListener->TP_HandleResize();
      mListener->TP_DisplayStatusMessage(wxT("")); //STM: Clear message if all tracks are removed
   }
}

// The tracks positions within the list have changed, so update the vertical
// ruler size for the track that triggered the event.
void TrackPanel::OnTrackListResized(wxCommandEvent & e)
{
   Track *t = (Track *) e.GetClientData();
   UpdateVRuler(t);
   e.Skip();
}

// Tracks have been added or removed from the list.  Handle adds as if
// a resize has taken place.
void TrackPanel::OnTrackListUpdated(wxCommandEvent & e)
{
   if (mUIHandle)
      mUIHandle->OnProjectChange(GetProject());

   // Tracks may have been deleted, so check to see if the focused track was on of them.
   if (!mTracks->Contains(GetFocusedTrack())) {
      SetFocusedTrack(NULL);
   }

   if (mLastPickedTrack && !mTracks->Contains(mLastPickedTrack))
      mLastPickedTrack = nullptr;

   if (e.GetClientData()) {
      OnTrackListResized(e);
      return;
   }

   e.Skip();
}

void TrackPanel::OnContextMenu(wxContextMenuEvent & WXUNUSED(event))
{
   OnTrackMenu();
}

void TrackPanel::HandleListSelection(Track *t, bool shift, bool ctrl,
                                     bool modifyState)
{
   // AS: If the shift button is being held down, invert
   //  the selection on this track.
   if (ctrl) {
      SelectTrack(t, !t->GetSelected(), true);
      Refresh(false);
   }
   else {
      if (shift && mLastPickedTrack)
         ChangeSelectionOnShiftClick( t );
      else{
         SelectNone();
         SelectTrack(t, true);
         SelectTrackLength(t);
      }

      SetFocusedTrack(t);
      this->Refresh(false);
      MixerBoard* pMixerBoard = this->GetMixerBoard();
      if (pMixerBoard)
         pMixerBoard->RefreshTrackClusters();
   }

   if (modifyState)
      MakeParentModifyState(true);
}

bool TrackInfo::TrackSelFunc(Track * WXUNUSED(t), wxRect rect, int x, int y)
{
   // First check the blank space to left of minimize button.
   wxRect selRect;
   GetMinimizeRect(rect, selRect); // for y and height
   selRect.x = rect.x;
   selRect.width = 16; // (kTrackInfoBtnSize)
   selRect.height++;
   if (selRect.Contains(x, y))
      return true;

   // Try the sync-lock rect.
   GetSyncLockIconRect(rect, selRect);
   selRect.height++;
   return selRect.Contains(x, y);
}

/// Handle mouse wheel rotation (for zoom in/out, vertical and horizontal scrolling)
void TrackPanel::HandleWheelRotation(wxMouseEvent & event)
{
   double steps {};
#if defined(__WXMAC__) && defined(EVT_MAGNIFY)
   // PRL:
   // Pinch and spread implemented in wxWidgets 3.1.0, or cherry-picked from
   // the future in custom build of 3.0.2
   if (event.Magnify()) {
      event.SetControlDown(true);
      steps = 2 * event.GetMagnification();
   }
   else
#endif
   {
      steps = event.m_wheelRotation /
         (event.m_wheelDelta > 0 ? (double)event.m_wheelDelta : 120.0);
   }

   if(event.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL) {
      // Two-fingered horizontal swipe on mac is treated like shift-mousewheel
      event.SetShiftDown(true);
      // This makes the wave move in the same direction as the fingers, and the scrollbar
      // thumb moves oppositely
      steps *= -1;
   }

   if(!event.HasAnyModifiers()) {
      // We will later un-skip if we do anything, but if we don't,
      // propagate the event up for the sake of the scrubber
      event.Skip();
      event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
   }

   // Delegate wheel handling to the cell under the mouse, if it knows how.
   const auto foundCell = FindCell( event.m_x, event.m_y );
   auto &inner = foundCell.inner;
   auto  pCell = foundCell.pCell;
   auto pTrack = foundCell.pTrack;
   if (pCell) {
      TrackPanelMouseEvent tpmEvent{ event, inner, pCell };
      tpmEvent.steps = steps;
      unsigned result =
      pCell->HandleWheelRotation(tpmEvent, GetProject());
      ProcessUIHandleResult(this, mRuler, pTrack, pTrack, result);
      if (!(result & RefreshCode::Cancelled))
         return;
   }
}

/// Filter captured keys typed into LabelTracks.
void TrackPanel::OnCaptureKey(wxCommandEvent & event)
{
   HandleInterruptedDrag();

   Track * const t = GetFocusedTrack();
   if (t) {
      wxKeyEvent *kevent = static_cast<wxKeyEvent *>(event.GetEventObject());
      const unsigned refreshResult =
         ((TrackPanelCell*)t)->CaptureKey(*kevent, *mViewInfo, this);
      ProcessUIHandleResult(this, mRuler, t, t, refreshResult);
   }
   else
      event.Skip();
}

void TrackPanel::OnKeyDown(wxKeyEvent & event)
{
   switch (event.GetKeyCode())
   {
   case WXK_ESCAPE:
      if(HandleEscapeKey(true))
         // Don't skip the event, eat it so that
         // AudacityApp does not also stop any playback.
         return;
      else
         break;

   case WXK_ALT:
      HandleAltKey(true);
      break;

   case WXK_SHIFT:
      HandleShiftKey(true);
      break;

   case WXK_CONTROL:
      HandleControlKey(true);
      break;

      // Allow PageUp and PageDown keys to
      //scroll the Track Panel left and right
   case WXK_PAGEUP:
      HandlePageUpKey();
      return;

   case WXK_PAGEDOWN:
      HandlePageDownKey();
      return;
   }

   Track *const t = GetFocusedTrack();

   if (t) {
      const unsigned refreshResult =
         ((TrackPanelCell*)t)->KeyDown(event, *mViewInfo, this);
      ProcessUIHandleResult(this, mRuler, t, t, refreshResult);
   }
   else
      event.Skip();
}

void TrackPanel::OnChar(wxKeyEvent & event)
{
   switch (event.GetKeyCode())
   {
   case WXK_ESCAPE:
   case WXK_ALT:
   case WXK_SHIFT:
   case WXK_CONTROL:
   case WXK_PAGEUP:
   case WXK_PAGEDOWN:
      return;
   }

   Track *const t = GetFocusedTrack();
   if (t) {
      const unsigned refreshResult =
         ((TrackPanelCell*)t)->Char(event, *mViewInfo, this);
      ProcessUIHandleResult(this, mRuler, t, t, refreshResult);
   }
   else
      event.Skip();
}

void TrackPanel::OnKeyUp(wxKeyEvent & event)
{
   bool didSomething = false;
   switch (event.GetKeyCode())
   {
   case WXK_ESCAPE:
      didSomething = HandleEscapeKey(false);
      break;
   case WXK_ALT:
      HandleAltKey(false);
      break;

   case WXK_SHIFT:
      HandleShiftKey(false);
      break;

   case WXK_CONTROL:
      HandleControlKey(false);
      break;
   }

   if (didSomething)
      return;

   Track * const t = GetFocusedTrack();
   if (t) {
      const unsigned refreshResult =
         ((TrackPanelCell*)t)->KeyUp(event, *mViewInfo, this);
      ProcessUIHandleResult(this, mRuler, t, t, refreshResult);
      return;
   }

   event.Skip();
}

/// Should handle the case when the mouse capture is lost.
void TrackPanel::OnCaptureLost(wxMouseCaptureLostEvent & WXUNUSED(event))
{
   wxMouseEvent e(wxEVT_LEFT_UP);

   e.m_x = mMouseMostRecentX;
   e.m_y = mMouseMostRecentY;

   OnMouseEvent(e);
}

/// This handles just generic mouse events.  Then, based
/// on our current state, we forward the mouse events to
/// various interested parties.
void TrackPanel::OnMouseEvent(wxMouseEvent & event)
try
{
#if defined(__WXMAC__) && defined(EVT_MAGNIFY)
   // PRL:
   // Pinch and spread implemented in wxWidgets 3.1.0, or cherry-picked from
   // the future in custom build of 3.0.2
   if (event.Magnify()) {
      HandleWheelRotation(event);
   }
#endif

   // If a mouse event originates from a keyboard context menu event then
   // event.GetPosition() == wxDefaultPosition. wxContextMenu events are handled in
   // TrackPanel::OnContextMenu(), and therefore associated mouse events are ignored here.
   // Not ignoring them was causing bug 613: the mouse events were interpreted as clicking
   // outside the tracks.
   if (event.GetPosition() == wxDefaultPosition && (event.RightDown() || event.RightUp())) {
      event.Skip();
      return;
   }

   if (event.m_wheelRotation != 0)
      HandleWheelRotation(event);

   if (event.LeftDown() || event.LeftIsDown() || event.Moving()) {
      // Skip, even if we do something, so that the left click or drag
      // may have an additional effect in the scrubber.
      event.Skip();
      event.ResumePropagation(wxEVENT_PROPAGATE_MAX);
   }

   if (event.LeftDown()) {
      // The activate event is used to make the
      // parent window 'come alive' if it didn't have focus.
      wxActivateEvent e;
      GetParent()->GetEventHandler()->ProcessEvent(e);

      // wxTimers seem to be a little unreliable, so this
      // "primes" it to make sure it keeps going for a while...

      // When this timer fires, we call TrackPanel::OnTimer and
      // possibly update the screen for offscreen scrolling.
      mTimer.Stop();
      mTimer.Start(kTimerInterval, FALSE);
   }

   if (event.ButtonDown()) {
      SetFocus();
   }
   if (event.ButtonUp()) {
      if (HasCapture())
         ReleaseMouse();
   }

   if (event.Leaving())
   {
      auto buttons =
         // Bug 1325: button state in Leaving events is unreliable on Mac.
         // Poll the global state instead.
         // event.ButtonIsDown(wxMOUSE_BTN_ANY);
         ::wxGetMouseState().ButtonIsDown(wxMOUSE_BTN_ANY);

      if(!buttons) {
         //SetCapturedTrack(NULL);
         HandleEscapeKey(true);

#if defined(__WXMAC__)

         // We must install the cursor ourselves since the window under
         // the mouse is no longer this one and wx2.8.12 makes that check.
         // Should re-evaluate with wx3.
         wxSTANDARD_CURSOR->MacInstall();
#endif
      }
   }

   if (mUIHandle) {
      const auto foundCell = FindCell( event.m_x, event.m_y );
      auto &inner = foundCell.inner;
      auto &pCell = foundCell.pCell;
      auto &pTrack = foundCell.pTrack;

      if (event.Dragging()) {
         // UIHANDLE DRAG
         const UIHandle::Result refreshResult =
            mUIHandle->Drag(TrackPanelMouseEvent(event, inner, pCell), GetProject());
         ProcessUIHandleResult(this, mRuler, mpClickedTrack, pTrack, refreshResult);
         if (refreshResult & RefreshCode::Cancelled) {
            // Drag decided to abort itself
            mUIHandle = NULL;
            mpClickedTrack = NULL;
            if (HasCapture())
               ReleaseMouse();
            // Should this be done?  As for cancelling?
            // HandleCursor(event);
         }
         else {
            HitTestPreview preview =
               mUIHandle->Preview(TrackPanelMouseEvent(event, inner, pCell), GetProject());
            mListener->TP_DisplayStatusMessage(preview.message);
            if (preview.cursor)
               SetCursor(*preview.cursor);
         }
      }
      else if (event.ButtonUp()) {
         // UIHANDLE RELEASE
         UIHandle::Result refreshResult =
            mUIHandle->Release(TrackPanelMouseEvent(event, inner, pCell), GetProject(), this);
         ProcessUIHandleResult(this, mRuler, mpClickedTrack, pTrack, refreshResult);
         mUIHandle = NULL;
         mpClickedTrack = NULL;
         // ReleaseMouse() already done above
         // Should this be done?  As for cancelling?
         // HandleCursor(event);
      }
   }
   else {
      // This is where most button-downs are detected
      HandleTrackSpecificMouseEvent(event);
   }

   if (event.ButtonDown() && IsMouseCaptured()) {
      if (!HasCapture())
         CaptureMouse();
   }

   //EnsureVisible should be called after the up-click.
   if (event.ButtonUp()) {
      wxRect rect;

      const auto foundCell = FindCell(event.m_x, event.m_y);
      auto t = foundCell.pTrack;
      if (t
          && foundCell.type == CellType::Track
      )
         EnsureVisible(t);
   }
}
catch( ... )
{
   // Abort any dragging, as if by hitting Esc
   if ( HandleEscapeKey( true ) )
      ;
   else {
      // Ensure these steps, if escape handling did nothing
      if (HasCapture())
         ReleaseMouse();
      wxMouseEvent dummy;
      HandleCursor(dummy);
      Refresh(false);
   }
   throw;
}

// AS: I don't really understand why this code is sectioned off
//  from the other OnMouseEvent code.
void TrackPanel::HandleTrackSpecificMouseEvent(wxMouseEvent & event)
{
   const auto foundCell = FindCell( event.m_x, event.m_y );
   auto &pTrack = foundCell.pTrack;
   auto &pCell = foundCell.pCell;
   auto &rect = foundCell.rect;
   auto &inner = foundCell.inner;

   if (event.LeftDown())
      mInitialLastPickedTrack = mLastPickedTrack;

   // see if I'm over the border area.
   // TrackPanelResizeHandle is the UIHandle subclass that TrackPanel knows
   // and uses directly, because allocating area to cells is TrackPanel's business,
   // and we implement a "hit test" directly here.
   if (mUIHandle == NULL &&
       event.LeftDown()) {
      if (pCell &&
          within(event.m_y, inner.GetBottom() + kBorderThickness, TRACK_RESIZE_REGION))
         mUIHandle = &TrackPanelResizeHandle::Instance();
   }

   if (pCell ||
       (mUIHandle == NULL &&
        foundCell.type != CellType::Track)) {
      //Determine if user clicked on the track's left-hand label
      if (!mUIHandle &&
         event.m_x < GetLeftOffset() &&
         pTrack &&
         (event.ButtonDown() || event.ButtonDClick())) {
         if (pCell)
            mUIHandle = pCell->HitTest(TrackPanelMouseEvent(event, inner), GetProject()).handle;
      }

      if (mUIHandle == NULL &&
         pCell &&
         (event.ButtonDown() || event.ButtonDClick()))
         mUIHandle =
         pCell->HitTest(TrackPanelMouseEvent(event, inner), GetProject()).handle;

      if (mUIHandle) {
         // UIHANDLE CLICK
         UIHandle::Result refreshResult =
            mUIHandle->Click(TrackPanelMouseEvent(event, inner, pCell), GetProject());
         if (refreshResult & RefreshCode::Cancelled)
            mUIHandle = NULL;
         else
            mpClickedTrack = pTrack;
         ProcessUIHandleResult(this, mRuler, pTrack, pTrack, refreshResult);
      }

      if (mUIHandle) {
         HandleCursor(event);
         return;
      }
   }

   if ((event.Moving() || event.LeftUp()))
      HandleCursor(event);
}

double TrackPanel::GetMostRecentXPos()
{
   return mViewInfo->PositionToTime(mMouseMostRecentX, GetLabelWidth());
}

void TrackPanel::RefreshTrack(Track *trk, bool refreshbacking)
{
   Track *link = trk->GetLink();

   if (link && !trk->GetLinked()) {
      trk = link;
      link = trk->GetLink();
   }

   // subtract insets and shadows from the rectangle, but not border
   // This matters because some separators do paint over the border
   wxRect rect(kLeftInset,
            -mViewInfo->vpos + trk->GetY() + kTopInset,
            GetRect().GetWidth() - kLeftInset - kRightInset - kShadowThickness,
            trk->GetHeight() - kTopInset - kShadowThickness);

   if (link) {
      rect.height += link->GetHeight();
   }

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   else if(MONO_WAVE_PAN(trk)){
      rect.height += trk->GetHeight(true);
   }
#endif

   if( refreshbacking )
   {
      mRefreshBacking = true;
   }

   Refresh( false, &rect );
}


/// This method overrides Refresh() of wxWindow so that the
/// boolean play indictaor can be set to false, so that an old play indicator that is
/// no longer there won't get  XORed (to erase it), thus redrawing it on the
/// TrackPanel
void TrackPanel::Refresh(bool eraseBackground /* = TRUE */,
                         const wxRect *rect /* = NULL */)
{
   // Tell OnPaint() to refresh the backing bitmap.
   //
   // Originally I had the check within the OnPaint() routine and it
   // was working fine.  That was until I found that, even though a full
   // refresh was requested, Windows only set the onscreen portion of a
   // window as damaged.
   //
   // So, if any part of the trackpanel was off the screen, full refreshes
   // didn't work and the display got corrupted.
   if( !rect || ( *rect == GetRect() ) )
   {
      mRefreshBacking = true;
   }
   wxWindow::Refresh(eraseBackground, rect);
   DisplaySelection();
}

/// Draw the actual track areas.  We only draw the borders
/// and the little buttons and menues and whatnot here, the
/// actual contents of each track are drawn by the TrackArtist.
void TrackPanel::DrawTracks(wxDC * dc)
{
   wxRegion region = GetUpdateRegion();

   wxRect clip = GetRect();

   wxRect panelRect = clip;
   panelRect.y = -mViewInfo->vpos;

   wxRect tracksRect = panelRect;
   tracksRect.x += GetLabelWidth();
   tracksRect.width -= GetLabelWidth();

   ToolsToolBar *pTtb = mListener->TP_GetToolsToolBar();
   bool bMultiToolDown = pTtb->IsDown(multiTool);
   bool envelopeFlag   = pTtb->IsDown(envelopeTool) || bMultiToolDown;
   bool bigPointsFlag  = pTtb->IsDown(drawTool) || bMultiToolDown;
   bool sliderFlag     = bMultiToolDown;

   // The track artist actually draws the stuff inside each track
   mTrackArtist->DrawTracks(GetTracks(), GetProject()->GetFirstVisible(),
                            *dc, region, tracksRect, clip,
                            mViewInfo->selectedRegion, *mViewInfo,
                            envelopeFlag, bigPointsFlag, sliderFlag);

   DrawEverythingElse(dc, region, clip);
}

/// Draws 'Everything else'.  In particular it draws:
///  - Drop shadow for tracks and vertical rulers.
///  - Zooming Indicators.
///  - Fills in space below the tracks.
void TrackPanel::DrawEverythingElse(wxDC * dc,
                                    const wxRegion &region,
                                    const wxRect & clip)
{
   // We draw everything else

   wxRect focusRect(-1, -1, 0, 0);
   wxRect trackRect = clip;
   trackRect.height = 0;   // for drawing background in no tracks case.

   VisibleTrackIterator iter(GetProject());
   for (Track *t = iter.First(); t; t = iter.Next()) {
      trackRect.y = t->GetY() - mViewInfo->vpos;
      trackRect.height = t->GetHeight();

      // If this track is linked to the next one, display a common
      // border for both, otherwise draw a normal border
      wxRect rect = trackRect;
      bool skipBorder = false;
      Track *l = t->GetLink();

      if (t->GetLinked()) {
         rect.height += l->GetHeight();
      }
      else if (l && trackRect.y >= 0) {
         skipBorder = true;
      }

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      if(MONO_WAVE_PAN(t)){
         rect.height += t->GetHeight(true);
      }
#endif

      // If the previous track is linked to this one but isn't on the screen
      // (and thus would have been skipped by VisibleTrackIterator) we need to
      // draw that track's border instead.
      Track *borderTrack = t;
      wxRect borderRect = rect, borderTrackRect = trackRect;

      if (l && !t->GetLinked() && trackRect.y < 0)
      {
         borderTrack = l;

         borderTrackRect.y = l->GetY() - mViewInfo->vpos;
         borderTrackRect.height = l->GetHeight();

         borderRect = borderTrackRect;
         borderRect.height += t->GetHeight();
      }

      if (!skipBorder) {
         if (mAx->IsFocused(t)) {
            focusRect = borderRect;
         }
         DrawOutside(borderTrack, dc, borderRect, borderTrackRect);
      }

      // Believe it or not, we can speed up redrawing if we don't
      // redraw the vertical ruler when only the waveform data has
      // changed.  An example is during recording.

#if DEBUG_DRAW_TIMING
//      wxRect rbox = region.GetBox();
//      wxPrintf(wxT("Update Region: %d %d %d %d\n"),
//             rbox.x, rbox.y, rbox.width, rbox.height);
#endif

      if (region.Contains(0, trackRect.y, GetLeftOffset(), trackRect.height)) {
         wxRect rect = trackRect;
         rect.x += GetVRulerOffset();
         rect.y += kTopMargin;
         rect.width = GetVRulerWidth();
         rect.height -= (kTopMargin + kBottomMargin);
         mTrackArtist->DrawVRuler(t, dc, rect);
      }

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      if(MONO_WAVE_PAN(t)){
         trackRect.y = t->GetY(true) - mViewInfo->vpos;
         trackRect.height = t->GetHeight(true);
         if (region.Contains(0, trackRect.y, GetLeftOffset(), trackRect.height)) {
            wxRect rect = trackRect;
            rect.x += GetVRulerOffset();
            rect.y += kTopMargin;
            rect.width = GetVRulerWidth();
            rect.height -= (kTopMargin + kBottomMargin);
            mTrackArtist->DrawVRuler(t, dc, rect);
         }
      }
#endif
   }

   if (mUIHandle)
      mUIHandle->DrawExtras(UIHandle::Cells, dc, region, clip);

   // Paint over the part below the tracks
   trackRect.y += trackRect.height;
   if (trackRect.y < clip.GetBottom()) {
      AColor::TrackPanelBackground(dc, false);
      dc->DrawRectangle(trackRect.x,
                        trackRect.y,
                        trackRect.width,
                        clip.height - trackRect.y);
   }

   // Sometimes highlight is not drawn on backing bitmap. I thought
   // it was because FindFocus did not return "this" on Mac, but
   // when I removed that test, yielding this condition:
   //     if (GetFocusedTrack() != NULL) {
   // the highlight was reportedly drawn even when something else
   // was the focus and no highlight should be drawn. -RBD
   if (GetFocusedTrack() != NULL && wxWindow::FindFocus() == this) {
      HighlightFocusedTrack(dc, focusRect);
   }

   if (mUIHandle)
      mUIHandle->DrawExtras(UIHandle::Panel, dc, region, clip);
}

// Make this #include go away!
#include "tracks/ui/TrackControls.h"

void TrackPanel::DrawOutside(Track * t, wxDC * dc, const wxRect & rec,
                             const wxRect & trackRect)
{
   wxRect rect = rec;
   int labelw = GetLabelWidth();
   int vrul = GetVRulerOffset();

   DrawOutsideOfTrack(t, dc, rect);

   rect.x += kLeftInset;
   rect.y += kTopInset;
   rect.width -= kLeftInset * 2;
   rect.height -= kTopInset;

   mTrackInfo.SetTrackInfoFont(dc);
   dc->SetTextForeground(theTheme.Colour(clrTrackPanelText));

   bool bIsWave = (t->GetKind() == Track::Wave);
#ifdef USE_MIDI
   bool bIsNote = (t->GetKind() == Track::Note);
#endif
   // don't enable bHasMuteSolo for Note track because it will draw in the
   // wrong place.
   mTrackInfo.DrawBackground(dc, rect, t->GetSelected(), bIsWave, labelw, vrul);

   // Vaughan, 2010-08-24: No longer doing this.
   // Draw sync-lock tiles in ruler area.
   //if (t->IsSyncLockSelected()) {
   //   wxRect tileFill = rect;
   //   tileFill.x = GetVRulerOffset();
   //   tileFill.width = GetVRulerWidth();
   //   TrackArtist::DrawSyncLockTiles(dc, tileFill);
   //}

   DrawBordersAroundTrack(t, dc, rect, labelw, vrul);
   DrawShadow(t, dc, rect);

   rect.width = mTrackInfo.GetTrackInfoWidth();

   // Need to know which button, if any, to draw as pressed.
   const MouseCaptureEnum mouseCapture =
      // This public global variable is a hack for now, which should go away
      // when TrackPanelCell gets a virtual function into which we move this
      // drawing code.  It's enough work for 2.1.2 just to move all the click and
      // drag code out of TrackPanel.  -- PRL
      MouseCaptureEnum(TrackControls::gCaptureState);
   const bool captured = (t == mpClickedTrack);

   mTrackInfo.DrawCloseBox(dc, rect, (captured && mouseCapture == IsClosing));
   mTrackInfo.DrawTitleBar(dc, rect, t, (captured && mouseCapture == IsPopping));

   mTrackInfo.DrawMinimize(dc, rect, t, (captured && mouseCapture == IsMinimizing));

   // Draw the sync-lock indicator if this track is in a sync-lock selected group.
   if (t->IsSyncLockSelected())
   {
      wxRect syncLockIconRect;
      mTrackInfo.GetSyncLockIconRect(rect, syncLockIconRect);
      wxBitmap syncLockBitmap(theTheme.Image(bmpSyncLockIcon));
      // Icon is 12x12 and syncLockIconRect is 16x16.
      dc->DrawBitmap(syncLockBitmap,
                     syncLockIconRect.x + 3,
                     syncLockIconRect.y + 2,
                     true);
   }

   mTrackInfo.DrawBordersWithin( dc, rect, bIsWave );

   auto wt = bIsWave ? static_cast<WaveTrack*>(t) : nullptr;
   if (bIsWave) {
      mTrackInfo.DrawMuteSolo(dc, rect, t, (captured && mouseCapture == IsMuting), false, HasSoloButton());
      mTrackInfo.DrawMuteSolo(dc, rect, t, (captured && mouseCapture == IsSoloing), true, HasSoloButton());

      mTrackInfo.DrawSliders(dc, (WaveTrack *)t, rect, captured);
      if (!t->GetMinimized()) {

         int offset = 8;
         if (rect.y + 22 + 12 < rec.y + rec.height - 19)
            dc->DrawText(TrackSubText(wt),
                         trackRect.x + offset,
                         trackRect.y + 22);

         if (rect.y + 38 + 12 < rec.y + rec.height - 19)
            dc->DrawText(GetSampleFormatStr(((WaveTrack *) t)->GetSampleFormat()),
                         trackRect.x + offset,
                         trackRect.y + 38);
      }
   }

#ifdef USE_MIDI
   else if (bIsNote) {
#ifdef EXPERIMENTAL_MIDI_OUT
      wxRect midiRect;
      TrackInfo::GetTrackControlsRect(rect, midiRect);
      ((NoteTrack *)t)->DrawLabelControls(*dc, midiRect);

      // Draw some lines for MuteSolo buttons (normally handled by DrawBordersWithin but not done for note tracks)
      if (rect.height > 48) {
         // Note: offset up by 34 units
         AColor::Line(*dc, rect.x + 48, rect.y + 16, rect.x + 48, rect.y + 32);    // between mute/solo
         AColor::Line(*dc, rect.x, rect.y + 32, kTrackInfoWidth, rect.y + 32);   // below mute/solo
      }
      mTrackInfo.DrawMuteSolo(dc, rect, t,
         (captured && mouseCapture == IsMuting), false, HasSoloButton());
      mTrackInfo.DrawMuteSolo(dc, rect, t,
         (captured && mouseCapture == IsSoloing), true, HasSoloButton());

      // Place a volume control below channel buttons (this will
      // control an offset to midi velocity).
      mTrackInfo.DrawVelocitySlider(dc, (NoteTrack *)t, rect, captured);
#endif
   }
#endif // USE_MIDI
}

void TrackPanel::DrawOutsideOfTrack(Track * t, wxDC * dc, const wxRect & rect)
{
   // Fill in area outside of the track
   AColor::TrackPanelBackground(dc, false);
   wxRect side;

   // Area between panel border and left track border
   side = rect;
   side.width = kLeftInset;
   dc->DrawRectangle(side);

   // Area between panel border and top track border
   side = rect;
   side.height = kTopInset;
   dc->DrawRectangle(side);

   // Area between panel border and right track border
   side = rect;
   side.x += side.width - kTopInset;
   side.width = kTopInset;
   dc->DrawRectangle(side);

   // Area between tracks of stereo group
   if (t->GetLinked()
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
       || MONO_WAVE_PAN(t)
#endif
       ) {
      // Paint the channel separator over (what would be) the shadow of the top
      // channel, and the top inset of the bottom channel
      side = rect;
      side.y += t->GetHeight() - kShadowThickness;
      side.height = kTopInset + kShadowThickness;
      dc->DrawRectangle(side);
   }
}

void TrackPanel::SetBackgroundCell(TrackPanelCell *pCell)
{
   mpBackground = pCell;
}

/// Draw a three-level highlight gradient around the focused track.
void TrackPanel::HighlightFocusedTrack(wxDC * dc, const wxRect & rect)
{
   wxRect theRect = rect;
   theRect.x += kLeftInset;
   theRect.y += kTopInset;
   theRect.width -= kLeftInset * 2;
   theRect.height -= kTopInset;

   dc->SetBrush(*wxTRANSPARENT_BRUSH);

   AColor::TrackFocusPen(dc, 0);
   dc->DrawRectangle(theRect.x - 1, theRect.y - 1, theRect.width + 2, theRect.height + 2);

   AColor::TrackFocusPen(dc, 1);
   dc->DrawRectangle(theRect.x - 2, theRect.y - 2, theRect.width + 4, theRect.height + 4);

   AColor::TrackFocusPen(dc, 2);
   dc->DrawRectangle(theRect.x - 3, theRect.y - 3, theRect.width + 6, theRect.height + 6);
}

void TrackPanel::UpdateVRulers()
{
   TrackListOfKindIterator iter(Track::Wave, GetTracks());
   for (Track *t = iter.First(); t; t = iter.Next()) {
      UpdateTrackVRuler(t);
   }

   UpdateVRulerSize();
}

void TrackPanel::UpdateVRuler(Track *t)
{
   UpdateTrackVRuler(t);

   UpdateVRulerSize();
}

void TrackPanel::UpdateTrackVRuler(const Track *t)
{
   wxASSERT(t);
   if (!t)
      return;

   wxRect rect(GetVRulerOffset(),
            kTopMargin,
            GetVRulerWidth(),
            t->GetHeight() - (kTopMargin + kBottomMargin));

   mTrackArtist->UpdateVRuler(t, rect);
   const Track *l = t->GetLink();
   if (l)
   {
      rect.height = l->GetHeight() - (kTopMargin + kBottomMargin);
      mTrackArtist->UpdateVRuler(l, rect);
   }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   else if(MONO_WAVE_PAN(t)){
      rect.height = t->GetHeight(true) - (kTopMargin + kBottomMargin);
      mTrackArtist->UpdateVRuler(t, rect);
   }
#endif
}

void TrackPanel::UpdateVRulerSize()
{
   TrackListIterator iter(GetTracks());
   Track *t = iter.First();
   if (t) {
      wxSize s = t->vrulerSize;
      while (t) {
         s.IncTo(t->vrulerSize);
         t = iter.Next();
      }
      if (vrulerSize != s) {
         vrulerSize = s;
         mRuler->SetLeftOffset(GetLeftOffset());  // bevel on AdornedRuler
         mRuler->Refresh();
      }
   }
   Refresh(false);
}

/// The following method moves to the previous track
/// selecting and unselecting depending if you are on the start of a
/// block or not.

/// \todo Merge related methods, TrackPanel::OnPrevTrack and
/// TrackPanel::OnNextTrack.
void TrackPanel::OnPrevTrack( bool shift )
{
   TrackListIterator iter( GetTracks() );
   Track* t = GetFocusedTrack();
   if( t == NULL )   // if there isn't one, focus on last
   {
      t = iter.Last();
      SetFocusedTrack( t );
      EnsureVisible( t );
      MakeParentModifyState(false);
      return;
   }

   Track* p = NULL;
   bool tSelected = false;
   bool pSelected = false;
   if( shift )
   {
      p = mTracks->GetPrev( t, true ); // Get previous track
      if( p == NULL )   // On first track
      {
         // JKC: wxBell() is probably for accessibility, so a blind
         // user knows they were at the top track.
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( GetTracks() );
            p = iter.Last();
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      tSelected = t->GetSelected();
      if (p)
         pSelected = p->GetSelected();
      if( tSelected && pSelected )
      {
         SelectTrack(t, false, false);
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         MakeParentModifyState(false);
         return;
      }
      if( tSelected && !pSelected )
      {
         SelectTrack(p, true, false);
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         MakeParentModifyState(false);
         return;
      }
      if( !tSelected && pSelected )
      {
         SelectTrack(p, false, false);
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         MakeParentModifyState(false);
         return;
      }
      if( !tSelected && !pSelected )
      {
         SelectTrack(t, true, false);
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         MakeParentModifyState(false);
         return;
      }
   }
   else
   {
      p = mTracks->GetPrev( t, true ); // Get next track
      if( p == NULL )   // On last track so stay there?
      {
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( GetTracks() );
            for( Track *d = iter.First(); d; d = iter.Next( true ) )
            {
               p = d;
            }
            SetFocusedTrack( p );   // Wrap to the first track
            EnsureVisible( p );
            MakeParentModifyState(false);
            return;
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      else
      {
         SetFocusedTrack( p );   // move focus to next track down
         EnsureVisible( p );
         MakeParentModifyState(false);
         return;
      }
   }
}

/// The following method moves to the next track,
/// selecting and unselecting depending if you are on the start of a
/// block or not.
void TrackPanel::OnNextTrack( bool shift )
{
   Track *t;
   Track *n;
   TrackListIterator iter( GetTracks() );
   bool tSelected,nSelected;

   t = GetFocusedTrack();   // Get currently focused track
   if( t == NULL )   // if there isn't one, focus on first
   {
      t = iter.First();
      SetFocusedTrack( t );
      EnsureVisible( t );
      MakeParentModifyState(false);
      return;
   }

   if( shift )
   {
      n = mTracks->GetNext( t, true ); // Get next track
      if( n == NULL )   // On last track so stay there
      {
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( GetTracks() );
            n = iter.First();
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      tSelected = t->GetSelected();
      nSelected = n->GetSelected();
      if( tSelected && nSelected )
      {
         SelectTrack(t, false, false);
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         MakeParentModifyState(false);
         return;
      }
      if( tSelected && !nSelected )
      {
         SelectTrack(n, true, false);
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         MakeParentModifyState(false);
         return;
      }
      if( !tSelected && nSelected )
      {
         SelectTrack(n, false, false);
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         MakeParentModifyState(false);
         return;
      }
      if( !tSelected && !nSelected )
      {
         SelectTrack(t, true, false);
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         MakeParentModifyState(false);
         return;
      }
   }
   else
   {
      n = mTracks->GetNext( t, true ); // Get next track
      if( n == NULL )   // On last track so stay there
      {
         wxBell();
         if( mCircularTrackNavigation )
         {
            TrackListIterator iter( GetTracks() );
            n = iter.First();
            SetFocusedTrack( n );   // Wrap to the first track
            EnsureVisible( n );
            MakeParentModifyState(false);
            return;
         }
         else
         {
            EnsureVisible( t );
            return;
         }
      }
      else
      {
         SetFocusedTrack( n );   // move focus to next track down
         EnsureVisible( n );
         MakeParentModifyState(false);
         return;
      }
   }
}

void TrackPanel::OnFirstTrack()
{
   Track *t = GetFocusedTrack();
   if (!t)
      return;

   TrackListIterator iter(GetTracks());
   Track *f = iter.First();
   if (t != f)
   {
      SetFocusedTrack(f);
      MakeParentModifyState(false);
   }
   EnsureVisible(f);
}

void TrackPanel::OnLastTrack()
{
   Track *t = GetFocusedTrack();
   if (!t)
      return;

   TrackListIterator iter(GetTracks());
   Track *l = iter.Last();
   if (t != l)
   {
      SetFocusedTrack(l);
      MakeParentModifyState(false);
   }
   EnsureVisible(l);
}

void TrackPanel::OnToggle()
{
   Track *t;

   t = GetFocusedTrack();   // Get currently focused track
   if (!t)
      return;

   SelectTrack( t, !t->GetSelected() );
   EnsureVisible( t );
   MakeParentModifyState(false);

   mAx->Updated();

   return;
}

// Make sure selection edge is in view
void TrackPanel::ScrollIntoView(double pos)
{
   int w;
   GetTracksUsableArea( &w, NULL );

   int pixel = mViewInfo->TimeToPosition(pos);
   if (pixel < 0 || pixel >= w)
   {
      mListener->TP_ScrollWindow
         (mViewInfo->OffsetTimeByPixels(pos, -(w / 2)));
      Refresh(false);
   }
}

void TrackPanel::ScrollIntoView(int x)
{
   ScrollIntoView(mViewInfo->PositionToTime(x, GetLeftOffset()));
}

void TrackPanel::OnTrackMenu(Track *t)
{
   if(!t) {
      t = GetFocusedTrack();
      if(!t)
         return;
   }

   TrackPanelCell *const pCell = t->GetTrackControl();
   const wxRect rect(FindTrackRect(t, true));
   const UIHandle::Result refreshResult =
      pCell->DoContextMenu(rect, this, NULL);
   ProcessUIHandleResult(this, mRuler, t, t, refreshResult);
}

Track * TrackPanel::GetFirstSelectedTrack()
{

   TrackListIterator iter(GetTracks());

   Track * t;
   for ( t = iter.First();t!=NULL;t=iter.Next())
      {
         //Find the first selected track
         if(t->GetSelected())
            {
               return t;
            }

      }
   //if nothing is selected, return the first track
   t = iter.First();

   if(t)
      return t;
   else
      return NULL;
}

void TrackPanel::EnsureVisible(Track * t)
{
   TrackListIterator iter(GetTracks());
   Track *it = NULL;
   Track *nt = NULL;

   SetFocusedTrack(t);

   int trackTop = 0;
   int trackHeight =0;

   for (it = iter.First(); it; it = iter.Next()) {
      trackTop += trackHeight;
      trackHeight =  it->GetHeight();

      //find the second track if this is stereo
      if (it->GetLinked()) {
         nt = iter.Next();
         trackHeight += nt->GetHeight();
      }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      else if(MONO_WAVE_PAN(it)){
         trackHeight += it->GetHeight(true);
      }
#endif
      else {
         nt = it;
      }

      //We have found the track we want to ensure is visible.
      if ((it == t) || (nt == t)) {

         //Get the size of the trackpanel.
         int width, height;
         GetSize(&width, &height);

         if (trackTop < mViewInfo->vpos) {
            height = mViewInfo->vpos - trackTop + mViewInfo->scrollStep;
            height /= mViewInfo->scrollStep;
            mListener->TP_ScrollUpDown(-height);
         }
         else if (trackTop + trackHeight > mViewInfo->vpos + height) {
            height = (trackTop + trackHeight) - (mViewInfo->vpos + height);
            height = (height + mViewInfo->scrollStep + 1) / mViewInfo->scrollStep;
            mListener->TP_ScrollUpDown(height);
         }

         break;
      }
   }
   Refresh(false);
}

void TrackPanel::DrawBordersAroundTrack(Track * t, wxDC * dc,
                                        const wxRect & rect, const int vrul,
                                        const int labelw)
{
   // Border around track and label area
   dc->SetBrush(*wxTRANSPARENT_BRUSH);
   dc->SetPen(*wxBLACK_PEN);
   dc->DrawRectangle(rect.x, rect.y, rect.width - 1, rect.height - 1);

   AColor::Line(*dc, labelw, rect.y, labelw, rect.y + rect.height - 1);       // between vruler and TrackInfo

   // The lines at bottom of 1st track and top of second track of stereo group
   // Possibly replace with DrawRectangle to add left border.
   if (t->GetLinked()
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
       || MONO_WAVE_PAN(t)
#endif
       ) {
      // The given rect has had the top inset subtracted
      int h1 = rect.y + t->GetHeight() - kTopInset;
      // h1 is the top coordinate of the second tracks' rectangle
      // Draw (part of) the bottom border of the top channel and top border of the bottom
      AColor::Line(*dc, vrul, h1 - kBottomMargin, rect.x + rect.width - 1, h1 - kBottomMargin);
      AColor::Line(*dc, vrul, h1 + kTopInset, rect.x + rect.width - 1, h1 + kTopInset);
   }
}

void TrackPanel::DrawShadow(Track * /* t */ , wxDC * dc, const wxRect & rect)
{
   int right = rect.x + rect.width - 1;
   int bottom = rect.y + rect.height - 1;

   // shadow
   dc->SetPen(*wxBLACK_PEN);

   // bottom
   AColor::Line(*dc, rect.x, bottom, right, bottom);
   // right
   AColor::Line(*dc, right, rect.y, right, bottom);

   // background
   AColor::Dark(dc, false);

   // bottom
   AColor::Line(*dc, rect.x, bottom, rect.x + 1, bottom);
   // right
   AColor::Line(*dc, right, rect.y, right, rect.y + 1);
}

/// Returns the string to be displayed in the track label
/// indicating whether the track is mono, left, right, or
/// stereo and what sample rate it's using.
wxString TrackPanel::TrackSubText(WaveTrack * t)
{
   wxString s = wxString::Format(wxT("%dHz"), (int) (t->GetRate() + 0.5));
   if (t->GetLinked()
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
       && t->GetChannel() != Track::MonoChannel
#endif
   )
      s = _("Stereo, ") + s;
   else {
      if (t->GetChannel() == Track::MonoChannel)
         s = _("Mono, ") + s;
      else if (t->GetChannel() == Track::LeftChannel)
         s = _("Left, ") + s;
      else if (t->GetChannel() == Track::RightChannel)
         s = _("Right, ") + s;
   }

   return s;
}

/// Determines which track is under the mouse
///  @param mouseX - mouse X position.
///  @param mouseY - mouse Y position.
TrackPanel::FoundCell TrackPanel::FindCell(int mouseX, int mouseY)
{
   wxRect rect;
   rect.x = 0;
   rect.y = -mViewInfo->vpos;
   rect.y += kTopInset;
   GetSize(&rect.width, &rect.height);

   // The type of cell that may be found is determined by the x coordinate.
   CellType type = CellType::Track;
   if (mouseX >= 0 && mouseX < rect.width) {
      if (mouseX < GetVRulerOffset())
         type = CellType::Label,
         rect.width = GetVRulerOffset();
      else if (mouseX < GetLeftOffset())
         type = CellType::VRuler,
         rect.x = GetVRulerOffset(),
         rect.width = GetLeftOffset() - GetVRulerOffset();
      else
         type = CellType::Track,
         rect.x = GetLeftOffset(),
         rect.width -= GetLeftOffset();
   }

   auto output = [&](Track *pTrack) -> FoundCell {
      wxRect inner;
      TrackPanelCell *pCell {};
      // If label or vruler, resulting rectangle OMITS left and top insets.
      // If track, resulting rectangle INCLUDES right and top insets.
      if (pTrack) {
         rect.y -= kTopInset;
         switch (type) {
            case CellType::Label:
               pCell = pTrack->GetTrackControl();
               rect.x += kLeftInset;
               rect.width -= kLeftInset;
               rect.y += kTopInset;
               rect.height -= kTopInset;
               inner = rect;
               break;
            case CellType::VRuler:
               pCell = pTrack->GetVRulerControl();
               rect.y += kTopInset;
               rect.height -= kTopInset;
               inner = rect;
               break;
            case CellType::Track:
               pCell = pTrack;
               inner = rect;
               inner.width = std::max(0, inner.width - kRightMargin);
               inner.y += kTopMargin;
               inner.height = std::max(0, inner.height - (kTopMargin + kBottomMargin));
               break;
            default:
               break;
         }
         return { pTrack, pCell, type, rect, inner };
      }
      else
         return { nullptr, nullptr, type, {}, {} };
   };

   VisibleTrackIterator iter(GetProject());
   for (Track * t = iter.First(); t; t = iter.Next()) {
      rect.y = t->GetY() - mViewInfo->vpos + kTopInset;
      rect.height = t->GetHeight();

      if (type == CellType::Label) {
         if (t->GetLink()) {
            Track *l = t->GetLink();
            int h = l->GetHeight();
            if (!t->GetLinked()) {
               t = l;
               rect.y = t->GetY() - mViewInfo->vpos + kTopInset;
            }
            rect.height += h;
         }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
         else if( MONO_WAVE_PAN(t) )
            rect.height += t->GetHeight(true);
#endif
      }

      //Determine whether the mouse is inside
      //the current rectangle.  If so, recalculate
      //the proper dimensions and return.
      if (rect.Contains(mouseX, mouseY)) {
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
         // PRL:  Is it good to have a side effect in a hit-testing routine?
         t->SetVirtualStereo(false);
#endif
         return output(t);
      }
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      if(type != CellType::Label && MONO_WAVE_PAN(t)){
         rect.y = t->GetY(true) - mViewInfo->vpos + kTopInset;
         rect.height = t->GetHeight(true);
         if (rect.Contains(mouseX, mouseY)) {
            // PRL:  Is it good to have a side effect in a hit-testing routine?
            t->SetVirtualStereo(true);
            return output(t);
         }
      }
#endif // EXPERIMENTAL_OUTPUT_DISPLAY
   }

   return { nullptr, nullptr, type, {}, {} };
}

/// This finds the rectangle of a given track, either the
/// of the label 'adornment' or the track itself
wxRect TrackPanel::FindTrackRect(Track * target, bool label)
{
   if (!target) {
      return wxRect(0,0,0,0);
   }

   wxRect rect(0,
            target->GetY() - mViewInfo->vpos,
            GetSize().GetWidth(),
            target->GetHeight());

   // The check for a null linked track is necessary because there's
   // a possible race condition between the time the 2 linked tracks
   // are added and when wxAccessible methods are called.  This is
   // most evident when using Jaws.
   if (target->GetLinked() && target->GetLink()) {
      rect.height += target->GetLink()->GetHeight();
   }

   if (label) {
      rect.x += kLeftInset;
      rect.width -= kLeftInset;
      rect.y += kTopInset;
      rect.height -= kTopInset;
   }

   return rect;
}

int TrackPanel::GetVRulerWidth() const
{
   return vrulerSize.x;
}

/// Displays the bounds of the selection in the status bar.
void TrackPanel::DisplaySelection()
{
   if (!mListener)
      return;

   // DM: Note that the Selection Bar can actually MODIFY the selection
   // if snap-to mode is on!!!
   mListener->TP_DisplaySelection();
}

Track *TrackPanel::GetFocusedTrack()
{
   return mAx->GetFocus();
}

void TrackPanel::SetFocusedTrack( Track *t )
{
   // Make sure we always have the first linked track of a stereo track
   if (t && !t->GetLinked() && t->GetLink())
      t = (WaveTrack*)t->GetLink();

   if (t && AudacityProject::GetKeyboardCaptureHandler()) {
      AudacityProject::ReleaseKeyboard(this);
   }

   if (t) {
      AudacityProject::CaptureKeyboard(this);
   }

   mAx->SetFocus( t );
   Refresh( false );
}

void TrackPanel::OnSetFocus(wxFocusEvent & WXUNUSED(event))
{
   SetFocusedTrack( GetFocusedTrack() );
   Refresh( false );
}

void TrackPanel::OnKillFocus(wxFocusEvent & WXUNUSED(event))
{
   if (AudacityProject::HasKeyboardCapture(this))
   {
      AudacityProject::ReleaseKeyboard(this);
   }
   Refresh( false);
}

/**********************************************************************

  TrackInfo code is destined to move out of this file.
  Code should become a lot cleaner when we have sizers.

**********************************************************************/

TrackInfo::TrackInfo(TrackPanel * pParentIn)
{
   pParent = pParentIn;

   wxRect rect(0, 0, 1000, 1000);
   wxRect sliderRect;

   GetGainRect(rect, sliderRect);

   /* i18n-hint: Title of the Gain slider, used to adjust the volume */
   mGain = std::make_unique<LWSlider>(pParent, _("Gain"),
                        wxPoint(sliderRect.x, sliderRect.y),
                        wxSize(sliderRect.width, sliderRect.height),
                        DB_SLIDER);
   mGain->SetDefaultValue(1.0);
   mGainCaptured = std::make_unique<LWSlider>(pParent, _("Gain"),
                                wxPoint(sliderRect.x, sliderRect.y),
                                wxSize(sliderRect.width, sliderRect.height),
                                DB_SLIDER);
   mGainCaptured->SetDefaultValue(1.0);

   GetPanRect(rect, sliderRect);

   /* i18n-hint: Title of the Pan slider, used to move the sound left or right */
   mPan = std::make_unique<LWSlider>(pParent, _("Pan"),
                       wxPoint(sliderRect.x, sliderRect.y),
                       wxSize(sliderRect.width, sliderRect.height),
                       PAN_SLIDER);
   mPan->SetDefaultValue(0.0);
   mPanCaptured = std::make_unique<LWSlider>(pParent, _("Pan"),
                               wxPoint(sliderRect.x, sliderRect.y),
                               wxSize(sliderRect.width, sliderRect.height),
                               PAN_SLIDER);
   mPanCaptured->SetDefaultValue(0.0);

#ifdef EXPERIMENTAL_MIDI_OUT
   GetVelocityRect(rect, sliderRect);

   /* i18n-hint: Title of the Velocity slider, used to adjust the volume of note tracks */
   mVelocity = std::make_unique<LWSlider>(pParent, _("Velocity"),
      wxPoint(sliderRect.x, sliderRect.y),
      wxSize(sliderRect.width, sliderRect.height),
      VEL_SLIDER);
   mVelocity->SetDefaultValue(0.0);
   mVelocityCaptured = std::make_unique<LWSlider>(pParent, _("Velocity"),
      wxPoint(sliderRect.x, sliderRect.y),
      wxSize(sliderRect.width, sliderRect.height),
      VEL_SLIDER);
   mVelocityCaptured->SetDefaultValue(0.0);
#endif

   UpdatePrefs();
}

TrackInfo::~TrackInfo()
{
}

int TrackInfo::GetTrackInfoWidth() const
{
   return kTrackInfoWidth;
}

void TrackInfo::GetCloseBoxRect(const wxRect & rect, wxRect & dest)
{
   dest.x = rect.x;
   dest.y = rect.y;
   dest.width = kTrackInfoBtnSize;
   dest.height = kTrackInfoBtnSize;
}

void TrackInfo::GetTitleBarRect(const wxRect & rect, wxRect & dest)
{
   dest.x = rect.x + kTrackInfoBtnSize; // to right of CloseBoxRect
   dest.y = rect.y;
   dest.width = kTrackInfoWidth - rect.x - kTrackInfoBtnSize; // to right of CloseBoxRect
   dest.height = kTrackInfoBtnSize;
}

void TrackInfo::GetMuteSoloRect(const wxRect & rect, wxRect & dest, bool solo, bool bHasSoloButton
#ifdef EXPERIMENTAL_MIDI_OUT
   , int trackKind)
#else
   )
#endif
{
   dest.x = rect.x ;
#ifdef EXPERIMENTAL_MIDI_OUT
   if (trackKind == Track::Note)
      dest.y = rect.y + 16;
   else
   {
      wxASSERT(trackKind == Track::Wave);
      dest.y = rect.y + 50;
   }
#else
   dest.y = rect.y + 50;
#endif
   dest.width = 48;
   dest.height = kTrackInfoBtnSize;

   if( !bHasSoloButton )
   {
      dest.width +=48;
   }
   else if (solo)
   {
      dest.x += 48;
   }
}

void TrackInfo::GetGainRect(const wxRect & rect, wxRect & dest)
{
   dest.x = rect.x + 7;
   dest.y = rect.y + 70;
   dest.width = 84;
   dest.height = 25;
}

void TrackInfo::GetPanRect(const wxRect & rect, wxRect & dest)
{
   dest.x = rect.x + 7;
   dest.y = rect.y + 100;
   dest.width = 84;
   dest.height = 25;
}

#ifdef EXPERIMENTAL_MIDI_OUT
void TrackInfo::GetVelocityRect(const wxRect & rect, wxRect & dest)
{
   dest.x = rect.x + 7;
   dest.y = rect.y + 100;
   dest.width = 84;
   dest.height = 25;
}
#endif

void TrackInfo::GetMinimizeRect(const wxRect & rect, wxRect &dest)
{
   const int kBlankWidth = kTrackInfoBtnSize + 4;
   dest.x = rect.x + kBlankWidth;
   dest.y = rect.y + rect.height - 19;
   // Width is kTrackInfoWidth less space on left for track select and on right for sync-lock icon.
   dest.width = kTrackInfoWidth - (2 * kBlankWidth);
   dest.height = kTrackInfoBtnSize;
}

void TrackInfo::GetSyncLockIconRect(const wxRect & rect, wxRect &dest)
{
   dest.x = rect.x + kTrackInfoWidth - kTrackInfoBtnSize - 4; // to right of minimize button
   dest.y = rect.y + rect.height - 19;
   dest.width = kTrackInfoBtnSize;
   dest.height = kTrackInfoBtnSize;
}


/// \todo Probably should move to 'Utils.cpp'.
void TrackInfo::SetTrackInfoFont(wxDC * dc) const
{
   dc->SetFont(mFont);
}

void TrackInfo::DrawBordersWithin(wxDC* dc, const wxRect & rect, bool bHasMuteSolo) const
{
   AColor::Dark(dc, false); // same color as border of toolbars (ToolBar::OnPaint())

   // below close box and title bar
   AColor::Line(*dc, rect.x, rect.y + kTrackInfoBtnSize, kTrackInfoWidth, rect.y + kTrackInfoBtnSize);

   // between close box and title bar
   AColor::Line(*dc, rect.x + kTrackInfoBtnSize, rect.y, rect.x + kTrackInfoBtnSize, rect.y + kTrackInfoBtnSize);

   if( bHasMuteSolo && (rect.height > (66+18) ))
   {
      AColor::Line(*dc, rect.x, rect.y + 50, kTrackInfoWidth, rect.y + 50);   // above mute/solo
      AColor::Line(*dc, rect.x + 48 , rect.y + 50, rect.x + 48, rect.y + 66);    // between mute/solo
      AColor::Line(*dc, rect.x, rect.y + 66, kTrackInfoWidth, rect.y + 66);   // below mute/solo
   }

   // left of and above minimize button
   wxRect minimizeRect;
   this->GetMinimizeRect(rect, minimizeRect);
   AColor::Line(*dc, minimizeRect.x - 1, minimizeRect.y,
                  minimizeRect.x - 1, minimizeRect.y + minimizeRect.height);
   AColor::Line(*dc, minimizeRect.x, minimizeRect.y - 1,
                  minimizeRect.x + minimizeRect.width, minimizeRect.y - 1);
}

//#define USE_BEVELS

#ifdef USE_BEVELS
void TrackInfo::DrawBackground(wxDC * dc, const wxRect & rect, bool bSelected,
   bool bHasMuteSolo, const int labelw, const int vrul) const
#else
void TrackInfo::DrawBackground(wxDC * dc, const wxRect & rect, bool bSelected,
   bool WXUNUSED(bHasMuteSolo), const int labelw, const int WXUNUSED(vrul)) const
#endif
{
   // fill in label
   wxRect fill = rect;
   fill.width = labelw-4;
   AColor::MediumTrackInfo(dc, bSelected);
   dc->DrawRectangle(fill);


#ifdef USE_BEVELS
   if( bHasMuteSolo )
   {
      int ylast = rect.height-20;
      int ybutton = wxMin(32,ylast-17);
      int ybuttonEnd = 67;

      fill=wxRect( rect.x+1, rect.y+17, vrul-6, ybutton);
      AColor::BevelTrackInfo( *dc, true, fill );
   
      if( ybuttonEnd < ylast ){
         fill=wxRect( rect.x+1, rect.y+ybuttonEnd, fill.width, ylast - ybuttonEnd);
         AColor::BevelTrackInfo( *dc, true, fill );
      }
   }
   else
   {
      fill=wxRect( rect.x+1, rect.y+17, vrul-6, rect.height-37);
      AColor::BevelTrackInfo( *dc, true, fill );
   }
#endif
}

const int cellWidth = 23, cellHeight = 16;

void TrackInfo::GetTrackControlsRect(const wxRect & rect, wxRect & dest)
{
   wxRect top;
   wxRect bot;
   GetTitleBarRect(rect, top);
   GetMinimizeRect(rect, bot);

   dest.x = 2 + rect.x;
   dest.width = cellWidth * 4;
   dest.y = top.y + 34;
   dest.height = std::min(cellHeight * 4, bot.GetTop() - top.GetBottom());
}


void TrackInfo::DrawCloseBox(wxDC * dc, const wxRect & rect, bool down) const
{
   wxRect bev;
   GetCloseBoxRect(rect, bev);

#ifdef EXPERIMENTAL_THEMING
   wxPen pen( theTheme.Colour( clrTrackPanelText ));
   dc->SetPen( pen );
#else
   dc->SetPen(*wxBLACK_PEN);
#endif

   // Draw the "X"
   const int s = 6;

   int ls = bev.x + ((bev.width - s) / 2);
   int ts = bev.y + ((bev.height - s) / 2);
   int rs = ls + s;
   int bs = ts + s;

   AColor::Line(*dc, ls,     ts, rs,     bs);
   AColor::Line(*dc, ls + 1, ts, rs + 1, bs);
   AColor::Line(*dc, rs,     ts, ls,     bs);
   AColor::Line(*dc, rs + 1, ts, ls + 1, bs);

   bev.Inflate(-1, -1);
   AColor::BevelTrackInfo(*dc, !down, bev);
}

void TrackInfo::DrawTitleBar(wxDC * dc, const wxRect & rect, Track * t,
                              bool down) const
{
   wxRect bev;
   GetTitleBarRect(rect, bev);
   bev.Inflate(-1, -1);

   // Draw title text
   SetTrackInfoFont(dc);
   wxString titleStr = t->GetName();
   int allowableWidth = kTrackInfoWidth - 38 - kLeftInset;

   wxCoord textWidth, textHeight;
   dc->GetTextExtent(titleStr, &textWidth, &textHeight);
   while (textWidth > allowableWidth) {
      titleStr = titleStr.Left(titleStr.Length() - 1);
      dc->GetTextExtent(titleStr, &textWidth, &textHeight);
   }

   // wxGTK leaves little scraps (antialiasing?) of the
   // characters if they are repeatedly drawn.  This
   // happens when holding down mouse button and moving
   // in and out of the title bar.  So clear it first.
   AColor::MediumTrackInfo(dc, t->GetSelected());
   dc->DrawRectangle(bev);
   dc->DrawText(titleStr, bev.x + 2, bev.y + (bev.height - textHeight) / 2);

   // Pop-up triangle
#ifdef EXPERIMENTAL_THEMING
   wxColour c = theTheme.Colour( clrTrackPanelText );
#else
   wxColour c = *wxBLACK;
#endif

   dc->SetPen(c);
   dc->SetBrush(c);

   int s = 10; // Width of dropdown arrow...height is half of width
   AColor::Arrow(*dc,
                 bev.GetRight() - s - 3, // 3 to offset from right border
                 bev.y + ((bev.height - (s / 2)) / 2),
                 s);

   AColor::BevelTrackInfo(*dc, !down, bev);
}

/// Draw the Mute or the Solo button, depending on the value of solo.
void TrackInfo::DrawMuteSolo(wxDC * dc, const wxRect & rect, Track * t,
                              bool down, bool solo, bool bHasSoloButton) const
{
   wxRect bev;
   if( solo && !bHasSoloButton )
      return;
#ifdef EXPERIMENTAL_MIDI_OUT
   GetMuteSoloRect(rect, bev, solo, bHasSoloButton, t->GetKind());
#else
   GetMuteSoloRect(rect, bev, solo, bHasSoloButton);
#endif
   bev.Inflate(-1, -1);

   if (bev.y + bev.height >= rect.y + rect.height - 19)
      return; // don't draw mute and solo buttons, because they don't fit into track label

   AColor::MediumTrackInfo( dc, t->GetSelected());
   if( solo )
   {
      if( t->GetSolo() )
      {
         AColor::Solo(dc, t->GetSolo(), t->GetSelected());
      }
   }
   else
   {
      if( t->GetMute() )
      {
         AColor::Mute(dc, t->GetMute(), t->GetSelected(), t->GetSolo());
      }
   }
   //(solo) ? AColor::Solo(dc, t->GetSolo(), t->GetSelected()) :
   //    AColor::Mute(dc, t->GetMute(), t->GetSelected(), t->GetSolo());
   dc->SetPen( *wxTRANSPARENT_PEN );//No border!
   dc->DrawRectangle(bev);

   wxCoord textWidth, textHeight;
   wxString str = (solo) ?
      /* i18n-hint: This is on a button that will silence this track.*/
      _("Solo") :
      /* i18n-hint: This is on a button that will silence all the other tracks.*/
      _("Mute");

   SetTrackInfoFont(dc);
   dc->GetTextExtent(str, &textWidth, &textHeight);
   dc->DrawText(str, bev.x + (bev.width - textWidth) / 2, bev.y + (bev.height - textHeight) / 2);

   AColor::BevelTrackInfo(*dc, (solo?t->GetSolo():t->GetMute()) == down, bev);
}

// Draw the minimize button *and* the sync-lock track icon, if necessary.
void TrackInfo::DrawMinimize(wxDC * dc, const wxRect & rect, Track * t, bool down) const
{
   wxRect bev;
   GetMinimizeRect(rect, bev);

   // Clear background to get rid of previous arrow
   AColor::MediumTrackInfo(dc, t->GetSelected());
   dc->DrawRectangle(bev);

#ifdef EXPERIMENTAL_THEMING
   wxColour c = theTheme.Colour(clrTrackPanelText);
   dc->SetBrush(c);
   dc->SetPen(c);
#else
   AColor::Dark(dc, t->GetSelected());
#endif

   AColor::Arrow(*dc,
                 bev.x - 5 + bev.width / 2,
                 bev.y - 2 + bev.height / 2,
                 10,
                 t->GetMinimized());

   AColor::BevelTrackInfo(*dc, !down, bev);
}

void TrackInfo::DrawSliders(wxDC *dc, WaveTrack *t, wxRect rect, bool captured) const
{
   wxRect sliderRect;

   GetGainRect(rect, sliderRect);
   if (sliderRect.y + sliderRect.height < rect.y + rect.height - 19) {
      GainSlider(t, captured)->OnPaint(*dc);
   }

   GetPanRect(rect, sliderRect);
   if (sliderRect.y + sliderRect.height < rect.y + rect.height - 19) {
      PanSlider(t, captured)->OnPaint(*dc);
   }
}

#ifdef EXPERIMENTAL_MIDI_OUT
void TrackInfo::DrawVelocitySlider(wxDC *dc, NoteTrack *t, wxRect rect, bool captured) const
{
   wxRect sliderRect;

   GetVelocityRect(rect, sliderRect);
   if (sliderRect.y + sliderRect.height < rect.y + rect.height - 19) {
      VelocitySlider(t, captured)->OnPaint(*dc);
   }
}
#endif

LWSlider * TrackInfo::GainSlider(WaveTrack *t, bool captured) const
{
   wxRect rect(kLeftInset, t->GetY() - pParent->GetViewInfo()->vpos + kTopInset, 1, t->GetHeight());
   wxRect sliderRect;
   GetGainRect(rect, sliderRect);

   wxPoint pos = sliderRect.GetPosition();
   float gain = t->GetGain();

   mGain->Move(pos);
   mGain->Set(gain);
   mGainCaptured->Move(pos);
   mGainCaptured->Set(gain);

   return (captured ? mGainCaptured : mGain).get();
}

LWSlider * TrackInfo::PanSlider(WaveTrack *t, bool captured) const
{
   wxRect rect(kLeftInset, t->GetY() - pParent->GetViewInfo()->vpos + kTopInset, 1, t->GetHeight());
   wxRect sliderRect;
   GetPanRect(rect, sliderRect);

   wxPoint pos = sliderRect.GetPosition();
   float pan = t->GetPan();

   mPan->Move(pos);
   mPan->Set(pan);
   mPanCaptured->Move(pos);
   mPanCaptured->Set(pan);

   return (captured ? mPanCaptured : mPan).get();
}

#ifdef EXPERIMENTAL_MIDI_OUT
LWSlider * TrackInfo::VelocitySlider(NoteTrack *t, bool captured) const
{
   wxRect rect(kLeftInset, t->GetY() - pParent->GetViewInfo()->vpos + kTopInset, 1, t->GetHeight());
   wxRect sliderRect;
   GetVelocityRect(rect, sliderRect);

   wxPoint pos = sliderRect.GetPosition();
   float velocity = t->GetGain();

   mVelocity->Move(pos);
   mVelocity->Set(velocity);
   mVelocityCaptured->Move(pos);
   mVelocityCaptured->Set(velocity);

   return (captured ? mVelocityCaptured : mVelocity).get();
}
#endif

void TrackInfo::UpdatePrefs()
{
   // Calculation of best font size depends on language, so it should be redone in case
   // the language preference changed.

   int fontSize = 10;
   mFont.Create(fontSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

   int allowableWidth = GetTrackInfoWidth() - 2; // 2 to allow for left/right borders
   int textWidth, textHeight;
   do {
      mFont.SetPointSize(fontSize);
      pParent->GetTextExtent(_("Stereo, 999999Hz"),
                             &textWidth,
                             &textHeight,
                             NULL,
                             NULL,
                             &mFont);
      fontSize--;
   } while (textWidth >= allowableWidth);
}

TrackPanelCellIterator::TrackPanelCellIterator(TrackPanel *trackPanel, bool begin)
   : mPanel(trackPanel)
   , mIter(trackPanel->GetProject())
   , mpCell(begin ? mIter.First() : NULL)
{
}

TrackPanelCellIterator &TrackPanelCellIterator::operator++ ()
{
   // To do, iterate over the other cells that are not tracks
   mpCell = mIter.Next();
   return *this;
}

TrackPanelCellIterator TrackPanelCellIterator::operator++ (int)
{
   TrackPanelCellIterator copy(*this);
   ++ *this;
   return copy;
}

auto TrackPanelCellIterator::operator* () const -> value_type
{
   Track *const pTrack = dynamic_cast<Track*>(mpCell);
   if (!pTrack)
      // to do: handle cells that are not tracks
      return std::make_pair((Track*)nullptr, wxRect());

   // Convert virtual coordinate to physical
   int width;
   mPanel->GetTracksUsableArea(&width, NULL);
   int y = pTrack->GetY() - mPanel->GetViewInfo()->vpos;
   return std::make_pair(
      mpCell,
      wxRect(
         mPanel->GetLeftOffset(),
         y + kTopMargin,
         width,
         pTrack->GetHeight() - (kTopMargin + kBottomMargin)
      )
   );
}

static TrackPanel * TrackPanelFactory(wxWindow * parent,
   wxWindowID id,
   const wxPoint & pos,
   const wxSize & size,
   const std::shared_ptr<TrackList> &tracks,
   ViewInfo * viewInfo,
   TrackPanelListener * listener,
   AdornedRulerPanel * ruler)
{
   wxASSERT(parent); // to justify safenew
   return safenew TrackPanel(
      parent,
      id,
      pos,
      size,
      tracks,
      viewInfo,
      listener,
      ruler);
}


// Declare the static factory function.
// We defined it in the class.
TrackPanel *(*TrackPanel::FactoryFunction)(
              wxWindow * parent,
              wxWindowID id,
              const wxPoint & pos,
              const wxSize & size,
              const std::shared_ptr<TrackList> &tracks,
              ViewInfo * viewInfo,
              TrackPanelListener * listener,
              AdornedRulerPanel * ruler) = TrackPanelFactory;

TrackPanelCell::~TrackPanelCell()
{
}

unsigned TrackPanelCell::HandleWheelRotation
(const TrackPanelMouseEvent &, AudacityProject *)
{
   return RefreshCode::Cancelled;
}

unsigned TrackPanelCell::DoContextMenu
   (const wxRect &, wxWindow*, wxPoint *)
{
   return RefreshCode::RefreshNone;
}

unsigned TrackPanelCell::CaptureKey(wxKeyEvent &event, ViewInfo &, wxWindow *)
{
   event.Skip();
   return RefreshCode::RefreshNone;
}

unsigned TrackPanelCell::KeyDown(wxKeyEvent &event, ViewInfo &, wxWindow *)
{
   event.Skip();
   return RefreshCode::RefreshNone;
}

unsigned TrackPanelCell::KeyUp(wxKeyEvent &event, ViewInfo &, wxWindow *)
{
   event.Skip();
   return RefreshCode::RefreshNone;
}

unsigned TrackPanelCell::Char(wxKeyEvent &event, ViewInfo &, wxWindow *)
{
   event.Skip();
   return RefreshCode::RefreshNone;
}
