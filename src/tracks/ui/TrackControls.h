/**********************************************************************

Audacity: A Digital Audio Editor

TrackControls.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_TRACK_CONTROLS__
#define __AUDACITY_TRACK_CONTROLS__

#include "CommonTrackPanelCell.h"

class Track;

class TrackControls : public CommonTrackPanelCell
{
public:
   TrackControls() : mpTrack(NULL) {}

   virtual ~TrackControls() = 0;

   Track *GetTrack() const { return mpTrack; }

protected:
   HitTestResult HitTest1
      (const TrackPanelMouseEvent &event,
      const AudacityProject *);

   HitTestResult HitTest2
      (const TrackPanelMouseEvent &event,
      const AudacityProject *);

   virtual HitTestResult HitTest
      (const TrackPanelMouseEvent &event,
       const AudacityProject *) = 0;

   virtual Track *FindTrack();

   friend class Track;
   Track *mpTrack;
};

#endif
