//  Build the diagram model from a molecule file.
//
//  Shared by tools/modiagram/dump (prints it) and
//  tools/modiagram/render (paints it through the REAL canvas).  It
//  exists so those two cannot disagree about what the engine produced:
//  the only difference between them is what they do with the model.
//
//  Kept in step with MoDiagramPanel::build(), which is the third
//  caller and the one that matters.  Input file format is documented
//  in dump.C.
#ifndef MODIAGRAM_MODEL__H
#define MODIAGRAM_MODEL__H

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

/** Everything the engine produced for one molecule. */
struct MoModel {
  MoColumn left, centre, right;
  vector<MoConnection> links;
  string group;
  string note;
  //  Kept for the oracle check, which only dump does.
  vector<double> coords;
  vector<string> elements;
  vector<string> labels;
  vector<int> perAtom, shellOf;
};

/** The share of one orbital carried by a named set of functions. */
static double shareOfFunctions(const vector<double>& c,
                               const vector<int>& which)
{
  double whole = 0.0, wanted = 0.0;
  for (size_t i = 0; i < c.size(); i++) whole += c[i]*c[i];
  for (size_t k = 0; k < which.size(); k++) {
    const double v = c[which[k]];
    wanted += v*v;
  }
  return (whole > 0.0) ? wanted/whole : 0.0;
}


static bool buildModel(const char* tables, const char* molecule,
                       MoModel& model)
{
  CharacterTable::loadFile(tables);

  string group;
  int charge = 0;
  vector<double> coords;
  vector<string> elements;
  vector<double> energies, occupancies;
  vector<string> labels;
  vector<int> perAtom;
  vector<int> shellOf;
  vector< vector<double> > coefficients;

  ifstream in(molecule);
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
    } else if (what == "shells") {
      int l;
      while (parse >> l) shellOf.push_back(l);
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

  MoColumn& left   = model.left;
  MoColumn& centre = model.centre;
  MoColumn& right  = model.right;
  vector<MoConnection>& links = model.links;

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

  string& note = model.note;
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

        //  And per shell, which is the only thing that can connect a
        //  diatomic: both its atoms carry exactly half of every
        //  orbital, so which ATOM a sigma-g came from has no answer,
        //  while which SHELL it came from has one.
        if (shellOf.size() == coefficients[mo].size()) {
          if (level.shellLeft.empty())  level.shellLeft.assign(3, 0.0);
          if (level.shellRight.empty()) level.shellRight.assign(3, 0.0);
          for (int want = 0; want < 3; want++) {
            vector<int> pickLeft, pickRight;
            int at = 0;
            for (size_t a = 0; a < perAtom.size(); a++) {
              bool isLeft = false, isRight = false;
              for (size_t k = 0; k < leftAtoms.size(); k++)
                if (leftAtoms[k] == (int)a) isLeft = true;
              for (size_t k = 0; k < rightAtoms.size(); k++)
                if (rightAtoms[k] == (int)a) isRight = true;
              for (int f = 0; f < perAtom[a]; f++) {
                if (shellOf[at + f] != want) continue;
                if (isLeft)  pickLeft.push_back(at + f);
                if (isRight) pickRight.push_back(at + f);
              }
              at += perAtom[a];
            }
            level.shellLeft[want]  += shareOfFunctions(coefficients[mo],
                                                       pickLeft);
            level.shellRight[want] += shareOfFunctions(coefficients[mo],
                                                       pickRight);
          }
        }
      }
      if (counted > 0) {
        level.shareLeft  = onLeft/counted;
        level.shareRight = onRight/counted;
        for (size_t k = 0; k < level.shellLeft.size(); k++) {
          level.shellLeft[k]  /= counted;
          level.shellRight[k] /= counted;
        }
      }
    }
  }
  //  THE FRAGMENT COLUMNS DO NOT NEED THE CODE'S SYMMETRY LABELS.
  //  They come from the geometry and the point group.  What the labels
  //  are for is matching an orbital to a fragment level BY IRREP;
  //  without them the matching falls back to composition, which needs
  //  no labels at all.  Kept in step with MoDiagramPanel::build().
  bool byIrrep = haveFragments;

  if (byIrrep) {
    string mismatch;
    if (!MoDiagram::reconcile(left.levels, right.levels, centre.levels,
                              mismatch)) {
      byIrrep = false;
      if (note.empty()) note = mismatch;
    }
  }
  //  COLUMNS THAT CARRY NO IRREP ARE NOT COLUMNS THAT DISAGREE.
  //
  //  A diatomic's fragment is one atom, which spans no representation
  //  of the molecule's group, so nothing on either side can match the
  //  molecular labels -- and clearing the molecular labels for want of
  //  a match threw away the only symmetry the diagram had.  N2 then
  //  paired its orbitals by energy order alone and coloured 1-sigma-g
  //  with 2-sigma-u, which are not partners.
  //
  //  Clear them only where the sides were compared and disagreed.
  bool columnsCarryIrreps = false;
  for (size_t j = 0; j < left.levels.size(); j++) {
    if (!left.levels[j].irrep.empty()) columnsCarryIrreps = true;
  }
  for (size_t j = 0; j < right.levels.size(); j++) {
    if (!right.levels[j].irrep.empty()) columnsCarryIrreps = true;
  }

  if (byIrrep && columnsCarryIrreps) {
    int matched = 0;
    for (size_t i = 0; i < centre.levels.size(); i++) {
      for (size_t j = 0; j < left.levels.size(); j++) {
        if (!centre.levels[i].irrep.empty() &&
            centre.levels[i].irrep == left.levels[j].irrep) { matched++; break; }
      }
    }
    if (matched == 0 && !centre.levels.empty()) byIrrep = false;
  }
  if (haveFragments) {
    if (!byIrrep && columnsCarryIrreps) {
      for (size_t i = 0; i < centre.levels.size(); i++) {
        centre.levels[i].irrep.clear();
      }
    }
    MoDiagram::classify(left.levels, centre.levels, right.levels);
    MoDiagram::connect(left.levels, centre.levels, right.levels, links);
    MoDiagram::placeFragments(centre, left, right, links);
    MoDiagram::classifyByEnergy(left.levels, centre.levels,
                                right.levels, links);
  }


  //  The name on the page, which is not always the name the generator
  //  gave: it has no infinite groups and calls N2 D4h.
  model.group = group;
  string linear;
  if (MoFragments::linearGroupName(coords, elements, linear)) {
    model.group = linear;
  }
  model.coords   = coords;
  model.elements = elements;
  model.labels   = labels;
  model.perAtom  = perAtom;
  model.shellOf  = shellOf;
  return haveFragments;
}

#endif
