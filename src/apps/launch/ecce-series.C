/*
 * ecce-series [--charge N] [--mult M] <template> <folder> <structure>...  (#621)
 *
 * One new calculation per structure in <folder>, each the template's setup
 * (code, theory, run type, details, basis set, launch settings) with its own
 * structure, charge and multiplicity and a generated deck, ready to launch.
 * A structure is an .xyz file (charge/mult from "charge=1 mult=3" in its
 * comment line, else --charge/--mult) or a stored calculation (its final
 * geometry, charge and multiplicity). Paths are in the local data folder, or
 * URLs. Works on the data directly: an open Organizer shows the new
 * calculations after the folder is refreshed.
 *
 * Prints "name: made <url>" or "name: not made: reason". Exit 0: all made;
 * 1: some were not; 2: no local data folder; 64: usage.
 */
#include <climits>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "dsm/Series.H"
#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/LocalData.H"

using std::string;
using std::vector;

namespace {

bool toUrl(const string& arg, string& url, string& why)
{
  if (arg.find("://") != string::npos) { url = arg; return true; }
  string root = LocalData::dir();
  if (root.empty()) { why = "this session uses a data server: give a URL"; return false; }
  char buf[PATH_MAX], rbuf[PATH_MAX];
  if (!realpath(arg.c_str(), buf)) { why = "no such calculation or folder"; return false; }
  string path = buf;
  string base = realpath(root.c_str(), rbuf) ? string(rbuf) : root;
  if (path.compare(0, base.size() + 1, base + "/") != 0) {
    why = "not inside the data folder " + base;
    return false;
  }
  url = "file://" + path;
  return true;
}

}  // namespace

int main(int argc, char **argv)
{
  if (!getenv("ECCE_REALUSERHOME")) {
    const char *h = getenv("HOME");
    if (h) setenv("ECCE_REALUSERHOME", h, 0);
  }
  int charge = 0, mult = 0;
  vector<string> args;
  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "--charge" && i + 1 < argc) charge = atoi(argv[++i]);
    else if (a == "--mult" && i + 1 < argc) mult = atoi(argv[++i]);
    else args.push_back(a);
  }
  if (args.size() < 3) {
    std::cerr << "usage: ecce-series [--charge N] [--mult M] <template calculation> "
                 "<folder> <structure.xyz or calculation>..." << std::endl;
    return 64;
  }
  Ecce::initialize();
  string tUrl, fUrl, why;
  if (!toUrl(args[0], tUrl, why)) { std::cout << args[0] << ": " << why << std::endl; return 2; }
  if (!toUrl(args[1], fUrl, why)) { std::cout << args[1] << ": " << why << std::endl; return 2; }
  vector<string> sources;
  bool allOk = true;
  for (size_t i = 2; i < args.size(); i++) {
    const string& a = args[i];
    if (a.size() > 4 && a.compare(a.size() - 4, 4, ".xyz") == 0) {
      char buf[PATH_MAX];
      sources.push_back(realpath(a.c_str(), buf) ? string(buf) : a);
    } else {
      string u;
      if (toUrl(a, u, why)) sources.push_back(u);
      else { std::cout << a << ": not made: " << why << std::endl; allOk = false; }
    }
  }
  vector<Series::Outcome> r = Series::make(EcceURL(tUrl), EcceURL(fUrl), sources, charge, mult);
  for (size_t i = 0; i < r.size(); i++) {
    if (r[i].ok) std::cout << r[i].name << ": made " << r[i].url
                           << (r[i].note.empty() ? "" : " (" + r[i].note + ")") << std::endl;
    else { std::cout << r[i].name << ": not made: " << r[i].note << std::endl; allOk = false; }
  }
  return allOk ? 0 : 1;
}
