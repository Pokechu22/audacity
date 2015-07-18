/**********************************************************************

Audacity: A Digital Audio Editor

WavelTrackSliderHandles.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_WAVE_TRACK_SLIDER_HANDLES__
#define __AUDACITY_WAVE_TRACK_SLIDER_HANDLES__

#include "../../ui/SliderHandle.h"

class Track;

struct HitTestResult;

class GainSliderHandle : public SliderHandle
{
   GainSliderHandle(const GainSliderHandle&);
   GainSliderHandle &operator=(const GainSliderHandle&);

   GainSliderHandle();
   virtual ~GainSliderHandle();
   static GainSliderHandle& Instance();

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

////////////////////////////////////////////////////////////////////////////////

class PanSliderHandle : public SliderHandle
{
   PanSliderHandle(const PanSliderHandle&);
   PanSliderHandle &operator=(const PanSliderHandle&);

   PanSliderHandle();
   virtual ~PanSliderHandle();
   static PanSliderHandle& Instance();

protected:
   virtual float GetValue();
   virtual Result SetValue(AudacityProject *pProject, float newValue);
   virtual Result CommitChanges
      (const wxMouseEvent &event, AudacityProject *pProject);

public:
   static HitTestResult HitTest
      (const wxMouseEvent &event, const wxRect &rect,
       const AudacityProject *pProject, Track *pTrack);
};

#endif
