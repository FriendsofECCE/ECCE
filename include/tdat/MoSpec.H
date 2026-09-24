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
#ifndef TDAT_MOSPEC__H
#define TDAT_MOSPEC__H

#include <cmath>
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
#include "tdat/Huckel.H"
#include "tdat/SymmetryAnalysis.H"

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
  int charge;

  MoModel() : charge(0) {}
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

  //  NO SPECTRUM?  WORK ONE OUT.
  //
  //  The fragment columns need no calculation, but the molecular
  //  column does, and a structure on its own is the commonest thing
  //  anyone has.  Extended Huckel supplies it from the geometry --
  //  orderings right, energies roughly, which is what a qualitative
  //  diagram is made of.
  //
  //  ONLY WHERE THERE IS NOTHING BETTER.  A calculation's own
  //  orbitals are used whenever they exist; this never overrides
  //  them.
  bool huckel = false;
  vector<double> overlap;              // only when Huckel supplied them
  if (energies.empty() && !elements.empty()) {
    string complaint;
    if (Huckel::solve(coords, elements, charge, energies, occupancies,
                      coefficients, perAtom, shellOf, complaint,
                      &overlap)) {
      huckel = true;
      labels.assign(energies.size(), string());

      //  AND LABEL THEM, since the coefficients are right here.
      //
      //  Extended Huckel reports no symmetries -- nothing about the
      //  method knows what a point group is -- but the orbitals it
      //  produces are symmetry-adapted exactly, because the overlaps
      //  they come from are.  Classifying them in this frame closes
      //  the loop: geometry in, a labelled diagram out, with no
      //  calculation anywhere.
      const CharacterTable *table = CharacterTable::lookup(group);
      vector<SymOp> ops;
      if (table != 0 && MoFragments::symmetryOperations(group, ops)) {
        vector< vector<int> > images;
        vector< vector<int> > classes;
        vector<int> classOfOp;
        if (SymmetryAnalysis::atomImages(coords, elements, ops, 1.0e-3,
                                         images)) {
          SymmetryAnalysis::conjugacyClasses(ops, classes);
          if (SymmetryAnalysis::matchClasses(ops, classes, *table,
                                             classOfOp)) {
            vector<string> derived;
            SymmetryAnalysis::labelSpectrum(coefficients, energies,
                                            vector<string>(), perAtom,
                                            shellOf, images, classOfOp,
                                            ops, *table, derived);
            for (size_t k = 0; k < derived.size() && k < labels.size(); k++) {
              labels[k] = derived[k];
            }
          }
        }
      }
    } else {
      model.note = complaint;
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

        //  And per shell -- which is the only thing that can connect
        //  a diatomic: both its atoms carry exactly half of every
        //  orbital, so which ATOM a sigma-g came from has no answer,
        //  while which SHELL it came from has one.
        //
        //  BINNED BY ELEMENT AS WELL AS ANGULAR MOMENTUM, using the
        //  column's own shell keys.  Binning by angular momentum
        //  alone lumps every s function in a fragment together, so
        //  formaldehyde's oxygen 2s and its hydrogens' 1s shared a
        //  bin, took identical weights, and were dragged towards each
        //  other -- oxygen's 2s/2p separation came out a third of the
        //  free atom's while carbon's stayed full size.
        if (shellOf.size() == coefficients[mo].size()) {
          if (level.shellLeft.size() != left.shellKeys.size()) {
            level.shellLeft.assign(left.shellKeys.size(), 0.0);
          }
          if (level.shellRight.size() != right.shellKeys.size()) {
            level.shellRight.assign(right.shellKeys.size(), 0.0);
          }

          for (int side = 0; side < 2; side++) {
            const MoColumn& column = (side == 0) ? left : right;
            const vector<int>& mine = (side == 0) ? leftAtoms : rightAtoms;
            vector<double>& into =
                (side == 0) ? level.shellLeft : level.shellRight;

            for (size_t s = 0; s < column.shellKeys.size(); s++) {
              const string& key = column.shellKeys[s];
              const string::size_type colon = key.find(':');
              if (colon == string::npos) continue;
              const string element = key.substr(0, colon);
              const int want = atoi(key.substr(colon + 1).c_str());

              vector<int> pick;
              int at = 0;
              for (size_t a = 0; a < perAtom.size(); a++) {
                bool ours = false;
                for (size_t k = 0; k < mine.size(); k++) {
                  if (mine[k] == (int)a) ours = true;
                }
                if (ours && a < elements.size() && elements[a] == element) {
                  for (int f = 0; f < perAtom[a]; f++) {
                    if (shellOf[at + f] == want) pick.push_back(at + f);
                  }
                }
                at += perAtom[a];
              }
              into[s] += shareOfFunctions(coefficients[mo], pick);
            }
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
  //  --- how much of each molecular orbital each TASO actually is ---
  //
  //  A fragment level is a SYMMETRY ORBITAL, not a shell, and the
  //  share of a molecular orbital on a shell is the same for every
  //  symmetry orbital built from it -- so water's in-phase and
  //  out-of-phase hydrogen combinations were weighted identically and
  //  placed at one energy, when the in-phase one is stabilised more.
  //
  //  The overlap of the TASO with the orbital is the right weight,
  //  and the projection that gives it is already stored: the same
  //  coefficients the phase sketches are drawn from.
  //
  //  Only where each atom carries exactly one function of that shell
  //  (or three, for p).  A larger basis contracts several radial
  //  functions into a shell and adding their coefficients is not an
  //  overlap; there the shell weighting stands.
  if (!perAtom.empty() && !coefficients.empty() &&
      shellOf.size() == (size_t)0 + (coefficients.empty() ? 0
                                     : coefficients[0].size())) {
    vector<int> base(perAtom.size(), 0);
    int at = 0;
    for (size_t a = 0; a < perAtom.size(); a++) { base[a] = at; at += perAtom[a]; }

    MoColumn* sides[2] = { &left, &right };
    for (int side = 0; side < 2; side++) {
      for (size_t i = 0; i < sides[side]->levels.size(); i++) {
        MoLevel& level = sides[side]->levels[i];
        if (level.phases.empty() || level.phaseAtoms.empty()) continue;
        if (level.shell < 0 || level.shell > 1) continue;

        const int comps = (level.shell == 0) ? 1 : 3;
        if (level.phases.size() != comps*level.phaseAtoms.size()) continue;

        //  Where this shell starts on each atom, and that it is the
        //  only one of its kind there.
        vector<int> where(level.phaseAtoms.size(), -1);
        bool usable = true;
        for (size_t k = 0; k < level.phaseAtoms.size() && usable; k++) {
          const int a = level.phaseAtoms[k];
          if (a < 0 || (size_t)a >= perAtom.size()) { usable = false; break; }
          int found = 0;
          for (int f = 0; f < perAtom[a]; f++) {
            if (shellOf[base[a] + f] != level.shell) continue;
            if (found == 0) where[k] = base[a] + f;
            found++;
          }
          if (found != comps) usable = false;
        }
        if (!usable) continue;

        //  Normalised, since a projection operator does not return a
        //  unit vector and a weight has to be comparable across
        //  levels.
        double norm = 0.0;
        for (size_t k = 0; k < level.phases.size(); k++) {
          norm += level.phases[k]*level.phases[k];
        }
        if (norm <= 0.0) continue;
        norm = sqrt(norm);

        level.moWeight.assign(centre.levels.size(), 0.0);
        for (size_t ci = 0; ci < centre.levels.size(); ci++) {
          for (size_t m = 0; m < centre.levels[ci].orbitals.size(); m++) {
            const int mo = centre.levels[ci].orbitals[m];
            if (mo < 0 || mo >= (int)coefficients.size()) continue;

            //  <phi|psi>.  Where the basis is not orthogonal the
            //  overlap matrix has to be in it: a dot product of
            //  coefficients is only an overlap when the functions are
            //  orthonormal.  A code that reports orthogonalised
            //  vectors, as MOPAC does, gives none and needs none.
            double value = 0.0;
            const size_t nf = coefficients[mo].size();
            for (size_t k = 0; k < level.phaseAtoms.size(); k++) {
              for (int comp = 0; comp < comps; comp++) {
                const double weight = level.phases[k*comps + comp]/norm;
                const size_t f = where[k] + comp;

                if (overlap.size() == nf*nf) {
                  for (size_t g = 0; g < nf; g++) {
                    value += weight*overlap[f*nf + g]*coefficients[mo][g];
                  }
                } else {
                  value += weight*coefficients[mo][f];
                }
              }
            }
            level.moWeight[ci] += value*value;
          }
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
  //  SAID ON THE PAGE.  A reader has to know whether the middle
  //  column is a calculation or an estimate, and the diagram looks
  //  identical either way.
  if (huckel) {
    const string said =
        "The molecular orbitals are from extended Huckel, worked out "
        "from the geometry: their ORDER and symmetries are meaningful, "
        "their energies approximate. A calculation's own orbitals are "
        "used instead whenever there are any.";
    model.note = model.note.empty() ? said : (model.note + " " + said);
  }

  model.group = group;
  string linear;
  if (MoFragments::linearGroupName(coords, elements, linear)) {
    model.group = linear;
  }
  model.charge   = charge;
  model.coords   = coords;
  model.elements = elements;
  model.labels   = labels;
  model.perAtom  = perAtom;
  model.shellOf  = shellOf;
  return haveFragments;
}

#endif
