///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TFormula.H
//
//
// CLASS SYNOPSIS:
//    Class Formula Handles Chemical Formulas as Strings
//
// EXPORTED TYPES:
//    Class Formula
//
// LOCAL TYPES:
//
// SEE ALSO:
//      
// DESCRIPTION:
//    Class formula provides a number of utility operations that
//    can be used to convert strings to well-structured chemistry
//    data types and back.
//
// EXAMPLES:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TFORMULA_HH
#define TFORMULA_HH

#include <iostream>
  using std::istream;
  using std::ostream;

#include <string>
#include <vector>
#include <functional>
  using std::string;
  using std::vector;
  using std::less;

#include "tdat/TTCPair.H"
#include "util/EcceSortedVector.H"

class TFormula
{

public:

  // Constructors
    TFormula(const string& formula);        // create from string
    TFormula(const TFormula&);

  // Virtual Destructor
    virtual ~TFormula(void);

  // Operators
    TFormula& operator=(const TFormula& formula);
    bool operator==(const TFormula& formula) const;
    bool operator!=(const TFormula& formula) const;
    operator vector<TTagCountPair>*(void) const;
    operator EcceSortedVector<TTagCountPair, less<TTagCountPair> >*(void) const;

  // Modifiers

  // Accessors
    string formula(int charge=0) const;

  // General Utility Functions
    static string empiricalFormula(
                             const vector<TTagCountPair>& tc,
                             int charge=0);
    static string formula(const EcceSortedVector<TTagCountPair,
	less<TTagCountPair> >& tc,
                             int charge=0);
    static string formula(const vector<TTagCountPair>& tc,
                             int charge=0);
    static EcceSortedVector<TTagCountPair, less<TTagCountPair> >*
	                               parse_formula(const string& formula);
    static bool test_formula(const string& formula);

protected:

    static string chargeString(int charge);
    static string formatElement(const string& tag, int count);

private:

  // State
    string p_formula;

  // Encapsulated Behavior
    static void formulaInsert(vector<string>& ordered,
                              const string& tag);
    static bool after(const string& tag1, const string& tag2);



  // Friends
    friend ostream& operator<< (ostream& os, const TFormula& formula);
    friend istream& operator>> (istream& is, TFormula& formula);

}; // TFormula

#endif /* TFORMULA_HH */
