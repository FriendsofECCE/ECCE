#include "inv/SbTime.H"
#include <wx/event.h>

#include "inv/SoWx/SoWxKeyboard.H"

//#include <iostream>
//using std::cerr;


SoWxKeyboard::SoWxKeyboard(wxEventType * whichEvents)
{
  p_enabled = true;

  // allocate the SoEvents we support here
  p_keyEvent = new SoKeyboardEvent;
}


SoWxKeyboard::~SoWxKeyboard()
{
  delete p_keyEvent;
}


void SoWxKeyboard::enable(wxWindow * win)
{
  p_enabled = true;
}


void SoWxKeyboard::disable(wxWindow * win)
{
  p_enabled = false;
}


const SoEvent * SoWxKeyboard::translateEvent(wxEvent * evt)
{
  SoEvent *event = NULL;

  wxKeyEvent * keyEvent = dynamic_cast<wxKeyEvent *>(evt);

  if (p_enabled && keyEvent != 0) {
    // switch on event type
    if (keyEvent->GetEventType() == wxEVT_KEY_DOWN)
      event = translateKeyboardEvent(keyEvent, SoButtonEvent::DOWN);
    else if (keyEvent->GetEventType() == wxEVT_KEY_UP)
      event = translateKeyboardEvent(keyEvent, SoButtonEvent::UP);
  }
  
  return event;
}


SoKeyboardEvent * SoWxKeyboard::translateKeyboardEvent(wxKeyEvent * event,
                                                       SoButtonEvent::State state)
{
  setEventPosition(p_keyEvent, event->GetX(), event->GetY());

  p_keyEvent->setKey(convert(event->GetKeyCode()));

  //  cerr << "Key Time stamp: " << event->GetTimestamp() << "\n";
  SbTime stamp;
  stamp.setMsecValue(event->GetTimestamp());
  p_keyEvent->setTime(stamp);
  p_keyEvent->setShiftDown(event->ShiftDown());
  p_keyEvent->setCtrlDown(event->ControlDown());
  p_keyEvent->setAltDown(event->AltDown());
  p_keyEvent->setState(state);

  //  cerr << p_keyEvent->getPrintableCharacter() << "\n";

  return p_keyEvent;
}


