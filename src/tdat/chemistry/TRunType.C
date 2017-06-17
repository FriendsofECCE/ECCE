///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TRunType.C
//
//
// DESIGN:
//	Object to keep track of valid runtype instances.  Runtype
//	is a classification of job types such as enery calculation
//	or geometry optimization.  The set of runtypes is somewhat 
//	arbitrary but was defined to be as general and applicable to
//	most codes as possible.
//
//
///////////////////////////////////////////////////////////////////////////////

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "tdat/TRunType.H"

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//	Constructs a runtype with "name".  If "name" is not valid,
//	EE_INVALID_RUNTYPE is put on the queue and the runtype is
//	undefined.
//
///////////////////////////////////////////////////////////////////////////////
TRunType::TRunType(const char *name, const bool& noSpherical)
{
  //enforce_initialization();
  validate_and_assign(name, noSpherical);
}
TRunType::TRunType(const string& name, const bool& noSpherical)
{
  //enforce_initialization();
  validate_and_assign(name, noSpherical);
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//	Copy constructor.  See standard constructor description.
//
///////////////////////////////////////////////////////////////////////////////
TRunType::TRunType(const TRunType& rhs)
{
  //enforce_initialization();
  validate_and_assign(rhs.name(), rhs.noSpherical());
}

// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//	Public Virtual Destructor is Required
//
///////////////////////////////////////////////////////////////////////////////
TRunType::~TRunType(void)
{ }




// ------------ Operators ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Standard operators.
//
//  Implementation
//    We changed this assignment to remove validation - because we'll
//    assume the constructor and modifiers do this work already.
//
///////////////////////////////////////////////////////////////////////////////
TRunType& TRunType::operator=(const TRunType& rhs)
{
  p_name = rhs.name();
  p_noSpherical = rhs.noSpherical();
  return *this;
}

bool TRunType::operator==(const TRunType& rhs) const
{ return (name()==rhs.name() && noSpherical()==rhs.noSpherical()); }

bool TRunType::operator!=(const TRunType& rhs) const
{ return !(*this == rhs); }

// ------------ Modifiers ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//	Sets the runtype name.
//
///////////////////////////////////////////////////////////////////////////////
void TRunType::name(const string& new_name)
{
  p_name = new_name;
}

void TRunType::noSpherical(const bool& new_noSpherical)
{
  p_noSpherical = new_noSpherical;
}

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//	Returns the runtype name.
//
///////////////////////////////////////////////////////////////////////////////
string TRunType::name() const
{
  return p_name;
}

bool TRunType::noSpherical() const
{
  return p_noSpherical;
}



// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Derived Class Accessors ----------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//	This is Hidden ensure that only constructors that can create
//	valid instances are available.
//
///////////////////////////////////////////////////////////////////////////////
TRunType::TRunType(void)
{
}


///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Just does assignment now that runtypes are unvalidated.
//
///////////////////////////////////////////////////////////////////////////////
bool TRunType::validate_and_assign(const string& name, const bool& noSpherical)
{
  bool ret=true;
  p_name = name;
  p_noSpherical = noSpherical;
  return ret;
}

// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os,const TRunType& runtype)
{
  os << "[" << runtype.name() << "]" << endl;
  return os;
}

