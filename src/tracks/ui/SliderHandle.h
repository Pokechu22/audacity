/**********************************************************************

Audacity: A Digital Audio Editor

SliderHandle.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_SLIDER_HANDLE__
#define __AUDACITY_SLIDER_HANDLE__

#include "../../UIHandle.h"

class LWSlider;
class Track;

class SliderHandle : public UIHandle
{
   SliderHandle(const SliderHandle&);
   SliderHandle &operator=(const SliderHandle&);

protected:
   SliderHandle();
   virtual ~SliderHandle();

   // These new abstract virtuals simplify the duties of further subclasses.
   // This class will decide whether to refresh the clicked cell for slider state
   // change.
   // Subclass can decide to refresh other things and the results will be ORed.
   virtual float GetValue() = 0;
   virtual Result SetValue(AudacityProject *pProject, float newValue) = 0;
   virtual Result CommitChanges
      (const wxMouseEvent &event, AudacityProject *pProject) = 0;

   // Return the same as for the other overload of Preview; useful for derived
   // class to define a hit test
   static HitTestPreview Preview();

   // These are overrides of the base abstract class:

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

   // Derived track is expected to set these two before Click():
   Track *mpTrack;
   LWSlider *mpSlider;

   float mStartingValue;
};

#endif
