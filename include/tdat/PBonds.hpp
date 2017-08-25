///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PBonds.H
//
//
// CLASS SYNOPSIS:
//
//      Represents all bonds in a forcefield
//
// EXPORTED TYPES:
//    Class PBonds
//
///////////////////////////////////////////////////////////////////////////////
#ifndef PBONDS_HH
#define PBONDS_HH

#include <vector>
#include <list>
#include <string>
  using std::vector;
  using std::list;
  using std::string;


#include "tdat/TBond.H"
#include "tdat/TAtm.H"
#include "tdat/Fragment.H"
#include "tdat/FFCoords.H"

// forward references
class PBonds;
class PBond;
class FFRules;

class PBonds
{

public:

   static const char* BOND_LINK;
   static const char* NUB_STRING;  // should be more universally defined TODO
   enum STRING_TYPE { BOND_NAME, BOND_TYPE};

  // Constructors
       PBonds( vector<TAtm*>* atmptrs, FFRules *rules);
       PBonds( vector<TAtm*>* atmptrs, vector<bool>* mask, FFRules *rules);
       PBonds(const PBonds& bonds);

  // Virtual Destructor
       virtual ~PBonds(void);

  // Modifiers (change to evaluations)
       void initBondList(void);
       void setBondParameters(void);

       double energy(const FFCoordinates* coords) const;
       void forces(const FFCoordinates* coords, FFCoordinates* result);

   // Accessors
       int    getCount( void) const;
       string getString( const int, const STRING_TYPE);
       int*   getAtomOrderedIndices( const int) const;
       void   getAtomIndices( const int, int*, int*);

private:

  // cached atom pointers
       vector<TAtm*>*           p_atmptrs;
       vector<PBond*>           p_bondList;
       vector<bool>*            p_mask;
       FFRules*                 p_rules;

       bool isValidIndex( const int) const;

}; // PBonds

#endif /* PBONDS_HH */
