//  The model-first MO correlation diagram.  See include/tdat/ModelDiagram.H.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>

#include "tdat/ModelDiagram.H"
#include "tdat/FragmentOrbitals.H"
#include "tdat/MoFragments.H"

using namespace std;

namespace {

const double HARTREE = 27.211386245988;      // eV
const string MINUS = "\xE2\x88\x92";         // U+2212

string fmt(const char* f, double v)
{
   char b[64];
   snprintf(b, sizeof b, f, v);
   return b;
}

//  A signed number with a real minus sign.
string signedNumber(double v, int digits)
{
   char f[16];
   snprintf(f, sizeof f, "%%.%df", digits);
   const string t = fmt(f, fabs(v));
   const bool zero = t.find_first_not_of("0.") == string::npos;
   return (v < 0 && !zero) ? MINUS + t : t;
}

string percent(double x)
{
   char b[16];
   snprintf(b, sizeof b, "%.0f %%", 100.0*x);
   return b;
}

string lower(string s)
{
   for (size_t i = 0; i < s.size(); i++) s[i] = (char)tolower((unsigned char)s[i]);
   return s;
}

//  The label of a calculated level without the diagram's own marks.
string stripMarks(string s)
{
   if (s.size() > 3 && s.compare(s.size() - 3, 3, " nb") == 0) s.erase(s.size() - 3);
   if (!s.empty() && s[s.size() - 1] == '*') s.erase(s.size() - 1);
   return s;
}

//  Hill-like formula of one connected piece: a heavy atom bonded to
//  hydrogens first (NH3, CH3), water as H2O, otherwise C, then H, then
//  alphabetical.
string pieceFormula(const vector<string>& symbols, const vector<int>& atoms)
{
   map<string,int> count;
   for (size_t i = 0; i < atoms.size(); i++) count[symbols[atoms[i]]]++;
   string heavy;
   int heavyCount = 0, hydrogens = count.count("H") ? count["H"] : 0;
   for (map<string,int>::iterator it = count.begin(); it != count.end(); ++it)
      if (it->first != "H") { heavy = it->first; heavyCount++; }
   vector<string> order;
   if (heavyCount == 1 && hydrogens >= 1 && heavy != "O") {
      order.push_back(heavy);
      order.push_back("H");
   } else if (count.count("C")) {
      order.push_back("C");
      if (count.count("H")) order.push_back("H");
      for (map<string,int>::iterator it = count.begin(); it != count.end(); ++it)
         if (it->first != "C" && it->first != "H") order.push_back(it->first);
   } else {
      for (map<string,int>::iterator it = count.begin(); it != count.end(); ++it)
         order.push_back(it->first);
   }
   string f;
   for (size_t i = 0; i < order.size(); i++) {
      f += order[i];
      if (count[order[i]] > 1) f += std::to_string(count[order[i]]);
   }
   return f;
}

//  The name of a set of atoms: its formula, with n identical connected
//  pieces told as "2 H" or "(CO)6".
struct Naming { string title, formula; };

Naming nameAtoms(const vector<string>& symbols, const vector<double>& coords,
                 const vector<int>& atoms)
{
   Naming r;
   vector< std::pair<int,int> > bonds;
   MoFragments::covalentBonds(coords, symbols, bonds);
   vector<int> comp(symbols.size(), -1);
   std::set<int> in(atoms.begin(), atoms.end());
   int ncomp = 0;
   for (size_t a = 0; a < atoms.size(); a++) {
      if (comp[atoms[a]] >= 0) continue;
      vector<int> stack(1, atoms[a]);
      comp[atoms[a]] = ncomp;
      while (!stack.empty()) {
         const int x = stack.back(); stack.pop_back();
         for (size_t b = 0; b < bonds.size(); b++) {
            int o = -1;
            if (bonds[b].first == x) o = bonds[b].second;
            else if (bonds[b].second == x) o = bonds[b].first;
            if (o < 0 || !in.count(o) || comp[o] >= 0) continue;
            comp[o] = ncomp;
            stack.push_back(o);
         }
      }
      ncomp++;
   }
   vector<string> formulas;
   for (int c = 0; c < ncomp; c++) {
      vector<int> mine;
      for (size_t a = 0; a < atoms.size(); a++) if (comp[atoms[a]] == c) mine.push_back(atoms[a]);
      formulas.push_back(pieceFormula(symbols, mine));
   }
   bool same = !formulas.empty();
   for (size_t i = 1; i < formulas.size(); i++) if (formulas[i] != formulas[0]) same = false;
   if (same && ncomp > 1) {
      const bool atom = atoms.size() == (size_t)ncomp;
      r.formula = atom ? formulas[0] + std::to_string(ncomp)
                       : "(" + formulas[0] + ")" + std::to_string(ncomp);
      r.title = atom ? std::to_string(ncomp) + " " + formulas[0] + " combinations"
                     : r.formula + " combinations";
   } else if (ncomp > 1) {
      //  Different pieces: "3 H + OH combinations".
      vector<string> order;
      map<string,int> n;
      for (size_t i = 0; i < formulas.size(); i++) if (!n[formulas[i]]++) order.push_back(formulas[i]);
      string t;
      for (size_t i = 0; i < order.size(); i++)
         t += (i ? " + " : "") + (n[order[i]] > 1 ? std::to_string(n[order[i]]) + " " : string()) + order[i];
      r.formula = pieceFormula(symbols, atoms);
      r.title = t + " combinations";
   } else {
      r.formula = pieceFormula(symbols, atoms);
      r.title = r.formula + " (extended H\xC3\xBC" "ckel)";
   }
   if (ncomp == 1 && atoms.size() == 1)
      r.title = symbols[atoms[0]] + " (extended H\xC3\xBC" "ckel)";
   return r;
}

//  Electrons into levels of capacity `cap` each (per spin 1, closed 2).
void fillByEnergy(const vector<double>& energy, const vector<int>& deg,
                  int electrons, int perOrbital, vector<double>& occ)
{
   occ.assign(energy.size(), 0.0);
   vector<size_t> order(energy.size());
   for (size_t i = 0; i < order.size(); i++) order[i] = i;
   std::stable_sort(order.begin(), order.end(),
                    [&](size_t a, size_t b) { return energy[a] < energy[b]; });
   for (size_t k = 0; k < order.size() && electrons > 0; k++) {
      const int room = perOrbital*deg[order[k]];
      const int put = std::min(room, electrons);
      occ[order[k]] = put;
      electrons -= put;
   }
}

//  Electrons into whole sets, as close as can be to what the molecule's
//  occupied orbitals put on each set (`pop`, in electrons): the subset of
//  sets that adds up to the fragment's electrons and lies closest to the
//  populations.  An odd count puts one electron on a single non-degenerate
//  set, never a half-filled degenerate one, and that set is the highest-lying
//  one that is not core-like.  Core-like sets are always full.
void fillByPopulation(const vector<double>& pop, const vector<double>& energy,
                      const vector<int>& deg, int electrons, vector<double>& occ,
                      const vector<bool>& core)
{
   const size_t m = pop.size();
   if (m > 18) { fillByEnergy(energy, deg, electrons, 2, occ); return; }
   double bestCost = 1e30;
   unsigned long bestMask = 0;
   int bestPartial = -1;
   for (unsigned long mask = 0; mask < (1UL << m); mask++) {
      int sum = 0;
      bool coreFull = true;
      for (size_t k = 0; k < m; k++) {
         if (mask & (1UL << k)) sum += 2*deg[k];
         else if (core[k]) coreFull = false;
      }
      if (!coreFull) continue;
      const int need = electrons - sum;
      if (need != 0 && need != 1) continue;
      for (int partial = -1; partial < (need ? (int)m : 0); partial++) {
         if (need == 1 && (partial < 0 || deg[partial] != 1 || core[partial] || (mask & (1UL << partial)))) continue;
         double cost = 0.0;
         for (size_t k = 0; k < m; k++) {
            const double o = (mask & (1UL << k)) ? 2.0*deg[k] : ((int)k == partial ? 1.0 : 0.0);
            cost += (o - pop[k])*(o - pop[k]);
         }
         if (cost < bestCost - 1e-9) { bestCost = cost; bestMask = mask; bestPartial = partial; }
      }
   }
   if (bestCost > 1e29) { fillByEnergy(energy, deg, electrons, 2, occ); return; }
   occ.assign(m, 0.0);
   for (size_t k = 0; k < m; k++) if (bestMask & (1UL << k)) occ[k] = 2.0*deg[k];
   if (bestPartial >= 0) {
      for (size_t k = 0; k < m; k++)
         if (deg[k] == 1 && !core[k] && occ[k] == 0.0 && energy[k] > energy[bestPartial]) bestPartial = (int)k;
      occ[bestPartial] = 1.0;
   }
}

//  How much of an orbital points along the bonds to `centre`: the s part and
//  the p part along each atom's own direction to it.  The rest (p across the
//  bond, d) is pi-like.
double sigmaFraction(const FragmentOrbitals::FoSet& set, const vector<double>& coef,
                     const vector<double>& coords, const double centre[3])
{
   struct Key { int atom, nq; bool operator<(const Key& o) const {
      return atom != o.atom ? atom < o.atom : nq < o.nq; } };
   map<Key, vector<double> > p;
   double total = 0.0, sigma = 0.0;
   for (size_t i = 0; i < set.funcs.size(); i++) {
      const FragmentOrbitals::Function& f = set.funcs[i];
      const double c2 = coef[i]*coef[i];
      total += c2;
      if (f.l == 0) { sigma += c2; continue; }
      if (f.l != 1) continue;
      Key k; k.atom = f.atom; k.nq = f.n;
      vector<double>& v = p[k];
      if (v.empty()) v.assign(3, 0.0);
      v[f.component == "x" ? 0 : f.component == "y" ? 1 : 2] = coef[i];
   }
   for (map<Key, vector<double> >::iterator it = p.begin(); it != p.end(); ++it) {
      double u[3], len = 0.0, d = 0.0;
      for (int k = 0; k < 3; k++) { u[k] = centre[k] - coords[3*it->first.atom + k]; len += u[k]*u[k]; }
      len = sqrt(len);
      if (len < 1e-9) continue;
      for (int k = 0; k < 3; k++) d += it->second[k]*u[k]/len;
      sigma += d*d;
   }
   return total > 0.0 ? sigma/total : 0.0;
}

//  A neutral ligand fragment of separate one-valence-bond atoms (F3, H3, Cl4):
//  each atom brings one unpaired electron into its sigma orbital and the rest
//  are pairs, so the `nSigma` sigma ligand group orbitals hold one electron each
//  and every other set is full.  The sigma sets are the ones with the most
//  sigma character (the fragment's own e' sigma and in-plane pi mix, so no
//  fixed cut-off tells them apart); false when they do not make exactly
//  nSigma orbitals or the electrons do not add up.
bool fillRadicalLigands(const vector<double>& sigmaShare, const vector<bool>& core,
                        const vector<int>& deg, int electrons, int nSigma, vector<double>& occ)
{
   vector<size_t> order;
   for (size_t k = 0; k < deg.size(); k++) if (!core[k]) order.push_back(k);
   std::stable_sort(order.begin(), order.end(),
                    [&](size_t a, size_t b) { return sigmaShare[a] > sigmaShare[b]; });
   occ.assign(deg.size(), 0.0);
   int sigma = 0, need = 0;
   size_t taken = 0;
   for (; taken < order.size() && sigma < nSigma; taken++) {
      const size_t k = order[taken];
      if (sigmaShare[k] <= 0.5) return false;
      occ[k] = deg[k];
      sigma += deg[k];
   }
   if (sigma != nSigma) return false;
   if (taken < order.size() && sigmaShare[order[taken]] >= sigmaShare[order[taken - 1]] - 1e-6) return false;
   for (size_t k = 0; k < deg.size(); k++) {
      if (occ[k] == 0.0) occ[k] = 2.0*deg[k];
      need += (int)occ[k];
   }
   return need == electrons;
}

//  How much of an orbital (coefficients over the functions) changes sign on
//  reflection through the plane with unit normal n: the p and d parts only,
//  s is even.  A planar molecule's orbitals are exactly even (sigma) or odd (pi).
double oddFraction(const FragmentOrbitals::FoSet& set, const vector<double>& coef,
                   const double n[3])
{
   struct Key { int atom, nq, l; bool operator<(const Key& o) const {
      if (atom != o.atom) return atom < o.atom; if (nq != o.nq) return nq < o.nq; return l < o.l; } };
   map<Key, vector<double> > shell;       // l=1: x y z; l=2: z2 x2-y2 xy xz yz
   double total = 0.0;
   for (size_t i = 0; i < set.funcs.size(); i++) {
      const FragmentOrbitals::Function& f = set.funcs[i];
      total += coef[i]*coef[i];
      if (f.l < 1 || f.l > 2) continue;
      Key k; k.atom = f.atom; k.nq = f.n; k.l = f.l;
      vector<double>& v = shell[k];
      if (v.empty()) v.assign(f.l == 1 ? 3 : 5, 0.0);
      const string& c = f.component;
      int idx = 0;
      if (f.l == 1) idx = c == "x" ? 0 : c == "y" ? 1 : 2;
      else idx = c == "z2" ? 0 : c == "x2-y2" ? 1 : c == "xy" ? 2 : c == "xz" ? 3 : 4;
      v[idx] = coef[i];
   }
   if (total <= 0.0) return 0.0;
   const double r3 = sqrt(3.0);
   double odd = 0.0;
   for (map<Key, vector<double> >::iterator it = shell.begin(); it != shell.end(); ++it) {
      const vector<double>& v = it->second;
      if (it->first.l == 1) {
         double d = 0.0;
         for (int k = 0; k < 3; k++) d += v[k]*n[k];
         odd += d*d;
      } else {
         //  The d function as a symmetric 3x3 form; its odd part under
         //  Q -> R Q R, R = 1 - 2 n n^T, is (Q - RQR)/2.
         double q[3][3] = {{0}};
         q[2][2] += v[0]; q[0][0] += -0.5*v[0]; q[1][1] += -0.5*v[0];
         q[0][0] += r3/2*v[1]; q[1][1] -= r3/2*v[1];
         q[0][1] += r3/2*v[2]; q[1][0] += r3/2*v[2];
         q[0][2] += r3/2*v[3]; q[2][0] += r3/2*v[3];
         q[1][2] += r3/2*v[4]; q[2][1] += r3/2*v[4];
         double R[3][3], t[3][3], rq[3][3];
         for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) R[a][b] = (a == b ? 1.0 : 0.0) - 2.0*n[a]*n[b];
         for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) { t[a][b] = 0; for (int c = 0; c < 3; c++) t[a][b] += R[a][c]*q[c][b]; }
         for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) { rq[a][b] = 0; for (int c = 0; c < 3; c++) rq[a][b] += t[a][c]*R[c][b]; }
         double fro = 0.0;
         for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) { const double x = 0.5*(q[a][b] - rq[a][b]); fro += x*x; }
         odd += fro/1.5;
      }
   }
   return odd/total;
}

