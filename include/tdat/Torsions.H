///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Torsions.H
//
//
// CLASS SYNOPSIS:
//      Represents the collection of all torsion angles in the system,
//      initiatializes torsion list, evaluates energies and forces due to
//      torsions.
//
// EXPORTED TYPES:
//    Class Torsions
//
// SEE ALSO:
//    ForceField.H
//    Torsion.H     
///////////////////////////////////////////////////////////////////////////////
#ifndef TORSIONS_HH
#define TORSIONS_HH

#include <vector>
  using std::vector;

class Torsion;
class FFCoordinates;
class TAtm;
class FFRules;

class Torsions
{

public:

   // Constructors
      Torsions(vector<TAtm*>* atmptrs, FFRules *rules);
      Torsions(vector<TAtm*>* atmptrs, vector<bool>* mask, FFRules *rules);

      Torsions(const Torsions&);

   // Virtual Destructor
      virtual ~Torsions(void);

   // General Methods
      void   initTorsionList(void);
      void   setTorsionParameters(void);
      void   forces(const FFCoordinates* coords, FFCoordinates* result);
      double energy(const FFCoordinates* coords);

   // Accessors
      int    getCount( void) const;
      string getName( const int);
      string getType( const int);
      int*   getAtomOrderedIndices( const int);
      void   getAtomIndices( const int, int*, int*, int*, int*);

private:
      bool isValidIndex(const int idx) const;

   // State
      vector<TAtm*>*     p_atmptrs;
      vector<bool>*      p_mask;
      vector<Torsion*>   p_torsionList;
      FFRules*           p_rules;

}; // Torsions

#endif /* TORSIONS_HH */
