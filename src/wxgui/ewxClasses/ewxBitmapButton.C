#include "wx/wxprec.h"
#include <iostream>
using namespace std;


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxHelpHandler.H"


ewxBitmapButton::ewxBitmapButton()
  : wxBitmapButton(),
    ewxStyledWindow()
{
}


ewxBitmapButton::ewxBitmapButton(wxWindow* parent, wxWindowID id,
        const wxBitmap& bitmap, const wxPoint& pos, const wxSize& size,
        long style, const wxValidator& validator, const wxString& name)
  : wxBitmapButton(),
    ewxStyledWindow()
{
  Create(parent, id, bitmap, pos, size, style, validator, name);
}


bool ewxBitmapButton::Create(wxWindow* parent, wxWindowID id,
        const wxBitmap& bitmap, const wxPoint& pos, const wxSize& size,
        long style, const wxValidator& validator, const wxString& name)
{
  if (!wxBitmapButton::Create(parent, id, bitmap, pos, size, style,
                              validator, name)) {
    wxFAIL_MSG( wxT("ewxBitmapButton creation failed") );
    return false;
  }

  //  wxMSW sizes the button from the bitmap at the logical scale, which
  //  leaves a scaled bitmap wider than its button.
  if (ewxBitmap::dpiScale() > 1 && bitmap.IsOk() && size == wxDefaultSize) {
    const wxSize best = GetBestSize(), bmp = bitmap.GetSize();
    SetMinSize(wxSize(wxMax(best.x, bmp.x + ewxBitmap::scaled(2)),
                      wxMax(best.y, bmp.y + ewxBitmap::scaled(2))));
  }
  PushEventHandler(new ewxHelpHandler(this));
  p_disabler = new ewxDisabler();
  PushEventHandler(p_disabler);
  p_editable = true;

  setStyles(this);

  return true;
}


ewxBitmapButton::~ewxBitmapButton()
{
  PopEventHandler(true);
  PopEventHandler(true);
}


void ewxBitmapButton::setSvg(const wxString& svg, int logical)
{
  //  The bundle, not a bitmap drawn at the scale: wxMSW takes a plain bitmap
  //  as 100 % and sizes the button for it, so a pre-scaled one stays small.
  wxBitmapBundle b = wxBitmapBundle::FromSVGFile(ewxBitmap::pixmapFile(svg),
                                                 wxSize(logical, logical));
  if (!b.IsOk()) {
    SetBitmapLabel(ewxBitmap::placeholder(svg));
    return;
  }
  SetBitmapLabel(b);
  if (ewxBitmap::dpiScale() > 1)
    SetMinSize(FromDIP(wxSize(logical + 2, logical + 2)));
}


/**
 * Set widget to our custom disabled style.
 * @param enabled true to enable, false to disable
 */
void ewxBitmapButton::setCustomDisabledStyle(bool enabled)
{
   if (p_disabler) p_disabler->setEnabled(enabled);
   SetEditable(enabled);
}


/**
 * Set widget to editable or not.  This is NOT
 * a function provided by wxwidgets but we use their method name
 * style since it should be and is less confusing.
 * No style currently implemented.  We have debated using a gray font
 * or disabling the button but decided against it.  So currently
 * this method mearly sets a flag.
 */
void ewxBitmapButton::SetEditable(bool editable)
{
   p_editable = editable;
   setStyles(this,false);
}

bool ewxBitmapButton::IsEditable()
{
   return p_editable;
}
