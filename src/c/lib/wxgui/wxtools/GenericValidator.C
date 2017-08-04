#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/dynarray.h>
#include <wx/gauge.h>
#include <wx/intl.h>
#include <wx/listbox.h>
#include <wx/radiobox.h>
#include <wx/radiobut.h>
#include <wx/scrolbar.h>
#include <wx/slider.h>
#include <wx/spinbutt.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>
#include <wx/utils.h>

#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/StringConverter.H"

#include "wxgui/GenericValidator.H"


DEFINE_EVENT_TYPE(wxEVT_VALIDATION_EVENT)


IMPLEMENT_DYNAMIC_CLASS(GenericValidator, wxValidator)


BEGIN_EVENT_TABLE(GenericValidator, wxValidator)
    EVT_CHAR(GenericValidator::OnChar)
    EVT_LEAVE_WINDOW(GenericValidator::OnLeaveWindow)
    EVT_ENTER_WINDOW(GenericValidator::OnEnterWindow)
    EVT_TIMER(wxID_ANY, GenericValidator::OnTimer)
    EVT_CHECKBOX(wxID_ANY, GenericValidator::OnSelection)
    EVT_CHOICE(wxID_ANY, GenericValidator::OnSelection)
    EVT_COMBOBOX(wxID_ANY, GenericValidator::OnSelection)
END_EVENT_TABLE()


/**
 * Default null validator.
 */
GenericValidator::GenericValidator()
  : wxValidator()
{
  Initialize();
}


/**
 * Boolean type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 */
GenericValidator::GenericValidator(bool * val)
  : wxValidator()
{
  Initialize();
  SetValue(val);
}


/**
 * Integer type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 */
GenericValidator::GenericValidator(int * val)
  : wxValidator()
{
  Initialize();
  SetValue(val);
}


/**
 * Long integer type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 */
GenericValidator::GenericValidator(long * val)
  : wxValidator()
{
  Initialize();
  SetValue(val);
}


/**
 * Float type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 */
GenericValidator::GenericValidator(float * val)
  : wxValidator()
{
  Initialize();
  SetValue(val);
}


/**
 * Double type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 */
GenericValidator::GenericValidator(double * val)
  : wxValidator()
{
  Initialize();
  SetValue(val);
}


/**
 * String type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 */
GenericValidator::GenericValidator(wxString* val)
  : wxValidator()
{
  Initialize();
  SetValue(val);
}


/**
 * Array type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 */
GenericValidator::GenericValidator(wxArrayInt* val)
  : wxValidator()
{
  Initialize();
  SetValue(val);
}


/**
 * Destructor.
 */
GenericValidator::~GenericValidator()
{
  ClearValue();
}


/**
 */
wxObject * GenericValidator::Clone() const
{
  GenericValidator * copy = new GenericValidator;
  copy->Copy(*this);
  return copy;
}


/**
 */
bool GenericValidator::Copy(const GenericValidator& that)
{
  bool ret = true;

  // shallow copy of pointers
  ret &= wxValidator::Copy(that);

  m_pBool = that.m_pBool;
  m_pInt = that.m_pInt;
  m_pLong = that.m_pLong;
  m_pFloat = that.m_pFloat;
  m_pDouble = that.m_pDouble;
  m_pString = that.m_pString;
  m_pArrayInt = that.m_pArrayInt;

  p_timer.SetOwner(this, wxID_ANY);
  p_timerEnabled = that.p_timerEnabled;

  return ret;
}


void GenericValidator::SetValue(bool * val)
{
  ClearValue();
  m_pBool = val;
}


void GenericValidator::SetValue(int * val)
{
  ClearValue();
  m_pInt = val;
}


void GenericValidator::SetValue(long * val)
{
  ClearValue();
  m_pLong = val;
}


void GenericValidator::SetValue(float * val)
{
  ClearValue();
  m_pFloat = val;
}


void GenericValidator::SetValue(double * val)
{
  ClearValue();
  m_pDouble = val;
}


void GenericValidator::SetValue(wxString * val)
{
  ClearValue();
  m_pString = val;
}


void GenericValidator::SetValue(wxArrayInt * val)
{
  ClearValue();
  m_pArrayInt = val;
}


