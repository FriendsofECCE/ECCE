//  Run the real diagram engine on a molecule and print the model.
//
//  WHY THIS EXISTS.  The diagram is drawn by a wxWidgets canvas inside
//  builder, so looking at a layout change meant building a package,
//  installing it, starting the services and opening a calculation --
//  minutes per iteration, on someone else's desktop.  This runs the
//  SAME MoFragments and MoDiagram code against a molecule file and
//  prints what they produced, which tools/modiagram/draw.py then
//  renders.
//
//  It is a development tool, not a second implementation: every number
//  it prints came out of the engine builder uses.  The drawing in
//  draw.py IS a second implementation of the layout, and is for
//  agreeing what the diagram should look like before that layout is
//  written into the canvas.
//
//  Input file:
//      group  <NAME>                 the point group, uppercase
//      atom   <symbol> <x> <y> <z>   Angstrom
//      orbital <energy> <occupancy> [label]    Hartree; repeatable
//      basis  <n> <n> ...            basis functions per atom
//      coef   <c> <c> ...            one line per orbital, in order
//
//  Output: plain text, one record per line, read by draw.py.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
using namespace std;

#include "tdat/MoFragments.H"
#include "tdat/MoDiagram.H"
#include "tdat/CharacterTable.H"

static void printColumn(const char* side, const MoColumn& col)
{
  printf("column\t%s\t%s\t%d\t%g\t%d\n", side, col.title.c_str(),
         col.hiddenCount, col.hiddenMaxEnergy, col.hiddenAboveCount);
  for (size_t i = 0; i < col.levels.size(); i++) {
    const MoLevel& l = col.levels[i];
    printf("level\t%s\t%zu\t%.10g\t%g\t%d\t%s\t%s\t%s\t%d\t%d",
           side, i, l.energy, l.occupancy, l.degeneracy,
           l.label.c_str(), l.irrep.c_str(),
           l.annotation.empty() ? "-" : l.annotation.c_str(),
           (int)l.character, l.pairing);
    printf("\t%.4f\t%.4f", l.shareLeft, l.shareRight);
    for (size_t k = 0; k < l.energies.size(); k++) {
      printf("\t%.10g", l.energies[k]);
    }
    printf("\n");
  }
}

int main(int argc, char** argv)
{
  if (argc < 3) {
    fprintf(stderr, "usage: dump <CharacterTables> <molecule>\n");
    return 2;
  }
  CharacterTable::loadFile(argv[1]);

  string group;
  int charge = 0;
  vector<double> coords;
  vector<string> elements;
  vector<double> energies, occupancies;
  vector<string> labels;
  vector<int> perAtom;
  vector< vector<double> > coefficients;

  ifstream in(argv[2]);
  string line;
  while (getline(in, line)) {
    istringstream parse(line);
    string what;
    if (!(parse >> what)) continue;
    if (what == "group") parse >> group;
    else if (what == "charge") parse >> charge;
    else if (what == "atom") {
      string symbol; double x, y, z;
      parse >> symbol >> x >> y >> z;
      elements.push_back(symbol);
      coords.push_back(x); coords.push_back(y); coords.push_back(z);
    } else if (what == "basis") {
      int n;
      while (parse >> n) perAtom.push_back(n);
    } else if (what == "coef") {
      vector<double> row;
      double c;
      while (parse >> c) row.push_back(c);
      coefficients.push_back(row);
    } else if (what == "orbital") {
      double e, o; string label;
      parse >> e >> o;
      if (!(parse >> label)) label = "";
      energies.push_back(e);
      occupancies.push_back(o);
      labels.push_back(label);
    }
  }

  bool anyLabel = false;
  for (size_t i = 0; i < labels.size(); i++) if (!labels[i].empty()) anyLabel = true;

  MoColumn left, centre, right;
  vector<MoConnection> links;

  //  Dimensions from the character table, so a t2 stays three
  //  orbitals however far a finite-precision calculation spreads them.
  map<string,int> dimensions;
  const CharacterTable *table = CharacterTable::lookup(group);
  if (table != 0) {
    const vector<string>& irreps = table->irreps();
    for (size_t i = 0; i < irreps.size(); i++) {
      dimensions[MoDiagram::canonicalIrrep(irreps[i])] =
          table->dimension(irreps[i]);
    }
  }

  MoDiagram::groupByIrrep(energies, occupancies,
                          anyLabel ? labels : vector<string>(),
                          dimensions, 1.0e-4, centre.levels);
  centre.title = "Molecular orbitals";
  MoDiagram::hideBelow(centre, MoDiagram::suggestCoreCutoff(centre.levels));
  MoDiagram::hideAbove(centre, MoDiagram::suggestVirtualCutoff(centre.levels));

  string note;
  vector<int> leftAtoms, rightAtoms;
  bool haveFragments = MoFragments::build(coords, elements, group, charge,
                                          left, right, note,
                                          &leftAtoms, &rightAtoms);

  //  The same composition the panel computes, so what is drawn here is
  //  what ECCE draws.
  if (haveFragments && !perAtom.empty() && !coefficients.empty()) {
    const vector<double> noOverlap;
    for (size_t i = 0; i < centre.levels.size(); i++) {
      MoLevel& level = centre.levels[i];
      double onLeft = 0.0, onRight = 0.0;
      int counted = 0;
      for (size_t k = 0; k < level.orbitals.size(); k++) {
        const int mo = level.orbitals[k];
        if (mo < 0 || mo >= (int)coefficients.size()) continue;
        const double l = MoFragments::share(coefficients[mo], perAtom,
                                            leftAtoms, noOverlap);
        const double r = MoFragments::share(coefficients[mo], perAtom,
                                            rightAtoms, noOverlap);
        if (l < 0.0 || r < 0.0) continue;
        onLeft += l; onRight += r; counted++;
      }
      if (counted > 0) {
        level.shareLeft  = onLeft/counted;
        level.shareRight = onRight/counted;
      }
    }
  }
  if (haveFragments) {
    //  Reconcile the axis conventions BEFORE anything is matched on
    //  the names: in C2v the character table and the code need not
    //  agree on which mirror is sigma-v, and water comes out inside
    //  out if they are compared as they stand.
    string mismatch;
    if (!MoDiagram::reconcile(left.levels, right.levels, centre.levels,
                              mismatch)) {
      haveFragments = false;
      note = mismatch;
    }
    MoDiagram::placeFragments(centre, left, right);
    MoDiagram::classify(left.levels, centre.levels, right.levels);
    MoDiagram::connect(left.levels, centre.levels, right.levels, links);
  }

  //  The energy beside each molecular level, which is what the MOs
  //  table shows and what the diagram is being read against.
  for (size_t i = 0; i < centre.levels.size(); i++) {
    char text[64];
    snprintf(text, sizeof(text), "%.4f", centre.levels[i].energy);
    centre.levels[i].annotation = text;
  }

  printf("group\t%s\n", group.c_str());
  printf("note\t%s\n", note.empty() ? "-" : note.c_str());
  printColumn("left", left);
  printColumn("centre", centre);
  printColumn("right", right);
  for (size_t i = 0; i < links.size(); i++) {
    printf("link\t%d\t%d\t%d\n", links[i].leftLevel, links[i].centreLevel,
           links[i].rightLevel);
  }
  return 0;
}
