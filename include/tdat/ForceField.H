///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: ForceField.H
//
//
// CLASS SYNOPSIS:
//
//     Provides "clean" functionality (geometry optimization) for
//     a Fragment (for now, only for non-periodic systems).
//
// EXPORTED TYPES:
//    Class ForceField
//
// LOCAL TYPES:
//    //<place locally used classes,typedefs here>
//
// SEE ALSO:
//      Class PBond
//      Class Angle
//      Class Torsion
//      Class Inversion
//      Template Class SDMinimizer
//      Template Class CGMinimizer
//      Class UFFParams
//
// DESCRIPTION:
//     Provides a container holding the list of molecular bonds, angles
//     torsions, and inversions (improper dihedrals) as well as the
//     nonbonded neighbor list and exclusion list.
//
//     The UFFParams class provides parameters and scalar force and energy
//     functions for the so-called "Universal ForceField" of
//     Rappe' and co-workers (J. Am. Chem. Soc. 1992, v114, p10024-10035)
//     For now, only molecular systems are
//     supported; R&G have published extensions, including one for metals, 
//     that might be included later.  
//
// TO DO:
//     *   Interpolate missing GMP electronegativities from other published
//         electronegativities.  (R&G have never released some of the values)
//     *   consider eliminating stored copy of coordinates (typical use just
//         modifies passed-in copies, anyway).  probably would need to store
//         number of atoms.
///////////////////////////////////////////////////////////////////////////////
#ifndef FORCEFIELD_HH
#define FORCEFIELD_HH

#include <stdio.h>

#include <vector>
  using std::vector;

#include "util/CancelException.H"
#include "util/ETimer.H"

#include "tdat/Fragment.H"
#include "tdat/FFCoords.H"

class PBonds;
class Angles;
class Torsions;
class Inversions;
class NonBond;
class UFFParams;
class FFRules;
class ProgressMonitor;

////////////////////////////////////////////////////////////////////////
// default values of convergence parameters for optimize()  function  
//
// NOTE : all defaults for the optimize function are now placed
//        where the optimize function is declared below.
////////////////////////////////////////////////////////////////////////
//
// Max number of Steepest (SD) steps to take
//const static int defaultFFNSDSteps = 0;
//
// Max number of Conjugate-Gradient (CG) steps to take
// IN CGMINIMIZER SET TO 30
//const static int defaultFFNCGSteps = 40;                 
//
// force norm convergence tolerance appropriate (units of kcal/mol/Angstrom)
//const static double  defaultFFConvForceNorm = 0.02;
//
// max step range; units are the same as inverse force constant below
//IN CGMINIMIZER SET TO 1.e-2
//const static double  defaultFFLineSearchRange = 1.e-1;   
//
// default scale factor to multiply the force vector to get a step vector
// in Angstroms; the units are thus Angstrom^2/kcal.  These are the inverse
// of the units of force constant; this value must be smaller than any
// eigenvalue of the second derivative (Hessian) matrix for convergence.
//const static double  defaultFFSDInvForceConst = 1.e-4;   
//const static double  defaultFFSDAtomMaxStep = 5.e-1;   
//
// max number of steps in CG linesearch
//const static int defaultFFMaxCGLineSteps = 200;     
//
// conjugate gradient linesearch energy convergence tolerance 
// (units of kcal/mol)
//const static double  defaultFFLineTol = 1.e-12;

////////////////////////////////////////////////////////////////////////
// misc. used values
////////////////////////////////////////////////////////////////////////

// Coulomb interaction cutoff range (Angstrom)
const static double  defaultCoulCutoff = 10.0;

// for estimation of the average number of other atoms within 
// defaultCoulCutoff of an atom (for initial list sizes),  use this
// along with the Coulomb cutoff range.  (density in atoms/Angstrom^3)
// 0.1 with a 10.0 A cutoff ==> 
const static double maxAvgAtomDensity = 0.01;

// within this range, atoms in the coordinates passed to the optimize() method
// are randomly moved apart.
const static double minAllowedRange = 0.3;

// Scale factor to multiply random x, y, z shifts of overlapping atoms
const static double moveScale = 1.0;

////////////////////////////////////////////////////////////////////////////// 

class ForceField
{

public:

  // Constructors
  ForceField(Fragment* chemsys, FFRules *rules,
             ProgressMonitor* = (ProgressMonitor*)0);
  ForceField(Fragment* chemsys, vector<int>* selection, FFRules *rules,
             ProgressMonitor* = (ProgressMonitor*)0);
  ForceField(const ForceField& forceField);

  // Virtual Destructor
  virtual ~ForceField(void);

  // ---- Accessors ---- //
  Fragment*            chemSys(void) const;
  const FFCoordinates* coordinates(void) const;

        int     getCount( int) 		const;
        string  getAtomSymbol( int) 	const; 
        int     getAtomicNumber( int) 	const; 
  const string  getName( int, int) 	const;
  const string  getType( int, int) 	const;
        void    getComponentIndices( int, int, vector<int>&, vector<int>&);
        void    getComponentIndices( int, vector<int>&, vector<int>&, 
                                                                 vector<int>&);

        void    printIndices( int, const char*);  // for debug

  // ---- Calculations ---- //
  void optimize(            int  nCGSteps=40,
                            double convForceNormAU=0.02,
                            double nonBondCutoff=100.0,
                            double lineSearchRange=1.e-1,
                            int  maxCGLineSteps=200,
                            double lineTol=2.0e-4 ) throw (CancelException);

  void moveOverLappingAtoms( FFCoordinates* coords );
  bool checkInterrupt();

  double energy(bool& canceled, const FFCoordinates* coords);
  FFCoordinates* force(bool& canceled, const FFCoordinates* coords,
                  FFCoordinates* force=(FFCoordinates*)0);
  void fdCheck(const FFCoordinates *coords, FFCoordinates *result,
               double cutoff);

private:

  Fragment*      p_chemsys;
  vector<TAtm*>* p_atmptrs;
  FFCoordinates* p_coordinates;
  PBonds*        p_bonds;
  Angles*        p_angles;
  Torsions*      p_torsions;
  Inversions*    p_inversions;
  NonBond*       p_nonbond;
  vector<bool>   p_mask;
  FFRules*       p_rules;
  ProgressMonitor *p_monitor;
  ETimer         p_timer;
  float          p_interval;

  // cutoff range for nonbond forces
  double         p_nonBondCutoff;

  bool isValidID( int, int) const;
  void pushAtom( int, vector<int>&);
  void pushBond( int, vector<int>&, vector<int>&);
  void pushAngle( int, vector<int>&, vector<int>&);
  void pushProper( int, vector<int>&, vector<int>&);
  void pushInversion( int, vector<int>&, vector<int>&);
  void cleanup();

}; // ForceField

#endif /* FORCEFIELD_HH */