bool GenericValidator::GetValue(bool *& val)
{
  if (m_pBool) {
    val = m_pBool;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(int *& val)
{
  if (m_pInt) {
    val = m_pInt;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(long *& val)
{
  if (m_pLong) {
    val = m_pLong;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(float *& val)
{
  if (m_pFloat) {
    val = m_pFloat;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(double *& val)
{
  if (m_pDouble) {
    val = m_pDouble;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(wxString *& val)
{
  if (m_pString) {
    val = m_pString;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(wxArrayInt *& val)
{
  if (m_pArrayInt) {
    val = m_pArrayInt;
    return true;
  }
  return false;
}


/*
bool GenericValidator::GetValue(bool & val)
{
  if (m_pBool) {
    val = *m_pBool;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(int & val)
{
  if (m_pInt) {
    val = *m_pInt;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(long & val)
{
  if (m_pLong) {
    val = *m_pLong;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(float & val)
{
  if (m_pFloat) {
    val = *m_pFloat;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(double & val)
{
  if (m_pDouble) {
    val = *m_pDouble;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(wxString & val)
{
  if (m_pString) {
    val = *m_pString;
    return true;
  }
  return false;
}


bool GenericValidator::GetValue(wxArrayInt & val)
{
  if (m_pArrayInt) {
    val = *m_pArrayInt;
    return true;
  }
  return false;
}
*/


bool GenericValidator::SetControlValue(const bool & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_CHECKBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxCheckBox)) ) {
    wxCheckBox* pControl = (wxCheckBox*) m_validatorWindow;
    pControl->SetValue(val) ;
    return true;
  } else
#endif
#if wxUSE_RADIOBTN
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxRadioButton)) ) {
    wxRadioButton* pControl = (wxRadioButton*) m_validatorWindow;
    pControl->SetValue(val) ;
    return true;
  } else
#endif
#if wxUSE_TOGGLEBTN
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxToggleButton)) ) {
    wxToggleButton *pControl = (wxToggleButton *) m_validatorWindow;
    pControl->SetValue(val) ;
    return true;
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::SetControlValue(const int & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_GAUGE
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxGauge)) ) {
    wxGauge* pControl = (wxGauge*) m_validatorWindow;
    pControl->SetValue(val) ;
    return true;
  } else
#endif
#if wxUSE_RADIOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxRadioBox)) ) {
    wxRadioBox* pControl = (wxRadioBox*) m_validatorWindow;
    pControl->SetSelection(val) ;
    return true;
  } else
#endif
#if wxUSE_SCROLLBAR
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxScrollBar)) ) {
    wxScrollBar* pControl = (wxScrollBar*) m_validatorWindow;
    pControl->SetThumbPosition(val) ;
    return true;
  } else
#endif
#if wxUSE_SPINCTRL && !defined(__WXMOTIF__)
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinCtrl)) ) {
    wxSpinCtrl* pControl = (wxSpinCtrl*) m_validatorWindow;
    pControl->SetValue(val) ;
    return true;
  } else
#endif
#if wxUSE_SPINBTN
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinButton)) ) {
    wxSpinButton* pControl = (wxSpinButton*) m_validatorWindow;
    pControl->SetValue(val) ;
    return true;
  } else
#endif
#if wxUSE_SLIDER
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxSlider)) ) {
    wxSlider* pControl = (wxSlider*) m_validatorWindow;
    pControl->SetValue(val) ;
    return true;
  } else
#endif
#if wxUSE_COMBOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
    wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
    pControl->SetSelection(val) ;
    return true;
  } else
#endif
#if wxUSE_CHOICE
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxChoice)) ) {
    wxChoice* pControl = (wxChoice*) m_validatorWindow;
    pControl->SetSelection(val) ;
    return true;
  } else
#endif
#if wxUSE_TEXTCTRL
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
    wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
    pControl->SetValue(StringConverter::toString(val));
    return true;
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::SetControlValue(const long & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_COMBOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
    wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
    string str = StringConverter::toString(val);
    if (pControl->FindString(str) != wxNOT_FOUND) {
      pControl->SetStringSelection(str);
      return true;
    }
    if ((m_validatorWindow->GetWindowStyle() & wxCB_READONLY) == 0) {
      pControl->SetValue(str);
      return true;
    }
  } else
