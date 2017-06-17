#include <iostream>
using namespace std;
///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TPerTab.C
//
//
// DESIGN:
//   The TPerTab provides a simple interface for periodic table
//   data.  It is a replacement for the old objectstore-based objects.
//   Mostly this class is used to translate between symbols and
//   numbers but all the data that was originally collected was 
//   retained.
//
//   The Van Derr Waals radius should be defined per charge state.
//   However, for this to be useful, we must support the definition
//   of charges on atoms.  Since we are not doing this, there is a
//   single vwr in the internal table.  This will have to be enhanced
//   some day.  Some of this data exists in the AtomicOxidatationStates
//   files but it was not being used.
//    
//   This is a very simple class.  The design goals:
//   . Be fast and cheap.  The bulk of the data is compiled in.  The
//     initial construction reads preferences but basically creating 
//     instances is very cheap.
//   . Support updates without re-compiling.  This is done with user
//     and system preference files.
//   . KISS - (Keep It Simple Stupid).  Well I think the code speaks
//     for itself.
//
//   The preference file format could use some improvement but it works.
//
///////////////////////////////////////////////////////////////////////////////

#include <iomanip>
  using std::ios;
#include <fstream>
  using std::ofstream;
  using std::ifstream;

#include "util/ErrMsg.H"
#include "util/KeyValueReader.H"
#include "tdat/TPerTab.H"
#include "tdat/TOxidationState.H"
#include "tdat/TRefElement.H"
#include "util/Color.H"
#include "util/Ecce.H"
#include "util/SFile.H"
#include "util/StringTokenizer.H"

map<string,int,less<string> > *TPerTab::p_symhash=0;
vector<TRefElement*>       *TPerTab::p_refel=0;
vector<TRefElement*>       *TPerTab::p_userel=0;

map<string*,TOxidationState*,ltstringPtr > *TPerTab::p_refoxhash=0;
map<string*,TOxidationState*,ltstringPtr > *TPerTab::p_useroxhash=0;


////////////////////////////////////////////////////////////////////
// Description
//   Create static reference data and copy to user data.  Then
//   override the user data.  In either case just do this once
//   per process unless otherwise requested.  The client prograrm
//   can force a reload of preference data.
////////////////////////////////////////////////////////////////////
TPerTab::TPerTab(bool refOnly) {

  bool firstTime = false;
  if (p_refel == 0) {
    firstTime = true;
    loadNeutralData();
    loadOxidationData();

    // create a hash map for atom symbols - always using lower case
    // This is static since it won't vary per instance.
    p_symhash = new map<string, int, less<string> >;

    string sym;
    int cnt = p_refel->size();
    for (int idx=0; idx<cnt; idx++) {
      sym = (*p_refel)[idx]->atomicSymbol();
      for (size_t i=0; i<sym.size(); i++) sym[i] = tolower(sym[i]);
      (*p_symhash)[sym] = idx;
    }

  }

  // These are here so we can change this class to provided either
  // reference or user data.
  if (refOnly) {
    p_elements = p_refel;
    p_oxhash = p_refoxhash;
  } else {
    p_elements = p_userel;
    p_oxhash = p_useroxhash;
  }

  if (firstTime) {
    // Now load the user data too
    loadUserData();
  }

}

////////////////////////////////////////////////////////////////////
// Description
//   Nothing to do since the data is static.
////////////////////////////////////////////////////////////////////
TPerTab::~TPerTab() {
}

/**
 * Cleans up static memory.
 **/
void
TPerTab::finalize() {

  if (p_symhash) {
    delete  p_symhash;
    p_symhash = 0;
  }

  if (p_refel) {
    for (size_t idx=0; idx<p_refel->size(); idx++) {
      delete (*p_refel)[idx];
    }
    p_refel->clear();
    delete p_refel;
    p_refel = 0;
  }

  if (p_userel) {
    for (size_t idx=0; idx<p_userel->size(); idx++) {
      delete (*p_userel)[idx];
    }
    p_userel->clear();
    delete p_userel;
    p_userel = 0;
  }

  map<string*,TOxidationState*,ltstringPtr >::iterator refit;
  for (refit=p_refoxhash->begin(); refit != p_refoxhash->end(); refit++) {
    delete (*refit).first;
    delete (*refit).second;
  }
  p_refoxhash->clear();
  delete p_refoxhash;
  p_refoxhash = 0;

  map<string*,TOxidationState*,ltstringPtr >::iterator userit;
  for (userit=p_useroxhash->begin(); userit != p_useroxhash->end(); userit++) {
    delete (*userit).first;
    delete (*userit).second;
  }
  p_useroxhash->clear();
  delete p_useroxhash;
  p_useroxhash = 0;


}

