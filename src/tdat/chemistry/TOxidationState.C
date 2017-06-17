///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TOxidationState.C
//
//
// DESIGN:
//   TOxidationState's are designed to contain information about an element
//   that is unique to the charge on the atom.
//
//   This class replaces the old objecstore implementation.  This info
//   is really not used in ecce at this time except for the neutral state
//   electron configuration is displayed in a gui.  Eventually the user
//   should be able to specify vwr for different charges.  User
//   preferences would have to be supported per charge too.  I don't know
//   what else this class might be used for.  Its really a data structure
//   more than anything.
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
  using std::ostream;
  using std::endl;

#include "tdat/TOxidationState.H"

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
TOxidationState::TOxidationState(const string& symbol, float charge)
{
   p_symbol = symbol;
   p_charge = charge;
}

TOxidationState::TOxidationState(const TOxidationState& oxidationState)
{
   p_symbol = oxidationState.symbol();
   p_charge = oxidationState.charge();
   p_vwr = oxidationState.vwr();
   p_electronConfig = oxidationState.electronConfiguration();
}

 
// ---------- Virtual Destructor ------------
TOxidationState::~TOxidationState(void)
{
}


// ---------- Operators ----------
TOxidationState& TOxidationState::operator=(const TOxidationState& oxidationState)
{
   if (this != &oxidationState) {
     p_vwr = oxidationState.vwr();
     p_electronConfig = oxidationState.electronConfiguration();
     p_symbol = oxidationState.symbol();
     p_charge = oxidationState.charge();
   }
   return *this;
}

bool TOxidationState::operator==(const TOxidationState& oxidationState) const
{
  return (this == &oxidationState);
}

bool TOxidationState::operator!=(const TOxidationState& oxidationState) const
{
  return (this != &oxidationState);
}


// ------------ Modifiers ------------
void TOxidationState::vwr(float value) 
{ p_vwr = value; }
void TOxidationState::electronConfiguration(const string&  value) 
{ p_electronConfig = value; }

// ------------ Accessors ------------
string TOxidationState::symbol(void) const
{ return p_symbol; }

float TOxidationState::charge(void) const
{ return p_charge; }

float TOxidationState::vwr(void) const
{ return p_vwr; }

string TOxidationState::electronConfiguration(void) const
{ return p_electronConfig; }

// ---------- General Methods  ----------



// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Derived Class Accessors ----------



// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
ostream& operator<<(ostream& os, const TOxidationState& oxidationState)
{
  os << "[TOxidationState<";
  os << "symbol: " << oxidationState.symbol() << ",";
  os << "charge: " << oxidationState.charge() << ",";
  os << "vwr: " << oxidationState.vwr() << ",";
  os << "electronconfig: " << oxidationState.electronConfiguration();
  os << ">]" << endl;
  return os;
}

// ---------- Encapsulated Behavior ----------

