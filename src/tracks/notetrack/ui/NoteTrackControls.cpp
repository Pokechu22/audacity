/**********************************************************************

Audacity: A Digital Audio Editor

NoteTrackControls.cpp

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#include "../../../Audacity.h"
#include "NoteTrackControls.h"
#include "../../../HitTestResult.h"
#include "../../../NoteTrack.h"
#include "../../../widgets/PopupMenuTable.h"
#include "../../../Project.h"
#include "../../../RefreshCode.h"
#include "../../../TrackPanel.h"
#include "../../../TrackPanelMouseEvent.h"
#include "../../../UIHandle.h"

///////////////////////////////////////////////////////////////////////////////
class NoteTrackClickHandle : public UIHandle
{
   NoteTrackClickHandle(const NoteTrackClickHandle&);
   NoteTrackClickHandle &operator=(const NoteTrackClickHandle&);
   NoteTrackClickHandle();
   virtual ~NoteTrackClickHandle();
   static NoteTrackClickHandle& Instance();

public:
   static HitTestResult HitTest
      (const wxMouseEvent &event, const wxRect &rect, Track *pTrack);

protected:
   virtual Result Click
      (const TrackPanelMouseEvent &event, AudacityProject *pProject);

   virtual Result Drag
      (const TrackPanelMouseEvent &event, AudacityProject *pProject);

   virtual HitTestPreview Preview
      (const TrackPanelMouseEvent &event, const AudacityProject *pProject);

   virtual Result Release
      (const TrackPanelMouseEvent &event, AudacityProject *pProject,
       wxWindow *pParent);

   virtual Result Cancel(AudacityProject *pProject);

   void OnProjectChange(AudacityProject *pProject) override;

   NoteTrack *mpTrack;
   wxRect mRect;
};

NoteTrackClickHandle::NoteTrackClickHandle()
   : mpTrack(NULL), mRect()
{
}

NoteTrackClickHandle::~NoteTrackClickHandle()
{
}

NoteTrackClickHandle &NoteTrackClickHandle::Instance()
{
   static NoteTrackClickHandle instance;
   return instance;
}

HitTestResult NoteTrackClickHandle::HitTest
   (const wxMouseEvent &event, const wxRect &rect, Track *pTrack)
{
   wxRect midiRect;
   TrackInfo::GetTrackControlsRect(rect, midiRect);
   if (pTrack->GetKind() == Track::Note &&
       midiRect.Contains(event.m_x, event.m_y)) {
         Instance().mpTrack = static_cast<NoteTrack*>(pTrack);
         Instance().mRect = rect;
         return HitTestResult(
            HitTestPreview(),
            &Instance()
         );
   }
   else
      return HitTestResult();
}

UIHandle::Result NoteTrackClickHandle::Click
(const TrackPanelMouseEvent &, AudacityProject *)
{
   return RefreshCode::RefreshNone;
}

UIHandle::Result NoteTrackClickHandle::Drag
(const TrackPanelMouseEvent &, AudacityProject *)
{
   return RefreshCode::RefreshNone;
}

HitTestPreview NoteTrackClickHandle::Preview
(const TrackPanelMouseEvent &, const AudacityProject *)
{
   // No special message or cursor
   return HitTestPreview();
}

UIHandle::Result NoteTrackClickHandle::Release
(const TrackPanelMouseEvent &evt, AudacityProject *, wxWindow *)
{
   using namespace RefreshCode;

   if (!mpTrack)
      return RefreshNone;

   const wxMouseEvent &event = evt.event;
   if (mpTrack->LabelClick(mRect, event.m_x, event.m_y,
      event.Button(wxMOUSE_BTN_RIGHT))) {
      // No undo items needed??
      return RefreshAll;
   }
   return RefreshNone;
}

UIHandle::Result NoteTrackClickHandle::Cancel(AudacityProject *)
{
   return RefreshCode::RefreshNone;
}

void NoteTrackClickHandle::OnProjectChange(AudacityProject *pProject)
{
   if (! pProject->GetTracks()->Contains(mpTrack)) {
      mpTrack = nullptr;
      mRect = {};
   }

   UIHandle::OnProjectChange(pProject);
}

///////////////////////////////////////////////////////////////////////////////
NoteTrackControls::NoteTrackControls()
{
}

NoteTrackControls &NoteTrackControls::Instance()
{
   static NoteTrackControls instance;
   return instance;
}

NoteTrackControls::~NoteTrackControls()
{
}

HitTestResult NoteTrackControls::HitTest
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
   if (event.ButtonDown() || event.ButtonDClick()) {
      // VJ: Check sync-lock icon and the blank area to the left of the minimize
      // button.
      // Have to do it here, because if track is shrunk such that these areas
      // occlude controls,
      // e.g., mute/solo, don't want positive hit tests on the buttons.
      // Only result of doing so is to select the track. Don't care whether isleft.
      const bool bTrackSelClick =
         TrackInfo::TrackSelFunc(GetTrack(), rect, event.m_x, event.m_y);

      if (!bTrackSelClick) {
#ifdef USE_MIDI
         // DM: If it's a NoteTrack, it has special controls
         if (mpTrack->GetKind() == Track::Note) {

            HitTestResult result;
            if (NULL != (result =
               NoteTrackClickHandle::HitTest(event, rect, GetTrack())).handle)
               return result;

#ifdef EXPERIMENTAL_MIDI_OUT
            // TODO:  rewrite the following analogously to WaveTrackControls::HitTest
            // So that this can be compiled again

               // this is an awful hack: make a NEW rectangle at an offset because
               // MuteSoloFunc thinks buttons are located below some text, e.g.
               // "Mono, 44100Hz 32-bit float", but this is not true for a Note track
               wxRect muteSoloRect(rect);
               muteSoloRect.y -= 34; // subtract the height of wave track text
               if (MuteSoloFunc(t, muteSoloRect, event.m_x, event.m_y, false) ||
                   MuteSoloFunc(t, muteSoloRect, event.m_x, event.m_y, true))
                  return;

               // this is a similar hack: GainFunc expects a Wave track slider, so it's
               // looking in the wrong place. We pass it a bogus rectangle created when
               // the slider was placed to "fake" GainFunc into finding the slider in
               // its actual location.
               if (GainFunc(t, ((NoteTrack *) t)->GetGainPlacementRect(),
                            event, event.m_x, event.m_y))
                  return;
#endif
         }
#endif
      }
   }

   return TrackControls::HitTest2(evt, pProject);
}

class NoteTrackMenuTable : public PopupMenuTable
{
   NoteTrackMenuTable() : mpData(NULL) {}
   DECLARE_POPUP_MENU(NoteTrackMenuTable);

public:
   static NoteTrackMenuTable &Instance();

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

   void OnChangeOctave(wxCommandEvent &);
};

NoteTrackMenuTable &NoteTrackMenuTable::Instance()
{
   static NoteTrackMenuTable instance;
   return instance;
}

enum {
   OnUpOctaveID = 30000,
   OnDownOctaveID,
};

/// This only applies to MIDI tracks.  Presumably, it shifts the
/// whole sequence by an octave.
void NoteTrackMenuTable::OnChangeOctave(wxCommandEvent &event)
{
   NoteTrack *const pTrack = static_cast<NoteTrack*>(mpData->pTrack);

   wxASSERT(event.GetId() == OnUpOctaveID
      || event.GetId() == OnDownOctaveID);
   wxASSERT(pTrack->GetKind() == Track::Note);

   const bool bDown = (OnDownOctaveID == event.GetId());
   pTrack->SetBottomNote
      (pTrack->GetBottomNote() + ((bDown) ? -12 : 12));

   AudacityProject *const project = ::GetActiveProject();
   project->ModifyState(true);
   mpData->result = RefreshCode::RefreshAll;
}

BEGIN_POPUP_MENU(NoteTrackMenuTable)
   POPUP_MENU_SEPARATOR()
   POPUP_MENU_ITEM(OnUpOctaveID, _("Up &Octave"), OnChangeOctave)
   POPUP_MENU_ITEM(OnDownOctaveID, _("Down Octa&ve"), OnChangeOctave)
END_POPUP_MENU()

PopupMenuTable *NoteTrackControls::GetMenuExtension(Track *)
{
#if defined(USE_MIDI)
   return &NoteTrackMenuTable::Instance();
#else
   return NULL;
#endif
}
