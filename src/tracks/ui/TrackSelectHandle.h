/**********************************************************************

Audacity: A Digital Audio Editor

TrackSelectHandle.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_TRACK_SELECT_HANDLE__
#define __AUDACITY_TRACK_SELECT_HANDLE__

#include "../../UIHandle.h"

struct HitTestResult;
class Track;

class TrackSelectHandle : public UIHandle
{
   TrackSelectHandle();
   TrackSelectHandle(const TrackSelectHandle&);
   TrackSelectHandle &operator=(const TrackSelectHandle&);
   static TrackSelectHandle& Instance();
   static HitTestPreview HitPreview(unsigned trackCount);

public:
   static HitTestResult HitAnywhere(unsigned trackCount);

   virtual ~TrackSelectHandle();

   virtual Result Click
      (const TrackPanelMouseEvent &event, AudacityProject *pProject);

   virtual Result Drag
      (const TrackPanelMouseEvent &event, AudacityProject *pProject);

   virtual HitTestPreview Preview
      (const TrackPanelMouseEvent &event, const AudacityProject *pProject);

   virtual Result Release
      (const TrackPanelMouseEvent &event, AudacityProject *pProject,
       wxWindow *pParent);

   virtual Result Cancel(AudacityProject *);

   bool StopsOnKeystroke() override { return true; }

private:
   Track *mpTrack;

   // JH: if the user is dragging a track, at what y
   //   coordinate should the dragging track move up or down?
   int mMoveUpThreshold;
   int mMoveDownThreshold;
   int mRearrangeCount;

   void CalculateRearrangingThresholds(const wxMouseEvent & event);
};

#endif
