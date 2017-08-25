///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Angle.C
//
//
// DESIGN:
//              <Discuss overall design/purpose of class.>
//     (See Angle.H)
//
///////////////////////////////////////////////////////////////////////////////

#include <string.h>

#include "util/ErrMsg.H"
#include "tdat/Angles.H"
#include "tdat/Angle.H"
#include "tdat/FFCoords.H"
#include "tdat/UFFParams.H"
#include "tdat/TBond.H"
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
//      Construct a complete set of angles for the system using a list of
//      atom pointers. If the vector p_mask is specified then forces are only
//      to be calculated for the atoms for which p_mask is set to true.
////////////////////////////////////////////////////////////////////////////// 
Angles::Angles(vector<TAtm*>* atmptrs, FFRules *rules)
{
  p_atmptrs = atmptrs;
  p_mask = (vector<bool>*)0;
  p_rules = rules;
  initAngleList();
  if (rules->getFFRulesName() == "UFF") setAngleParameters();
}

Angles::Angles(vector<TAtm*>* atmptrs, vector<bool>* mask, FFRules *rules)
{
  p_atmptrs = atmptrs;
  p_mask = mask;
  p_rules = rules;
  initAngleList();
  if (rules->getFFRulesName() == "UFF") setAngleParameters();
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Angles copy constructor.  (Uses assignment operator).
//////////////////////////////////////////////////////////////////////////////

Angles::Angles(Angles& angle)
{
  (*this) = angle;
}

 
// ---------- Virtual Destructor ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
////////////////////////////////////////////////////////////////////////////// 
Angles::~Angles(void)
{
  for (size_t i=0; i<p_angleList.size(); i++) delete p_angleList[i];
}


// ------------ General Methods ------------
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Construct a list of angles in the system
////////////////////////////////////////////////////////////////////////////////
void Angles::initAngleList(void)
{
  int natoms = p_atmptrs->size();
  int iat,jat,kat,lat,i,j,nbnd;
  double obnd1, obnd2;
  TBond *bond1, *bond2;
  TAtm *atm;
  for (iat=0; iat<natoms; iat++) {
    atm = (*p_atmptrs)[iat];
    vector<TBond*> list = atm->bondList();
    nbnd = list.size();
    if (nbnd > 1) {
      for (i=0; i<nbnd-1; i++) {
        bond1 = list[i];
        jat = bond1->atom1()->index();
        kat = bond1->atom2()->index();
        if (kat != iat) jat = kat;
        obnd1 = bond1->order();
        for (j=i+1; j<nbnd; j++) {
          bond2 = list[j];
          kat = bond2->atom1()->index();
          lat = bond2->atom2()->index();
          if (lat != iat) kat = lat;
          obnd2 = bond2->order();
          if (p_mask == (vector<bool>*)0) {
            Angle *angle = new Angle(jat,iat,kat,obnd1,obnd2);
            p_angleList.push_back(angle);
          } else {
            if ((*p_mask)[iat] || (*p_mask)[jat] || (*p_mask)[kat]) {
              Angle *angle = new Angle(jat,iat,kat,obnd1,obnd2);
              p_angleList.push_back(angle);
            }
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Set parameters for each angle from the Universal Force Field.
////////////////////////////////////////////////////////////////////////////////
void Angles::setAngleParameters(void)
{
  UFFParams *parameterserver = new UFFParams(p_atmptrs);
  Angle *angle;
#ifdef HOLD
  int iang, nang, iat, jat, kat;
#else
  int iang, nang;
#endif
  double bo1, bo2;

  double *params = new double[5];

  // Scan over all angles in the list
  nang = p_angleList.size();
  for (iang = 0; iang < nang; iang++) {
    angle = p_angleList[iang];
    bo1 = angle->bondOrder1();
    bo2 = angle->bondOrder2();

    // -- begin setup params -- //

    int uffTypeIndex1 = parameterserver->uffIndex(angle->atom1Index());
    int uffTypeIndex2 = parameterserver->uffIndex(angle->atom2Index());
    int uffTypeIndex3 = parameterserver->uffIndex(angle->atom3Index());

    // force constant
    // (Why convert to integer? Check documentation)
    int bond1Order = (int)bo1;
    int bond2Order = (int)bo2;
    double forceConst;
    double thetaEq, sinThetaEq, cosThetaEq, sinThetaEq2, cosThetaEq2;
    if (bond1Order > 0 && bond2Order > 0) {
      double r12 =
        parameterserver->r12( uffTypeIndex1, uffTypeIndex2, bond1Order);
      double r23 =
        parameterserver->r12( uffTypeIndex2, uffTypeIndex3, bond2Order);

      double zStar1 = parameterserver->effCharge(uffTypeIndex1);
      double zStar3 = parameterserver->effCharge(uffTypeIndex3);

      thetaEq = parameterserver->bondAngle(uffTypeIndex2);

      cosThetaEq = cos( thetaEq DEG );
      sinThetaEq = sin( thetaEq DEG );
      cosThetaEq2 = cosThetaEq * cosThetaEq;
      sinThetaEq2 = sinThetaEq * sinThetaEq;

      double r13 = sqrt( r12*r12 + r23*r23 - 2.0*r12*r23*cosThetaEq );

      double r13m1 = 1.0 / r13;
      double r13m2 = r13m1 * r13m1;
      double r13m5 = r13m2 * r13m2 * r13m1;   // r13^(-5)
      double beta = 664.12 / (r12 * r23);

      forceConst = beta * zStar1 * zStar3 * r12 * r23 * r13m5
          * (3.0 * r12*r23*(1.0-cosThetaEq2) - r13*r13*cosThetaEq );
    } else {
      thetaEq = parameterserver->bondAngle(uffTypeIndex2);

      cosThetaEq = cos( thetaEq DEG );
      sinThetaEq = sin( thetaEq DEG );
      cosThetaEq2 = cosThetaEq * cosThetaEq;
      sinThetaEq2 = sinThetaEq * sinThetaEq;
      forceConst = 0.0;
    }

    params[0] = forceConst;

    if ( thetaEq != 180.0 && thetaEq != 120.0 && thetaEq != 90.0 ) {

      double c2 = 1.0/(4.0 * sinThetaEq2);
      double c1 = -4.0 * c2 * cosThetaEq;
      double c0 = c2 * ( 2.0 * cosThetaEq2 + 1.0 );
          
      params[1] = c0;
      params[2] = c1;
      params[3] = c2;

    }

    params[4] = (thetaEq); // used to determine which function.

    // -- end setup params -- //

    angle->setParams(params);

  }
  delete parameterserver;
  delete params;
  return;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Evaluate contribution to the forces from all angles.
////////////////////////////////////////////////////////////////////////////////
void Angles::forces(const FFCoordinates* coords, FFCoordinates *result)
{
  Angle* angle;
  int nang, iang, iat, jat, kat;
  double *aForce = new double[9];

  nang = p_angleList.size();
  const double *c1;
  const double *c2;
  const double *c3;

  for (iang=0; iang<nang; iang++) {
    angle = p_angleList[iang];
    iat = angle->atom1Index();
    jat = angle->atom2Index();
    kat = angle->atom3Index();
    c1 = coords->row(iat);
    c2 = coords->row(jat);
    c3 = coords->row(kat);
    angle->forceVector(c1,c2,c3,aForce);
    if (p_mask == (vector<bool>*)0) {
      result->add(iat, aForce);
      result->add(jat, aForce+3);
      result->add(kat, aForce+6);
    } else {
      if ((*p_mask)[iat]) result->add(iat, aForce);
      if ((*p_mask)[jat]) result->add(jat, aForce+3);
      if ((*p_mask)[kat]) result->add(kat, aForce+6);
    }
  }
  delete[] aForce;
}

////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Evaluate contribution to the total energy from angles.
////////////////////////////////////////////////////////////////////////////////
double Angles::energy(const FFCoordinates* coords)
{
  Angle* angle;
  int nang, iang, iat, jat, kat;
  double ret = 0.0;

  nang = p_angleList.size();
  const double *c1;
  const double *c2;
  const double *c3;

  for (iang=0; iang<nang; iang++) {
    angle = p_angleList[iang];
    iat = angle->atom1Index();
    jat = angle->atom2Index();
    kat = angle->atom3Index();
    c1 = coords->row(iat);
    c2 = coords->row(jat);
    c3 = coords->row(kat);
    ret += angle->energy(c1,c2,c3);
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Description:
//   Return number of angles in topology
////////////////////////////////////////////////////////////////////////////////
int Angles::getCount() const
{
   return p_angleList.size();
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns a string containing the angle force  field name where
//              the name is made up of concatonated atom types with the 
//              character '-' separates the atom types.  A carbon atom, C, will
//              have a name that uniquely identifies the atom in a specific 
//              structure based on its location. The order of the atoms in the 
//              angle name is determined by the alphabetical order of the 
//              atomic symbols of the outer atoms of the angle.
//
// Param: idx is the index of the target angle in the local angle list.
//
// Return: angle type name. An empty string is returned if the index is invalid.
//
////////////////////////////////////////////////////////////////////////////////
string Angles::getName( const int idx)
{
  string name;
  int *indices = getAtomOrderedIndices( idx);            // validates idx

  if ( indices != 0)                              // null if idx is invalid
  {
    // retrieve order list of component atom indices
    name.append( (*p_atmptrs)[indices[0]]->atomicSymbol());
    name.append( 1, '-');
    name.append(  (*p_atmptrs)[indices[1]]->atomicSymbol());
    name.append( 1, '-');
    name.append( (*p_atmptrs)[indices[2]]->atomicSymbol());
    delete [] indices;
  }
  return name;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves the angle field type name where the name is made up 
//              of concatonated atom types with the character '-' separating 
//              the atom types.
//              A carbon atom, C, may have several types, e.g., CT, CA, CS, ...
//              The order of the atom types in the angle type name is
//              determined by the alphabetical order of the atomic symbols of
//              the outer atoms of the angle.
//
// Param: idx is the index of the target angle in the local angle list.
//
// Return: angle type name. An empty string is returned if the index is invalid.
//
////////////////////////////////////////////////////////////////////////////////
string Angles::getType( const int idx)
{
  string type;
  int *indices = Angles::getAtomOrderedIndices( idx);       // validates idx

  if ( indices != 0)                                 // null if invalid idx
  {
    // retrieve order list of component atom indices
    type.append( (*p_atmptrs)[indices[0]]->cAtomType());
    type.append( 1, '-');
    type.append( (*p_atmptrs)[indices[1]]->cAtomType());
    type.append( 1, '-');
    type.append( (*p_atmptrs)[indices[2]]->cAtomType());
    delete [] indices;
  }
  return type;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns an array of the indices to the atoms of the angle
//              specified in the input index. The order of the indices in
//              the returned array is determined by the alphanumberic 
//              order of the outer atom types.
//
// Param: idx is the index of the target angle in the local angle list.
//
// Return: a pointer to an integer array containing the indices of component
//         atoms. If the angle index is invalid, the returned pointer is
//         null.
//
////////////////////////////////////////////////////////////////////////////////
int* Angles::getAtomOrderedIndices( const int idx)
{
  int *indices = (int*)0;
  if (isValidIndex(idx))
  {
    indices = new int[3];

    Angle *angle = p_angleList[idx];
    int id1 = angle->atom1Index();
    int id3 = angle->atom3Index();
    char* t1 = (*p_atmptrs)[id1]->cAtomType();
    char* t3 = (*p_atmptrs)[id3]->cAtomType();

    if ( strcmp( t1, t3) < 0) {
      indices[0] = id1;
      indices[2] = id3;
    } else {
      indices[0] = id3;
      indices[2] = id1;
    }
    indices[1] = angle->atom2Index();
  }
  return indices;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the component atom indices for the specified angle. If
//              the angle index is out of range, the returned values for the
//              component atom indices are set to invalid values (-1).
//
// Param: idx index of the target angle in this Angles object.
// Param: atom1 pointer to index of the first atom
// Param: atom2 pointer to index of the second atom
// Param: atom3 pointer to index of the third atom
//
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
void Angles::getAtomIndices( const int idx, int* atom1, int* atom2, int* atom3)
{
  if (isValidIndex(idx)) {
    Angle *angle = p_angleList[idx];
    *atom1 = angle->atom1Index();
    *atom2 = angle->atom2Index();
    *atom3 = angle->atom3Index();
  }
  else  {
    *atom1 = -1;
    *atom2 = -1;
    *atom3 = -1;
  }
}

////////////////////////////////////////////////////////////////////////////////
//------------------------Private Methods---------------------------------------
////////////////////////////////////////////////////////////////////////////////
// Description:
//   Return true if idx is a valid index
////////////////////////////////////////////////////////////////////////////////
bool Angles::isValidIndex(const int idx) const
{
  return idx >= 0 && idx < p_angleList.size();
}