struct SetD
{
   int fragment;                 // 0 left, 1 right
   vector<int> fos;
   double energy;                // Hartree
   int degeneracy;
   string shell;                 // "3d", "2p"
   double occupancy;
   string irrep;
   int drawn;                    // index in its column, -1
   map<string,double> chars;     // orbital character -> fraction (sums to 1)
   double nsShare;               // of it, the ns of a ligand atom that also has np (F 2s, Cl 3s)
   SetD() : fragment(0), energy(0), degeneracy(0), occupancy(0), drawn(-1), nsShare(0) {}
};

struct Cluster
{
   vector<int> mos;
   double energy;                // Hartree, mean
   int degeneracy;
   vector<double> share;         // per set
   double shareLeft, shareRight;
   double op;                    // overlap population, mean per orbital
   int realLevel;                // index into the calculated levels, -1
   double overlap;
   string irrep, base;
   MoLevel::Character character;
   int pairing;
   double occupancy, occupancyBeta;
   bool core;                    // mostly ligand ns: core-like, never a bond of the diagram
   bool drawn, clipped, lfFold;
   double shownEnergy;
   Cluster() : energy(0), degeneracy(0), shareLeft(0), shareRight(0), op(0),
               realLevel(-1), overlap(0), character(MoLevel::NONBONDING),
               pairing(-1), occupancy(0), occupancyBeta(-1), core(false), drawn(true), clipped(false), lfFold(false), shownEnergy(0) {}
};

//  The orbital character of a ligand fragment orbital, as the fragment's own
//  names: "F 2p" for a one-atom ligand, "CO sigma"/"CO pi*" for a diatomic
//  one (p functions along the bond are sigma, across it pi; a star when the
//  orbital is antibonding inside the ligand), "NH3 sigma" for a larger one.
void ligandCharacter(const FragmentOrbitals::FoSet& set, const vector<int>& members,
                     const vector<string>& symbols, const vector<double>& coords,
                     const vector<int>& atoms, map<string,double>& out, bool piOnly = false)
{
   out.clear();
   const size_t nf = set.funcs.size();
   vector<int> comp(symbols.size(), -1);
   vector< std::pair<int,int> > bonds;
   MoFragments::covalentBonds(coords, symbols, bonds);
   std::set<int> mine(atoms.begin(), atoms.end());
   vector< vector<int> > unit;
   for (size_t a = 0; a < atoms.size(); a++) {
      if (comp[atoms[a]] >= 0) continue;
      const int id = (int)unit.size();
      unit.push_back(vector<int>());
      vector<int> stack(1, atoms[a]);
      comp[atoms[a]] = id;
      while (!stack.empty()) {
         const int x = stack.back(); stack.pop_back();
         unit[id].push_back(x);
         for (size_t b = 0; b < bonds.size(); b++) {
            int o = -1;
            if (bonds[b].first == x) o = bonds[b].second;
            else if (bonds[b].second == x) o = bonds[b].first;
            if (o < 0 || !mine.count(o) || comp[o] >= 0) continue;
            comp[o] = id;
            stack.push_back(o);
         }
      }
   }
   vector<string> unitName(unit.size());
   for (size_t u = 0; u < unit.size(); u++) {
      vector<int> sorted = unit[u];
      std::sort(sorted.begin(), sorted.end());
      unitName[u] = pieceFormula(symbols, sorted);
   }
   double total = 0.0;
   for (size_t m = 0; m < members.size(); m++) {
      const FragmentOrbitals::FO& fo = set.fos[members[m]];
      vector<double> sc(nf, 0.0);
      double intra = 0.0;
      for (size_t i = 0; i < nf; i++) {
         if (fo.coef[i] == 0.0) continue;
         for (size_t j = 0; j < nf; j++) {
            if (fo.coef[j] == 0.0) continue;
            sc[i] += set.overlap[i*nf + j]*fo.coef[j];
            const int ai = set.funcs[i].atom, aj = set.funcs[j].atom;
            if (ai != aj && comp[ai] >= 0 && comp[ai] == comp[aj])
               intra += fo.coef[i]*set.overlap[i*nf + j]*fo.coef[j];
         }
      }
      const bool star = intra < -0.10;
      map<string,double> mine2;
      for (size_t i = 0; i < nf; i++) {
         if (fo.coef[i] == 0.0) continue;
         const double w = fo.coef[i]*sc[i];
         const FragmentOrbitals::Function& fn = set.funcs[i];
         const int u = comp[fn.atom];
         if (u < 0) continue;
         if (piOnly) {
            //  The pi system is one p orbital per atom: named for it, whatever the unit.
            mine2[fn.element + " " + std::to_string(fn.n) + "spdfg"[fn.l] + " \xCF\x80"] += w;
         } else if (unit[u].size() == 1) {
            mine2[fn.element + " " + std::to_string(fn.n) + "spdfg"[fn.l]] += w;
         } else if (unit[u].size() == 2 && fn.l == 1) {
            double ax[3];
            for (int k = 0; k < 3; k++)
               ax[k] = coords[3*unit[u][0] + k] - coords[3*unit[u][1] + k];
            const double len = sqrt(ax[0]*ax[0] + ax[1]*ax[1] + ax[2]*ax[2]);
            const int dir = fn.component == "x" ? 0 : fn.component == "y" ? 1 : 2;
            const double c2 = len > 0 ? ax[dir]*ax[dir]/(len*len) : 1.0;
            mine2[unitName[u] + " \xCF\x83" + (star ? "*" : "")] += w*c2;
            mine2[unitName[u] + " \xCF\x80" + (star ? "*" : "")] += w*(1.0 - c2);
         } else {
            mine2[unitName[u] + " \xCF\x83" + (star ? "*" : "")] += w;
         }
      }
      for (map<string,double>::iterator it = mine2.begin(); it != mine2.end(); ++it) {
         out[it->first] += it->second;
         total += it->second;
      }
   }
   if (total > 0.0)
      for (map<string,double>::iterator it = out.begin(); it != out.end(); ++it) it->second /= total;
}


//  The atoms' principal axes about their mean, widest spread first, each
//  signed so its largest component is positive (so the picture does not
//  flip from run to run).  The sketches are drawn in this frame, whatever
//  frame the structure was stored in.
void principalAxes(const vector<double>& coords, const vector<int>& atoms,
                   double mean[3], double axes[3][3])
{
   for (int k = 0; k < 3; k++) mean[k] = 0.0;
   for (size_t i = 0; i < atoms.size(); i++)
      for (int k = 0; k < 3; k++) mean[k] += coords[3*atoms[i] + k]/atoms.size();
   double a[3][3] = {{0}}, v[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
   for (size_t i = 0; i < atoms.size(); i++)
      for (int r = 0; r < 3; r++)
         for (int c = 0; c < 3; c++)
            a[r][c] += (coords[3*atoms[i] + r] - mean[r])*(coords[3*atoms[i] + c] - mean[c]);
   for (int sweep = 0; sweep < 30; sweep++) {          // Jacobi
      double off = fabs(a[0][1]) + fabs(a[0][2]) + fabs(a[1][2]);
      if (off < 1e-14) break;
      for (int p = 0; p < 2; p++)
         for (int q = p + 1; q < 3; q++) {
            if (fabs(a[p][q]) < 1e-300) continue;
            const double th = (a[q][q] - a[p][p])/(2.0*a[p][q]);
            const double t = (th >= 0 ? 1.0 : -1.0)/(fabs(th) + sqrt(th*th + 1.0));
            const double c = 1.0/sqrt(t*t + 1.0), sn = t*c;
            for (int k = 0; k < 3; k++) {
               const double akp = a[k][p], akq = a[k][q];
               a[k][p] = c*akp - sn*akq; a[k][q] = sn*akp + c*akq;
            }
            for (int k = 0; k < 3; k++) {
               const double apk = a[p][k], aqk = a[q][k];
               a[p][k] = c*apk - sn*aqk; a[q][k] = sn*apk + c*aqk;
            }
            for (int k = 0; k < 3; k++) {
               const double vkp = v[k][p], vkq = v[k][q];
               v[k][p] = c*vkp - sn*vkq; v[k][q] = sn*vkp + c*vkq;
            }
         }
   }
   int order[3] = {0, 1, 2};
   std::sort(order, order + 3, [&](int x, int y) { return a[x][x] > a[y][y]; });
   for (int j = 0; j < 3; j++) {
      int big = 0;
      for (int k = 1; k < 3; k++) if (fabs(v[k][order[j]]) > fabs(v[big][order[j]]) + 1e-9) big = k;
      const double sgn = v[big][order[j]] < 0 ? -1.0 : 1.0;
      for (int k = 0; k < 3; k++) axes[j][k] = sgn*v[k][order[j]];
   }
}

//  The positions the sketches of one fragment column are drawn at.
void sketchFrame(const vector<double>& coords, const vector<int>& atoms, MoColumn& col)
{
   col.sketchX.clear(); col.sketchY.clear();
   if (atoms.size() < 2) return;
   double mean[3], ax[3][3];
   principalAxes(coords, atoms, mean, ax);
   double big = 0.0;
   for (size_t i = 0; i < atoms.size(); i++) {
      double x = 0, y = 0;
      for (int k = 0; k < 3; k++) {
         x += (coords[3*atoms[i] + k] - mean[k])*ax[0][k];
         y += (coords[3*atoms[i] + k] - mean[k])*ax[1][k];
      }
      col.sketchX.push_back(x); col.sketchY.push_back(y);
      big = std::max(big, std::max(fabs(x), fabs(y)));
   }
   if (big <= 1e-9) { col.sketchX.clear(); col.sketchY.clear(); return; }
   for (size_t i = 0; i < col.sketchX.size(); i++) { col.sketchX[i] /= big; col.sketchY[i] /= big; }
   col.sketchNormal = 2;
}

//  The phase pattern of a fragment orbital: the sign and size of its s
//  coefficient on each atom, or of its p vector (in the sketch frame), by
//  whichever shell it is mostly made of.  Only for a fragment of several
//  atoms of one element, where the picture says something a label does not.
void sketchPhases(const FragmentOrbitals::FoSet& set, const FragmentOrbitals::FO& fo,
                  const vector<string>& symbols, const vector<double>& coords,
                  const vector<int>& atoms, MoLevel& lv)
{
   if (atoms.size() < 2) return;
   for (size_t i = 1; i < atoms.size(); i++)
      if (symbols[atoms[i]] != symbols[atoms[0]]) return;
   double ws = 0.0, wp = 0.0, total = 0.0;
   for (size_t i = 0; i < set.funcs.size(); i++) {
      const double w = fo.coef[i]*fo.coef[i];
      total += w;
      if (set.funcs[i].l == 0) ws += w; else if (set.funcs[i].l == 1) wp += w;
   }
   if (total <= 0.0 || std::max(ws, wp) < 0.5*total) return;
   double mean[3], ax[3][3];
   principalAxes(coords, atoms, mean, ax);
   const bool sType = ws >= wp;
   vector<double> ph(atoms.size()*(sType ? 1 : 3), 0.0);
   for (size_t i = 0; i < set.funcs.size(); i++) {
      if (fo.coef[i] == 0.0) continue;
      const FragmentOrbitals::Function& f = set.funcs[i];
      size_t a = 0;
      while (a < atoms.size() && atoms[a] != f.atom) a++;
      if (a == atoms.size()) continue;
      if (sType && f.l == 0) ph[a] += fo.coef[i];
      else if (!sType && f.l == 1) {
         const int dir = f.component == "x" ? 0 : f.component == "y" ? 1 : 2;
         for (int k = 0; k < 3; k++) ph[3*a + k] += fo.coef[i]*ax[k][dir];
      }
   }
   lv.phases = ph;
   lv.phaseAtoms = atoms;
}

//  Phase sketches of a central-atom diagram: one point per ligand unit (a
//  connected piece of the ligand fragment), at its donor atom, the atom nearest
//  the centre.  A recognisable octahedron or non-planar tetrahedron is drawn in
//  an oblique 3-D view; the frame is built from the ligand directions, not from
//  the point-group name.
struct DonorSketch
{
   enum View { NONE, LEGACY, FLAT, OBLIQUE };
   View view;
   vector<int> donors;
   vector<double> dir;                    // unit centre -> donor, 3 each
   double frame[3][3];                    // rows X (across), Y (up), Z (depth), world axes
   double mean[3];
   vector<double> px, py, pz;
   string why;                            // set when view == NONE and a unit is the reason
   DonorSketch() : view(NONE) {
      for (int a = 0; a < 3; a++) { mean[a] = 0; for (int b = 0; b < 3; b++) frame[a][b] = a == b; }
   }
};

double dot3(const double a[3], const double b[3]) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }

