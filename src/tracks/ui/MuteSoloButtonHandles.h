/**********************************************************************

Audacity: A Digital Audio Editor

MuteSoloButtonHandles.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_MUTE_SOLO_BUTTON_HANDLES__
#define __AUDACITY_MUTE_SOLO_BUTTON_HANDLES__

#include "ButtonHandle.h"

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

   bool StopsOnKeystroke() override { return true; }

public:
   static HitTestResult HitTest
      (const wxMouseEvent &event, const wxRect &rect, const AudacityProject *pProject, int trackKind);
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

   bool StopsOnKeystroke() override { return true; }

public:
   static HitTestResult HitTest
      (const wxMouseEvent &event, const wxRect &rect, const AudacityProject *pProject, int trackKind);
};

#endif
