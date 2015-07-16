/**********************************************************************

Audacity: A Digital Audio Editor

ButtonHandle.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_BUTTON_HANDLE__
#define __AUDACITY_BUTTON_HANDLE__

#include "../../UIHandle.h"
#include <wx/gdicmn.h>

class Track;

class ButtonHandle : public UIHandle
{
   ButtonHandle(const ButtonHandle&);
   ButtonHandle &operator=(const ButtonHandle&);

protected:
   explicit ButtonHandle(int dragCode);
   virtual ~ButtonHandle();

   // This new abstract virtual simplifies the duties of further subclasses.
   // This class will decide whether to refresh the clicked cell for button state
   // change.
   // Subclass can decide to refresh other things and the results will be ORed.
   virtual Result CommitChanges
      (const wxMouseEvent &event, AudacityProject *pProject, wxWindow *pParent) = 0;

   // Return the same as for the other overload of Preview; useful for derived
   // class to define a hit test
   static HitTestPreview Preview();

   // These are overrides of the base abstract class:

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

   wxRect mRect;
   Track *mpTrack;
   const int mDragCode;
};

#endif
