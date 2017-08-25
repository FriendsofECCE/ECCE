///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TChartOfNuclides.H
//
//
// CLASS SYNOPSIS:
//   Class for Accessing Isotope information.
//
// EXPORTED TYPES:
//    Class TChartOfNuclides
//
// LOCAL TYPES:
//
// SEE ALSO:
//    Class TRefIsotope
//      
// DESCRIPTION:
//    The TChartOfNuclides class provides the interface from programmer
//    to Chart of the Nuclides reference
//    information.  
//
//    Pictorially, the ChartOfNuclides class is related to other
//    classes as:
//
//
// EXAMPLES:
//
//    TRefIsotope &A;
//    TChartOfNuclides CN;
//
//    A = CN::isotope("H",2);
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TCHARTOFNUCLIDES_HH
#define TCHARTOFNUCLIDES_HH

#include <string>
#include <vector>
  using std::string;
  using std::vector;


class TRefIsotope;

class TChartOfNuclides 
{

public:
  // Constructors
    TChartOfNuclides(void);

  // Virtual Destructor
    virtual ~TChartOfNuclides(void);

  // Accessors
    TRefIsotope const * isotope(const string& atomic_symbol,
                              int atomicMassNumber) const;
    bool contains(const string& atomic_symbol,
                       int atomicMassNumber) const;
    int entries(void) const;
    int numIsotopes(const string& atomic_symbol) const;
    int numIsotopes(int atomicMassNumber) const;
    vector <TRefIsotope*> 
                   *isotopeList(const string& atomicSymbol) const;
    vector<TRefIsotope*>
                   *isotopeList(int atomicNumber) const;
    TRefIsotope const * mostAbundantIsotope(const string& atomicSymbol) const;
    TRefIsotope const * mostAbundantIsotope(int atomicNumber) const;


protected:

private:

  // State
    static vector<TRefIsotope*>  *p_cn;

  // Encapsulated Behavior
    static bool load(const string& file_name);


};

#endif // TCHARTOFNUCLIDES_HH