////////////////////////////////////////////////////////////////////
// Description
////////////////////////////////////////////////////////////////////
void TPerTab::writeRadiiOverrides(double* vanderWaalsRadii,
                                  double* covalentRadii,
                                  double* covalentRadiiTolerances)
{
  string radiiFile = Ecce::realUserPrefPath();
  radiiFile += "AtomicRadii";
  ofstream radiiStream(radiiFile.c_str(), (ios::out | ios::trunc));

  if (radiiStream) {
    radiiStream << "# This file contains user modified defaults for " <<
    "covalent and van der Waals\n# radii.  Values are not required for all " <<
    "elements since these values\n# just override system defaults.\n#\n";
    radiiStream << "# To override the value for an element, add the element " <<
    "name, van der Waals\n# radius, covalent radius then tolerance for " <<
    "determining bond formation\n#  (minus the the '#' comment) as in:\n" <<
    "#H: 1.1 0.4 1.2\n#\n# Order of elements is not significant.  Case is " <<
    "sensitive.\n#\n";

    int nAtoms = numAtoms();
    for (int idx=0; idx<nAtoms; idx++)
      if (vanderWaalsRadii[idx]!=vwr(idx) ||
          covalentRadii[idx]!=covalentRadius(idx) ||
          covalentRadiiTolerances[idx]!=covalentRadiusTolerance(idx))
        radiiStream << atomicSymbol(idx) << ": " << vanderWaalsRadii[idx] <<
                       " " << covalentRadii[idx] <<
                       " " << covalentRadiiTolerances[idx] << endl;

    radiiStream.close();
  }
}

//////////////////////////////////////////////////////////////////////////
// Description
//   Load in the reference element data currently from a text file.  This should
//   only happen once per process.
//   There ought to be some error handling!!!!
//////////////////////////////////////////////////////////////////////////
void TPerTab::loadNeutralData() {
  if (p_refel == 0) {
    p_refel = new vector<TRefElement*>;
    p_userel = new vector<TRefElement*>;

    string tmp =  Ecce::ecceDataPrefPath();
    tmp += "/AtomicElements";
    ifstream is(tmp.c_str());
    int atomicNum = 0;
    EE_RT_ASSERT((is.good()),EE_FATAL,"Unable to load Element data.");
    KeyValueReader reader(is);
    reader.stripTrailingComments(false);
    string key, value;
    int ival;
    float fval;
    TRefElement *el;
    while (reader.getpair(key,value)) {
      key = TPerTab::mixedCase(key);
      el = new TRefElement();
      StringTokenizer tokenizer(value,",");

      tmp = tokenizer.next();
      el->atomicSymbol(key);
      el->name(tmp);
      el->atomicNumber(atomicNum++);

      // row and column
      tmp = tokenizer.next();
      sscanf(tmp.c_str(),"%d",&ival);
      el->row(ival);
      tmp = tokenizer.next();
      sscanf(tmp.c_str(),"%d",&ival);
      el->column(ival);

      // Physical state
      tmp = tokenizer.next();
      if (tmp.find("gas") != string::npos) {
        el->state(TPerTab::gasElement);
      } else if (tmp.find("solid") != string::npos) {
        el->state(TPerTab::solidElement);
      } else if (tmp.find("liquid") != string::npos) {
        el->state(TPerTab::liquidElement);
      } else if (tmp.find("synthetic") != string::npos) {
        el->state(TPerTab::syntheticElement);
      } else {
        EE_RT_ASSERT(0,EE_WARNING,"Unrecongized state " + tmp);
      }

      // Metallic state
      tmp = tokenizer.next();
      if (tmp.find("metallic") != string::npos) {
        el->metal(TPerTab::metallic);
      } else if (tmp.find("nonMetallic") != string::npos) {
        el->metal(TPerTab::nonMetallic);
      } else if (tmp.find("semiMetallic") != string::npos) {
        el->metal(TPerTab::semiMetallic);
      } else {
        EE_RT_ASSERT(0,EE_WARNING,"Unrecongized state " + tmp);
      }

      // Color
      tmp = tokenizer.next();
      int idx = tmp.size();
      for (int i = tmp.size()-1; i>=0; i--) {
        if (tmp[i] != ' ') {
          break;
        } else {
          idx = i;
        }
      }
      tmp.erase(idx,tmp.size()-idx);
      Color defcolor(tmp);
      el->color(defcolor);

      // Electron affinity
      tmp = tokenizer.next();
      el->electronAffinities(tmp);

      // weight
      tmp = tokenizer.next();
      sscanf(tmp.c_str(),"%f",&fval);
      el->atomicWeight(fval);

      // density
      tmp = tokenizer.next();
      sscanf(tmp.c_str(),"%f",&fval);
      el->density(fval);

      // melting point
      tmp = tokenizer.next();
      sscanf(tmp.c_str(),"%f",&fval);
      el->meltingPoint(fval);

      // boiling point
      tmp = tokenizer.next();
      sscanf(tmp.c_str(),"%f",&fval);
      el->boilingPoint(fval);

      // covalent radius
      tmp = tokenizer.next();
      sscanf(tmp.c_str(),"%f",&fval);
      el->covalentRadius(fval);

      // covalent radius tolerance
      tmp = tokenizer.next();
      sscanf(tmp.c_str(),"%f",&fval);
      el->crTolerance(fval);
      p_refel->push_back(el);
      // Make an exact copy for user data - override later
      p_userel->push_back(new TRefElement(*el));
    }
    is.close();
  }
}

