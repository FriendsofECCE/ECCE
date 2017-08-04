///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TRefElement.C
//
//
// DESIGN:
//    TRefElement objects are intended to be created only during the
//    loading of the Periodic Table database (this is why the class
//    PeriodicTable is a friend).  End-users and other developers should
//    never need the ability to create or destroy these objects.
//    If fact they are no longer used in the periodic table interface.
//
///////////////////////////////////////////////////////////////////////////////

// system includes
// general includes

// application includes
#include "tdat/TRefElement.H"
#include "util/Color.H"
#include "util/ErrMsg.H"



// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//  Description
//    Virtual RefElement Object Destructor
//
///////////////////////////////////////////////////////////////////////////////
TRefElement::~TRefElement(void)
{
}

// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  Description
//    Identity Based Comparison Operators
//
///////////////////////////////////////////////////////////////////////////////
bool TRefElement::operator==(const TRefElement& refElement) const
{ return this == &refElement; }
bool TRefElement::operator!=(const TRefElement& refElement) const
{ return this != &refElement; }


// ------------ Modifiers ------------

// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Accessors for Information Contained in Each TRefElement
//
///////////////////////////////////////////////////////////////////////////////
string TRefElement::atomicSymbol(void) const
{ return p_atomicSymbol.c_str(); }
int TRefElement::atomicNumber(void) const
{ return p_atomicNumber; }
int TRefElement::row(void) const
{ return p_row; }
int TRefElement::column(void) const
{ return p_column; }
TPerTab::ElementState TRefElement::state(void) const
{ return p_state; }
TPerTab::ElementMetal TRefElement::metal(void) const
{ return p_metal; }
const Color& TRefElement::color(void) const
{ return p_color; }
string TRefElement::name(void) const
{ return p_name.c_str(); }
string TRefElement::electronAffinities(void) const
{ return p_electronAffinities.c_str(); }
float TRefElement::atomicWeight(void) const
{ return p_atomicWeight; }
float TRefElement::density(void) const
{ return p_density; }
float TRefElement::meltingPoint(void) const
{ return p_meltingPoint; }
float TRefElement::boilingPoint(void) const
{ return p_boilingPoint; }
float TRefElement::covalentRadius(void) const
{ return p_covalentRadius; }
float TRefElement::crTolerance(void) const
{ return p_crTolerance; }

// ---------- General Methods  ----------


// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Hidden Constructor/Destructor ---
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Hidden Default Constructor and Virtual Destructor
//
//  Implementation
//    We hide the default constructor to ensure the compiler
//    doesn't create one that everyone can use.
//
///////////////////////////////////////////////////////////////////////////////
TRefElement::TRefElement(void)
{}

// ---------- Derived Class Modifiers ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Modifiers for Information Contained in Each RefElement
//    Assume that we are within a transaction for each accessor
//
///////////////////////////////////////////////////////////////////////////////
void TRefElement::atomicSymbol(const string& newAtomicSymbol)
{ p_atomicSymbol = newAtomicSymbol; }
void TRefElement::atomicNumber(int newAtomicNumber)
{ p_atomicNumber = newAtomicNumber; }
void TRefElement::row(int newRow)
{ p_row = newRow; }
void TRefElement::column(int newColumn)
{ p_column = newColumn; }
void TRefElement::state(TPerTab::ElementState newState)
{ p_state = newState; }
void TRefElement::metal(TPerTab::ElementMetal newMetal)
{ p_metal = newMetal; }
void TRefElement::color(const Color& newColor)
{ p_color = newColor; }
void TRefElement::name(const string& newName)
{ p_name = newName; }
void TRefElement::electronAffinities(const string& newElectronAffinities)
{ p_electronAffinities = newElectronAffinities; }
void TRefElement::atomicWeight(float newAtomicWeight)
{
  p_atomicWeight = newAtomicWeight;
}
void TRefElement::density(float newDensity)
{
  EE_ASSERT(newDensity >= 0.0, EE_FATAL,
  "Atomic Density Less than zero");
  p_density = newDensity;
}
void TRefElement::meltingPoint(float newMeltingPoint)
{
  EE_ASSERT(newMeltingPoint >= 0.0, EE_FATAL,
  "Melting Point Less than zero");
  p_meltingPoint = newMeltingPoint;
}
void TRefElement::boilingPoint(float newBoilingPoint)
{
  EE_ASSERT(newBoilingPoint >= 0.0, EE_FATAL,
  "Boiling Point Less than zero");
  p_boilingPoint = newBoilingPoint;
}
void TRefElement::covalentRadius(float value)
{ p_covalentRadius = value; }
void TRefElement::crTolerance(float value)
{ p_crTolerance = value; }
// ------------------------
// Private Member Functions
// ------------------------

// ---------- Friends ----------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Print a Formatted TRefElement Object to the Specified Stream
//
//  Implementation
//    We use public accesors but kept our friendship, just in case.
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const TRefElement& refElement)
{
  os << "[TRefElement<";
  os << "symbol: " << refElement.atomicSymbol() << ",";
  os << "number: " << refElement.atomicNumber() << ",";
  os << "row: " << (int) refElement.row() << ",";
  os << "column: " << (int) refElement.column() << ",";
  os << "state: " << refElement.state() << ",";
  os << "metal: " << refElement.metal() << ",";
  os << "color: " << refElement.color() << ",";
  os << "name: " << refElement.name() << ",";
  os << "electronAffinities: " << refElement.electronAffinities() << ",";
  os << "Atomic weight: " << refElement.atomicWeight() << ",";
  os << "density: " << refElement.density() << ",";
  os << "melting_point: " << refElement.meltingPoint() << ",";
  os << "boiling_point: " << refElement.boilingPoint() << ",";
  os << "covalent radius: " << refElement.covalentRadius() << ",";
  os << "cr tolerance: " << refElement.crTolerance() << ",";
  os << ">]" << endl;
  return os;
}

// ---------- Encapsulated Behavior ----------
