///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TPerTab.H
//
//
// CLASS SYNOPSIS:
//   Class for Accessing Periodic Table Reference Information
//
// SEE ALSO:
//      
// DESCRIPTION:
//   This class implements the singleton pattern.
//   The basic data is compiled in.  Where appropriate, this data
//   can be overridden by system files for easier maintenance.
//
//   The data is primarily reference data (read-only) but with support
//   for user preference overrides.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TPERTAB_HH
#define TPERTAB_HH

#include <string>
#include <vector>
#include <map>
  using std::string;
  using std::vector;
  using std::map;
  using std::less;

#include "util/CompareOps.H"

class Color;
class TOxidationState;
class TRefElement;

// application includes
// This is unfortunate but some classes need a number for array
// sizes in the header file.  Rather than fix those pieces of code,
// just add this BUT we have to keep it up to date.
static const int TPTnumAtoms = 120;


class TPerTab 
{

public:
    enum ElementState { 
         solidElement, gasElement, liquidElement, syntheticElement
    };

    enum ElementMetal {
      nonMetallic, semiMetallic, metallic
    };

  // Constructors
    TPerTab(bool refOnly=false);

  // Virtual Destructor
    virtual ~TPerTab(void);

  // Accessors
    int numAtoms(void) const;
    int nubAtom(void) const;
    int atomicNumber(const string& atomicSymbol) const;
    string atomicSymbol(int atomicNumber) const;
    bool isValid(const string& atomicSymbol) const;
    bool isValid(int atomicNumber) const;

    int                row(int atomicNumber) const;
    int                column(int atomicNumber) const;
    TPerTab::ElementState state(int atomicNumber) const;
    TPerTab::ElementMetal metal(int atomicNumber) const;
    const Color&       color(int atomicNumber) const;
    string             colorName(int atomicNumber) const;
    string             name(int atomicNumber) const;
    string             electronAffinity(int atomicNumber) const;
    float              atomicWeight(int atomicNumber) const;
    float              density(int atomicNumber) const;
    float              meltingPoint(int atomicNumber) const;
    float              boilingPoint(int atomicNumber) const;
    double             covalentRadius(int atomicNumber) const;
    double             covalentRadiusTolerance(int atomicNumber) const;
    double             vwr(int atomicNumber,int charge=0);
    string             electronConfig(int atomicNumber,int charge=0);

    void               writeRadiiOverrides(double* vanderWaalsRadii,
                                           double* covalentRadii,
                                           double* covalentRadiiTolerances);

    // Force load or reload of user preference data.
    void loadUserData();

    // Clear internal static data - useful to call at program termination to
    // clean up memory.
    static void finalize();

    static string mixedCase(const string& oldSymbol);


protected:

    // These point to either the ref or user versions depending on the
    // instance was constructed.
    vector<TRefElement*> *p_elements;
    map<string*, TOxidationState*, ltstringPtr > *p_oxhash;

    // for fast symbol to number conversion
    static map<string,int, less<string> > *p_symhash;
    static vector<TRefElement*>           *p_refel;
    static vector<TRefElement*>           *p_userel;

    // hash is symbol+charge
    static map<string*, TOxidationState*, ltstringPtr> *p_refoxhash;
    static map<string*, TOxidationState*, ltstringPtr> *p_useroxhash;

    void loadOxidationData();
    void loadNeutralData();

    TOxidationState *oxidation(int atomicNumber, int charge);

private:

};

#endif // TPERTAB_HH
