/**
 * @file
 *
 *
 */

#include <iostream>
  using std::ostream;
  using std::endl;

#include <string.h>

#include "tdat/Residue.H"
#include "tdat/Fragment.H"
#include "tdat/TResTab.H"

#include "util/STLUtil.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"

static const char* vstatusMap[]   = {"Missing", "Complete", "Incomplete",
                                     "Unknown", "Generatable"};
static int vnumStatus = sizeof(vstatusMap)/sizeof(char*);


// -----------------------
// Public Member Functions
// -----------------------


// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  Description
//    Create a residue. 
//
//
///////////////////////////////////////////////////////////////////////////////
Residue::Residue(const char *cname, const int& number)
      : p_displayStyle(DisplayStyle::BALLWIRE)
{

  name(cname);
  p_number = number;
  p_chain = ' ';
  p_coupling = Residue::MONOMER;
  p_status = Residue::UNKNOWN;
  p_attribute = Residue::NONE;
  p_insertCode = ' ';
  p_firstAtomPtr = (TAtm*)0 ;
  strncpy(p_segID,"    ",4);
  p_segID[4] = '\0';

  p_index = 0;

}


///////////////////////////////////////////////////////////////////////////////
//  Description
//    Copy constructor.
//
///////////////////////////////////////////////////////////////////////////////
Residue::Residue(const Residue& rhs)
{ 
  *this = rhs; 
}

 
// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Virtual Destructor
//
///////////////////////////////////////////////////////////////////////////////
Residue::~Residue(void)
{
}

// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
//  Description
//    Assign one Residue from another Residue
//
///////////////////////////////////////////////////////////////////////////////
Residue& Residue::operator=(const Residue& rhs)
{
  if (this != &rhs) {
    name(rhs.name().c_str());
    number(rhs.number());
    chain(rhs.chain());
    insertCode(rhs.insertCode());
    couplingInfo(rhs.couplingInfo());
    residueStatus(rhs.residueStatus());
    setProteinAttribute(rhs.getProteinAttribute());
    segID(rhs.segID().c_str());
    displayStyle(rhs.displayStyle());
    index(rhs.index());

    //do we copy this?
    firstAtomPtr(rhs.firstAtomPtr());
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
//  Description
//    Identity-Based Comparison
//
///////////////////////////////////////////////////////////////////////////////
bool Residue::operator==(const Residue& rhs) const
{ return (this == &rhs); }

///////////////////////////////////////////////////////////////////////////////
//  Description
//    Identity-Based Comparison
//
///////////////////////////////////////////////////////////////////////////////
bool Residue::operator!=(const Residue& rhs) const
{ return (this != &rhs); }


unsigned long Residue::hFun(const Residue& r)
{
  return (unsigned long)&r;
}


/**
 * Convert status string to integer representation.
 */
Residue::ResidueStatus Residue::statusFromString(const string& status)
{
   Residue::ResidueStatus ret = MISSING;
   for (int idx=0; idx<vnumStatus; idx++) {
      if (status == vstatusMap[idx]) {
         ret = (Residue::ResidueStatus)idx;
         break;
      }
   }
   return ret;
}

/**
 * Convert status value to string representation.
 */
string Residue::statusToString(Residue::ResidueStatus status)
{
   return vstatusMap[status];
}

string Residue::residueStatusAsString() const
{
   return statusToString(p_status);
}

/**
 * Parse input string and try and come up with a residue name, PDB index, and
 * chain ID. If all else fails, come up with an absolute residue index
 * (corresponding to the internal index of the residue in the SGFragment).
 */
void Residue::parseString(string input, string &name, int &pdb_idx,
                          char &chain, int &idx)
{
  name = "";
  pdb_idx = -1;
  chain = ' ';
  idx = -1;
  StringTokenizer tokenizer(input, ":");
  vector<string> tokens = tokenizer.tokenize(":");

  int nsize = tokens.size();
  int i;
  for  (i=0; i<nsize; i++) {
    STLUtil::stripLeadingAndTrailingWhiteSpace(tokens[i]);
  }

  if (nsize == 1) {
    // If token is an integer, then assume that it corresponds to the Ecce
    // residue index. Subtract 1 to get an internal index.
    if (!StringConverter::toInt(tokens[0], idx)) {
      if (tokens[0].size() < 5) {
        name = tokens[0];
      }
    } else {
      idx--;
    }
  } else if (nsize == 2) {
    // Assume first token is residue name if it is not an integer, otherwise
    // assume that it is a PDB index
    if (!StringConverter::toInt(tokens[0], pdb_idx)) {
      if (tokens[0].size() < 5) {
        name = tokens[0];
        // Assume second token is either a PDB index or a chain ID
        if (!StringConverter::toInt(tokens[1],pdb_idx)) {
          if (tokens[1].size() == 1) {
            chain = (tokens[1])[0];
          }
        }
      }
    } else {
      // First token is PDB index, see if second token is a chain ID
      if (tokens[1].size() == 1) {
        chain = (tokens[1])[0];
      }
    }
  } else if (nsize >= 3) {
    // Assume first token is a residue name, second token is a PDB index,
    // and third token is a chain ID. If fourth token exists, assume
    // that it is the internal index. Ignore any additional tokens.
    if (!StringConverter::toInt(tokens[0], i)) {
      if (tokens[0].size() < 5) {
        name = tokens[0];
      }
    }
    StringConverter::toInt(tokens[1], pdb_idx);
    if (!StringConverter::toInt(tokens[2],i)) {
      if (tokens[2].size() == 1) {
        chain = (tokens[2])[0];
      }
    }
    if (nsize >= 4) {
      if (StringConverter::toInt(tokens[1], idx)) {
        idx--;
      }
    }
  }
  // if residue name is not blank, then make sure it is formatted correcly
  // as a four character PDB residue name.
  if (name != "") {
    int len = name.length();
    char t_name[5];
    strncpy(t_name,"    ",4);
    t_name[4] = '\0';

    // Make sure that 4th position is blank if name is three
    // characters or less. Otherwise use all characters (up to a
    // maximum of four).

    if (len < 4) {
      int ioff = 3-len;
      strncpy(t_name+ioff,name.c_str(),len);
      t_name[3] = ' ';
    } else {
      strncpy(t_name,name.c_str(),4);
    }
    name.clear();
    name = t_name;
  }
}

// ------------ Accessors -----------
///////////////////////////////////////////////////////////////////////////////
//  Description
//    Simple accessors.
//
///////////////////////////////////////////////////////////////////////////////
string Residue::name(void) const
{ return p_name; }
string Residue::segID(void) const
{ return p_segID; }
int Residue::number(void) const
{ return p_number; }
char Residue::chain(void) const
{ return p_chain; }
char Residue::insertCode(void) const
{ return p_insertCode; }
Residue::Coupling Residue::couplingInfo(void) const
{ return p_coupling; }
Residue::ResidueStatus Residue::residueStatus(void) const
{ return p_status; }
Residue::Attribute Residue::getProteinAttribute(void) const
{ return p_attribute; }
TAtm* Residue::firstAtomPtr(void) const
{ return p_firstAtomPtr; }
DisplayStyle Residue::displayStyle() const
{return p_displayStyle; }
int Residue::index() const
{ return p_index; }


// ------------ Modifiers -----------
///////////////////////////////////////////////////////////////////////////////
//  Description
//    Set the data members for this residue.
//
///////////////////////////////////////////////////////////////////////////////
void Residue::name(const char *name)
{ 

// Remove leading and trailing white space

  string resname(name);
  STLUtil::stripLeadingAndTrailingWhiteSpace(resname);

  int len = resname.length();
  strncpy(p_name,"    ",4);
  p_name[4] = '\0';

  // Make sure that 4th position is blank if name is three
  // characters or less. Otherwise use all characters (up to a
  // maximum of four).

  if (len < 4) {
    int ioff = 3-len;
    strncpy(p_name+ioff,resname.c_str(),len);
    p_name[3] = ' ';
  } else {
    strncpy(p_name,resname.c_str(),4);
  }
}

void Residue::number(const int& number)
{ p_number = number; }

void Residue::segID(const char *name)
{

// make sure segment ID is left-justified

  int len = strlen(name);
  p_segID[4] = '\0';
  strncpy(p_segID,"    ",4);
  int i;
  if (len < 4) {
    for (i=0; i < 4; i++) {
      if (i<len) {
        p_segID[i] = name[i];
      } else {
        p_segID[i] = ' ';
      }
    }
  } else {
    strncpy(p_segID,name,4);
  }
}
void Residue::chain(const char value) 
{ if (value != '\0') {
    p_chain = value;
  } else {
    p_chain = ' ';
  }
}
void Residue::insertCode(const char value) 
{ if (value != '\0') {
    p_insertCode = value;
  } else {
    p_insertCode = ' ';
  }
}
void Residue::couplingInfo(const Coupling& value) 
{ p_coupling = value; }
void Residue::residueStatus(const ResidueStatus& value) 
{ p_status = value; }
void Residue::setProteinAttribute(const Attribute& value)
{ p_attribute = value; }
void Residue::firstAtomPtr(TAtm* atom) 
{ p_firstAtomPtr = atom; }


/**
 * Set residue display style.
 */
void Residue::displayStyle(const DisplayStyle& ds )
{ 
   p_displayStyle = ds; 
} 

void Residue::index(int value)
{
   p_index = value;
}

/**
 * Make a determination of whether residue represent a solvent
 * molecule or not
 */
bool Residue::isSolvent() const
{
  bool ret = false;
  if (p_name[3] != ' ') {
    ret = true;
  } else if (strncmp(p_name,"WAT ",4) == 0) {
    ret = true;
  } else if (strncmp(p_name,"HOH ",4) == 0) {
    ret = true;
  }
  return ret;
}


// --------------------------
// Private Members
// --------------------------

// ---------- Encapsulated Behavior ----------

/////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Extraction Operator
//
/////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const Residue& rhs)
{
  // Header
  os << "[Residue<";
  os << "name: " << rhs.name().c_str() << ",";
  os << "number: " << rhs.number() << ",";
  os << "chain: " << rhs.chain() << ",";
  os << "coupling: ";
  switch (rhs.couplingInfo()) {
    case Residue::INTERIOR: 
      os << "Internal" << endl; break;
    case Residue::TAIL: 
      os << "Head" << endl; break;
    case Residue::HEAD: 
    default:
      os << "Tail" << endl; break;
  }
  os << "Status: ";
  switch (rhs.residueStatus()) {
    case Residue::MISSING: 
      os << "Missing" << endl; break;
    case Residue::COMPLETE: 
      os << "Complete" << endl; break;
    case Residue::INCOMPLETE: 
      os << "Incomplete" << endl; break;
    case Residue::UNKNOWN: 
      os << "Unknown" << endl; break;
    case Residue::GENERATABLE: 
      os << "Missing Hydrogens" << endl; break;
    default:
      os << "Missing" << endl; break;
  }

  // Trailer
  os << "]" << endl;
  return os;
}