#endif
#if wxUSE_TEXTCTRL
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
    wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
    pControl->SetValue(StringConverter::toString(val));
    return true;
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::SetControlValue(const float & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_COMBOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
    wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
    string str = StringConverter::toString(val);
    if (pControl->FindString(str) != wxNOT_FOUND) {
      pControl->SetStringSelection(str);
      return true;
    }
    if ((m_validatorWindow->GetWindowStyle() & wxCB_READONLY) == 0) {
      pControl->SetValue(str);
      return true;
    }
  } else
#endif
#if wxUSE_TEXTCTRL
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
    wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
    pControl->SetValue(StringConverter::toString(val));
    return true;
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::SetControlValue(const double & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_COMBOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
    wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
    string str = StringConverter::toString(val);
    if (pControl->FindString(str) != wxNOT_FOUND) {
      pControl->SetStringSelection(str);
      return true;
    }
    if ((m_validatorWindow->GetWindowStyle() & wxCB_READONLY) == 0) {
      pControl->SetValue(str);
      return true;
    }
  } else
#endif
#if wxUSE_TEXTCTRL
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
    wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
    pControl->SetValue(StringConverter::toString(val));
    return true;
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::SetControlValue(const wxString & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_BUTTON
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxButton)) ) {
    wxButton* pControl = (wxButton*) m_validatorWindow;
    pControl->SetLabel(val);
    return true;
  } else
#endif
#if wxUSE_COMBOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
    wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
    if (pControl->FindString(val) != wxNOT_FOUND) {
      pControl->SetStringSelection(val);
      return true;
    }
    if ((m_validatorWindow->GetWindowStyle() & wxCB_READONLY) == 0) {
      pControl->SetValue(val);
      return true;
    }
  } else
#endif
#if wxUSE_CHOICE
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxChoice)) ) {
    wxChoice* pControl = (wxChoice*) m_validatorWindow;
    if (pControl->FindString(val) != wxNOT_FOUND) {
      pControl->SetStringSelection(val);
      return true;
    }
  } else
#endif
#if wxUSE_STATTEXT
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxStaticText)) ) {
    wxStaticText* pControl = (wxStaticText*) m_validatorWindow;
    pControl->SetLabel(val);
    return true;
  } else
#endif
#if wxUSE_TEXTCTRL
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
    wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
    pControl->SetValue(val);
    return true;
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::SetControlValue(const wxArrayInt & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_CHECKLISTBOX
  // NOTE: wxCheckListBox isa wxListBox, so wxCheckListBox MUST come first:
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxCheckListBox)) ) {
    wxCheckListBox* pControl = (wxCheckListBox*) m_validatorWindow;
    // clear all selections
    size_t i, count = pControl->GetCount();
    for (i=0; i<count; ++i) {
      pControl->Check(i,false);
    }
    // select each item in our array
    count = val.GetCount();
    for (i=0; i<count; ++i) {
      pControl->Check(val.Item(i));
    }
    return true;
  } else
#endif
#if wxUSE_LISTBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxListBox)) ) {
    wxListBox* pControl = (wxListBox*) m_validatorWindow;
    // clear all selections
    size_t i, count = pControl->GetCount();
    for (i=0; i<count; ++i) {
      pControl->Deselect(i);
    }
    // select each item in our array
    count = val.GetCount();
    for (i=0; i<count; ++i) {
      pControl->SetSelection(val.Item(i));
    }
    return true;
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::GetControlValue(bool & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_CHECKBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxCheckBox)) ) {
    wxCheckBox* pControl = (wxCheckBox*) m_validatorWindow;
    val = pControl->GetValue() ;
    return true;
  } else
#endif
#if wxUSE_RADIOBTN
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxRadioButton)) ) {
    wxRadioButton* pControl = (wxRadioButton*) m_validatorWindow;
    val = pControl->GetValue() ;
    return true;
  } else
