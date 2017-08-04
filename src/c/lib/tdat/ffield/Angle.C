///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Angle.C
//
//
// DESIGN:
//              <Discuss overall design/purpose of class.>
//     (See Angle.H)
//
///////////////////////////////////////////////////////////////////////////////

// library includes
#include "util/ErrMsg.H"
#include "tdat/Angle.H"
#include "tdat/FFCoords.H"
// application includes

#define PI  3.14159265358979323846
#define RAD * 180.0 / PI
// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Construct an Angle given the indices of its constituant atoms.
//       Atom 2 is the central atom. The bond orders are sometimes useful in
//       assigning force field parameters so they are included in the Angle
//       class for easy access.
////////////////////////////////////////////////////////////////////////////// 
Angle::Angle( int iatom1, int iatom2, int iatom3,
	      double obond1, double obond2)
  : p_atom1Index(iatom1), p_atom2Index(iatom2), p_atom3Index(iatom3),
    p_bOrder1(obond1), p_bOrder2(obond2)
{
  p_params = (double*) 0;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Angle copy constructor.  (Uses assignment operator).
//////////////////////////////////////////////////////////////////////////////

Angle::Angle(Angle& angle)
{
  (*this) = angle;
}

 
// ---------- Virtual Destructor ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
////////////////////////////////////////////////////////////////////////////// 
Angle::~Angle(void)
{
  if (  p_params != (double*) 0 ) {
    delete[] p_params;
    p_params = (double*)0;
  }
}

// ------------ Modifiers ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Assign parameter values to angle
////////////////////////////////////////////////////////////////////////////// 

void Angle::setParams(double *params)
{ p_params = new double[5];
  for (int i=0; i<5; i++) p_params[i] = params[i];
}
// ------------ Accessors ------------
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Simple Accessors for Angle indices and bond orders
////////////////////////////////////////////////////////////////////////////////
int Angle::atom1Index(void) const
{ return p_atom1Index; }
int Angle::atom2Index(void) const
{ return p_atom2Index; }
int Angle::atom3Index(void) const
{ return p_atom3Index; }
double Angle::bondOrder1(void) const
{ return p_bOrder1; }
double Angle::bondOrder2(void) const
{ return p_bOrder2; }

////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Return cosine of the Angle. This function should probably be moved
//       into energy and forceVector methods for improved performance.
////////////////////////////////////////////////////////////////////////////////
double Angle::cosine(const double *c1, const double *c2, const double *c3) const
{
  double dot = 0.0;
  double vec1mag = 0.0;
  double vec2mag = 0.0;

  double delta1;
  double delta2;

  int ixyz;
  for (ixyz = 0; ixyz < 3; ixyz++) {
    delta1 = c1[ixyz] - c2[ixyz];
    delta2 = c3[ixyz] - c2[ixyz];
    vec1mag += delta1 * delta1;
    vec2mag += delta2 * delta2;
    dot += delta1 * delta2;
  }

  vec1mag = sqrt(vec1mag);
  vec2mag = sqrt(vec2mag);

  double result = dot / (vec1mag*vec2mag);
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Return sine of the Angle. This function should probably be moved into
//       the energy and forceVector methods for improved performance.
///////////////////////////////////////////////////////////////////////////////
double Angle::sine(const double *c1, const double *c2, const double *c3) const
{
  double dot = 0.0;
  double vec1mag = 0.0;
  double vec2mag = 0.0;

  double delta1;
  double delta2;

  int ixyz;
  for (ixyz = 0; ixyz < 3; ixyz++) {
    delta1 = c1[ixyz] - c2[ixyz];
    delta2 = c3[ixyz] - c2[ixyz];
    vec1mag += delta1 * delta1;
    vec2mag += delta2 * delta2;
    dot += delta1 * delta2;
  }

  vec1mag = sqrt(vec1mag);
  vec2mag = sqrt(vec2mag);

  double cosine = dot / (vec1mag*vec2mag);
  double result = sqrt(1.0 - cosine*cosine*ONEMINUS5EPS);

  return result;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Returns contribution to the energy (in kcal/mol) from this angle.
////////////////////////////////////////////////////////////////////////////// 
double Angle::energy(const double *c1, const double *c2, const double* c3)
{
  double cosTheta = cosine(c1, c2, c3);
  double thetaEq = p_params[4];
  double result;

  if( thetaEq == 180.0 ) {

    double forceConst = p_params[0];
    result = forceConst * (1.0 + cosTheta);   // n = 1

  } else if ( thetaEq == 120.0 ){

    double forceConst = p_params[0];
    double cos3Theta = 4.0*cosTheta*cosTheta*cosTheta - 3.0*cosTheta;
    result = forceConst/9.0 * (1.0 - cos3Theta);

  } else if ( thetaEq == 90.0 ){

    double forceConst = p_params[0];
    double cos2 = cosTheta * cosTheta;
    double cos4 = cos2 * cos2;
    double cos4Theta = 8.0*cos4 - 8.0*cos2 + 1.0;
    result = forceConst/16.0 * (1.0 - cos4Theta);

  } else {

    double forceConst = p_params[0];
    double c_0 = p_params[1];
    double c_1 = p_params[2];
    double c_2 = p_params[3];
    double cos2Theta = 2.0*cosTheta*cosTheta - 1.0;
    result = forceConst * ( c_0 + c_1*cosTheta + c_2*cos2Theta );

  }
  return result;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Returns the forces (in kcal/mole) force the atoms in this angle.
////////////////////////////////////////////////////////////////////////////// 
void Angle::forceVector(const double *c1, const double* c2,
       const double *c3, double *force)
{
  double scalar;
  double thetaEq = p_params[4];
  double sinTheta = sine(c1, c2, c3);

  if ( thetaEq == 180.0 ) {

    // Scalar force function for a bond angle 
    // with equilibrium value 180 deg.

    double forceConst = p_params[0];
    if (sinTheta > 0.000001) {
      scalar = forceConst * sinTheta;
    } else {
      scalar = forceConst;
    }
  } else if ( thetaEq == 120.0 ){

    // Scalar force function for a bond angle 
    // with equilibrium value 120 deg.

    double forceConst = p_params[0];
    double sin3Theta = 3.0*sinTheta - 4.0*sinTheta*sinTheta*sinTheta;
    if (sinTheta > 0.000001) {
      scalar = - forceConst / 3.0 * sin3Theta;
    } else {
      scalar = -forceConst;
    }

  } else if ( thetaEq == 90.0 ){

    // Scalar force function for a bond angle 
    // with equilibrium value 90 deg.

    double forceConst = p_params[0];
    double cosTheta = cosine(c1, c2, c3);
    double sin4Theta = 
           cosTheta * (4.0*sinTheta - 8.0*sinTheta*sinTheta*sinTheta);
    if (sinTheta > 0.000001) {
      scalar = - forceConst / 4.0 * sin4Theta;
    } else {
      scalar = -forceConst;
    }

  } else {

    // Scalar force function for a general bond angle 

    double forceConst = p_params[0];
    double c_1 = p_params[2];
    double c_2 = p_params[3];
    double cosTheta = cosine(c1, c2, c3);
    double sin2Theta = 2.0 * sinTheta * cosTheta;
    if (sinTheta > 0.000001) {
      scalar = forceConst * ( c_1 * sinTheta + 2.0 * c_2 * sin2Theta );
    } else {
      scalar = forceConst * (c_1 + 4.0 * c_2);
    }
  }

  double rn12, rn32, rn12_3, rn32_3;
  double r12[3], r32[3], dot;
  int i;
  rn12=0.0;
  rn32=0.0;
  dot=0.0;
  for (i=0; i<3; i++) { 
    r12[i] = c1[i] - c2[i];
    r32[i] = c3[i] - c2[i];
    rn12 += r12[i]*r12[i];
    rn32 += r32[i]*r32[i];
    dot += r12[i]*r32[i];
  }
  rn12 = sqrt(rn12);
  rn32 = sqrt(rn32);
  rn12_3 = rn12*rn12*rn12;
  rn32_3 = rn32*rn32*rn32;

  double dTheta1[3], dTheta2[3], dTheta3[3];
  for (i=0; i<3; i++) {
    dTheta1[i] = r32[i]/(rn12*rn32)-dot*r12[i]/(rn12_3*rn32);
    dTheta2[i] = (-r32[i]-r12[i])/(rn12*rn32)+dot*(r12[i]/(rn12_3*rn32)
               + r32[i]/(rn12*rn32_3));
    dTheta3[i] = r12[i]/(rn12*rn32)-dot*r32[i]/(rn12*rn32_3);
  }
  if (sinTheta > 0.000001) scalar = scalar/sinTheta;
  for (i=0; i<3; i++) {
    force[i] = -scalar*dTheta1[i];
    force[i+3] = -scalar*dTheta2[i];
    force[i+6] = -scalar*dTheta3[i];
  }
  return;
}
