///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Torsions.C
//
//
// DESIGN:
//              <Discuss overall design/purpose of class.>
//   (See Torsions.H)
//
///////////////////////////////////////////////////////////////////////////////

#include <string.h>

// application includes
#include "util/ErrMsg.H"
#include "tdat/Torsion.H"
#include "tdat/Torsions.H"
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
//      Construct a class containing all torsions in the system. If p_mask
//      is specified, then forces only calculated for those atoms for which
//      p_mask is true
////////////////////////////////////////////////////////////////////////////// 
Torsions::Torsions(vector<TAtm*>* atmptrs, FFRules *rules)
{
  p_atmptrs = atmptrs;
  p_mask = (vector<bool>*)0;
  p_rules = rules;
  initTorsionList();
  if (rules->getFFRulesName() == "UFF") setTorsionParameters();
}

Torsions::Torsions(vector<TAtm*>* atmptrs, vector<bool>* mask, FFRules *rules)
{
  p_atmptrs = atmptrs;
  p_mask = mask;
  p_rules = rules;
  initTorsionList();
  if (rules->getFFRulesName() == "UFF") setTorsionParameters();
}
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Torsion copy constructor.  Uses assignment operator.
////////////////////////////////////////////////////////////////////////////// 
Torsions::Torsions(const Torsions& torsion)
{ (*this) = torsion; }

 
// ---------- Virtual Destructor ------------
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//      Simple destructor.
////////////////////////////////////////////////////////////////////////////// 
Torsions::~Torsions(void)
{
  for (int i=0; i<p_torsionList.size(); i++) delete p_torsionList[i];
}