#endif
#if wxUSE_TOGGLEBTN
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxToggleButton)) ) {
    wxToggleButton *pControl = (wxToggleButton *) m_validatorWindow;
    val = pControl->GetValue() ;
    return true;
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::GetControlValue(int & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_GAUGE
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxGauge)) ) {
    wxGauge* pControl = (wxGauge*) m_validatorWindow;
    val = pControl->GetValue() ;
    return true;
  } else
#endif
#if wxUSE_RADIOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxRadioBox)) ) {
    wxRadioBox* pControl = (wxRadioBox*) m_validatorWindow;
    val = pControl->GetSelection() ;
    return true;
  } else
#endif
#if wxUSE_SCROLLBAR
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxScrollBar)) ) {
    wxScrollBar* pControl = (wxScrollBar*) m_validatorWindow;
    val = pControl->GetThumbPosition() ;
    return true;
  } else
#endif
#if wxUSE_SPINCTRL && !defined(__WXMOTIF__)
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinCtrl)) ) {
    wxSpinCtrl* pControl = (wxSpinCtrl*) m_validatorWindow;
    val = pControl->GetValue() ;
    return true;
  } else
#endif
#if wxUSE_SPINBTN
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinButton)) ) {
    wxSpinButton* pControl = (wxSpinButton*) m_validatorWindow;
    val = pControl->GetValue() ;
    return true;
  } else
#endif
#if wxUSE_SLIDER
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxSlider)) ) {
    wxSlider* pControl = (wxSlider*) m_validatorWindow;
    val = pControl->GetValue() ;
    return true;
  } else
#endif
#if wxUSE_COMBOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
    wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
    val = pControl->GetSelection() ;
    return true;
  } else
#endif
#if wxUSE_CHOICE
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxChoice)) ) {
    wxChoice* pControl = (wxChoice*) m_validatorWindow;
    val = pControl->GetSelection() ;
    return true;
  } else
#endif
#if wxUSE_TEXTCTRL
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
    wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
    return StringConverter::toValue(pControl->GetValue().c_str(),val);
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::GetControlValue(long & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_COMBOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
    wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
    string str;
    if (m_validatorWindow->GetWindowStyle() & wxCB_READONLY) {
      str = pControl->GetStringSelection();
    } else {
      str = pControl->GetValue();
    }
    return StringConverter::toValue(str,val);
  } else
#endif
#if wxUSE_TEXTCTRL
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
    wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
    return StringConverter::toValue(pControl->GetValue().c_str(),val);
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::GetControlValue(float & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_COMBOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
    wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
    string str;
    if (m_validatorWindow->GetWindowStyle() & wxCB_READONLY) {
      str = pControl->GetStringSelection();
    } else {
      str = pControl->GetValue();
    }
    return StringConverter::toValue(str,val);
  } else
#endif
#if wxUSE_TEXTCTRL
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
    wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
    string str;
    str = pControl->GetValue();
    return StringConverter::toValue(str,val);
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::GetControlValue(double & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_COMBOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
    wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
    string str;
    if (m_validatorWindow->GetWindowStyle() & wxCB_READONLY) {
      str = pControl->GetStringSelection();
    } else {
      str = pControl->GetValue();
    }
    return StringConverter::toValue(str,val);
  } else
#endif
#if wxUSE_TEXTCTRL
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
    wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
    string str;
    str = pControl->GetValue();
    return StringConverter::toValue(str,val);
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::GetControlValue(wxString & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_BUTTON
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxButton)) ) {
    wxButton* pControl = (wxButton*) m_validatorWindow;
    val = pControl->GetLabel() ;
    return true;
  } else
#endif
#if wxUSE_COMBOBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
    wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
    if (m_validatorWindow->GetWindowStyle() & wxCB_READONLY)
      val = pControl->GetStringSelection();
    else
      val = pControl->GetValue();
    return true;
  } else
#endif
#if wxUSE_CHOICE
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxChoice)) ) {
    wxChoice* pControl = (wxChoice*) m_validatorWindow;
    val = pControl->GetStringSelection();
    return true;
  } else
#endif
#if wxUSE_STATTEXT
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxStaticText)) ) {
    wxStaticText* pControl = (wxStaticText*) m_validatorWindow;
    val = pControl->GetLabel() ;
    return true;
  } else
