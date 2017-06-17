/**
 * @file
 *
 */

#include "wx/checkbox.h"
#include "wx/choice.h"
#include "wx/textctrl.h"

#include "wxgui/WxPovrayOptions.H"

WxPovrayOptions::WxPovrayOptions()
{

}


WxPovrayOptions::WxPovrayOptions( wxWindow* parent, wxWindowID id,
                                  const wxPoint& pos, const wxSize& size,
                                  long style )
:WxPovrayOptionsGUI(parent, id, pos, size, style)
{

}

string WxPovrayOptions::getFinishStyle() const
{
  string ret = "";

  wxChoice * choice = wxDynamicCast(FindWindow(ID_POVRAY_FINISH), wxChoice);
  if (choice)
    ret = choice->GetStringSelection();

  return ret;
}


string WxPovrayOptions::getBondStyle() const
{
  string ret = "";

  wxChoice * choice = wxDynamicCast(FindWindow(ID_POVRAY_BONDS), wxChoice);

  if (choice)
    ret = choice->GetStringSelection();

  return ret;
}


string WxPovrayOptions::getIsosurfaceStyle() const
{
  string ret = "";

  wxChoice * choice = wxDynamicCast(FindWindow(ID_POVRAY_ISOSURFACES),
                                    wxChoice);

  if (choice)
    ret = choice->GetStringSelection();

  return ret;
}


bool WxPovrayOptions::isDisplayChecked() const
{
  bool ret = false;

  wxCheckBox * check = wxDynamicCast(FindWindow(ID_POVRAY_DISPLAY), wxCheckBox);

  if (check)
    ret = check->GetValue();

  return ret;
}


int WxPovrayOptions::getWidth() const
{
  int ret = 0;

  wxTextCtrl * text = wxDynamicCast(FindWindow(ID_POVRAY_WIDTH),
                                    wxTextCtrl);

  if (text)
    ret = wxAtoi(text->GetValue());

  return ret;
}


int WxPovrayOptions::getHeight() const
{
  int ret = 0;

  wxTextCtrl * text = wxDynamicCast(FindWindow(ID_POVRAY_HEIGHT),
                                    wxTextCtrl);

  if (text)
    ret = wxAtoi(text->GetValue());

  return ret;
}

