/**
 * @file
 *
 *
 */
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/gauge.h>
#include <wx/radiobox.h>
#include <wx/radiobut.h>
#include <wx/scrolbar.h>
#include <wx/spinbutt.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/slider.h>
#include <wx/tglbtn.h>

#include "util/Preferences.H"
#include "util/PreferenceLabels.H"

#include "wxgui/ewxGenericValidator.H"


IMPLEMENT_DYNAMIC_CLASS(ewxGenericValidator, wxGenericValidator)


BEGIN_EVENT_TABLE(ewxGenericValidator, wxGenericValidator)
    EVT_CHAR(ewxGenericValidator::onChar)
    EVT_LEAVE_WINDOW(ewxGenericValidator::onLeaveWindow)
    EVT_ENTER_WINDOW(ewxGenericValidator::onEnterWindow)
    EVT_TIMER(wxID_ANY, ewxGenericValidator::onTimer)
#if wxUSE_COMBOBOX
    EVT_COMBOBOX(wxID_ANY, ewxGenericValidator::onComboBoxSelected)
#endif
END_EVENT_TABLE()


/**
 * Default null validator.
 * Kind-of a hack, since the superclass wxGenericValidator does not have a
 * no argument constructor.  This constructor exists so that controls can 
 * have the ecce standard event handlers defined.
 */
ewxGenericValidator::ewxGenericValidator()
  :wxGenericValidator(new wxString(""))
{
  Initialize();

  // Reset the underlying wxGenericValidator to a blank slate.
  clearValue();
}


/**
 * Boolean type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 */
ewxGenericValidator::ewxGenericValidator(bool* val)
  :wxGenericValidator(val)
{
  Initialize();
}


/**
 * Integer type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 */
ewxGenericValidator::ewxGenericValidator(int* val)
  :wxGenericValidator(val)
{
  Initialize();
}


/**
 * Double type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 * Kind-of a hack, since the superclass wxGenericValidator does not have a
 * double argument constructor.
 */
ewxGenericValidator::ewxGenericValidator(double* val)
  :wxGenericValidator(new wxString(""))
{
  Initialize();

  // Reset the underlying wxGenericValidator to a blank slate.
  clearValue();

  m_pDouble = val;
}


/**
 * String type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 */
ewxGenericValidator::ewxGenericValidator(wxString* val)
  :wxGenericValidator(val)
{
  Initialize();
}


/**
 * Array type validator.
 * The parameter passed must not be deleted until the dialog containing the
 * validated control is dismissed.  The parameter pointer represents the last
 * known good value for the control.
 */
ewxGenericValidator::ewxGenericValidator(wxArrayInt* val)
  :wxGenericValidator(val)
{
  Initialize();
}


/**
 * Copy constructor.
 */
ewxGenericValidator::ewxGenericValidator(const ewxGenericValidator& copyFrom)
  :wxGenericValidator(copyFrom)
{ 
  Copy(copyFrom);
}


/**
 * Boolean type validator without variable access.
 * The last known good value is maintained but is not accessible to the 
 * parent dialog.  Used to supply a default value for the control.
 */
ewxGenericValidator::ewxGenericValidator(bool val)
  :wxGenericValidator(new bool(val))
{
  Initialize();
  p_internalCopy = true;
}


/**
 * Integer type validator without variable access.
 * The last known good value is maintained but is not accessible to the 
 * parent dialog.  Used to supply a default value for the control.
 */
ewxGenericValidator::ewxGenericValidator(int val)
  :wxGenericValidator(new int(val))
{
  Initialize();
  p_internalCopy = true;
}


/**
 * Double type validator without variable access.
 * The last known good value is maintained but is not accessible to the 
 * parent dialog.  Used to supply a default value for the control.
 * Kind-of a hack, since the superclass wxGenericValidator does not have a
 * double argument constructor.
 */
ewxGenericValidator::ewxGenericValidator(double val)
  :wxGenericValidator(new wxString(""))
{
  Initialize();
  p_internalCopy = true;

  // Reset the underlying wxGenericValidator to a blank slate.
  clearValue();

  m_pDouble = new double(val);
}


/**
 * String type validator without variable access.
 * The last known good value is maintained but is not accessible to the 
 * parent dialog.  Used to supply a default value for the control.
 */
