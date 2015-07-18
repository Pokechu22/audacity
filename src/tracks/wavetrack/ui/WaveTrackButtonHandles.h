/**********************************************************************

Audacity: A Digital Audio Editor

WavelTrackButtonHandles.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_WAVE_TRACK_BUTTON_HANDLES__
#define __AUDACITY_WAVE_TRACK_BUTTON_HANDLES__

#include "../../ui/ButtonHandle.h"

struct HitTestResult;

class MuteButtonHandle : public ButtonHandle
{
   MuteButtonHandle(const MuteButtonHandle&);
   MuteButtonHandle &operator=(const MuteButtonHandle&);

   MuteButtonHandle();
   virtual ~MuteButtonHandle();
   static MuteButtonHandle& Instance();

protected:
   virtual Result CommitChanges
      (const wxMouseEvent &event, AudacityProject *pProject, wxWindow *pParent);

   bool StopsOnKeystroke () override { return true; }

public:
   static HitTestResult HitTest
      (const wxMouseEvent &event, const wxRect &rect, const AudacityProject *pProject);
};

////////////////////////////////////////////////////////////////////////////////

class SoloButtonHandle : public ButtonHandle
{
   SoloButtonHandle(const SoloButtonHandle&);
   SoloButtonHandle &operator=(const SoloButtonHandle&);

   SoloButtonHandle();
   virtual ~SoloButtonHandle();
   static SoloButtonHandle& Instance();

protected:
   virtual Result CommitChanges
      (const wxMouseEvent &event, AudacityProject *pProject, wxWindow *pParent);

   bool StopsOnKeystroke () override { return true; }

public:
   static HitTestResult HitTest
      (const wxMouseEvent &event, const wxRect &rect, const AudacityProject *pProject);
};

#endif
