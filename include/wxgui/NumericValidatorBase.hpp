#ifndef NUMERICVALIDATORBASE_H_
#define NUMERICVALIDATORBASE_H_

#include "wxgui/GenericValidator.H"

class wxStaticText;


class NumericValidatorBase : public GenericValidator
{
public:
  NumericValidatorBase();
  NumericValidatorBase(bool * val);
  NumericValidatorBase(int * val);
  NumericValidatorBase(long * val);
  NumericValidatorBase(float * val);
  NumericValidatorBase(double * val);
  NumericValidatorBase(wxString * val);
  NumericValidatorBase(wxArrayInt * val);
  virtual ~NumericValidatorBase();

  virtual wxObject * Clone() const;
  bool Copy(const NumericValidatorBase& val);

  virtual void SetUnitClass(const wxString& clazz);
  virtual void SetBaseFamily(const wxString& family);
  virtual void SetFamily(const wxString& family);
  virtual void SetBaseUnits(const wxString& units);
  virtual void SetUnits(const wxString& units);
  virtual void SetUnitsLabel(wxStaticText * text);
  virtual void SetRangeLabel(wxStaticText * text);

  NumericValidatorBase& UnitClass(const wxString& clazz);
  NumericValidatorBase& BaseFamily(const wxString& family);
  NumericValidatorBase& Family(const wxString& family);
  NumericValidatorBase& BaseUnits(const wxString& units);
  NumericValidatorBase& Units(const wxString& units);
  NumericValidatorBase& UnitsLabel(wxStaticText * text);
  NumericValidatorBase& RangeLabel(wxStaticText * text);

  virtual wxString GetUnitClass() const { return p_unitsClass; }
  virtual wxString GetBaseUnits() const { return p_baseUnits; }
  virtual wxString GetUnits() const { return p_units; }
  virtual wxStaticText* GetUnitsLabel() const { return p_unitsLabel; }
  virtual wxStaticText* GetRangeLabel() const { return p_rangeLabel; }
  virtual wxString GetHardRange() const;
  virtual wxString GetSoftRange() const;

protected:
  void Initialize();

  wxString p_unitsClass;
  wxString p_baseUnits;
  wxString p_units;

  wxStaticText * p_unitsLabel;
  wxStaticText * p_rangeLabel;

private:
  DECLARE_NO_COPY_CLASS(NumericValidatorBase)
};

#endif // NUMERICVALIDATORBASE_H_
