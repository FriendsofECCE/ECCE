///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: TChartOfNuclides.C
//
//
// DESIGN:
//    The ChartOfNuclides class encapsulates all the state and functionality
//    necessary to create a standard Chart of The Nuclides interface.
//
//    Because Chart of the Nuclides instances are only references to shared
//    information, many operators are not needed (copy constructor,
//    comparison operators, assignemnt operators, etc.).
//
//    The class itself manages a collection of TRefIsotope objects.  Because
//    TChartOfNuclides and TRefIsotope are closely tied together,
//    TChartOfNuclides is a friend of RefIsotopes.  This decision was weighed
//    against making TRefIsotopes manage their own extent of instances and
//    merging TChartOfNuclides class functionality together with the
//    TRefIsotope functionality.  We choose to keep the two classes
//    separate and avoid semantic snags such as:  what does it mean to
//    copy a TRefIsotope and what affect does that have on the TChartOfNuclides.
//
///////////////////////////////////////////////////////////////////////////////

// system includes
#include <stdlib.h>
#include <fstream>
using std::ifstream;


// application includes
#include "tdat/TChartOfNuclides.H"
#include "tdat/TRefIsotope.H"
#include "tdat/TPerTab.H"

//#include "util/ErrMsg.H"
#include "util/Preferences.H"
#include "util/Ecce.H"
#include "util/StringTokenizer.H"

// Class statics
vector<TRefIsotope*> *TChartOfNuclides::p_cn = 0;

// File statics
static TPerTab v_pt;


// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Default Constructor
//
//  Implementation
//    Load the data the first time the constructor is called.
//
///////////////////////////////////////////////////////////////////////////////
TChartOfNuclides::TChartOfNuclides(void)
{
  if (p_cn == 0) {
    load("Isotopes.txt");
  }
}

// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Virtual Destructor
//
//  Implementation
//    Nothing to do.  On process exit the p_cn vector will be cleaned up.
//
///////////////////////////////////////////////////////////////////////////////
TChartOfNuclides::~TChartOfNuclides(void)
{ }


// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Retrieve Reference Information by Atomic Symbol and Atomic Mass
//
//    The atomicMassNumber will default to 0. A zero atomicMassNumber
//    will be interpreted as the most abundant isotope for the given
//    element.
//
///////////////////////////////////////////////////////////////////////////////
TRefIsotope const * TChartOfNuclides::isotope(const string& atomicSymbol,
                         int atomicMassNumber = 0) const
{
  TRefIsotope const * answer = 0;
  if (atomicMassNumber == 0) {
    answer = mostAbundantIsotope(atomicSymbol);
  } else {
    int cnt = p_cn->size();
    TRefIsotope *obj;
    for (int idx=0; idx<cnt; idx++) {
      obj = (*p_cn)[idx];
      if (obj->atomicMassNumber() == atomicMassNumber &&
          atomicSymbol == obj->atomicSymbol().c_str() ) {
        answer = obj;
        break;
      }
    }
  }
  return answer;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Check to See if Atomic Symbol, atomic mass number combination is Valid
//
//  Implementation
//    This function uses isotope() to return true or false based
//    on whether the atomic symbol, mass number combination was found in the
//    Nuclide Chart.
//
///////////////////////////////////////////////////////////////////////////////
bool TChartOfNuclides::contains(const string& atomicSymbol,
                                    int atomicMassNumber) const
{
  return  isotope(atomicSymbol, atomicMassNumber) != 0;
}

///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//    Get the Number of Elements in the Nuclide Chart.
//	
//  Implementation
//    Use the cardinality function for the os_Array that is the Chart of
//    the nuclides to determine the number of elements.
//
///////////////////////////////////////////////////////////////////////////////
int TChartOfNuclides::entries(void) const
{
  return p_cn->size();
}

///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//    Get the Number of Isotopes for a given element.
//	
//  Implementation
//    Use the cardinality function for the os_Array associated with the
//    given element.
//
///////////////////////////////////////////////////////////////////////////////
int TChartOfNuclides::numIsotopes(const string& atomic_symbol) const
{
  int ret = 0;
  vector<TRefIsotope*>* list = isotopeList(atomic_symbol);
  if (list) ret = list->size();
  delete list;
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//  man 
//
//  Description
//    Get the Number of Isotopes for a given element.
//	
//
///////////////////////////////////////////////////////////////////////////////
int TChartOfNuclides::numIsotopes(int atomicMassNumber) const
{
  int ret = 0;
  vector<TRefIsotope*>* list = 
              isotopeList(v_pt.atomicSymbol(atomicMassNumber));
  if (list) ret = list->size();
  delete list;
  return ret;
}


///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Get a list of isotopes corresponding to a given atomic symbol
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
vector<TRefIsotope*>*
TChartOfNuclides::isotopeList(const string& atomicSymbol) const
{
   vector<TRefIsotope*> *answer = (vector<TRefIsotope*>*) 0;
   if (v_pt.isValid(atomicSymbol)) {
     // 10 is just some number lower than default of 64
     answer = new vector<TRefIsotope*>;
     int cnt = p_cn->size();
     for (int idx=0; idx<cnt; idx++) {
       if (atomicSymbol == (*p_cn)[idx]->atomicSymbol().c_str() ) {
         answer->push_back((*p_cn)[idx]);
       }
     }
   }
   return answer;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Get a list of isotopes corresponding to a given atomic number
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
vector<TRefIsotope*>*
TChartOfNuclides::isotopeList(int atomicNumber) const
{
   return isotopeList(v_pt.atomicSymbol(atomicNumber));
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Get a pointer to the most abundant isotope for a given element.
//
//  Implementation
//    Null value returned for invalid atomic symbol.
//
///////////////////////////////////////////////////////////////////////////////
TRefIsotope const *
TChartOfNuclides::mostAbundantIsotope(const string& atomicSymbol) const
{
   TRefIsotope const * answer = (TRefIsotope*) 0;
   if (v_pt.isValid(atomicSymbol)) {
     TRefIsotope const * tmp = 0;
     int cnt = p_cn->size();
     answer = (TRefIsotope*)0;
     for (int idx=0; idx<cnt; idx++) {
       tmp = (*p_cn)[idx];
       if (atomicSymbol == tmp->atomicSymbol().c_str() ) {
         if (answer == 0) {
           answer = tmp;
         } else if (tmp->abundance() > answer->abundance()) {
           answer = tmp;
         }
       }
       if (answer == (TRefIsotope*)0) {
       }
     }
   }
   return answer;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Get a pointer to the most abundant isotope for a given element.
//
//  Implementation
//    This can be called from either inside or outside of a transaction
//    Null value returned for invalid atomic symbol.
//
///////////////////////////////////////////////////////////////////////////////
TRefIsotope const *
TChartOfNuclides::mostAbundantIsotope(int atomicNumber) const
{
   return mostAbundantIsotope(v_pt.atomicSymbol(atomicNumber));
}

// ------------------------
// Private Member Functions
// ------------------------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Load the isotope data in to internal structures.
//	
///////////////////////////////////////////////////////////////////////////////
bool TChartOfNuclides::load(const string& load_file)
{
  string inFile((string(Ecce::ecceDataPrefPath()) + "/" + load_file).c_str());

  p_cn = new vector<TRefIsotope*>;

// Open the input file

  string master(inFile);

  ifstream masterstr(master.c_str());

  //EE_RT_ASSERT(masterstr,EE_FATAL,"Isotope data file not found");


  string rwline;
  char buf[1024];
  while ( !masterstr.eof() ) {
    masterstr.getline(buf, 1023);
    rwline = buf;

//  Skip blank lines

    if (rwline.length() == 0) continue;

//  Create a new isotope to put in the table

    TRefIsotope *new_isotope = new TRefIsotope();

//  Parse out the isotope information from the file

    StringTokenizer tok(rwline);
    string inNumber,inSymbol,inMassNumber,inWeight,inAbundance;
    inNumber = tok.next();
    inSymbol = tok.next();

    //  Skip to next line if there is no data

    if (inSymbol == "void") continue;

    inMassNumber = tok.next();
    inAbundance = tok.next();
    inWeight = tok.next();

//  Separate out the atomic symbol from the symbol stored in the file
//  Input file provides "WHT-XX-NM" - we want the XX with the "WHT-" and
//  "-NM" removed. (WHT = weight number, NM = atomic number)

    int i = inSymbol.find('-');
    if (i != string::npos) inSymbol.erase(0,i+1);
    i = inSymbol.find('-');
    if (i != string::npos) inSymbol.erase(i,inSymbol.size()-i);

//  Place the information from the input file into the isotope object

    new_isotope->atomicSymbol(inSymbol.c_str());
    new_isotope->atomicMassNumber(atoi(inMassNumber.c_str()));
    new_isotope->isotopeWeight(atof(inWeight.c_str()));
    new_isotope->abundance(atof(inAbundance.c_str())/100);

// Insert the new isotope into the nuclide chart

    p_cn->push_back(new_isotope);
  }
  masterstr.close();
  return true;
}
