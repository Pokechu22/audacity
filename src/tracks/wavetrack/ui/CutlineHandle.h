/**********************************************************************

Audacity: A Digital Audio Editor

CutlineHandle.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_CUTLINE_HANDLE__
#define __AUDACITY_CUTLINE_HANDLE__

#include "../../../UIHandle.h"

struct HitTestResult;
class Track;

class CutlineHandle : public UIHandle
{
   CutlineHandle();
   CutlineHandle(const CutlineHandle&);
   CutlineHandle &operator=(const CutlineHandle&);
   static CutlineHandle& Instance();
   static HitTestPreview HitPreview(bool cutline, bool unsafe);

public:
   static HitTestResult HitAnywhere(const AudacityProject *pProject, bool cutline);
   static HitTestResult HitTest
      (const wxMouseEvent &event, const wxRect &rect,
      const AudacityProject *pProject, Track *pTrack);

   virtual ~CutlineHandle();

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
   enum Operation { Merge, Expand, Remove };
   Operation mOperation;
   double mStartTime, mEndTime;
   bool mbCutline;
};

#endif
