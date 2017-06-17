///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: CGMinimizer.cc
//
//
// DESIGN:
//    Polak-Ribiere conjugate gradient algorithm: see CGMinimizer.h
//              <Discuss overall design/purpose of class.>
//    Note that the Force vector (-gradient) is used, rather than the 
//    gradient, as would be more conventional.  
//
///////////////////////////////////////////////////////////////////////////////

#include <math.h>

#include <iostream>
  using std::cout;
  using std::endl;

#include "util/ErrMsg.H"
#include "tdat/CGMinimizer.H"

#define ZEPS 1.0e-10
#define CGOLD 0.3819660
#define GOLD 1.618034
#define TINY 1.0e-20
#define GLIMIT 100.0
#define CG_RANGE 1.0
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

//#define STEEPESTDECENT

// some verbose output
// #define DEBUG

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//       Constructor from forcefield and coordinate pointers.  The coordinates
//       are modified in place, not copied.
//
////////////////////////////////////////////////////////////////////////////// 
CGMinimizer::CGMinimizer(const ObjectiveFunction* func, FFCoordinates* coords)
  : p_coords(coords)
{
  // check for null pointers
  EE_ASSERT((func != (ObjectiveFunction*)0), EE_FATAL,
       "Null pointer for objective function");
  EE_ASSERT((coords != (FFCoordinates*)0), EE_FATAL,
       "Null pointer for initial coords");

  p_objectiveFunction = func;
  p_searchVec = new FFCoordinates(*coords);// working array with class scope
}
 
// ---------- Destructor ------------
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Destructor. 
//
////////////////////////////////////////////////////////////////////////////// 
CGMinimizer::~CGMinimizer(void)
{
  if ( p_searchVec != (FFCoordinates*) 0) {
    delete p_searchVec;
    p_searchVec   = (FFCoordinates*) 0;
  }
}

