/**
 * @file 
 *
 *
 */
#ifndef GEOMCONSTRAINTRULES_HH
#define GEOMCONSTRAINTRULES_HH


/**
 * Class that contains rules about what types and combinations of geometry
 * constraints are allowed.  These rules are expected to vary for each
 * code.  The rules are intended to be used by the GeomConstraintModel
 * but are pulled out into a separate class for clean separation.
 *
 * The types of constraints/restraints a code may support is based somewhat
 * on rules associated with whether they are specified in cartesian
 * or Z coordinates because of the properties of each coordinate system.
 * The following table shows likely scenario based on
 * the fact that Z matrix is by definition relative and so fixing coordinates
 * doesn't really make sense while constraints (usually specified as Z matrix) 
 * can't be used with Cartesian coordinates.  Rules can of course vary by
 * code and Gaussian in particular is quite sophisticated about providing
 * user control over optimization.
 *
 * <code>
 *                   | Cartesian   |   Z Matrix
 *                   |   used      |     used
 *       Fixed Atoms |     +       |      -
 *       Constraints |     -       |      +
 *       Restraints  |     +       |      +?
 * </code>
 *
 * If a rule returns true, it is implied that at least one of the 
 * coordinate systems supports the operation and the code generator will
 * pick the appropriate one.
 */
class GeomConstraintRules
{

   public:

      GeomConstraintRules();
      GeomConstraintRules(const GeomConstraintRules& rhs);
      virtual ~GeomConstraintRules();

      // Accessors
      bool supportsAtomConstraints() const;
      bool supportsConstrainBonds() const;
      bool supportsConstrainAngles() const;
      bool supportsConstrainTorsions() const;

      bool supportsRestrainBonds() const;
      bool supportsRestrainAngles() const;
      bool supportsRestrainTorsions() const;

      bool supportsFixedAtomsAndConstraints() const;
      bool supportsFixedAtomsAndRestraints() const;
      bool supportsMixedConstraintsAndRestraints() const;


      // Modifiers
      void setSupportsAtomConstraints(bool isSupported);
      void setSupportsConstrainBonds(bool isSupported);
      void setSupportsConstrainAngles(bool isSupported);
      void setSupportsConstrainTorsions(bool isSupported);

      void setSupportsRestrainBonds(bool isSupported);
      void setSupportsRestrainAngles(bool isSupported);
      void setSupportsRestrainTorsions(bool isSupported);

      void setSupportsFixedAtomsAndConstraints(bool isSupported);
      void setSupportsFixedAtomsAndRestraints(bool isSupported);
      void setSupportsMixedConstraintsAndRestraints(bool isSupported);

   private:

      bool p_supportsAtomConstraints;

      bool p_supportsConstrainBonds;
      bool p_supportsConstrainAngles;
      bool p_supportsConstrainTorsions;

      bool p_supportsRestrainBonds;
      bool p_supportsRestrainAngles;
      bool p_supportsRestrainTorsions;

      bool p_supportsFixedAtomsAndConstraints;
      bool p_supportsFixedAtomsAndRestraints;
      bool p_supportsMixedConstraintsAndRestraints;

};


#endif 
