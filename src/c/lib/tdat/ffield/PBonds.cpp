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
#include "tdat/PBonds.H"
#include "tdat/PBond.H"
#include "tdat/UFFParams.H"
#include "tdat/TPerTab.H"
#include "tdat/FFCoords.H"
#include "tdat/FFRules.H"

const char* PBonds::BOND_LINK = "-";
const char* PBonds::NUB_STRING = "Nub";  // string as tested in Fragment.C
                                         // not sure where set.  TODO

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:  
//       Create a class containing all bonds in the system. If the vector
//       p_mask is defined then forces are only calculated on those atoms for
//       which p_mask is true.
////////////////////////////////////////////////////////////////////////////// 
PBonds::PBonds( vector<TAtm*>* atmptrs, FFRules *rules)
{
  p_atmptrs = atmptrs;
  p_mask = (vector<bool>*)0;
  p_rules = rules;
  initBondList();
  if (rules->getFFRulesName() == "UFF") setBondParameters();
}

PBonds::PBonds( vector<TAtm*>* atmptrs, vector<bool>* mask, FFRules *rules)
{
  p_atmptrs = atmptrs;
  p_mask = mask;
  p_rules = rules;
  initBondList();
  if (rules->getFFRulesName() == "UFF") setBondParameters();
}

//////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description:
//      Copy constructor.  Uses assignment operator.
////////////////////////////////////////////////////////////////////////////// 
PBonds::PBonds(const PBonds& bonds)
{ (*this) = bonds; }
 
// ---------- Virtual Destructor ------------
PBonds::~PBonds(void)
{
  for (int i=0; i<p_bondList.size(); i++) delete p_bondList[i];
}