// ---------- General Methods  ----------
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//     Optimize geometry.  When algorithm details settle down, some 
//     parameters will be removed and some "magic numbers" will be added
//     to the interface.
////////////////////////////////////////////////////////////////////////////// 
FFCoordinates* CGMinimizer::optimize( double convForceNorm,
                               int maxSteps,
                               double lineSearchRange,
                               double lineTol,
                               int maxLineSteps)  
{
  bool canceled = false;
  ////////////////////////////////////////////////////////////////////////
  // allocate (automatic) FFCoordinates to hold forces and search vector
  ////////////////////////////////////////////////////////////////////////
  FFCoordinates tmp1(*p_coords); FFCoordinates *force1 = &tmp1;
  FFCoordinates tmp2(*p_coords); FFCoordinates *force2 = &tmp2;
  FFCoordinates tmp3(*p_coords); FFCoordinates *uTemp  = &tmp3;

  ////////////////////////////////////////////////////////////////////////
  // initialization.  get force vector at current coordinates 
  ////////////////////////////////////////////////////////////////////////
  force1 = p_objectiveFunction->getGradient(canceled, p_coords,force1);
  if (canceled) throw CancelException(WHERE);

  *p_searchVec = *force1; // copies elements; no allocation

  double forcenorm = force1->maxRowNorm();

#if (!defined(INSTALL) && defined(DEBUG))
  cout << " CGMinimizer::optimize using parameters: " << endl;
    cout << " new searchvec = " << endl;
    p_searchVec->print(cout,5);

  cout << "\n  convForceNorm: " << convForceNorm
       << "  maxSteps: " <<   maxSteps << endl;
  cout << "  lineSearchRange: " <<   lineSearchRange
       << "  lineTol: " << lineTol << endl;
  cout << "  maxLineSteps: " <<   maxLineSteps
       << "  forcenorm: " << forcenorm << endl;
#endif 

  double lower_bound,upper_bound,initial_guess;

  int istep;
  for (istep=0; istep<maxSteps; istep++ ) {
    if (forcenorm < convForceNorm) {
#if (!defined(INSTALL) && defined(DEBUG))
      cout << "CGMinimizer converged after " <<istep<<" interations "<<endl;
#endif
      break;
    }

    ////////////////////////////////////////////////////////////////////////
    // set new coordinates to line minimum of energy along p_searchVec 
    ////////////////////////////////////////////////////////////////////////

#if (!defined(INSTALL) && defined(DEBUG))
    cout << "calling fminbr: range,lineTol: " << range << "  "
	 << lineTol << endl;
#endif 

#ifdef STEEPESTDECENT
    if ((forcenorm - firstCGNorm) < 1 || istep >= 40) {
#endif
      lower_bound=0.0;
      upper_bound=1.0;
      // if upper_bound will result in too large and atom movement then
      // scale it back
      if (forcenorm*upper_bound > 2.0) upper_bound = 2.0/forcenorm;

      initial_guess=upper_bound*CGOLD;

      shift_limits(&lower_bound,&initial_guess,&upper_bound);

      double x = fminbr(lower_bound,initial_guess,upper_bound,
                        lineTol,maxLineSteps);
    
      ////////////////////////////////////////////////////////////////////////
      // search with a greater range if needed
      ////////////////////////////////////////////////////////////////////////
      while ( x/upper_bound > 0.99 ) {
        upper_bound *= 2.0;
        // Put in check to halt runaway searches for systems where atoms
        // start out too close together
        if (funAlongLine(x) == funAlongLine(upper_bound)) break;
#if (!defined(INSTALL) && defined(DEBUG))
        cout << "doubling linesearch range " << endl;
        cout << "(2) calling fminbr: range,lineTol: "
  	   << range << "   " << lineTol << endl;
#endif 
        lower_bound=0.0;
        initial_guess=CGOLD*upper_bound;
        shift_limits(&lower_bound,&initial_guess,&upper_bound);
        x = fminbr(lower_bound,initial_guess,upper_bound,lineTol,maxLineSteps);
      }

      *p_coords = *p_coords->add(*p_searchVec,x);

#ifdef STEEPESTDECENT
    } else {
      *p_coords = *p_coords->add(*p_searchVec,1.e-4);
    }
#endif

    ////////////////////////////////////////////////////////////////////////
    // calculate force vector at new coordinates 
    ////////////////////////////////////////////////////////////////////////
    force2 = p_objectiveFunction->getGradient(canceled, p_coords,force2);
    if (canceled) throw CancelException(WHERE);

#if (!defined(INSTALL) && defined(DEBUG))
    cout << "CGMinimizer:  optimize: x = " << x
	 << "  maxrange: " << range << endl;
    cout << " new forcvec = " << endl;
    force2->print(cout,5);
    cout << " norm(searchvec): " << p_searchVec->norm();
    cout << " steplength: " << p_searchVec->norm()*x << endl;
    cout << " energy: " <<  p_objectiveFunction->getValue(p_coords);
    cout << " new forcenorm: " <<  force2->maxRowNorm() << endl;
    if ( x > 0.99* range ) {
      cout << "possible too-short linesearch range " << endl;
    }
#endif
    
    ////////////////////////////////////////////////////////////////////////
    // check for convergence 
    ////////////////////////////////////////////////////////////////////////
    forcenorm = force2->maxRowNorm();

    if (forcenorm < convForceNorm)
      break;

    ////////////////////////////////////////////////////////////////////////
    // This is the smarts of the algorithm:
    // construct new search vector for linesearch, using
    // p_searchVec = force2 + gamma p_searchVec, where
    // gamma = [(force2 - force1) dot force2]/[force1 dot force1] 
    //
    // Note that this uses forces, not gradients (differs in sign), but 
    // this sign cancels in gamma.
    // 
    ////////////////////////////////////////////////////////////////////////

#ifdef STEEPESTDECENT
    if ((forcenorm - firstCGNorm) < 1 || istep >= 40) {
#endif
      *uTemp = *force2;  // copies elements... no allocation
      double gamma = uTemp->add(*force1,-1.0)->dotdot(*force2) /
      force1->dotdot(*force1);
      p_searchVec->scalarMult(gamma); 
      p_searchVec->add(*force2);
#ifdef STEEPESTDECENT
    } else {
      p_searchVec = force2;
    }
#endif

    ////////////////////////////////////////////////////////////////////////
    // done with force1, switch pointers force1 and force2 
    ////////////////////////////////////////////////////////////////////////
    FFCoordinates* tempPointer;
    tempPointer = force1;
    force1 = force2;
    force2 = tempPointer;

  }
  return p_coords;
}

// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Derived Class Accessors ----------



