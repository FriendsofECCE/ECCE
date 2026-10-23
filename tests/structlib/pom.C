// Loads every Structure Library entry under Polyoxometalates the way the
// Builder does (FragUtil::load, MVM) and checks that it parses, that the
// element counts equal the "# formula:" line and the title [formula]n+-, that
// "charge:" agrees with the title, that it has bonds, and that the source
// label and reference comments are present.  Usage: structlib_pom <dir>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "tdat/FragUtil.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"

using namespace std;
namespace fs = std::filesystem;

static int failures = 0;
static void check(bool ok, const string& what)
{
  if (!ok) { cout << "FAIL " << what << endl; failures++; }
}

static map<string, int> parseFormula(const string& f)
{
  map<string, int> m;
  for (size_t i = 0; i < f.size();) {
    string el(1, f[i++]);
    while (i < f.size() && islower((unsigned char)f[i])) el += f[i++];
    int n = 0;
    while (i < f.size() && isdigit((unsigned char)f[i])) n = n * 10 + (f[i++] - '0');
    m[el] += n ? n : 1;
  }
  return m;
}

int main(int argc, char **argv)
{
  fs::path dir = argc > 1 ? argv[1] : ".";
  int n = 0;
  for (auto& p : fs::recursive_directory_iterator(dir)) {
    if (p.path().extension() != ".mvm") continue;
    n++;
    string name = fs::relative(p.path(), dir).string();
    string formula, title, label, ref;
    int charge = 0;
    bool haveCharge = false;
    {
      ifstream is(p.path());
      string line;
      while (getline(is, line)) {
        if (line.rfind("# formula:", 0) == 0) formula = line.substr(10);
        else if (line.rfind("# Reference:", 0) == 0) ref = line;
        else if (line.rfind("# computed geometry (", 0) == 0 ||
                 line.rfind("# crystal structure (", 0) == 0 ||
                 line.rfind("# built model (", 0) == 0) label = line;
        else if (line.rfind("title:", 0) == 0) title = line.substr(6);
        else if (line.rfind("charge:", 0) == 0) { charge = atoi(line.c_str() + 7); haveCharge = true; }
      }
    }
    formula.erase(0, formula.find_first_not_of(' '));
    title.erase(0, title.find_first_not_of(" \t"));
    check(!label.empty() && !ref.empty(), name + ": source label and reference comments");
    check(!formula.empty() && haveCharge, name + ": formula and charge lines");
    size_t rb = title.find(']');
    check(title.size() > 2 && title[0] == '[' && rb != string::npos &&
          title.substr(1, rb - 1) == formula, name + ": title matches formula");
    if (rb != string::npos) {
      int tc = atoi(title.substr(rb + 1).c_str());
      if (title.back() == '-') tc = -tc;
      check(tc == charge, name + ": title charge == charge:");
    }
    ifstream is(p.path());
    Fragment frag;
    FragUtil util(&frag);
    bool ok = util.load(is, "MVM", 1.0, true, 1, " ", " ");
    check(ok, name + ": loads");
    if (!ok) continue;
    map<string, int> have;
    for (int i = 0; i < frag.numAtoms(); i++) have[frag.atomRef(i)->atomicSymbol()]++;
    check(have == parseFormula(formula), name + ": element counts match " + formula);
    check(frag.numBonds() > 0, name + ": has bonds");
  }
  check(n >= 50, "found " + to_string(n) + " entries");
  cout << n << " entries checked, " << failures << " failures" << endl;
  return failures ? 1 : 0;
}