void cross3(const double a[3], const double b[3], double c[3])
{
   c[0] = a[1]*b[2] - a[2]*b[1]; c[1] = a[2]*b[0] - a[0]*b[2]; c[2] = a[0]*b[1] - a[1]*b[0];
}

bool unit3(double a[3])
{
   const double n = sqrt(dot3(a, a));
   if (n < 1e-9) return false;
   for (int k = 0; k < 3; k++) a[k] /= n;
   return true;
}

//  Octahedron: three trans pairs along orthogonal axes; the frame is those axes.
bool octahedronFrame(const vector<double>& dir, double frame[3][3])
{
   if (dir.size() != 18) return false;
   double a[3] = { dir[0], dir[1], dir[2] }, b[3] = {0, 0, 0};
   bool trans = false;
   for (size_t i = 1; i < 6; i++) {
      const double d[3] = { dir[3*i], dir[3*i+1], dir[3*i+2] };
      if (dot3(a, d) < -0.8) trans = true;
      else if (fabs(dot3(a, d)) < 0.4 && b[0] == 0 && b[1] == 0 && b[2] == 0)
         for (int k = 0; k < 3; k++) b[k] = d[k];
   }
   if (!trans) return false;
   const double ab = dot3(a, b);
   for (int k = 0; k < 3; k++) b[k] -= ab*a[k];
   if (!unit3(b)) return false;
   double c[3];
   cross3(a, b, c);
   //  Y up = a, Z depth = b, X = Y x Z.
   for (int k = 0; k < 3; k++) { frame[1][k] = a[k]; frame[2][k] = b[k]; }
   cross3(frame[1], frame[2], frame[0]);
   for (size_t i = 0; i < 6; i++) {
      const double d[3] = { dir[3*i], dir[3*i+1], dir[3*i+2] };
      double big = 0.0;
      for (int f = 0; f < 3; f++) big = std::max(big, fabs(dot3(d, frame[f])));
      if (big < 0.85) return false;
   }
   return true;
}

//  Tetrahedron: all pairs near the tetrahedral angle; the frame is the cube's axes.
bool tetrahedronFrame(const vector<double>& dir, double frame[3][3])
{
   if (dir.size() != 12) return false;
   for (int i = 0; i < 4; i++)
      for (int j = i + 1; j < 4; j++) {
         const double d1[3] = { dir[3*i], dir[3*i+1], dir[3*i+2] }, d2[3] = { dir[3*j], dir[3*j+1], dir[3*j+2] };
         if (fabs(dot3(d1, d2) + 1.0/3.0) > 0.25) return false;
      }
   double x[3], a[3], b[3];
   for (int k = 0; k < 3; k++) { x[k] = dir[k] + dir[3+k]; a[k] = dir[k] - dir[3+k]; }
   if (!unit3(x) || !unit3(a)) return false;
   cross3(x, a, b);
   const double r = sqrt(0.5);
   for (int k = 0; k < 3; k++) {
      frame[0][k] = x[k];
      frame[1][k] = (a[k] - b[k])*r;
      frame[2][k] = (a[k] + b[k])*r;
   }
   for (int i = 0; i < 4; i++) {
      const double d[3] = { dir[3*i], dir[3*i+1], dir[3*i+2] };
      for (int f = 0; f < 3; f++) if (fabs(fabs(dot3(d, frame[f])) - sqrt(1.0/3.0)) > 0.15) return false;
   }
   return true;
}

void findDonorSketch(const vector<string>& symbols, const vector<double>& coords,
                     int centre, const vector<int>& ligandAtoms, DonorSketch& ds)
{
   ds = DonorSketch();
   vector< std::pair<int,int> > bonds;
   MoFragments::covalentBonds(coords, symbols, bonds);
   std::set<int> mine(ligandAtoms.begin(), ligandAtoms.end());
   std::map<int,int> comp;
   vector< vector<int> > unit;
   for (size_t a = 0; a < ligandAtoms.size(); a++) {
      if (comp.count(ligandAtoms[a])) continue;
      const int id = (int)unit.size();
      unit.push_back(vector<int>());
      vector<int> stack(1, ligandAtoms[a]);
      comp[ligandAtoms[a]] = id;
      while (!stack.empty()) {
         const int x = stack.back(); stack.pop_back();
         unit[id].push_back(x);
         for (size_t b = 0; b < bonds.size(); b++) {
            int o = -1;
            if (bonds[b].first == x) o = bonds[b].second;
            else if (bonds[b].second == x) o = bonds[b].first;
            if (o < 0 || !mine.count(o) || comp.count(o)) continue;
            comp[o] = id;
            stack.push_back(o);
         }
      }
   }
   bool allSingle = true;
   for (size_t u = 0; u < unit.size(); u++) if (unit[u].size() > 1) allSingle = false;
   for (size_t u = 0; u < unit.size(); u++) {
      double best = 1e30, second = 1e30;
      int donor = -1;
      for (size_t i = 0; i < unit[u].size(); i++) {
         double d2 = 0.0;
         for (int k = 0; k < 3; k++) {
            const double d = coords[3*unit[u][i] + k] - coords[3*centre + k];
            d2 += d*d;
         }
         const double d = sqrt(d2);
         if (d < best) { second = best; best = d; donor = unit[u][i]; }
         else if (d < second) second = d;
      }
      if (donor < 0 || second - best < 0.25) {
         vector<int> sorted = unit[u];
         std::sort(sorted.begin(), sorted.end());
         if (!allSingle) ds.why = pieceFormula(symbols, sorted);
         return;
      }
      ds.donors.push_back(donor);
      double d[3];
      for (int k = 0; k < 3; k++) d[k] = coords[3*donor + k] - coords[3*centre + k];
      unit3(d);
      for (int k = 0; k < 3; k++) ds.dir.push_back(d[k]);
   }
   const size_t n = ds.donors.size();
   if (n < 2) return;

   //  Planar through the centre?  Mirror every direction so the mean is zero.
   vector<double> sym;
   vector<int> idx;
   for (size_t i = 0; i < n; i++)
      for (int sgn = -1; sgn <= 1; sgn += 2) {
         for (int k = 0; k < 3; k++) sym.push_back(sgn*ds.dir[3*i + k]);
         idx.push_back((int)idx.size());
      }
   double m0[3], ax[3][3];
   principalAxes(sym, idx, m0, ax);
   bool planar = true;
   for (size_t i = 0; i < n; i++) {
      const double d[3] = { ds.dir[3*i], ds.dir[3*i+1], ds.dir[3*i+2] };
      if (fabs(dot3(d, ax[2])) > 0.2) planar = false;
   }

   bool oblique = false;
   if (!planar && n == 6) oblique = octahedronFrame(ds.dir, ds.frame);
   else if (!planar && n == 4) oblique = tetrahedronFrame(ds.dir, ds.frame);
   if (oblique) {
      ds.view = DonorSketch::OBLIQUE;
      double big = 0.0;
      for (size_t i = 0; i < n; i++) {
         const double d[3] = { ds.dir[3*i], ds.dir[3*i+1], ds.dir[3*i+2] };
         ds.px.push_back(dot3(d, ds.frame[0]));
         ds.py.push_back(dot3(d, ds.frame[1]));
         ds.pz.push_back(dot3(d, ds.frame[2]));
         big = std::max(big, std::max(fabs(ds.px[i]), std::max(fabs(ds.py[i]), fabs(ds.pz[i]))));
      }
      for (size_t i = 0; i < n; i++) { ds.px[i] /= big; ds.py[i] /= big; ds.pz[i] /= big; }
      return;
   }
   if (allSingle) { ds.view = DonorSketch::LEGACY; return; }

   //  Flat, in the donors' own principal-axis frame.
   principalAxes(coords, ds.donors, ds.mean, ds.frame);
   double big = 0.0;
   for (size_t i = 0; i < n; i++) {
      double x = 0, y = 0;
      for (int k = 0; k < 3; k++) {
         x += (coords[3*ds.donors[i] + k] - ds.mean[k])*ds.frame[0][k];
         y += (coords[3*ds.donors[i] + k] - ds.mean[k])*ds.frame[1][k];
      }
      ds.px.push_back(x); ds.py.push_back(y); ds.pz.push_back(0.0);
      big = std::max(big, std::max(fabs(x), fabs(y)));
   }
   if (big <= 1e-9) { ds.view = DonorSketch::NONE; return; }
   for (size_t i = 0; i < n; i++) { ds.px[i] /= big; ds.py[i] /= big; }
   ds.view = DonorSketch::FLAT;
}