ewxGenericValidator::ewxGenericValidator(wxString val)
  :wxGenericValidator(new wxString(val))
{
  Initialize();
  p_internalCopy = true;
}


/**
 * Array type validator without variable access.
 * The last known good value is maintained but is not accessible to the 
 * parent dialog.  Used to supply a default value for the control.
 */
ewxGenericValidator::ewxGenericValidator(wxArrayInt val)
  :wxGenericValidator(new wxArrayInt(val))
{
  Initialize();
  p_internalCopy = true;
}


/**
 * Destructor.
 * Deletes the internal pointer if the non-pointer constructors were used.
 */
ewxGenericValidator::~ewxGenericValidator()
{
  clearValue();
}


void ewxGenericValidator::setValue(bool* val)
{
  clearValue();
  p_internalCopy = false;
  m_pBool = val;
}


void ewxGenericValidator::setValue(int* val)
{
  clearValue();
  p_internalCopy = false;
  m_pInt = val;
}


void ewxGenericValidator::setValue(double* val)
{
  clearValue();
  p_internalCopy = false;
  m_pDouble = val;
}


void ewxGenericValidator::setValue(wxString* val)
{
  clearValue();
  p_internalCopy = false;
  m_pString = val;
}


void ewxGenericValidator::setValue(wxArrayInt* val)
{
  clearValue();
  p_internalCopy = false;
  m_pArrayInt = val;
}


void ewxGenericValidator::setValue(bool val)
{
  setValue(new bool(val));
  p_internalCopy = true;
}


void ewxGenericValidator::setValue(int val)
{
  setValue(new int(val));
  p_internalCopy = true;
}


void ewxGenericValidator::setValue(double val)
{
  setValue(new double(val));
  p_internalCopy = true;
}


void ewxGenericValidator::setValue(wxString val)
{
  setValue(new wxString(val));
  p_internalCopy = true;
}


void ewxGenericValidator::setValue(wxArrayInt val)
{
  setValue(new wxArrayInt(val));
  p_internalCopy = true;
}


/**
 *
 */
bool ewxGenericValidator::Copy(const ewxGenericValidator& copyFrom)
{
  bool ret = true;

  // shallow copy of pointers
  ret &= wxGenericValidator::Copy(copyFrom);
  m_pDouble = copyFrom.m_pDouble;

  p_timer.SetOwner(this, wxID_ANY);
  p_leaveAsEnter = copyFrom.p_leaveAsEnter;
  p_timerEnabled = copyFrom.p_timerEnabled;
  p_internalCopy = copyFrom.p_internalCopy;

  // deep copy of pointers if interally stored
  if (p_internalCopy) {
    if (m_pBool) { m_pBool = new bool(*m_pBool); }
    if (m_pInt) { m_pInt = new int(*m_pInt); }
    if (m_pDouble) { m_pDouble = new double(*m_pDouble); }
    if (m_pString) { m_pString = new wxString(*m_pString); }
    if (m_pArrayInt) { m_pArrayInt = new wxArrayInt(*m_pArrayInt); }
  }

  return ret;
}


/**
 * Always returns true.
 */
bool ewxGenericValidator::Validate(wxWindow * WXUNUSED(parent))
{
  return afterValidation(true);
}


/**
 * Transfer the last known good value to the control.
 */
bool ewxGenericValidator::TransferToWindow()
{
  // Note: if this validator is storing a double value, it is handled specially
  // because the base class wxGenericValidator does not naturally handle
  // double values.

  if (hasValue()) {
    if (m_pDouble) {
      if ( !m_validatorWindow ) {
        return false;
      } else {
        // string controls
#if wxUSE_COMBOBOX
        if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) )
        {
          wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
          wxString str;
          str.Printf(wxT("%f"), *m_pDouble);
          if (pControl->FindString(str) != wxNOT_FOUND) {
            pControl->SetStringSelection(str);
          }
          if ((m_validatorWindow->GetWindowStyle() & wxCB_READONLY) == 0) {
            pControl->SetValue(str);
          }
          return true;
        } else
#endif
#if wxUSE_TEXTCTRL
        if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) )
        {
          wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
          wxString str;
          str.Printf(wxT("%f"), *m_pDouble);
          pControl->SetValue(str);
          return true;
        } else
#endif
          // unrecognized control for double use, or bad pointer
          return false; // to match the last 'else' above
      }
    }
    else {
      return wxGenericValidator::TransferToWindow();
    }
  }
  return true;
}