// ---------- General Methods  ----------
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Construct a list of torsions in the system
////////////////////////////////////////////////////////////////////////////////
void Torsions::initTorsionList(void)
{
  int natoms = p_atmptrs->size();
  int iat, jat, kat, lat, mat, i, j, k, nbnd2, nbnd3;
  double obnd12, obnd32, obnd43;
  TBond *bond12, *bond32, *bond43;
  TAtm *atm2, *atm3;
  //Atoms along central bond are indexed by 2 and 3, outer atoms are 1 and 4
  for (iat=0; iat<natoms; iat++){
    atm2 = (*p_atmptrs)[iat];
    vector<TBond*> list2 = atm2->bondList();
    nbnd2 = list2.size();
    if (nbnd2 > 1) {
      for (i=0; i<nbnd2; i++) {
        bond32 = list2[i];
        jat = bond32->atom1()->index();
        kat = bond32->atom2()->index();
        if (kat != iat) jat=kat;
        // Use this requirement to avoid duplicates in list
        if (jat > iat) {
          atm3 = (*p_atmptrs)[jat];
          vector<TBond*> list3 = atm3->bondList();
          nbnd3 = list3.size();
          if (nbnd3 > 1) {
            // At this point, we have determined that there are some torsions
            // associated with bond 32. We now need to find them.
            for (j=0; j<nbnd2; j++) {
              bond12 = list2[j];
              kat = bond12->atom1()->index();
              lat = bond12->atom2()->index();
              if (lat != iat) kat=lat;
              for (k=0; k<nbnd3; k++) {
                bond43 = list3[k];
                lat = bond43->atom1()->index();
                mat = bond43->atom2()->index();
                if (mat != jat) lat=mat;
                // Eliminate the possibility that one of the end bonds is actually
                // the center bond or that the system forms a three-member ring.
                if (kat != jat && lat != iat && kat != lat) {
                  obnd32 = bond32->order();
                  obnd12 = bond12->order();
                  obnd43 = bond43->order();
                  if (p_mask == (vector<bool>*)0) {
                    Torsion *torsion = new Torsion(kat, iat, jat, lat,
                                                   obnd12, obnd32, obnd43);
                    p_torsionList.push_back(torsion);
                  } else if ((*p_mask)[iat] || (*p_mask)[jat] || (*p_mask)[kat]
                             || (*p_mask)[lat]) {
                    Torsion *torsion = new Torsion(kat, iat, jat, lat,
                                                   obnd12, obnd32, obnd43);
                    p_torsionList.push_back(torsion);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Assign parameters to each torsion in torsion list using the Universal
//    Force Field.
//
//      TORSION RULES for periodicity of potential nPeriod, equilibrium
//                    angle phi0, and barrier height Vphi
//      * Torsion forces are defined only for central pairs of atoms that are
//        BOTH: (1) sp2 or sp3 hybridized  AND (2) Non-metals
//
//      General Cases (central atoms j and k):
//      * case a: j=sp3 and k=sp3 ==> nPeriod=3, phi0=60, and Vphi=sqrt(ViVj)
//      * case b: j=sp2 and k=sp3 ==> nPeriod=6, phi0=0, and Vphi=1.0 kcal/mol
//      * case c: j=sp2 and k=sp2 ==> nPeriod=2, phi0=60, and
//              Vphi= 5 sqrt(UjUk)(1 + 4.18 ln(BondOrder_jk)) [Rappe' Eq 17]
//
//      Exceptions to above cases:
//      * case e1: j and k both sp3 group6 (O column of periodic table)
//                 connected by a bond of order 1.0. nPeriod=2, phi0=90 and
//                 Vphi=sqrt(VjVk); Vi= 2.0 kcal/m for O, 6.8 kcal/m otherwise
//      * case e2: j is sp3 group6 and k is sp2 non-group6, singly-bonded:
//                 nPeriod=2, phi0=90, Vphi from Eq 17 as above case c.
//      * case e3: j=sp3 k=sp2 where k is bonded to another sp2:
//                 nPeriod=3, phi0=180, Vphi=2.0kcal/m
//
//      (in the following, sp2=trigonalPlanar=trigonal planar, and
//      sp3=tetrahedral)
////////////////////////////////////////////////////////////////////////////////
void Torsions::setTorsionParameters(void)
{
  int ntor = p_torsionList.size();
  int itor;
  Torsion* torsion;
  UFFParams *parameterserver = new UFFParams(p_atmptrs);
  double obnd32;
  double *params = new double[3];
  for (itor=0; itor<ntor; itor++) {

    torsion = p_torsionList[itor];
    obnd32 = torsion->bond32Order();
    int iatom1 = torsion->atom2Index();
    int iatom2 = torsion->atom3Index();

    const TAtm* middleAtom1 = (*p_atmptrs)[iatom1];
    const TAtm* middleAtom2 = (*p_atmptrs)[iatom2];

    //
    // information from reference atom objects
    int atomicNumber1 = (int)middleAtom1->atomicNumber();
    int atomicNumber2 = (int)middleAtom2->atomicNumber();
    ShapeData::Shape shape1 = middleAtom1->shape();
    ShapeData::Shape shape2 = middleAtom2->shape();

    ////////////////////////////////////////////////////////////////////////
    // no torsions for central or terminal bonds of order < 1.0.  These
    // represent metal-pi bonds in the ECCE' builder, and exert only
    // along-bond forces
    //
    // Also, torsion forces are only included in UFF for central atoms both
    // sp2/sp3 and non-metal; return null pointer if not
    ////////////////////////////////////////////////////////////////////////
    if ( ( obnd32 < 0.99 )
         || (shape1 != ShapeData::trigonalPlanar && 
             shape1 != ShapeData::tetrahedral)
         || (shape2 != ShapeData::trigonalPlanar && 
             shape2 != ShapeData::tetrahedral) ) {
      // Just set parameters to zero values to prevent access errors
      params[0] = 0.0;
      params[1] = 0.0;
      params[2] = 2.0;
      torsion->setParams(params);

    } else if ( parameterserver->isMetal(atomicNumber1) || 
                parameterserver->isMetal(atomicNumber2) ) {
      // Just set parameters to zero values to prevent access errors
      params[0] = 0.0;
      params[1] = 0.0;
      params[2] = 2.0;
      torsion->setParams(params);
    } else {

    ////////////////////////////////////////////////////////////////////////
    // there are only torsions around this bond if both central atoms
    // have other bonds
    ////////////////////////////////////////////////////////////////////////
      int neighborsOfAtom1 = middleAtom1->bondList().size();
      int neighborsOfAtom2 = middleAtom2->bondList().size();

      int nTorsionsAroundThisBond =
        (neighborsOfAtom1 - 1) * (neighborsOfAtom2 - 1);

      if ( nTorsionsAroundThisBond > 0 ) {

        ////////////////////////////////////////////////////////////////////////
        // parameters to define torsion potential
        ////////////////////////////////////////////////////////////////////////
        int nPeriod = 0;     // periodicity of torsion potential
        double phi0 = 0;         // equilibrium torsion angle in degrees
        double Vphi = 0;         // torsion barrier height in kcal/mol

        static const double u[] = { 0.0, 2.0, 1.25, 0.7, 0.2, 0.1 };

        ////////////////////////////////////////////////////////////////////////
        // assign parameters, according to rules stated under Implemenation above.
        ////////////////////////////////////////////////////////////////////////
        if ( ( parameterserver->isGroup6(atomicNumber1) && 
               parameterserver->isGroup6(atomicNumber2 ) ) &&
             ( shape1 == ShapeData::tetrahedral && 
               shape2 == ShapeData::tetrahedral ) &&
             ( obnd32 == 1.0 )) {

          ////////////////////////////////////////////////////////////////////////
          // exception e1
          ////////////////////////////////////////////////////////////////////////
          nPeriod = 2;  phi0 = 90.0;
          double V1 = 0.0;  double V2 = 0.0;
          if (atomicNumber1 == 8) {V1 = 2.0;} else { V1 = 6.8;}
          if (atomicNumber2 == 8) {V2 = 2.0;} else { V2 = 6.8;}
          Vphi = sqrt(V1*V2);    Vphi = Vphi/nTorsionsAroundThisBond;

        } else if ( (( parameterserver->isGroup6(atomicNumber1)
                       && shape1 == ShapeData::tetrahedral)
                     && (!parameterserver->isGroup6(atomicNumber2)
                         && shape2 == ShapeData::trigonalPlanar)) ||
                    (( parameterserver->isGroup6(atomicNumber2)
                       && shape2 == ShapeData::tetrahedral)
                     && (!parameterserver->isGroup6(atomicNumber1)
                         && shape1 == ShapeData::trigonalPlanar)) ){
          ////////////////////////////////////////////////////////////////////////
          // exception e2: eqn 17 of Rappe'
          ////////////////////////////////////////////////////////////////////////
          nPeriod = 2;  phi0 = 90.0;
          Vphi = 5.0 * sqrt( u[parameterserver->period(atomicNumber1)] * 
                             u[parameterserver->period(atomicNumber1)] )
            * (1.0 + 4.18 * log(obnd32));
          Vphi = Vphi/nTorsionsAroundThisBond;

        } else if ( shape1 == ShapeData::tetrahedral && 
                    shape2 == ShapeData::tetrahedral ) {
          ////////////////////////////////////////////////////////////////////////
          // case a
          ////////////////////////////////////////////////////////////////////////
          nPeriod = 3;  phi0 = 60.0;
          Vphi = sqrt( parameterserver->vsp3(atomicNumber1) * 
                       parameterserver->vsp3(atomicNumber2) );
          Vphi = Vphi/nTorsionsAroundThisBond;

        } else if ( shape1 == ShapeData::tetrahedral || 
                    shape2 == ShapeData::tetrahedral ) {

          ////////////////////////////////////////////////////////////////////////
          // case b or e3: one is sp3, the other sp2.  check for exception e3:
          // i.e. see if the sp2 middle atom has an sp2 neighbor
          ////////////////////////////////////////////////////////////////////////
          int isp2MiddleAtom;   // index of sp2 middle atom

          if (shape1 == ShapeData::trigonalPlanar) {
            isp2MiddleAtom = iatom1;
          } else {
            isp2MiddleAtom = iatom2;
          }

          if ( parameterserver->hasSp2Neighbor(isp2MiddleAtom) ) {
            ////////////////////////////////////////////////////////////////////
            // exception e3
            ////////////////////////////////////////////////////////////////////
            nPeriod = 3;  phi0 = 60.0;  Vphi = 2.0;
            Vphi = Vphi/nTorsionsAroundThisBond;

          } else {
            ////////////////////////////////////////////////////////////////////
            // case b
            ////////////////////////////////////////////////////////////////////
            nPeriod = 6;  phi0 = 0.0;  Vphi = 1.0;
            Vphi = Vphi/nTorsionsAroundThisBond;

          }
        } else {
          ////////////////////////////////////////////////////////////////////////
          // case c: both atms sp2 == trigonalPlanar == trigonal planar
          ////////////////////////////////////////////////////////////////////////
          nPeriod = 2;  phi0 = 180.0;
          Vphi = 5.0 * sqrt( u[parameterserver->period(atomicNumber1)] * 
                             u[parameterserver->period(atomicNumber2)] )
          * (1.0 + 4.18 * log(obnd32));
          Vphi = Vphi/nTorsionsAroundThisBond;
        }
        params[0] = Vphi;
        params[1] = cos(nPeriod * phi0 DEG);
        params[2] = nPeriod;
        torsion->setParams(params);
      } else {
        // Just set parameters to zero values to prevent access errors
        params[0] = 0.0;
        params[1] = 0.0;
        params[2] = 2.0;
        torsion->setParams(params);
      }
    }
  }
  delete parameterserver;
  delete[] params;
  return;
}

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Returns contribution to the forces from torsions.
////////////////////////////////////////////////////////////////////////////// 
void Torsions::forces(const FFCoordinates* coords, FFCoordinates* result)
{

  Torsion* torsion;
  int ntor,itor,iat,jat,kat,lat;
  double *tForce = new double[12];

  ntor = p_torsionList.size();
  const double *c1;
  const double *c2;
  const double *c3;
  const double *c4;
  for (itor=0; itor<ntor; itor++) {
    torsion = p_torsionList[itor];
    iat = torsion->atom1Index();
    jat = torsion->atom2Index();
    kat = torsion->atom3Index();
    lat = torsion->atom4Index();
    c1 = coords->row(iat);
    c2 = coords->row(jat);
    c3 = coords->row(kat);
    c4 = coords->row(lat);
    torsion->forceVector(c1,c2,c3,c4,tForce);
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

//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Returns the contribution to the energy (in kcal/mol) from these
//       torsions.
////////////////////////////////////////////////////////////////////////////// 
double Torsions::energy(const FFCoordinates* coords)
{

  Torsion* torsion;
  int ntor,itor,iat,jat,kat,lat;
  double ret = 0.0;

  ntor = p_torsionList.size();
  const double *c1;
  const double *c2;
  const double *c3;
  const double *c4;
  for (itor=0; itor<ntor; itor++) {
    torsion = p_torsionList[itor];
    iat = torsion->atom1Index();
    jat = torsion->atom2Index();
    kat = torsion->atom3Index();
    lat = torsion->atom4Index();
    c1 = coords->row(iat);
    c2 = coords->row(jat);
    c3 = coords->row(kat);
    c4 = coords->row(lat);
    ret += torsion->energy(c1,c2,c3,c4);
  }
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// Description:
//   Return total number of torsions in torsion list
//////////////////////////////////////////////////////////////////////////////
int Torsions::getCount() const
{
   return p_torsionList.size();
}
////////////////////////////////////////////////////////////////////////////////
// Description: Returns the torsion force field name where the name is made up 
//              of concatonated atomic symbols with the character '-' separating
//              the atom types.
//              The order of the atom types in the torsion type name is
//              determined by the alphabetical order of the atomic symbols of
//              the two inner atoms of the torsion.
//
// Param: idx is the index of the target torsion in the local torsion list.
//
// Return: torsion name. An empty string is returned if the index is invalid.
//
////////////////////////////////////////////////////////////////////////////////
string Torsions::getName(const int idx)
{
   string name;
   int *indices = getAtomOrderedIndices( idx);  // validates idx

   if (indices != 0)                           // idx is valid
   {
     // retrieve component names, construct torsion name
     name.append( (*p_atmptrs)[indices[0]]->atomicSymbol());
     int i;
     for ( i=1;i<4;i++)  {
        name.append( 1, '-');
        name.append( (*p_atmptrs)[indices[i]]->atomicSymbol().c_str());
     }
      delete [] indices;
   }
   return name;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Retrieves the torsion force field type name where the name is 
//              made up of concatonated atom types with the character '-' 
//              separating the atom types.
//              A carbon atom, C, may have several types, e.g., CT, CA, CS, ...
//              The order of the atom types in the torsion type name is
//              determined by the alphabetical order of the atomic symbols of
//              the two inner atoms of the torsion.
//
// Param: idx is the index of the target torsion in the local torsion list.
//
// Return: torsion type name. An empty string is returned if the index is 
//              invalid.
//
////////////////////////////////////////////////////////////////////////////////
string Torsions::getType( const int idx)
{
   string type;
   int *indices = getAtomOrderedIndices( idx);  // validates idx

   if (indices != 0)                           // idx is valid
   {
      type.append( (*p_atmptrs)[indices[0]]->cAtomType());
      int i;
      for ( i=1;i<4;i++)  {
        type.append( 1, '-');
        type.append( (*p_atmptrs)[indices[i]]->cAtomType());
      }
      delete [] indices;
   }
  return type;
}

////////////////////////////////////////////////////////////////////////////////// Description: Returns an array of the indices to the atoms of the torsion
//              specified in the input index. The order of the indices in
//              the returned array is determined by the alphanumberic 
//              order of the inner atoms' types.
//
// Param: torsionID is the index of the target torsion in the local torsion list
//
// Return: a pointer to an integer array containing the indices of component
//         atoms. If the torsion index is invalid, the returned pointer
//         is null.
//
////////////////////////////////////////////////////////////////////////////////
int* Torsions::getAtomOrderedIndices( const int torsionID)
{
   int *indices = 0;

   if ( isValidIndex( torsionID))
   {
      indices = new int[4];

      Torsion *torsion = p_torsionList[torsionID];
      int id2 = torsion->atom2Index();
      int id3 = torsion->atom3Index();
      char *t2 =  (*p_atmptrs)[id2]->cAtomType();
      char *t3 =  (*p_atmptrs)[id3]->cAtomType();
      if ( strcmp( t2, t3) < 0)  {
         indices[0] = torsion->atom1Index();
         indices[1] = id2;
         indices[2] = id3;
         indices[3] = torsion->atom4Index();
      } else {
         indices[0] = torsion->atom4Index();
         indices[1] = id3;
         indices[2] = id2;
         indices[3] = torsion->atom1Index();
      }
   }
   return indices;
}

////////////////////////////////////////////////////////////////////////////////
// Description: Returns the component atom indices for the specified torsion. If
//              the torsion index is out of range, the returned values for the
//              component atom indices are set to invalid values (-1).
//
// Param: idx index of the target torsion in this Torsions object.
// Param: atom1 pointer to index of the first atom
// Param: atom2 pointer to index of the second atom
// Param: atom3 pointer to index of the third atom
// Param: atom4 pointer to index of the fourth atom
//
// Return: none
//
////////////////////////////////////////////////////////////////////////////////
void Torsions::getAtomIndices( const int idx, int* atom1, int* atom2,
                                              int* atom3, int* atom4)
{
  if (isValidIndex(idx)) {
    Torsion *torsion = p_torsionList[idx];
    *atom1 = torsion->atom1Index();
    *atom2 = torsion->atom2Index();
    *atom3 = torsion->atom3Index();
    *atom4 = torsion->atom4Index();
  }
  else  {
    *atom1 = -1;
    *atom2 = -1;
    *atom3 = -1;
    *atom4 = -1;
  }
}

//////////////////////////////////////////////////////////////////////////////
//---------------------------Private Methods----------------------------------
//////////////////////////////////////////////////////////////////////////////
// Description:
//   Return true if idx is a valid index
//////////////////////////////////////////////////////////////////////////////
bool Torsions::isValidIndex(const int idx) const
{
  return idx >= 0 && idx < p_torsionList.size();
}