// ------------------------
// Private Member Functions
// ------------------------
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Evaluates energy value at a displacement x * p_searchVec from the
//      current geometry.
//
////////////////////////////////////////////////////////////////////////////// 
double CGMinimizer::funAlongLine(double x) const
{
  FFCoordinates coordTemp(*p_coords);

#if (!defined(INSTALL) && defined(DEBUG))
    cout << " funAlongLine reference coords:"  << endl;
    coordTemp.print(cout,5);
#endif  

  coordTemp.add(*p_searchVec,x);

#if (!defined(INSTALL) && defined(DEBUG))
    cout << " funAlongLine shifted coords for x = " << x << endl;
    coordTemp.print(cout,5);
#endif  

  bool canceled = false;
  double result = p_objectiveFunction->getValue(canceled,&coordTemp);
  if (canceled) throw CancelException(WHERE);
  return result;
}
//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description: fminbr was obtained from netlib.  It has been hacked 
//  slightly to avoid includes for machine precision, etc, that are not 
//  needed here.
//
//  Implementation:
//      
//
////////////////////////////////////////////////////////////////////////////// 
 /************************************************************************
 *
 *			  Numerical Math Package
 *
 *		    Brent's one-dimensional minimizer 
 *
 *	     finds a local minimum of a single argument function
 *			  over the given range
 *
 * Input
 *	double fminbr(ax,bx,f,tol,maxLineSteps)
 *	const double ax		        a and b, a < b, specify the interval
 *	const double bx  		the minimum is to be sought in
 *	double (*f)(const double x)	Ptr to the function under investigation
 *	const double tol		Acceptable tolerance for the minimum
 *					location. It is an optional parameter
 *					with default value EPSILON
 *
 * Output
 *	Fminbr returns an estimate to the minimum location with accuracy
 *	3*SQRT_EPSILON*abs(x) + tol.
 *	The procedure always determines a local minimum, which coincides with
 *	the global one if and only if the function under investigation is
 *	unimodular.
 *	If a function being examined possesses no local minimum within
 *	the given range, Fminbr returns either the left or the right end
 *	point of the interval, wherever the function value is smaller.
 *
 * Algorithm
 *	G.Forsythe, M.Malcolm, C.Moler, Computer methods for mathematical
 *	computations. M., Mir, 1980, p.202 of the Russian edition
 *
 * The function makes use of the "gold section" procedure combined with
 * the parabolic interpolation.
 * At every step program operates three abscissae - x,v, and w.
 * 	x - the last and the best approximation to the minimum location,
 *		i.e. f(x) <= f(a) or/and f(x) <= f(b)
 * 	    (if the function f has a local minimum in (a,b), then both
 *  	     conditions are met after one or two steps).
 *	v,w are previous approximations to the minimum location. They may
 *	coincide with a, b, or x (although the algorithm tries to make all
 *	u, v, and w distinct). 
 * Points x, v, and w are used to construct an interpolating parabola,
 * whose minimum is to be regarded as a new approximation to the minimum
 * location if the former falls within [a,b] and reduces the minimum 
 * encompassing interval to a larger extent than the gold section procedure.
 * When f(x) has a second derivative positive at the point of minimum
 * (not coinciding with a or b) the procedure converges superlinearly
 * at a rate of about 1.324
 *
 ************************************************************************
 */

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Modifications:
//       Uses class function funAlongLine instead of passed-in function pointer.
//       tolerances hard-wired (to eliminate includes).  These can be put
//       into another header file or made modifiable if needed.  Also,
//       all register variables have been made ordinary variables.  HT
//
////////////////////////////////////////////////////////////////////////////// 