//  The donor atoms' share of a fragment orbital: s and the p along centre-donor
//  (a sigma lobe, signed by the lobe that faces the centre) or the p across it
//  (a pi vector, in the sketch frame), whichever the donors carry more of.
void donorPhases(const DonorSketch& ds, const FragmentOrbitals::FoSet& set,
                 const FragmentOrbitals::FO& fo, MoLevel& lv)
{
   const size_t n = ds.donors.size();
   vector<double> sg(n, 0.0), pi(3*n, 0.0);
   double total = 0.0, wSigma = 0.0, wPi = 0.0;
   for (size_t i = 0; i < set.funcs.size(); i++) total += fo.coef[i]*fo.coef[i];
   for (size_t k = 0; k < n; k++) {
      double s = 0.0, P[3] = {0, 0, 0};
      for (size_t i = 0; i < set.funcs.size(); i++) {
         const FragmentOrbitals::Function& f = set.funcs[i];
         if (f.atom != ds.donors[k] || fo.coef[i] == 0.0) continue;
         if (f.l == 0) s += fo.coef[i];
         else if (f.l == 1) P[f.component == "x" ? 0 : f.component == "y" ? 1 : 2] += fo.coef[i];
      }
      const double u[3] = { ds.dir[3*k], ds.dir[3*k+1], ds.dir[3*k+2] };
      const double along = dot3(P, u);
      sg[k] = s - along;
      double across[3];
      for (int a = 0; a < 3; a++) across[a] = P[a] - along*u[a];
      for (int f = 0; f < 3; f++) pi[3*k + f] = dot3(across, ds.frame[f]);
      wSigma += s*s + along*along;
      wPi += dot3(across, across);
   }
   if (total <= 0.0 || std::max(wSigma, wPi) < 0.15*total) return;
   lv.phases = wSigma >= wPi ? sg : pi;
   lv.phaseAtoms = ds.donors;
}

string characterTag(const map<string,double>& acc, double floor_)
{
   vector< std::pair<double,string> > r;
   for (map<string,double>::const_iterator it = acc.begin(); it != acc.end(); ++it)
      r.push_back(std::make_pair(-it->second, it->first));
   std::sort(r.begin(), r.end());
   string t;
   for (size_t i = 0; i < r.size(); i++) {
      if (i > 0 && -r[i].first < floor_) break;
      if (i > 3) break;
      t += (i ? " + " : "") + r[i].second;
   }
   return t;
}

string joinCaveats(const vector<string>& v)
{
   string r;
   for (size_t i = 0; i < v.size(); i++) r += (i ? "; " : "") + v[i];
   return r;
}

string joinNames(const vector<string>& v)
{
   string s;
   for (size_t i = 0; i < v.size(); i++) s += (i ? ", " : "") + v[i];
   return s;
}

}  // namespace


bool ModelDiagram::applicable(const vector<string>& symbols, string& why)
{
   why.clear();
   FragmentOrbitals::loadParameters();
   string missing;
   if (!FragmentOrbitals::canSolve(symbols, missing)) {
      why = "No extended H\xC3\xBC" "ckel parameters for " + missing + ".";
      return false;
   }
   return true;
}


