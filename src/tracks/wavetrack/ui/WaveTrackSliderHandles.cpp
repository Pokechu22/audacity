#include "WaveTrackSliderHandles.h"

/**********************************************************************

Audacity: A Digital Audio Editor

WaveTrackSliderHandles.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../Audacity.h"
#include "WaveTrackSliderHandles.h"

#include "../../../HitTestResult.h"
#include "../../../MixerBoard.h"
#include "../../../Project.h"
#include "../../../RefreshCode.h"
#include "../../../TrackPanel.h"
#include "../../../UndoManager.h"
#include "../../../WaveTrack.h"
#include "../../../widgets/ASlider.h"

GainSliderHandle::GainSliderHandle()
   : SliderHandle()
{
}

GainSliderHandle::~GainSliderHandle()
{
}

GainSliderHandle &GainSliderHandle::Instance()
{
   static GainSliderHandle instance;
   return instance;
}

float GainSliderHandle::GetValue()
{
   return mpTrack->GetGain();
}

UIHandle::Result GainSliderHandle::SetValue
(AudacityProject *pProject, float newValue)
{
#ifdef EXPERIMENTAL_MIDI_OUT
   if (capturedTrack->GetKind() == Track::Wave)
#endif
   {
      mpTrack->SetGain(newValue);

      // Assume linked track is wave or null
      const auto link = static_cast<WaveTrack*>(mpTrack->GetLink());
      if (link)
         link->SetGain(newValue);

      MixerBoard *const pMixerBoard = pProject->GetMixerBoard();
      if (pMixerBoard)
         pMixerBoard->UpdateGain(mpTrack);
   }
#ifdef EXPERIMENTAL_MIDI_OUT
   else {
      // mpTrack is not wave, so assume it is note
      static_cast<NoteTrack *>(mpTrack)->SetGain(newValue);
#ifdef EXPERIMENTAL_MIXER_BOARD
      if (pMixerBoard)
         // probably should modify UpdateGain to take a track that is
         // either a WaveTrack or a NoteTrack.
         pMixerBoard->UpdateGain(static_cast<WaveTrack*>(mpTrack));
#endif
   }
#endif

   return RefreshCode::RefreshNone;
}

UIHandle::Result GainSliderHandle::CommitChanges
(const wxMouseEvent &, AudacityProject *pProject)
{
#ifdef EXPERIMENTAL_MIDI_OUT
   if (capturedTrack->GetKind() == Track::Wave)
#endif
   {
      pProject->PushState(_("Moved gain slider"), _("Gain"), UndoPush::CONSOLIDATE);
   }
#ifdef EXPERIMENTAL_MIDI_OUT
   else {
      pProject->PushState(_("Moved velocity slider"), _("Velocity"), true);
   }
#endif

   return RefreshCode::RefreshCell;
}

HitTestResult GainSliderHandle::HitTest
(const wxMouseEvent &event, const wxRect &rect,
 const AudacityProject *pProject, Track *pTrack)
{
   if (!event.Button(wxMOUSE_BTN_LEFT))
      return HitTestResult();

   wxRect sliderRect;
   TrackInfo::GetGainRect(rect, sliderRect);
   if (sliderRect.Contains(event.m_x, event.m_y)) {
      WaveTrack *const wavetrack = static_cast<WaveTrack*>(pTrack);
      LWSlider *const slider =
         pProject->GetTrackPanel()->GetTrackInfo()->GainSlider(wavetrack, true);
      Instance().mpSlider = slider;
      Instance().mpTrack = static_cast<WaveTrack*>(pTrack);
      return HitTestResult(
         Preview(),
         &Instance()
      );
   }
   else
      return HitTestResult();
}

////////////////////////////////////////////////////////////////////////////////

PanSliderHandle::PanSliderHandle()
   : SliderHandle()
{
}

PanSliderHandle::~PanSliderHandle()
{
}

PanSliderHandle &PanSliderHandle::Instance()
{
   static PanSliderHandle instance;
   return instance;
}

float PanSliderHandle::GetValue()
{
   return mpTrack->GetPan();
}

UIHandle::Result PanSliderHandle::SetValue(AudacityProject *pProject, float newValue)
{
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   bool panZero = false;
#endif

#ifdef EXPERIMENTAL_MIDI_OUT
   if (capturedTrack->GetKind() == Track::Wave)
#endif
   {
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
      panZero = static_cast<WaveTrack*>(mpTrack)->SetPan(newValue);
#else
      mpTrack->SetPan(newValue);
#endif

      // Assume linked track is wave or null
      const auto link = static_cast<WaveTrack*>(mpTrack->GetLink());
      if (link)
         link->SetPan(newValue);

      MixerBoard *const pMixerBoard = pProject->GetMixerBoard();
      if (pMixerBoard)
         pMixerBoard->UpdatePan(mpTrack);
   }

   using namespace RefreshCode;
   Result result = RefreshNone;
#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   result |= FixScrollbars;
#endif
   return result;
}

UIHandle::Result PanSliderHandle::CommitChanges
(const wxMouseEvent &, AudacityProject *pProject)
{
#ifdef EXPERIMENTAL_MIDI_OUT
   if (capturedTrack->GetKind() == Track::Wave)
#endif
   {
      pProject->PushState(_("Moved pan slider"), _("Pan"), UndoPush::CONSOLIDATE);
   }
#ifdef EXPERIMENTAL_MIDI_OUT
   else {
      pProject->PushState(_("Moved velocity slider"), _("Velocity"), UndoPush::CONSOLIDATE);
   }
#endif

   return RefreshCode::RefreshCell;
}

HitTestResult PanSliderHandle::HitTest
(const wxMouseEvent &event, const wxRect &rect,
 const AudacityProject *pProject, Track *pTrack)
{
   if (!event.Button(wxMOUSE_BTN_LEFT))
      return HitTestResult();

   wxRect sliderRect;
   TrackInfo::GetPanRect(rect, sliderRect);
   if (sliderRect.Contains(event.m_x, event.m_y)) {
      WaveTrack *const wavetrack = static_cast<WaveTrack*>(pTrack);
      LWSlider *const slider =
         pProject->GetTrackPanel()->GetTrackInfo()->PanSlider(wavetrack, true);
      Instance().mpSlider = slider;
      Instance().mpTrack = static_cast<WaveTrack*>(pTrack);
      return HitTestResult(
         Preview(),
         &Instance()
      );
   }
   else
      return HitTestResult();
}
