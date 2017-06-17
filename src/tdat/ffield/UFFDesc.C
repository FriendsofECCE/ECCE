///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: UFFDesc.C
//
//
// DESIGN:
//   
//
///////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <string>
  using std::string;


#include "tdat/UFFDesc.H"

UFFDesc::UFFDesc(void)
{
  setFFRulesName("UFF");
}

UFFDesc::~UFFDesc(void)
{
}


////////////////////////////////////////////////////////////////////////////////
//  Description:
//    Check to see if an atom center has improper torsions
////////////////////////////////////////////////////////////////////////////////
bool UFFDesc::checkInversion(TAtm *atm, vector<TBond*> list) const
{
  int nbnd = list.size();
  if (nbnd != 3) return false;
  ShapeData::Shape shape = atm->shape();
  double obnd21, obnd31, obnd41;
  if (!(shape == ShapeData::pyramidal
      || shape == ShapeData::trigonalPlanar)) return false;
  // check to make sure that center atom is either in N or O column of the
  // periodic table for pyramidal geometries or that it is a C atom with
  // sp2 geometry.
  int atomNumber = atm->atomicNumber();
  if ((shape == ShapeData::pyramidal && (atomNumber == 7 ||
       atomNumber == 8 || atomNumber == 15 || atomNumber == 16 ||
       atomNumber == 33 || atomNumber == 34 || atomNumber == 51 ||
       atomNumber == 52 || atomNumber == 83 || atomNumber == 84)) ||
       (shape == ShapeData::trigonalPlanar && atomNumber == 6)) {
    // check to make sure that there are no bonds with bond order less than
    // 1.0
    obnd21 = list[0]->order();
    obnd31 = list[1]->order();
    obnd41 = list[2]->order();
    if (obnd21 < 0.99 || obnd31 < 0.99 || obnd41 < 0.99) return false;
    return true;
  }
  return false;
}

vector<bool> UFFDesc::findInversions(TAtm *atm, vector<TBond*> list,
                                     vector<int> *ringAtom) const
{
  vector<bool> ret;
  ret.push_back(true);
  ret.push_back(true);
  ret.push_back(true);
  return ret;
}
