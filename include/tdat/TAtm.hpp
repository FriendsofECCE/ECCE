/**
 * @file
 *
 *
 */
#ifndef TATM_HH
#define TATM_HH

#include <iostream>
using std::ostream;

#include "tdat/ShapeData.H"
#include "tdat/DisplayStyle.H"


#include <string>
#include <vector>
using std::string;
using std::vector;

class TBond;
class Residue;


/**
 * Class TAtm contains data associated with an individual atom.
 *
 * TAtms contain a state that is expected to change on an atom by atom 
 * basis.  This includes attributes specified by the PDB format,
 * MD attributes, and a host of miscelleneous attributes.  Even if 
 * attributes are not used, they aer kept so that we can preserve the
 * full data set when writing data back out.
 *
 * Attributes that are common to all Atms of a 
 * given element or isotope are accessable through a lookup in the 
 * Periodic Table and Chart of the Nuclides reference databases. 
 */
class TAtm 
{

   public:

      enum LinkType {INTERIOR, NEGATIVE, POSITIVE, CROSSLINK};

      /**
       * An attribute that defines the role an atom has within
       * a system.  This is used for mixed theory calculation and
       * is a preliminary list.
       */
      enum BehaviorType {UNSPECIFIED, QUANTUM, POINT, BQ, BQX };

      // Constructors
      TAtm();                                 // create ghost atom
      TAtm(short atomicNumber, const double* coord = (const double*)0,
            const string& suffix = "");
      TAtm(const char *atomicNumber, const double* coord = (const double*)0,
            const string& suffix = "");
      TAtm(const TAtm& atom);
      virtual ~TAtm();

      // Operators
      TAtm& operator=(const TAtm& atom);                   // Assignment
      bool operator==(const TAtm& atom) const;       // contents
      bool operator!=(const TAtm& atom) const;       // contents
      virtual TAtm* clone() const;

      // For making hash tables and so forth.
      static unsigned long hFun(const TAtm& a);

      // For parsing strings to get atom parameters
      static void parseString(string input, string &name, int &atom_index,
	                      string &parent_name, int &parent_pdb_idx,
			      char &parent_chain, int &parent_idx);

      // Maybe there should be a behavior class.  For now use statics
      static string behaviorTypeToString(TAtm::BehaviorType type);
      static TAtm::BehaviorType stringToBehaviorType(const string& type);

      // Accessors
      string        displayLabel() const;
      string        suffixHack() const;
      string        atomName(void) const;
      char*         cAtomName(void) const;     // return atom name with no blanks
      int              index(void) const;
      string        atomType(void) const;
      char*         cAtomType(void) const;     // return atom type with no blanks
      string        atomicSymbol(void) const;
      bool          isNub(void) const;
      int              atomicNumber(void) const;
      double           partialCharge(void) const;
      short            assembly(void) const;
      short            chargeGroup(void) const;
      short            polarizationGroup(void) const;
      int              connectionIndex(void) const;
      ShapeData::Shape       shape(void) const;
      string      shapeString(void) const;
      const double*    coordinates(void) const; // Return array of three elements
      char             chain() const;
      char             icode() const;
      string        residueName() const;
      int              residueNumber() const;
      string        segment() const;
      Residue*         getResidue() const;
      float            occupancy() const;
      float            bFactor() const;
      DisplayStyle     displayStyle() const;
      float            polarizability() const;
      short            dihedralIndex(void) const;
      double           getMass() const;
      BehaviorType     getBehaviorType() const;

      // Bond Information
      const vector<TBond*>&      bondList(void) const;
      vector<TAtm*>* connectedAtoms(void) const;
      void       connectedAtoms(vector<TAtm*>& result,
            const char* atomicSymbol = 0) const;
      double     angleBetween(TAtm* atom1, TAtm* atom2);
      double     distanceFrom(TAtm* atom);

      // Modifiers
      void displayLabel(const string& label);
      void suffixHack(const string& suffix);
      void index(const int& value);
      void atomicSymbol(const string& symbol);
      void atomicNumber(short atomicNumber);
      void atomName(const char *name);
      void atomType(const char *name);
      void assembly(short value);
      void occupancy(const float& value);
      void polarizability(const float& value);
      void bFactor(const float& value);
      void partialCharge(const double& partialCharge);
      void chargeGroup(short cgNum);
      void polarizationGroup(short pgNum);
      void connectionIndex(short cIndx);
      void connectionIndex(int cIndx);
      void atomResidue(Residue* residue);
      void shape(ShapeData::Shape shape);
      void shapeString(const string& shapeStr);
      void coordinates(const double* coords);  // Pass in array of three doubles
      void setMass(double mass);
      void coordinates(const double& x, const double& y, const double& z);  
      void displayStyle( const DisplayStyle& ds );
      void shallowCopy(const TAtm* atm);
      void dihedralIndex(const short dIndx);
      void setDisplayed(const bool flag);
      void setBehaviorType(TAtm::BehaviorType type);
      void setMostAbundantIsotopeMass(int atomicNumber);
      void setMostAbundantIsotopeMass(string atomicSymbol);

      void copyResidueInfo(TAtm* src);
      string getHydrogenName(void);
      void cleanAtom(void);

      // Bond-Atom Relationship Maintenance
      void removeBond(const TBond* bond);
      void removeBonds(void);
      void addBond(const TBond* bond);

      // Builder Query Operations
      bool traverse(vector<TAtm*>& visited,
            const TAtm* lastAtom,
            const TAtm* firstAtom) const;   // Result = Found Cycle


   protected:

      void assignDefaultCoordinates();

   private:

      /** the atom index - kept for performance reasons. */
      int       p_index;

      /** the atom name in the PDB sense. */
      char      p_atomName[5];

      /** type in teh MD sense */
      char      p_atomType[5];

      /** xyz coordinates */
      double    p_coords[3];

      /** the atomic number for this atom (in the periodic table sense) */
      int       p_atomicNumber;

      /** the element symbol */
      /** @todo derive this? */
      char      p_atomicSymbol[4];

      /** the label to display; used to support selective display */
      string    p_displayLabel;

      /** unique suffix from XYZ file for NWChem - hack for BQ atoms */
      string    p_suffixHack;       

      /** user customizable mass */
      double     p_mass;

      // It might be better to store this as a string and avoid continuous
      // conversions between enumerations and shapes.  The tradeoff is
      // space associated with strings.
      ShapeData::Shape p_shape;
      int       p_dihedralIndex;

      float     p_occupancy;

      /** pdb field preserved for writing pdb but not used. */
      float     p_Bfactor;

      double     p_partialCharge;
      DisplayStyle p_displayStyle;
      float     p_polarizability;
      short     p_assembly;
      short     p_chargeGroup;
      short     p_polarizationGroup;
      short     p_connectionIndex;

      /** the residue this atom belongs to; 0 pointer for simple structures */
      Residue*  p_residue;

      /** See BehaviorType description. */
      BehaviorType p_behavior;

      /** The bonds for this atom. */
      vector<TBond*> p_bonds;

      friend ostream& operator<< (ostream& os, const TAtm& atom);
}; 


#endif
