// mocomp (run as ecce-mocomp): the atomic-orbital composition of one molecular orbital of a
// calculation, without a GUI (#161).  The same MoAoBasis the Builder's
// Orbitals panel uses, so the two cannot disagree.
//
//   ecce-mocomp CALC --mo N [--beta] [--method mulliken|c2] [--min PCT]
//               [--by ao,shell,type,atom] [--tsv]
//   ecce-mocomp CALC --list [--beta]
//
// CALC is a calculation folder in local data mode (a path, or file:// URL).
// --mo takes a number, "homo" or "lumo", and may be repeated.  Mulliken
// (C_i (S C)_i, normalised to 100%) is the default: it is the population
// the MO diagram uses and the one ORCA and Multiwfn report.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <string>
#include <vector>

#include "util/EcceURL.H"
#include "tdat/Fragment.H"
#include "tdat/MoAoBasis.H"
#include "tdat/PropVecString.H"
#include "tdat/PropVector.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"

using namespace std;

static void usage()
{
  cerr << "usage: ecce-mocomp CALC --mo N [--beta] [--method mulliken|c2]\n"
          "                   [--min PCT] [--by ao,shell,type,atom] [--tsv]\n"
          "       ecce-mocomp CALC --list [--beta]\n"
          "  CALC   a calculation folder in local data mode\n"
          "  --mo   orbital number (1-based), homo or lumo; repeatable\n"
          "  --min  hide parts below PCT percent (default 0.1)\n"
          "  --tsv  tab-separated: kind label atom l shell component percent\n";
}

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
  bool beta = false, tsv = false, list = false;
  MoAoBasis::Method method = MoAoBasis::MULLIKEN;
  double minPct = 0.1;
  string by = "ao,shell,type,atom";

  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "--mo" && i+1 < argc) mos.push_back(argv[++i]);
    else if (a == "--beta") beta = true;
    else if (a == "--tsv") tsv = true;
    else if (a == "--list") list = true;
    else if (a == "--min" && i+1 < argc) minPct = atof(argv[++i]);
    else if (a == "--by" && i+1 < argc) by = argv[++i];
    else if (a == "--method" && i+1 < argc) {
      string m = argv[++i];
      if (m == "mulliken") method = MoAoBasis::MULLIKEN;
      else if (m == "c2") method = MoAoBasis::COEFFICIENT_SQUARED;
      else { usage(); return 2; }
    }
    else if (a[0] != '-' && target.empty()) target = a;
    else { usage(); return 2; }
  }
  if (target.empty() || (mos.empty() && !list)) { usage(); return 2; }

  string url = target.find("://") != string::npos
                   ? target : "file://" + absolute(target);
  EDSIServerCentral central;
  Resource *res = EDSIFactory::getResource(EcceURL(url));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  if (!calc) {
    cerr << "ecce-mocomp: " << target << " is not a calculation" << endl;
    return 1;
  }

  //  The coefficients are in the frame the code ran in, which for a
  //  geometry optimisation is the last step's.
  unique_ptr<Fragment> frag(calc->getFragmentStep(-1));
  if (!frag || frag->numAtoms() == 0) {
    cerr << "ecce-mocomp: the calculation has no chemical system" << endl;
    return 1;
  }
  vector<string> symbols;
  vector<double> coords;
  if (!MoAoBasis::fragmentGeometry(frag.get(), symbols, coords)) {
    cerr << "ecce-mocomp: the chemical system has no usable atoms" << endl;
    return 1;
  }

  vector< vector<double> > rows;
  if (!MoAoBasis::coefficients(calc, symbols, beta, rows)) {
    cerr << "ecce-mocomp: the calculation has no "
         << (beta ? "beta " : "") << "MO coefficients" << endl;
    return 1;
  }

  PropVector *energies =
      (PropVector*) calc->getProperty(beta ? "ORBENGBETA" : "ORBENG");
  PropVector *occs =
      (PropVector*) calc->getProperty(beta ? "ORBOCCBETA" : "ORBOCC");
  PropVecString *syms =
      (PropVecString*) calc->getProperty(beta ? "ORBSYMBETA" : "ORBSYM");

  if (list) {
    for (size_t m = 0; m < rows.size(); m++) {
      printf("%zu", m + 1);
      if (energies && (int)m < energies->rows()) printf("\t%.6f", energies->value(m));
      if (occs && (int)m < occs->rows()) printf("\t%.3f", occs->value(m));
      if (syms && (int)m < syms->rows()) printf("\t%s", syms->value(m).c_str());
      printf("\n");
    }
    return 0;
  }

  MoAoBasis basis;
  if (!basis.build(calc, symbols, coords, (int)rows[0].size())) {
    cerr << "ecce-mocomp: " << basis.complaint << endl;
    return 1;
  }

  int homo = 0;
  if (occs) for (int m = 0; m < occs->rows(); m++) if (occs->value(m) > 0.5) homo = m + 1;

  int status = 0;
  for (size_t q = 0; q < mos.size(); q++) {
    int mo = 0;
    if (mos[q] == "homo") mo = homo;
    else if (mos[q] == "lumo") mo = homo + 1;
    else mo = atoi(mos[q].c_str());
    if (mo < 1 || mo > (int)rows.size()) {
      cerr << "ecce-mocomp: no orbital " << mos[q] << " (1.."
           << rows.size() << ")" << endl;
      status = 1;
      continue;
    }

    MoAoBasis::Composition comp;
    if (!basis.analyse(rows[mo-1], method, comp)) {
      cerr << "ecce-mocomp: orbital " << mo << " has no norm" << endl;
      status = 1;
      continue;
    }

    if (!tsv) {
      printf("MO %d%s", mo, beta ? " (beta)" : "");
      if (energies && mo <= energies->rows()) printf("  E = %.5f Eh", energies->value(mo-1));
      if (occs && mo <= occs->rows()) printf("  occ %.2f", occs->value(mo-1));
      if (syms && mo <= syms->rows()) printf("  %s", syms->value(mo-1).c_str());
      printf("\n%s population, normalised to 100%%; c.S.c = %.4f\n",
             MoAoBasis::methodName(method), comp.norm);
      if (method == MoAoBasis::MULLIKEN && fabs(comp.norm - 1.0) > 0.02)
        printf("WARNING: the orbital is not normalised in this basis; the "
               "coefficients may not match the stored geometry or basis "
               "order.\n");
    }

    struct Row { string kind, label; int atom, l, seq; string comp; double pct; };
    vector<Row> out;
    auto wanted = [&](const char *k) { return by.find(k) != string::npos; };
    if (wanted("ao")) {
      for (size_t mu = 0; mu < basis.functions.size(); mu++) {
        const MoAoBasis::Function& f = basis.functions[mu];
        char buf[16];
        snprintf(buf, sizeof buf, "%d%c", f.shellSeq, "spdfghik"[min(f.l, 7)]);
        Row r{"ao", basis.elements[f.atom] + to_string(f.atom + 1) + " " + buf +
              " " + f.component, f.atom, f.l, f.shellSeq, f.component,
              100.0*comp.perFunction[mu]};
        out.push_back(r);
      }
    }
    auto addShares = [&](const char *kind, const vector<MoAoBasis::Share>& v) {
      for (size_t k = 0; k < v.size(); k++)
        out.push_back(Row{kind, v[k].label, v[k].atom, v[k].l, v[k].shellSeq,
                          "", 100.0*v[k].share});
    };
    if (wanted("shell")) addShares("shell", comp.shells);
    if (wanted("type"))  addShares("type", comp.types);
    if (wanted("atom"))  addShares("atom", comp.atoms);

    const char *titles[] = { "ao", "Atomic orbitals", "shell", "Shells",
                             "type", "Shell types (all shells of that l)",
                             "atom", "Atoms" };
    for (int t = 0; t < 8; t += 2) {
      vector<Row> part;
      for (size_t k = 0; k < out.size(); k++)
        if (out[k].kind == titles[t]) part.push_back(out[k]);
      if (part.empty()) continue;
      stable_sort(part.begin(), part.end(),
                  [](const Row& a, const Row& b) { return a.pct > b.pct; });
      if (!tsv) printf("\n%s\n", titles[t+1]);
      double shown = 0.0;
      for (size_t k = 0; k < part.size(); k++) {
        const Row& r = part[k];
        if (tsv) {
          printf("%d\t%s\t%s\t%d\t%d\t%d\t%s\t%.4f\n", mo, r.kind.c_str(),
                 r.label.c_str(), r.atom + 1, r.l, r.seq, r.comp.c_str(), r.pct);
        } else if (fabs(r.pct) >= minPct) {
          printf("  %-14s %8.2f %%\n", r.label.c_str(), r.pct);
          shown += r.pct;
        }
      }
      if (!tsv) printf("  %-14s %8.2f %%\n", "(sum shown)", shown);
    }
  }
  return status;
}
