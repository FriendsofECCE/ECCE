#ifndef NUMERICVALIDATOR_H
#define NUMERICVALIDATOR_H

#include <wx/combobox.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/log.h>

#include "util/InternalException.H"
#include "util/StringConverter.H"
#include "util/UnitConverter.H"
#include "util/UnitFactory.H"

#include "wxgui/NumericValidatorBase.H"


template <class T>
class NumericValidator : public NumericValidatorBase
{
public:
  NumericValidator();
  NumericValidator(T * val, const T& min, const T& max,
          const bool& minClosed=true, const bool& maxClosed=true);
  NumericValidator(T * val, T * min, T * max,
          const bool& minClosed=true, const bool& maxClosed=true);
  virtual ~NumericValidator();

  virtual wxObject * Clone() const;
  bool Copy(const NumericValidator& that);

  void SetHardRange(const T& min, const T& max,
          const bool& minClosed=true, const bool& maxClosed=true);
  void SetHardRange(T * min, T * max,
          const bool& minClosed=true, const bool& maxClosed=true);
  void SetSoftRange(const T& min, const T& max,
          const bool& minClosed=true, const bool& maxClosed=true);
  void SetSoftRange(T * min, T * max,
          const bool& minClosed=true, const bool& maxClosed=true);

  bool IsHardMinClosed() const { return p_isHardMinClosed; }
  bool IsHardMaxClosed() const { return p_isHardMaxClosed; }
  void GetHardRange(T &min, T& max) const;
  virtual wxString GetHardRange() const;

  bool IsUsingSoftRange() const { return p_isUsingSoftRange; }
  bool IsSoftMinClosed() const { return p_isSoftMinClosed; }
  bool IsSoftMaxClosed() const { return p_isSoftMaxClosed; }
  void GetSoftRange(T &min, T& max) const;
  virtual wxString GetSoftRange() const;

  virtual bool Validate(wxWindow * WXUNUSED(parent));

  virtual bool TransferToWindow();
  virtual bool TransferFromWindow();

protected:
  virtual void OnChar(wxKeyEvent& event);

  void Initialize();

  void Convert(T & val, const wxString& from, const wxString& to) const;
  void Convert(T & min, T & max, const wxString& from, const wxString& to)const;

  bool p_isInternalHardRange;
  bool p_isHardMinClosed;
  bool p_isHardMaxClosed;
  T * p_hardMin;
  T * p_hardMax;
  
  bool p_isInternalSoftRange;
  bool p_isUsingSoftRange;
  bool p_isSoftMinClosed;
  bool p_isSoftMaxClosed;
  T * p_softMin;
  T * p_softMax;

private:
  DECLARE_NO_COPY_CLASS(NumericValidator)
};



template <class T>
NumericValidator<T>::NumericValidator()
  : NumericValidatorBase()
{
  Initialize();
}


template <class T>
NumericValidator<T>::NumericValidator(T * val, const T& min, const T& max,
        const bool& minClosed, const bool& maxClosed)
  : NumericValidatorBase(val)
{
  Initialize();
  p_isInternalHardRange = true;
  p_hardMin = new T(min);
  p_hardMax = new T(max);
  p_isHardMinClosed = minClosed;
  p_isHardMaxClosed = maxClosed;
}


template <class T>
NumericValidator<T>::NumericValidator(T * val, T * min, T * max,
        const bool& minClosed, const bool& maxClosed)
  : NumericValidatorBase(val)
{
  Initialize();
  p_isInternalHardRange = false;
  p_hardMin = min;
  p_hardMax = max;
  p_isHardMinClosed = minClosed;
  p_isHardMaxClosed = maxClosed;
}


template <class T>
NumericValidator<T>::~NumericValidator()
{
  if (p_isInternalHardRange) {
    delete p_hardMin;
    delete p_hardMax;
  }
  if (p_isInternalSoftRange) {
    delete p_softMin;
    delete p_softMax;
  }
}


template <class T>
wxObject * NumericValidator<T>::Clone() const
{
  NumericValidator<T> * copy = new NumericValidator<T>;
  copy->Copy(*this);
  return copy;
}


