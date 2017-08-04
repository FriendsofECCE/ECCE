#include <iostream.h>
#include <strstream.h>

#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/EDSIServerCentral.H"
#include "tdat/TGaussianBasisSet.H"
#include "tdat/TGBSConfig.H"
#include "tdat/Fragment.H"
#include "tdat/TTCPair.H"

int main(int argc, char** argv)
{


  static string xyz = "8\n";
  xyz +=  "Ethane\n";
  xyz +=  "C   -0.537090    0.148330   -0.000410\n";
  xyz +=  "C    0.965490    0.079930    0.000250\n";
  xyz +=  "H   -0.900450    1.054480   -0.502500\n";
  xyz +=  "H   -0.979670   -0.711650   -0.519840\n";
  xyz +=  "H   -0.942390    0.156680    1.019810\n";
  xyz +=  "H    1.409980    0.944880    0.509490\n";
  xyz +=  "H    1.329880   -0.820590    0.511890\n";
  xyz +=  "H    1.370610    0.061380   -1.019850\n";

  Fragment frag;

  istrstream istr(xyz.data());

  frag.restoreXYZ(istr,true);

  string name = "STO-3G*";


  EDSIServerCentral central;
  EDSIGaussianBasisSetLibrary *gbsFactory = 
      new EDSIGaussianBasisSetLibrary(central.getDefaultBasisSetLibrary());

  vector<TGaussianBasisSet*> tgbs = gbsFactory->lookup(name.data(), TGaussianBasisSet::pople);
  if (tgbs.empty()) {
    cout << "UNable to lookup " << name << endl;
  } else {
    TGBSConfig *config = new TGBSConfig();
    // Get vector of atoms or symbols from fragment
    // What I really need is unique pairs
//    vector<TTagCountPair> *tcounts = frag.tagCounts();
//    int nAtoms = tcounts->size();
//    string symbol;
//    for (int idx=0; idx<nAtoms; idx++) {
//      symbol += tcounts->at(idx).tag.data();
//      symbol += " ";
//    }
//    delete tcounts;
//    config->insertGBS(symbol.data(), tgbs);

    // check isConsistent and isComplete and return null if there are
    // errors??? or have application do that?

//    config->type(TGBSConfig::simple);
//    config->name(name);
//    config->coordsys(Cartesian);
//    config->optimize(true);

    const char * output = config->dump("NWChem");
    cout << "NWChem: " << endl;
    cout << output << endl;

    // maybe get rid of max_l_code modifier - don't know what its for


  }

  return 0;
}
