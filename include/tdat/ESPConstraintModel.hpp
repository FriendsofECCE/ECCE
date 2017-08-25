///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: ESPConstraintModel.H
//
//
// CLASS SYNOPSIS:
//    Class ESPConstraintModel contains data associated with a set of ESP
//    constraints for an NWChem ESP calculation and the method to manipulate
//    the set of constraints.
//
// DESCRIPTION:
//    This class is a fairly lightweight set of operations on a list (vector)
//    of ESPConstraint objects.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef ESPCONSTRAINTMODEL_HH
#define ESPCONSTRAINTMODEL_HH

#include "util/IndexOutOfRangeException.H"

#include "util/Serializable.H"
#include "tdat/ESPConstraint.H"


#include <vector>

class Fragment;

class ESPConstraintModel : public Serializable
{

public:

    ESPConstraintModel(const Fragment& frag);
    virtual ~ESPConstraintModel();

    /** Get the number of constraints. */
    /** WARNING: switching all the ints to size_t broke the old builder so
        be very careful when cleaning up compiler warnings from this class.
        GDB 1/6/2009 */
    int size() const;

    /** remove all constraints. */
    void clear();

    /** associate fragment with this model. Automatically clean up groups. */
    int setFragment(const Fragment& frag);

    /** Returns fragment currently in use by the class.  */
    const Fragment& getFragment() const;

    /** Get the nth constraint. */
    ESPConstraint *get(int index) const throw (IndexOutOfRangeException);

    /** Create a new constraint of the specified type. */
    void add(ESPConstraint::Type type);

    /** Add constraint. */
    void add(ESPConstraint *constraint);

    /** Automatically create constraints for all methyls. */
    int generateMethylConstraints();

    /** Automatically create constraints for all Hydrogens. */
    int generateHydrogenConstraints();

    /** Generate fixed constraints (using partial charges) for the atoms. */
    int generateFixedConstraints(const vector<int>& atoms);

    /** remove n'th constraint. */
    bool remove(int index) throw (IndexOutOfRangeException);

    /** Compute bitmap of unconstrained atoms using Fragment. */
    std::vector<bool> unconstrained();

    /** Compute bitmap of multiply constrained atoms using Fragment. */
    std::vector<bool> multipleeConstrained();

    /** Check if a Methyl constraint for the specified atom exists. */
    bool methylConstraintExists(const TAtm& atom) const;

    /** Find and remove constraints with unmatched atoms - using Fragment. */
    int cleanUnmatchedAtoms();

    vector<TAtm*> indicesToGroup(const vector<int>& indices)  const;

private:
    std::vector<ESPConstraint*> p_constraints;
    const Fragment *p_frag;


}; 


#endif
