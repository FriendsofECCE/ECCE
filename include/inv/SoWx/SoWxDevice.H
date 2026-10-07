#ifndef  __SOWXDEVICE_H
#define  __SOWXDEVICE_H

#include <wx/window.h>
#include <wx/event.h>
#include "inv/events/SoEvent.H"
#include <cmath>


// Framebuffer pixels per wx logical unit of the window an event came from:
// 2 on a Retina display or under GDK_SCALE=2, 1 on Windows (wxMSW logical
// units are pixels).  The viewport and every SoEvent position are pixels.
inline double soWxPixelScale(const wxEvent *e)
{
  wxWindow *w = e ? wxDynamicCast(e->GetEventObject(), wxWindow) : 0;
  return w ? w->GetContentScaleFactor() : 1.0;
}

inline int soWxToPixels(const wxEvent *e, int v)
{
  return (int)std::lround(v * soWxPixelScale(e));
}


class SoWxDevice {
public:
  virtual void enable(wxWindow * win) = 0;
  virtual void disable(wxWindow * win) = 0;
    
  virtual const SoEvent * translateEvent(wxEvent * event) = 0;

  void setWindowSize(const SbVec2s &s)
  { winSize = s; }

  const SbVec2s & getWindowSize() const
  { return winSize; }

protected:
  inline void setEventPosition(SoEvent *event, const wxEvent *src,
                               int x, int y) const
  {
    x = soWxToPixels(src, x);
    y = soWxToPixels(src, y);
    event->setPosition(SbVec2s(x, (winSize[1] - 1) - y));
  }
    
private:
  // size of the window this device works in
  SbVec2s winSize;
};


#endif
