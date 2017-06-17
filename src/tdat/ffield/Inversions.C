///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Inversions.C
//
//
// DESIGN:
//              <Discuss overall design/purpose of class.>
//   (See Inversions.H)
//
///////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "util/ErrMsg.H"
#include "tdat/Inversion.H"
#include "tdat/Inversions.H"
#include "tdat/FFCoords.H"
#include "tdat/UFFParams.H"
#include "tdat/TBond.H"
#include "tdat/TAtm.H"
#include "tdat/FFRules.H"

#define PI  3.14159265358979323846
#define DEG * PI / 180.0

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Construct the set of all inversion angles in the system. If p_mask is
//      specified then forces are only calculated on those atoms for which
//      p_mask is true.
////////////////////////////////////////////////////////////////////////////// 
Inversions::Inversions(vector<TAtm*>* atmptrs, FFRules *rules)
{
  p_atmptrs = atmptrs;
  p_rules = rules;
  p_mask = (vector<bool>*)0;
  p_rings = (vector<int>*)0;
  initInversionList();
  if (rules->getFFRulesName() == "UFF") setInversionParameters();
}

Inversions::Inversions(vector<TAtm*>* atmptrs, vector<bool>* mask,
                       FFRules *rules)
{
  p_atmptrs = atmptrs;
  p_rules = rules;
  p_mask = mask;
  p_rings = (vector<int>*)0;
  initInversionList();
  if (rules->getFFRulesName() == "UFF") setInversionParameters();
}

Inversions::Inversions(vector<TAtm*>* atmptrs,
                       FFRules *rules, vector<int> *ringAtoms)
{
  p_atmptrs = atmptrs;
  p_rules = rules;
  p_mask = (vector<bool>*)0;
  p_rings = ringAtoms;
  initInversionList();
  if (rules->getFFRulesName() == "UFF") setInversionParameters();
}
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Inversion copy constructor.  Uses assignment operator.
////////////////////////////////////////////////////////////////////////////// 
Inversions::Inversions(const Inversions& inversion)
{ (*this) = inversion; }

 
// ---------- Virtual Destructor ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Simple destructor.
////////////////////////////////////////////////////////////////////////////// 
Inversions::~Inversions(void)
{
  for (int i=0; i<p_inversionList.size(); i++) delete p_inversionList[i];
}

