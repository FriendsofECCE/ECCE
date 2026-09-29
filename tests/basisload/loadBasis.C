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
 *
 *   loadBasis <library-url> --oracle [composite]
 *
 * looks up every multi-file aggregate, puts the result in one group as the
 * Basis Set Tool does, and prints per aggregate the shells of each element
 * in the order TGBSConfig::dump() writes them, then dump("NWChem")'s own
 * text (by name, and with every primitive).  "composite" makes lookup() ignore the whole-set placeholder file,
 * i.e. load the set from its components, which is the reference the whole
 * file is compared with.  "sweep composite" does the same for --sweep.
 *
 * --dump and --complete are described where they are defined.
 */
#include <string.h>
#include <iostream>
#include <strstream>
#include <string>
#include <vector>
#include <set>
#include <stdio.h>

#include "util/Ecce.H"
#include "util/EcceURL.H"
#define private public
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#undef private
#define private public
#include "dsm/ICalcUtils.H"
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

//  Make lookup() see an aggregate without its "-AGG." placeholder, exactly
//  as a library whose placeholders are empty (or a client that skips them)
//  does.
static void hideWholeFiles(EDSIGaussianBasisSetLibrary& library)
{
  for (int t = 0; t <= (int)TGaussianBasisSet::charge; t++) {
    const vector<gbs_alias*>* aliases =
        library.getAliasList((TGaussianBasisSet::GBSType)t);
    if (aliases == 0) continue;
    for (size_t a = 0; a < aliases->size(); a++) {
      gbs_alias* alias = (*aliases)[a];
      if (alias->files.size() > 1 && strstr(alias->files[0], "-AGG."))
        alias->files.erase(alias->files.begin());
    }
  }
}

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

static void printShells(const TGBSGroup* group, ostream& out)
{
  set<string> elements;
  const vector<TGaussianBasisSet*>* list =
      const_cast<TGBSGroup*>(group)->getOrderedList();
  for (size_t i = 0; i < list->size(); i++)
    for (ContractionMap::iterator it = (*list)[i]->p_contractions.begin();
         it != (*list)[i]->p_contractions.end(); it++)
      elements.insert(it->first);
  char buf[64];
  for (set<string>::iterator e = elements.begin(); e != elements.end(); e++) {
    out << "element " << *e << endl;
    for (size_t i = 0; i < list->size(); i++) {
      ContractionMap::iterator it = (*list)[i]->p_contractions.find(*e);
      if (it == (*list)[i]->p_contractions.end()) continue;
      for (size_t c = 0; c < it->second->size(); c++) {
        Contraction_* cont = (*it->second)[c];
        out << "shell ";
        for (size_t k = 0; k < cont->shells.size(); k++)
          out << TGaussianBasisSet::shell_formatter[cont->shells[k]];
        out << " " << cont->num_exponents << " " << cont->num_coefficients
            << endl;
        for (size_t p = 0; p < cont->num_exponents; p++) {
          snprintf(buf, sizeof buf, "%.17g", cont->exponents[p]);
          out << buf;
          for (size_t k = 0; k < cont->num_coefficients; k++) {
            snprintf(buf, sizeof buf, " %.17g",
                     cont->coefficients[p * cont->num_coefficients + k]);
            out << buf;
          }
          out << endl;
        }
      }
    }
  }
}

static int oracle(EDSIGaussianBasisSetLibrary& library)
{
  for (int t = 0; t <= (int)TGaussianBasisSet::charge; t++) {
    TGaussianBasisSet::GBSType type = (TGaussianBasisSet::GBSType)t;
    const vector<gbs_alias*>* aliases = library.getAliasList(type);
    if (aliases == 0) continue;
    for (size_t a = 0; a < aliases->size(); a++) {
      const gbs_alias* alias = (*aliases)[a];
      size_t components = alias->files.size();
      if (components > 0 && strstr(alias->files[0], "-AGG.")) components--;
      if (components < 2) continue;
      vector<TGaussianBasisSet*> list =
          library.lookup(alias->nicename, type, 0);
      cout << "AGG " << TGaussianBasisSet::gbs_type_formatter[t] << "|"
           << alias->nicename << "|" << list.size() << endl;
      TGBSConfig config;
      TGBSGroup* group = new TGBSGroup();
      group->insertOrbitalGBS(alias->nicename, list, true);
      set<string> elements;
      for (size_t i = 0; i < list.size(); i++)
        for (ContractionMap::iterator it = list[i]->p_contractions.begin();
             it != list[i]->p_contractions.end(); it++)
          elements.insert(it->first);
      string tag;
      for (set<string>::iterator e = elements.begin(); e != elements.end(); e++)
        tag += (tag.empty() ? "" : " ") + *e;
      printShells(group, cout);
      if (!tag.empty()) {
        config.insertGBSGroup(tag, group);
        //  By name, as a saved calculation is written, and in full.
        for (int named = 1; named >= 0; named--) {
          const char* text = config.dump("NWChem", named != 0);
          cout << (named ? "dump named\n" : "dump explicit\n") << text << endl;
          delete [] text;
        }
      }
      cout << "END" << endl;
    }
  }
  return 0;
}

