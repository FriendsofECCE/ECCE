/*
 * loadBasis -- load a basis set from a REAL data server the way the Basis
 * Set Tool and CalcEd do, and print the BasisSet.ecce_basisset document
 * CalcEd would store for it.
 *
 *   loadBasis <library-url> <name> <type|quick> <tag>
 *
 * <type> is the list the user picked it from (pople, ECPOrbital, ...):
 * the Basis Set Tool's updateGroup() looks up with that type and inserts
 * with TGBSGroup::insertOrbitalGBS().  "quick" is CalcEd's Quick Basis
 * menu, EDSIGaussianBasisSetLibrary::simpleLookup(), which looks up with
 * UnknownGBSType.
 *
 * The document goes through DavCalculation's own writers -- the ones
 * gbsConfig(TGBSConfig*) uses to store the file -- so what is checked is
 * what a saved calculation holds, not a reconstruction of it.  Those
 * writers are private and touch no member state, hence the #define.
 *
 * stderr gets one line per basis set lookup() returned, before any group
 * deduplication: "LOOKUP <name>|<type>|<elements>".
 *
 *   loadBasis <library-url> --sweep
 *
 * looks up EVERY multi-file aggregate in every index and prints, per
 * aggregate, "AGG <index>|<name>|<files>" followed by one
 * "COMP <file>|<name>|<type>|<nelements>" line per component lookup()
 * returned.  The checks are made by run_tests.py.
 */
#include <string.h>
#include <iostream>
#include <strstream>
#include <string>
#include <vector>

#include "util/Ecce.H"
#include "util/EcceURL.H"
#define private public
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#undef private
#include "dsm/TGBSConfig.H"
#include "dsm/TGBSGroup.H"
#include "dsm/TGaussianBasisSet.H"
#define private public
#define protected public
#include "dsm/DavCalculation.H"
#undef private
#undef protected

using namespace std;

static int sweep(EDSIGaussianBasisSetLibrary& library)
{
  for (int t = 0; t <= (int)TGaussianBasisSet::charge; t++) {
    TGaussianBasisSet::GBSType type = (TGaussianBasisSet::GBSType)t;
    const vector<gbs_alias*>* aliases = library.getAliasList(type);
    if (aliases == 0) continue;
    for (size_t a = 0; a < aliases->size(); a++) {
      const gbs_alias* alias = (*aliases)[a];
      vector<string> files;
      for (size_t f = 0; f < alias->files.size(); f++)
        if (!strstr(alias->files[f], "-AGG.")) files.push_back(alias->files[f]);
      if (files.size() < 2) continue;
      cout << "AGG " << TGaussianBasisSet::gbs_type_formatter[t] << "|"
           << alias->nicename << "|";
      for (size_t f = 0; f < files.size(); f++) cout << files[f] << " ";
      cout << endl;
      TGaussianBasisSet::GBSType lookupType = type;
      vector<TGaussianBasisSet*> list =
          library.lookup(alias->nicename, lookupType, 0);
      //  lookup() skips a file it cannot read, so components and files
      //  are matched by position only when every one came back.
      for (size_t i = 0; i < list.size(); i++) {
        cout << "COMP " << (list.size() == files.size() ? files[i] : "?")
             << "|" << list[i]->p_name << "|"
             << TGaussianBasisSet::gbs_type_formatter[list[i]->p_type] << "|"
             << list[i]->p_contractions.size() << endl;
        delete list[i];
      }
    }
  }
  return 0;
}

int main(int argc, char** argv)
{
  if (argc == 3 && string(argv[2]) == "--sweep") {
    Ecce::initialize();
    const EcceURL libraryUrl(argv[1]);
    EDSIGaussianBasisSetLibrary library(libraryUrl);
    return sweep(library);
  }
  if (argc != 5) {
    cerr << "usage: loadBasis <library-url> <name> <type|quick> <tag>\n";
    return 2;
  }
  Ecce::initialize();
  const EcceURL libraryUrl(argv[1]);
  EDSIGaussianBasisSetLibrary library(libraryUrl);
  const string name = argv[2], how = argv[3], tag = argv[4];

  TGBSConfig* config = 0;
  vector<TGaussianBasisSet*> gbsList;
  if (how == "quick") {
    config = library.simpleLookup(name.c_str(), tag.c_str());
  } else {
    TGaussianBasisSet::GBSType type = TGaussianBasisSet::strToType(how);
    gbsList = library.lookup(name.c_str(), type, tag.c_str());
    config = new TGBSConfig;
    TGBSGroup* group = new TGBSGroup();
    group->insertOrbitalGBS(name, gbsList, true);
    config->insertGBSGroup(tag, group);
  }
  if (how != "quick") {
    for (size_t i = 0; i < gbsList.size(); i++) {
      cerr << "LOOKUP " << gbsList[i]->p_name << "|"
           << TGaussianBasisSet::gbs_type_formatter[gbsList[i]->p_type] << "|";
      for (ContractionMap::iterator it = gbsList[i]->p_contractions.begin();
           it != gbsList[i]->p_contractions.end(); it++)
        cerr << it->first << " ";
      cerr << endl;
    }
  }

  DavCalculation calc(EcceURL("http://localhost/unused"));
  ostrstream output;
  output << "<MolecularConfig>\n";
  calc.writeConfigMapElement(config, output);
  calc.writeConfigDataElement(config, output);
  output << "</MolecularConfig>" << ends;
  char* text = output.str();
  cout << text;
  delete [] text;
  return 0;
}
