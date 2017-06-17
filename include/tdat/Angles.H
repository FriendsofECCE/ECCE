#ifndef ANGLES_HH
#define ANGLES_HH

#include <float.h> // DBL_EPSILON

#include <math.h>  // fabs()

#include <vector>
#include <list>
  using std::vector;
  using std::list;


class Angle;
class FFCoordinates;
class TAtm;
class FFRules;
// used in expressions like sqrt(1-a*a*ONEMINUS5EPS) to avoid sqrt(-DBL_EPSILON)
#define ONEMINUS5EPS (1.0 - 5.0*DBL_EPSILON)

/** 
 * Angle represents the collection of all angles in a system, evaluate
 * contribution to the total energy and forces from all angles,
 * initialize list of all angles.
 */
class Angles
{
   public:

      // Constructors
      Angles(vector<TAtm*>* atmptrs, FFRules *rules);
      Angles(vector<TAtm*>* atmptrs, vector<bool>* mask, FFRules *rules);
      Angles(Angles&);

      // Virtual Destructor
      virtual ~Angles(void);

      // General Methods
      void initAngleList(void);
      void setAngleParameters(void);
      void forces(const FFCoordinates *coords, FFCoordinates *results);
      double energy(const FFCoordinates *coords);

      // Accessors
      int   getCount( void) const;
      string getName( const int);
      string getType( const int);
      int*  getAtomOrderedIndices( const int);
      void  getAtomIndices( const int, int*, int*, int*);

      static const char BOND_LINK;

   protected:
      vector<TAtm*>*   p_atmptrs;
      vector<bool>*    p_mask;
      vector<Angle*>   p_angleList;
      FFRules*         p_rules;

      bool isValidIndex( const int) const;

}; // Angles

#endif /* ANGLES_HH */