#endif
#if wxUSE_TEXTCTRL
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
    wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
    val = pControl->GetValue() ;
    return true;
  } else
#endif
    return false;

  return false;
}


bool GenericValidator::GetControlValue(wxArrayInt & val)
{
  if ( !m_validatorWindow )
    return false;

#if wxUSE_CHECKLISTBOX
  // NOTE: wxCheckListBox isa wxListBox, so wxCheckListBox MUST come first:
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxCheckListBox)) ) {
    wxCheckListBox* pControl = (wxCheckListBox*) m_validatorWindow;
    pControl->GetSelections(val);
    return true;
  } else
#endif
#if wxUSE_LISTBOX
  if (m_validatorWindow->IsKindOf(CLASSINFO(wxListBox)) ) {
    wxListBox* pControl = (wxListBox*) m_validatorWindow;
    pControl->GetSelections(val);
    return true;
  } else
#endif
    return false;

  return false;
}


/**
 * Always returns true.
 * Automatically transfers value from the control to the internal pointer
 * and sends a validation event.
 */
bool GenericValidator::Validate(wxWindow * WXUNUSED(parent))
{
  if (HasValueChanged()) { 
    TransferFromWindow();
    GenerateEvent();
  }
  return true;
}


/**
 * Transfer the last known good value to the control.
 */
bool GenericValidator::TransferToWindow()
{
  if (!HasValue())
    return true;

  if ( !m_validatorWindow )
    return false;

  if (m_pBool) {
    return SetControlValue(*m_pBool);
  } else if (m_pInt) {
    return SetControlValue(*m_pInt);
  } else if (m_pLong) {
    return SetControlValue(*m_pLong);
  } else if (m_pFloat) {
    return SetControlValue(*m_pFloat);
  } else if (m_pDouble) {
    return SetControlValue(*m_pDouble);
  } else if (m_pString) {
    return SetControlValue(*m_pString);
  } else if (m_pArrayInt) {
    return SetControlValue(*m_pArrayInt);
  }

  // we fell through, so now what?
  return false;
}


/**
 * Store the control's value as the last known good value.
 */
bool GenericValidator::TransferFromWindow()
{
  if (!HasValue())
    return true;

  if (!m_validatorWindow)
    return false;

  if (m_pBool) {
    return GetControlValue(*m_pBool);
  } else if (m_pInt) {
    return GetControlValue(*m_pInt);
  } else if (m_pLong) {
    return GetControlValue(*m_pLong);
  } else if (m_pFloat) {
    return GetControlValue(*m_pFloat);
  } else if (m_pDouble) {
    return GetControlValue(*m_pDouble);
  } else if (m_pString) {
    return GetControlValue(*m_pString);
  } else if (m_pArrayInt) {
    return GetControlValue(*m_pArrayInt);
  }

  // we fell through, so now what?
  return false;
}


/**
 * Event handler for a user key stroke.
 * If the mouse is outside of the control, a 1-second timer is (re)started 
 * which will call Validate(0) upon firing.
 */
void GenericValidator::OnChar(wxKeyEvent& event)
{
  if (event.GetKeyCode() == WXK_RETURN) {
    if (p_timerEnabled) { p_timer.Stop(); }
    Validate(0);
  } else {
    if (p_timerEnabled) { p_timer.Start(1000, true); }
  }
  event.Skip();
}


/**
 * Event handler for the mouse leaving the control.
 * If the control's data has changed, a 1-second timer is started which will
 * call Validate(0) upon firing.
 */
void GenericValidator::OnLeaveWindow(wxMouseEvent& event)
{
  p_timerEnabled = true;

  bool focusFollowMouse = false;
  Preferences::getGlobalPref()->getBool(PrefLabels::FOCUSFOLLOWMOUSE,
                                        focusFollowMouse);

  if (focusFollowMouse) {
    Validate(0);
  } else {
    p_timer.Start(1000, true);
  }

  event.Skip();
}


/**
 * Event handler for the mouse entering the control.
 * Any running timers for the control are stopped, the text displayed is 
 * selected, and focus is acquired.
 */
