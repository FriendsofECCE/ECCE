#ifndef _EWXLISTBOX_H
#define _EWXLISTBOX_H

#include "wx/listbox.h"

#include "wxgui/ewxGenericValidator.H" // for ewxDefaultValidator
#include "wxgui/ewxStyledWindow.H"

class ewxDisabler;

class ewxListBox : public wxListBox, public ewxStyledWindow
{
  public:
    // constructors
    ewxListBox();
    // For backward compatibility
    ewxListBox(wxWindow* parent, wxWindowID id = wxID_ANY,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               int n = 0, const wxString choices[] = (const wxString *) NULL,
               long style = 0,
               const wxValidator& validator = ewxDefaultValidator,
               const wxString& name = "ewxListBox");

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                int n = 0, const wxString choices[] = (const wxString *) NULL,
                long style = 0,
                const wxValidator& validator = ewxDefaultValidator,
                const wxString& name = "ewxListBox");

    ewxListBox(wxWindow* parent, wxWindowID id,
               const wxPoint& pos,
               const wxSize& size,
               const wxArrayString& choices,
               long style = 0,
               const wxValidator& validator = ewxDefaultValidator,
               const wxString& name = "ewxListBox");

    bool Create(wxWindow* parent, wxWindowID id,
                const wxPoint& pos,
                const wxSize& size,
                const wxArrayString& choices,
                long style = 0,
                const wxValidator& validator = ewxDefaultValidator,
                const wxString& name = "ewxListBox");

    virtual ~ewxListBox();

    virtual void setCustomDisabledStyle(bool disabled);
    virtual void SetEditable(bool editable);
    virtual bool IsEditable();

  protected:
    bool p_editable;
    ewxDisabler *p_disabler;
};

#endif
