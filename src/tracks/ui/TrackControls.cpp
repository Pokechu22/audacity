/**********************************************************************

Audacity: A Digital Audio Editor

TrackControls.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../Audacity.h"
#include "TrackControls.h"
#include "TrackButtonHandles.h"
#include "../../HitTestResult.h"
#include "../../RefreshCode.h"
#include "../../TrackPanel.h"
#include "../../TrackPanelMouseEvent.h"

int TrackControls::gCaptureState;

TrackControls::~TrackControls()
{
}

HitTestResult TrackControls::HitTest1
(const TrackPanelMouseEvent &evt,
 const AudacityProject *)
{
   const wxMouseEvent &event = evt.event;
   const wxRect &rect = evt.rect;
   HitTestResult result;

   if (NULL != (result = CloseButtonHandle::HitTest(event, rect)).handle)
      return result;

   if (NULL != (result = MenuButtonHandle::HitTest(event, rect, this)).handle)
      return result;

   // VJ: Check sync-lock icon and the blank area to the left of the minimize
   // button.
   // Have to do it here, because if track is shrunk such that these areas
   // occlude controls,
   // e.g., mute/solo, don't want positive hit tests on the buttons.
   // Only result of doing so is to select the track. Don't care whether isleft.
   const bool bTrackSelClick =
      TrackInfo::TrackSelFunc(GetTrack(), rect, event.m_x, event.m_y);

   if (!bTrackSelClick) {
      if (NULL != (result = MinimizeButtonHandle::HitTest(event, rect)).handle)
         return result;
   }

   return result;
}

HitTestResult TrackControls::HitTest2
(const TrackPanelMouseEvent &,
 const AudacityProject *)
{
   return HitTestResult();
}

HitTestResult TrackControls::HitTest
(const TrackPanelMouseEvent &evt,
 const AudacityProject *pProject)
{
   HitTestResult result = HitTest1(evt, pProject);
   if (!result.handle)
      result = HitTest2(evt, pProject);
   return result;
}

Track *TrackControls::FindTrack()
{
   return GetTrack();
}

class TrackMenuTable : public PopupMenuTable
{
   TrackMenuTable() : mpData(NULL) {}
   DECLARE_POPUP_MENU(TrackMenuTable);

public:
   static TrackMenuTable &Instance();

private:

   virtual void InitMenu(Menu*, void *pUserData)
   {
      mpData = static_cast<TrackControls::InitMenuData*>(pUserData);
   }

   virtual void DestroyMenu()
   {
      mpData = NULL;
   }

   TrackControls::InitMenuData *mpData;
};

TrackMenuTable &TrackMenuTable::Instance()
{
   static TrackMenuTable instance;
   return instance;
}

BEGIN_POPUP_MENU(TrackMenuTable)
END_POPUP_MENU()

unsigned TrackControls::DoContextMenu
   (const wxRect &rect, wxWindow *pParent, wxPoint *)
{
   wxRect buttonRect;
   TrackInfo::GetTitleBarRect(rect, buttonRect);

   InitMenuData data;
   data.pParent = pParent;
   data.pTrack = mpTrack;
   data.result = RefreshCode::RefreshNone;

   TrackMenuTable *const pTable = &TrackMenuTable::Instance();
   std::unique_ptr<PopupMenuTable::Menu>
      pMenu(PopupMenuTable::BuildMenu(pParent, pTable, &data));

   PopupMenuTable *const pExtension = GetMenuExtension(mpTrack);
   if (pExtension)
      pMenu->Extend(pExtension);

   pParent->PopupMenu
      (pMenu.get(), buttonRect.x + 1, buttonRect.y + buttonRect.height + 1);

   return data.result;
}
