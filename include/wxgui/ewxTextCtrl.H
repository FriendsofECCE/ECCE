/**
 * @file
 *
 *
 */
#ifndef _EWXTEXTCTRL_H
#define _EWXTEXTCTRL_H

#include <string>
  using std::string;

#include "wx/textctrl.h"

#include "wxgui/ewxGenericValidator.H" // for ewxDefaultValidator
#include "wxgui/ewxStyledWindow.H"


class ewxUnitHelper;
class ewxDisabler;


/**
 * An Ecce style text input control.
 * Besides color/font, leave events are mapped to enter events.
 */
class ewxTextCtrl : public wxTextCtrl, public ewxStyledWindow
{
   public:
      // constructors
      ewxTextCtrl();
      ewxTextCtrl(wxWindow* parent, wxWindowID id = wxID_ANY,
                  const wxString& value = "",
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize, long style = 0,
                  const wxValidator& validator = ewxDefaultValidator,
                  const wxString& name = "ewxTextCtrl");
      bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
                  const wxString& value = "",
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize, long style = 0,
                  const wxValidator& validator = ewxDefaultValidator,
                  const wxString& name = "ewxTextCtrl");
      virtual ~ewxTextCtrl();

      void setUnitHelper(ewxUnitHelper *unitHelper);
      ewxUnitHelper *getUnitHelper() const;
      void setLeaveAsEnter(bool value);
      virtual void SetValidator(const wxValidator& validator);
      //virtual void SetValue(const wxString & value);

      virtual void     SetBaseValue(wxString value);
      virtual wxString GetBaseValue();
      virtual void     setValueAsInt(int value);
      virtual int      getValueAsInt();
      virtual int      getBaseValueAsInt();
      virtual void     setValueAsDouble(double value);
      virtual void     setValueAsDouble(double value, string family);
      virtual double   getValueAsDouble();
      virtual double   getBaseValueAsDouble();
      virtual string   getBaseValueAsDoubleString();

      virtual void setCustomDisabledStyle(bool disabled);

   protected:
      void restoreValue();

      ewxUnitHelper *p_unitHelper;
      ewxDisabler *p_disabler;
};

#endif
