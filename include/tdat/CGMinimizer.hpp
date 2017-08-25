///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: CGMinimizer.h
//
//
// CLASS SYNOPSIS:
//    Performs Polak-Ribiere conjugate gradient minimization of an arbitrary 
//    function (see Numerical Recipes, Chap. 10) given an initial guess vector, 
//    a pointer to a function, and a pointer to its gradient function.
//    Template arguments (T is the class to be minimized; U is a matrix or 
//    vector class):
//        U:  a matrix or vector class (Matrix<double>) supporting:
//                EUInt32 rows = u.rows(void);
//                EUInt32 columns = u.columns(void);
//                U* u3 = u1.add(U& u2,double x);
//                void u.print(ostream os,EUInt32 precision);
//                double u.norm(void)
//                double u.dotdot(U& u2)
//
//        T:  (ForceField) class with
//          double t.energy(U* u1); }
//          U*     t.force(U* u1)
//
//    The state of this class includes current and previous force vectors and
//    the conjugate search vector (g_{i}, g_{i+1}, and h_{i},  in the 
//    notation of Numerical Recipes), plus the convergence tolerance and 
//    maximum number of optimization steps to perform.
//    The copy of coordinates that is passed to this class is actually 
//    modified in place, rather than constructing a new() copy on return
//    from minimize();
//    this was thought to be more versatile, since a new copy may be 
//    readily constructed outside.
//    The linesearch is a standard Brent (see Numerical Recipes).  The code
//    is slightly modified from that placed in the NetLib repository by
//    Oleg Kiselyov <oleg@ponder.csci.unt.edu, oleg@unt.edu>    
// EXPORTED TYPES:
//    Class CGMinimizer
//
// LOCAL TYPES:
//    //<place locally used classes,typedefs here>
//
// SEE ALSO:
//      
// DESCRIPTION:
//    Variable names and notation: force1 and force2 correspond to the
//    the vectors g_{i} and g_{i+1} in the notation of Numerical Recipes;
//    searchvec is the conjugate gradient search vector h_{i}
//
// EXAMPLES:
//     In the test directory .../src/db/ff, see
//     the program ForceField.C.
//
// To Do:
//     More accessor/modifer pairs when parameters established.
//     Add linesearch with derivatives
//     Add parameter values to optimize() arglist
//
///////////////////////////////////////////////////////////////////////////////
#ifndef CGMINIMIZER_HH
#define CGMINIMIZER_HH

#include "tdat/FFCoords.H"
#include "tdat/ForceField.H"
#include "tdat/ObjectiveFunction.H"

////////////////////////////////////////////////////////////////////////////// 
// Defaults (magic numbers) chosen for the problem at hand in units
//  of kcal/mol and Angstroms.  
//
////////////////////////////////////////////////////////////////////////////// 

// use steepest descent until largest gradient component smaller than:
const double firstCGNorm = 1.e-1;  

class CGMinimizer {

public:
  // Constructor
  CGMinimizer(const ObjectiveFunction* func, FFCoordinates* coords=0);

  // copy constructor
  CGMinimizer(const CGMinimizer& cgmin);

  // Virtual Destructor
  ~CGMinimizer(void);

  // General Methods
  FFCoordinates* optimize( double convForceNorm = 1.e-2,
	                   int maxSteps = 30,
	                   double lineSearchRange = 1.e-2,
	                   double lineTol = 2.0e-4,
	                   int maxLineSteps = 200);

protected:

private:

  const ObjectiveFunction* p_objectiveFunction;
  FFCoordinates*    p_coords;     // coordinate vector
  FFCoordinates*    p_searchVec;  // this is given class scope, so that
                                  // the imported linesearch routine does 
                                  // not need to pass the searchVector 
                                  // through to the energy evaluation.

  // linesearch function (based on value only; not gradient)
  double fminbr(const double a, double xmin, const double b, const double tol, 
                const int maxLineSteps);
  void shift_limits(double* ax, double* bx,double* cx);

  // value of function displaced by x along the vector p_searchVec 
  // relative to p_coords
  double funAlongLine(double x) const;

};

#endif /* CGMINIMIZER_HH */
 