/**
 * Store the control's value as the last known good value.
 */
bool ewxGenericValidator::TransferFromWindow()
{
  // Note: if this validator is storing a double value, it is handled specially
  // because the base class wxGenericValidator does not naturally handle
  // double values.

  if (hasValue()) {
    if (m_pDouble) {
      if (!m_validatorWindow) {
        return false;
      } else {
        // string controls
#if wxUSE_COMBOBOX
        if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) )
        {
          wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
          wxString value;
          if (m_validatorWindow->GetWindowStyle() & wxCB_READONLY) {
            value = pControl->GetStringSelection();
          } else {
            value = pControl->GetValue();
          }
          if (!value.IsEmpty()) {
              *m_pDouble = wxAtof(value);
              return true;
          }
        } else
#endif
#if wxUSE_TEXTCTRL
        if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) )
        {
          wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
          wxString value = pControl->GetValue();
          if (!value.IsEmpty()) {
              *m_pDouble = wxAtof(value);
              return true;
          }
        } else
#endif
          // unrecognized control for double use, or bad pointer
          return false; // to match the last 'else' above
      }
    } else {
      return wxGenericValidator::TransferFromWindow();
    }
  }
  return true;
}


/**
 * Event handler for a user key stroke.
 * If the mouse is outside of the control, a 1-second timer is (re)started 
 * which will call Validate(0) upon firing.
 */
void ewxGenericValidator::onChar(wxKeyEvent& event)
{
  if (event.GetKeyCode() == WXK_RETURN) {
    if (hasChanged()) {
      if (p_timerEnabled) { p_timer.Stop(); }
      Validate(0);
    }
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
void ewxGenericValidator::onLeaveWindow(wxMouseEvent& event)
{
  p_timerEnabled = true;

  bool focusFollowMouse = false;
  Preferences::getGlobalPref()->getBool(PrefLabels::FOCUSFOLLOWMOUSE,
                                        focusFollowMouse);

  if (focusFollowMouse) {
    if (hasChanged()) { Validate(0); }
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
void ewxGenericValidator::onEnterWindow(wxMouseEvent& event)
{
  p_timer.Stop();
  p_timerEnabled = false;

  if ( !m_validatorWindow )
    return;

  bool focusFollowMouse = false;
  Preferences::getGlobalPref()->getBool(PrefLabels::FOCUSFOLLOWMOUSE,
                                        focusFollowMouse);

  if (focusFollowMouse) {
    m_validatorWindow->SetFocus();

    // int controls
#if wxUSE_SPINCTRL && !defined(__WXMOTIF__)
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinCtrl)) )
    {
      wxSpinCtrl* pControl = (wxSpinCtrl*) m_validatorWindow;
      pControl->SetSelection(-1,-1);
      return;
    } else
#endif
#if wxUSE_SLIDER
      // Windows 95 only
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSlider)) )
    {
      wxSlider* pControl = (wxSlider*) m_validatorWindow;
      pControl->SetSelection(-1,-1) ;
      return;
    } else
#endif

    // string controls
#if wxUSE_COMBOBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) )
    {
      wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
      pControl->SetSelection(0,pControl->GetLastPosition()+1);
      return;
    } else
#endif
#if wxUSE_TEXTCTRL
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) )
    {
      wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
      pControl->SetSelection(-1,-1);
      return;
    } else
#endif

    ;   // to match the last 'else' above
    // unrecognized control or bad pointer, do nothing
  }

  event.Skip();
}


/**
 * Event handler for the validation timer.
 * Simply calls Validate(0).
 */
void ewxGenericValidator::onTimer(wxTimerEvent& event)
{
  if (hasChanged()) { Validate(0); }

  event.Skip();
}


/**
 * Event handler for a combo box selection.
 * Simply calls Validate(0).
 */
void ewxGenericValidator::onComboBoxSelected(wxCommandEvent& event)
{
  if (hasChanged()) { Validate(0); }

  event.Skip();
}


/**
 * If we are storing a value or value reference, return true.
 */
bool ewxGenericValidator::hasValue() const 
{
  return (m_pBool) || (m_pInt) || (m_pDouble) || (m_pString) || (m_pArrayInt);
}


/**
 *
 */
