//  Are the Basis Set Tool's two name rules right?
//
//      tests/basis/testBasisNameRules <path to data/admin/basissets>
//
//  Both rules decide something the user never sees spelled out, and both
//  fail silently when wrong:
//
//    * matchesFilter() (#117) decides which of 377 basis sets a list box
//      shows.  Too strict and a set the library really has becomes
//      unreachable, with nothing to say so.
//
//    * pairedECP() (#122) decides which effective core potential is
//      offered alongside an orbital basis set.  Miss a pairing and the
//      offer simply never appears, which is indistinguishable from the
//      old behaviour; make a WRONG pairing and the tool offers a
//      potential that does not belong to the basis at all.
//
//  So both are checked against the SHIPPED library index files rather
//  than against invented names -- a library refresh that renames a set or
//  drops a .POT should fail here, not in front of a user.
//
//  Compiled with plain g++: GBSNameRules deliberately has no ECCE
//  dependency, so this needs no build tree.  See tests/basis/run_tests.py.

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "dsm/GBSNameRules.H"

using namespace std;

static int failures = 0;

//  std::map::operator[] INSERTS on a miss, which would quietly inflate the
//  pairing count this test reports; look up without touching the map.
static string lookup(const map<string, string>& m, const string& key)
{
  map<string, string>::const_iterator it = m.find(key);
  return (it == m.end()) ? string("") : it->second;
}

static void check(bool ok, const string& what)
{
  if (!ok) {
    cout << "  FAIL: " << what << endl;
    failures++;
  }
}


//  One record of a basis set index file: a name and the files it names.
struct Record {
  string name;
  vector<string> files;
};


static vector<Record> readIndex(const string& path)
{
  vector<Record> records;
  ifstream in(path.c_str());
  string line;

  while (getline(in, line)) {
    //  Trailing whitespace is real in these files (several files= lines
    //  end in a space), so trim before splitting.
    while (!line.empty() && (line[line.size()-1] == ' ' ||
                             line[line.size()-1] == '\t' ||
                             line[line.size()-1] == '\r')) {
      line.erase(line.size()-1);
    }

    if (line.compare(0, 6, "name= ") == 0) {
      Record r;
      r.name = line.substr(6);
      records.push_back(r);
    } else if (line.compare(0, 7, "files= ") == 0 && !records.empty()) {
      string rest = line.substr(7);
      string::size_type pos = 0;
      while (pos < rest.size()) {
        while (pos < rest.size() && rest[pos] == ' ') pos++;
        string::size_type start = pos;
        while (pos < rest.size() && rest[pos] != ' ') pos++;
        if (pos > start) {
          records.back().files.push_back(rest.substr(start, pos - start));
        }
      }
    }
  }

  return records;
}


