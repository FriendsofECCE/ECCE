// ecce-npa: natural population analysis from a calculation's results.h5 (#1054).
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"
#include "tdat/NaturalPopulation.H"
#include "tdat/ResultsFromCalc.H"
#include "tdat/ResultsH5.H"
#include "util/EcceURL.H"

using std::string;

static const char* USAGE =
"usage: ecce-npa <calculation | results.h5> [--json]\n"
"\n"
"Natural population analysis (Reed, Weinstock, Weinhold, J. Chem. Phys. 83,\n"
"735 (1985)) from the orbitals in a calculation's results.h5: natural charges,\n"
"core/valence/Rydberg populations, spin populations (open shell) and the\n"
"natural electron configuration of each atom.\n"
"\n"
"<calculation> is a calculation's URL or folder (its results.h5 or\n"
"Outputs/results.h5 is read directly when present), or a results.h5 file.\n"
"Not for semiempirical (orthonormal basis) or periodic calculations.\n";

static bool isFile(const string& p) { struct stat st; return !stat(p.c_str(), &st) && S_ISREG(st.st_mode); }
static bool isDir(const string& p) { struct stat st; return !stat(p.c_str(), &st) && S_ISDIR(st.st_mode); }

int main(int argc, char** argv) {
  string target;
  bool json = false;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--json")) json = true;
    else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) { fputs(USAGE, stdout); return 0; }
    else if (target.empty()) target = argv[i];
    else { fputs(USAGE, stderr); return 2; }
  }
  if (target.empty()) { fputs(USAGE, stderr); return 2; }

  resultsh5::Model model;
  string err;
  string path;
  string local = target.compare(0, 7, "file://") == 0 ? target.substr(7) : target;
  if (isFile(local)) path = local;
  else if (isDir(local) && isFile(local + "/results.h5")) path = local + "/results.h5";
  else if (isDir(local) && isFile(local + "/Outputs/results.h5")) path = local + "/Outputs/results.h5";
  if (!path.empty()) {
    if (!resultsh5::readFile(path, model, err)) { fprintf(stderr, "ecce-npa: %s: %s\n", path.c_str(), err.c_str()); return 1; }
    if (model.checks.compare(0, 6, "failed") == 0) { fprintf(stderr, "ecce-npa: %s is marked %s\n", path.c_str(), model.checks.c_str()); return 1; }
  } else {
    // a stored calculation (local or on a data server)
    string url = target;
    if (url.find("://") == string::npos) {
      char buf[4096];
      if (url[0] != '/' && getcwd(buf, sizeof buf)) url = string(buf) + "/" + url;
      url = "file://" + url;
    }
    EDSIServerCentral central;
    IPropCalculation* calc = dynamic_cast<IPropCalculation*>(EDSIFactory::getResource(EcceURL(url)));
    if (!calc) { fprintf(stderr, "ecce-npa: %s is not a calculation or results.h5 file\n", target.c_str()); return 1; }
    std::shared_ptr<const resultsh5::Model> m = ResultsFromCalc::stored(calc);
    if (!m) { fprintf(stderr, "ecce-npa: %s has no usable results.h5 (orbitals)\n", target.c_str()); return 1; }
    model = *m;
  }

  npa::Result r;
  if (!npa::compute(model, r, err)) { fprintf(stderr, "ecce-npa: not computed: %s\n", err.c_str()); return 1; }
  fputs(json ? npa::toJson(r).c_str() : npa::toText(r).c_str(), stdout);
  return 0;
}
