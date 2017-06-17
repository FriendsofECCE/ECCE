///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PBond.C
//
//
// DESIGN:
//   
//
///////////////////////////////////////////////////////////////////////////////

// system includes
#include <math.h>
// application includes
#include "util/ErrMsg.H"
#include "tdat/PBond.H"

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------

//////////////////////////////////////////////////////////////////////////////
//
//  Description:  
//       Create a PBond from two atom indices.  
////////////////////////////////////////////////////////////////////////////// 
PBond::PBond( int atom1Index, int atom2Index)
{
  EE_ASSERT( atom1Index != atom2Index, EE_FATAL, "atom1 same as atom2")
  if (atom1Index < atom2Index) {
    p_atom1Index = atom1Index;
    p_atom2Index = atom2Index;
  } else {
    p_atom1Index = atom2Index;
    p_atom2Index = atom1Index;
  }
  p_params = 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Copy constructor.  Uses assignment operator.
////////////////////////////////////////////////////////////////////////////// 
PBond::PBond(const PBond& bond)
{ (*this) = bond; }
 
// ---------- Virtual Destructor ------------
PBond::~PBond(void)
{
  if (p_params != (double*)0) {
    delete[] p_params;
    p_params = (double*)0;
  }
}

// ------------ Modifiers ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Assign parameters for harmonic bond
//////////////////////////////////////////////////////////////////////////////
void PBond::setParams(double r12, double forceConst)
{ p_params = new double[2];
  p_params[equillength] = r12;
  p_params[forceconst] = forceConst;
}
// ------------ Accessors ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Simple accessors to get atom indices
////////////////////////////////////////////////////////////////////////////// 
int PBond::atom1Index(void) const
{ return p_atom1Index; }

int PBond::atom2Index(void) const
{ return p_atom2Index; }

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Harmonic oscillator scalar energy function:
//       E = 1/2 * ForceConstant * ( R - Req )^2
//       where R and Req are the actual and equilibrium bond lengths.
////////////////////////////////////////////////////////////////////////////// 
double PBond::energy(const double *c1, const double *c2) const
{ 
  double tmp=0.0, diff[3];
  int i;
  for (i=0; i<3; i++) {
    diff[i] = c1[i] - c2[i];
    tmp += diff[i]*diff[i];
  }
  tmp = sqrt(tmp);
  return 0.5 * p_params[forceconst] * (p_params[equillength] - tmp)
                *  (p_params[equillength] - tmp);
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//       Returns the contribution to the forces from this bond
////////////////////////////////////////////////////////////////////////////// 
void PBond::forceVector(const double *c1, const double *c2, double* force)
{

  double length = 0.0, diff[3];
  int i;
  for (i=0; i<3; i++) {
    diff[i] = c1[i] - c2[i];
    length += diff[i]*diff[i];
  }
  length = sqrt(length);

  // calculate force
  double du = p_params[forceconst]*
                 (p_params[equillength] - length);

  //  factor of 2 is cancelled by factor of 1/2 in potential

  force[0] = (diff[0])/length * du;
  force[1] = (diff[1])/length * du;
  force[2] = (diff[2])/length * du;

  force[3] = -force[0];
  force[4] = -force[1];
  force[5] = -force[2];

  return;
}

