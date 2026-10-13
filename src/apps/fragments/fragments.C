// fragments (run as ecce-fragments): fragment-orbital analysis of a parent
// calculation against fragment calculations at the same geometry and basis,
// without a GUI (#201).
//
//   ecce-fragments PARENT --fragment CALC [--label NAME] ... [--mo LO-HI]
//                  [--threshold PCT] [--method mulliken|scpa] [--json]
//
// Every calculation is a folder in local data mode.  --fragment is repeated
// once per fragment; --label names the fragment given by the --fragment
// before it.  --mo takes numbers or homo/lumo with an offset (homo-5-lumo+5,
// the default).  Fragment orbitals contributing at least --threshold percent
// (default 10, Multiwfn's default) are listed.  The run stops with the
// reason when a consistency check fails (see FragmentAnalysis.H).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "tdat/FragmentAnalysis.H"
#include "tdat/FragmentLoader.H"

using namespace std;

static void usage()
{
  cerr << "usage: ecce-fragments PARENT --fragment CALC [--label NAME]\n"
          "                      [--fragment CALC ...] [--mo LO-HI]\n"
          "                      [--threshold PCT] [--method mulliken|scpa]\n"
          "                      [--tol T] [--position-tol ANGSTROM] [--json]\n"
          "  --mo   orbital range, numbers or homo/lumo with offset\n"
          "         (default homo-5-lumo+5)\n";
}

//  "12", "homo", "homo-3", "lumo+2".
static bool orbitalNumber(const string& s, int homo, int& n)
{
  if (s.compare(0, 4, "homo") == 0 || s.compare(0, 4, "lumo") == 0) {
    n = homo + (s[0] == 'l' ? 1 : 0);
    if (s.size() > 4) {
      if (s[4] != '+' && s[4] != '-') return false;
      n += atoi(s.c_str() + 4);
    }
    return true;
  }
  char *end = 0;
  n = (int)strtol(s.c_str(), &end, 10);
  return end && *end == 0 && !s.empty();
}

//  "LO-HI": the split is the '-' that is not an offset sign, i.e. the one
//  followed by a digit or "homo"/"lumo" after an operand.
static bool orbitalRange(const string& s, int homo, int nmo, int& lo, int& hi)
{
  for (size_t k = 1; k < s.size(); k++) {
    if (s[k] != '-') continue;
    int a, b;
    if (orbitalNumber(s.substr(0, k), homo, a) &&
        orbitalNumber(s.substr(k + 1), homo, b)) {
      lo = max(1, a); hi = min(nmo, b);
      return lo <= hi;
    }
  }
  int a;
  if (!orbitalNumber(s, homo, a) || a < 1 || a > nmo) return false;
  lo = hi = a;
  return true;
}