//////////////////////////////////////////////////////////////////////////
//
// Description
//   Load in the reference data that varies per charge state.
//////////////////////////////////////////////////////////////////////////
void TPerTab::loadOxidationData() {
  if (p_refoxhash == 0) {
    p_refoxhash = new map<string*,TOxidationState*,ltstringPtr >;
    p_useroxhash = new map<string*,TOxidationState*,ltstringPtr >;
    string tmp =  Ecce::ecceDataPrefPath();
    tmp += "/AtomicOxidationStates";
    ifstream is(tmp.c_str());
    EE_RT_ASSERT((is.good()),EE_WARNING,"Unable to load Oxidatation data.");
    KeyValueReader reader(is);
    string key, value, scharge, svwr, econfig;
    TOxidationState *ox;
    int charge;
    double vwr;
    while (reader.getpair(key,value)) {
      charge = 0;
      vwr = 1.0;
      StringTokenizer tokenizer(value);
      scharge = tokenizer.next();
      if (!scharge.empty()) sscanf(scharge.c_str(),"%d",&charge);
      svwr = tokenizer.next();
      if (!svwr.empty()) sscanf(svwr.c_str(),"%lf",&vwr);
      econfig = tokenizer.next();
      ox = new TOxidationState(key,charge);
      ox->vwr(vwr);
      ox->electronConfiguration(econfig);
      (*p_refoxhash)[new string(key+scharge)] = ox;
      (*p_useroxhash)[new string(key+scharge)] = new TOxidationState(*ox);
    }
    is.close();
  }
} 



//////////////////////////////////////////////////////////////////////////
//
// Description
//  Load in user data.  Currently it is ASSUMED that there are
//  OxidatationState objects already in existence for any vwr that
//  the user can set preferences on.
//////////////////////////////////////////////////////////////////////////
void TPerTab::loadUserData()
{
  string tmp = Ecce::realUserPrefPath();
  tmp += "AtomicRadii";
  SFile user(tmp);
  if (user.exists()) {
    ifstream useris(user.path().c_str());
    KeyValueReader reader(useris);
    string symbol, value, token;
    int anum;
    double val;
    TRefElement *el;
    while (reader.getpair(symbol,value)) {
      if (!isValid(symbol)) continue;    // skip if not an atomic symbol
      anum = atomicNumber(symbol);
      el = (*p_userel)[anum];
      StringTokenizer tokenizer(value);
      if (!((token = tokenizer.next()).empty())) {
        TOxidationState *ox = oxidation(anum,0); // neutral only for now
        sscanf(token.c_str(),"%lf",&val);
        ox->vwr(val);
      }
      if (!((token = tokenizer.next()).empty())) {
        sscanf(token.c_str(),"%lf",&val);
        el->covalentRadius(val);
      }
      if (!((token = tokenizer.next()).empty())) {
        sscanf(token.c_str(),"%lf",&val);
        el->crTolerance(val);
      }
    }
    useris.close();
  }
}


