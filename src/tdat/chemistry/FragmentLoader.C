#include <cmath>
#include <cstdlib>
#include <memory>
#include <unistd.h>

#include "tdat/FragmentLoader.H"
#include "tdat/Fragment.H"
#include "tdat/PropVector.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"
#include "util/EcceURL.H"

static string absolute(const string& path)
{
  if (!path.empty() && path[0] == '/') return path;
  if (path.size() > 2 && path[1] == ':' && (path[2] == '/' || path[2] == '\\'))
    return path;
  char buf[4096];
  if (!getcwd(buf, sizeof buf)) return path;
  return string(buf) + "/" + path;
}


bool loadCalc(const string& path, LoadedCalc& out, string& problem)
{
  static EDSIServerCentral central;       // must outlive the resources
  out = LoadedCalc();
  out.name = path;
  const string url = path.find("://") != string::npos
                         ? path : "file://" + absolute(path);
  Resource *res = EDSIFactory::getResource(EcceURL(url));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  if (!calc) { problem = path + " is not a calculation"; return false; }

  //  Coefficients are in the frame of the last geometry step.
  std::unique_ptr<Fragment> frag(calc->getFragmentStep(-1));
  if (!frag || frag->numAtoms() == 0 ||
      !MoAoBasis::fragmentGeometry(frag.get(), out.symbols, out.coordsAng)) {
    problem = path + ": no chemical system";
    return false;
  }
  vector< vector<double> > beta;
  if (MoAoBasis::coefficients(calc, out.symbols, true, beta)) {
    problem = path + ": unrestricted (alpha/beta) orbitals are not supported yet";
    return false;
  }
  if (!MoAoBasis::coefficients(calc, out.symbols, false, out.rows) ||
      out.rows.empty()) {
    problem = path + ": no MO coefficients";
    return false;
  }
  if (!out.basis.build(calc, out.symbols, out.coordsAng,
                       (int)out.rows[0].size()) || out.basis.orthonormal) {
    problem = path + ": " + (out.basis.complaint.empty()
        ? string("no overlap matrix for this code") : out.basis.complaint);
    return false;
  }
  PropVector *e = (PropVector*) calc->getProperty("ORBENG");
  PropVector *o = (PropVector*) calc->getProperty("ORBOCC");
  for (size_t m = 0; m < out.rows.size(); m++) {
    out.energies.push_back(e && (int)m < e->rows() ? e->value(m) : 0.0);
    out.occupations.push_back(o && (int)m < o->rows() ? o->value(m) : 0.0);
  }
  return true;
}


FragmentParent parentOf(const LoadedCalc& c)
{
  FragmentParent p;
  p.nAO = (int)c.basis.functions.size();
  p.overlap = c.basis.overlap;
  p.coefficients = c.rows;
  p.energies = c.energies;
  p.occupations = c.occupations;
  return p;
}


bool sameBasisFragment(const LoadedCalc& parent, const LoadedCalc& frag,
                       double tol, FragmentOrbitalSet& out, string& problem)
{
  out = FragmentOrbitalSet();
  out.name = frag.name;
  out.sameBasis = true;
  out.nAO = (int)frag.basis.functions.size();
  out.energies = frag.energies;
  out.occupations = frag.occupations;
  out.coefficients = frag.rows;
  out.selfOverlap = frag.basis.overlap;

  const size_t np = parent.symbols.size(), nf = frag.symbols.size();
  vector<size_t> offset(np + 1, 0);
  for (size_t a = 0; a < np; a++) offset[a+1] = offset[a] + parent.basis.perAtom[a];
  vector<size_t> foffset(nf + 1, 0);
  for (size_t a = 0; a < nf; a++) foffset[a+1] = foffset[a] + frag.basis.perAtom[a];

  vector<bool> used(np, false);
  out.parentIndex.assign(out.nAO, -1);
  for (size_t a = 0; a < nf; a++) {
    int match = -1;
    for (size_t p = 0; p < np && match < 0; p++) {
      if (used[p] || parent.symbols[p] != frag.symbols[a]) continue;
      double d2 = 0.0;
      for (int k = 0; k < 3; k++) {
        const double d = parent.coordsAng[3*p + k] - frag.coordsAng[3*a + k];
        d2 += d * d;
      }
      if (sqrt(d2) <= tol) match = (int)p;
    }
    if (match < 0) {
      problem = frag.name + ": atom " + std::to_string(a + 1) + " (" +
                frag.symbols[a] + ") has no unused " + frag.symbols[a] +
                " in the parent within " + std::to_string(tol) + " Angstrom";
      return false;
    }
    used[match] = true;
    const size_t n = frag.basis.perAtom[a];
    if (n != (size_t)parent.basis.perAtom[match]) {
      problem = frag.name + ": atom " + std::to_string(a + 1) + " (" +
                frag.symbols[a] + ") has " + std::to_string(n) +
                " basis functions, the parent's has " +
                std::to_string(parent.basis.perAtom[match]);
      return false;
    }
    for (size_t k = 0; k < n; k++) {
      const MoAoBasis::Function& f = frag.basis.functions[foffset[a] + k];
      const MoAoBasis::Function& g = parent.basis.functions[offset[match] + k];
      if (f.l != g.l || f.shellSeq != g.shellSeq || f.component != g.component) {
        problem = frag.name + ": atom " + std::to_string(a + 1) + " (" +
                  frag.symbols[a] + "): basis function " + std::to_string(k + 1) +
                  " is " + f.component + " (l=" + std::to_string(f.l) +
                  "), the parent's is " + g.component + " (l=" +
                  std::to_string(g.l) + "): different basis set";
        return false;
      }
      out.parentIndex[foffset[a] + k] = (int)(offset[match] + k);
    }
  }
  return true;
}
