///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TProperty.C
//
//
// DESIGN:
//  TProperty is an abstract class and therefore contains only information
//  common to all properties.  Since values are stored differently for
//  different properties (i.e., VECTOR, VALUE, TABLE, VECTABLE, VECSTRING),
//  value accessors and modifier functions are derived.
//
///////////////////////////////////////////////////////////////////////////////

// system includes
   #include <ctype.h>   // for tolower

// general includes
// library includes

// application includes
   #include "util/ErrMsg.H"
   #include "tdat/TProperty.H"


// -----------------------
// Public Member Functions
// -----------------------
 
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//    Destroy a Single Property Object
//    Caller (subclass) is Responsible for Destroying value() Memory
//
///////////////////////////////////////////////////////////////////////////////
TProperty::~TProperty(void)
{ }


// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//    Assignment Operator
//    Warning - The Value Attribute is Shared Between Property Objects!
//
///////////////////////////////////////////////////////////////////////////////
TProperty& TProperty::operator=(const TProperty& property)
{
  p_type = property.type();
  p_units = property.units();
  p_classType = property.classType();
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//    Property Address Comparison Operators
//
///////////////////////////////////////////////////////////////////////////////
bool TProperty::operator==(const TProperty& property) const
{
  return (this == &property);
}

bool TProperty::operator!=(const TProperty& property) const
{
  return (this != &property);
}


// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//    Access Attributes of this Property Object
//
///////////////////////////////////////////////////////////////////////////////
string TProperty::type(void) const
{
  return p_type;
}


string TProperty::units(void) const
{
  return p_units;
}

TProperty::ClassType TProperty::classType(void) const
{
  return p_classType;
}

double TProperty::scalarize(void) const
{ 
  // Scalarize is invalid except for PropValue, PropTSVector,
  // and PropVectors with a rowLabel == "Coordinate"
  return 0.0; 
}

bool TProperty::canScalarize(void) const
{ 
  // Can only scalarize PropValue, PropTSVector, and some PropVectors
  return false; 
}


// -----------------------
// Static Member Functions
// -----------------------
///////////////////////////////////////////////////////////
string TProperty::classTypeToStr (TProperty::ClassType classType)
{
  string classString;

  switch (classType)
  {
    case TProperty::PROPVALUE       : classString = "propvalue";       break;
    case TProperty::PROPSTRING      : classString = "propstring";      break;
    case TProperty::PROPVECTOR      : classString = "propvector";      break;
    case TProperty::PROPVECSTRING   : classString = "propvecstring";   break;
    case TProperty::PROPTABLE       : classString = "proptable";       break;
    case TProperty::PROPVECTABLE    : classString = "propvectable";    break;
    case TProperty::PROPVECGRID     : classString = "propvecgrid";     break;
    case TProperty::PROPTSVECTABLE  : classString = "proptsvectable";  break;
    case TProperty::PROPTSTABLE     : classString = "proptstable";     break;
    case TProperty::PROPTSVECSTRING : classString = "proptsvecstring"; break;
    case TProperty::PROPTSVECVECTOR : classString = "proptsvecvector"; break;
    case TProperty::PROPTSVECTOR    : classString = "proptsvector";    break;
    case TProperty::SUPER           : classString = "super";
  };
  return classString;

}

///////////////////////////////////////////////////////////
TProperty::ClassType TProperty::strToClassType (const string& str)
{
  string classString = "";
  TProperty::ClassType classType;

  for (int i = 0; i < str.size(); i++)
    classString += tolower(str[i]); 

  if (classString == "propvalue")
     classType = TProperty::PROPVALUE;
  else if (classString == "propstring")
     classType = TProperty::PROPSTRING;
  else if (classString == "propvector")
     classType = TProperty::PROPVECTOR;
  else if (classString == "propvecstring")
     classType = TProperty::PROPVECSTRING;
  else if (classString == "proptable")
     classType = TProperty::PROPTABLE;
  else if (classString == "propvectable")
     classType = TProperty::PROPVECTABLE;
  else if (classString == "propvecgrid")
     classType = TProperty::PROPVECGRID;
  else if (classString == "proptsvectable")
     classType = TProperty::PROPTSVECTABLE;
  else if (classString == "proptsvecstring")
     classType = TProperty::PROPTSVECSTRING;
  else if (classString == "proptsvecvector")
     classType = TProperty::PROPTSVECVECTOR;
  else if (classString == "proptsvector")
     classType = TProperty::PROPTSVECTOR;
  else if (classString == "proptstable")
     classType = TProperty::PROPTSTABLE;
  else if (classString == "super")
     classType = TProperty::SUPER;
  else
    EE_RT_ASSERT(false, EE_FATAL, "invalid ClassType");

  return classType;
}

// -----------------------
// Protected Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//    Construct a New TProperty Object
//    
//
// Preconditions:
//    The type, untits, and classType are all VALID values.
//    Use the PropFactory class to make your TProperty, ensuring validity.
//
// Post:
//    TProperty is valid.
///////////////////////////////////////////////////////////////////////////////
TProperty::TProperty(const string& type, const string& units)
{
  p_type = type;
  p_units = units;
  p_classType = TProperty::SUPER;
}

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//    Copy Construct a New TProperty Object
//
///////////////////////////////////////////////////////////////////////////////
TProperty::TProperty(const TProperty& property)
{ (*this) = property; }

// ------------ Modifiers ------------
///////////////////////////////////////////////////////////////////////////////
//
// Description
//    Change _type and _units values.  Since no error checking is performed
//    here, these data members may be changed to an invalid value.
//
// Precondition:
//    type and value are VALID
//
// Implementation
//    The ONLY class that should use this behavior is the AnyProp
//    class.  Instead of making it a friend, however, we provide a cleaner
//    interface that is accessible to all subclasses.
//
///////////////////////////////////////////////////////////////////////////////
void TProperty::type(const string& type)
{
  p_type = type;
}


void TProperty::units(const string& units)
{ 
  p_units = units;
}


// ---------- Friends ----------
///////////////////////////////////////////////////////////////////////////////
//
// Description
//    Send Formatted String to Specified Stream
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const TProperty& property)
{
  os << "[Property<";
  os << "type: " << property.type() << ",";
  os << "units: " << property.units();
  os  << ">]" << endl;
  return os;
}