// ---------- General Methods  ----------
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Construct a list of bonds. Each bond is represented in the list only once.
////////////////////////////////////////////////////////////////////////////////
void PBonds::initBondList(void)
{  
  int natoms = p_atmptrs->size();
  int iat=0, jat=0;
  TAtm *atm1;
  // Construct list if all atoms selected
  if (p_mask == (vector<bool>*)0) {
    for (iat=0; iat<natoms; iat++) {
      atm1 = (*p_atmptrs)[iat];
      vector<TAtm*>* nghbrs = atm1->connectedAtoms();
      for (jat=0; jat < nghbrs->size(); jat++) {
        // This condition avoids duplicates
        if (iat < (*nghbrs)[jat]->index()) {
          PBond *bond = new PBond(iat,(*nghbrs)[jat]->index());
          p_bondList.push_back(bond);
        }
      }
      delete nghbrs;
    }
  } else {
  // Construct list if only some atoms selected
    for (iat=0; iat<natoms; iat++) {
      atm1 = (*p_atmptrs)[iat];
      vector<TAtm*>* nghbrs = atm1->connectedAtoms();
      for (jat=0; jat < nghbrs->size(); jat++) {
        if ((*p_mask)[iat] || (*p_mask)[jat]) {
          // This condition avoids duplicates
          if ((iat < (*nghbrs)[jat]->index() && (*p_mask)[iat] &&
                (*p_mask)[jat]) || (*p_mask)[iat] != (*p_mask)[jat]) {
            PBond *bond = new PBond(iat,(*nghbrs)[jat]->index());
            p_bondList.push_back(bond);
          }
        }
      }
    }
  }
}
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//    Assign parameters to all bonds in bond list using the Universal Force
//    Field.
////////////////////////////////////////////////////////////////////////////////
void PBonds::setBondParameters(void)
{
  static const double ballScale = 0.67;
  TPerTab tpt;

  UFFParams *parameterserver = new UFFParams(p_atmptrs);
  int nbonds = p_bondList.size();
  int ibond=0;
  int iat, jat, kat, lat;
  double bondOrder;
  TAtm *atm1, *atm2;
  for (ibond=0; ibond<nbonds; ibond++) {
    iat = p_bondList[ibond]->atom1Index();
    jat = p_bondList[ibond]->atom2Index();
    int uffTypeIndex1 = parameterserver->uffIndex(iat);
    int uffTypeIndex2 = parameterserver->uffIndex(jat);

    // Find bond order. This is not pretty.
    atm1 = (*p_atmptrs)[iat];
    const vector<TBond*>& bonds = atm1->bondList();
    int jbond = 0;
    for (jbond=0; jbond < bonds.size(); jbond++) {
      kat = bonds[jbond]->atom1()->index();
      lat = bonds[jbond]->atom2()->index();
      if (kat == jat || lat == jat) {
        bondOrder = bonds[jbond]->order();
        break;
      }
    }
    double r12 = parameterserver->r12(uffTypeIndex1, uffTypeIndex2,
                                        bondOrder);
    double zStar1 = parameterserver->effCharge(uffTypeIndex1);
    double zStar2 = parameterserver->effCharge(uffTypeIndex2);
    double r12cubed = r12 * r12 * r12;

    //angstrom, kcal/mol
    double forceConst = 664.12 * zStar1 * zStar2 / r12cubed;

    // nub bond force constant set to 500.0
    // also set bond length equal to display radius so that
    // the nub sphere will appear half-submerged in its supporting atom.
    // ballScale must match the value in VRSimpleFrag.C


    if (uffTypeIndex1 == 133) {
      r12 = tpt.covalentRadius(atm1->atomicNumber()) * ballScale;
      forceConst = 500.0;
    } else if (uffTypeIndex2 == 133) {
      atm2 = (*p_atmptrs)[jat];
      r12 = tpt.covalentRadius(atm2->atomicNumber()) * ballScale;
      forceConst = 500.0;
    }

    p_bondList[ibond]->setParams(r12, forceConst);
  }
  delete parameterserver;
}
////////////////////////////////////////////////////////////////////////////////
//
//  Description:
//     Calculate contributions to forces from all harmonic bonds.
////////////////////////////////////////////////////////////////////////////////
void PBonds::forces(const FFCoordinates* coords, FFCoordinates* result)
{  
  // loop over bonds
  PBond* bond;
  int iat, jat;
  double *bForce;
  bForce = new double[6];
  int cnt = p_bondList.size();

  const double *c1;
  const double *c2;
  for(int idx=0;idx<cnt;idx++) {
    bond = p_bondList[idx];
    iat = bond->atom1Index();
    jat = bond->atom2Index();
    c1 = coords->row(iat);
    c2 = coords->row(jat);
    bond->forceVector(c1, c2, bForce);
    if (p_mask == (vector<bool>*)0) {
      result->add(iat, bForce);
      result->add(jat, bForce+3);
    } else {
      if ((*p_mask)[iat]) result->add(iat, bForce);
      if ((*p_mask)[jat]) result->add(jat, bForce+3);
    }
  }
  delete[] bForce;
}
//////////////////////////////////////////////////////////////////////////////
//
//  Description:
//       Calculate contribution to the energy from all harmonic bonds.
////////////////////////////////////////////////////////////////////////////// 
double PBonds::energy(const FFCoordinates* coords) const
{ 
  double ret=0.0;
  int i,iat,jat;
  const double *c1;
  const double *c2;
  PBond *bond;
  for (i=0; i<p_bondList.size(); i++) {
    bond = p_bondList[i];
    iat = bond->atom1Index();
    jat = bond->atom2Index();
    c1 = coords->row(iat);
    c2 = coords->row(jat);
    ret += bond->energy(c1, c2);
  }
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
//  Description: Returns the number of bonds
//
////////////////////////////////////////////////////////////////////////////// 
int PBonds::getCount( void) const
{
   return p_bondList.size();
}

//////////////////////////////////////////////////////////////////////////////
//  Description: Returns the type of the bond indexed by the function argument.
//               A null pointer is returned if the index is not valid.
//               The type of the bond is created by linking the type of
//               each atom with the link character.
//
// Param: idx is the index of the target bond 
// Param: type is one of two enumerated types of BOND::STRING_TYPE
//
// Return: A pointer to a dynamically allocated string with the name or type
//         of the specified bond.
//
////////////////////////////////////////////////////////////////////////////// 
string PBonds::getString( const int idx, const STRING_TYPE stype)
{
   string rString;

   // Retrieve ordered component atom indices
   const int *indices = getAtomOrderedIndices( idx);  // validates idx value
   if ( indices != 0)                                 // idx was valid
   {
      // Build the name string
      switch ( stype)  {
         case BOND_NAME: rString.append( (*p_atmptrs)[indices[0]]->cAtomName());
                         rString.append( BOND_LINK);
                         rString.append( (*p_atmptrs)[indices[1]]->cAtomName());
                         break;
         case BOND_TYPE: rString.append( (*p_atmptrs)[indices[0]]->cAtomType());
                         rString.append( BOND_LINK);
                         rString.append( (*p_atmptrs)[indices[1]]->cAtomType());
         default:        break;
      }
      delete [] indices;
   }
   return rString;
}

//////////////////////////////////////////////////////////////////////////////
// Description: Returns an array of ordered conponent atom indices for the 
//               specified bond.  The order is set by the alphabetical order
//               of the conponent atom type names.
//
// Param: idx is the index of the target bond in this PBonds object.
//
// Return: index of the atom at the specificed location in the specified bond.
//
////////////////////////////////////////////////////////////////////////////// 
int* PBonds::getAtomOrderedIndices( const int idx) const
{
   int* indices = (int*)0;

   if ( isValidIndex( idx))
   {
      indices = new int[2];

      // Find the component atom indices.
      PBond *bond = p_bondList[idx];
      int id1 = bond->atom1Index();
      int id2 = bond->atom2Index();
      
      // Order the component atom indices.
      if ( strcmp(  (*p_atmptrs)[id1]->cAtomType(), 
                    (*p_atmptrs)[id2]->cAtomType()) <= 0)  {
         indices[0] = id1;
         indices[1] = id2;
      }
      else  {
         indices[0] = id2;
         indices[1] = id1;
      }
   }
   return indices;
}

//////////////////////////////////////////////////////////////////////////////
// Description: Returns the component atom indices for the specified bond. If
//              the bond index is out of range, the returned values for the
//              component atom indices are set to invalid values (-1).
//
// Param: idx index of the target bond in this PBonds object.
// Param: atom1 pointer to index of the first atom
// Param: atom2 pointer to index of the second atom
//
// Return: none
//
////////////////////////////////////////////////////////////////////////////// 
void PBonds::getAtomIndices( const int idx, int* atom1, int* atom2)
{
   if ( isValidIndex( idx))
   {
      *atom1 = p_bondList[idx]->atom1Index();
      *atom2 = p_bondList[idx]->atom2Index();
   }
   else  {
      *atom1 = -1;
      *atom2 = -1;
   }
}

//////////////////////////////////////////////////////////////////////////////
//----------------------private methods---------------------------------------
//////////////////////////////////////////////////////////////////////////////
//  Description: Returns true if the input index is within range.
//
////////////////////////////////////////////////////////////////////////////// 
bool PBonds::isValidIndex( const int idx) const
{
   return idx >= 0 && idx < p_bondList.size();
}

