/**********************************************************************

Audacity: A Digital Audio Editor

TrackControls.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_TRACK_CONTROLS__
#define __AUDACITY_TRACK_CONTROLS__

#include "CommonTrackPanelCell.h"

class PopupMenuTable;
class Track;

class TrackControls : public CommonTrackPanelCell
{
public:
   TrackControls() : mpTrack(NULL) {}

   virtual ~TrackControls() = 0;

   Track *GetTrack() const { return mpTrack; }

   // This is passed to the InitMenu() methods of the PopupMenuTable
   // objects returned by GetMenuExtension:
   struct InitMenuData
   {
   public:
      InitMenuData()
         : pTrack(NULL), pParent(NULL)
      {}

      Track *pTrack;
      wxWindow *pParent;
      unsigned result;
   };

   // Make this hack go away!  See TrackPanel::DrawOutside
   static int gCaptureState;

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

   virtual unsigned DoContextMenu
      (const wxRect &rect, wxWindow *pParent, wxPoint *pPosition);
   virtual PopupMenuTable *GetMenuExtension(Track *pTrack) = 0;

   friend class Track;
   Track *mpTrack;
};

#endif
