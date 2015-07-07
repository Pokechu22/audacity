/**********************************************************************

Audacity: A Digital Audio Editor

TrackPanelMouseEvent.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_TRACK_PANEL_MOUSE_EVENT__
#define __AUDACITY_TRACK_PANEL_MOUSE_EVENT__

class wxMouseEvent;
class wxRect;
class TrackPanelCell;

// Augment a mouse event with information about which track panel cell and
// sub-rectangle was hit.
struct TrackPanelMouseEvent
{
   TrackPanelMouseEvent
      (wxMouseEvent &event_, const wxRect &rect_, TrackPanelCell *pCell_ = NULL)
      : event(event_), rect(rect_), pCell(pCell_)
   {
   }

   wxMouseEvent &event;
   const wxRect &rect;
   TrackPanelCell *pCell; // may be NULL
   double steps { 0.0 };  // for mouse wheel rotation
};

#endif
