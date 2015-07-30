/**********************************************************************

Audacity: A Digital Audio Editor

WavelTrackButtonHandles.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_TRACK_BUTTON_HANDLES__
#define __AUDACITY_TRACK_BUTTON_HANDLES__

#include "../ui/ButtonHandle.h"

struct HitTestResult;

class MinimizeButtonHandle : public ButtonHandle
{
   MinimizeButtonHandle(const MinimizeButtonHandle&);
   MinimizeButtonHandle &operator=(const MinimizeButtonHandle&);

   MinimizeButtonHandle();
   virtual ~MinimizeButtonHandle();
   static MinimizeButtonHandle& Instance();

protected:
   virtual Result CommitChanges
      (const wxMouseEvent &event, AudacityProject *pProject, wxWindow *pParent);

public:
   static HitTestResult HitTest(const wxMouseEvent &event, const wxRect &rect);
};

////////////////////////////////////////////////////////////////////////////////

class CloseButtonHandle : public ButtonHandle
{
   CloseButtonHandle(const CloseButtonHandle&);
   CloseButtonHandle &operator=(const CloseButtonHandle&);

   CloseButtonHandle();
   virtual ~CloseButtonHandle();
   static CloseButtonHandle& Instance();

protected:
   virtual Result CommitChanges
      (const wxMouseEvent &event, AudacityProject *pProject, wxWindow *pParent);

   bool StopsOnKeystroke () override { return true; }
   
public:
   static HitTestResult HitTest(const wxMouseEvent &event, const wxRect &rect);
};

////////////////////////////////////////////////////////////////////////////////

#include <wx/event.h>
#include "../../widgets/PopupMenuTable.h"

class MenuButtonHandle : public ButtonHandle
{
   MenuButtonHandle(const MenuButtonHandle&);
   MenuButtonHandle &operator=(const MenuButtonHandle&);

   MenuButtonHandle();
   virtual ~MenuButtonHandle();
   static MenuButtonHandle& Instance();

protected:
   virtual Result CommitChanges
      (const wxMouseEvent &event, AudacityProject *pProject, wxWindow *pParent);

public:
   static HitTestResult HitTest
      (const wxMouseEvent &event, const wxRect &rect, TrackPanelCell *pCell);

private:
   TrackPanelCell *mpCell;
};

#endif
