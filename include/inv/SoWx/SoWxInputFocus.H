#ifndef __SOWXINPUTFOCUS_H 
#define __SOWXINPUTFOCUS_H

#include "inv/SoWx/SoWxDevice.H"

#define SO_WX_ALL_FOCUS_EVENTS \
    (EnterWindowMask | LeaveWindowMask)

class SoWxInputFocus : public SoWxDevice {
public:

  // valid event mask values:
  //	    EnterWindowMask	- input focus entered this window
  //	    LeaveWindowMask	- input focus left this window
  // Bitwise OR these to specify whichEvents this device should queue.
  SoWxInputFocus();
  ~SoWxInputFocus() {}

  // these functions will enable/disable this device for the passed widget.
  // the callback function f will be invoked when events occur in w.
  // data is the clientData which will be passed.
  virtual void enable(wxWindow * win);
  virtual void disable(wxWindow * win);
  
  // this converts an X event into an SoEvent,
  // returning NULL if the event is not from this device.
  virtual const SoEvent * translateEvent(wxEvent * event);
  
private:

  bool p_enabled;
};

#endif
