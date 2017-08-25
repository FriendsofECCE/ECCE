#include <wx/stattext.h>

#include "util/InternalException.H"
#include "util/UnitFactory.H"
#include "util/UnitFamily.H"

#include "wxgui/NumericValidator.H"


NumericValidatorBase::NumericValidatorBase()
  : GenericValidator()
{
  Initialize();
}


NumericValidatorBase::NumericValidatorBase(bool * val)
  : GenericValidator(val)
{
  Initialize();
}


NumericValidatorBase::NumericValidatorBase(int * val)
  : GenericValidator(val)
{
  Initialize();
}


NumericValidatorBase::NumericValidatorBase(long * val)
  : GenericValidator(val)
{
  Initialize();
}


NumericValidatorBase::NumericValidatorBase(float * val)
  : GenericValidator(val)
{
  Initialize();
}


NumericValidatorBase::NumericValidatorBase(double * val)
  : GenericValidator(val)
{
  Initialize();
}


NumericValidatorBase::NumericValidatorBase(wxString * val)
  : GenericValidator(val)
{
  Initialize();
}


NumericValidatorBase::NumericValidatorBase(wxArrayInt * val)
  : GenericValidator(val)
{
  Initialize();
}


NumericValidatorBase::~NumericValidatorBase()
{
}


wxObject * NumericValidatorBase::Clone() const
{
  NumericValidatorBase * copy = new NumericValidatorBase;
  copy->Copy(*this);
  return copy;
}


bool NumericValidatorBase::Copy(const NumericValidatorBase& that)
{
  bool ret = true;

  // shallow copy of pointers
  ret &= GenericValidator::Copy(that);

  p_unitsClass = that.p_unitsClass;
  p_baseUnits = that.p_baseUnits;
  p_units = that.p_units;

  p_unitsLabel = that.p_unitsLabel;
  p_rangeLabel = that.p_rangeLabel;

  return ret;
}


void NumericValidatorBase::SetUnitClass(const wxString& clazz)
{
  INTERNALEXCEPTION(p_unitsClass.IsEmpty(),
          "Must only call SetUnitClass once");
  p_unitsClass = clazz;
}


void NumericValidatorBase::SetBaseFamily(const wxString& family)
{
  if (p_unitsClass.IsEmpty()) return;

  INTERNALEXCEPTION(p_baseUnits.IsEmpty(),
          "Must only call SetBaseFamily or SetBaseUnits once");
  UnitFactory& unitFactory = UnitFactory::getInstance();
  UnitFamily& unitFamily = unitFactory.getUnitFamily(family.c_str());
  SetBaseUnits(unitFamily.get(p_unitsClass.c_str()));
}


void NumericValidatorBase::SetFamily(const wxString& family)
{
  if (p_unitsClass.IsEmpty()) return;

  UnitFactory& unitFactory = UnitFactory::getInstance();
  UnitFamily& unitFamily = unitFactory.getUnitFamily(family.c_str());
  SetUnits(unitFamily.get(p_unitsClass.c_str()));
}


void NumericValidatorBase::SetBaseUnits(const wxString& units)
{
  INTERNALEXCEPTION(p_baseUnits.IsEmpty(),
          "Must only call SetBaseUnits once");
  p_baseUnits = units;
}


void NumericValidatorBase::SetUnits(const wxString& units)
{
  p_units = units;
  if (p_unitsLabel) {
    p_unitsLabel->SetLabel(p_units);
  }
}


void NumericValidatorBase::SetUnitsLabel(wxStaticText * text)
{
  p_unitsLabel = text;
  if (p_unitsLabel) {
    p_unitsLabel->SetLabel(p_units);
  }
}


void NumericValidatorBase::SetRangeLabel(wxStaticText * text)
{
  p_rangeLabel = text;
  if (p_rangeLabel) {
    p_rangeLabel->SetLabel(GetHardRange());
  }
}


NumericValidatorBase& NumericValidatorBase::UnitClass(const wxString& clazz)
{
  SetUnitClass(clazz);
  return *this;
}


NumericValidatorBase& NumericValidatorBase::BaseFamily(const wxString& family)
{
  SetBaseFamily(family);
  return *this;
}


NumericValidatorBase& NumericValidatorBase::Family(const wxString& family)
{
  SetFamily(family);
  return *this;
}


NumericValidatorBase& NumericValidatorBase::BaseUnits(const wxString& units)
{
  SetBaseUnits(units);
  return *this;
}


NumericValidatorBase& NumericValidatorBase::Units(const wxString& units)
{
  SetUnits(units);
  return *this;
}


NumericValidatorBase& NumericValidatorBase::UnitsLabel(wxStaticText * text)
{
  SetUnitsLabel(text);
  return *this;
}


NumericValidatorBase& NumericValidatorBase::RangeLabel(wxStaticText * text)
{
  SetRangeLabel(text);
  return *this;
}


wxString NumericValidatorBase::GetHardRange() const
{
  return "";
}


wxString NumericValidatorBase::GetSoftRange() const
{
  return "";
}


void NumericValidatorBase::Initialize()
{
  p_unitsClass = "";
  p_baseUnits = "";
  p_units = "";

  p_unitsLabel = NULL;
  p_rangeLabel = NULL;
}


