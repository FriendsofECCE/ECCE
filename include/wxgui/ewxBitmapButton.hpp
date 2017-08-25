#ifndef _EWXBITMAPBUTTON_H
#define _EWXBITMAPBUTTON_H

#include "wxgui/ewxStyledWindow.H"
#include "wx/bmpbuttn.h"

class ewxDisabler;

class ewxBitmapButton : public wxBitmapButton, public ewxStyledWindow
{
  public:
    ewxBitmapButton();
    ewxBitmapButton( wxWindow* parent, wxWindowID id, 
                     const wxBitmap& bitmap, 
                     const wxPoint& pos = wxDefaultPosition, 
                     const wxSize& size = wxDefaultSize, 
                     long style = wxBU_AUTODRAW, 
                     const wxValidator& validator = wxDefaultValidator, 
                     const wxString& name = "button");
    bool Create( wxWindow* parent, wxWindowID id, 
                 const wxBitmap& bitmap, 
                 const wxPoint& pos = wxDefaultPosition, 
                 const wxSize& size = wxDefaultSize, 
                 long style = wxBU_AUTODRAW, 
                 const wxValidator& validator = wxDefaultValidator, 
                 const wxString& name = "button");
    virtual ~ewxBitmapButton();
    virtual void setCustomDisabledStyle(bool disabled);
    virtual void SetEditable(bool editable);
    virtual bool IsEditable();

  protected:
    bool p_editable;
    ewxDisabler *p_disabler;
};

#endif
