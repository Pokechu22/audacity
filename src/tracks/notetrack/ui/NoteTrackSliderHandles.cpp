/**********************************************************************

Audacity: A Digital Audio Editor

NoteTrackSliderHandles.cpp

Poke based from TrackPanel.cpp

**********************************************************************/

#include "../../../Audacity.h"
#include "../../../Experimental.h"

#ifdef EXPERIMENTAL_MIDI_OUT
#include "NoteTrackSliderHandles.h"

#include "../../../HitTestResult.h"
#include "../../../MixerBoard.h"
#include "../../../Project.h"
#include "../../../RefreshCode.h"
#include "../../../TrackPanel.h"
#include "../../../UndoManager.h"
#include "../../../NoteTrack.h"
#include "../../../widgets/ASlider.h"

VelocitySliderHandle::VelocitySliderHandle()
   : SliderHandle()
{
}

VelocitySliderHandle::~VelocitySliderHandle()
{
}

VelocitySliderHandle &VelocitySliderHandle::Instance()
{
   static VelocitySliderHandle instance;
   return instance;
}

float VelocitySliderHandle::GetValue()
{
   return static_cast<NoteTrack *>(mpTrack)->GetVelocity();
}

UIHandle::Result VelocitySliderHandle::SetValue
(AudacityProject *pProject, float newValue)
{
   static_cast<NoteTrack *>(mpTrack)->SetVelocity(newValue);
   MixerBoard *const pMixerBoard = pProject->GetMixerBoard();
   if (pMixerBoard)
      pMixerBoard->UpdateVelocity(static_cast<NoteTrack *>(mpTrack));

   return RefreshCode::RefreshNone;
}

UIHandle::Result VelocitySliderHandle::CommitChanges
(const wxMouseEvent &, AudacityProject *pProject)
{
   pProject->PushState(_("Moved velocity slider"), _("Velocity"), UndoPush::CONSOLIDATE);

   return RefreshCode::RefreshCell;
}

HitTestResult VelocitySliderHandle::HitTest
(const wxMouseEvent &event, const wxRect &rect,
 const AudacityProject *pProject, Track *pTrack)
{
   if (!event.Button(wxMOUSE_BTN_LEFT))
      return HitTestResult();

   wxRect sliderRect;
   TrackInfo::GetVelocityRect(rect, sliderRect);
   if (sliderRect.Contains(event.m_x, event.m_y)) {
      NoteTrack *const notetrack = static_cast<NoteTrack*>(pTrack);
      LWSlider *const slider =
         pProject->GetTrackPanel()->GetTrackInfo()->VelocitySlider(notetrack, true);
      Instance().mpSlider = slider;
      Instance().mpTrack = notetrack;
      return HitTestResult(
         Preview(),
         &Instance()
      );
   }
   else
      return HitTestResult();
}
#endif
