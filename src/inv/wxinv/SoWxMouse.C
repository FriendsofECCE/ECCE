#include "inv/SbTime.H"
#include <wx/event.h>

#include "inv/SoWx/SoWxMouse.H"

//#include <iostream>
//using std::cerr;


SoWxMouse::SoWxMouse(wxEventType * whichEvents)
{
  p_enabled = true;

  // allocate the SoEvents we support here
  p_loc2Event = new SoLocation2Event;
  p_buttonEvent = new SoMouseButtonEvent;
}


SoWxMouse::~SoWxMouse()
{
  delete p_loc2Event;
  delete p_buttonEvent;
}


void SoWxMouse::enable(wxWindow * win)
{
  p_enabled = true;
}


void SoWxMouse::disable(wxWindow * win)
{
  p_enabled = false;
}


const SoEvent * SoWxMouse::translateEvent(wxEvent * evt)
{
  SoEvent *event = NULL;

  wxMouseEvent * mouseEvent = dynamic_cast<wxMouseEvent *>(evt);
  if (p_enabled && mouseEvent != 0) {
    // switch on event type
    if (mouseEvent->ButtonDown() || mouseEvent->ButtonDClick())
      event = translateButtonEvent(mouseEvent, SoButtonEvent::DOWN);
    else if (mouseEvent->ButtonUp())
      event = translateButtonEvent(mouseEvent, SoButtonEvent::UP);
    else if (mouseEvent->Moving() || mouseEvent->Dragging())
      event = translateMotionEvent(mouseEvent);
  }
  
  return event;
}


SoMouseButtonEvent * SoWxMouse::translateButtonEvent(wxMouseEvent * event,
                                                     SoButtonEvent::State state)
{
  SoMouseButtonEvent::Button button;
  
  if (event->Button(wxMOUSE_BTN_LEFT))
    button = SoMouseButtonEvent::BUTTON1;
  else if (event->Button(wxMOUSE_BTN_MIDDLE))
    button = SoMouseButtonEvent::BUTTON2;
  else if (event->Button(wxMOUSE_BTN_RIGHT))
    button = SoMouseButtonEvent::BUTTON3;
  else 
    button = SoMouseButtonEvent::ANY;
  
  setEventPosition(p_buttonEvent, event->GetX(), event->GetY());

  //cerr << "\nButton Time stamp: " << event->GetTimestamp() << "\n";
  //cerr << "\nButton State: " << state << "\n";
  //cerr << "\nButton Button: " << button << "\n";
  SbTime stamp;
  stamp.setMsecValue(event->GetTimestamp());
  p_buttonEvent->setTime(stamp);
  p_buttonEvent->setShiftDown(event->ShiftDown());
  p_buttonEvent->setCtrlDown(event->ControlDown());
  p_buttonEvent->setAltDown(event->AltDown());
  p_buttonEvent->setState(state);
  p_buttonEvent->setButton(button);
    
  return p_buttonEvent;
}


SoLocation2Event * SoWxMouse::translateMotionEvent(wxMouseEvent * event)
{
  setEventPosition(p_loc2Event, event->GetX(), event->GetY());

  //  cerr << "Mouse Move Time stamp: " << event->GetTimestamp() << "\n";
  SbTime stamp;
  stamp.setMsecValue(event->GetTimestamp());
  p_loc2Event->setTime(stamp);
  p_loc2Event->setShiftDown(event->ShiftDown());
  p_loc2Event->setCtrlDown(event->ControlDown());
  p_loc2Event->setAltDown(event->AltDown());

  return p_loc2Event;
}
