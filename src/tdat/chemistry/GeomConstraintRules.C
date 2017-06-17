/**
 * @file
 *
 *
 */
#include "tdat/GeomConstraintRules.H"

/**
 * Constructor.
 */
GeomConstraintRules::GeomConstraintRules()
{
   // Set to nwchem defaults for lack of a better choice
   setSupportsAtomConstraints(true);
   setSupportsConstrainBonds(true);
   setSupportsConstrainAngles(true);
   setSupportsConstrainTorsions(true);
   setSupportsRestrainBonds(true);
   setSupportsRestrainAngles(false);
   setSupportsRestrainTorsions(false);
   setSupportsFixedAtomsAndConstraints(false);
   setSupportsFixedAtomsAndRestraints(true);
   setSupportsMixedConstraintsAndRestraints(false);

}



/**
 * Copy Constructor.
 */
GeomConstraintRules::GeomConstraintRules(const GeomConstraintRules& rhs)
{
   setSupportsAtomConstraints( rhs.supportsAtomConstraints() );

   setSupportsConstrainBonds( rhs.supportsConstrainBonds() );

   setSupportsConstrainAngles( rhs.supportsConstrainAngles() );
   
   setSupportsConstrainTorsions( rhs.supportsConstrainTorsions() );

   setSupportsRestrainBonds( rhs.supportsRestrainBonds() );

   setSupportsRestrainAngles( rhs.supportsRestrainAngles() );
   
   setSupportsRestrainTorsions( rhs.supportsRestrainTorsions() );

   setSupportsFixedAtomsAndConstraints(
             rhs.supportsFixedAtomsAndConstraints() );

   setSupportsFixedAtomsAndRestraints(
             rhs.supportsFixedAtomsAndRestraints() );

   setSupportsMixedConstraintsAndRestraints( 
             rhs.supportsMixedConstraintsAndRestraints() );

}



/**
 * Destructor.
 */
GeomConstraintRules::~GeomConstraintRules()
{
}



bool GeomConstraintRules::supportsAtomConstraints() const
{
   return p_supportsAtomConstraints;
}



bool GeomConstraintRules::supportsConstrainBonds() const
{
   return p_supportsConstrainBonds;
}



bool GeomConstraintRules::supportsConstrainAngles() const
{
   return p_supportsConstrainAngles;
}



bool GeomConstraintRules::supportsConstrainTorsions() const
{
   return p_supportsConstrainTorsions;
}



bool GeomConstraintRules::supportsRestrainBonds() const
{
   return p_supportsRestrainBonds;
}



bool GeomConstraintRules::supportsRestrainAngles() const
{
   return p_supportsRestrainAngles;
}



bool GeomConstraintRules::supportsRestrainTorsions() const
{
   return p_supportsRestrainTorsions;
}



bool GeomConstraintRules::supportsFixedAtomsAndConstraints() const
{
   return p_supportsFixedAtomsAndConstraints;
}



bool GeomConstraintRules::supportsFixedAtomsAndRestraints() const
{
   return p_supportsFixedAtomsAndRestraints;
}



bool GeomConstraintRules::supportsMixedConstraintsAndRestraints() const
{
   return p_supportsMixedConstraintsAndRestraints;
}



void GeomConstraintRules::setSupportsAtomConstraints(bool isSupported)
{
   p_supportsAtomConstraints = isSupported;
}



void GeomConstraintRules::setSupportsConstrainBonds(bool isSupported)
{
   p_supportsConstrainBonds = isSupported;
}



void GeomConstraintRules::setSupportsConstrainAngles(bool isSupported)
{
   p_supportsConstrainAngles = isSupported;
}



void GeomConstraintRules::setSupportsConstrainTorsions(bool isSupported)
{
   p_supportsConstrainTorsions = isSupported;
}



void GeomConstraintRules::setSupportsRestrainBonds(bool isSupported)
{
   p_supportsRestrainBonds = isSupported;
}



void GeomConstraintRules::setSupportsRestrainAngles(bool isSupported)
{
   p_supportsRestrainAngles = isSupported;
}



void GeomConstraintRules::setSupportsRestrainTorsions(bool isSupported)
{
   p_supportsRestrainTorsions = isSupported;
}



void GeomConstraintRules::setSupportsFixedAtomsAndConstraints(
                                                         bool isSupported)
{
   p_supportsFixedAtomsAndConstraints = isSupported;
}



void GeomConstraintRules::setSupportsFixedAtomsAndRestraints(
                                                         bool isSupported)
{
   p_supportsFixedAtomsAndRestraints = isSupported;
}



void GeomConstraintRules::setSupportsMixedConstraintsAndRestraints(
                                                         bool isSupported)
{
   p_supportsMixedConstraintsAndRestraints = isSupported;
}
