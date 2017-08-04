///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Inversion.C
//
//
// DESIGN:
//              <Discuss overall design/purpose of class.>
//   (See Inversion.H)
//
///////////////////////////////////////////////////////////////////////////////

// application includes
#include "util/ErrMsg.H"
#include "tdat/Inversion.H"
#include "tdat/TAtm.H"
#include "tdat/FFCoords.H"

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Construct the Inversion. Atom 1 is the central atom, atoms 2, 3, and
//      4 are the surrounding atoms.
////////////////////////////////////////////////////////////////////////////// 
Inversion::Inversion( int iatom1, int iatom2,
                      int iatom3, int iatom4,
                      double obond21, double obond31, double obond41) :
  p_atom1Index(iatom1), p_atom2Index(iatom2), p_atom3Index(iatom3),
  p_atom4Index(iatom4), p_bOrder21(obond21), p_bOrder31(obond31),
  p_bOrder41(obond41)
{
  p_params = (double*)0;
}
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Inversion copy constructor.  Uses assignment operator.
////////////////////////////////////////////////////////////////////////////// 
Inversion::Inversion(const Inversion& torsion)
{ (*this) = torsion; }

 
// ---------- Virtual Destructor ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Simple destructor.
////////////////////////////////////////////////////////////////////////////// 
Inversion::~Inversion(void)
{
  if (p_params != (double*)0) {
    delete[] p_params;
    p_params = (double*)0;
  }
}


// ------------ Accessors ------------
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Return the atom indices of current Inversion.
////////////////////////////////////////////////////////////////////////////////
int Inversion::atom1Index(void) const
{ return p_atom1Index; }

int Inversion::atom2Index(void) const
{ return p_atom2Index; }

int Inversion::atom3Index(void) const
{ return p_atom3Index; }

int Inversion::atom4Index(void) const
{ return p_atom4Index; }
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Return the bond orders of the current Inversion.
////////////////////////////////////////////////////////////////////////////////
double Inversion::bond21Order(void) const
{ return p_bOrder21; }
double Inversion::bond31Order(void) const
{ return p_bOrder31; }
double Inversion::bond41Order(void) const
{ return p_bOrder41; }

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Set inversion potential parameters.
////////////////////////////////////////////////////////////////////////////// 
void Inversion::setParams(double *params)
{ 
  p_params = new double[4];
  for (int i=0; i<4; i++) p_params[i] = params[i];
  return;
}


