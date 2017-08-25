// HEADER FILENAME: TRefElement.H
//
//
// CLASS SYNOPSIS:
//   RefElements Objects Contain Periodic Table Reference Information.
//
// SEE ALSO:
//      
// DESCRIPTION:
//   Class TRefElement represents library-type reference information about
//   all known elements.  This not only includes common properties such
//   as boiling/melting points, but also connections to isotope information.
//   The collection of all RefElements form the Periodic Table (and is
//   supported by the PeriodicTable Class).
//
//   These are comments from a previous version of class Atom that has
//   been converted into two classes, RefElement and Atom.
//		The Atom class provides knowledge about atoms.  The 
//		knowledge includes basic periodic table information
//		plus some additional data to support computational
//		chemistry research.  
//
//		The current implementation was designed and built 
//		to support a persistent periodic table.  Because of
//		this, the emphasis in the interface is on getting
//		information about an atom.  However, the 
//		class should work in a stand-alone fashion.
//
//		Some atomic information is well defined - atomic
//		numbers and weights for example.  Other data, such
//		as covelent radii may have more than one published
//		value.  This current implementation does not support
//		named preferences at this time (but will).
//
//		All attributes of simple types (int, float, char[])
//		are public.  This design was chosen since many
//		attributes are expected to exist, they won`t be
//		changing, and modifier/selector functions for each
//		is overkill.  More complicated types or types
//		representing data that may change are private
//		with selector function.
//
// EXAMPLES:
//    testPeriodicTable.C
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TREFELEMENT_HH
#define TREFELEMENT_HH

#include <iostream>
  using std::ostream;

#include <string>
using std::string;

#include "util/Color.H"
#include "tdat/TPerTab.H"


class PeriodicTable;
class Color;

class TRefElement 
{

public:

  // Constructors
    // no user-allowed construction - this is reference data

  // Virtual Destructor
    virtual ~TRefElement(void);

  // Operators - identity based comparison
    bool operator==(const TRefElement& refelement) const;
    bool operator!=(const TRefElement& refelement) const;

  // Modifiers
    // This is Reference information so modifiers aren't
    // supported in a public interface.

  // Accessors
    string                atomicSymbol(void) const;
    int                      atomicNumber(void) const;
    int                      row(void) const;
    int                      column(void) const;
    TPerTab::ElementState state(void) const;
    TPerTab::ElementMetal metal(void) const;
    const Color&             color(void) const;
    string                name(void) const;
    string                electronAffinities(void) const;
    float                    atomicWeight(void) const;
    float                    density(void) const;
    float                    meltingPoint(void) const;
    float                    boilingPoint(void) const;
    float                    covalentRadius(void) const;
    float                    crTolerance(void) const;


protected:

  // Hidden/friend Constructor
    TRefElement(void);

  // Derived/Friend Class Modifiers
    void atomicSymbol(const string& newSymbol);
    void atomicNumber(int newNumber);
    void row(int newRow);
    void column(int newColumn);
    void state(TPerTab::ElementState newState);
    void metal(TPerTab::ElementMetal newMetal);
    void color(const Color& newColor);
    void name(const string& newName);
    void electronAffinities(const string& newElectronAffinities);
    void atomicWeight(float newAtomicWeight);
    void density(float newDensity);
    void meltingPoint(float newMeltingPoint);
    void boilingPoint(float newBoilingPoint);
    void covalentRadius(float value);
    void crTolerance(float value);

private:

   // State
   string                     p_atomicSymbol; // Atomic Symbol
   int                        p_atomicNumber; // Atomic Number
   int                        p_row;          // Row of Periodic Table
   int                        p_column;       // Column of Periodic Table
   TPerTab::ElementState      p_state;// Material State of Element
   TPerTab::ElementMetal      p_metal;// Metallic Properties
   Color                      p_color;     	 // Default display color
   string                     p_name;         // Full Name
   string                     p_electronAffinities;
   float                      p_atomicWeight; // Atomic Weight
   float                      p_density;	 // Atomic Density (kg m-3)
   float                      p_boilingPoint; // in K
   float                      p_meltingPoint; // in K
   float                      p_covalentRadius;
   float                      p_crTolerance;

  // Friends
    friend ostream& operator<< (ostream& os, const TRefElement& refElement);
    friend class TPerTab;

}; // TRefElement

#endif // TREFELEMENT_HH
