///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: NonBond.H
//
//
// CLASS SYNOPSIS:
//      Class for evaluating non bonded interactions. Contains methods for
//      evaluating non-bonded pair list, determining third neighbors from
//      bond lists, and evaluating forces and energy.
//
// EXPORTED TYPES:
//    Class NonBond
//
// SEE ALSO:
//    ForceField.H
///////////////////////////////////////////////////////////////////////////////
#ifndef NONBOND_HH
#define NONBOND_HH

#include <vector>
#include <set>
  using std::vector;
  using std::set;
  using std::less;

class NonBond;
class FFCoordinates;
class TAtm;

class NonBond
{

public:

  // Constructors
        NonBond(vector<TAtm*>* atmptrs);
        NonBond(vector<TAtm*>* atmptrs, vector<bool>* mask,
	        bool halfatoms = false);

        NonBond(const NonBond&);

  // Virtual Destructor
        virtual ~NonBond(void);

  // General Methods
        void initExclusionList();
        void init3rdNeighborList();
        void getNeighborList(const FFCoordinates *coords, double cutoff);
        void setAtomParameters(void);
        void forces(const FFCoordinates* coords, FFCoordinates* result,
                    double cutoff);
        double energy(const FFCoordinates* coords, double cutoff);

private:

  // State
        vector<TAtm*>*               p_atmptrs;
        vector<bool>*                p_mask;
        vector<double>               p_epsln;
        vector<double>               p_sigma;
        vector<double>               p_charge;
        vector<int>                  p_ngbrList;
        vector<int>                  p_ngbrLast;
        vector<set<int,less<int> >*> p_exclsnList;
        vector<vector<int>*>         p_thrdNgbrList;
        FFCoordinates*               p_oldCoords;
	bool                         p_halfAtoms;
}; // NonBond

#endif /* NONBOND_HH */
