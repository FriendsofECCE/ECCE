#include <iostream>
using namespace std;

// #ifndef WX_PRECOMP
// #include "wx/wx.h"
// #include "wx/spinctrl.h"
// #endif
// 

#include "wxgui/ewxDisabler.H"

// TODO add other handlers here
BEGIN_EVENT_TABLE(ewxDisabler, wxEvtHandler)
    EVT_CHOICE( wxID_ANY, ewxDisabler::filter)
    EVT_BUTTON( wxID_ANY, ewxDisabler::filter)
    EVT_CHECKBOX( wxID_ANY, ewxDisabler::filter)
    EVT_SPINCTRL( wxID_ANY, ewxDisabler::filterSpin)
    EVT_TEXT_ENTER( wxID_ANY, ewxDisabler::filter)
    EVT_RADIOBUTTON( wxID_ANY, ewxDisabler::filter)
    EVT_COMBOBOX( wxID_ANY, ewxDisabler::filter)
    EVT_LEFT_DOWN(ewxDisabler::filterMouse)
    EVT_LEFT_UP(ewxDisabler::filterMouse)
    EVT_KEY_UP(ewxDisabler::filterKey)
    EVT_MENU(wxID_ANY, ewxDisabler::filter)
    EVT_TEXT(wxID_ANY, ewxDisabler::filter)
END_EVENT_TABLE()

ewxDisabler::ewxDisabler() 
{
   p_isEnabled = true;
}

ewxDisabler::~ewxDisabler() 
{ 
}

void ewxDisabler::setEnabled(bool enable)
{
   p_isEnabled = enable;
}
bool ewxDisabler::getEnabled() const
{
   return p_isEnabled;
}

void ewxDisabler::filter( wxCommandEvent& event )
{
   if (p_isEnabled) event.Skip();
}
void ewxDisabler::filterMouse( wxMouseEvent& event )
{
   if (p_isEnabled) event.Skip();
}
void ewxDisabler::filterKey( wxKeyEvent& event )
{
   if (p_isEnabled) event.Skip();
}


void ewxDisabler::filterSpin( wxSpinEvent& event)
{
    if (p_isEnabled) event.Skip();
}
