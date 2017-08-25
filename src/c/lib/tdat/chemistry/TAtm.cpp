///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TAtm.C
//
//
// DESIGN:
//   Atms are instances of elements that are meant to be included
//   into other chemical systems.  Reference information about the
//   element type of the atom is contained in the Periodic Table
//   reference database.  
//
///////////////////////////////////////////////////////////////////////////////

// system includes
#include <float.h>
#include <math.h>           // angle calculations
#include <stdio.h>
#include <algorithm>

using std::find;

// application includes
#include "tdat/Residue.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"
#include "tdat/TChartOfNuclides.H"
#include "tdat/TPerTab.H"
#include "tdat/TRefIsotope.H"

#include "util/ErrMsg.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"

#define  PI  3.14159265358979323846
#define  RAD * 180.0 / PI

// -----------------------
// Public Member Functions
// -----------------------


// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Create a null TAtm
//
//  Implementation
//    Initialize to a ghost atom
//
///////////////////////////////////////////////////////////////////////////////
TAtm::TAtm() 
      : p_displayStyle(DisplayStyle::BALLWIRE)
{
  TPerTab tpt;
  strcpy(p_atomName,"");
  strcpy(p_atomType,"    ");
  atomicSymbol("X");
  p_index = -1;
  p_assembly = 0;
  p_residue = (Residue*)0;

  p_polarizability = 0.;
  p_chargeGroup = 1;
  p_polarizationGroup = 0;
  p_connectionIndex = 0;
  bFactor(0);
  occupancy(0);
  p_displayLabel = "";
  p_suffixHack = "";

  atomicNumber(tpt.atomicNumber("X"));

  partialCharge(0.0);
  p_dihedralIndex = 0;
  p_shape = ShapeData::lone;
  assignDefaultCoordinates();
  setBehaviorType(TAtm::UNSPECIFIED);

  setMass(0.0);
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Up through version 1.2, this method created an TAtm from a Tag String.
//    However this is slow and it looks like we never made use of Tag
//    stuff so it was a waste of time.  I was a little afraid to just do
//    away with it so I created the unused constructor that takes a
//    tag.  The overhead for the builder was too much!.
//
//  Implementation
//    Use the tag class to transform the string into various components;
//
///////////////////////////////////////////////////////////////////////////////
TAtm::TAtm(const char *tagString, const double* coord, const string& suffix)
          : p_displayStyle(DisplayStyle::BALLWIRE)
{
  TPerTab tpt;
  p_atomicNumber = tpt.atomicNumber(tagString);
  p_index = -1;
  p_assembly = 0;
  p_residue = (Residue*)0;
  p_displayLabel = "";
  p_suffixHack = suffix;

  strcpy(p_atomName,"");
  strcpy(p_atomType,"    ");
  strcpy(p_atomicSymbol,tagString);
  p_partialCharge = 0.0;
  p_chargeGroup = 1;
  p_polarizationGroup = 0;
  p_connectionIndex = 0;
  p_shape = ShapeData::lone;

  p_Bfactor = 0;
  p_occupancy = 0;
  p_polarizability = 0.;
  p_dihedralIndex = 0;

  if (coord == (const double*)0) {
    assignDefaultCoordinates();
  } else {
    coordinates(coord);
  }

  setBehaviorType(TAtm::UNSPECIFIED);
  setMostAbundantIsotopeMass(p_atomicNumber);
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Create an TAtm from a Tag String
//
//  Implementation
//    Use the tag class to transform the string into various components;
//
///////////////////////////////////////////////////////////////////////////////
TAtm::TAtm(short atomicNumber, const double* coord, const string& suffix) 
      : p_displayStyle(DisplayStyle::BALLWIRE)
{
  TPerTab tpt;
  string tagString = tpt.atomicSymbol(atomicNumber);
  atomicSymbol(tagString);
  strcpy(p_atomName,"");
  p_atomicNumber = atomicNumber;
  p_index = -1;
  p_assembly = 0;
  p_residue = (Residue*)0;
  p_displayLabel = "";
  p_suffixHack = suffix;

  p_partialCharge = 0.0;
  p_chargeGroup = 1;
  p_polarizationGroup = 0;
  p_connectionIndex = 0;
  p_shape = ShapeData::lone;

  if (coord == (const double*)0)
    assignDefaultCoordinates();
  else
    coordinates(coord);

  strcpy(p_atomType,"");
  p_polarizability = 0.;
  p_Bfactor = 0;
  p_occupancy = 0;
  p_dihedralIndex = 0;

  setBehaviorType(TAtm::UNSPECIFIED);
  setMostAbundantIsotopeMass(p_atomicNumber);
}



/**
 * Copy constructor.
 */
   TAtm::TAtm(const TAtm& atom)
: p_displayStyle(DisplayStyle::BALLWIRE)
{ 
   *this = atom; 
}

 
/**
 * Destructor
 *  All associated TBonds are destroyed.
 */
TAtm::~TAtm()
{
   removeBonds();
}

// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Assign one TAtm from another TAtm
//
//  Implementation
//    Invoke superclass assignment, copy atom atom information.
//
///////////////////////////////////////////////////////////////////////////////
TAtm& TAtm::operator=(const TAtm& atom)
{
   if (this != &atom) {

      // Simple assignment stuff
      // I tried to reduce duplication by calling shallow copy plus 
      // other stuff.  Not sure of the following:
      //  Why is dihedralIndex not part of shallow copy
      shallowCopy(&atom);


      index(atom.index());  // not sure this is a valid thing to do
      dihedralIndex(atom.dihedralIndex());

      // residue info
      atomResidue(atom.getResidue());

   }
   return *this;
}



/**
 * Identity based comparison operator.
 */
bool TAtm::operator==(const TAtm& atom) const
{ return (this == &atom); }



/**
 * Identity based comparison operator.
 */
bool TAtm::operator!=(const TAtm& atom) const
{ 
   return (this != &atom); 
}



/**
 * Create an exact copy of this atom.
 */
TAtm *TAtm::clone() const
{
   return  new TAtm((*this));
}



/**
 * Converts a BehaviorType into a string.
 */
string TAtm::behaviorTypeToString(TAtm::BehaviorType type)
{
   string ret = ""; // same as UNSPECIFIED
   switch (type) {
      case UNSPECIFIED: ret = ""; break;
      case QUANTUM: ret = "Quantum"; break;
      case POINT: ret = "Point"; break;
      case BQ: ret = "Bq"; break;
      case BQX: ret = "BqX"; break;
      default:
        cerr << "Unknown behavior type " << type << endl;
   }
   return ret;
}



TAtm::BehaviorType TAtm::stringToBehaviorType(const string& type)
{
   TAtm::BehaviorType ret = UNSPECIFIED;
   string itype = type;
   STLUtil::toLower(itype);
   if (itype == "") {
      ret = UNSPECIFIED;
   } else if (itype == "quantum") {
      ret = QUANTUM;
   } else if (itype == "point") {
      ret = POINT;
   } else if (itype == "bq") {
      ret = BQ;
   } else if (itype == "bqx") {
      ret = BQX;
   } else {
      cerr << "Unknown behavior type " << type << endl;
   }
   return ret;
}



unsigned long TAtm::hFun(const TAtm& a)
{
   return (unsigned long)&a;
}

/**
 * Parse input string and try and come up with a residue name, PDB index, and
 * chain ID. If all else fails, come up with an absolute residue index
 * (corresponding to the internal index of the residue in the SGFragment).
 */
void TAtm::parseString(string input, string &name, int &atom_index,
                       string &parent_name, int &parent_pdb_idx,
                       char &parent_chain, int &parent_idx)
{
  name = "";
  atom_index = -1;
  parent_name = "";
  parent_pdb_idx = -1;
  parent_chain = ' ';
  parent_idx = -1;
  StringTokenizer tokenizer(input, ":");
  vector<string> tokens = tokenizer.tokenize(":");

  int nsize = tokens.size();
  int i,offset;
  for  (i=0; i<nsize; i++) {
    STLUtil::stripLeadingAndTrailingWhiteSpace(tokens[i]);
  }
  
  if (nsize > 0) {
    // If token is an integer then assume that it is an atom index, otherwise,
    // assume that it is an atom name. Subtract 1 to get internal atom index.
    if (!StringConverter::toInt(tokens[0], atom_index)) {
      name = tokens[0];
    } else {
      atom_index--;
    }
    // if second token is an integer then assume that it is an atom index
    // (assuming an atom index has not aready been found), otherwise
    // assume that it marks the beginning of a string describing a residue.
    if (nsize > 1) {
      if (atom_index < 0 && StringConverter::toInt(tokens[1], atom_index)) {
        offset = 2;
      } else {
        offset = 1;
      }
      if (nsize>offset) {
        string residue = tokens[offset];
        for (i=offset+1; i<nsize; i++) {
          if (tokens.size() > 0) {
            residue.append(":");
            residue.append(tokens[i]);
          }
        }
        Residue::parseString(residue, parent_name, parent_pdb_idx, parent_chain,
                             parent_idx);
      }
    }
  }
}

// ------------ Accessors -----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Get the Atomic Symbol of this TAtm
//
//  Implementation
//    Member access.
//    User does not need to be in a transaction.
//
///////////////////////////////////////////////////////////////////////////////
string TAtm::atomicSymbol(void) const
{ return p_atomicSymbol; }
int TAtm::atomicNumber(void) const
{ return p_atomicNumber; }
string TAtm::atomName(void) const
{ return p_atomName; }
string TAtm::atomType(void) const
{ return p_atomType; }
short TAtm::dihedralIndex(void) const
{ return p_dihedralIndex; }
string TAtm::displayLabel() const
{ return p_displayLabel; }
string TAtm::suffixHack() const
{ return p_suffixHack; }

double TAtm::getMass() const
{
   return p_mass;
}

TAtm::BehaviorType TAtm::getBehaviorType() const
{
   return p_behavior;
}

///////////////////////////////////////////////////////////////////////////////
// Description: Returns True if this atom has "Nub" as its atomicSymbol
//
// Param: none.
//
// Return: True if atomicSymbol is "Nub", False otherwise.
//
///////////////////////////////////////////////////////////////////////////////
bool TAtm::isNub(void) const
{
   return ( strcmp( p_atomicSymbol, "Nub") == 0);
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns a pointer to a dynamically allocated character array
//              containing the name of this atom's type. The name contains
//              no blanks.
//
// Param: none.
//
// Return: a pointer to a character array with the name of this atom's type.
//         The name contains no blanks.  If this atom's local copy of its type 
//         contains only blanks, an empty string is returned.
//
////////////////////////////////////////////////////////////////////////////////
char* TAtm::cAtomType(void) const
{
   int numBlanks = 0;

   // count number of blanks
   int len = strlen( p_atomType);
   int i;
   for ( i=0;i<len;i++)  {
      if ( p_atomType[i] == ' ')
         numBlanks++;
   }

   // copy non-blank characters into return string
   len -= numBlanks;
   char* ntype = new char[ len + 1];
   int j = 0;
   for ( i=0;i<len;i++)  {
      if ( p_atomType[i] != ' ')
         ntype[j++] = p_atomType[i];
   }
   ntype[j] = 0;

   return ntype;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns a pointer to a dynamically allocated character array
//              containing the name of this atom. The name contains
//              no blanks.
//
// Param: none.
//
// Return: a pointer to a character array with the name of this atom.
//         The name contains no blanks.  If this atom's local copy of its name 
//         contains only blanks, an empty string is returned.
//
////////////////////////////////////////////////////////////////////////////////
char* TAtm::cAtomName(void) const
{
   int numBlanks = 0;

   // count number of blanks
   int len = strlen( p_atomName);
   int i;
   for ( i=0;i<len;i++)  {
      if ( p_atomName[i] == ' ')
         numBlanks++;
   }

   // copy non-blanks from original string into new char array.
   int j = 0;
   char* name = new char[ len - numBlanks + 1];
   for ( i=0;i<len;i++)  {
      if ( p_atomName[i] != ' ')
         name[j++] = p_atomName[i];
   }
   name[j] = 0;

   return name;
}

Residue* TAtm::getResidue(void) const
{ return p_residue; }
string TAtm::segment(void) const
{ if (p_residue != (Residue*)0) {
    return p_residue->segID().c_str();
  } else {
    return "";
  }
}
string TAtm::residueName(void) const
{ if (p_residue != (Residue*)0) {
    return p_residue->name().c_str();
  } else {
    return "";
  }
}
int TAtm::index(void) const
{ return p_index; }
int TAtm::residueNumber(void) const
{ if (p_residue != (Residue*)0) {
    return p_residue->number();
  } else {
    return 0;
  }
}
char TAtm::chain(void) const
{ if (p_residue != (Residue*)0) {
    return p_residue->chain();
  } else {
    return ' ';
  }
}
char TAtm::icode(void) const
{ if (p_residue != (Residue*)0) {
    return p_residue->insertCode();
  } else {
    return ' ';
  }
}
float TAtm::occupancy(void) const
{ return p_occupancy; }
float TAtm::polarizability(void) const
{ return p_polarizability; }
float TAtm::bFactor(void) const
{ return p_Bfactor; }
DisplayStyle TAtm::displayStyle(void) const
{return p_displayStyle; }
short TAtm::assembly(void) const
{ return p_assembly; }
short TAtm::chargeGroup(void) const
{ return p_chargeGroup; }
short TAtm::polarizationGroup(void) const
{ return p_polarizationGroup; }
int TAtm::connectionIndex(void) const
{ return (int)p_connectionIndex; }

double TAtm::partialCharge(void) const
{ return (double)p_partialCharge; }
ShapeData::Shape TAtm::shape() const
{ return p_shape; }
string TAtm::shapeString(void) const
{ return ShapeData::shapeToString(p_shape); }

const double* TAtm::coordinates(void) const
{ return p_coords; }

void TAtm::copyResidueInfo(TAtm* sourceAtm)
{
    p_residue = sourceAtm->getResidue();
}

string TAtm::getHydrogenName(void)
{
//
//  Attempt to assign hydrogen atoms a name based on the name of the
//  parent atom
//
    vector<TAtm*>* parentlist = this->connectedAtoms();
    string ret="";
//
//  If hydrogen is connected to more than one atom, return empty string
//
    if (parentlist->size() != 1) return ret;
    TAtm* parent = (*parentlist)[0];
//
//  If parent has no name or parent is an atom with a two character symbol,
//  then return empty string
//
    string parentName = parent->atomName();
    string atsym = parent->atomicSymbol();
    if (parentName == "" || atsym.length() == 2)
      return ret;
    vector<TAtm*>* siblinglist = parent->connectedAtoms();
    int isize = siblinglist->size();
    TAtm* sibling;
    int i;
    int hcount=0;
//
//  scan through sibling atoms looking for other hydrogens
//
    for (i=0; i < isize; i++) {
      sibling = (*siblinglist)[i];
      if (sibling->atomicNumber() == 1) hcount++;
    } 
    if (hcount <= 1) {
      hcount = 2;
    } else {
      hcount++;
    }
//
//  If, for some bizarre reason, more than 9 hydrogens are already
//  attached, return empty string
//
    char name[5];
    if (hcount > 9) return ret;
    sprintf (name,"%1d",hcount);
    name[1] = 'H';

    if (parentName.length() > 0) {
      name[2] = parentName[2];
      name[3] = parentName[3];
      name[4] = '\0';
    }

    ret = name;
    return  ret;
}

void TAtm::cleanAtom(void)
{
//  Utility function that clears atom info from atom (atom name,
//  type, etc.) Useful if atom symbol is changed.

    strcpy(p_atomName,"");
    strcpy(p_atomType,"");
    p_polarizability = 0.;
    p_chargeGroup = 0;
    p_polarizationGroup = 0;
    p_connectionIndex = 0;
    bFactor(0);
    occupancy(0);
    partialCharge(0.0);
}
// Bond Information
const vector<TBond*>& TAtm::bondList(void) const
{ return p_bonds; }

vector<TAtm*>* TAtm::connectedAtoms(void) const
{
  vector<TAtm*>* result = new vector<TAtm*>;

  //Get bond list
  const  vector<TBond*>& bonds = bondList();
  int cnt = bonds.size();

  TBond *theBond = (TBond*)0;

  // check atom1 with this - if equal put atom2 into list
  // then check atom2 with this - if equal put atom1 into list
  // I need to make sure I'm not just doing pointer comparison
  for (int idx=0; idx<cnt; idx++) {
    theBond = bonds[idx];
    if (*(theBond->atom1()) == *this) {
      result->push_back(theBond->atom2());
    } else if (*(theBond->atom2()) == *this) {
      result->push_back(theBond->atom1());
    }
  }
  return result;
}

void TAtm::connectedAtoms(vector<TAtm*> & result, const char* atomSym) const
{
  bool checkSymbol = (atomSym != (char*)0);

  const  vector<TBond*>& bonds = bondList();
  int cnt = bonds.size();
  TBond *theBond = (TBond*)0;

  if (checkSymbol) {
    for (int idx=0; idx<cnt; idx++) {
      theBond = bonds[idx];
      if (*(theBond->atom1()) == *this) {
        if (theBond->atom2()->atomicSymbol() == atomSym)
          result.push_back(theBond->atom2());
      } else if (*(theBond->atom2()) == *this) {
        if (theBond->atom1()->atomicSymbol() == atomSym)
          result.push_back(theBond->atom1());
      }
    }
  } else {
    for (int idx=0; idx<cnt; idx++) {
      theBond = bonds[idx];
      if (*(theBond->atom1()) == *this) {
        result.push_back(theBond->atom2());
      } else if (*(theBond->atom2()) == *this) {
        result.push_back(theBond->atom1());
      }
    }
  }
}

// ------------ Modifiers -----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Set the data members for this atom
//
///////////////////////////////////////////////////////////////////////////////
void TAtm::atomicSymbol(const string&symbol)
{ 
  strcpy(p_atomicSymbol,symbol.c_str());
  setMostAbundantIsotopeMass(symbol);
}
void TAtm::atomName(const char *name)
{
  strncpy(p_atomName,name,4);
  p_atomName[4] = '\0';

// make sure that remaining spaces are blank

  for (int i=strlen(name); i<4; i++) {
    p_atomName[i] = ' ';
  }
}
void TAtm::atomType(const char *value)
{
  string svalue(value);
  STLUtil::stripLeadingAndTrailingWhiteSpace(svalue);
  strcpy(p_atomType,svalue.c_str());
  p_atomType[4] = '\0';

// make sure that remaining spaces are blank

  for (int i=strlen(svalue.c_str()); i<4; i++) {
    p_atomType[i] = ' ';
  }
}
void TAtm::index(const int& value)
{ p_index = value; }
void TAtm::atomicNumber(short atomicNumber)
{ 
  p_atomicNumber = atomicNumber;
  setMostAbundantIsotopeMass(p_atomicNumber);
}
void TAtm::displayLabel(const string& label)
{ p_displayLabel = label; }
void TAtm::suffixHack(const string& suffix)
{ p_suffixHack = suffix; }
void TAtm::atomResidue(Residue* residue)
{ p_residue = residue; }
void TAtm::partialCharge(const double& partialCharge)
{ p_partialCharge = partialCharge; }
void TAtm::occupancy(const float& value) 
{ p_occupancy = value; }
void TAtm::polarizability(const float& value) 
{ p_polarizability = value; }
void TAtm::bFactor(const float& value) 
{ p_Bfactor = value; }
void TAtm::shape(ShapeData::Shape value)
{ p_shape = value; }
void TAtm::assembly(short value)
{ p_assembly = value; }
void TAtm::chargeGroup(short cgNum)
{ p_chargeGroup = cgNum; }
void TAtm::polarizationGroup(short pgNum)
{ p_polarizationGroup = pgNum; }
void TAtm::connectionIndex(short cIndx)
{ p_connectionIndex = cIndx; }
void TAtm::connectionIndex(int cIndx)
{ p_connectionIndex = (short)cIndx; }
void TAtm::shapeString(const string& shapeStr)
{ p_shape = ShapeData::stringToShape(shapeStr); }

void TAtm::setMass(double mass)
{
   p_mass = mass;
}


void TAtm::setBehaviorType(TAtm::BehaviorType behavior)
{
   p_behavior = behavior;
}

void TAtm::setMostAbundantIsotopeMass(int atomicNumber)
{
  if (p_atomicNumber > 0) {
    TChartOfNuclides ichart;
    const TRefIsotope *isotope = ichart.mostAbundantIsotope(atomicNumber);
    if (isotope != (TRefIsotope*)0) {
      p_mass = isotope->isotopeWeight();
    } else {
      p_mass = 0.0;
    }
  } else {
    p_mass = 0.0;
  }
}

void TAtm::setMostAbundantIsotopeMass(string atomicSymbol)
{
  TPerTab pt;
  setMostAbundantIsotopeMass(pt.atomicNumber(atomicSymbol));
}

void TAtm::coordinates(const double* coords)
{ 
   p_coords[0] = coords[0]; 
   p_coords[1] = coords[1]; 
   p_coords[2] = coords[2]; 
}



void TAtm::assignDefaultCoordinates()
{ 
   p_coords[0] = p_coords[1] = p_coords[2] = DBL_MAX; 
}



void TAtm::coordinates( const double& x, const double& y, const double& z)
{ p_coords[0] = x; p_coords[1] = y; p_coords[2] = z; }
void TAtm::dihedralIndex(const short idx)
{ p_dihedralIndex = idx; }

void TAtm::displayStyle(const DisplayStyle& ds )
{ 
   p_displayStyle = ds; 
   //cout << "TAtm::displayStyle(): " << p_displayStyle << endl ;
} 
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//     Copy all non-pointer data from one atom to another.
////////////////////////////////////////////////////////////////////////////////
void TAtm::shallowCopy(const TAtm* atm)
{
   atomicSymbol(atm->atomicSymbol());
   atomicNumber(atm->atomicNumber());
   atomName(atm->atomName().c_str());
   partialCharge(atm->partialCharge());
   shapeString(atm->shapeString());
   coordinates(atm->coordinates());
   displayLabel(atm->displayLabel());
   suffixHack(atm->suffixHack());

   occupancy(atm->occupancy());
   bFactor(atm->bFactor());

   atomType(atm->atomType().c_str());
   polarizability(atm->polarizability());
   chargeGroup(atm->chargeGroup());
   polarizationGroup(atm->polarizationGroup());
   connectionIndex(atm->connectionIndex());
   shape(atm->shape());

   assembly(atm->assembly());
   displayStyle(atm->displayStyle());
   setMass(atm->getMass());
   setBehaviorType(atm->getBehaviorType());
}



// ---------- Bond-Atom Relationship Maintenance ----------
void TAtm::removeBond(const TBond* bond)
{ vector<TBond*>::iterator it = p_bonds.begin();
  it = find (p_bonds.begin(), p_bonds.end(), bond);
  p_bonds.erase(it);
}

void TAtm::removeBonds(void)
{
  for (int idx=p_bonds.size()-1; idx>=0; idx--) {
    delete p_bonds[idx];
  }
  p_bonds.clear();
}

void TAtm::addBond(const TBond* bond)
{ 
   ((TBond*)bond)->displayStyle(p_displayStyle);
   //cout << "TAtm::addBond style: " << p_displayStyle << endl ;
   p_bonds.push_back((TBond*)bond); 
}

// ---------- Builder Query Operations ----------

///////////////////////////////////////////////////////////////////////////////
// man
//
// Description
//    Find a Connection List of Atoms.
//    The visited Argument must Contain The First Atom.
//    On the First Call lastAtom and firstAtom Should be Equal and
//    Represents the Atom on the Other Side of the Bond Being Broken.
//
// Implementation
//    Standard recursion with the visited list marking our path.
//
///////////////////////////////////////////////////////////////////////////////
bool TAtm::traverse(vector<TAtm*>& visited,
	                 const TAtm* lastAtom,
	                 const TAtm* firstAtom) const
{
  bool foundCycle = false;

  int cnt = p_bonds.size();
  TBond *theBond = (TBond*)0;
  for (int idx=0; idx<cnt; idx++) {
    theBond = p_bonds[idx];
    const TAtm* nextAtom;
    if ((theBond->atom1()) == this) {
      nextAtom = theBond->atom2();
    } else { 
      nextAtom = theBond->atom1();
    }
    if (nextAtom != lastAtom) {
      if (nextAtom == firstAtom) {
        foundCycle = true;
        break;
      } else if (find(visited.begin(), visited.end(), (TAtm*)nextAtom) ==
                 visited.end()) {
        visited.push_back((TAtm*)nextAtom);
        if ((foundCycle = nextAtom->traverse(visited, this, firstAtom)))
          break;
      }
    }
  }

  return foundCycle;
}

// --------------------------
// Private Members
// --------------------------

// ---------- Encapsulated Behavior ----------

////////////////////////////////////////////////////////////////////////////////
//
// Description
//   Calculate the angle with two other atoms.
//
// Implementation
//   Algorithm copied from Matrix class
//
////////////////////////////////////////////////////////////////////////////////
double TAtm::angleBetween(TAtm* atom1, TAtm* atom2)
{
#define PI  3.14159265358979323846
#define RAD * 180.0 / PI

  double delta1,delta2;
  double vec1mag = 0;
  double vec2mag = 0;
  double dot = 0;
  int axis;
  for(axis=0;axis<3;axis++)
  {
    delta1 = (atom1->p_coords[axis]) - p_coords[axis];
    delta2 = (atom2->p_coords[axis]) - p_coords[axis];
    vec1mag += delta1 * delta1;
    vec2mag += delta2 * delta2;
    dot += delta1 * delta2;
  }
  vec1mag = sqrt(vec1mag);
  vec2mag = sqrt(vec2mag);
  return (acos(dot/(vec1mag*vec2mag)) RAD);
}

////////////////////////////////////////////////////////////////////////////////
//
// Description
//   Calculate the distance of this atom from another
//
// Implementation
//   D = sqrt(deltaX^2 + deltaY^2 + deltaZ^2)
//
////////////////////////////////////////////////////////////////////////////////
double TAtm::distanceFrom(TAtm* atom)
{
  double temp = 0;
  double delta = 0;
  int cntr;
  const double* thisCoords = coordinates();
  const double* thatCoords = atom->coordinates();
  for (cntr=0; cntr <3; cntr++)
  {
    delta = (thisCoords[cntr] - thatCoords[cntr]);
    temp += delta * delta;
  }
  return (sqrt(temp));
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Extraction Operator
//
//  Implementation
//    We dump the atom to the specified stream.  Reference information and
//    information contained in the superclass, in addition to the state
//    if the instance, are output.
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const TAtm& atom)
{
  // Header
  os << "[TAtm<";
  os << "index: " << atom.index() << ",";
  os << "residueName: " << atom.residueName() << ",";
  os << "residue#: " << atom.residueNumber() << ",";
  os << "atomicSymbol: " << atom.atomicSymbol() << ",";
  os << "atomName: " << atom.atomName() << ",";
  os << "atomicNumber: " << atom.atomicNumber() << ",";
  os << "partial charge: " << atom.partialCharge() << ",";
  //os << "shape: " << atom.shape() << ",";
  os << "shape: " << atom.shapeString() << ",";
  os << "coordinates: " << atom.coordinates()[0];
  os << " : " << atom.coordinates()[1];
  os << " : " << atom.coordinates()[2] << ",";
  os << "#bonds " << atom.p_bonds.size() << endl;

  // Trailer
  os << ">]" << endl;
  return os;
}



