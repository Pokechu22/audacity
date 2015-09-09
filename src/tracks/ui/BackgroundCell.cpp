/**********************************************************************

Audacity: A Digital Audio Editor

BackgroundCell.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../Audacity.h"
#include "BackgroundCell.h"

#include "../../HitTestResult.h"
#include "../../Project.h"
#include "../../RefreshCode.h"
#include "../../TrackPanel.h"
#include "../../TrackPanelMouseEvent.h"
#include "../../UIHandle.h"

#include <wx/cursor.h>
#include <wx/event.h>

namespace
{
   // Define this, just so the click to deselect can dispatch here
   class BackgroundHandle : public UIHandle
   {
      BackgroundHandle() {}
      BackgroundHandle(const BackgroundHandle&);
      BackgroundHandle &operator=(const BackgroundHandle&);

   public:

      static BackgroundHandle& Instance()
      {
         static BackgroundHandle instance;
         return instance;
      }

      static HitTestPreview HitPreview()
      {
         static wxCursor arrowCursor(wxCURSOR_ARROW);
         return HitTestPreview(
            wxString(),
            &arrowCursor
         );
      }

      static HitTestResult HitAnywhere()
      {
         return HitTestResult(
            HitPreview(),
            &BackgroundHandle::Instance()
         );
      }

      virtual ~BackgroundHandle()
      {}

      virtual Result Click
         (const TrackPanelMouseEvent &evt, AudacityProject *pProject)
      {
         using namespace RefreshCode;
         const wxMouseEvent &event = evt.event;
         // Do not start a drag
         Result result = Cancelled;

         // AS: If the user clicked outside all tracks, make nothing
         //  selected.
         if ((event.ButtonDown() || event.ButtonDClick())) {
            pProject->GetTrackPanel()->SelectNone();
            result |= RefreshAll;
         }

         return result;
      }

      virtual Result Drag
         (const TrackPanelMouseEvent &, AudacityProject *)
      { return RefreshCode::RefreshNone; }

      virtual HitTestPreview Preview
         (const TrackPanelMouseEvent &, const AudacityProject *)
      { return HitPreview(); }

      virtual Result Release
         (const TrackPanelMouseEvent &, AudacityProject *,
          wxWindow *)
      { return RefreshCode::RefreshNone; }

      virtual Result Cancel(AudacityProject *)
      { return RefreshCode::RefreshNone; }
   };
}

BackgroundCell::~BackgroundCell()
{
}

HitTestResult BackgroundCell::HitTest
(const TrackPanelMouseEvent &,
 const AudacityProject *)
{
   return BackgroundHandle::HitAnywhere();
}

Track *BackgroundCell::FindTrack()
{
   return NULL;
}