// ---------- General Methods  ----------
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Construct a list of inversions in the system
////////////////////////////////////////////////////////////////////////////////
void Inversions::initInversionList(void)
{
  int natoms = p_atmptrs->size();
  int iat, jat, kat, lat, mat;
  TAtm *atm;
  double obnd21,obnd31,obnd41;
  //Atoms along central bond are indexed by 2 and 3, outer atoms are 1 and 4
  for (iat=0; iat<natoms; iat++){
    atm = (*p_atmptrs)[iat];
    vector<TBond*> list = atm->bondList();
    if (p_rules->checkInversion(atm,list)) {
      TBond *bond;
      bond = list[0];
      jat = bond->atom1()->index();
      kat = bond->atom2()->index();
      if (kat != iat) jat = kat;
      bond = list[1];
      kat = bond->atom1()->index();
      lat = bond->atom2()->index();
      if (lat != iat) kat = lat;
      bond = list[2];
      lat = bond->atom1()->index();
      mat = bond->atom2()->index();
      if (mat != iat) lat = mat;
      Inversion * inversion;
      obnd21 = list[0]->order();
      obnd31 = list[1]->order();
      obnd41 = list[2]->order();
      vector<bool> check = p_rules->findInversions(atm,list,p_rings);
      if (p_mask == (vector<bool>*)0) {
        if (check[0]) {
          inversion = new Inversion(iat,jat,kat,lat,obnd21,obnd31,obnd41);
          p_inversionList.push_back(inversion);
        }
        if (check[1]) {
          inversion = new Inversion(iat,kat,jat,lat,obnd41,obnd21,obnd31);
          p_inversionList.push_back(inversion);
        }
        if (check[2]) {
          inversion = new Inversion(iat,lat,kat,jat,obnd31,obnd41,obnd21);
          p_inversionList.push_back(inversion);
        }
      } else if  ((*p_mask)[iat] || (*p_mask)[jat] || (*p_mask)[kat] ||
                 (*p_mask)[lat]) {
        if (check[0]) {
          inversion = new Inversion(iat,jat,kat,lat,obnd21,obnd31,obnd41);
          p_inversionList.push_back(inversion);
        }
        if (check[1]) {
          inversion = new Inversion(iat,lat,jat,kat,obnd41,obnd21,obnd31);
          p_inversionList.push_back(inversion);
        }
        if (check[2]) {
          inversion = new Inversion(iat,kat,lat,jat,obnd31,obnd41,obnd21);
          p_inversionList.push_back(inversion);
        }
      }
    }
  }
}
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Assign parameters to each inversion in inversion list.
//
//     See Rappe' et al.  BUT WATCH THEIR ALGEBRA.  Note the distinction
//     between the angles omega (out-of-plane angle) and gamma (angle w/r to
//     a normal to the plane).   Rappe' et al. define this term w/r to omega,
//     but gamma is used for the actual implementation.  Beware several
//     errors in their paper on this point.
//     gamma = pi/2 - omega, defined such that gamma > 0.0 on the side of
//     the center away from the other two substituents.  E.g., for ethylene,
//     omega = 0 and gamma = +pi/2.
//
//     The range for gamma is -Pi < gamma <= Pi
//
//     INVERSION FORCE RULES FROM RAPPE' ET AL:
//     * functional form:
//
//       E_omega = (K/3.0) * (C0 + C1*cos(omega) + C2*cos(2*omega))
//
//               = (K/3.0) * (C0 + C1*sin(gamma) + C2*cos(2*gamma))
//
//
//     * atm atom must be bonded to exactly 3 other atoms
//       ShapeData::shape() = trigonalPlanar or pyramidal
//       NOT COUNTING any Metal-Pi bonds! (identified by bondorder < 1.0)
//
//     * for C_2 and C_R sp2 atoms,  C0=1.0, C1=-1.0, C2=0.0 (omega0=0, planar)
//           ** if the C is bonded to an O_2, the force constant K = 50 kcal/mol
//           ** else K = 6.0 kcal/mol
//
//     * for group 5 (N column) and group 6 (O column) atm atoms,
//       Cn chosen for Minimum at experimental omega0; maximum at omega=0
//
//       Barrier energy for N 5.74 kcal/mol  P 37.06 kcal/mol (Cotton&Wilkinson)
//                          N 0.0  kcal/mol  P 22    kcal/mol  Rappe'
//
//  numerical solution shows that a K of 33.0 gives a barrier height of 5.74
//  for NH3.  K of 12 gives a barrier height for PH3 near the 22.0 kcal/mol
//  chosen by Rappe'.
//
//       The values from Cotton and Wilkinson are used here.
//       BUT Rappe' also claims to use experimental values AND to set them to
//       zero for group6.  Will use same constants for group5 and group6, since
//       pyramidal group6 molecules will usually be cations isoelectronic to
//       group5 neutrals.  To get equilibrium angles near omega = 90deg, want
//       approximately C0=1.0, C1=0.0, C2=1.0
//
//       Three kinds of angles are used here:  the apex angle theta of an
//       equilateral triangular pyramid (this is the angle usually given in
//       describing the geometry of NH3); omega, the angle from one leg of the
//       pyramid to the plane of the other two legs (the forcefield is defined
//       in terms of this angle), and gamma, the angle of
//       one leg with the _normal_ to the plane of the the other two legs (the
//       forcefield is actually _implemented_ in terms of gamma = omega - Pi/2
//       (see the discussion in Rappe', but look at corrections in Notes.ps).
//
//       NH3 HNH angle theta ia  106.75, omega0(NH3) = 118.887
//         ==> gamma0(NH3) = + 28.887deg
//
//       PH3 HPH angle theta is 93.7, omega0(PH3) = 95.4
//         ==> gamma0 = + 5.4deg
//
////////////////////////////////////////////////////////////////////////////////
void Inversions::setInversionParameters(void)
{
  int ninv = p_inversionList.size();
  int iinv;
  Inversion* inversion;
  UFFParams *parameterserver = new UFFParams(p_atmptrs);
  double *params = new double[4];
  for (iinv=0; iinv<ninv; iinv++) {
    inversion = p_inversionList[iinv];
    int iatom1 = inversion->atom1Index();
    int iatom2 = inversion->atom2Index();
    int iatom3 = inversion->atom3Index();
    int iatom4 = inversion->atom4Index();

    TAtm *atm = (*p_atmptrs)[iatom1];

    ShapeData::Shape shape = atm->shape();
    int atomicNumber = atm->atomicNumber();
    bool foundAtom = false;
    double omega0, forceConst;
    if (shape == ShapeData::pyramidal) {
      if (atomicNumber == 7 || atomicNumber == 8) {
        ////////////////////////////////////////////////////////////////////////
        // forceConst chosen to give an inversion barrier height of
        // roughly 5.74 kcal/mol (graphically)
        ////////////////////////////////////////////////////////////////////////
        omega0 = 118.89 DEG;
        forceConst = 33.0;
        foundAtom = true;
      } else if (atomicNumber == 15 || atomicNumber == 16 ||
                 atomicNumber == 33 || atomicNumber == 34 ||
                 atomicNumber == 51 || atomicNumber == 52 ||
                 atomicNumber == 83 || atomicNumber == 84) {
      //////////////////////////////////////////////////////////////////////////
      //// the rest of the N and O columns have these params
      //////////////////////////////////////////////////////////////////////////

        omega0 = 95.4 DEG;
        forceConst = 12.0;
        foundAtom = true;
      } else {
        EE_RT_ASSERT(0,EE_WARNING,"Unknown pyramidal inversion found");
      }
    } else if (shape == ShapeData::trigonalPlanar) {
      if (atomicNumber == 6) {
        omega0 = 180.0 DEG;
        forceConst = 6.0;
      //////////////////////////////////////////////////////////////////////////
      // is this C bonded to an sp2 Oxygen? Check the atom types of neighbors
      //////////////////////////////////////////////////////////////////////////
        if (parameterserver->uffIndex(iatom2) == 17 ||
            parameterserver->uffIndex(iatom3) == 17 ||
            parameterserver->uffIndex(iatom4) == 17) {
          forceConst = 50.0;
        }
        foundAtom = true;
      } else {
        EE_RT_ASSERT(0,EE_WARNING,"Unknown trigonal planar inversion found");
      }
    } else {
      EE_RT_ASSERT(0,EE_WARNING,"Unknown inversion found");
    }
    if (foundAtom) {
      double c0, c1, c2;
      if (omega0 == 180.0 DEG) {
        c0 = 1.0;
        c1 = 1.0;
        c2 = 0.0;
      } else {
        double sinOmega = sin(omega0);
        double cosOmega = cos(omega0);
        c2 = 1.0/(4.0 * sinOmega * sinOmega);
        c1 = -4.0 * c2 * cosOmega;
        c0 = c2 * (2.0 * cosOmega * cosOmega + 1.0);
      }
      params[0] = forceConst;
      params[1] = c0;
      params[2] = c1;
      params[3] = c2;
    }
    // averaged over 3 inversions (see Rappe')
    params[0] = params[0]/3.0;
    inversion->setParams(params);
  }
  delete parameterserver;
  delete[] params;
  return;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Returns contribution to the forces from inversions.
////////////////////////////////////////////////////////////////////////////////
void Inversions::forces(const FFCoordinates* coords, FFCoordinates* result)
{

  Inversion* inversion;
  int ntor,itor,iat,jat,kat,lat;
  double *tForce = new double[12];

  ntor = p_inversionList.size();
  const double *c1;
  const double *c2;
  const double *c3;
  const double *c4;
  for (itor=0; itor<ntor; itor++) {
    inversion = p_inversionList[itor];
    iat = inversion->atom1Index();
    jat = inversion->atom2Index();
    kat = inversion->atom3Index();
    lat = inversion->atom4Index();
    c1 = coords->row(iat);
    c2 = coords->row(jat);
    c3 = coords->row(kat);
    c4 = coords->row(lat);
    inversion->forceVector(c1,c2,c3,c4,tForce);
    if (p_mask == (vector<bool>*)0) {
      result->add(iat, tForce);
      result->add(jat, tForce+3);
      result->add(kat, tForce+6);
      result->add(lat, tForce+9);
    } else {
      if ((*p_mask)[iat]) result->add(iat, tForce);
      if ((*p_mask)[jat]) result->add(jat, tForce+3);
      if ((*p_mask)[kat]) result->add(kat, tForce+6);
      if ((*p_mask)[lat]) result->add(lat, tForce+9);
    }
  }
  delete[] tForce;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Returns the contribution to the energy (in kcal/mol) from these 
//       inversions.
////////////////////////////////////////////////////////////////////////////////
double Inversions::energy(const FFCoordinates* coords)
{

  Inversion* inversion;
  int ntor,itor,iat,jat,kat,lat;
  double ret = 0.0;

  ntor = p_inversionList.size();
  const double *c1;
  const double *c2;
  const double *c3;
  const double *c4;
  for (itor=0; itor<ntor; itor++) {
    inversion = p_inversionList[itor];
    iat = inversion->atom1Index();
    jat = inversion->atom2Index();
    kat = inversion->atom3Index();
    lat = inversion->atom4Index();
    c1 = coords->row(iat);
    c2 = coords->row(jat);
    c3 = coords->row(kat);
    c4 = coords->row(lat);
    ret += inversion->energy(c1,c2,c3,c4);
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//   Return total number of inversions
////////////////////////////////////////////////////////////////////////////////
int Inversions::getCount() const
{
   return p_inversionList.size();
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves the inversion force field name where the name is made
//              up of concatonated atomic symbols with the character '-' 
//              separating the atom types.
//              The order of the atom types in the inversion type name is
//              as follows: atom1 is placed in the 3rd position, atom 4 is
//              placed in the 4th position, atom2 and atom3 are place in the
//              1st and 2nd positions by alphabetical order.
//
// Param: idx is the index of the target inversion in the local inversion list.
//
// Return: inversion name. An empty string is returned if the index is invalid.
//
////////////////////////////////////////////////////////////////////////////////
string Inversions::getName( const int idx)
{
   string name;
   int *indices = getAtomOrderedIndices( idx);  // validates idx

   if ( indices != 0)                    // idx is valid
   {
      name.append( (*p_atmptrs)[indices[0]]->atomicSymbol());
      int i;
      for ( i=1;i<4;i++) {
         name.append( 1, '-');
         name.append( (*p_atmptrs)[indices[i]]->atomicSymbol());
      }
      delete [] indices;
   }
   return name;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves the inversion force field type name where the name is
//              made up of concatonated atom types with the character '-' 
//              separating the atom types.
//              A carbon atom, C, may have several types, e.g., CT, CA, CS, ...
//              The order of the atom types in the inversion type name is
//              as follows: atom1 is placed in the 3rd position, atom 4 is
//              placed in the 4th position, atom2 and atom3 are place in the
//              1st and 2nd positions by alphabetical order.
//
// Param: idx is the index of the target inversion in the local inversion list.
//
// Return: inversion type name. An empty string is returned if the index is 
//              invalid.
//
////////////////////////////////////////////////////////////////////////////////
string Inversions::getType( const int idx)
{
  string type;
  int *indices = getAtomOrderedIndices( idx);  // validates idx

  if ( indices != 0)                    // idx is valid
  {
     type.append( (*p_atmptrs)[indices[0]]->cAtomType());
     int i;
     for ( i=1;i<4;i++) {
        type.append( 1, '-');
        type.append( (*p_atmptrs)[indices[i]]->cAtomType());
     }
     delete [] indices;
   }
   return type;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns an array of the indices to the atoms of the inversion
//              specified in the input index. The order of the indices in
//              the returned array is determined by the alphanumberic
//              order of atom2 and atom3 types.
//
// Param: idx is the index of the target inversion in the local inversion list.
//
// Return: a pointer to an integer array containing the indices of component
//         atoms. If the inversion index is invalid, the returned pointer
//         is null.
//
////////////////////////////////////////////////////////////////////////////////
int* Inversions::getAtomOrderedIndices( const int idx)
{
   int *indices = 0;

   if ( isValidIndex( idx))
   {
      indices = new int[4];

      Inversion *inversion = p_inversionList[idx];
      int id3 = inversion->atom3Index();
      int id4 = inversion->atom4Index();
      char *t3 =  (*p_atmptrs)[id3]->cAtomType();
      char *t4 =  (*p_atmptrs)[id4]->cAtomType();
      if ( strcmp( t3, t4) < 0)  {
         indices[0] = id3;
         indices[1] = id4;
      } else {
         indices[0] = id4;
         indices[1] = id3;
      }
      indices[2] = inversion->atom1Index();
      indices[3] = inversion->atom2Index();
   }
   return indices;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the component atom indices for the specified inversion.
//               If the inversion index is out of range, the returned values for
//               the component atom indices are set to invalid values (-1).
//
// Param: idx index of the target inversion in this Inversions object.
// Param: atom1 pointer to index of the first atom
// Param: atom2 pointer to index of the second atom
// Param: atom3 pointer to index of the third atom
// Param: atom4 pointer to index of the fourth atom
//
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
void Inversions::getAtomIndices( const int idx, int* atom1, int* atom2,
                                                int* atom3, int* atom4)
{
  if (isValidIndex(idx)) {
    Inversion *inversion = p_inversionList[idx];
    *atom3 = inversion->atom1Index();
    *atom4 = inversion->atom2Index();
    *atom1 = inversion->atom3Index();
    *atom2 = inversion->atom4Index();
  }
  else  {
    *atom1 = -1;
    *atom2 = -1;
    *atom3 = -1;
    *atom4 = -1;
  }
}

////////////////////////////////////////////////////////////////////////////////
//----------------------Private Methods-----------------------------------------
////////////////////////////////////////////////////////////////////////////////
// Description
//   Return true if idx is a valid index
////////////////////////////////////////////////////////////////////////////////
bool Inversions::isValidIndex(const int idx) const
{
  return idx >=0 && idx < p_inversionList.size();
}
