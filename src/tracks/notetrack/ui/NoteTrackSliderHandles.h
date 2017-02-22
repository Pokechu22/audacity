/**********************************************************************

Audacity: A Digital Audio Editor

NoteTrackSliderHandles.h

Poke based from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_WAVE_TRACK_SLIDER_HANDLES__
#define __AUDACITY_WAVE_TRACK_SLIDER_HANDLES__

#include "../../../Audacity.h"
#include "../../../Experimental.h"

#ifdef EXPERIMENTAL_MIDI_OUT

#include "../../ui/SliderHandle.h"

class Track;

struct HitTestResult;

class VelocitySliderHandle : public SliderHandle
{
   VelocitySliderHandle(const VelocitySliderHandle&);
   VelocitySliderHandle &operator=(const VelocitySliderHandle&);

   VelocitySliderHandle();
   virtual ~VelocitySliderHandle();
   static VelocitySliderHandle& Instance();

protected:
   virtual float GetValue();
   virtual Result SetValue
      (AudacityProject *pProject, float newValue);
   virtual Result CommitChanges
      (const wxMouseEvent &event, AudacityProject *pProject);

public:
   static HitTestResult HitTest
      (const wxMouseEvent &event, const wxRect &rect,
       const AudacityProject *pProject, Track *pTrack);
};
#endif
#endif