void ewxGenericValidator::clearValue() 
{
  if (p_internalCopy) {
    if (m_pBool)      { delete m_pBool; }
    if (m_pInt)       { delete m_pInt; }
    if (m_pDouble)    { delete m_pDouble; }
    if (m_pString)    { delete m_pString; }
    if (m_pArrayInt)  { delete m_pArrayInt; }
  }
  m_pBool = 0;
  m_pInt = 0;
  m_pDouble = 0;
  m_pString = 0;
  m_pArrayInt = 0;
}


/**
 * Set whether the mouse leaving event should also generate an
 * EVT_COMMAND_TEXT_ENTER.
 */
void ewxGenericValidator::setLeaveAsEnter(bool value) 
{
  p_leaveAsEnter = value;
}


/**
 * Return the last known good value for this validator as a wxString.
 */
wxString ewxGenericValidator::getValue() const
{
  wxString ret("");

  if (m_pBool) {
    ret.Printf("%u", *m_pBool);
  } else if (m_pInt) {
    ret.Printf("%d", *m_pInt);
  } else if (m_pDouble) {
    ret.Printf("%f", *m_pDouble);
  } else if (m_pString) {
    ret = *m_pString;
  } else if (m_pArrayInt) {
    if (!m_pArrayInt->IsEmpty())
      ret.Printf("%d", m_pArrayInt->Last());
  }

  return ret;
}


/**
 * Return the value currently stored by the control window as a wxString, no 
 * matter what kind of object is stored internally (conversion to wxString).
 */
wxString ewxGenericValidator::getControlValue()
{
  bool* pBool = 0;
  int* pInt = 0;
  double* pDouble = 0;
  wxString* pString = 0;
  wxArrayInt* pArrayInt = 0;

  wxString ret = "";

  bool check = getControlValue(pBool, pInt, pDouble, pString, pArrayInt);
  if (check) {
         if (pBool)     { ret.Printf("%u", *pBool); }
    else if (pInt)      { ret.Printf("%d", *pInt); }
    else if (pDouble)   { ret.Printf("%f", *pDouble); }
    else if (pString)   { ret = *pString; }
    else if (pArrayInt) {
      if (!pArrayInt->IsEmpty()) { ret.Printf("%d", pArrayInt->Last()); }
    }
  }

  return ret;
}


/**
 * These actions are to be performed conditionally at the end of a Validate().  
 * Transfers data from the control to the internal last known good value 
 * variable.  Also generates an EVT_TEXT_ENTER if the option to map leave 
 * events as text enters is set.
 *
 * @param ok  The value that Validate would return
 * @see       Validate(wxWindow * WXUNUSED(parent))
 * @return    Returns the value of the parameter ok
 */
bool ewxGenericValidator::afterValidation(bool ok)
{
  // save the last known good value
  // or replace the current control's displayed value
  if (ok) {
    TransferFromWindow();
  } else {
    TransferToWindow();
  }

  if (p_leaveAsEnter) {
    wxWindow* window = (wxWindow*) m_validatorWindow;
    wxCommandEvent event(wxEVT_COMMAND_TEXT_ENTER, window->GetId());
    event.SetEventObject(window);
    if (m_pBool) { event.SetInt(*m_pBool); }
    else if (m_pInt) { event.SetInt(*m_pInt); }
    else if (m_pString) { event.SetString(*m_pString); }
    window->GetEventHandler()->ProcessEvent(event);
  }

  return ok;
}


/**
 *
 */
bool ewxGenericValidator::getControlValueAsBool(bool &cBool)
{
  bool* pBool = 0;
  int* pInt = 0;
  double* pDouble = 0;
  wxString* pString = 0;
  wxArrayInt* pArrayInt = 0;

  bool ret = getControlValue(pBool, pInt, pDouble, pString, pArrayInt);
  if (pBool && ret) { cBool = *pBool; delete pBool; }
  else { ret = false; }
  return ret;
}


/**
 *
 */
bool ewxGenericValidator::getControlValueAsInt(int &cInt)
{
  bool* pBool = 0;
  int* pInt = 0;
  double* pDouble = 0;
  wxString* pString = 0;
  wxArrayInt* pArrayInt = 0;

  bool ret = getControlValue(pBool, pInt, pDouble, pString, pArrayInt);
  if (pInt && ret) { cInt = *pInt; delete pInt; }
  else { ret = false; }
  return ret;
}


/**
 *
 */
