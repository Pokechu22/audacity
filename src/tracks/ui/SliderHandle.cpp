/**********************************************************************

Audacity: A Digital Audio Editor

SliderHandle.cpp

Paul Licameli

**********************************************************************/

#include "../../Audacity.h"
#include "SliderHandle.h"
#include "../../widgets/ASlider.h"
#include "../../HitTestResult.h"
#include "../../RefreshCode.h"
#include "../../TrackPanelMouseEvent.h"

SliderHandle::SliderHandle()
   : mpTrack(NULL)
   , mpSlider(NULL)
   , mStartingValue(0.0f)
{
}

SliderHandle::~SliderHandle()
{
}

HitTestPreview SliderHandle::Preview()
{
   // No special message or cursor
   return HitTestPreview();
}

UIHandle::Result SliderHandle::Click
(const TrackPanelMouseEvent &evt, AudacityProject *)
{
   wxMouseEvent &event = evt.event;
   using namespace RefreshCode;
   if (!event.Button(wxMOUSE_BTN_LEFT))
      return Cancelled;

   // Come here for left click or double click
   mStartingValue = GetValue();
   mpSlider->Set(mStartingValue);
   mpSlider->OnMouseEvent(event);
   return RefreshCell;
}

UIHandle::Result SliderHandle::Drag
(const TrackPanelMouseEvent &evt, AudacityProject *pProject)
{
   wxMouseEvent &event = evt.event;
   using namespace RefreshCode;
   mpSlider->OnMouseEvent(event);
   const float newValue = mpSlider->Get();

   // Make a non-permanent change to the project data:
   SetValue(pProject, newValue);

   return RefreshCell;
}

HitTestPreview SliderHandle::Preview
(const TrackPanelMouseEvent &, const AudacityProject *)
{
   // No special message or cursor
   return HitTestPreview();
}

UIHandle::Result SliderHandle::Release
(const TrackPanelMouseEvent &evt, AudacityProject *pProject,
 wxWindow *)
{
   using namespace RefreshCode;
   wxMouseEvent &event = evt.event;
   mpSlider->OnMouseEvent(event);
   const float newValue = mpSlider->Get();

   Result result = RefreshCell;

   // Commit changes to the project data:
   result |= SetValue(pProject, newValue);
   result |= CommitChanges(event, pProject);
   return result;
}

UIHandle::Result SliderHandle::Cancel(AudacityProject *pProject)
{
   wxMouseEvent event(wxEVT_LEFT_UP);
   mpSlider->OnMouseEvent(event);

   // Undo un-committed changes to project data:
   SetValue(pProject, mStartingValue);
   return RefreshCode::RefreshCell;
}