void GenericValidator::OnEnterWindow(wxMouseEvent& event)
{
  if ( !m_validatorWindow )
    return;

  p_timer.Stop();
  p_timerEnabled = false;

  bool focusFollowMouse = false;
  Preferences::getGlobalPref()->getBool(PrefLabels::FOCUSFOLLOWMOUSE,
                                        focusFollowMouse);

  if (focusFollowMouse) {
    m_validatorWindow->SetFocus();

#if wxUSE_SPINCTRL && !defined(__WXMOTIF__)
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinCtrl)) ) {
      wxSpinCtrl* pControl = (wxSpinCtrl*) m_validatorWindow;
      pControl->SetSelection(-1,-1);
    } else
#endif
#if wxUSE_SLIDER
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSlider)) ) {
      wxSlider* pControl = (wxSlider*) m_validatorWindow;
      pControl->SetSelection(-1,-1) ;
    } else
#endif
#if wxUSE_COMBOBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) )
    {
      wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
      pControl->SetSelection(0,pControl->GetLastPosition()+1);
    } else
#endif
#if wxUSE_TEXTCTRL
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) )
    {
      wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
      pControl->SetSelection(-1,-1);
    } else
#endif

    ;   // to match the last 'else' above
    // unrecognized control or bad pointer, do nothing
  }

  event.Skip();
}


/**
 * Event handler for the validation timer.
 * Simply calls Validate(0) (and event.Skip().)
 */
void GenericValidator::OnTimer(wxTimerEvent& event)
{
  Validate(0);

  event.Skip();
}


/**
 * Event handler for a combo box selection.
 * Simply calls Validate(0) (and event.Skip().)
 */
void GenericValidator::OnSelection(wxCommandEvent& event)
{
  Validate(0);

  event.Skip();
}


/**
 * If we are storing a value or value reference, return true.
 */
bool GenericValidator::HasValue() const 
{
  return m_pBool
        || m_pInt
        || m_pLong
        || m_pFloat
        || m_pDouble
        || m_pString
        || m_pArrayInt;
}


/**
 *
 */
void GenericValidator::ClearValue() 
{
  m_pBool = 0;
  m_pInt = 0;
  m_pLong = 0;
  m_pFloat = 0;
  m_pDouble = 0;
  m_pString = 0;
  m_pArrayInt = 0;
}


/**
 */
void GenericValidator::GenerateEvent(const bool& success)
{
  if (m_validatorWindow) {
    wxCommandEvent event(wxEVT_VALIDATION_EVENT, m_validatorWindow->GetId());
    event.SetEventObject(m_validatorWindow);
    // indicate successful validation using IsChecked()
    event.SetInt(success ? 1 : 0);
    m_validatorWindow->GetEventHandler()->AddPendingEvent(event);
  }
}


/**
 * Determines if the value currently in the control is different than the 
 * last known good value.
 */
bool GenericValidator::HasValueChanged()
{
  bool bval;
  int ival;
  long lval;
  float fval;
  double dval;
  wxString sval;
  wxArrayInt aval;

  if (m_pBool && GetControlValue(bval)) {
    return *m_pBool != bval;
  } else if (m_pInt && GetControlValue(ival)) {
    return *m_pInt != ival;
  } else if (m_pLong && GetControlValue(lval)) {
    return *m_pLong != lval;
  } else if (m_pFloat && GetControlValue(fval)) {
    return *m_pFloat != fval;
  } else if (m_pDouble && GetControlValue(dval)) {
    return *m_pDouble != dval;
  } else if (m_pString && GetControlValue(sval)) {
    return *m_pString != sval;
  } else if (m_pArrayInt && GetControlValue(aval)) {
    // no == operator, must step through arrays
    if (m_pArrayInt->GetCount() == aval.GetCount()) {
      for (size_t idx = 0; idx < m_pArrayInt->GetCount(); ++idx) {
        if (m_pArrayInt->Item(idx) != aval.Item(idx)) {
          return true;
        }
      }
      return false;
    }
    return true;
  }

  return false;
}


/**
 */
void GenericValidator::Initialize()
{
  p_timer.SetOwner(this, wxID_ANY);
  p_timerEnabled = true;

  ClearValue();
}

