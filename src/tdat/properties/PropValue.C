///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropValue.C
//
//
// DESIGN:
//         Contains one double value and one string label.
//         Uncertainty has been removed.
//
///////////////////////////////////////////////////////////////////////////////

#include <float.h>
#include <stdlib.h>         // atoi

#include "tdat/PropValue.H"


// ---------- Constructors ------------

// ---------- Private Default Constructor (that only PropFactory can use)
///////////////////////////////////////////////////////////////////////////////
//
// Initialize value to have the value MAXDOUBLE to indicate null value
//
///////////////////////////////////////////////////////////////////////////////
PropValue::PropValue(const string& type,
                     const string& units) : TProperty(type, units)
{
  p_value = DBL_MAX; 
  p_classType = TProperty::PROPVALUE;
  // empty p_label is created from default string constructor

}

// ---------- Public Copy Constructor ------------
PropValue::PropValue(const PropValue& propValue) : TProperty(propValue)
{
  p_value = propValue.p_value;
  p_label = propValue.p_label;
}
 

// ---------- Virtual Destructor ------------
PropValue::~PropValue(void)
{
}


// ---------- Operators ----------
// defined in parent class


// ------------ Accessors ------------

double PropValue::value(void) const
{ return p_value; }

string PropValue::label(void) const
{ return p_label; }

bool PropValue::canScalarize(void) const
{ return true; }

double PropValue::scalarize(void) const
{ return p_value; }

// ------------ Modifiers ------------
void PropValue::value(const double& newValue)
{
  p_value = newValue;
}

void PropValue::label(const string& newLabel)
{ 
  p_label = newLabel;
}

bool PropValue::update(const string& msg)
{
  p_value = atoi(msg.c_str());
  return true;
}
