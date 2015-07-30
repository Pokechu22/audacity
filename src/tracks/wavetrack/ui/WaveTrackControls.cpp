/**********************************************************************

Audacity: A Digital Audio Editor

WaveTrackControls.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../Audacity.h"
#include "WaveTrackControls.h"
#include "WaveTrackButtonHandles.h"
#include "WaveTrackSliderHandles.h"

#include "../../../HitTestResult.h"
#include "../../../WaveTrack.h"
#include "../../../TrackPanel.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../../widgets/PopupMenuTable.h"

WaveTrackControls::WaveTrackControls()
{
}

WaveTrackControls &WaveTrackControls::Instance()
{
   static WaveTrackControls instance;
   return instance;
}

WaveTrackControls::~WaveTrackControls()
{
}


HitTestResult WaveTrackControls::HitTest
(const TrackPanelMouseEvent & evt,
 const AudacityProject *pProject)
{
   {
      HitTestResult result = TrackControls::HitTest1(evt, pProject);
      if (result.handle)
         return result;
   }

   const wxMouseEvent &event = evt.event;
   const wxRect &rect = evt.rect;
   if (event.Button(wxMOUSE_BTN_LEFT)) {
      // VJ: Check sync-lock icon and the blank area to the left of the minimize
      // button.
      // Have to do it here, because if track is shrunk such that these areas
      // occlude controls,
      // e.g., mute/solo, don't want positive hit tests on the buttons.
      // Only result of doing so is to select the track. Don't care whether isleft.
      const bool bTrackSelClick =
         TrackInfo::TrackSelFunc(GetTrack(), rect, event.m_x, event.m_y);

      if (!bTrackSelClick) {
         if (mpTrack->GetKind() == Track::Wave) {
            HitTestResult result;
            if (NULL != (result = MuteButtonHandle::HitTest(event, rect, pProject)).handle)
               return result;

            if (NULL != (result = SoloButtonHandle::HitTest(event, rect, pProject)).handle)
               return result;

            if (NULL != (result =
               GainSliderHandle::HitTest(event, rect, pProject, mpTrack)).handle)
               return result;

            if (NULL != (result =
               PanSliderHandle::HitTest(event, rect, pProject, mpTrack)).handle)
               return result;
         }
      }
   }

   return TrackControls::HitTest2(evt, pProject);
}

class WaveTrackMenuTable : public PopupMenuTable
{
   WaveTrackMenuTable() : mpData(NULL) {}
   DECLARE_POPUP_MENU(WaveTrackMenuTable);

public:
   static WaveTrackMenuTable &Instance();

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

WaveTrackMenuTable &WaveTrackMenuTable::Instance()
{
   static WaveTrackMenuTable instance;
   return instance;
}

BEGIN_POPUP_MENU(WaveTrackMenuTable)
END_POPUP_MENU()

PopupMenuTable *WaveTrackControls::GetMenuExtension(Track*)
{
   return &WaveTrackMenuTable::Instance();
}