bool ModelDiagram::build(const Input& in,
                         const vector<EspBasisFunction>& parentBasis,
                         const vector<double>& S,
                         const vector< vector<double> >& moRows,
                         const vector<double>& moEnergies,
                         const vector<MoLevel>& wholeIn,
                         Result& out, string& why)
{
   out = Result();
   why.clear();
   const size_t nAtoms = in.symbols.size();
   const size_t nP = parentBasis.size(), nMO = moRows.size();
   if (nAtoms == 0 || in.coordsAng.size() != 3*nAtoms || in.leftAtoms.empty() ||
       in.rightAtoms.empty() || nMO != moEnergies.size() || nP == 0 ||
       S.size() != nP*nP) {
      why = "The structure and the calculation do not fit together.";
      return false;
   }
   {
      vector<int> seen(nAtoms, 0);
      for (size_t i = 0; i < in.leftAtoms.size(); i++) {
         if (in.leftAtoms[i] < 0 || in.leftAtoms[i] >= (int)nAtoms) { why = "Bad atom."; return false; }
         seen[in.leftAtoms[i]]++;
      }
      for (size_t i = 0; i < in.rightAtoms.size(); i++) {
         if (in.rightAtoms[i] < 0 || in.rightAtoms[i] >= (int)nAtoms) { why = "Bad atom."; return false; }
         seen[in.rightAtoms[i]]++;
      }
      for (size_t i = 0; i < nAtoms; i++) {
         if (seen[i] != 1) {
            why = "The two fragments do not make up the whole molecule.";
            return false;
         }
      }
   }
   if (!applicable(in.symbols, why)) return false;

   //  Pi only: the plane's normal, and a molecule that has a plane.
   double planeN[3] = {0, 0, 1};
   if (in.piOnly) {
      vector<int> all(nAtoms);
      for (size_t i = 0; i < nAtoms; i++) all[i] = (int)i;
      double mean[3], ax[3][3];
      principalAxes(in.coordsAng, all, mean, ax);
      for (int k = 0; k < 3; k++) planeN[k] = ax[2][k];
      for (size_t i = 0; i < nAtoms; i++) {
         double d = 0.0;
         for (int k = 0; k < 3; k++) d += (in.coordsAng[3*i + k] - mean[k])*planeN[k];
         if (fabs(d) > 0.15) { why = "The pi system is only defined for a planar molecule."; return false; }
      }
   }

   //  --- fragments and the model ------------------------------------
   const Naming leftName = nameAtoms(in.symbols, in.coordsAng, in.leftAtoms);
   const Naming rightName = nameAtoms(in.symbols, in.coordsAng, in.rightAtoms);
   vector<FragmentOrbitals::Fragment> frags(2);
   frags[0].atoms = in.leftAtoms;  frags[0].charge = in.leftCharge;
   frags[1].atoms = in.rightAtoms; frags[1].charge = in.rightCharge;
   for (int f = 0; f < 2; f++) {
      const bool single = frags[f].atoms.size() == 1;
      frags[f].kind = single ? FragmentOrbitals::METAL : FragmentOrbitals::LIGAND;
      frags[f].name = single ? in.symbols[frags[f].atoms[0]]
                             : string(f ? "R:" : "L:") + (f ? rightName : leftName).formula;
   }
   FragmentOrbitals::FoSet set;
   if (!FragmentOrbitals::build(in.symbols, in.coordsAng, frags,
                                FragmentOrbitals::PER_LIGAND, set, why))
      return false;
   const size_t nf = set.funcs.size();

   vector<double> modelE;
   vector< vector<double> > modelRows;
   if (!FragmentOrbitals::solveWhole(set, modelE, modelRows, why)) return false;
   for (size_t m = 0; m < nf; m++) {
      double n = 0.0;
      for (size_t i = 0; i < nf; i++) {
         double sc = 0.0;
         for (size_t j = 0; j < nf; j++) sc += set.overlap[i*nf+j]*modelRows[m][j];
         n += modelRows[m][i]*sc;
      }
      if (n > 1e-12) for (size_t i = 0; i < nf; i++) modelRows[m][i] /= sqrt(n);
   }

   //  Exact projection on the fragment orbitals (same basis: coverage 1).
   vector<FragmentOrbitals::MoShares> sh;
   vector< vector<double> > amp;
   {
      vector<EspBasisFunction> own;
      for (size_t i = 0; i < nf; i++) own.push_back(set.funcs[i].fn);
      if (!FragmentOrbitals::compose(set, own, set.overlap, modelRows, sh, why, &amp))
         return false;
   }

   //  --- fragment sets, their electrons ------------------------------
   vector< vector<int> > groups;
   FragmentOrbitals::groupFos(set, groups);
   vector<SetD> sets;
   for (size_t g = 0; g < groups.size(); g++) {
      SetD s;
      s.fos = groups[g];
      s.fragment = set.fos[groups[g][0]].fragment;
      s.degeneracy = (int)groups[g].size();
      double e = 0.0;
      for (size_t k = 0; k < groups[g].size(); k++) e += set.fos[groups[g][k]].energy;
      s.energy = e/s.degeneracy;
      if (frags[s.fragment].kind == FragmentOrbitals::METAL) {
         const FragmentOrbitals::LevelShare d = FragmentOrbitals::describeGroup(set, groups[g]);
         const string::size_type sp = d.label.find(' ');
         s.shell = sp == string::npos ? d.label : d.label.substr(sp + 1);
      }
      if (frags[s.fragment].kind == FragmentOrbitals::METAL) {
         if (!s.shell.empty())
            s.chars[in.symbols[frags[s.fragment].atoms[0]] + " " + s.shell.substr(s.shell.size() - 1)] = 1.0;
      } else {
         ligandCharacter(set, groups[g], in.symbols, in.coordsAng, frags[s.fragment].atoms, s.chars, in.piOnly);
         //  The ns of an atom that also has np (F 2s, Cl 3s) is core-like in a
         //  central-atom diagram: the textbook sigma bonds are made of np.
         for (map<string,double>::iterator it = s.chars.begin(); it != s.chars.end(); ++it) {
            const string::size_type sp = it->first.find(' ');
            if (sp == string::npos || it->first.back() != 's') continue;
            const string el = it->first.substr(0, sp);
            for (size_t q = 0; q < set.funcs.size(); q++)
               if (set.funcs[q].element == el && set.funcs[q].l == 1) { s.nsShare += it->second; break; }
         }
      }
      sets.push_back(s);
   }
   //  --- the model's levels, electrons -------------------------------
   vector<Cluster> cl;
   for (size_t m = 0; m < nf; m++) {
      if (!cl.empty() && fabs(modelE[m] - modelE[cl.back().mos.back()]) < DEGENERATE) {
         cl.back().mos.push_back((int)m);
      } else {
         cl.push_back(Cluster());
         cl.back().mos.push_back((int)m);
      }
   }
   int electrons = -in.charge;
   for (size_t i = 0; i < nAtoms; i++) electrons += FragmentOrbitals::valenceElectrons(in.symbols[i]);
   int unpaired = std::max(0, in.multiplicity - 1);
   if (electrons < 0) electrons = 0;
   if (unpaired > electrons || (electrons - unpaired) % 2 != 0) {
      unpaired = electrons % 2;
      out.warnings.push_back("The multiplicity does not fit the electron count of the model; "
                             "the model is filled with " + std::to_string(unpaired) + " unpaired.");
   }
   const bool openShell = unpaired > 0;
   {
      vector<double> en, occ, occA, occB;
      vector<int> dg;
      for (size_t c = 0; c < cl.size(); c++) {
         double e = 0.0;
         for (size_t k = 0; k < cl[c].mos.size(); k++) e += modelE[cl[c].mos[k]];
         cl[c].degeneracy = (int)cl[c].mos.size();
         cl[c].energy = e/cl[c].degeneracy;
         en.push_back(cl[c].energy);
         dg.push_back(cl[c].degeneracy);
      }
      if (openShell) {
         fillByEnergy(en, dg, (electrons + unpaired)/2, 1, occA);
         fillByEnergy(en, dg, (electrons - unpaired)/2, 1, occB);
         for (size_t c = 0; c < cl.size(); c++) { cl[c].occupancy = occA[c]; cl[c].occupancyBeta = occB[c]; }
      } else {
         fillByEnergy(en, dg, electrons, 2, occ);
         for (size_t c = 0; c < cl.size(); c++) cl[c].occupancy = occ[c];
      }
   }

   //  --- shares on the sets, overlap population ----------------------
   std::set< std::pair<int,int> > bondedAtoms;
   if (in.piOnly) {
      vector< std::pair<int,int> > bonds;
      MoFragments::covalentBonds(in.coordsAng, in.symbols, bonds);
      for (size_t b = 0; b < bonds.size(); b++)
         bondedAtoms.insert(std::make_pair(std::min(bonds[b].first, bonds[b].second),
                                           std::max(bonds[b].first, bonds[b].second)));
   }
   vector<bool> onLeft(nf, false);
   for (size_t i = 0; i < nf; i++)
      for (size_t a = 0; a < in.leftAtoms.size(); a++)
         if (set.funcs[i].atom == in.leftAtoms[a]) onLeft[i] = true;
   for (size_t c = 0; c < cl.size(); c++) {
      Cluster& L = cl[c];
      L.share.assign(sets.size(), 0.0);
      double op = 0.0;
      for (size_t k = 0; k < L.mos.size(); k++) {
         const int m = L.mos[k];
         for (size_t s = 0; s < sets.size(); s++)
            for (size_t q = 0; q < sets[s].fos.size(); q++) {
               const double a = amp[m][sets[s].fos[q]];
               L.share[s] += a*a/L.degeneracy;
            }
         if (in.piOnly) {
            //  Nothing of the pi system is shared between the fragments (the
            //  hydrogens have no pi orbital): bonding is between the bonded atoms.
            for (size_t i = 0; i < nf; i++)
               for (size_t j = i + 1; j < nf; j++) {
                  const int ai = set.funcs[i].atom, aj = set.funcs[j].atom;
                  if (ai == aj || !bondedAtoms.count(std::make_pair(std::min(ai, aj), std::max(ai, aj)))) continue;
                  op += 2.0*modelRows[m][i]*modelRows[m][j]*set.overlap[i*nf+j];
               }
         } else
         for (size_t i = 0; i < nf; i++) {
            if (!onLeft[i]) continue;
            for (size_t j = 0; j < nf; j++)
               if (!onLeft[j]) op += 2.0*modelRows[m][i]*modelRows[m][j]*set.overlap[i*nf+j];
         }
      }
      L.op = op/L.degeneracy;
      for (size_t s = 0; s < sets.size(); s++)
         (sets[s].fragment == 0 ? L.shareLeft : L.shareRight) += L.share[s];
   }

   const bool centralAtom = in.leftAtoms.size() == 1 && in.rightAtoms.size() >= 2;
   if (centralAtom)
      for (size_t c = 0; c < cl.size(); c++) {
         double ns = 0.0;
         for (size_t s2 = 0; s2 < sets.size(); s2++) ns += cl[c].share[s2]*sets[s2].nsShare;
         cl[c].core = ns >= 0.5;
      }

   //  --- pi only: the levels that change sign in the molecular plane --
   vector<bool> setOdd(sets.size(), true);
   int piElectrons = 0;
   if (in.piOnly) {
      for (size_t s2 = 0; s2 < sets.size(); s2++)
         setOdd[s2] = oddFraction(set, set.fos[sets[s2].fos[0]].coef, planeN) > 0.5;
      vector<Cluster> keep;
      for (size_t c = 0; c < cl.size(); c++) {
         double odd = 0.0;
         for (size_t k = 0; k < cl[c].mos.size(); k++)
            odd += oddFraction(set, modelRows[cl[c].mos[k]], planeN);
         if (odd/cl[c].degeneracy > 0.5) {
            keep.push_back(cl[c]);
            piElectrons += (int)lround(cl[c].occupancy + std::max(0.0, cl[c].occupancyBeta));
         }
      }
      cl = keep;
   }

   //  --- the fragments' electrons ------------------------------------
   //  Neutral fragments (the metal ionic).  A one-atom fragment fills by
   //  energy; a ligand puts its electrons where the molecule's occupied
   //  orbitals put them.
   for (int f = 0; f < 2; f++) {
      int n = -frags[f].charge;
      for (size_t k = 0; k < frags[f].atoms.size(); k++)
         n += FragmentOrbitals::valenceElectrons(in.symbols[frags[f].atoms[k]]);
      vector<size_t> mine;
      vector<double> en, pop;
      vector<int> dg;
      vector<bool> coreSet;
      double piPop = 0.0;
      for (size_t s2 = 0; s2 < sets.size(); s2++) {
         if (sets[s2].fragment != f || !setOdd[s2]) continue;
         mine.push_back(s2); en.push_back(sets[s2].energy); dg.push_back(sets[s2].degeneracy);
         coreSet.push_back(centralAtom && sets[s2].nsShare >= 0.5);
         double e = 0.0;
         for (size_t c = 0; c < cl.size(); c++)
            e += (cl[c].occupancy + std::max(0.0, cl[c].occupancyBeta))*cl[c].share[s2];
         pop.push_back(e);
         piPop += e;
      }
      if (in.piOnly) n = (int)lround(piPop);
      vector<double> occ;
      fillByEnergy(en, dg, n < 0 ? 0 : n, 2, occ);
      //  A ligand that the energy order leaves with a half-filled set (three
      //  neutral F atoms are an odd number of electrons) is filled the way
      //  the molecule's own orbitals fill it.
      bool partial = false;
      for (size_t k = 0; k < occ.size(); k++)
         if (occ[k] > 0.0 && occ[k] < 2.0*dg[k] - 1e-9 && dg[k] > 1) partial = true;
      for (size_t k = 0; k < occ.size(); k++)
         if (coreSet[k] && occ[k] < 2.0*dg[k] - 1e-9) partial = true;
      if (partial && frags[f].kind != FragmentOrbitals::METAL)
         fillByPopulation(pop, en, dg, n < 0 ? 0 : n, occ, coreSet);
      //  Covalent model: neutral ligand atoms put their unpaired electrons in
      //  the sigma group orbitals, not wherever the energy order or the
      //  molecule's populations leave them.
      if (centralAtom && f == 1 && !in.piOnly && frags[f].charge == 0 &&
          frags[f].kind != FragmentOrbitals::METAL) {
         double c0[3];
         for (int k = 0; k < 3; k++) c0[k] = in.coordsAng[3*frags[0].atoms[0] + k];
         vector<double> sg, rad;
         for (size_t k = 0; k < mine.size(); k++)
            sg.push_back(sigmaFraction(set, set.fos[sets[mine[k]].fos[0]].coef, in.coordsAng, c0));
         if (fillRadicalLigands(sg, coreSet, dg, n < 0 ? 0 : n, (int)frags[f].atoms.size(), rad)) occ = rad;
      }
      for (size_t k = 0; k < mine.size(); k++) sets[mine[k]].occupancy = occ[k];
   }

   //  --- the calculation's levels, and the match ---------------------
   vector<MoLevel> real = wholeIn;
   if (real.empty()) {
      for (size_t r = 0; r < nMO; r++) {
         MoLevel l;
         l.energy = moEnergies[r];
         l.label = std::to_string(r + 1);
         l.degeneracy = 1;
         l.orbitals.push_back((int)r);
         real.push_back(l);
      }
   }
   vector< vector<double> > O(nf, vector<double>(nMO, 0.0));
   {
      vector< vector<double> > X(nf, vector<double>(nP));
      for (size_t i = 0; i < nf; i++)
         for (size_t j = 0; j < nP; j++)
            X[i][j] = EspField::overlapOf(set.funcs[i].fn, parentBasis[j]);
      vector< vector<double> > T(nf, vector<double>(nP, 0.0));
      for (size_t m = 0; m < nf; m++)
         for (size_t i = 0; i < nf; i++) {
            const double c = modelRows[m][i];
            if (c == 0.0) continue;
            for (size_t j = 0; j < nP; j++) T[m][j] += c*X[i][j];
         }
      for (size_t r = 0; r < nMO; r++) {
         if (moRows[r].size() != nP) { why = "An MO row does not match the basis."; return false; }
         double n = 0.0;
         for (size_t i = 0; i < nP; i++) {
            double sc = 0.0;
            for (size_t j = 0; j < nP; j++) sc += S[i*nP+j]*moRows[r][j];
            n += moRows[r][i]*sc;
         }
         const double k = n > 1e-12 ? 1.0/sqrt(n) : 0.0;
         for (size_t m = 0; m < nf; m++) {
            double s = 0.0;
            for (size_t j = 0; j < nP; j++) s += T[m][j]*moRows[r][j];
            O[m][r] = s*k;
         }
      }
   }
   //  Pi only: the calculated levels that are mostly odd model orbitals.
   if (in.piOnly) {
      vector<bool> oddModel(nf, false);
      for (size_t m = 0; m < nf; m++) oddModel[m] = oddFraction(set, modelRows[m], planeN) > 0.5;
      vector<MoLevel> kept;
      for (size_t R = 0; R < real.size(); R++) {
         double odd = 0.0, all = 0.0;
         for (size_t q = 0; q < real[R].orbitals.size(); q++) {
            const int r = real[R].orbitals[q];
            if (r < 0 || r >= (int)nMO) continue;
            for (size_t m = 0; m < nf; m++) {
               all += O[m][r]*O[m][r];
               if (oddModel[m]) odd += O[m][r]*O[m][r];
            }
         }
         if (all > 0.0 && odd/all > 0.5) kept.push_back(real[R]);
      }
      real = kept;
   }
   for (size_t c = 0; c < cl.size(); c++) {
      Cluster& L = cl[c];
      double best = -1.0;
      for (size_t R = 0; R < real.size(); R++) {
         double w = 0.0;
         for (size_t k = 0; k < L.mos.size(); k++)
            for (size_t q = 0; q < real[R].orbitals.size(); q++) {
               const int r = real[R].orbitals[q];
               if (r >= 0 && r < (int)nMO) w += O[L.mos[k]][r]*O[L.mos[k]][r];
            }
         double v = sqrt(std::min(1.0, w/L.degeneracy));
         //  The same number of orbitals wins a tie.
         if ((int)real[R].orbitals.size() == L.degeneracy) v += 1e-6;
         if (v > best) { best = v; L.realLevel = (int)R; L.overlap = std::min(1.0, v); }
      }
      if (L.realLevel >= 0) {
         L.irrep = real[L.realLevel].irrep;
         L.base = stripMarks(real[L.realLevel].label);
      }
   }

   //  --- bonding, antibonding, pairs --------------------------------
   const bool equalFragments = in.leftAtoms.size() == in.rightAtoms.size() &&
                               leftName.formula == rightName.formula;
   for (size_t c = 0; c < cl.size(); c++) {
      cl[c].character = cl[c].op > OP_TOLERANCE ? MoLevel::BONDING
          : (cl[c].op < -OP_TOLERANCE ? MoLevel::ANTIBONDING : MoLevel::NONBONDING);
   }
   {
      struct Cand { double score; size_t b, a; };
      vector<Cand> cands;
      for (size_t b = 0; b < cl.size(); b++) {
         if (cl[b].character != MoLevel::BONDING || cl[b].core) continue;
         for (size_t a = 0; a < cl.size(); a++) {
            if (cl[a].character != MoLevel::ANTIBONDING || cl[a].core) continue;
            if (cl[a].energy <= cl[b].energy || cl[a].degeneracy != cl[b].degeneracy) continue;
            //  Equal fragments (N + N, CH2 + CH2) combine in phase and out of
            //  phase, and those carry different irreps: the partners of a
            //  pair then differ in irrep by construction.
            if (!equalFragments && !in.piOnly && !cl[a].irrep.empty() && !cl[b].irrep.empty() &&
                cl[a].irrep != cl[b].irrep) continue;
            if (!in.piOnly &&
                std::min(std::min(cl[a].shareLeft, cl[a].shareRight),
                         std::min(cl[b].shareLeft, cl[b].shareRight)) < PAIR_FLOOR) continue;
            //  The strongest bonding level pairs with the strongest antibonding
            //  one; the shared fragment sets only break ties.
            double score = std::min(cl[b].op, -cl[a].op);
            double similar = 0.0;
            for (size_t s = 0; s < sets.size(); s++) similar += std::min(cl[b].share[s], cl[a].share[s]);
            //  Without symmetry labels nothing else keeps an e level from
            //  pairing with a t2: weigh in how much of the same fragment
            //  levels the two are made of.
            if (cl[a].irrep.empty() || cl[b].irrep.empty()) score *= 0.25 + similar;
            score += 1e-3*similar;
            Cand cd; cd.score = score; cd.b = b; cd.a = a;
            cands.push_back(cd);
         }
      }
      std::stable_sort(cands.begin(), cands.end(),
                       [](const Cand& x, const Cand& y) { return x.score > y.score; });
      vector<bool> used(cl.size(), false);
      int next = 0;
      for (size_t i = 0; i < cands.size(); i++) {
         if (used[cands[i].b] || used[cands[i].a]) continue;
         used[cands[i].b] = used[cands[i].a] = true;
         cl[cands[i].b].pairing = cl[cands[i].a].pairing = next++;
      }
      for (size_t c = 0; c < cl.size(); c++)
         if (!used[c] || cl[c].core) cl[c].character = MoLevel::NONBONDING;
   }

   vector<string> caveats;               // tier B: one combined line
   bool lfFolded = false;
   //  --- what is drawn ----------------------------------------------
   const bool bandRule = !in.showAll && in.leftAtoms.size() == 1 && in.rightAtoms.size() >= 2;
   for (size_t c = 0; c < cl.size(); c++)
      cl[c].drawn = !(bandRule && (cl[c].shareRight >= INTERNAL_SHARE || cl[c].core));
   //  The HOMO and the LUMO are never folded away, whatever fragment they sit on.
   {
      int homo = -1, lumo = -1;
      for (size_t c = 0; c < cl.size(); c++) {
         const bool occ = cl[c].occupancy + std::max(0.0, cl[c].occupancyBeta) > 0.0;
         if (occ && (homo < 0 || cl[c].energy > cl[homo].energy)) homo = (int)c;
         if (!occ && (lumo < 0 || cl[c].energy < cl[lumo].energy)) lumo = (int)c;
      }
      if (homo >= 0) cl[homo].drawn = true;
      if (lumo >= 0) cl[lumo].drawn = true;
   }
   //  A level the match gave no irrep takes its partner's (g/u swapped when
   //  the two fragments are equal), so it is not labelled by a bare number.
   for (size_t c = 0; c < cl.size(); c++) {
      if (!cl[c].irrep.empty() || cl[c].pairing < 0) continue;
      for (size_t o = 0; o < cl.size(); o++) {
         if (o == c || cl[o].pairing != cl[c].pairing || cl[o].irrep.empty()) continue;
         string ir = cl[o].irrep;
         if (equalFragments && !ir.empty()) {
            char& ch = ir[ir.size() - 1];
            if (ch == 'U') ch = 'G'; else if (ch == 'G') ch = 'U';
            else if (ch == 'u') ch = 'g'; else if (ch == 'g') ch = 'u';
         }
         cl[c].irrep = ir;
         bool digits = true;
         for (size_t k = 0; k < cl[c].base.size(); k++)
            if (!isdigit((unsigned char)cl[c].base[k])) digits = false;
         if (digits) cl[c].base += ir;
         break;
      }
   }
   for (size_t c = 0; c < cl.size(); c++) {      // a pair is drawn whole
      if (!cl[c].drawn || cl[c].pairing < 0) continue;
      for (size_t o = 0; o < cl.size(); o++)
         if (cl[o].pairing == cl[c].pairing) cl[o].drawn = true;
   }
   //  Character of every model level, from the fragment orbitals it is made of.
   vector<map<string,double> > charAcc(cl.size());
   for (size_t c = 0; c < cl.size(); c++)
      for (size_t s = 0; s < sets.size(); s++)
         for (map<string,double>::iterator it = sets[s].chars.begin(); it != sets[s].chars.end(); ++it)
            charAcc[c][it->first] += cl[c].share[s]*it->second;

   //  The ligand-field view: empty levels that sit on the ligands and lie above
   //  the metal-based HOMO are folded into a note, whatever else would draw them.
   const bool lfEligible = frags[0].kind == FragmentOrbitals::METAL &&
                       frags[1].kind == FragmentOrbitals::LIGAND && in.rightAtoms.size() >= 2 &&
                       FragmentOrbitals::isTransitionMetal(in.symbols[in.leftAtoms[0]]);
   const bool lfView = in.ligandFieldView && lfEligible;
   out.lfApplicable = lfEligible;
   if (lfView) {
      int homo = -1, metalHomo = -1;
      for (size_t c = 0; c < cl.size(); c++) {
         if (cl[c].occupancy + std::max(0.0, cl[c].occupancyBeta) <= 0.0) continue;
         if (homo < 0 || cl[c].energy > cl[homo].energy) homo = (int)c;
         if (cl[c].shareLeft >= 0.5 && (metalHomo < 0 || cl[c].energy > cl[metalHomo].energy)) metalHomo = (int)c;
      }
      if (metalHomo < 0) metalHomo = homo;
      if (metalHomo >= 0)
         for (size_t c = 0; c < cl.size(); c++) {
            if (!cl[c].drawn || cl[c].occupancy + std::max(0.0, cl[c].occupancyBeta) > 0.0) continue;
            if (cl[c].shareRight < LF_LIGAND_SHARE || cl[c].energy <= cl[metalHomo].energy) continue;
            cl[c].drawn = false;
            cl[c].lfFold = true;
         }
   }

   //  The window above the HOMO: nothing up there is a model of anything.
   vector<bool> tooHigh(cl.size(), false);
   vector<size_t> order0(cl.size());
   for (size_t i = 0; i < order0.size(); i++) order0[i] = i;
   std::stable_sort(order0.begin(), order0.end(),
                    [&](size_t a, size_t b) { return cl[a].energy < cl[b].energy; });
   double windowTop = 1e30;
   {
      double homo = -1e30;
      for (size_t c = 0; c < cl.size(); c++)
         if (cl[c].occupancy + std::max(0.0, cl[c].occupancyBeta) > 0.0) homo = std::max(homo, cl[c].energy);
      windowTop = homo + WINDOW_EV/HARTREE;
      int n = 0, clipped = 0;
      for (size_t k = 0; k < order0.size(); k++) {
         const size_t c = order0[k];
         if (cl[c].energy <= windowTop) continue;
         if (cl[c].drawn) {
            //  Drawn at the top of the axis, in order, its own energy written beside it.
            cl[c].shownEnergy = windowTop + (1.5 + 1.0*clipped)/HARTREE;
            cl[c].clipped = true;
            clipped += cl[c].degeneracy;
         } else if (!cl[c].lfFold) {
            tooHigh[c] = true;
            n += cl[c].degeneracy;
         }
      }
      if (clipped > 0)
         caveats.push_back(std::to_string(clipped) + " high empty level" + (clipped == 1 ? "" : "s") +
             " drawn at the top (marked with an up arrow)");
      if (n > 0)
         caveats.push_back(std::to_string(n) + " very high empty level" + (n == 1 ? "" : "s") + " not drawn");
   }

   //  Sets of the right side get the irrep of the levels they mostly sit in.
   for (size_t s = 0; s < sets.size(); s++) {
      if (sets[s].degeneracy == 1 && frags[sets[s].fragment].kind == FragmentOrbitals::METAL) continue;
      map<string,double> by;
      double tot = 0.0;
      for (size_t c = 0; c < cl.size(); c++) {
         by[cl[c].irrep] += cl[c].share[s]*cl[c].degeneracy;
         tot += cl[c].share[s]*cl[c].degeneracy;
      }
      string best;
      double w = 0.0;
      for (map<string,double>::iterator it = by.begin(); it != by.end(); ++it)
         if (it->second > w) { w = it->second; best = it->first; }
      if (!best.empty() && w >= 0.5*tot &&
          (frags[sets[s].fragment].kind != FragmentOrbitals::METAL))
         sets[s].irrep = best;
   }

   //  --- columns, levels, lines -------------------------------------
   vector<size_t> order(cl.size());
   for (size_t i = 0; i < order.size(); i++) order[i] = i;
   std::stable_sort(order.begin(), order.end(),
                    [&](size_t a, size_t b) { return cl[a].energy < cl[b].energy; });

   //  A level's irrep spelled like the calculation's labels.
   out.centre.title = "Model MOs";
   out.left.title = leftName.title;
   out.right.title = rightName.title;
   vector<int> centreIndex(cl.size(), -1);
   vector<int> centreReal;           // per centre level: the calculated level it matched
   vector<double> centreTrueE;       // its model energy, also when drawn at the top
   for (size_t oi = 0; oi < order.size(); oi++) {
      const Cluster& L = cl[order[oi]];
      if (!L.drawn) continue;
      MoLevel lv;
      lv.model = true;
      lv.clipped = L.clipped;
      lv.energy = L.clipped ? L.shownEnergy : L.energy;
      lv.irrep = L.irrep;
      lv.label = lower(L.base);
      if (L.character == MoLevel::ANTIBONDING) lv.label += "*";
      else if (L.character == MoLevel::NONBONDING && !lv.label.empty()) lv.label += " nb";
      lv.character = L.character;
      lv.pairing = L.pairing;
      lv.occupancy = L.occupancy;
      lv.occupancyBeta = L.occupancyBeta;
      lv.degeneracy = L.degeneracy;
      lv.shareLeft = L.shareLeft;
      lv.shareRight = L.shareRight;
      for (size_t k = 0; k < L.mos.size(); k++)
         lv.energies.push_back(L.clipped ? L.shownEnergy : modelE[L.mos[k]]);
      if (L.realLevel >= 0) {
         const MoLevel& R = real[L.realLevel];
         lv.orbitals = R.orbitals;
         std::sort(lv.orbitals.begin(), lv.orbitals.end());
         lv.hasCalc = true;
         lv.calcEnergy = R.energy;
         lv.calcOverlap = L.overlap;
         lv.calcLabel = lower(R.irrep.empty() && !L.irrep.empty() ? L.base : stripMarks(R.label)) +
                        (L.character == MoLevel::ANTIBONDING ? "*" : "");
         lv.calcOccupancy = R.occupancy;
         lv.calcOccupancyBeta = R.occupancyBeta;
         lv.calcDegeneracy = R.degeneracy > 0 ? R.degeneracy : (int)R.orbitals.size();
      }
      lv.annotation = L.clipped ? string("\xE2\x86\x91") : string();
      lv.tag = characterTag(charAcc[order[oi]], CHARACTER_MIN);
      //  The hover text.
      vector< std::pair<double,string> > parts;
      for (size_t s = 0; s < sets.size(); s++) {
         if (L.share[s] < 0.03) continue;
         const string nm = sets[s].fragment == 0
             ? (frags[0].kind == FragmentOrbitals::METAL ? in.symbols[in.leftAtoms[0]] + " " + sets[s].shell
                                                         : leftName.formula + " " + (sets[s].irrep.empty() ? string("FO") : lower(sets[s].irrep)))
             : (frags[1].kind == FragmentOrbitals::METAL ? in.symbols[in.rightAtoms[0]] + " " + sets[s].shell
                                                         : rightName.formula + " " + (sets[s].irrep.empty() ? string("FO") : lower(sets[s].irrep)));
         parts.push_back(std::make_pair(-L.share[s], nm + " " + percent(L.share[s])));
      }
      std::sort(parts.begin(), parts.end());
      string text = "Model MO (extended H\xC3\xBC" "ckel), E = " + signedNumber(L.energy*HARTREE, 2) +
                    " eV.  " + (L.character == MoLevel::BONDING ? "Bonding"
                      : L.character == MoLevel::ANTIBONDING ? "Antibonding" : "Non-bonding") +
                    " (overlap population " + signedNumber(L.op, 2) + ").\nMade of: ";
      for (size_t k = 0; k < parts.size() && k < 5; k++) text += (k ? ", " : "") + parts[k].second;
      if (L.realLevel >= 0) {
         const MoLevel& R = real[L.realLevel];
         text += "\nMatches your MO ";
         for (size_t k = 0; k < lv.orbitals.size();) {
            size_t e = k;
            while (e + 1 < lv.orbitals.size() && lv.orbitals[e + 1] == lv.orbitals[e] + 1) e++;
            text += (k ? ", " : "") + std::to_string(lv.orbitals[k] + 1);
            if (e > k) text += "\xE2\x80\x93" + std::to_string(lv.orbitals[e] + 1);
            k = e + 1;
         }
         text += " (E = " + signedNumber(R.energy*HARTREE, 2) + " eV, overlap " + fmt("%.2f", L.overlap) + ").";
         if (L.overlap < POOR_OVERLAP) text += "  A poor match: no calculated orbital resembles this model level.";
      }
      lv.detail = text;
      centreIndex[order[oi]] = (int)out.centre.levels.size();
      out.centre.levels.push_back(lv);
      centreReal.push_back(L.realLevel);
      centreTrueE.push_back(L.energy);
   }

   DonorSketch ds;
   if (centralAtom) {
      findDonorSketch(in.symbols, in.coordsAng, in.leftAtoms[0], frags[1].atoms, ds);
      if (ds.view == DonorSketch::NONE && !ds.why.empty())
         caveats.push_back("no phase sketches: " + ds.why + " has no single donor atom");
   }
   //  Fragment sets that carry a line (or a pair) are drawn.
   for (size_t s = 0; s < sets.size(); s++) {
      bool lined = false;
      for (size_t c = 0; c < cl.size(); c++)
         if (cl[c].drawn && cl[c].share[s] >= LINK_MIN) lined = true;
      if (!lined || (bandRule && sets[s].nsShare >= 0.5)) continue;
      MoColumn& col = sets[s].fragment == 0 ? out.left : out.right;
      MoLevel lv;
      lv.energy = sets[s].energy;
      lv.degeneracy = sets[s].degeneracy;
      lv.occupancy = sets[s].occupancy;
      lv.irrep = sets[s].irrep;
      if (frags[sets[s].fragment].kind == FragmentOrbitals::METAL) lv.label = sets[s].shell;
      else if (!sets[s].irrep.empty()) lv.label = lower(sets[s].irrep);
      else {
         int i0 = 1 << 30, i1 = 0;
         for (size_t q = 0; q < sets[s].fos.size(); q++) {
            i0 = std::min(i0, set.fos[sets[s].fos[q]].index);
            i1 = std::max(i1, set.fos[sets[s].fos[q]].index);
         }
         lv.label = "FO" + std::to_string(i0) + (i1 > i0 ? "-" + std::to_string(i1) : string());
      }
      lv.annotation.clear();
      lv.detail = "Fragment orbital set from extended H\xC3\xBC" "ckel, E = " +
                  signedNumber(sets[s].energy*HARTREE, 2) + " eV, " +
                  std::to_string((int)lround(sets[s].occupancy)) + " electrons.";
      const bool donorMode = centralAtom && sets[s].fragment == 1 &&
                             (ds.view == DonorSketch::FLAT || ds.view == DonorSketch::OBLIQUE);
      if (donorMode) {
         if (col.sketchX.empty()) {
            col.sketchX = ds.px; col.sketchY = ds.py; col.sketchZ = ds.pz;
            col.sketchView = ds.view == DonorSketch::OBLIQUE ? 1 : 0;
            col.sketchNormal = 2;
         }
         donorPhases(ds, set, set.fos[sets[s].fos[0]], lv);
      } else if (!(centralAtom && sets[s].fragment == 1 && ds.view != DonorSketch::LEGACY)) {
         if (col.sketchX.empty() && frags[sets[s].fragment].atoms.size() > 1)
            sketchFrame(in.coordsAng, frags[sets[s].fragment].atoms, col);
         if (!col.sketchX.empty())
            sketchPhases(set, set.fos[sets[s].fos[0]], in.symbols, in.coordsAng,
                         frags[sets[s].fragment].atoms, lv);
      }
      sets[s].drawn = (int)col.levels.size();
      col.levels.push_back(lv);
   }
   //  Both columns ascending in energy, links remapped through the order.
   for (int side = 0; side < 2; side++) {
      MoColumn& col = side == 0 ? out.left : out.right;
      vector<size_t> idx(col.levels.size());
      for (size_t i = 0; i < idx.size(); i++) idx[i] = i;
      std::stable_sort(idx.begin(), idx.end(),
                       [&](size_t a, size_t b) { return col.levels[a].energy < col.levels[b].energy; });
      vector<MoLevel> sorted;
      vector<int> where(idx.size());
      for (size_t i = 0; i < idx.size(); i++) { where[idx[i]] = (int)i; sorted.push_back(col.levels[idx[i]]); }
      col.levels = sorted;
      for (size_t s = 0; s < sets.size(); s++)
         if (sets[s].fragment == side && sets[s].drawn >= 0) sets[s].drawn = where[sets[s].drawn];
   }
   //  Two sets of one irrep in a column are told apart by their orbitals.
   for (int side = 0; side < 2; side++) {
      MoColumn& col = side == 0 ? out.left : out.right;
      for (size_t i = 0; i < col.levels.size(); i++) {
         int same = 0;
         for (size_t j = 0; j < col.levels.size(); j++) same += col.levels[j].label == col.levels[i].label;
         if (same < 2) continue;
         for (size_t s2 = 0; s2 < sets.size(); s2++) {
            if (sets[s2].fragment != side || sets[s2].drawn != (int)i) continue;
            int i0 = 1 << 30, i1 = 0;
            for (size_t q = 0; q < sets[s2].fos.size(); q++) {
               i0 = std::min(i0, set.fos[sets[s2].fos[q]].index);
               i1 = std::max(i1, set.fos[sets[s2].fos[q]].index);
            }
            col.levels[i].label += " (FO" + std::to_string(i0) + (i1 > i0 ? "-" + std::to_string(i1) : string()) + ")";
         }
      }
   }

   for (size_t c = 0; c < cl.size(); c++) {
      if (!cl[c].drawn) continue;
      for (size_t s = 0; s < sets.size(); s++) {
         if (sets[s].drawn < 0 || cl[c].share[s] < LINK_MIN) continue;
         MoConnection k;
         k.centreLevel = centreIndex[c];
         (sets[s].fragment == 0 ? k.leftLevel : k.rightLevel) = sets[s].drawn;
         k.weight = cl[c].share[s];
         out.links.push_back(k);
      }
   }

   //  Ligand-internal levels: grey bars with their level and electron counts.
   for (int occupied = 1; occupied >= 0; occupied--) {
      double lo = 1e30, hi = -1e30;
      int levels = 0, coreLv = 0;
      double e = 0.0;
      for (size_t c = 0; c < cl.size(); c++) {
         if (cl[c].drawn || tooHigh[c] || cl[c].lfFold) continue;
         if ((cl[c].occupancy + std::max(0.0, cl[c].occupancyBeta) > 0.0) != (occupied == 1)) continue;
         lo = std::min(lo, cl[c].energy); hi = std::max(hi, cl[c].energy);
         levels += cl[c].degeneracy;
         if (cl[c].core) coreLv += cl[c].degeneracy;
         e += cl[c].occupancy + std::max(0.0, cl[c].occupancyBeta);
      }
      if (levels == 0) continue;
      //  A bar never runs off the axis the drawn levels make.
      double floorE = 1e30, ceilE = -1e30;
      for (size_t i = 0; i < out.centre.levels.size(); i++) {
         floorE = std::min(floorE, out.centre.levels[i].energy);
         if (!out.centre.levels[i].clipped) ceilE = std::max(ceilE, out.centre.levels[i].energy);
      }
      if (floorE < 1e29) { lo = std::max(lo, floorE - 3.0/HARTREE); hi = std::min(hi, ceilE); }
      if (hi < lo) hi = lo;
      MoBand b;
      b.low = lo; b.high = hi; b.levels = levels;
      b.label = rightName.formula + (bandRule ? " internal" : "") + ": " + std::to_string(levels) +
                (levels == 1 ? " level" : " levels") +
                (occupied == 1 ? ", " + std::to_string((int)lround(e)) + " e" : string(", empty")) +
                (coreLv ? " (" + std::to_string(coreLv) + " core-like " + (coreLv == 1 ? "level" : "levels") + ")" : string());
      out.right.bands.push_back(b);
   }

   //  The ligand-field view's own note: what it folded away, and that it did.
   if (lfView) {
      double lo = 1e30, hi = -1e30;
      int levels = 0;
      map<string,double> acc;
      for (size_t c = 0; c < cl.size(); c++) {
         if (!cl[c].lfFold) continue;
         lo = std::min(lo, cl[c].energy); hi = std::max(hi, cl[c].energy);
         levels += cl[c].degeneracy;
         for (map<string,double>::iterator it = charAcc[c].begin(); it != charAcc[c].end(); ++it)
            acc[it->first] += it->second*cl[c].degeneracy;
      }
      if (levels > 0) {
         string kind = "ligand-centred";
         string top = characterTag(acc, 1e9);          // the single largest
         const string::size_type sp = top.find(' ');
         if (!top.empty() && sp != string::npos && top.find(" + ") == string::npos) kind = "ligand " + top.substr(sp + 1);
         MoBand b;
         b.low = lo; b.high = hi; b.levels = levels;
         b.label = kind + " levels: " + std::to_string(levels) + (levels == 1 ? " level" : " levels") +
                   ", empty (Ligand-field view)";
         out.right.bands.push_back(b);
         lfFolded = true;
      }
   }

   //  --- the verdict -------------------------------------------------
   //  Only the occupied levels, and the LUMO when the model's LUMO is the
   //  calculation's; order above the LUMO is a basis-set matter (About).
   bool lumoDiffers = false, lumoJudged = false, aboveDiffers = false;
   {
      const size_t nC = out.centre.levels.size();
      auto occupied = [&](size_t i) {
         const MoLevel& l = out.centre.levels[i];
         return l.occupancy + std::max(0.0, l.occupancyBeta) > 0.0;
      };
      auto plain = [](string t) { if (t.size() > 3 && t.compare(t.size() - 3, 3, " nb") == 0) t.erase(t.size() - 3); return t; };
      vector<int> vi;                        // levels with a calculated partner
      for (size_t i = 0; i < nC; i++)
         if (out.centre.levels[i].hasCalc) vi.push_back((int)i);

      //  The calculation's LUMO: its lowest level without electrons.
      int calcLumo = -1;
      bool anyOcc = false;
      for (size_t R = 0; R < real.size(); R++)
         if (real[R].occupancy + std::max(0.0, real[R].occupancyBeta) > 0.0) anyOcc = true;
      if (anyOcc)
         for (size_t R = 0; R < real.size(); R++) {
            if (real[R].occupancy + std::max(0.0, real[R].occupancyBeta) > 0.0) continue;
            if (calcLumo < 0 || real[R].energy < real[calcLumo].energy) calcLumo = (int)R;
         }
      //  The model's LUMO: its lowest drawn empty level.
      int modelLumo = -1;
      for (size_t k = 0; k < vi.size(); k++) {
         if (occupied(vi[k])) continue;
         if (modelLumo < 0 || centreTrueE[vi[k]] < centreTrueE[modelLumo]) modelLumo = vi[k];
      }
      if (lfView) modelLumo = -1;           // the empty ligand levels are not drawn, so not judged
      if (modelLumo >= 0 && calcLumo >= 0) {
         if (centreReal[modelLumo] == calcLumo) lumoJudged = true;
         else lumoDiffers = true;
      }
      //  Above the LUMO: does the model's order of the empty levels hold?
      for (size_t a2 = 0; a2 < vi.size(); a2++)
         for (size_t b2 = 0; b2 < vi.size(); b2++) {
            if (occupied(vi[a2]) || occupied(vi[b2]) || vi[a2] == modelLumo || vi[b2] == modelLumo) continue;
            if (centreTrueE[vi[a2]] < centreTrueE[vi[b2]] &&
                out.centre.levels[vi[a2]].calcEnergy > out.centre.levels[vi[b2]].calcEnergy + SAME_ENERGY)
               aboveDiffers = true;
         }

      //  The calculation's HOMO and LUMO are always drawn, matched or not.
      int calcHomo = -1;
      for (size_t R = 0; R < real.size(); R++)
         if (real[R].occupancy + std::max(0.0, real[R].occupancyBeta) > 0.0 &&
             (calcHomo < 0 || real[R].energy > real[calcHomo].energy)) calcHomo = (int)R;
      for (int pass = 0; pass < 2; pass++) {
         if (pass == 1 && lfView) continue;      // its LUMO is a ligand level
         const int R = pass == 0 ? calcHomo : calcLumo;
         if (R < 0 || std::find(centreReal.begin(), centreReal.end(), R) != centreReal.end()) continue;
         //  What the model folds as internal is folded in the calculation too.
         bool folded = false;
         for (size_t c = 0; c < cl.size(); c++)
            if (!cl[c].drawn && !tooHigh[c] && !cl[c].lfFold && cl[c].realLevel == R) folded = true;
         if (folded) continue;
         MoLevel lv;
         lv.energy = real[R].energy;
         lv.irrep = real[R].irrep;
         lv.label = lv.calcLabel = lower(stripMarks(real[R].label));
         lv.orbitals = real[R].orbitals;
         std::sort(lv.orbitals.begin(), lv.orbitals.end());
         lv.occupancy = lv.calcOccupancy = real[R].occupancy;
         lv.occupancyBeta = lv.calcOccupancyBeta = real[R].occupancyBeta;
         lv.degeneracy = lv.calcDegeneracy = real[R].degeneracy > 0 ? real[R].degeneracy : (int)real[R].orbitals.size();
         lv.hasCalc = true;
         lv.calcEnergy = real[R].energy;
         lv.detail = pass == 0 ? "Your HOMO: no model partner." : "Your LUMO: no model partner.";
         out.centre.calcOnly.push_back(lv);
      }

      //  The judged levels, in the model's energy order.
      vector<int> judged;
      for (size_t k = 0; k < vi.size(); k++)
         if (occupied(vi[k]) || (lumoJudged && vi[k] == modelLumo)) judged.push_back(vi[k]);
      std::stable_sort(judged.begin(), judged.end(), [&](int a2, int b2) { return centreTrueE[a2] < centreTrueE[b2]; });
      out.valenceLevels = (int)judged.size();
      int pairs = 0;
      vector<string> bad;
      double lo = 1.0, hi = 0.0;
      for (size_t x = 0; x < judged.size(); x++) {
         const MoLevel& i1 = out.centre.levels[judged[x]];
         lo = std::min(lo, i1.calcOverlap);
         hi = std::max(hi, i1.calcOverlap);
         for (size_t y = x + 1; y < judged.size(); y++) {
            pairs++;
            const MoLevel& j1 = out.centre.levels[judged[y]];
            if (i1.calcEnergy > j1.calcEnergy + SAME_ENERGY) {
               out.inversions++;
               if (bad.size() < 3) bad.push_back("the model puts " + plain(i1.label) + " below " + plain(j1.label));
            }
         }
      }
      out.overlapMin = lo; out.overlapMax = hi;

      string lumoSentence;
      if (lumoDiffers)
         lumoSentence = "The empty levels differ: your LUMO is " + lower(stripMarks(real[calcLumo].label)) +
                        ", the model's is " + plain(out.centre.levels[modelLumo].label) + ".";
      if (judged.empty()) {
         out.verdict = lumoSentence;
         if (lumoDiffers) out.verdictOk = false;
      } else if (out.inversions == 0) {
         out.verdict = string("The model explains your occupied orbitals") + (lumoJudged ? " and the LUMO" : "") +
                       " in the right order.";
         if (lumoDiffers) { out.verdict += " " + lumoSentence; out.verdictOk = false; }
      } else {
         out.verdictOk = false;
         string first = joinNames(bad);
         first[0] = (char)toupper((unsigned char)first[0]);
         out.verdict = first + "; your calculation has " + (out.inversions == 1 ? "them" : "those") +
                       " the other way round (" + std::to_string(out.inversions) + " of " + std::to_string(pairs) +
                       " pairs of the " + std::to_string(judged.size()) + " levels judged).";
         if (lumoDiffers) out.verdict += " " + lumoSentence;
      }
      vector<const MoLevel*> v;              // every level with a calculated partner
      for (size_t k = 0; k < vi.size(); k++) v.push_back(&out.centre.levels[vi[k]]);
      //  Delta_o of an octahedral complex.
      const MoLevel *t2g = 0, *eg = 0;
      for (size_t i = 0; i < v.size(); i++) {
         const string ir = lower(v[i]->irrep);
         if (v[i]->shareLeft < 0.5) continue;
         if (ir == "t2g" && (t2g == 0 || v[i]->shareLeft > t2g->shareLeft)) t2g = v[i];
         if (ir == "eg" && (eg == 0 || v[i]->shareLeft > eg->shareLeft)) eg = v[i];
      }
      if (t2g && eg && in.leftMetal) {
         out.haveDeltaO = true;
         out.deltaCalcEv = fabs(eg->calcEnergy - t2g->calcEnergy)*HARTREE;
      }
      int poor = 0;
      for (size_t i = 0; i < v.size(); i++) if (v[i]->calcOverlap < POOR_OVERLAP) poor++;
      if (poor > 0)
         caveats.push_back(std::to_string(poor) + (poor == 1 ? " level matches" : " levels match") +
             " the calculation only loosely (overlap < " + fmt("%.2f", POOR_OVERLAP) + ")");
   }
   if (!caveats.empty()) out.warnings.push_back(joinCaveats(caveats));

   //  --- the text ----------------------------------------------------
   out.summary = "Model: order and approximate spacing from extended H\xC3\xBC" "ckel (qualitative) \xC2\xB7 "
                 "calculation in eV";
   if (out.haveDeltaO)
      out.summary += " \xC2\xB7 \xCE\x94o in your calculation: " + fmt("%.1f", out.deltaCalcEv) + " eV";
   if (!out.right.bands.empty()) {
      string t;
      bool plainBands = false;
      for (size_t i = 0; i < out.right.bands.size(); i++) {
         const string& l = out.right.bands[i].label;
         const bool lf = l.find("(Ligand-field view)") != string::npos;
         if (!lf) plainBands = true;
         t += i == 0 || lf ? (i ? "; " : "") + l : "; " + l.substr(l.find(": ") == string::npos ? 0 : l.find(": ") + 2);
      }
      out.summary += " \xC2\xB7 not drawn: " + t;
      if (plainBands) out.summary += "; tick Show all levels";
      if (lfFolded) out.summary += "; untick Ligand-field view to draw them";
      out.foldedAny = true;
   }
   //  Where every electron is: per fragment (drawn, folded as core-like ns,
   //  folded as internal) and in the model column; each adds up to its total.
   {
      string part = "electrons: ";
      for (int f = 0; f < 2; f++) {
         double all = 0, drawnE = 0, coreE = 0, otherE = 0;
         int coreL = 0, otherL = 0, odd = 0;
         string coreName;
         for (size_t k = 0; k < sets.size(); k++) {
            if (sets[k].fragment != f || !setOdd[k]) continue;
            all += sets[k].occupancy;
            odd += (int)lround(sets[k].occupancy <= sets[k].degeneracy ? sets[k].occupancy : 2.0*sets[k].degeneracy - sets[k].occupancy);
            if (sets[k].drawn >= 0) drawnE += sets[k].occupancy;
            else if (bandRule && sets[k].nsShare >= 0.5) {
               coreE += sets[k].occupancy; coreL += sets[k].degeneracy;
               if (coreName.empty())
                  for (map<string,double>::iterator it = sets[k].chars.begin(); it != sets[k].chars.end(); ++it)
                     if (!it->first.empty() && it->first.back() == 's') coreName = it->first;
            } else { otherE += sets[k].occupancy; otherL += sets[k].degeneracy; }
         }
         const string name = f ? rightName.formula : leftName.formula;
         auto n = [](double x) { return std::to_string((int)lround(x)); };
         part += (f ? "; " : "") + name + " " + n(all);
         if (frags[f].charge == 0 && odd > 0) part += " (neutral: " + std::to_string(odd) + " unpaired e)";
         part += ", drawn " + n(drawnE);
         if (coreL > 0) part += ", " + (coreName.empty() ? string("ns") : coreName) + " core-like " + std::to_string(coreL) +
                                (coreL == 1 ? " level " : " levels ") + n(coreE) + " e";
         if (otherL > 0) part += ", internal " + std::to_string(otherL) + (otherL == 1 ? " level " : " levels ") + n(otherE) + " e";
      }
      double mDrawn = 0, mFold = 0, mCore = 0;
      int mFoldL = 0, mCoreL = 0;
      for (size_t i = 0; i < out.centre.levels.size(); i++)
         mDrawn += out.centre.levels[i].occupancy + std::max(0.0, out.centre.levels[i].occupancyBeta);
      for (size_t c = 0; c < cl.size(); c++) {
         if (cl[c].drawn || tooHigh[c] || cl[c].lfFold) continue;
         const double e = cl[c].occupancy + std::max(0.0, cl[c].occupancyBeta);
         mFold += e; mFoldL += cl[c].degeneracy;
         if (cl[c].core) { mCore += e; mCoreL += cl[c].degeneracy; }
      }
      part += "; model and calculation: " + std::to_string((int)lround(mDrawn)) + " drawn + " +
              std::to_string((int)lround(mFold)) + " folded (" + std::to_string(mFoldL) + " levels";
      if (mCoreL > 0) part += ", " + std::to_string(mCoreL) + " core-like, " + std::to_string((int)lround(mCore)) + " e";
      part += ") = " + std::to_string((int)lround(mDrawn + mFold));
      out.summary += " \xC2\xB7 " + part;
   }
   {
      int total = 0;
      string aboutVerdict;
      if (out.valenceLevels > 0 && out.inversions == 0)
         aboutVerdict = string("The model explains your occupied orbitals") + (lumoJudged ? " and the LUMO" : "") +
                        " in the right order. ";
      if (aboveDiffers)
         aboutVerdict += "The order of some empty levels above the LUMO differs between the model and your calculation. ";
      aboutVerdict += "Virtual orbitals above the LUMO depend strongly on the basis set.";
      for (size_t i = 0; i < sets.size(); i++) total += (int)lround(sets[i].occupancy);
      char b[64];
      snprintf(b, sizeof b, "%d", in.piOnly ? piElectrons : electrons);
      out.about = "The extended H\xC3\xBC" "ckel diagram explains the calculation's orbitals: nothing more, nothing less.\n\n"
          "**The model.** Every column is extended H\xC3\xBC" "ckel (Alvarez parameters). The middle "
          "column is the molecule's own extended H\xC3\xBC" "ckel run, the side columns are the two fragments' "
          "(" + leftName.formula + " and " + rightName.formula + "). The basis is the same in all three, so the lines "
          "are an exact projection of each model MO on the fragment orbitals: a line joins a model MO to a "
          "fragment level that makes up at least 10 % of it.\n\n"
          "**Bonding and antibonding** are read from the sign of each model MO's overlap population between the "
          "two fragments, then paired per symmetry label and degeneracy, so each antibonding level has its bonding "
          "partner; a level without a partner is non-bonding. Colours mark the pairs.\n\n"
          "**Scale.** The model columns show the order of the levels and their approximate spacing from extended H\xC3\xBC" "ckel, not a scale: close levels are drawn further apart so they can be read, and no model energy is printed (it is in the hover text). Extended H\xC3\xBC" "ckel antibonding energies are qualitative and typically far too high for short bonds to diffuse atoms such as B, Be and Al.\n\n"
          "**The calculation** is the column on the right, on its own energy axis in eV (not on the model's scale). For each model level it shows the calculated "
          "orbital (or set of near-degenerate orbitals) with the largest overlap with it, at its calculated energy, "
          "in the colour of the model level it matches. A level marked \xE2\x89\x88 matches only loosely (overlap below 0.80; the value is in the hover text). Labels are those of the matched calculated orbital. Clicking a "
          "level shows that orbital in 3-D.\n\n" + aboutVerdict + "\n\n**Total electrons: " + string(b) + "** (" + leftName.formula + " " +
          std::to_string((int)lround([&] { double s = 0; for (size_t i = 0; i < sets.size(); i++) if (sets[i].fragment == 0) s += sets[i].occupancy; return s; }())) +
          ", " + rightName.formula + " " +
          std::to_string((int)lround([&] { double s = 0; for (size_t i = 0; i < sets.size(); i++) if (sets[i].fragment == 1) s += sets[i].occupancy; return s; }())) + ").";
      (void)total;
      out.about += "\n\nEach model level carries a short grey tag naming the fragment orbitals it is mostly made of "
                   "(shares of at least 20 %), such as Cr d or CO \xCF\x80*. Ligand-field view hides empty "
                   "ligand-centred levels such as CO \xCF\x80*; transitions into them are charge transfer, not d\xE2\x80\x93" "d.";
      if (out.foldedAny)
         out.about += "\n\nThe levels counted in the note under the diagram are model levels that sit almost entirely on the " + rightName.formula +
                      " and take no part in the interaction; they are counted, not drawn, in the calculation's column as well. "
                      "The ns orbitals of a ligand atom that also has np (F 2s, Cl 3s) are treated as core-like here: "
                      "the textbook sigma bonds are made of np.";
      if (!caveats.empty()) {
         out.about += "\n\n**Caveats.** ";
         for (size_t i = 0; i < caveats.size(); i++) out.about += (i ? "; " : "") + caveats[i];
         out.about += ". Extended H\xC3\xBC" "ckel puts empty antibonding orbitals tens of eV up, so an empty level more than " +
                      fmt("%.0f", WINDOW_EV) + " eV above the HOMO is drawn at the top of the diagram, marked with an up arrow, "
                      "or not at all when it sits almost entirely on one fragment. A loose match means no "
                      "calculated orbital resembles the model level closely; read its calculated energy with care.";
      }
   }
   return true;
}
