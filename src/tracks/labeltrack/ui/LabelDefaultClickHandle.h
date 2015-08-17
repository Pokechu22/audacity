/**********************************************************************

Audacity: A Digital Audio Editor

LabelDefaultClickHandle.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_LABEL_DEFAULT_CLICK_HANDLE__
#define __AUDACITY_LABEL_DEFAULT_CLICK_HANDLE__

#include "../../../UIHandle.h"

struct HitTestResult;
class LabelTrack;

class LabelDefaultClickHandle : public UIHandle
{
   LabelDefaultClickHandle();
   LabelDefaultClickHandle(const LabelDefaultClickHandle&);
   LabelDefaultClickHandle &operator=(const LabelDefaultClickHandle&);

public:
   static LabelDefaultClickHandle& Instance();
   virtual ~LabelDefaultClickHandle();

   static void DoClick
      (const wxMouseEvent &event, AudacityProject *pProject, TrackPanelCell *pCell);

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

   virtual void DrawExtras
      (DrawingPass pass,
      wxDC * dc, const wxRegion &updateRegion, const wxRect &panelRect);

   bool StopsOnKeystroke() override;

   void OnProjectChange(AudacityProject *pProject) override;

   UIHandle *mpForward;
};

#endif
