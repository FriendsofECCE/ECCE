#ifndef  __SOWXDEVICE_H
#define  __SOWXDEVICE_H

#include <wx/window.h>
#include <wx/event.h>
#include "inv/events/SoEvent.H"


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
  inline void setEventPosition(SoEvent *event, int x, int y) const
  { event->setPosition(SbVec2s(x, (winSize[1] - 1) - y)); }
    
private:
  // size of the window this device works in
  SbVec2s winSize;
};


#endif
