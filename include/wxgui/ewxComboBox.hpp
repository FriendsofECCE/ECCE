#ifndef _EWXCOMBOBOX_H
#define _EWXCOMBOBOX_H

#include "wx/combobox.h"

#include "wxgui/ewxGenericValidator.H" // for ewxDefaultValidator
#include "wxgui/ewxStyledWindow.H"

class ewxDisabler;
class ewxComboEvtHelper;

class ewxComboBox : public wxComboBox, public ewxStyledWindow
{
   public:
      // constructors
      ewxComboBox();
      ewxComboBox(wxWindow* parent, wxWindowID id, const wxString& value = "",
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize, int n = 0,
                  const wxString choices[] = NULL, long style = 0,
                  const wxValidator& validator = ewxDefaultValidator,
                  const wxString& name = "ewxComboBox");
      bool Create(wxWindow* parent, wxWindowID id, const wxString& value = "",
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize, int n = 0,
                  const wxString choices[] = NULL, long style = 0,
                  const wxValidator& validator = ewxDefaultValidator,
                  const wxString& name = "ewxComboBox");
      virtual ~ewxComboBox();
      virtual void setCustomDisabledStyle(bool disabled);
      virtual void setLeaveAsEnter(bool value);
      virtual void SetEditable(bool editable);
      virtual bool IsEditable();

   protected:
      ewxDisabler * p_disabler;
      ewxComboEvtHelper * p_comboHelper;

      bool p_editable;
};

#endif
