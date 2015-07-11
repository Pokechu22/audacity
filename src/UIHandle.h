/**********************************************************************

Audacity: A Digital Audio Editor

UIHandle.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_UI_HANDLE__
#define __AUDACITY_UI_HANDLE__

#include <utility>

class wxDC;
class wxMouseEvent;
class wxRect;
class wxRegion;
class wxWindow;

class AudacityProject;
struct HitTestPreview;
class TrackPanelCell;
struct TrackPanelMouseEvent;

class UIHandle
{
public:
   // See RefreshCode.h for bit flags:
   typedef unsigned Result;

   // Future: may generalize away from current Track class
   typedef TrackPanelCell Cell;

   // Argument for the drawing function
   enum DrawingPass {
      Cells,
      Panel,
   };

   virtual ~UIHandle() = 0;

   // Assume hit test (implemented in other classes) was positive.
   // May return Cancelled, overriding the hit test decision and stopping drag.
   // Otherwise the framework will later call Release or Cancel after
   // some number of Drag calls.
   virtual Result Click
      (const TrackPanelMouseEvent &event, AudacityProject *pProject) = 0;

   // Assume previously Clicked and not yet Released or Cancelled.
   // pCell may be other than for Click; may be NULL, and rect empty.
   // Return value may include the Cancelled return flag,
   // in which case the handle will not be invoked again.
   virtual Result Drag
      (const TrackPanelMouseEvent &event, AudacityProject *pProject) = 0;

   // Update the cursor and status message.
   virtual HitTestPreview Preview
      (const TrackPanelMouseEvent &event, const AudacityProject *pProject) = 0;

   // Assume previously Clicked and not yet Released or Cancelled.
   // pCell may be other than for Click; may be NULL, and rects empty.
   // Can use pParent as parent to pop up a context menu,
   // connecting and disconnecting event handlers for the menu items.
   // Cancelled return flag is ignored.
   virtual Result Release
      (const TrackPanelMouseEvent &event, AudacityProject *pProject,
       wxWindow *pParent) = 0;

   // Assume previously Clicked and not yet Released or Cancelled.
   // Cancelled return flag is ignored.
   virtual Result Cancel(AudacityProject *pProject) = 0;

   // Draw extras over cells.  Default does nothing.
   // Supplies only the whole panel rectangle for now.
   // If pass is Cells, then any drawing that extends outside the cells
   // is later overlaid with the cell bevels and the empty background color.
   // Otherwise (Panel), it is a later drawing pass that will not be overlaid.
   virtual void DrawExtras
      (DrawingPass pass,
       wxDC * dc, const wxRegion &updateRegion, const wxRect &panelRect);

   // Whether to force completion (not cancellation) of the drag when a
   // keystroke command is about to be dispatched.  Default is always false.
   virtual bool StopsOnKeystroke();

   // Notification after a command is dispatched; generally, it will need to
   // be overridden only in case StopOnKeystroke() returns false.  Default
   // does nothing.
   virtual void OnProjectChange(AudacityProject *pProject);
};
#endif
