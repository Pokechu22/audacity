/**********************************************************************

Audacity: A Digital Audio Editor

TimeShiftHandle.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_TIMESHIFT_HANDLE__
#define __AUDACITY_TIMESHIFT_HANDLE__

#include "../../UIHandle.h"

#include "../../MemoryX.h"

#include "../../Snap.h"
#include "../../Track.h"

struct HitTestResult;
class WaveClip;

class TimeShiftHandle : public UIHandle
{
   TimeShiftHandle();
   TimeShiftHandle(const TimeShiftHandle&);
   TimeShiftHandle &operator=(const TimeShiftHandle&);
   static TimeShiftHandle& Instance();
   static HitTestPreview HitPreview
      (const AudacityProject *pProject, bool unsafe);

public:
   static HitTestResult HitAnywhere(const AudacityProject *pProject);
   static HitTestResult HitTest
      (const wxMouseEvent &event, const wxRect &rect, const AudacityProject *pProject);

   virtual ~TimeShiftHandle();

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
       wxDC * dc, const wxRegion &, const wxRect &panelRect);

   bool StopsOnKeystroke() override { return true; }

private:
   Track *mCapturedTrack;
   wxRect mRect;

   // non-NULL only if click was in a WaveTrack and without Shift key:
   WaveClip *mCapturedClip;

   TrackClipArray mCapturedClipArray;
   TrackArray mTrackExclusions;
   bool mCapturedClipIsSelection;

   // The amount that clips are sliding horizontally; this allows
   // us to undo the slide and then slide it by another amount
   double mHSlideAmount;

   bool mDidSlideVertically;
   bool mSlideUpDownOnly;

   bool mSnapPreferRightEdge;

   int mMouseClickX;

   // Handles snapping the selection boundaries or track boundaries to
   // line up with existing tracks or labels.  mSnapLeft and mSnapRight
   // are the horizontal index of pixels to display user feedback
   // guidelines so the user knows when such snapping is taking place.
   std::unique_ptr<SnapManager> mSnapManager;
   wxInt64 mSnapLeft;
   wxInt64 mSnapRight;
};

#endif
