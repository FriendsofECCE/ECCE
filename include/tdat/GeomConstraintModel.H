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

      /// Run types whose input uses the constraints; for any other they are
      /// kept with the structure but left out of the input (#306).
      static bool usedByRunType(const std::string& runtype)
      { return runtype == "Geometry" || runtype == "GeoVib"; }

      /// Whether the code's input takes stored constraints and scans for a
      /// run type.  ECCE-QM also leaves them out of GeoVib, whose
      /// frequencies would be at a constrained geometry.
      enum Use { USED, IGNORED };
      static Use useBy(const std::string& code, const std::string& runtype)
      {
        if (code == "ECCE-QM" && runtype == "GeoVib") return IGNORED;
        return usedByRunType(runtype) ? USED : IGNORED;
      }

      /// "not used by an Energy run", with the reason where ECCE-QM has one.
      static std::string notUsedText(const std::string& code,
                                     const std::string& runtype)
      {
        if (code == "ECCE-QM" && runtype == "GeoVib")
          return "not used by a GeoVib run (frequencies need an "
                 "unconstrained minimum)";
        if (code == "ECCE-QM" && runtype == "Transition State (Opt-TS)")
          return "not used by a transition-state search";
        const bool vowel = !runtype.empty() &&
            std::string("AEIOUaeiou").find(runtype[0]) != std::string::npos;
        return std::string("not used by ") + (vowel ? "an " : "a ") +
               runtype + " run";
      }

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

      void addConstraint(GeomConstraint *constraint);
      bool fixAtoms(const vector<int>& atoms);
      bool addConstraint(const vector<int>& atoms);
      bool addRestraint(const vector<int>& atoms);
      bool remove(const GeomConstraint *constraint);
      bool removeLike(const GeomConstraint& constraint);

      void recomputeAll();

      void pretestRules(bool& fixedAtoms, 
                        bool& constraints, 
                        bool& restraints) const;

      int cleanUnmatchedAtoms();

      vector<TAtm*> *indicesToAtoms(const vector<int>& indices)  const
;

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
