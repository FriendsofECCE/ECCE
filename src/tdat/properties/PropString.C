///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropString.C
//
//
// DESIGN:
//         Contains one string value and one string label.
//         Uncertainty has been removed.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>

#include "tdat/PropString.H"


// ---------- Constructors ------------

// ---------- Private Default Constructor (that only PropFactory can use)
PropString::PropString(const string& type,
                     const string& units) : TProperty(type, units)
{
  p_value = "";
  p_classType = TProperty::PROPSTRING;
  // empty p_label is created from default string constructor

}

// ---------- Public Copy Constructor ------------
PropString::PropString(const PropString& propValue) : TProperty(propValue)
{
  p_value = propValue.p_value;
  p_label = propValue.p_label;
}
 

// ---------- Virtual Destructor ------------
PropString::~PropString(void)
{
}


// ---------- Operators ----------
// defined in parent class


// ------------ Accessors ------------

string PropString::value(void) const
{ return p_value; }

string PropString::label(void) const
{ return p_label; }

// ------------ Modifiers ------------
void PropString::value(const string& newValue)
{
  p_value = newValue;
}

void PropString::label(const string& newLabel)
{ 
  p_label = newLabel;
}

bool PropString::update(const string& msg)
{
  p_value = atoi(msg.c_str());
  return true;
}
