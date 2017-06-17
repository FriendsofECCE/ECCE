///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Torsion.C
//
//
// DESIGN:
//              <Discuss overall design/purpose of class.>
//   (See Torsion.H)
//
///////////////////////////////////////////////////////////////////////////////

// application includes
#include "util/ErrMsg.H"
#include "tdat/Torsion.H"
#include "tdat/TAtm.H"
#include "tdat/FFCoords.H"

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Construct the Torsion. Atoms 2 and 3 represent the central bond, atoms
//      1 and 4 are the atoms attached at either end of the central bond.
////////////////////////////////////////////////////////////////////////////// 
Torsion::Torsion( int iatom1, int iatom2,
                  int iatom3, int iatom4,
                  double obond12, double obond32, double obond43) :
  p_atom1Index(iatom1), p_atom2Index(iatom2), p_atom3Index(iatom3),
  p_atom4Index(iatom4), p_bOrder12(obond12), p_bOrder32(obond32),
  p_bOrder43(obond43)
{
  p_params = (double*)0;
}
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Torsion copy constructor.  Uses assignment operator.
////////////////////////////////////////////////////////////////////////////// 
Torsion::Torsion(const Torsion& torsion)
{ (*this) = torsion; }

 
// ---------- Virtual Destructor ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Simple destructor.
////////////////////////////////////////////////////////////////////////////// 
Torsion::~Torsion(void)
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
//       Simple accessors for returning atom indices.
////////////////////////////////////////////////////////////////////////////////
int Torsion::atom1Index(void) const
{ return p_atom1Index; }

int Torsion::atom2Index(void) const
{ return p_atom2Index; }

int Torsion::atom3Index(void) const
{ return p_atom3Index; }

int Torsion::atom4Index(void) const
{ return p_atom4Index; }
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Return the bond order of the current Torsion.
////////////////////////////////////////////////////////////////////////////////
double Torsion::bond12Order(void) const
{ return p_bOrder12; }
double Torsion::bond32Order(void) const
{ return p_bOrder32; }
double Torsion::bond43Order(void) const
{ return p_bOrder43; }

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Set torsion potential parameters.
////////////////////////////////////////////////////////////////////////////// 
void Torsion::setParams(double *params)
{ 
  p_params = new double[3];
  for (int i=0; i<3; i++) p_params[i] = params[i];
  return;
}


