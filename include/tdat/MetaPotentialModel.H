/**
 * @file 
 *
 *
 */
#ifndef METAPOTENTIALMODEL_HH
#define METAPOTENTIALMODEL_HH

#include "util/Serializable.H"
#include "util/InvalidException.H"

#include "tdat/MetaPotential.H"

#include <vector>

class Fragment;
class MetaPotential;

/**
 * Class for building up a set of meta-dynamics potentials on a fragment.
 * Meta-dynamics potentials consist of a list of atoms that define the potential
 * and a list of parameters associated with the potential. See the MetaPotential
 * class for more information.
 */
class MetaPotentialModel : public Serializable
{

  public:

    MetaPotentialModel(const Fragment& frag);
    MetaPotentialModel(const MetaPotentialModel& rhs); 
    virtual ~MetaPotentialModel();

    MetaPotentialModel& operator=(const MetaPotentialModel& rhs);

    int size() const;
    void clear();

    int setFragment(const Fragment& frag);
    int cleanUnmatchedAtoms();
    const Fragment& getFragment() const;

    vector<MetaPotential*> *get2SitePotentials() const;
    vector<MetaPotential*> *get3SitePotentials() const;

    void addPotential(MetaPotential *mpot) throw(InvalidException);
    bool addPotential(const vector<int>& atoms) throw(InvalidException);
    bool remove(const MetaPotential *mpot);

    // Do we need this one?
    //   bool removeLike(const MetaPotential& mpot);

  protected:

    vector<MetaPotential*>::iterator find(const MetaPotential *searchFor);
    //  void categorize(int& nFixedAtoms, int& nConstraints, 
    //                  int& nRestraints) const;
    //  vector<MetaPotential*> *getOfType(int numAtoms) const;
    void remove(int index);

  private:

    /**  The set of meta-dynamics potentials.  */
    vector<MetaPotential*> p_mpotential;

    /** * The system the user is operating on. */
    const Fragment *p_frag;
};


#endif 