SoKeyboardEvent::Key SoWxKeyboard::convert(int keyCode)
{
  SoKeyboardEvent::Key key;
  switch(keyCode) {
  case 'A':
    key = SoKeyboardEvent::A;   break;
  case 'B':
    key = SoKeyboardEvent::B;   break;
  case 'C':
    key = SoKeyboardEvent::C;   break;
  case 'D':
    key = SoKeyboardEvent::D;   break;
  case 'E':
    key = SoKeyboardEvent::E;   break;
  case 'F':
    key = SoKeyboardEvent::F;   break;
  case 'G':
    key = SoKeyboardEvent::G;   break;
  case 'H':
    key = SoKeyboardEvent::H;   break;
  case 'I':
    key = SoKeyboardEvent::I;   break;
  case 'J':
    key = SoKeyboardEvent::J;   break;
  case 'K':
    key = SoKeyboardEvent::K;   break;
  case 'L':
    key = SoKeyboardEvent::L;   break;
  case 'M':
    key = SoKeyboardEvent::M;   break;
  case 'N':
    key = SoKeyboardEvent::N;   break;
  case 'O':
    key = SoKeyboardEvent::O;   break;
  case 'P':
    key = SoKeyboardEvent::P;   break;
  case 'Q':
    key = SoKeyboardEvent::Q;   break;
  case 'R':
    key = SoKeyboardEvent::R;   break;
  case 'S':
    key = SoKeyboardEvent::S;   break;
  case 'T':
    key = SoKeyboardEvent::T;   break;
  case 'U':
    key = SoKeyboardEvent::U;   break;
  case 'V':
    key = SoKeyboardEvent::V;   break;
  case 'W':
    key = SoKeyboardEvent::W;   break;
  case 'X':
    key = SoKeyboardEvent::X;   break;
  case 'Y':
    key = SoKeyboardEvent::Y;   break;
  case 'Z':
    key = SoKeyboardEvent::Z;   break;

  case '0':
    key = SoKeyboardEvent::NUMBER_0;   break;
  case '1':
    key = SoKeyboardEvent::NUMBER_1;   break;
  case '2':
    key = SoKeyboardEvent::NUMBER_2;   break;
  case '3':
    key = SoKeyboardEvent::NUMBER_3;   break;
  case '4':
    key = SoKeyboardEvent::NUMBER_4;   break;
  case '5':
    key = SoKeyboardEvent::NUMBER_5;   break;
  case '6':
    key = SoKeyboardEvent::NUMBER_6;   break;
  case '7':
    key = SoKeyboardEvent::NUMBER_7;   break;
  case '8':
    key = SoKeyboardEvent::NUMBER_8;   break;
  case '9':
    key = SoKeyboardEvent::NUMBER_9;   break;

  case WXK_BACK:
    key = SoKeyboardEvent::BACKSPACE;   break;
  case WXK_TAB:
    key = SoKeyboardEvent::TAB;   break;
  case WXK_RETURN:
    key = SoKeyboardEvent::RETURN;   break;
  case WXK_ESCAPE:
    key = SoKeyboardEvent::ESCAPE;   break;
  case WXK_SPACE:
    key = SoKeyboardEvent::SPACE;   break;
  case WXK_SHIFT:
    //Don't know how to tell if it is right/left in wx, return left for now
    key = SoKeyboardEvent::LEFT_SHIFT;   break;
  case WXK_CONTROL:
    //Don't know how to tell if it is right/left in wx, return left for now
    key = SoKeyboardEvent::LEFT_CONTROL;   break;
  case WXK_ALT:
    //Don't know how to tell if it is right/left in wx, return left for now
    key = SoKeyboardEvent::LEFT_ALT;   break;

  case WXK_HOME:
    key = SoKeyboardEvent::HOME;   break;
  case WXK_END:
    key = SoKeyboardEvent::END;   break;
  case WXK_DELETE:
    key = SoKeyboardEvent::DELETE;   break;
  case WXK_PAGEUP:
    key = SoKeyboardEvent::PAGE_UP;   break;
  case WXK_PAGEDOWN:
    key = SoKeyboardEvent::PAGE_DOWN;   break;
  case WXK_INSERT:
    key = SoKeyboardEvent::INSERT;   break;

  case WXK_LEFT:
    key = SoKeyboardEvent::LEFT_ARROW;   break;
  case WXK_RIGHT:
    key = SoKeyboardEvent::RIGHT_ARROW;   break;
  case WXK_UP:
    key = SoKeyboardEvent::UP_ARROW;   break;
  case WXK_DOWN:
    key = SoKeyboardEvent::DOWN_ARROW;   break;

  case WXK_NUMPAD0:
    key = SoKeyboardEvent::PAD_0;   break;
  case WXK_NUMPAD1:
    key = SoKeyboardEvent::PAD_1;   break;
  case WXK_NUMPAD2:
    key = SoKeyboardEvent::PAD_2;   break;
  case WXK_NUMPAD3:
    key = SoKeyboardEvent::PAD_3;   break;
  case WXK_NUMPAD4:
    key = SoKeyboardEvent::PAD_4;   break;
  case WXK_NUMPAD5:
    key = SoKeyboardEvent::PAD_5;   break;
  case WXK_NUMPAD6:
    key = SoKeyboardEvent::PAD_6;   break;
  case WXK_NUMPAD7:
    key = SoKeyboardEvent::PAD_7;   break;
  case WXK_NUMPAD8:
    key = SoKeyboardEvent::PAD_8;   break;
  case WXK_NUMPAD9:
    key = SoKeyboardEvent::PAD_9;   break;
  case WXK_NUMPAD_ENTER:
    key = SoKeyboardEvent::PAD_ENTER;   break;
  case WXK_NUMPAD_F1:
    key = SoKeyboardEvent::PAD_F1;   break;
  case WXK_NUMPAD_F2:
    key = SoKeyboardEvent::PAD_F2;   break;
  case WXK_NUMPAD_F3:
    key = SoKeyboardEvent::PAD_F3;   break;
  case WXK_NUMPAD_F4:
    key = SoKeyboardEvent::PAD_F4;   break;
  case WXK_NUMPAD_ADD:
    key = SoKeyboardEvent::PAD_ADD;   break;
  case WXK_NUMPAD_SUBTRACT:
    key = SoKeyboardEvent::PAD_SUBTRACT;   break;
  case WXK_NUMPAD_MULTIPLY:
    key = SoKeyboardEvent::PAD_MULTIPLY;   break;
  case WXK_NUMPAD_DIVIDE:
    key = SoKeyboardEvent::PAD_DIVIDE;   break;
  case WXK_NUMPAD_SPACE:
    key = SoKeyboardEvent::PAD_SPACE;   break;
  case WXK_NUMPAD_TAB:
    key = SoKeyboardEvent::PAD_TAB;   break;
  case WXK_NUMPAD_INSERT:
    key = SoKeyboardEvent::PAD_INSERT;   break;
  case WXK_NUMPAD_DELETE:
    key = SoKeyboardEvent::PAD_DELETE;   break;
  case WXK_NUMPAD_DECIMAL:
    key = SoKeyboardEvent::PAD_PERIOD;   break;

  case WXK_F1:
    key = SoKeyboardEvent::F1;   break;
  case WXK_F2:
    key = SoKeyboardEvent::F2;   break;
  case WXK_F3:
    key = SoKeyboardEvent::F3;   break;
  case WXK_F4:
    key = SoKeyboardEvent::F4;   break;
  case WXK_F5:
    key = SoKeyboardEvent::F5;   break;
  case WXK_F6:
    key = SoKeyboardEvent::F6;   break;
  case WXK_F7:
    key = SoKeyboardEvent::F7;   break;
  case WXK_F8:
    key = SoKeyboardEvent::F8;   break;
  case WXK_F9:
    key = SoKeyboardEvent::F9;   break;
  case WXK_F10:
    key = SoKeyboardEvent::F10;   break;
  case WXK_F11:
    key = SoKeyboardEvent::F11;   break;
  case WXK_F12:
    key = SoKeyboardEvent::F12;   break;

  case WXK_PAUSE:
    key = SoKeyboardEvent::PAUSE;   break;
  case WXK_SCROLL:
    key = SoKeyboardEvent::SCROLL_LOCK;   break;
  case WXK_PRINT:
    key = SoKeyboardEvent::PRINT;   break;
  case WXK_NUMLOCK:
    key = SoKeyboardEvent::NUM_LOCK;   break;
  case WXK_CAPITAL:
    key = SoKeyboardEvent::CAPS_LOCK;   break;

  case '\'':
  case '\"':
    key = SoKeyboardEvent::APOSTROPHE;   break;
  case ',':
  case '<':
    key = SoKeyboardEvent::COMMA;   break;
  case '-':
  case '_':
    key = SoKeyboardEvent::MINUS;   break;
  case '.':
  case '>':
    key = SoKeyboardEvent::PERIOD;   break;
  case '/':
  case '?':
    key = SoKeyboardEvent::SLASH;   break;
  case ';':
  case ':':
    key = SoKeyboardEvent::SEMICOLON;   break;
  case '=':
  case '+':
    key = SoKeyboardEvent::EQUAL;   break;
  case '[':
  case '{':
    key = SoKeyboardEvent::BRACKETLEFT;   break;
  case '\\':
  case '|':
    key = SoKeyboardEvent::BACKSLASH;   break;
  case ']':
  case '}':
    key = SoKeyboardEvent::BRACKETRIGHT;   break;
  case '`':
  case '~':
    key = SoKeyboardEvent::GRAVE;   break;

  default:
    key = SoKeyboardEvent::ANY;
  }
  return key;
}



/*
     enum Key {
          SoKeyboardEvent::ANY         Special constant for any key

          SoKeyboardEvent::PRIOR
          SoKeyboardEvent::NEXT
          SoKeyboardEvent::ENTER
          SoKeyboardEvent::SHIFT_LOCK
     }

*/
