// CrestCheck::warning (#831): all conformers removed by the topology check,
// the best conformer's bonding against the input's, and a normal run.

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "comm/CrestCheck.H"

using namespace std;

static int failures = 0;
static void check(const string& name, bool ok)
{
  cout << (ok ? "ok   " : "FAIL ") << name << endl;
  if (!ok) failures++;
}

static string slurp(const string& path)
{
  ifstream in(path.c_str());
  ostringstream oss;
  oss << in.rdbuf();
  return oss.str();
}

// Stand-in for covalent radius times tolerance: Hg-S bonds up to 2.6, S-S to 2.2.
static double radius(const string& s)
{
  return s == "Hg" ? 1.5 : s == "S" ? 1.1 : 0.7;
}

int main(int argc, char** argv)
{
  const string dir = argc > 1 ? argv[1] : ".";
  string w = CrestCheck::warning(slurp(dir + "/crest/all-removed.crestout"), radius);
  cout << w << endl;
  check("all removed: topology warning",
        w.find("removed by the topology check; the result is the input structure") != string::npos);

  w = CrestCheck::warning(slurp(dir + "/crest/bonding-changed.crestout"), radius);
  cout << w << endl;
  check("bonding changed: bond made S1\xe2\x80\x93S2 with its distance",
        w.find("bond made S1\xe2\x80\x93S2 (2.03 \xc3\x85)") != string::npos);
  check("bonding changed: bonds broken Hg1\xe2\x80\x93S1, Hg1\xe2\x80\x93S2",
        w.find("bonds broken Hg1\xe2\x80\x93S1, Hg1\xe2\x80\x93S2") != string::npos);

  w = CrestCheck::warning(slurp(dir + "/crest/normal.crestout"), radius);
  check("normal run: no warning", w.empty());
  check("no ensemble: no warning", CrestCheck::warning("CREST output only", radius).empty());

  string protonate = slurp(dir + "/crest/bonding-changed.crestout");
  size_t p = protonate.find("--ewin 6");
  protonate.replace(p, 8, "--protonate");
  check("protonation run: atoms may change, no warning",
        CrestCheck::warning(protonate, radius).empty());

  cout << (failures ? "FAILED" : "all passed") << endl;
  return failures ? 1 : 0;
}
