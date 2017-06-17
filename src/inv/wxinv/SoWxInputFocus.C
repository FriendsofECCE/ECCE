#include "inv/SbTime.H"

#include "inv/SoWx/SoWxInputFocus.H"

// #include <iostream>
// using std::cerr;

SoWxInputFocus::SoWxInputFocus()
{
}


void SoWxInputFocus::enable(wxWindow * win)
{
  p_enabled = true;
}


void SoWxInputFocus::disable(wxWindow * win)
{
  p_enabled = false;
}


const SoEvent * SoWxInputFocus::translateEvent(wxEvent * evt)
{
//   if (evt->GetEventType() == wxEVT_SET_FOCUS)
//     cerr << "Matching set focus event\n";
//   else if (evt->GetEventType() == wxEVT_KILL_FOCUS)
//     cerr << "Matching kill focus event\n";
  return NULL;
}