//  loadBasis <library-url> --dump <name> <type> <tag> named|explicit
//  prints TGBSConfig::dump("NWChem") of that one set: the .basis file
//  CalcEd hands ai.nwchem.
static int dumpOne(int argc, char** argv)
{
  if (argc != 7) {
    cerr << "usage: loadBasis <library-url> --dump <name> <type> <tag> "
            "named|explicit\n";
    return 2;
  }
  Ecce::initialize();
  const EcceURL libraryUrl(argv[1]);
  EDSIGaussianBasisSetLibrary library(libraryUrl);
  TGBSConfig config;
  vector<TGaussianBasisSet*> list = library.lookup(
      argv[3], TGaussianBasisSet::strToType(argv[4]), argv[5]);
  TGBSGroup* group = new TGBSGroup();
  group->insertOrbitalGBS(argv[3], list, true);
  config.insertGBSGroup(argv[5], group);
  const char* text = config.dump("NWChem", string(argv[6]) == "named");
  cout << text << endl;
  delete [] text;
  return 0;
}

//  loadBasis <library-url> --complete <name> <type> <tag>
//  builds the group a stored calculation from an older release holds (the
//  aggregate as its components) and completes it for <tag> the way
//  ICalcUtils does when the molecule changes; prints what is in the result.
static int completeOne(int argc, char** argv)
{
  if (argc != 6) {
    cerr << "usage: loadBasis <library-url> --complete <name> <type> <tag>\n";
    return 2;
  }
  Ecce::initialize();
  const EcceURL libraryUrl(argv[1]);
  EDSIGaussianBasisSetLibrary library(libraryUrl);
  //  ICalcUtils built its own library at static-initialisation time, before
  //  this process knew which server to ask.
  delete ICalcUtils::p_gbsFactory;
  ICalcUtils::p_gbsFactory = new EDSIGaussianBasisSetLibrary(libraryUrl);
  hideWholeFiles(library);
  vector<TGaussianBasisSet*> list = library.lookup(
      argv[3], TGaussianBasisSet::strToType(argv[4]), argv[5]);
  TGBSGroup* stored = new TGBSGroup();
  stored->insertOrbitalGBS(argv[3], list, true);
  TGBSGroup* group = ICalcUtils::completeGroup(stored, argv[5]);
  const vector<TGaussianBasisSet*>* sets = group->getOrderedList();
  for (size_t i = 0; i < sets->size(); i++)
    cout << "SET " << (*sets)[i]->p_name << "|"
         << TGaussianBasisSet::gbs_type_formatter[(*sets)[i]->p_type] << endl;
  printShells(group, cout);
  return 0;
}

//  loadBasis - --collide
//  Two distinct parts of one aggregate with the same name and type (the
//  #164 shape) must warn; the same part inserted twice must not.  No
//  server needed: this exercises TGBSGroup alone.
static int collide()
{
  TGaussianBasisSet::GBSType pople = TGaussianBasisSet::pople;
  string name = "6-31G*";
  //  Separate objects per group: a group owns what it inserts.
  TGaussianBasisSet* a = new TGaussianBasisSet(pople, name);
  TGaussianBasisSet* b = new TGaussianBasisSet(pople, name);
  TGaussianBasisSet* c = new TGaussianBasisSet(pople, name);
  vector<TGaussianBasisSet*> same, twins;
  same.push_back(a);
  same.push_back(a);
  twins.push_back(b);
  twins.push_back(c);
  cerr << "CASE same\n";
  TGBSGroup g1;
  g1.insertOrbitalGBS("6-31G*", same, false);
  cerr << "CASE twins\n";
  TGBSGroup g2;
  g2.insertOrbitalGBS("6-31G*", twins, false);
  return 0;
}

int main(int argc, char** argv)
{
  if (argc >= 3 && string(argv[2]) == "--collide") return collide();
  if (argc >= 3 && string(argv[2]) == "--complete")
    return completeOne(argc, argv);
  if (argc >= 3 && string(argv[2]) == "--dump") return dumpOne(argc, argv);
  if (argc >= 3 && string(argv[2]) == "--oracle") {
    Ecce::initialize();
    const EcceURL libraryUrl(argv[1]);
    EDSIGaussianBasisSetLibrary library(libraryUrl);
    if (argc == 4 && string(argv[3]) == "composite") hideWholeFiles(library);
    return oracle(library);
  }
  if (argc >= 3 && string(argv[2]) == "--sweep") {
    Ecce::initialize();
    const EcceURL libraryUrl(argv[1]);
    EDSIGaussianBasisSetLibrary library(libraryUrl);
    if (argc == 4 && string(argv[3]) == "composite") hideWholeFiles(library);
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