int main(int argc, char** argv)
{
  string dir = (argc > 1) ? argv[1] : "../../data/admin/basissets";

  //  ---------------------------------------------------------------
  //  #117 -- the filter
  //  ---------------------------------------------------------------
  check(GBSNameRules::matchesFilter("cc-pVTZ", ""),
        "an empty filter lists everything (that is the 'all' option)");
  check(GBSNameRules::matchesFilter("cc-pVTZ", "   \t "),
        "a whitespace-only filter lists everything too");
  check(GBSNameRules::matchesFilter("cc-pVTZ-PP", "CC-PV"),
        "the filter ignores case");
  check(GBSNameRules::matchesFilter("cc-pVTZ-PP", "cc pp"),
        "several words all have to match, in any order");
  check(!GBSNameRules::matchesFilter("cc-pVTZ", "cc pp"),
        "...and a name missing one of them is rejected");
  check(GBSNameRules::matchesFilter("6-31G(3df,3pd)", "31 3df"),
        "punctuation in a name is matched literally");
  check(!GBSNameRules::matchesFilter("cc-pVDZ", "def2"),
        "an unrelated name is filtered out");
  check(GBSNameRules::matchesFilter("def2-svp(p)", "def2-svp("),
        "a parenthesis in the filter is not a metacharacter");

  //  The point of the whole exercise: the filter has to collapse a real
  //  list to something scrollable.  Measured against the shipped index.
  vector<Record> cc = readIndex(dir + "/correlation_consistent");
  check(cc.size() > 90, "read the correlation_consistent index");

  int all = 0, augOnly = 0, def2Only = 0;
  size_t i;
  for (i = 0; i < cc.size(); i++) {
    if (GBSNameRules::matchesFilter(cc[i].name, "")) all++;
    if (GBSNameRules::matchesFilter(cc[i].name, "aug-cc-pV")) augOnly++;
    if (GBSNameRules::matchesFilter(cc[i].name, "def2")) def2Only++;
  }
  check(all == (int)cc.size(), "no filter hides nothing");
  check(augOnly > 5 && augOnly < all / 2,
        "'aug-cc-pV' cuts the correlation consistent list down sharply");
  check(def2Only == 0, "'def2' finds nothing among the Dunning sets");

  //  ---------------------------------------------------------------
  //  #122 -- the basis / ECP pairing
  //  ---------------------------------------------------------------
  vector<Record> orbitals = readIndex(dir + "/ECPOrbital");
  vector<Record> ecps = readIndex(dir + "/ecp");

  check(orbitals.size() > 30, "read the ECPOrbital index");
  check(ecps.size() > 20, "read the ecp index");

  vector<string> ecpNames, ecpFiles;
  for (i = 0; i < ecps.size(); i++) {
    ecpNames.push_back(ecps[i].name);
    ecpFiles.push_back(ecps[i].files.empty() ? string("") : ecps[i].files[0]);
  }

  map<string, string> paired;
  vector<string> unpaired;

  for (i = 0; i < orbitals.size(); i++) {
    string match = GBSNameRules::pairedECP(orbitals[i].name,
                                           orbitals[i].files,
                                           ecpNames, ecpFiles);
    if (match.empty()) {
      unpaired.push_back(orbitals[i].name);
    } else {
      paired[orbitals[i].name] = match;
    }
  }

  //  The pairing has to be a name the ecp index really carries, or the
  //  tool would offer something the library cannot then look up.
  map<string, string>::const_iterator pit;
  for (pit = paired.begin(); pit != paired.end(); pit++) {
    bool known = false;
    for (i = 0; i < ecpNames.size(); i++) {
      if (ecpNames[i] == pit->second) known = true;
    }
    check(known, pit->first + " pairs with an ecp the index really lists");
  }

  //  The three routes, each with a case that ONLY it can find.
  check(lookup(paired, "def2-svp") == "def2-svp-ecp",
        "def2-svp pairs with def2-svp-ecp (found by name: every def2 "
        "orbital record points at a shared DEF2_ECP.POT the ecp index "
        "does not list at all)");
  check(lookup(paired, "def2-qzvppd") == "def2-qzvppd-ecp",
        "and so does the rest of the def2 family");
  check(lookup(paired, "LANL2TZ") == "LANL2TZ-ecp",
        "LANL2TZ pairs with LANL2TZ-ecp");
  check(lookup(paired, "LANL2DZ ECP") == "LANL2DZ ECP",
        "LANL2DZ ECP is listed under the same name in both indexes");
  check(lookup(paired, "cc-pVDZ-PP") == "Stuttgart-Koeln MCDHF RSC ECP",
        "cc-pVDZ-PP pairs through the .POT file it names (nothing in its "
        "name says Stuttgart)");
  check(lookup(paired, "aug-cc-pVQZ-PP") == "Stuttgart-Koeln MCDHF RSC ECP",
        "...and so does the rest of the pseudopotential cc family");
  check(lookup(paired, "SDB-cc-pVTZ") == "SDB RLC ECP",
        "SDB-cc-pVTZ pairs through its .POT file too");

  //  A pairing must never be invented: the ecp has to be one whose own
  //  .POT file the orbital set names, OR one whose name follows the
  //  convention.  Spot-check the one shape that could go wrong quietly.
  check(lookup(paired, "Hay-Wadt MB (n+1) ECP") == "HAY/WADT (N-1) ECP",
        "Hay-Wadt MB pairs through HAY-WADT2.POT -- the two indexes spell "
        "the same potential differently, so only the file can match them");

  //  Whatever is left over is reported, so that a library refresh which
  //  breaks a pairing shows up as a CHANGE rather than as silence.  The
  //  tool simply makes no offer for these, which is the old behaviour.
  cout << "  " << paired.size() << " of " << orbitals.size()
       << " ECP orbital sets have a matching potential" << endl;
  for (i = 0; i < unpaired.size(); i++) {
    cout << "    no ECP paired with: " << unpaired[i] << endl;
  }
  check(unpaired.empty(),
        "every ECP orbital set in the shipped library has a potential "
        "(they all do today -- a new one that does not is worth knowing "
        "about, since the tool simply makes no offer for it)");

  if (failures == 0) {
    cout << "  basis set name rules: PASS" << endl;
  }
  return failures == 0 ? 0 : 1;
}
