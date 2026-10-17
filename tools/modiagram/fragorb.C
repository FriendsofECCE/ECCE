// fragorb: fragment-orbital composition of a calculation's MOs (#301).
//
//   fragorb CALC [--ligands shell|each] [--beta] [--mo N|homo|lumo|all]...
//                [--mult M] [--tsv]
//   fragorb CALC --diagram
//   fragorb CALC --gfo FRAGCALC:ATOMS ... [--mo ...]
//
// --diagram prints what the Builder's fragment-orbital diagram draws: the
// drawn molecular levels (cut off as MoSpec does), the metal and ligand
// columns, the fitted scale and the lines with their shares.
//
// The second form is the projection check: each FRAGCALC is a calculation
// of one fragment in the parent's basis (ATOMS: its 1-based parent atom
// numbers, comma separated, in the fragment's own order), and its MOs are
// used as the fragment orbitals instead of extended Hueckel ones.  It
// prints each MO's capture and per-fragment shares for comparison with
// Multiwfn's charge decomposition analysis.
//
// CALC is a calculation folder in local data mode, as for ecce-mocomp.
// Prints, per MO, its class (metal/ligand/mixed/core), how much of it the
// fragment orbitals capture, the metal and ligand shares and the largest
// fragment orbitals.  The FO energies are extended Hueckel and qualitative;
// the MO energies are the calculation's.  A development tool: the Builder
// panel will call FragmentOrbitals the same way.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <unistd.h>

#include "util/EcceURL.H"
#include "tdat/EspField.H"
#include "tdat/Fragment.H"
#include "tdat/FoDiagram.H"
#include "tdat/FragmentOrbitals.H"
#include "tdat/MoDiagram.H"
#include "tdat/MoFragments.H"
#include "tdat/MoAoBasis.H"
#include "tdat/PropVecString.H"
#include "tdat/PropVector.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"

using namespace std;

static string absolute(const string& path)
{
  if (!path.empty() && path[0] == '/') return path;
  char buf[4096];
  if (!getcwd(buf, sizeof buf)) return path;
  return string(buf) + "/" + path;
}

