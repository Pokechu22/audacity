/**********************************************************************

Audacity: A Digital Audio Editor

StretchHandle.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_STRETCH_HANDLE__
#define __AUDACITY_STRETCH_HANDLE__

#include "../../../UIHandle.h"

#include "../../../MemoryX.h"

class Alg_seq;
struct HitTestResult;
class NoteTrack;
class Track;
class ViewInfo;

class StretchHandle : public UIHandle
{
   enum StretchEnum {
      stretchLeft,
      stretchCenter,
      stretchRight
   };

   StretchHandle();
   StretchHandle(const StretchHandle&);
   StretchHandle &operator=(const StretchHandle&);
   static StretchHandle& Instance();
   static HitTestPreview HitPreview(StretchEnum stretchMode, bool unsafe);

   static HitTestResult HitTest
      (const TrackPanelMouseEvent &evt, const AudacityProject *pProject,
       NoteTrack *pTrack,
       double &stretchSel0, double &stretchSel1,
       double &selStart, double &stretchStart,
       double &stretchLeftBeats, double &stretchRightBeats,
       StretchEnum &stretchMode);

   void Duplicate();

public:
   static HitTestResult HitTest
      (const TrackPanelMouseEvent &event, const AudacityProject *pProject,
       NoteTrack *pTrack);

   virtual ~StretchHandle();

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

   bool StopsOnKeystroke() override { return true; }

private:
   void Stretch
      (AudacityProject *pProject, int mouseXCoordinate, int trackLeftEdge, Track *pTrack);

   NoteTrack *mpTrack;
   std::unique_ptr<NoteTrack> mpBackup;
   double mSelStart;
   int mLeftEdge;

   // Stretching applies to a selected region after quantizing the
   // region to beat boundaries (subbeat stretching is not supported,
   // but maybe it should be enabled with shift or ctrl or something)
   // Stretching can drag the left boundary (the right stays fixed),
   // the right boundary (the left stays fixed), or the center (splits
   // the selection into two parts: when left part grows, the right
   // part shrinks, keeping the leftmost and rightmost boundaries
   // fixed.
   StretchEnum mStretchMode; // remembers what to drag
   bool mStretched;
   double mStretchSel0;  // initial sel0 (left) quantized to nearest beat
   double mStretchSel1;  // initial sel1 (left) quantized to nearest beat
   double mStretchStart; // time of initial mouse position, quantized
   // to the nearest beat
   double mStretchLeftBeats; // how many beats from left to cursor
   double mStretchRightBeats; // how many beats from cursor to right

   double mOrigSel0, mOrigSel1;
};

#endif