double CGMinimizer::fminbr(             // An estimate to the min location
        const double ax,                // Specify the interval the minimum
        double bx,
        const double cx,                // to be sought in
        const double tol,               // Acceptable tolerance;
        const int maxLineSteps)
{

#if (!defined(INSTALL) && defined(DEBUG))
  cout << "linesearch: ax " << ax << " bx " << bx << " cx " << cx << endl;
#endif

  double a,b,etemp,fu,fv,fw,fx,p,q,r,tol1,tol2,u,v,w,x,xm;
  double d=0.0; // no need for an initial value because it is assigned 
                // a value before it is used  but it gets rid of a
                // warning message during compile time...stupid reason I guess
  double e=0.0;

  a=(ax < cx ? ax : cx);
  b=(ax > cx ? ax : cx);

  w=v=x=bx;
  fw=fv=fx=funAlongLine(x);       // First step - always gold section

  for(int istep=0; istep<maxLineSteps;istep++)  // Main iteration loop
  {
    //If a and b are sufficiently close together,
    //then system is converged
    xm = (a+b)/2;
    tol2 = 2.0*(tol1=(tol)*fabs(x) + ZEPS);

    if( fabs(x-xm) <= (tol2 - 0.5 * (b-a)) ) {

#if (!defined(INSTALL) && defined(DEBUG))
      cout << "fminbr converged in " << istep << " steps, to x tol: "
           << tol1 << endl << endl;
#endif

      return x;

    }

    // Decide on the interpolation  
    if( fabs(e) > tol1 )
    {          // interpolation may be tried
               // Interpolation step is calculated
               // as p/q; division operation
               // is delayed until last moment
      r = (x-w) * (fx-fv);
      q = (x-v) * (fx-fw);
      p = (x-v)*q - (x-w)*r;
      q = 2*(q-r);
      if (q > 0.0) p = -p;

      q = fabs(q);
      etemp=e;
      e=d;
      if(fabs(p) >= fabs(0.5*q*etemp) ||
         p<=q*(a-x) ||
         p>=q*(b-x)) {
        d=CGOLD*(e=(x >= xm ? a-x : b-x));
      } else {
        d=p/q;
        u=x+d;
        if(u-a < tol2 || b-u < tol2) {
          d=SIGN(tol1,xm-x);
        }
      }
    } else {
      d=CGOLD*(e=(x >= xm ? a-x : b-x));
    }
    u=(fabs(d) >= tol1 ? x+d : x+SIGN(tol1,d));
    fu = funAlongLine(u);

#if (!defined(INSTALL) && defined(DEBUG))
    cout << " fminbr E(2): " << fv;
#endif

    if( fu <= fx )
    {                                 // t is a better approximation
      if( u < x )                     // Reduce the interval so that
        b = x;                        // t would fall within it
      else
        a = x;
      
      v = w;  w = x;  x = u;          // Assign the best approx to x
      fv=fw;  fw=fx;  fx=fu;
    }
    else                              // x remains the better approx
    {
      if( u < x )                     // Reduce the interval encompassing x
        a = u;
      else
        b = u;

      if( fu <= fw || w==x )
      {
        v = w;  w = u;
        fv=fw;  fw=fu;
      }
      else if( fu<=fv || v==x || v==w )
        v = u, fv = fu;
    }
  }// ===== End of loop =====

#if (!defined(INSTALL) && defined(DEBUG))
  cout << "CGMinimizer: fminbr not converged" << endl;
#endif

  return x; // added by HT; was infinite loop
} // end of fminbr(v)


void CGMinimizer::shift_limits(double* ax, double* bx, double* cx) {


  double ulim, u, r, q, fu, dum;
  double fa = funAlongLine(*ax);
  double fb = funAlongLine(*bx);

  if (fb > fa) {
    dum = *ax; *ax = *bx; *bx = dum;
    dum = fb; fb = fa; fa = dum;
  }
  *cx = (*bx)+GOLD*(*bx-*ax);
  double fc = funAlongLine(*cx);

  while(fb > fc) {
    r=(*bx-*ax)*(fb-fc);
    q=(*bx-*cx)*(fb-fa);
    u=(*bx)-((*bx-*cx)*q-(*bx-*ax)*r) / (2.0*SIGN(MAX(fabs(q-r),TINY),q-r));

    ulim=(*bx)+GLIMIT*(*cx-*bx);
    if((*bx-u)*(u-*cx) > 0.0) {
      fu=funAlongLine(u);
      if(fu < fc) {
        *ax=(*bx);
        *bx=u;
        fa=fb;
        fb=fu;
        return;
      } else if(fu > fb) {
        *cx=u;
        fc=fu;
        return;
      }
      u=(*cx)+GOLD*(*cx-*bx);
      fu=funAlongLine(u);
    } else if((*cx-u)*(u-ulim) > 0.0) {
      fu=funAlongLine(u);
      if(fu < fc) {
        *bx = *cx; *cx = u; u = *cx + GOLD * (*cx-*bx);
        fb = fc; fc = fu; fu = funAlongLine(u);
      }
    } else if((u-ulim)*(ulim-*cx) >= 0.0) {
      u=ulim;
      fu=funAlongLine(u);
    } else {
      u=(*cx)+GOLD*(*cx-*bx);
      fu=funAlongLine(u);
    }
    *ax = *bx; *bx = *cx; *cx = u;
    fa = fb; fb = fc; fc = fu;
  }
}
