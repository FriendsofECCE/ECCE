/**
 * @file
 *
 *
 */
#ifndef EWXGENERICVALIDATOR_H
#define EWXGENERICVALIDATOR_H

#include <wx/timer.h>
#include <wx/valgen.h>

/**
 * This validator does not perform any real validation but contains event 
 * handlers to be used by controls throughout ECCE.  It can be easily
 * extended for use in any widget type by overloading the Validate vunction.
 *
 * If a data pointer is used, it will always contain the "Validate"d data.
 *
 * NOTE: the constructor taking either a double or a pointer to a double can
 * only currently be used by wxTextControl or wxComboBox.  Untested; use with
 * caution.
 *
 * This code copied from wxGenericValidator and then modified.  The
 * wxWidget coding style is mostly maintained.
 */
class ewxGenericValidator : public wxGenericValidator
{
DECLARE_DYNAMIC_CLASS(ewxGenericValidator)
DECLARE_EVENT_TABLE()
public:
  ewxGenericValidator();

  ewxGenericValidator(bool* val);
  ewxGenericValidator(int* val);
  ewxGenericValidator(double* val);
  ewxGenericValidator(wxString* val);
  ewxGenericValidator(wxArrayInt* val);
  ewxGenericValidator(const ewxGenericValidator& copyFrom);

  ewxGenericValidator(bool val);
  ewxGenericValidator(int val);
  ewxGenericValidator(double val);
  ewxGenericValidator(wxString val);
  ewxGenericValidator(wxArrayInt val);

  ~ewxGenericValidator();

  virtual void setValue(bool* val);
  virtual void setValue(int* val);
  virtual void setValue(double* val);
  virtual void setValue(wxString* val);
  virtual void setValue(wxArrayInt* val);

  virtual void setValue(bool val);
  virtual void setValue(int val);
  virtual void setValue(double val);
  virtual void setValue(wxString val);
  virtual void setValue(wxArrayInt val);

  // Make a clone of this validator (or return NULL) - currently necessary
  // if you're passing a reference to a validator.
  // Another possibility is to always pass a pointer to a new validator
  // (so the calling code can use a copy constructor of the relevant class).
  virtual wxObject *Clone() const { return new ewxGenericValidator(*this); }
  bool Copy(const ewxGenericValidator& val);
  virtual bool Validate(wxWindow * WXUNUSED(parent));
  virtual bool TransferToWindow();
  virtual bool TransferFromWindow();

  void setLeaveAsEnter(bool value);

  virtual wxString getValue() const;
  virtual wxString getControlValue();

protected:
  // event handlers
  void onChar(wxKeyEvent& event);
  void onLeaveWindow(wxMouseEvent& event);
  void onEnterWindow(wxMouseEvent& event);
  void onTimer(wxTimerEvent& event);
  void onComboBoxSelected(wxCommandEvent& event);

  bool hasValue() const;
  void clearValue();

  virtual bool afterValidation(bool ok);
  virtual bool getControlValueAsBool(bool &cBool);
  virtual bool getControlValueAsInt(int &cInt);
  virtual bool getControlValueAsDouble(double &cDouble);
  virtual bool getControlValueAsString(wxString &cString);
  virtual bool getControlValueAsArrayInt(wxArrayInt &cArrayInt);
  virtual bool hasChanged();

  void Initialize();

  wxTimer     p_timer;
  bool        p_leaveAsEnter;
  bool        p_timerEnabled;
  bool        p_internalCopy;

  double     *m_pDouble;

private:
// Cannot use
//  DECLARE_NO_COPY_CLASS(ewxGenericValidator)
// because copy constructor is explicitly declared above;
// but no copy assignment operator is defined, so declare
// it private to prevent the compiler from defining it:
  ewxGenericValidator& operator=(const ewxGenericValidator&);

  bool getControlValue(bool* &cBool, int* &cInt, double* &cDouble, 
                       wxString* &cString, wxArrayInt* &cArrayInt);
};

const ewxGenericValidator ewxDefaultValidator;

#endif // EWXGENERICVALIDATOR_H