template <class T>
bool NumericValidator<T>::Copy(const NumericValidator<T>& that)
{
  bool ret = true;

  ret &= NumericValidatorBase::Copy(that);

  p_isInternalHardRange = that.p_isInternalHardRange;
  p_isHardMinClosed = that.p_isHardMinClosed;
  p_isHardMaxClosed = that.p_isHardMaxClosed;
  if (p_isInternalHardRange) {
    p_hardMin = new T(*that.p_hardMin);
    p_hardMax = new T(*that.p_hardMax);
  } else {
    p_hardMin = that.p_hardMin;
    p_hardMax = that.p_hardMax;
  }

  p_isInternalSoftRange = that.p_isInternalSoftRange;
  p_isUsingSoftRange = that.p_isUsingSoftRange;
  p_isSoftMinClosed = that.p_isSoftMinClosed;
  p_isSoftMaxClosed = that.p_isSoftMaxClosed;
  if (p_isInternalSoftRange) {
    p_softMin = new T(*that.p_softMin);
    p_softMax = new T(*that.p_softMax);
  } else {
    p_softMin = that.p_softMin;
    p_softMax = that.p_softMax;
  }

  return true;
}


template <class T>
void NumericValidator<T>::SetHardRange(const T& min, const T& max,
        const bool& minClosed, const bool& maxClosed)
{
  if (!p_isInternalHardRange) {
    p_hardMin = new T(min);
    p_hardMax = new T(max);
  } else {
    *p_hardMin = min;
    *p_hardMax = max;
  }
  p_isInternalHardRange = true;
  p_isHardMinClosed = minClosed;
  p_isHardMaxClosed = maxClosed;
}


template <class T>
void NumericValidator<T>::SetHardRange(T * min, T * max,
        const bool& minClosed, const bool& maxClosed)
{
  if (p_isInternalHardRange) {
    delete p_hardMin;
    delete p_hardMax;
  }
  p_isInternalHardRange = false;
  p_hardMin = min;
  p_hardMax = max;
  p_isHardMinClosed = minClosed;
  p_isHardMaxClosed = maxClosed;
}


template <class T>
void NumericValidator<T>::SetSoftRange(const T& min, const T& max,
        const bool& minClosed, const bool& maxClosed)
{
  p_isUsingSoftRange = true;
  if (!p_isInternalSoftRange) {
    p_softMin = new T(min);
    p_softMax = new T(max);
  } else {
    *p_softMin = min;
    *p_softMax = max;
  }
  p_isInternalSoftRange = true;
  p_isSoftMinClosed = minClosed;
  p_isSoftMaxClosed = maxClosed;
}


template <class T>
void NumericValidator<T>::SetSoftRange(T * min, T * max,
        const bool& minClosed, const bool& maxClosed)
{
  p_isUsingSoftRange = true;
  if (p_isInternalSoftRange) {
    delete p_softMin;
    delete p_softMax;
  }
  p_isInternalSoftRange = false;
  p_softMin = min;
  p_softMax = max;
  p_isSoftMinClosed = minClosed;
  p_isSoftMaxClosed = maxClosed;
}


template <class T>
void NumericValidator<T>::GetHardRange(T &min, T& max) const
{
  min = *p_hardMin;
  max = *p_hardMax;
}


template <class T>
void NumericValidator<T>::GetSoftRange(T &min, T& max) const
{
  min = *p_softMin;
  max = *p_softMax;
}


template <class T>
wxString NumericValidator<T>::GetHardRange() const
{
  wxString str;
  T min, max;
  GetHardRange(min,max);
  Convert(min,max,p_baseUnits,p_units);
  str << (p_isHardMinClosed ? "[" : "(");
  str << StringConverter::toString(min);
  str << "..";
  str << StringConverter::toString(max);
  str << (p_isHardMaxClosed ? "]" : ")");
  return str;
}


template <class T>
wxString NumericValidator<T>::GetSoftRange() const
{
  wxString str;
  T min, max;
  GetSoftRange(min,max);
  Convert(min,max,p_baseUnits,p_units);
  str << (p_isSoftMinClosed ? "[" : "(");
  str << StringConverter::toString(min);
  str << "..";
  str << StringConverter::toString(max);
  str << (p_isSoftMaxClosed ? "]" : ")");
  return str;
}


template <class T>
bool NumericValidator<T>::Validate(wxWindow * WXUNUSED(parent))
{
  wxString msg;
  T val;

  if (GetControlValue(val)) {
    if ((p_isHardMinClosed && val < *p_hardMin)
            || (!p_isHardMinClosed && val <= *p_hardMin)
            || (p_isHardMaxClosed && val > *p_hardMax)
            || (!p_isHardMaxClosed && val >= *p_hardMax)) {
      msg << val << " is outside allowable range.";
    } else if (p_isUsingSoftRange) {
      if ((p_isSoftMinClosed && val < *p_softMin)
              || (!p_isSoftMinClosed && val <= *p_softMin)
              || (p_isSoftMaxClosed && val > *p_softMax)
              || (!p_isSoftMaxClosed && val >= *p_softMax)) {
        msg << val << " is outside recommended range.";
        wxLogWarning("%s", msg.c_str());
        msg.Empty();
      }
    }
  } else {
    msg << val << " is not of the correct type.";
  }

  if (msg.IsEmpty()) {
    TransferFromWindow();
    GenerateEvent();
    return true;
  }
  else {
    wxLogError("%s", msg.c_str());
    TransferToWindow();
    GenerateEvent(false);
    return false;
  }

  return false; // unreachable
}