// ---------- General Methods  ----------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Returns the contribution to the energy (in kcal/mol) from this 
//       inversion.
////////////////////////////////////////////////////////////////////////////// 
double Inversion::energy(const double *c1, const double *c2,
                         const double *c3, const double *c4)
{
  // evaluate cosine(Gamma);
  double r21[3],r31[3],r41[3];
  double r21x31[3];
  int i;
  double result = 0.0;

  for (i=0; i<3; i++) {
    r21[i] = c2[i] - c1[i];
    r31[i] = c3[i] - c1[i];
    r41[i] = c4[i] - c1[i];
  }
  // evaluate cross product of r21 x r31
  r21x31[0] = r21[1]*r31[2]-r31[1]*r21[2];
  r21x31[1] = r21[2]*r31[0]-r31[2]*r21[0];
  r21x31[2] = r21[0]*r31[1]-r31[0]*r21[1];

  double dotx41 = 0.0;
  double rnx = 0.0;
  double rn41 = 0.0;
  for (i=0; i<3; i++) {
    dotx41 += r21x31[i]*r41[i];
    rnx += r21x31[i]*r21x31[i];
    rn41 += r41[i]*r41[i];
  }
  rnx = sqrt(rnx);
  rn41 = sqrt(rn41);

  // bail out and return zero energy if we run into a degenerate angle
  if (rnx < 1.e-9 || rn41 < 1.e-9) return result;

  double cosGamma = dotx41/(rnx*rn41);
  double sinGamma = sqrt(1.0-cosGamma*cosGamma*ONEMINUS5EPS);

  double forceConst = p_params[0];
  double c_0 = p_params[1];
  double c_1 = p_params[2];
  double c_2 = p_params[3];
  double cos2Gamma = 2.0 * cosGamma * cosGamma - 1.0;
  result = forceConst * (c_0 + c_1*sinGamma - c_2*cos2Gamma);
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Returns the forces (in kcal/mol) from this inversion.
///////////////////////////////////////////////////////////////////////////////
void Inversion::forceVector(const double *c1, const double *c2,
                            const double *c3, const double *c4,
                            double* force)
{

  // evaluate gradients of cosine(Gamma);
  double r21[3],r31[3],r41[3];
  double r21x31[3];
  double g1r21x31[3][3], g2r21x31[3][3], g3r21x31[3][3];
  double ident[3][3];
  int i,j;

  for (i=0; i<12; i++) force[i] = 0.0;
  for (i=0; i<3; i++) {
    r21[i] = c2[i] - c1[i];
    r31[i] = c3[i] - c1[i];
    r41[i] = c4[i] - c1[i];
  }
  // evaluate cross product of r21 x r31
  r21x31[0] = r21[1]*r31[2]-r31[1]*r21[2];
  r21x31[1] = r21[2]*r31[0]-r31[2]*r21[0];
  r21x31[2] = r21[0]*r31[1]-r31[0]*r21[1];

  // evaluate gradients of cross product r21 x r31
  g1r21x31[0][0] =  0.0;
  g1r21x31[0][1] = -r21[2]+r31[2];
  g1r21x31[0][2] = -r31[1]+r21[1];
  g1r21x31[1][0] = -r31[2]+r21[2];
  g1r21x31[1][1] = 0.0;
  g1r21x31[1][2] = -r21[0]+r31[0];
  g1r21x31[2][0] = -r21[1]+r31[1];
  g1r21x31[2][1] = -r31[0]+r21[0];
  g1r21x31[2][2] =  0.0;

  g2r21x31[0][0] =  0.0;
  g2r21x31[0][1] = -r31[2];
  g2r21x31[0][2] =  r31[1];
  g2r21x31[1][0] =  r31[2];
  g2r21x31[1][1] =  0.0;
  g2r21x31[1][2] = -r31[0];
  g2r21x31[2][0] = -r31[1];
  g2r21x31[2][1] =  r31[0];
  g2r21x31[2][2] =  0.0;

  g3r21x31[0][0] =  0.0;
  g3r21x31[0][1] =  r21[2];
  g3r21x31[0][2] = -r21[1];
  g3r21x31[1][0] = -r21[2];
  g3r21x31[1][1] =  0.0;
  g3r21x31[1][2] =  r21[0];
  g3r21x31[2][0] =  r21[1];
  g3r21x31[2][1] = -r21[0];
  g3r21x31[2][2] =  0.0;

  double dotx41 = 0.0;
  double rnx = 0.0;
  double rn41 = 0.0;
  for (i=0; i<3; i++) {
    dotx41 += r21x31[i]*r41[i];
    rnx += r21x31[i]*r21x31[i];
    rn41 += r41[i]*r41[i];
    for (j=0; j<3; j++) {
      if (i==j) ident[i][j]=1.0;
      else ident[i][j]=0.0;
    }
  }
  rnx = sqrt(rnx);
  rn41 = sqrt(rn41);

  // bail out and return zero force if we run into a degenerate angle
  if (rnx < 1.e-9 || rn41 < 1.e-9) return;

  double cosGamma = dotx41/(rnx*rn41);
  double ri41 = 1.0/(rn41);
  double ri41_3 = ri41*ri41*ri41;
  double rix = 1.0/(rnx);
  double rix_3 = rix*rix*rix;
  double dGamma1[3],dGamma2[3],dGamma3[3],dGamma4[3];
  for (i=0; i<3; i++) {
    dGamma1[i] = 0.0;
    dGamma2[i] = 0.0;
    dGamma3[i] = 0.0;
    dGamma4[i] = 0.0;
    for (j=0; j<3; j++) {
      dGamma1[i] = dGamma1[i] + rix*ri41*g1r21x31[i][j]*r41[j]
                 - dotx41*rix_3*ri41*g1r21x31[i][j]*r21x31[j]
                 - rix*ri41*ident[i][j]*r21x31[j]
                 + rix*ri41_3*dotx41*ident[i][j]*r41[j];
      dGamma2[i] = dGamma2[i] + rix*ri41*g2r21x31[i][j]*r41[j]
                 - dotx41*rix_3*ri41*g2r21x31[i][j]*r21x31[j];
      dGamma3[i] = dGamma3[i] + rix*ri41*g3r21x31[i][j]*r41[j]
                 - dotx41*rix_3*ri41*g3r21x31[i][j]*r21x31[j];
      dGamma4[i] = dGamma4[i] + rix*ri41*ident[i][j]*r21x31[j]
                 - rix*ri41_3*dotx41*ident[i][j]*r41[j];
    }
  }
  double sinGamma = sqrt(1.0-cosGamma*cosGamma*ONEMINUS5EPS);

  double forceConst = p_params[0];
  double c_1 = p_params[2];
  double c_2 = p_params[3];
  double sin2Gamma = 2.0 * sinGamma * cosGamma;
  double scalar = forceConst * (-c_1*cosGamma - 2.0*c_2*sin2Gamma);
  scalar = scalar/sinGamma;

  for (i=0; i<3; i++) {
    force[i]   = scalar*dGamma1[i];
    force[i+3] = scalar*dGamma2[i];
    force[i+6] = scalar*dGamma3[i];
    force[i+9] = scalar*dGamma4[i];
  }
  return;
}