int main(int argc, char **argv)
{
  string target;
  vector<string> mos;
  bool beta = false, tsv = false, perFo = false, occOnly = false, diagram = false;
  int mult = 1;
  FragmentOrbitals::LigandSet ligands = FragmentOrbitals::LIGAND_SHELL;
  vector<string> gfo;

  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "--mo" && i+1 < argc) mos.push_back(argv[++i]);
    else if (a == "--beta") beta = true;
    else if (a == "--tsv") tsv = true;
    else if (a == "--perfo") perFo = true;
    else if (a == "--diagram") diagram = true;
    else if (a == "--gfo-occ") occOnly = true;
    else if (a == "--mult" && i+1 < argc) mult = atoi(argv[++i]);
    else if (a == "--gfo" && i+1 < argc) gfo.push_back(argv[++i]);
    else if (a == "--ligands" && i+1 < argc) {
      string v = argv[++i];
      if (v == "shell") ligands = FragmentOrbitals::LIGAND_SHELL;
      else if (v == "each") ligands = FragmentOrbitals::PER_LIGAND;
      else { cerr << "--ligands shell|each\n"; return 2; }
    }
    else if (a[0] != '-' && target.empty()) target = a;
    else {
      cerr << "usage: fragorb CALC [--ligands shell|each] [--beta] "
              "[--mo N|homo|lumo|all]... [--mult M] [--tsv] [--perfo] [--gfo-occ]\n";
      return 2;
    }
  }
  if (target.empty()) { cerr << "usage: fragorb CALC ...\n"; return 2; }
  if (mos.empty()) mos.push_back("all");

  string url = target.find("://") != string::npos
                   ? target : "file://" + absolute(target);
  EDSIServerCentral central;
  Resource *res = EDSIFactory::getResource(EcceURL(url));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  if (!calc) { cerr << "fragorb: " << target << " is not a calculation\n"; return 1; }

  unique_ptr<Fragment> frag(calc->getFragmentStep(-1));
  if (!frag || frag->numAtoms() == 0) {
    cerr << "fragorb: the calculation has no chemical system\n";
    return 1;
  }
  vector<string> symbols;
  vector<double> coords;
  if (!MoAoBasis::fragmentGeometry(frag.get(), symbols, coords)) {
    cerr << "fragorb: the chemical system has no usable atoms\n";
    return 1;
  }

  vector< vector<double> > rows;
  if (!MoAoBasis::coefficients(calc, symbols, beta, rows)) {
    cerr << "fragorb: the calculation has no MO coefficients\n";
    return 1;
  }

  MoAoBasis basis;
  if (!basis.build(calc, symbols, coords, (int)rows[0].size())) {
    cerr << "fragorb: " << basis.complaint << endl;
    return 1;
  }
  if (basis.orthonormal || basis.overlap.empty()) {
    cerr << "fragorb: declined: the coefficients are in an orthonormalised "
            "basis (a semiempirical code), so the projection on fragment "
            "orbitals is not defined for them.\n";
    return 1;
  }

  //  The functions themselves, for <FO|parent AO>.
  vector<int> perAtom;
  vector<double> S;
  vector<EspBasisFunction> pb;
  if (!MoAoBasis::overlapMatrix(calc, symbols, coords, (int)rows[0].size(),
                                perAtom, S, nullptr, &pb) ||
      pb.size() != rows[0].size()) {
    cerr << "fragorb: the parent basis functions could not be built\n";
    return 1;
  }

  vector<FragmentOrbitals::Fragment> frags;
  string why;
  if (!gfo.empty()) {
    vector<int> foFragment;
    vector< vector<double> > foRows;
    vector<int> offset(symbols.size() + 1, 0);
    for (size_t a = 0; a < symbols.size(); a++) offset[a+1] = offset[a] + perAtom[a];
    for (size_t g = 0; g < gfo.size(); g++) {
      const string::size_type colon = gfo[g].rfind(':');
      if (colon == string::npos) { cerr << "--gfo CALC:ATOMS\n"; return 2; }
      const string path = gfo[g].substr(0, colon);
      FragmentOrbitals::Fragment fr;
      string list = gfo[g].substr(colon + 1);
      for (size_t pos = 0; pos < list.size();) {
        const int atom = atoi(list.c_str() + pos) - 1;
        if (atom < 0 || atom >= (int)symbols.size()) { cerr << "bad atom\n"; return 2; }
        fr.atoms.push_back(atom);
        const string::size_type comma = list.find(',', pos);
        if (comma == string::npos) break;
        pos = comma + 1;
      }
      fr.kind = FragmentOrbitals::isTransitionMetal(symbols[fr.atoms[0]])
                    ? FragmentOrbitals::METAL : FragmentOrbitals::LIGAND;
      for (size_t k = 0; k < fr.atoms.size(); k++) fr.name += symbols[fr.atoms[k]];
      frags.push_back(fr);

      EDSIServerCentral c2;
      Resource *r2 = EDSIFactory::getResource(EcceURL("file://" + absolute(path)));
      IPropCalculation *fc = dynamic_cast<IPropCalculation*>(r2);
      unique_ptr<Fragment> ff(fc ? fc->getFragmentStep(-1) : 0);
      vector<string> fs; vector<double> fx;
      vector< vector<double> > frow;
      if (!ff || !MoAoBasis::fragmentGeometry(ff.get(), fs, fx) ||
          !MoAoBasis::coefficients(fc, fs, beta, frow)) {
        cerr << "fragorb: cannot read fragment calculation " << path << endl;
        return 1;
      }
      int width = 0;
      for (size_t k = 0; k < fr.atoms.size(); k++) width += perAtom[fr.atoms[k]];
      if (frow.empty() || (int)frow[0].size() != width) {
        cerr << "fragorb: " << path << " has " << (frow.empty() ? 0 : frow[0].size())
             << " functions, the atoms named have " << width << endl;
        return 1;
      }
      PropVector *fo = (PropVector*) fc->getProperty(beta ? "ORBOCCBETA" : "ORBOCC");
      for (size_t m = 0; m < frow.size(); m++) {
        if (occOnly && (!fo || (int)m >= fo->rows() || fo->value(m) < 0.5)) continue;
        vector<double> row(rows[0].size(), 0.0);
        int at = 0;
        for (size_t k = 0; k < fr.atoms.size(); k++)
          for (int f = 0; f < perAtom[fr.atoms[k]]; f++)
            row[offset[fr.atoms[k]] + f] = frow[m][at++];
        foRows.push_back(row);
        foFragment.push_back((int)g);
      }
    }
    vector<FragmentOrbitals::MoShares> sh;
    if (!FragmentOrbitals::composeGaussian(frags, foFragment, foRows, S, rows, sh, why)) {
      cerr << "fragorb: " << why << endl;
      return 1;
    }
    printf("MO capture metal ligand | per fragment\n");
    for (size_t m = 0; m < sh.size(); m++) {
      printf("%d %.5f %.5f %.5f |", (int)m + 1, sh[m].capture, sh[m].metal, sh[m].ligand);
      vector<double> per(frags.size(), 0.0);
      for (size_t k = 0; k < foFragment.size(); k++) per[foFragment[k]] += sh[m].perFo[k];
      for (size_t g = 0; g < per.size(); g++) printf(" %.5f", per[g]);
      if (perFo) {
        printf(" |");
        for (size_t k = 0; k < sh[m].perFo.size(); k++) printf(" %.6f", sh[m].perFo[k]);
      }
      printf("\n");
    }
    return 0;
  }
  if (!FragmentOrbitals::autoFragments(symbols, coords, frag->charge(), mult,
                                       frags, why)) {
    cerr << "fragorb: " << why << endl;
    return 1;
  }
  if (diagram) {
    PropVector *en = (PropVector*) calc->getProperty(beta ? "ORBENGBETA" : "ORBENG");
    PropVector *oc = (PropVector*) calc->getProperty(beta ? "ORBOCCBETA" : "ORBOCC");
    if (!en || en->rows() != (int)rows.size()) {
      cerr << "fragorb: no orbital energies\n";
      return 1;
    }
    vector<double> e, o;
    for (int m = 0; m < en->rows(); m++) {
      e.push_back(en->value(m));
      o.push_back(oc && m < oc->rows() ? oc->value(m) : 0.0);
    }
    MoColumn centre;
    MoDiagram::groupByIrrep(e, o, vector<string>(), map<string,int>(), 5.0e-4,
                            centre.levels);
    MoDiagram::hideBelow(centre, MoDiagram::suggestCoreCutoff(centre.levels));
    MoDiagram::hideAbove(centre, MoDiagram::suggestVirtualCutoff(centre.levels));
    int room = 0;
    for (size_t i = 0; i < symbols.size(); i++)
      room += MoFragments::valenceOrbitals(symbols[i]);
    //  The panel keeps a metal's semicore levels (the diagram tells core by
    //  capture), and they take room: occupied orbitals beyond the valence
    //  electron pairs.
    int pairs = -frag->charge();
    for (size_t i = 0; i < symbols.size(); i++)
      pairs += MoFragments::valenceElectrons(symbols[i]);
    pairs /= 2;
    int occupied = 0;
    for (size_t i = 0; i < centre.levels.size(); i++)
      if (centre.levels[i].occupancy > 0.0) occupied += centre.levels[i].degeneracy;
    MoDiagram::hideBeyondValence(centre, room + (occupied > pairs ? occupied - pairs : 0));

    FoDiagram::Result res;
    if (!FoDiagram::build(symbols, coords, frag->charge(), pb, S, rows, e,
                          centre, res, why)) {
      cerr << "fragorb: " << why << endl;
      return 1;
    }
    printf("%s\n", res.note.c_str());
    printf("scale %.4f %.5f capture %.4f %.4f\n", res.slope, res.intercept,
           res.captureMin, res.captureMax);
    const MoColumn* cols[3] = { &res.left, &centre, &res.right };
    const char* names[3] = { "metal", "mo", "ligand" };
    for (int c = 0; c < 3; c++)
      for (size_t i = 0; i < cols[c]->levels.size(); i++) {
        const MoLevel& l = cols[c]->levels[i];
        printf("%s %zu E=%.4f deg=%d occ=%.0f label=\"%s\" ann=\"%s\"", names[c], i,
               l.energy, l.degeneracy, l.occupancy, l.label.c_str(),
               l.annotation.c_str());
        if (c == 1) printf(" mo=%d", l.orbitals.empty() ? 0 : l.orbitals[0] + 1);
        printf("\n");
      }
    for (size_t i = 0; i < res.links.size(); i++)
      printf("link mo=%d left=%d right=%d weight=%.3f\n", res.links[i].centreLevel,
             res.links[i].leftLevel, res.links[i].rightLevel, res.links[i].weight);
    return 0;
  }
  FragmentOrbitals::FoSet set;
  if (!FragmentOrbitals::build(symbols, coords, frags, ligands, set, why)) {
    cerr << "fragorb: " << why << endl;
    return 1;
  }
  vector<FragmentOrbitals::MoShares> sh;
  if (!FragmentOrbitals::compose(set, pb, S, rows, sh, why)) {
    cerr << "fragorb: " << why << endl;
    return 1;
  }

  PropVector *energies =
      (PropVector*) calc->getProperty(beta ? "ORBENGBETA" : "ORBENG");
  PropVector *occs =
      (PropVector*) calc->getProperty(beta ? "ORBOCCBETA" : "ORBOCC");
  PropVecString *syms =
      (PropVecString*) calc->getProperty(beta ? "ORBSYMBETA" : "ORBSYM");
  int homo = 0;
  if (occs) for (int m = 0; m < occs->rows(); m++) if (occs->value(m) > 0.5) homo = m + 1;

  if (!tsv) {
    printf("Fragments (%s):", ligands == FragmentOrbitals::LIGAND_SHELL
                                  ? "ligand shell" : "each ligand");
    for (size_t i = 0; i < set.fragments.size(); i++)
      printf(" %s[%+d]", set.fragments[i].name.c_str(), set.fragments[i].charge);
    printf("\nFO energies are extended Hueckel (qualitative); MO energies are "
           "the calculation's.\nSTO fit quality >= %.5f.  class: metal/ligand if "
           "one side holds >= %.0f%% of the captured part, core if capture < "
           "%.0f%%.\n", set.worstFit, 100*FragmentOrbitals::SIDE_FRACTION,
           100*FragmentOrbitals::CORE_CAPTURE);
    printf("%4s %10s %4s %-5s %-6s %7s %6s %6s  %s\n", "MO", "E(Eh)", "occ",
           "irrep", "class", "capture", "metal", "ligand", "largest FOs");
  }
  for (size_t q = 0; q < mos.size(); q++) {
    vector<int> pick;
    if (mos[q] == "all") for (size_t m = 0; m < sh.size(); m++) pick.push_back((int)m + 1);
    else if (mos[q] == "homo") pick.push_back(homo);
    else if (mos[q] == "lumo") pick.push_back(homo + 1);
    else pick.push_back(atoi(mos[q].c_str()));
    for (size_t p = 0; p < pick.size(); p++) {
      const int mo = pick[p];
      if (mo < 1 || mo > (int)sh.size()) { cerr << "no MO " << mo << endl; continue; }
      const FragmentOrbitals::MoShares& s = sh[mo-1];
      const double e = (energies && mo <= energies->rows()) ? energies->value(mo-1) : 0.0;
      const double o = (occs && mo <= occs->rows()) ? occs->value(mo-1) : 0.0;
      const string irrep = (syms && mo <= syms->rows()) ? syms->value(mo-1) : "";
      printf(tsv ? "%d\t%.6f\t%.3f\t%s\t%s\t%.4f\t%.4f\t%.4f" :
                   "%4d %10.5f %4.1f %-5s %-6s %7.3f %6.3f %6.3f ",
             mo, e, o, irrep.c_str(), s.klass.c_str(), s.capture, s.metal, s.ligand);
      for (size_t t = 0; t < s.top.size() && t < 3; t++) {
        printf(tsv ? "\t%s\t%.4f\t%.3f" : "  %s %.0f%% (%.1f eV)",
               s.top[t].label.c_str(),
               tsv ? s.top[t].share : 100*s.top[t].share,
               s.top[t].energy);
      }
      printf("\n");
    }
  }
  return 0;
}
