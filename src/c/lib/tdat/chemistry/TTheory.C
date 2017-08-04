///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Theory.C
//
//
// DESIGN:
//   A theory is a string with no enforcement on the range of values any
//   longer..
//
//
///////////////////////////////////////////////////////////////////////////////

// application includes
#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "tdat/TTheory.H"


///////////////////////////////////////////////////////////////////////////////
//  Constructors.
///////////////////////////////////////////////////////////////////////////////
TTheory::TTheory(const char *cat, const char *name, const bool& needsBasis)
{
  validate_and_assign(cat,name,needsBasis);
}

TTheory::TTheory(const string& cat, const string& name, const bool& needsBasis)
{
  validate_and_assign(cat,name,needsBasis);
}

TTheory::TTheory(const TTheory& rhs)
{
  validate_and_assign(rhs.category(),rhs.name(),rhs.needsBasis());
}


 
///////////////////////////////////////////////////////////////////////////////
//
//	Required Public Virtual Destructor
//
///////////////////////////////////////////////////////////////////////////////
TTheory::~TTheory(void)
{ 
}


// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//    Assignment.  Same behavior as copy contructor.
//
//  Implementation
//    We changed this assignment to remove validation - because we'll
//    assume the constructor and modifiers do this work already.
//
///////////////////////////////////////////////////////////////////////////////
TTheory& TTheory::operator=(const TTheory& rhs)
{
  if (this != &rhs) {
    p_category = rhs.category();
    p_name = rhs.name();
    p_needsBasis = rhs.needsBasis();
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//	Returns TRUE if categories and theory names of both instances
//	are equal.
//
//  Implementation
//  	The stuff about compiler deficiency is that the don't support
//	comparisons like rsh.name() == name && ... since this requires
//	a temporary variable with a destructor.  This is true for
//	both && and ||.
//
///////////////////////////////////////////////////////////////////////////////
bool TTheory::operator==(const TTheory& rhs) const
{
  if (p_category == rhs.p_category && p_name == rhs.p_name)
    return true;
  else
    return false;
}

///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//	Returns TRUE if either category or theory names of both instances
//	are not equal.
//
///////////////////////////////////////////////////////////////////////////////
bool TTheory::operator!=(const TTheory& rhs) const
{
  return !(*this == rhs);
}

// ------------ Modifiers ------------
///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//	Sets the category.  If it's not valid in conjunction with the
//	existing theory name, EE_INVALID_THEORY is placed on the
//	error queue and the instance state is not changed.
//
///////////////////////////////////////////////////////////////////////////////
void TTheory::category(const string& rhs_cat)
{
  validate_and_assign(rhs_cat,name(),needsBasis());
}

///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//	Sets the theory name.  If it's not valid in conjunction with the
//	existing category, EE_INVALID_THEORY is placed on the
//	error queue and the instance state is not changed.
//
///////////////////////////////////////////////////////////////////////////////
void TTheory::name(const string& rhs_name)
{
  validate_and_assign(category(),rhs_name,needsBasis());
}

void TTheory::needsBasis(const bool& rhs_needsBasis)
{
  validate_and_assign(category(),name(),rhs_needsBasis);
}

///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//	Re-defines the instance to the new category/name values.
//	If the new values are not valid, EE_INVALID_THEORY is placed
//	on the queue and the state is not changed.
//
///////////////////////////////////////////////////////////////////////////////
void TTheory::reset(const string& cat, const string& name,
                    const bool& needsBasis)
{
  validate_and_assign(cat,name,needsBasis);
}

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//	Simple accessors to return category and theory names.
//
///////////////////////////////////////////////////////////////////////////////
string 
TTheory::category() const 
{ 
  return p_category; 
}

string 
TTheory::name() const 
{ 
  return p_name; 
}

bool 
TTheory::needsBasis() const 
{ 
  return p_needsBasis; 
}

/**
 * Return concise representation of theory beautified for user view.
 */
string TTheory::toConciseString() const
{
  string ret;
  if (p_name == "None" || p_name.empty()) {

    ret = p_category;
  } else {
    ret = p_name;;
  }
  return ret;
}

TTheory::TTheory()
{
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//	Assigns new category and name to the current instance only
//	if the values are valid.  No changes are made unless the
//	category is valid and the name is valid for the category.
//	EE_INVALID_THEORY is placed on the error queue.
//
///////////////////////////////////////////////////////////////////////////////
bool TTheory::validate_and_assign(const string& cat, const string& name,
                                  const bool& needsBasis)
{
  p_category = cat;
  p_name = name;
  p_needsBasis = needsBasis;
  return true;
}


// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//	Send the theory category and name to the specified stream.
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os,const TTheory& theory)
{
  os << "[TTheory<" << theory.category() << "." << theory.name() << "]>"<< endl;
  return os;
}

