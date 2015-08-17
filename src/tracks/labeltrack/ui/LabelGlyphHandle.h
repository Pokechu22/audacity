/**********************************************************************

Audacity: A Digital Audio Editor

LabelGlyphHandle.h

Paul Licameli split from TrackPanel.cpp

**********************************************************************/

#ifndef __AUDACITY_LABEL_GLYPH_HANDLE__
#define __AUDACITY_LABEL_GLYPH_HANDLE__

#include "../../../UIHandle.h"
#include <wx/gdicmn.h>

struct HitTestResult;
class LabelTrack;

class LabelGlyphHandle : public UIHandle
{
   LabelGlyphHandle();
   LabelGlyphHandle(const LabelGlyphHandle&);
   LabelGlyphHandle &operator=(const LabelGlyphHandle&);
   static LabelGlyphHandle& Instance();
   static HitTestPreview HitPreview(bool hitCenter, unsigned refreshResult);

public:
   static HitTestResult HitTest
      (const wxMouseEvent &event, LabelTrack *pLT);

   virtual ~LabelGlyphHandle();

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
   LabelTrack *mpLT;
   wxRect mRect;
};

#endif
