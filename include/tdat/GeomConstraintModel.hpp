/**
 * @file 
 *
 *
 */
#ifndef GEOMCONSTRAINTMODEL_HH
#define GEOMCONSTRAINTMODEL_HH

#include "util/Serializable.H"
#include "util/InvalidException.H"

#include "tdat/GeomConstraint.H"

#include <vector>

class Fragment;
class GeomConstraintRules;
class GeomConstraint;

/**
 * Class for building up a set of constraints on a fragment.
 * Constriants can really be three things:
 * <li>fixed atom constraints</li>
 * <li>bond,angle,torsion constraints</li>
 * <li>restraints on the above. This is implemented by a potential</li>
 * See the GeomConstraint class for more information.
 *
 * Constraints are used to constrain how a systems geometry can move
 * during an optimization.
 *
 * There are rules regarding how these can or cannot be used together.
 * This class enforces those rules while providing and interface to
 * build up/manage a set of constraints.  An exception is thrown if
 * a rule is violated during and addConstraint operation.
 */
class GeomConstraintModel : public Serializable
{

   public:

      GeomConstraintModel(const Fragment& frag, 
                          const GeomConstraintRules& rules);
      GeomConstraintModel(const GeomConstraintModel& rhs); 
      virtual ~GeomConstraintModel();

      GeomConstraintModel& operator=(const GeomConstraintModel& rhs);

      int size() const;
      void clear();

      int setFragment(const Fragment& frag);
      const Fragment& getFragment() const;

      void setRules(GeomConstraintRules *rules);
      const GeomConstraintRules& getRules() const;

      vector<GeomConstraint*> *getAtomConstraints() const;
      vector<GeomConstraint*> *getBondConstraints() const;
      vector<GeomConstraint*> *getAngleConstraints() const;
      vector<GeomConstraint*> *getTorsionConstraints() const;

      void addConstraint(GeomConstraint *constraint) throw(InvalidException);
      bool fixAtoms(const vector<int>& atoms) throw(InvalidException);
      bool addConstraint(const vector<int>& atoms) throw(InvalidException);
      bool addRestraint(const vector<int>& atoms) throw(InvalidException);
      bool remove(const GeomConstraint *constraint);
      bool removeLike(const GeomConstraint& constraint);

      void recomputeAll();

      void pretestRules(bool& fixedAtoms, 
                        bool& constraints, 
                        bool& restraints) const;

      int cleanUnmatchedAtoms();

      vector<TAtm*> *indicesToAtoms(const vector<int>& indices)  const
         throw (InvalidException);

   protected:

      vector<GeomConstraint*>::iterator find(const GeomConstraint *searchFor);
      void categorize(int& nFixedAtoms, int& nConstraints, 
                      int& nRestraints) const;
      vector<GeomConstraint*> *getOfType(int numAtoms) const;
      void remove(int index);

   private:

      /**  The set of constraints.  */
      vector<GeomConstraint*> p_constraints;

      /** * The system the user is operating on. */
      const Fragment *p_frag;

      /**
       * The logic rules that indicate which type of constraints can
       * be used together and/are supported.
       */
      GeomConstraintRules *p_rules;

};


#endif 
