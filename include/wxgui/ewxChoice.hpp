#ifndef _EWXCHOICE_H
#define _EWXCHOICE_H

#include <wx/choice.h>

#include "wxgui/ewxGenericValidator.H" // for ewxDefaultValidator
#include "wxgui/ewxStyledWindow.H"

class ewxDisabler;

class ewxChoice : public wxChoice, public ewxStyledWindow
{
  public:
    ewxChoice();
    ewxChoice(wxWindow *parent, wxWindowID id,
            const wxPoint& pos, const wxSize& size,
            int n, const wxString choices[], long style = 0,
            const wxValidator& validator = wxDefaultValidator,
            const wxString& name = "ewxChoice");
    ewxChoice(wxWindow *parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            const wxArrayString& choices = wxArrayString(), long style = 0,
            const wxValidator& validator = wxDefaultValidator,
            const wxString& name = "ewxChoice");
    virtual ~ewxChoice();

    bool Create(wxWindow *parent, wxWindowID id,
            const wxPoint& pos, const wxSize& size,
            int n, const wxString choices[], long style = 0,
            const wxValidator& validator = wxDefaultValidator,
            const wxString& name = "ewxChoice");
    bool Create(wxWindow *parent, wxWindowID id,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            const wxArrayString& choices = wxArrayString(), long style = 0,
            const wxValidator& validator = wxDefaultValidator,
            const wxString& name = "ewxChoice");

    virtual void setCustomDisabledStyle(bool disabled);
    virtual void SetEditable(bool editable);
    virtual bool IsEditable();

  protected:
    bool Create();

    bool p_editable;
    ewxDisabler *p_disabler;
};

#endif
