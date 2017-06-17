#ifndef INVERSIONS_HH
#define INVERSIONS_HH

#include <vector>
  using std::vector;

class Inversion;
class FFCoordinates;
class TAtm;
class FFRules;

/**
 * Represents the collection of all inversion angles in the system.
 * Initializes the inversion angle list, calculates total energy and forces
 * from all inversions.
 */
class Inversions
{

   public:

      // Constructors
      Inversions(vector<TAtm*>* atmptrs, FFRules* rules);
      Inversions(vector<TAtm*>* atmptrs, vector<bool>* mask, FFRules* rules);
      Inversions(vector<TAtm*>* atmptrs, FFRules* rules, vector<int>* atomRings);

      Inversions(const Inversions&);

      // Virtual Destructor
      virtual ~Inversions(void);

      // General Methods
      void   initInversionList(void);
      void   setInversionParameters(void);
      void   forces(const FFCoordinates* coords, FFCoordinates* result);
      double energy(const FFCoordinates* coords);

      // Accessors
      int    getCount( void) 	const;
      string getName( const int);
      string getType( const int);
      int*   getAtomOrderedIndices( const int);
      void   getAtomIndices( const int, int*, int*, int*, int*);

   private:
      bool isValidIndex(const int idx) const;

      // State
      vector<TAtm*>*       p_atmptrs;
      vector<bool>*        p_mask;
      vector<int>*         p_rings;
      vector<Inversion*>   p_inversionList;
      FFRules*             p_rules;

}; // Inversions

#endif /* INVERSIONS_HH */