int TPerTab::numAtoms() const {
  return p_elements->size();
}

int TPerTab::nubAtom() const {
  return (*p_elements)[numAtoms()-1]->atomicNumber();
}


int TPerTab::atomicNumber(const string& atomicSymbol) const {
  int ret=0;
  string sym = atomicSymbol;
  for (size_t i=0; i<sym.size(); i++) sym[i] = tolower(sym[i]);
  if (p_symhash->find(sym) != p_symhash->end()) ret = (*p_symhash)[sym];
  return ret;
}

string TPerTab::atomicSymbol(int atomicNumber) const {
  string ret = "X";
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->atomicSymbol();
}

// Description
//    Validates against the range of atomic numbers.
bool TPerTab::isValid(int atomicNumber) const {
  return atomicNumber >= 0 && atomicNumber <= numAtoms();
}

bool TPerTab::isValid(const string& symbol) const  {
  string sym = symbol;
  for (size_t i=0; i<sym.size(); i++) sym[i] = tolower(sym[i]);
  return (p_symhash->find(sym) != p_symhash->end());
}
string TPerTab::name(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->name();
}

int TPerTab::row(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->row();
}
int TPerTab::column(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->column();
}
TPerTab::ElementState TPerTab::state(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->state();
}
TPerTab::ElementMetal TPerTab::metal(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->metal();
}
string TPerTab::electronAffinity(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->electronAffinities();
}
float TPerTab::atomicWeight(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->atomicWeight();
}
float TPerTab::density(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->density();
}
float TPerTab::meltingPoint(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->meltingPoint();
}
float TPerTab::boilingPoint(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->boilingPoint();
}

double TPerTab::covalentRadius(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->covalentRadius();
}
double TPerTab::covalentRadiusTolerance(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->crTolerance();
}
double TPerTab::vwr(int atomicNumber,int charge) {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  double ret = 0;
  TOxidationState *ox = oxidation(atomicNumber,charge);
  if (ox) ret = ox->vwr();
     
  return ret;
}

string TPerTab::electronConfig(int atomicNumber,int charge) {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  string ret;
  TOxidationState *ox = oxidation(atomicNumber,charge);
  if (ox) ret = ox->electronConfiguration();
  return ret;
}

TOxidationState *TPerTab::oxidation(int atomicNumber, int charge) {
  string key = atomicSymbol(atomicNumber);
  char buf[10];  // way big
  sprintf(buf,"%d",charge);
  key += buf;
  TOxidationState *ox = (TOxidationState*)0;
  if (p_oxhash->find(&key) != p_oxhash->end()) {
    ox = (*p_oxhash)[&key];
  }
  EE_RT_ASSERT(ox,EE_WARNING,"Unable to find vwr for " + key);
  return ox;
}

const Color& TPerTab::color(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->color();
}

// Return X-style color name
string TPerTab::colorName(int atomicNumber) const {
  EE_ASSERT(isValid(atomicNumber), EE_WARNING,"Atomic number out of range ");
  return (*p_elements)[atomicNumber]->color().xcolor();
}


string TPerTab::mixedCase(const string& oldSymbol) {
   // Don't dump core due to this - the  app must handle it
   if (oldSymbol.length() == 0) return "";

   string newSymbol = oldSymbol;
   newSymbol[0] = toupper(oldSymbol[0]);
   for (size_t i = 1; i<oldSymbol.size(); i++) newSymbol[i] =
     tolower(oldSymbol[i]);
   return newSymbol;
}
