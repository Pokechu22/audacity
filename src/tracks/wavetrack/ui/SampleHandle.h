/**********************************************************************

Audacity: A Digital Audio Editor

SampleHandle.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_SAMPLE_HANDLE__
#define __AUDACITY_SAMPLE_HANDLE__

#include "../../../UIHandle.h"
#include "audacity/Types.h"

#include <wx/gdicmn.h>

struct HitTestResult;
class Track;
class ViewInfo;
class WaveTrack;

class SampleHandle : public UIHandle
{
   SampleHandle();
   SampleHandle(const SampleHandle&);
   SampleHandle &operator=(const SampleHandle&);
   static SampleHandle& Instance();
   static HitTestPreview HitPreview
      (const wxMouseEvent &event, const AudacityProject *pProject, bool unsafe);

public:
   static HitTestResult HitAnywhere
      (const wxMouseEvent &event, const AudacityProject *pProject);
   static HitTestResult HitTest
      (const wxMouseEvent &event, const wxRect &rect,
       const AudacityProject *pProject, Track *pTrack);

   virtual ~SampleHandle();

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

   bool StopsOnKeystroke() override { return true; }

private:
   float FindSampleEditingLevel
      (const wxMouseEvent &event, const ViewInfo &viewInfo, double t0);

   WaveTrack *mClickedTrack;
   wxRect mRect;

   int mClickedTrackTop;
   sampleCount mClickedStartSample;
   sampleCount mLastDragSample;
   float mLastDragSampleValue;
   bool mAltKey;
};

#endif