bool ewxGenericValidator::getControlValueAsDouble(double &cDouble)
{
  bool* pBool = 0;
  int* pInt = 0;
  double* pDouble = 0;
  wxString* pString = 0;
  wxArrayInt* pArrayInt = 0;

  bool ret = getControlValue(pBool, pInt, pDouble, pString, pArrayInt);
  if (pDouble && ret) { cDouble = *pDouble; delete pDouble; }
  else { ret = false; }
  return ret;
}


/**
 *
 */
bool ewxGenericValidator::getControlValueAsString(wxString &cString)
{
  bool* pBool = 0;
  int* pInt = 0;
  double* pDouble = 0;
  wxString* pString = 0;
  wxArrayInt* pArrayInt = 0;

  bool ret = getControlValue(pBool, pInt, pDouble, pString, pArrayInt);
  if (pString && ret) { cString = *pString; delete pString; }
  else { ret = false; }
  return ret;
}


/**
 *
 */
bool ewxGenericValidator::getControlValueAsArrayInt(wxArrayInt &cArrayInt)
{
  bool* pBool = 0;
  int* pInt = 0;
  double* pDouble = 0;
  wxString* pString = 0;
  wxArrayInt* pArrayInt = 0;

  bool ret = getControlValue(pBool, pInt, pDouble, pString, pArrayInt);
  if (pArrayInt && ret) { cArrayInt = *pArrayInt; delete pArrayInt; }
  else { ret = false; }
  return ret;
}


/**
 * Determines if the value currently in the control is different than the 
 * last known good value.
 */
bool ewxGenericValidator::hasChanged()
{
  bool* pBool = 0;
  int* pInt = 0;
  double* pDouble = 0;
  wxString* pString = 0;
  wxArrayInt* pArrayInt = 0;

  bool ret = getControlValue(pBool, pInt, pDouble, pString, pArrayInt);
  if (ret) {
    if (m_pBool && pBool) {
      ret = *m_pBool != *pBool;
      delete pBool;
    } else if (m_pInt && pInt) {
      ret = *m_pInt != *pInt;
      delete pInt;
    } else if (m_pDouble && pDouble) {
      ret = *m_pDouble != *pDouble;
      delete pDouble;
    } else if (m_pString && pString) {
      ret = *m_pString != *pString;
      delete pString;
    } else if (m_pArrayInt && pArrayInt) {
      if (m_pArrayInt->GetCount() == pArrayInt->GetCount()) {
        size_t i, count = m_pArrayInt->GetCount();
        ret = false;
        for (i = 0; i < count; i++) {
          if (pArrayInt->Index((*m_pArrayInt)[i]) == wxNOT_FOUND) {
            ret = true;
            break;
          }
        }
      } else {
        ret = true;
      }
      delete pArrayInt;
    } else {
      ret = true;
    }
  } else {
    ret = true;
  }
  return ret;
}


/**
 *
 */
void ewxGenericValidator::Initialize()
{
  p_timer.SetOwner(this, wxID_ANY);
  p_leaveAsEnter = true;
  p_timerEnabled = true;
  p_internalCopy = false;

  m_pDouble = 0;
}


/**
 * Get the current value displayed by the control we are validating.
 * It is impossible to determine the type and return it, so we get everything.
 * Should only be used internally.
 */
bool ewxGenericValidator::getControlValue(bool* &cBool, int* &cInt,
                                          double* &cDouble, wxString* &cString,
                                          wxArrayInt* &cArrayInt)
{
    if ( !m_validatorWindow )
        return false;

    // BOOL CONTROLS **************************************
#if wxUSE_CHECKBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxCheckBox)) ) {
        wxCheckBox* pControl = (wxCheckBox*) m_validatorWindow;
        cBool = new bool(pControl->GetValue()) ;
        return true;
    } else
#endif
#if wxUSE_RADIOBTN
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxRadioButton)) ) {
        wxRadioButton* pControl = (wxRadioButton*) m_validatorWindow;
        cBool = new bool(pControl->GetValue()) ;
        return true;
    } else
#endif
#if wxUSE_TOGGLEBTN
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxToggleButton)) ) {
        wxToggleButton *pControl = (wxToggleButton *) m_validatorWindow;
        cBool = new bool(pControl->GetValue()) ;
        return true;
    } else
#endif

    // INT CONTROLS ***************************************