/**
 * Transfer the last known good value to the control.
 */
template <class T>
bool NumericValidator<T>::TransferToWindow()
{
  if (!HasValue())
    return true;

  if ( !m_validatorWindow )
    return false;

  T *value, controlValue;
  if (GetValue(value)) {
    controlValue = *value;
    Convert(controlValue,p_baseUnits,p_units);
    if (SetControlValue(controlValue)) {
      if (p_unitsLabel) {
        p_unitsLabel->SetLabel(p_units);
      }
      if (p_rangeLabel) {
        p_rangeLabel->SetLabel(GetHardRange());
      }
      return true;
    }
  }

  return false;
}


/**
 * Store the control's value as the last known good value.
 */
template <class T>
bool NumericValidator<T>::TransferFromWindow()
{
  if (!HasValue())
    return true;

  if (!m_validatorWindow)
    return false;

  T *value, controlValue;
  if (GetControlValue(controlValue)) {
    Convert(controlValue,p_units,p_baseUnits);
    if (GetValue(value)) {
      *value = controlValue;
      return true;
    }
  }

  return false;
}


template <class T>
void NumericValidator<T>::OnChar(wxKeyEvent& event)
{
  bool good = false;
  int keyCode = event.GetKeyCode();
  if (keyCode > WXK_SPACE && keyCode < WXK_DELETE) {
    wxString val,val2;
    long point; 
#if wxUSE_COMBOBOX
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxComboBox)) ) {
      wxComboBox* pControl = (wxComboBox*) m_validatorWindow;
      if ((m_validatorWindow->GetWindowStyle() & wxCB_READONLY) == 0) {
        val = pControl->GetValue();
        point = pControl->GetInsertionPoint();
      }
    } else
#endif
#if wxUSE_TEXTCTRL
    if (m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)) ) {
      wxTextCtrl* pControl = (wxTextCtrl*) m_validatorWindow;
      val = pControl->GetValue() ;
      point = pControl->GetInsertionPoint();
    } else
#endif
      val = "";

    if (!val.IsEmpty()) {
      wxString val2;
      val2 << val.Mid(0,point) << wxChar(keyCode) << val.Mid(point);
      val = val2;
      T tmp;
      if (StringConverter::toValue(val.c_str(),tmp)) {
        good = true;
      }
    } else {
      good = true;
    }
  } else {
    good = true;
  }

  if (good) {
    NumericValidatorBase::OnChar(event);
  }
}


template <class T>
void NumericValidator<T>::Initialize()
{
  p_isInternalHardRange = true;
  p_isHardMinClosed = true;
  p_isHardMaxClosed = true;
  p_hardMin = new T(0);
  p_hardMax = new T(0);
  
  p_isInternalSoftRange = true;
  p_isUsingSoftRange = false;
  p_isSoftMinClosed = true;
  p_isSoftMaxClosed = true;
  p_softMin = new T(0);
  p_softMax = new T(0);

  p_unitsClass = "";
  p_baseUnits = "";
  p_units = "";
}


template <class T>
void NumericValidator<T>::Convert(T & val,
        const wxString& from, const wxString& to) const
{
  if (p_unitsClass.IsEmpty()) return;
  if (p_baseUnits.IsEmpty()) return;
  if (p_units.IsEmpty()) return;

  // Get converter
  UnitFactory& unitFactory = UnitFactory::getInstance();
  UnitConverter& converter =
          unitFactory.getUnitConverter(p_unitsClass.c_str());
  converter.setBaseUnits(from.c_str());

  // convert value
  val = (T)converter.convertTo(val, to.c_str());
}


template <class T>
void NumericValidator<T>::Convert(T & min, T & max,
        const wxString& from, const wxString& to) const
{
  if (p_unitsClass.IsEmpty()) return;
  if (p_baseUnits.IsEmpty()) return;
  if (p_units.IsEmpty()) return;

  // Get converter
  UnitFactory& unitFactory = UnitFactory::getInstance();
  UnitConverter& converter =
          unitFactory.getUnitConverter(p_unitsClass.c_str());
  converter.setBaseUnits(from.c_str());

  // convert value
  min = (T)converter.convertTo(min, to.c_str());
  max = (T)converter.convertTo(max, to.c_str());
}

#endif // NUMERICVALIDATOR_H