int main(int argc, char **argv)
{
  string parentPath, range = "homo-5-lumo+5", method = "mulliken";
  vector<string> paths, labels;
  double threshold = 10.0, tol = 1.0e-5, posTol = 1.0e-4;
  bool json = false;

  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "--fragment" && i+1 < argc) { paths.push_back(argv[++i]); labels.push_back(""); }
    else if (a == "--label" && i+1 < argc && !labels.empty()) labels.back() = argv[++i];
    else if (a == "--mo" && i+1 < argc) range = argv[++i];
    else if (a == "--threshold" && i+1 < argc) threshold = atof(argv[++i]);
    else if (a == "--method" && i+1 < argc) method = argv[++i];
    else if (a == "--tol" && i+1 < argc) tol = atof(argv[++i]);
    else if (a == "--position-tol" && i+1 < argc) posTol = atof(argv[++i]);
    else if (a == "--json") json = true;
    else if (a[0] != '-' && parentPath.empty()) parentPath = a;
    else { usage(); return 2; }
  }
  if (parentPath.empty() || paths.empty() ||
      (method != "mulliken" && method != "scpa")) { usage(); return 2; }

  string problem;
  LoadedCalc parent;
  if (!loadCalc(parentPath, parent, problem)) {
    cerr << "ecce-fragments: " << problem << endl;
    return 1;
  }
  vector<FragmentOrbitalSet> frags;
  for (size_t f = 0; f < paths.size(); f++) {
    LoadedCalc lc;
    FragmentOrbitalSet fs;
    if (!loadCalc(paths[f], lc, problem) ||
        !sameBasisFragment(parent, lc, posTol, fs, problem)) {
      cerr << "ecce-fragments: " << problem << endl;
      return 1;
    }
    if (labels[f].empty()) {
      string p = paths[f];
      while (p.size() > 1 && p.back() == '/') p.pop_back();
      labels[f] = p.substr(p.find_last_of('/') + 1);
    }
    fs.name = labels[f];
    frags.push_back(fs);
  }

  FragmentParent pp = parentOf(parent);
  FragmentAnalysis fa;
  if (!fa.run(pp, frags, tol, problem)) {
    cerr << "ecce-fragments: check failed:\n" << problem;
    return 1;
  }

  const int nmo = (int)pp.coefficients.size();
  int homo = 0;
  for (int m = 0; m < nmo; m++) if (pp.occupations[m] > 0.5) homo = m + 1;
  int lo, hi;
  if (!orbitalRange(range, homo, nmo, lo, hi)) {
    cerr << "ecce-fragments: bad --mo range " << range << " (1.." << nmo << ")" << endl;
    return 2;
  }
  const vector< vector<double> >& comp = method == "scpa" ? fa.scpa : fa.mulliken;

  if (json) {
    printf("{\"method\":\"%s\",\"homo\":%d,\"fragments\":[", method.c_str(), homo);
    for (size_t f = 0; f < frags.size(); f++)
      printf("%s\"%s\"", f ? "," : "", frags[f].name.c_str());
    printf("],\"maxNormError\":%.3g,\"maxCaptureError\":%.3g,\"orbitals\":[",
           fa.checks.maxNormError, fa.checks.maxCaptureError);
  } else {
    printf("%s composition over fragment orbitals; fragments:", method.c_str());
    for (size_t f = 0; f < frags.size(); f++)
      printf(" %zu=%s", f + 1, frags[f].name.c_str());
    printf("\nchecks passed (tolerance %.0e): fragment orthonormality worst %.1e, "
           "parent norm worst %.1e, summed composition worst %.1e\n",
           tol, *max_element(fa.checks.orthonormality.begin(),
                             fa.checks.orthonormality.end()),
           fa.checks.maxNormError, fa.checks.maxCaptureError);
  }

  for (int m = lo; m <= hi; m++) {
    const int k = m - 1;
    if (json) {
      printf("%s{\"mo\":%d,\"energy\":%.6f,\"occ\":%.3f,\"capture\":%.6f,"
             "\"fragments\":[", m > lo ? "," : "", m, pp.energies[k],
             pp.occupations[k], fa.capture[k]);
      for (size_t f = 0; f < frags.size(); f++)
        printf("%s%.4f", f ? "," : "", 100.0 * fa.perFragment[k][f]);
      printf("],\"fo\":[");
    } else {
      printf("\nMO %d  E = %.5f Eh  occ %.2f%s\n", m, pp.energies[k],
             pp.occupations[k], m == homo ? "  (HOMO)" : m == homo + 1 ? "  (LUMO)" : "");
      printf("  per fragment:");
      for (size_t f = 0; f < frags.size(); f++)
        printf("  %s %.1f %%", frags[f].name.c_str(), 100.0 * fa.perFragment[k][f]);
      printf("\n");
    }
    bool first = true;
    for (size_t j = 0; j < fa.orbitals.size(); j++) {
      const double pct = 100.0 * comp[k][j];
      if (fabs(pct) < threshold) continue;
      const FragmentAnalysis::Orbital& o = fa.orbitals[j];
      if (json) {
        printf("%s{\"f\":%d,\"i\":%d,\"e\":%.6f,\"occ\":%.3f,\"pct\":%.4f}",
               first ? "" : ",", o.fragment + 1, o.index + 1, o.energy,
               o.occupation, pct);
      } else {
        printf("  %s orbital %3d  E = %10.5f  occ %.2f  %6.2f %%\n",
               frags[o.fragment].name.c_str(), o.index + 1, o.energy,
               o.occupation, pct);
      }
      first = false;
    }
    if (json) printf("]}");
  }
  printf(json ? "]}\n" : "");
  return 0;
}
