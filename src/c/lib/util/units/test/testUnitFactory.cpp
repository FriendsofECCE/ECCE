#include <iostream.h>
#include <strstream.h>

#include "util/UnitFactory.H"
#include "util/UnitFamily.H"

int main(int argc, char** argv)
{


  UnitFactory& uf = UnitFactory::getInstance();

  const UnitFamily& si = uf.getUnitFamily("SI");

  const UnitConverter& uc = uf.getUnitConverter("Null");

  vector<string> names;
  int idx;
  
  names = uf.getConverterNames();
  cout << "Converter Classes " << endl;
  for (idx=0; idx<names.size(); idx++) {
     cout << "\t" << names[idx] << endl;
  }

  names = uf.getFamilyNames();
  cout << "Families " << endl;
  for (idx=0; idx<names.size(); idx++) {
     cout << "\t" << names[idx] << endl;
  }


  return 0;
}
