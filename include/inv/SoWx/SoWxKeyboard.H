#ifndef  __SOWXKEYBOARD_H
#define  __SOWXKEYBOARD_H

#include "inv/events/SoKeyboardEvent.H"

#include "inv/SoWx/SoWxDevice.H"

#define SO_WX_ALL_KEYBOARD_EVENTS 0

class SoWxKeyboard : public SoWxDevice {
public:
  // which events contains a list of event types this device should listen to
  // Last item in the event type array needs to be 0
  SoWxKeyboard(wxEventType * whichEvents = SO_WX_ALL_KEYBOARD_EVENTS);
  ~SoWxKeyboard();
    
  virtual void enable(wxWindow * win);
  virtual void disable(wxWindow * win);
    
  virtual const SoEvent * translateEvent(wxEvent * event);
  
protected:

  bool p_enabled;

  SoKeyboardEvent * p_keyEvent;

  SoKeyboardEvent * translateKeyboardEvent(wxKeyEvent * event,
                                           SoButtonEvent::State s);

  static SoKeyboardEvent::Key convert(int keyCode);

};

#endif
