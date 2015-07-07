/**********************************************************************

Audacity: A Digital Audio Editor

ZoomHandle.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_ZOOM_HANDLE__
#define __AUDACITY_ZOOM_HANDLE__

#include "../../UIHandle.h"

#include <wx/gdicmn.h>

struct HitTestResult;

class ZoomHandle : public UIHandle
{
   ZoomHandle();
   ZoomHandle(const ZoomHandle&);
   ZoomHandle &operator=(const ZoomHandle&);
   static ZoomHandle& Instance();
   static HitTestPreview HitPreview
      (const wxMouseEvent &event, const AudacityProject *pProject);

public:
   static HitTestResult HitAnywhere
      (const wxMouseEvent &event, const AudacityProject *pProject);
   static HitTestResult HitTest
      (const wxMouseEvent &event, const AudacityProject *pProject);

   virtual ~ZoomHandle();

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

private:
   bool IsDragZooming() const;

   int mZoomStart, mZoomEnd;
   wxRect mRect;
};

#endif
