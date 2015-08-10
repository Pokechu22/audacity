/**********************************************************************

Audacity: A Digital Audio Editor

ZoomHandle.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_TRACK_PANEL_RESIZE_HANDLE__
#define __AUDACITY_TRACK_PANEL_RESIZE_HANDLE__

#include "UIHandle.h"

struct HitTestResult;
class Track;

class TrackPanelResizeHandle : public UIHandle
{
   TrackPanelResizeHandle();
   TrackPanelResizeHandle(const TrackPanelResizeHandle&);
   TrackPanelResizeHandle &operator=(const TrackPanelResizeHandle&);

public:
   static TrackPanelResizeHandle& Instance();
   static HitTestPreview HitPreview(bool bLinked);

   virtual ~TrackPanelResizeHandle();

   virtual Result Click
      (const TrackPanelMouseEvent &event, AudacityProject *pProject);

   virtual Result Drag
      (const TrackPanelMouseEvent &event, AudacityProject *pProject);

   virtual HitTestPreview Preview
      (const TrackPanelMouseEvent &event,  const AudacityProject *pProject);

   virtual Result Release
      (const TrackPanelMouseEvent &event, AudacityProject *pProject,
       wxWindow *pParent);

   virtual Result Cancel(AudacityProject *pProject);

   void OnProjectChange(AudacityProject *pProject) override;

private:
   enum Mode {
      IsResizing,
      IsResizingBetweenLinkedTracks,
      IsResizingBelowLinkedTracks,
   };
   Mode mMode;

   Track *mpTrack;

   bool mInitialMinimized;
   int mInitialTrackHeight;
   int mInitialActualHeight;
   int mInitialUpperTrackHeight;
   int mInitialUpperActualHeight;

   int mMouseClickY;
};

#endif
