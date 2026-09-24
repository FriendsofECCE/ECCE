//  Drive MoFragments::basisSpansReported from a molecule on stdin.
//
//  The check itself is in C++ and the molecules come from MOPAC, so
//  this is the seam between them.  Input:
//
//      <group>
//      <natoms>
//      <symbol> <x> <y> <z>          x natoms
//      <functions per atom>          natoms integers
//      <l per function>              sum(functions) integers
//      <label> <label> ...           one per orbital
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "tdat/MoFragments.H"
#include "tdat/CharacterTable.H"

int main(int argc, char** argv)
{
  const char *home = getenv("ECCE_HOME");
  string tables = string(home ? home : ".") +
                  "/data/client/config/CharacterTables";
  CharacterTable::loadFile(tables.c_str());

  string group, line;
  int natoms = 0;
  if (!getline(cin, group)) return 2;
  cin >> natoms;

  vector<double> coords;
  vector<string> elements;
  for (int i = 0; i < natoms; i++) {
    string symbol; double x, y, z;
    cin >> symbol >> x >> y >> z;
    elements.push_back(symbol);
    coords.push_back(x); coords.push_back(y); coords.push_back(z);
  }
  getline(cin, line);

  vector<int> perAtom;
  getline(cin, line);
  { istringstream in(line); int n; while (in >> n) perAtom.push_back(n); }

  vector<int> shellOf;
  getline(cin, line);
  { istringstream in(line); int n; while (in >> n) shellOf.push_back(n); }

  vector<string> labels;
  getline(cin, line);
  { istringstream in(line); string s; while (in >> s) labels.push_back(s); }

  //  One entry per SHELL, not per function.
  vector< vector<int> > shellsPerAtom;
  int at = 0;
  for (size_t a = 0; a < perAtom.size(); a++) {
    vector<int> mine;
    for (int f = 0; f < perAtom[a]; ) {
      const int l = shellOf[at + f];
      mine.push_back(l);
      f += 2*l + 1;
    }
    shellsPerAtom.push_back(mine);
    at += perAtom[a];
  }

  string detail;
  const bool agree = MoFragments::basisSpansReported(
      coords, elements, group, shellsPerAtom, labels, false, detail);
  printf("%s\n", detail.c_str());
  return agree ? 0 : 1;
}
