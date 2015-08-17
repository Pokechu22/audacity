/**********************************************************************

Audacity: A Digital Audio Editor

LabelTextHandle.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_LABEL_TEXT_HANDLE__
#define __AUDACITY_LABEL_TEXT_HANDLE__

#include "../../../UIHandle.h"
#include <wx/gdicmn.h>

struct HitTestResult;
class LabelTrack;

class LabelTextHandle : public UIHandle
{
   LabelTextHandle();
   LabelTextHandle(const LabelTextHandle&);
   LabelTextHandle &operator=(const LabelTextHandle&);
   static LabelTextHandle& Instance();

public:
   static HitTestResult HitTest(const wxMouseEvent &event, LabelTrack *pLT);

   virtual ~LabelTextHandle();

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

private:
   LabelTrack *mpLT;
   wxRect mRect;
   int mLabelTrackStartXPos;
   int mLabelTrackStartYPos;
};

#endif
