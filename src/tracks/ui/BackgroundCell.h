/**********************************************************************

Audacity: A Digital Audio Editor

BackgroundCell.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_BACKGROUND_CELL__
#define __AUDACITY_BACKGROUND_CELL__

#include "CommonTrackPanelCell.h"

class AudacityProject;

class BackgroundCell : public CommonTrackPanelCell
{
public:
   BackgroundCell(AudacityProject *pProject)
      : mpProject(pProject)
   {}

   virtual ~BackgroundCell();

protected:
   virtual HitTestResult HitTest
      (const TrackPanelMouseEvent &event,
      const AudacityProject *);

   virtual Track *FindTrack();

private:
   AudacityProject *mpProject;
};

#endif
