#ifndef _EWXCHECKBOX_H
#define _EWXCHECKBOX_H

#include "wx/checkbox.h"
#include "wxgui/ewxGenericValidator.H" // for ewxDefaultValidator
#include "wxgui/ewxStyledWindow.H"

class ewxDisabler;

class ewxCheckBox : public wxCheckBox, public ewxStyledWindow
{
   public:
      // constructors
      ewxCheckBox();
      ewxCheckBox(wxWindow* parent, wxWindowID id, const wxString& label,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize, long style = 0,
                  const wxValidator& validator = ewxDefaultValidator,
                  const wxString& name = "ewxCheckBox");
      bool Create(wxWindow* parent, wxWindowID id, const wxString& label,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize, long style = 0,
                  const wxValidator& validator = ewxDefaultValidator,
                  const wxString& name = "ewxCheckBox");
      virtual ~ewxCheckBox();
      virtual void setCustomDisabledStyle(bool disabled);
      virtual void SetEditable(bool editable);
      virtual bool IsEditable();

  protected:
      bool p_editable;
      ewxDisabler *p_disabler;
};

#endif
