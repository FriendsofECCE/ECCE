///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Bond.C
//
//
// DESIGN:
//    Bonds are a simple container class that pairs references to two
//    Atm objects and attaches a floating point value as the bond order.
//
//    This class doesn't handle any persistence.  It is assumed that
//    the user of this class is in a transaction when necessary and that
//    the Bond is created persistently if the Atm are persistent.
//
///////////////////////////////////////////////////////////////////////////////

// system includes
#include <math.h>
#include <limits.h>

// application includes
#include "util/ErrMsg.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Create a Bond from Two Atm Objects and a Bond Order
//
//  Implementation
//    We should validate persistence.
//    Because the user may have given us a bad order, we set the order
//    to Single by default in case the order modifier doesn't set the
//    correct value.
//
///////////////////////////////////////////////////////////////////////////////
TBond::TBond(const TAtm* atom1, const TAtm* atom2, double bondOrder) 
        : p_atom1(0), p_atom2(0), p_order(TBond::Single)
{
  EE_RT_ASSERT((atom1 != atom2),
               EE_FATAL, "Bonding atom to self");
  EE_RT_ASSERT((atom1 != (TAtm*)0) && (atom2 != (TAtm*)0),
               EE_FATAL, "Illegal Atoms Passed to Bond Constructor");
  this->atom1(atom1);
  this->atom2(atom2);

  // we need to assign the display style to something - a parent atom is
  // a safe bet
  displayStyle(atom1->displayStyle());
  order(bondOrder);
  index(-1);

}

TBond::TBond(const TAtm* atom1, const TAtm* atom2, DisplayStyle display_style, double bondOrder)
        : p_atom1(0), p_atom2(0), p_order(TBond::Single)
{
  EE_RT_ASSERT((atom1 != atom2),
               EE_FATAL, "Bonding atom to self");
  EE_RT_ASSERT((atom1 != (TAtm*)0) && (atom2 != (TAtm*)0),
               EE_FATAL, "Illegal Atoms Passed to Bond Constructor");
  this->atom1(atom1);
  this->atom2(atom2);
  order(bondOrder);
  index(-1);

  displayStyle( display_style );

}

TBond::TBond(const TBond& bond) 
        : p_atom1(0), p_atom2(0), p_order(TBond::Single)
{ (*this) = bond; }

 
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//
///////////////////////////////////////////////////////////////////////////////
TBond::~TBond(void)
{
  TAtm* atom = atom1();
  if (atom != (TAtm*)0) {
    atom->removeBond(this);
  }
  atom = atom2();
  if (atom != (TAtm*)0) {
    atom->removeBond(this);
  }
}


// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Value Copy Assignment Operator.
//
///////////////////////////////////////////////////////////////////////////////
TBond& TBond::operator=(const TBond& bond)
{
  atom1(bond.atom1());
  atom2(bond.atom2());
  order(bond.order());
  index(bond.index());
  displayStyle(bond.displayStyle());
  return *this;
}
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Value Less Than Operator.
//
///////////////////////////////////////////////////////////////////////////////
bool TBond::operator<(const TBond& bond) const
{
  int aa1 = bond.atom1()->index();
  int aa2 = bond.atom2()->index();
  int bb1 = this->atom1()->index();
  int bb2 = this->atom2()->index();
  int a1, a2, b1, b2;
  if (aa1 < aa2 ) {
    a1 = aa1;
    a2 = aa2;
  } else {
    a1 = aa2;
    a2 = aa1;
  }
  if (bb1 < bb2 ) {
    b1 = bb1;
    b2 = bb2;
  } else {
    b1 = bb2;
    b2 = bb1;
  }
  if (a1 != b1) {
    return (a1 < b1);
  } else {
    return (a2 < b2);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Value-Based Equality.
//    Equality is Based on On the Component Atoms; order independent!
//
///////////////////////////////////////////////////////////////////////////////
bool TBond::operator==(const TBond& bond) const
{ return ((atom1() == bond.atom1()) && (atom2() == bond.atom2())); }
bool TBond::operator!=(const TBond& bond) const
{ return !((*this) == bond); }
unsigned long TBond::hFun(const TBond& a)
{ return (unsigned long)&a; }

// ------------ Modifiers ------------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Modify the Bond Order of Specified Bond.
//
///////////////////////////////////////////////////////////////////////////////
void TBond::order(const double& bondOrder)
{
  if (bondOrder <= 0) {
    ErrMsg().message("EE_RANGE", EE_WARNING, WHERE,0,INT_MAX,(int)bondOrder);
  } else {
    p_order = bondOrder;
  }
}
void TBond::index(const int& value)
{ p_index = value; }

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Simple Accessors.
//
///////////////////////////////////////////////////////////////////////////////
TAtm* TBond::atom1(void) const
{ return p_atom1; }
TAtm* TBond::atom2(void) const
{ return p_atom2; }
double TBond::order(void) const
{ return p_order; }

DisplayStyle TBond::displayStyle() const
{ 
   return p_displayStyle; 
}


int TBond::index(void) const
{ return p_index; }

static const char *order_strs[] = {
    "Single",
    "Aromatic",
    "Double",
    "Triple",
    "Hydrogen",
    "Metal-Pi",
    "Unknown"
};
static int num_order = sizeof(order_strs)/sizeof(char*);
///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//   Return a string name for the bond order.  Bond orders are typically
//   viewed as one of the predefined values but they can be set to any
//   arbitrary number.  If the number isn't one of the predefined types,
//   and empty string is returned.
///////////////////////////////////////////////////////////////////////////////
const char* TBond::orderToString(double order)
{
  const char* ret = order_strs[num_order];;
  if (order == TBond::Single) {
    ret = order_strs[0];
  } else if (order == TBond::Aromatic) {
    ret = order_strs[1];
  } else if (order == TBond::Double) {
    ret = order_strs[2];
  } else if (order == TBond::Triple) {
    ret = order_strs[3];
  } else if (order == TBond::Hydrogen) {
    ret = order_strs[4];
  } else if (order == TBond::Metal) {
    ret = order_strs[5];
  } else {
    ret = order_strs[6];
  }
  return ret;
}

double TBond::strToOrder(const string& str)
{
  double ret;
  if (str == "Single") {
    ret = TBond::Single;
  } else if (str == "Aromatic") {
    ret = TBond::Aromatic;
  } else if (str == "Double") {
    ret = TBond::Double;
  } else if (str == "Triple") {
    ret = TBond::Triple;
  } else if (str == "Metal-Pi") {
    ret = TBond::Metal;
  } else if (str == "Hydrogen") {
    ret = TBond::Hydrogen;
  } else {
    ret = 0;
  }
  return ret;
}


string TBond::orderToSmallImage(double order)
{
  string pixmap;
  
  if (order == TBond::Single) {
    pixmap = "bond_single.xpm";
  } else if (order == TBond::Double) {
    pixmap = "bond_double.xpm";
  } else if (order == TBond::Triple) {
    pixmap = "bond_triple.xpm";
  } else if (order == TBond::Aromatic) {
    pixmap = "bond_aromatic.xpm";
  } else if (order == TBond::Metal) {
    pixmap = "bond_metal.xpm";
  }

  return pixmap;
}


string TBond::orderToImage(double order)
{
  string pixmap;
  
  if (order == TBond::Single) {
    pixmap = "bond_single_large.xpm";
  } else if (order == TBond::Double) {
    pixmap = "bond_double_large.xpm";
  } else if (order == TBond::Triple) {
    pixmap = "bond_triple_large.xpm";
  } else if (order == TBond::Aromatic) {
    pixmap = "bond_aromatic_large.xpm";
  } else if (order == TBond::Metal) {
    pixmap = "bond_metal_large.xpm";
  }

  return pixmap;
}


// ------------ General ------------
double TBond::length(void) const
{
  const double* atom1Coord = atom1()->coordinates();
  const double* atom2Coord = atom2()->coordinates();
  double dx = atom2Coord[0] - atom1Coord[0];
  double dy = atom2Coord[1] - atom1Coord[1];
  double dz = atom2Coord[2] - atom1Coord[2];
  return sqrt(dx*dx + dy*dy + dz*dz);
}

bool TBond::contains(const TAtm& atom) const
{ return ((atom1() == &atom) || (atom2() == &atom)); }



// --------------------------
// Protected Member Functions
// --------------------------
//  ---------- Protected Modifiers ----------	
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Simple Modifiers.
//
///////////////////////////////////////////////////////////////////////////////
void TBond::atom1(const TAtm* atom1)
{
  TAtm *oldAtom = p_atom1;
  if (oldAtom) oldAtom->removeBond(this);
  p_atom1 = (TAtm*)atom1;
  p_atom1->addBond(this);
}
void TBond::atom2(const TAtm* atom2)
{
  TAtm *oldAtom = p_atom2;
  if (oldAtom) oldAtom->removeBond(this);
  p_atom2 = (TAtm*)atom2;
  p_atom2->addBond(this);
}

void TBond::displayStyle(const DisplayStyle ds) 
{
  p_displayStyle = ds; 
  // Also set display flag if DNONE
}



// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
ostream& operator<<(ostream& os, const TBond& bond)
{
  os << "[Bond<";
  os << "atom1:FragIndex:" // << bond.atom1Index()
     << ":" << *(bond.atom1()) << ",";
  os << "atom2:FragIndex:" // << bond.atom2Index()
     << ":" << *(bond.atom2()) << ",";
  os << "order:" << bond.order();
  os << ">]" << endl;
  return os;
}

const double TBond::Single = 1.0;
const double TBond::Aromatic = 1.5;
const double TBond::Double = 2.0;
const double TBond::Triple = 3.0;
const double TBond::Hydrogen = 0.1;
const double TBond::Metal = 0.5;
