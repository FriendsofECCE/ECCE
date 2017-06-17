#ifndef  __SOWXMOUSE_H
#define  __SOWXMOUSE_H

#include "inv/events/SoLocation2Event.H"
#include "inv/events/SoMouseButtonEvent.H"

#include "inv/SoWx/SoWxDevice.H"

#define SO_WX_ALL_MOUSE_EVENTS 0

class SoWxMouse : public SoWxDevice {
public:
  // which events contains a list of event types this device should listen to
  // Last item in the event type array needs to be 0
  SoWxMouse(wxEventType * whichEvents = SO_WX_ALL_MOUSE_EVENTS);
  ~SoWxMouse();
    
  virtual void enable(wxWindow * win);
  virtual void disable(wxWindow * win);

  virtual const SoEvent * translateEvent(wxEvent * event);
  
protected:

  bool p_enabled;

  SoLocation2Event * p_loc2Event;
  SoMouseButtonEvent * p_buttonEvent;

  SoLocation2Event * translateMotionEvent(wxMouseEvent * event);
  SoMouseButtonEvent * translateButtonEvent(wxMouseEvent * event,
                                            SoButtonEvent::State s);
};

#endif