#if wxUSE_GAUGE
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxGauge)) ) {
        wxGauge* pControl = (wxGauge*) m_validatorWindow;
        cInt = new int(pControl->GetValue()) ;
        return true;
    } else
#endif
#if wxUSE_RADIOBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxRadioBox)) ) {
        wxRadioBox* pControl = (wxRadioBox*) m_validatorWindow;
        cInt = new int(pControl->GetSelection()) ;
        return true;
    } else
#endif
#if wxUSE_SCROLLBAR
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxScrollBar)) ) {
        wxScrollBar* pControl = (wxScrollBar*) m_validatorWindow;
        cInt = new int(pControl->GetThumbPosition()) ;
        return true;
    } else
#endif
#if wxUSE_SPINCTRL && !defined(__WXMOTIF__)
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinCtrl)) ) {
        wxSpinCtrl* pControl = (wxSpinCtrl*) m_validatorWindow;
        cInt = new int(pControl->GetValue());
        return true;
    } else
#endif
#if wxUSE_SPINBTN
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSpinButton)) ) {
        wxSpinButton* pControl = (wxSpinButton*) m_validatorWindow;
        cInt = new int(pControl->GetValue()) ;
        return true;
    } else
#endif
#if wxUSE_SLIDER
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxSlider)) ) {
        wxSlider* pControl = (wxSlider*) m_validatorWindow;
        cInt = new int(pControl->GetValue()) ;
        return true;
    } else
#endif

    // STRING CONTROLS ************************************
#if wxUSE_BUTTON
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxButton)) ) {
        wxButton* pControl = (wxButton*) m_validatorWindow;
        cString = new wxString(pControl->GetLabel()) ;
        return true;
    } else
#endif
#if wxUSE_COMBOBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
        wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
        if (m_pInt) {
            cInt = new int(pControl->GetSelection()) ;
            return true;
        } else if (m_pString) {
            if (m_validatorWindow->GetWindowStyle() & wxCB_READONLY)
                cString = new wxString(pControl->GetStringSelection());
            else
                cString = new wxString(pControl->GetValue());
            return true;
        } else if (m_pDouble) {
            if (m_validatorWindow->GetWindowStyle() & wxCB_READONLY)
                cDouble = new double(wxAtof(pControl->GetStringSelection()));
            else
                cDouble = new double(wxAtof(pControl->GetValue()));
            return true;
        }
    } else
#endif
#if wxUSE_CHOICE
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxChoice)) ) {
        wxChoice* pControl = (wxChoice*) m_validatorWindow;
        if (m_pInt) {
            cInt = new int(pControl->GetSelection()) ;
            return true;
        } else if (m_pString) {
            cString = new wxString(pControl->GetStringSelection());
            return true;
        }
    } else
#endif
#if wxUSE_STATTEXT
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxStaticText)) ) {
        wxStaticText* pControl = (wxStaticText*) m_validatorWindow;
        cString = new wxString(pControl->GetLabel()) ;
        return true;
    } else
#endif
#if wxUSE_TEXTCTRL
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
        wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
        wxString value = pControl->GetValue();
        if (!value.IsEmpty()) {
            if (m_pString) {
                cString = new wxString(value) ;
                return true;
            } else if (m_pInt) {
                cInt = new int(wxAtoi(value));
                return true;
            } else if (m_pDouble) {
                cDouble = new double(wxAtof(value));
                return true;
            }
        }
    } else
#endif

    // ARRAY CONTROLS *************************************
#if wxUSE_CHECKLISTBOX
    // NOTE: wxCheckListBox isa wxListBox, so wxCheckListBox MUST come first:
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxCheckListBox)) ) {
        wxCheckListBox* pControl = (wxCheckListBox*) m_validatorWindow;
        if (m_pArrayInt) {
            // create new empty array
            cArrayInt = new wxArrayInt();
            pControl->GetSelections(*cArrayInt);
            return true;
        }
        else
            return false;
    } else
#endif
#if wxUSE_LISTBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxListBox)) ) {
        wxListBox* pControl = (wxListBox*) m_validatorWindow;
        if (m_pArrayInt) {
            // create new empty array
            cArrayInt = new wxArrayInt();
            pControl->GetSelections(*cArrayInt);
            return true;
        }
    } else
#endif

    // unrecognized control, or bad pointer
        return false;

    return false;
}

