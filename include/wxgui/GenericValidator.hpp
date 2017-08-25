#ifndef GENERICVALIDATOR_H
#define GENERICVALIDATOR_H

#include <wx/timer.h>
#include <wx/validate.h>

DECLARE_EVENT_TYPE(wxEVT_VALIDATION_EVENT, -1)

/**
 * This validator does not perform any real validation but contains event 
 * handlers to be used by controls throughout ECCE.  It can be easily
 * extended for use in any widget type by overloading the Validate vunction.
 *
 * NOTE: the constructor taking a pointer to a long or double can
 * only currently be used by wxTextControl or wxComboBox.
 *
 * This code copied from wxGenericValidator and then modified.  The
 * wxWidget coding style is mostly maintained.
 */
class GenericValidator: public wxValidator
{
public:
  GenericValidator();
  GenericValidator(bool * val);
  GenericValidator(int * val);
  GenericValidator(long * val);
  GenericValidator(float * val);
  GenericValidator(double * val);
  GenericValidator(wxString * val);
  GenericValidator(wxArrayInt * val);
  virtual ~GenericValidator();

  virtual wxObject * Clone() const;
  bool Copy(const GenericValidator& val);

  virtual void SetValue(bool * val);
  virtual void SetValue(int * val);
  virtual void SetValue(long * val);
  virtual void SetValue(float * val);
  virtual void SetValue(double * val);
  virtual void SetValue(wxString * val);
  virtual void SetValue(wxArrayInt * val);

  virtual bool GetValue(bool *& val);
  virtual bool GetValue(int *& val);
  virtual bool GetValue(long *& val);
  virtual bool GetValue(float *& val);
  virtual bool GetValue(double *& val);
  virtual bool GetValue(wxString *& val);
  virtual bool GetValue(wxArrayInt *& val);

  /*
  virtual bool GetValue(bool & val);
  virtual bool GetValue(int & val);
  virtual bool GetValue(long & val);
  virtual bool GetValue(float & val);
  virtual bool GetValue(double & val);
  virtual bool GetValue(wxString & val);
  virtual bool GetValue(wxArrayInt & val);
  */

  virtual bool SetControlValue(const bool & val);
  virtual bool SetControlValue(const int & val);
  virtual bool SetControlValue(const long & val);
  virtual bool SetControlValue(const float & val);
  virtual bool SetControlValue(const double & val);
  virtual bool SetControlValue(const wxString & val);
  virtual bool SetControlValue(const wxArrayInt & val);

  virtual bool GetControlValue(bool & val);
  virtual bool GetControlValue(int & val);
  virtual bool GetControlValue(long & val);
  virtual bool GetControlValue(float & val);
  virtual bool GetControlValue(double & val);
  virtual bool GetControlValue(wxString & val);
  virtual bool GetControlValue(wxArrayInt & val);

  virtual bool Validate(wxWindow * WXUNUSED(parent));

  virtual bool TransferToWindow();
  virtual bool TransferFromWindow();

protected:
  // event handlers
  virtual void OnChar(wxKeyEvent& event);
  virtual void OnLeaveWindow(wxMouseEvent& event);
  virtual void OnEnterWindow(wxMouseEvent& event);
  virtual void OnTimer(wxTimerEvent& event);
  virtual void OnSelection(wxCommandEvent& event);

  bool HasValue() const;
  bool HasValueChanged();
  void ClearValue();

  void GenerateEvent(const bool& success=true);

  void Initialize();

  wxTimer     p_timer;
  bool        p_timerEnabled;

  bool       *m_pBool;
  int        *m_pInt;
  long       *m_pLong;
  float      *m_pFloat;
  double     *m_pDouble;
  wxString   *m_pString;
  wxArrayInt *m_pArrayInt;

private:
  DECLARE_DYNAMIC_CLASS(GenericValidator)
  DECLARE_NO_COPY_CLASS(GenericValidator)
  DECLARE_EVENT_TABLE()
};

const GenericValidator DefaultValidator;

#endif // GENERICVALIDATOR_H