// ---------- General Methods  ----------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Returns the contribution to the energy (in kcal/mol) from this 
//       torsion.
////////////////////////////////////////////////////////////////////////////// 
double Torsion::energy(const double *c1, const double *c2,
                       const double *c3, const double *c4)
{
  // evaluate cosine(Phi);
  double r12[3],r32[3],r43[3];
  double r12p[3],r43p[3];
  int i;
  for (i=0; i<3; i++) {
    r12[i] = c1[i] - c2[i];
    r32[i] = c3[i] - c2[i];
    r43[i] = c4[i] - c3[i];
  }
  double dot12_32 = 0.0;
  double dot43_32 = 0.0;
  double rn32i = 0.0;
  for (i=0; i<3; i++) {
    dot12_32 += r12[i]*r32[i];
    dot43_32 += r43[i]*r32[i];
    rn32i += r32[i]*r32[i];
  }
  rn32i = 1.0/rn32i;

  // evaluate components of end vectors perpendicular to torsion bond
  for (i=0; i<3; i++) {
    r12p[i] = r12[i] - dot12_32*rn32i*r32[i];
    r43p[i] = r43[i] - dot43_32*rn32i*r32[i];
  }
  // evaluate lengths of perpendicular components of end vectors and
  // calculate cos(Phi)
  double rn12p = 0.0;
  double rn43p = 0.0;
  double dot12_43 = 0.0;
  for (i=0; i<3; i++) {
    rn12p += r12p[i]*r12p[i];
    rn43p += r43p[i]*r43p[i];
    dot12_43 += r12p[i]*r43p[i];
  }
  rn12p = sqrt(rn12p);
  rn43p = sqrt(rn43p);
  // bail out and return zero energy if we run into a linear angle
  if (rn12p < 1.e-9 || rn43p < 1.e-9) return 0.0;

  double cosPhi = dot12_43/(rn12p*rn43p);

  double nPeriod = p_params[2];
  double result;

  if(nPeriod == 2) {

    double Vphi = p_params[0];
    double cos2Phi0 = p_params[1];
    double cos2Phi = 2.0 * cosPhi*cosPhi - 1.0;
    result = 0.5 * Vphi * ( 1.0 - cos2Phi0 * cos2Phi );

  } else if(nPeriod == 3) {

    double Vphi = p_params[0];
    double cos3Phi0 = p_params[1];
    double cos3Phi = 4.0 * cosPhi * cosPhi * cosPhi - 3.0 * cosPhi;
    result = 0.5 * Vphi * ( 1.0 - cos3Phi0 * cos3Phi );

  } else if(nPeriod == 6) {

    double Vphi = p_params[0];
    double cos6Phi0 = p_params[1];
    double cosPhi2 = cosPhi * cosPhi;
    double cosPhi4 = cosPhi2 * cosPhi2;
    double cosPhi6 = cosPhi4 * cosPhi2;
    double cos6Phi = 32.0*cosPhi6 - 48.0*cosPhi4 + 18.0*cosPhi2 - 1.0;
    result = 0.5 * Vphi * ( 1.0 - cos6Phi0 * cos6Phi );

  }
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//       Returns the forces (in kcal/mol) from this torsion.
///////////////////////////////////////////////////////////////////////////////
void Torsion::forceVector(const double *c1, const double *c2,
                          const double *c3, const double *c4,
                          double* force)
{

  // evaluate gradients of cosine(Phi);
  double r12[3],r32[3],r43[3];
  double r12p[3],r43p[3];
  double gr12p1[3][3],gr12p2[3][3],gr12p3[3][3];
  double gr43p2[3][3],gr43p3[3][3],gr43p4[3][3];
  double rn32i,rn32i_4,ident[3][3];
  int i,j;

  for (i=0; i<12; i++) force[i] = 0.0;
  for (i=0; i<3; i++) {
    r12[i] = c1[i] - c2[i];
    r32[i] = c3[i] - c2[i];
    r43[i] = c4[i] - c3[i];
  }
  double dot12_32 = 0.0;
  double dot43_32 = 0.0;
  rn32i = 0.0;
  for (i=0; i<3; i++) {
    dot12_32 += r12[i]*r32[i];
    dot43_32 += r43[i]*r32[i];
    rn32i += r32[i]*r32[i];
  }
  rn32i = 1.0/rn32i;
  rn32i_4 = 2.0*rn32i*rn32i;

  // evaluate components of end vectors perpendicular to torsion bond
  for (i=0; i<3; i++) {
    r12p[i] = r12[i] - dot12_32*rn32i*r32[i];
    r43p[i] = r43[i] - dot43_32*rn32i*r32[i];
  }
  for (i=0; i<3; i++) {
    for (j=0; j<3; j++) {
      if (i == j) ident[i][j] = 1.0;
      else ident[i][j] = 0.0;
    }
  }

  // evaluate gradients of perpendicular components of end vectors.
  // Note that parts of these gradients are NOT symmetric with
  // respect to interchange of i and j!!!!!!!!!! (e.g.
  // r12[i]*r32[j] != r32[i]*r12[j])
  for (i=0; i<3; i++) {
    for (j=0; j<3; j++) {
      gr12p1[i][j] =  ident[i][j] - rn32i*r32[i]*r32[j];
      gr12p2[i][j] = -ident[i][j] + rn32i*(r32[i]*r32[j]+r12[i]*r32[j]
                   +  dot12_32*ident[i][j]) - dot12_32*rn32i_4*r32[i]*r32[j];
      gr12p3[i][j] = -rn32i*(r12[i]*r32[j]+dot12_32*ident[i][j])
                   +  dot12_32*rn32i_4*r32[i]*r32[j];
      gr43p2[i][j] =  rn32i*(r43[i]*r32[j]+dot43_32*ident[i][j])
                   -  dot43_32*rn32i_4*r32[i]*r32[j];
      gr43p3[i][j] = -ident[i][j] + rn32i*(r32[i]*r32[j]-r43[i]*r32[j]
                   -  dot43_32*ident[i][j]) + dot43_32*rn32i_4*r32[i]*r32[j];
      gr43p4[i][j] =  ident[i][j] - rn32i*r32[i]*r32[j];
    }
  }

  // evaluate lengths of perpendicular components of end vectors and
  // calculate cos(Phi) and its gradients
  double rn12p = 0.0;
  double rn43p = 0.0;
  double dot12_43 = 0.0;
  for (i=0; i<3; i++) {
    rn12p += r12p[i]*r12p[i];
    rn43p += r43p[i]*r43p[i];
    dot12_43 += r12p[i]*r43p[i];
  }
  rn12p = sqrt(rn12p);
  rn43p = sqrt(rn43p);

  // bail out and return zero force if we run into a linear angle
  if (rn12p < 1.e-9 || rn43p < 1.e-9) return;

  double cosPhi = dot12_43/(rn12p*rn43p);
  double ri11 = 1.0/(rn12p*rn43p);
  double ri13 = 1.0/(rn12p*rn43p*rn43p*rn43p);
  double ri31 = 1.0/(rn12p*rn12p*rn12p*rn43p);
  double dPhi1[3],dPhi2[3],dPhi3[3],dPhi4[3];
  for (i=0; i<3; i++) {
    dPhi1[i] = 0.0;
    dPhi2[i] = 0.0;
    dPhi3[i] = 0.0;
    dPhi4[i] = 0.0;
    for (j=0; j<3; j++) {
      dPhi1[i] = dPhi1[i] + ri11*gr12p1[i][j]*r43p[j]
               - dot12_43*ri31*gr12p1[i][j]*r12p[j];
      dPhi2[i] = dPhi2[i] + ri11*(gr12p2[i][j]*r43p[j]
               + gr43p2[i][j]*r12p[j])
               - dot12_43*(ri31*gr12p2[i][j]*r12p[j]
               + ri13*gr43p2[i][j]*r43p[j]);
      dPhi3[i] = dPhi3[i] + ri11*(gr12p3[i][j]*r43p[j]
               + gr43p3[i][j]*r12p[j])
               - dot12_43*(ri31*gr12p3[i][j]*r12p[j]
               + ri13*gr43p3[i][j]*r43p[j]);
      dPhi4[i] = dPhi4[i] + ri11*gr43p4[i][j]*r12p[j]
               - dot12_43*ri13*gr43p4[i][j]*r43p[j];
    }
  }
  double sinPhi = sqrt(1.0-cosPhi*cosPhi*ONEMINUS5EPS);

  double scalar;
  double nPeriod = p_params[2];

  if(nPeriod == 2) {

    double Vphi = p_params[0];
    double cos2Phi0 = p_params[1];
    double sin2Phi = 2.0 * sinPhi * cosPhi;
    if (fabs(sinPhi) > 1.e-9) {
      scalar = - Vphi * cos2Phi0 * sin2Phi;
    } else {
      scalar = - 2.0 * Vphi * cos2Phi0;
    }

  } else if(nPeriod == 3) {

    double Vphi = p_params[0];
    double cos3Phi0 = p_params[1];
    double sin3Phi = 3.0*sinPhi - 4.0*sinPhi*sinPhi*sinPhi;
    if (fabs(sinPhi) > 1.e-9) {
      scalar = - 1.5 * Vphi * cos3Phi0 * sin3Phi;
    } else {
      scalar = - 4.5 * Vphi * cos3Phi0;
    }

  } else if(nPeriod == 6) {

    double Vphi = p_params[0];
    double cos6Phi0 = p_params[1];
    double sinPhi3 = sinPhi * sinPhi * sinPhi;
    double sinPhi5 = sinPhi3 * sinPhi * sinPhi;
    double sin6Phi = cosPhi * (32.0*sinPhi5 - 32.0*sinPhi3 + 6.0*sinPhi);
    if (fabs(sinPhi) > 1.e-9) {
      scalar = - 3.0 * Vphi * cos6Phi0 * sin6Phi;
    } else {
      scalar = - 18.0 * Vphi * cos6Phi0;
    }
  }
  if (fabs(sinPhi) > 1.e-9) scalar = scalar/sinPhi;
  for (i=0; i<3; i++) {
    force[i] = -scalar*dPhi1[i];
    force[i+3] = -scalar*dPhi2[i];
    force[i+6] = -scalar*dPhi3[i];
    force[i+9] = -scalar*dPhi4[i];
  }
  return;
}
