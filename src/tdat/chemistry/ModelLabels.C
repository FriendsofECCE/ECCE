//  Construction labels of the model-first MO diagram (#458); see ModelDiagram.H.

#include <algorithm>
#include <cmath>
#include <map>

#include "tdat/ModelDiagram.H"
#include "tdat/FragmentOrbitals.H"

using namespace std;

namespace {

const char* SIGMA = "\xCF\x83";
const char* PI    = "\xCF\x80";
const char* DELTA = "\xCE\xB4";

typedef vector< vector<double> > Mat;

struct Level
{
   vector<int> mos;
   double energy;
   Level() : energy(0) {}
};

vector<Level> clusters(const vector<double>& E)
{
   vector<Level> cl;
   for (size_t m = 0; m < E.size(); m++) {
      if (!cl.empty() && fabs(E[m] - E[cl.back().mos.back()]) < ModelDiagram::DEGENERATE)
         cl.back().mos.push_back((int)m);
      else { cl.push_back(Level()); cl.back().mos.push_back((int)m); }
   }
   for (size_t c = 0; c < cl.size(); c++) {
      for (size_t k = 0; k < cl[c].mos.size(); k++) cl[c].energy += E[cl[c].mos[k]];
      cl[c].energy /= cl[c].mos.size();
   }
   return cl;
}

//  Orbitals normalised against their own overlap matrix.
void normalise(const vector<double>& S, size_t nf, Mat& rows)
{
   for (size_t m = 0; m < rows.size(); m++) {
      double n = 0.0;
      for (size_t i = 0; i < nf; i++) {
         double sc = 0.0;
         for (size_t j = 0; j < nf; j++) sc += S[i*nf+j]*rows[m][j];
         n += rows[m][i]*sc;
      }
      if (n > 1e-12) for (size_t i = 0; i < nf; i++) rows[m][i] /= sqrt(n);
   }
}

string shellName(const FragmentOrbitals::FoSet& set, size_t f)
{
   static const char letters[] = "spdf";
   const FragmentOrbitals::Function& fn = set.funcs[f];
   return fn.element + " " + to_string(fn.n) + string(1, fn.l < 4 ? letters[fn.l] : '?');
}

//  Leading shell of a group of atoms in one orbital (largest summed square).
string leadingShell(const FragmentOrbitals::FoSet& set, const vector<double>& c,
                    const vector<int>& atoms, bool nameElementOnly = false)
{
   (void)nameElementOnly;
   map<string,double> w;
   for (size_t i = 0; i < set.funcs.size(); i++)
      if (find(atoms.begin(), atoms.end(), set.funcs[i].atom) != atoms.end())
         w[shellName(set, i)] += c[i]*c[i];
   string best;
   double top = -1.0;
   for (map<string,double>::iterator it = w.begin(); it != w.end(); ++it)
      if (it->second > top) { top = it->second; best = it->first; }
   return best;
}

double dot(const double a[3], const double b[3]) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }

//  The weight of an orbital with 0 / 1 / 2 nodal planes containing the axis
//  (sigma / pi / delta), summed over the atoms in `atoms`.  s and d_z2 have none,
//  p along the axis none, p across it one, d by its m about the axis.
void axisWeights(const FragmentOrbitals::FoSet& set, const vector<double>& c,
                 const vector<int>& atoms, const double axis[3], double& par, double& perp,
                 double& delta)
{
   par = perp = delta = 0.0;
   for (size_t a = 0; a < atoms.size(); a++) {
      double v[3] = {0, 0, 0}, T[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
      const double r3 = sqrt(3.0);
      for (size_t i = 0; i < set.funcs.size(); i++) {
         const FragmentOrbitals::Function& f = set.funcs[i];
         if (f.atom != atoms[a]) continue;
         if (f.l == 0) par += c[i]*c[i];
         else if (f.l == 1) v[f.component == "x" ? 0 : (f.component == "y" ? 1 : 2)] += c[i];
         else if (f.l == 2) {      // a d function is a traceless symmetric tensor T: sum T_ab r_a r_b
            const string& n = f.component;
            if (n == "z2") { T[0][0] -= 0.5*c[i]; T[1][1] -= 0.5*c[i]; T[2][2] += c[i]; }
            else if (n == "x2-y2") { T[0][0] += 0.5*r3*c[i]; T[1][1] -= 0.5*r3*c[i]; }
            else if (n == "xy") { T[0][1] += 0.5*r3*c[i]; T[1][0] += 0.5*r3*c[i]; }
            else if (n == "xz") { T[0][2] += 0.5*r3*c[i]; T[2][0] += 0.5*r3*c[i]; }
            else if (n == "yz") { T[1][2] += 0.5*r3*c[i]; T[2][1] += 0.5*r3*c[i]; }
         }
      }
      const double p = dot(v, axis);
      par += p*p;
      perp += max(0.0, dot(v, v) - p*p);
      //  m content of T about the axis; each real d function has |T|^2 = 1.5.
      double e1[3], e2[3];
      const double t[3] = {fabs(axis[0]) < 0.6 ? 1.0 : 0.0, fabs(axis[0]) < 0.6 ? 0.0 : 1.0, 0.0};
      const double dd = dot(t, axis);
      for (int k = 0; k < 3; k++) e1[k] = t[k] - dd*axis[k];
      double n1 = sqrt(dot(e1, e1));
      for (int k = 0; k < 3; k++) e1[k] /= n1;
      e2[0] = axis[1]*e1[2] - axis[2]*e1[1]; e2[1] = axis[2]*e1[0] - axis[0]*e1[2]; e2[2] = axis[0]*e1[1] - axis[1]*e1[0];
      auto q = [&](const double* x, const double* y) {
         double r = 0.0;
         for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) r += x[i]*T[i][j]*y[j];
         return r;
      };
      const double tuu = q(axis, axis), tu1 = q(axis, e1), tu2 = q(axis, e2), t11 = q(e1, e1), t22 = q(e2, e2), t12 = q(e1, e2);
      par += 1.5*tuu*tuu/1.5;
      perp += 2.0*(tu1*tu1 + tu2*tu2)/1.5;
      delta += (0.5*(t11 - t22)*(t11 - t22) + 2.0*t12*t12)/1.5;
   }
}

}  // namespace


bool ModelDiagram::constructLabels(const Input& in, vector<ConstructLabel>& out, string& why)
{
   out.clear();
   const size_t nAtoms = in.symbols.size();
   if (nAtoms < 2 || in.coordsAng.size() != 3*nAtoms || in.leftAtoms.empty() || in.rightAtoms.empty()) {
      why = "No structure.";
      return false;
   }
   if (!applicable(in.symbols, why)) return false;

   vector<FragmentOrbitals::Fragment> frags(2);
   frags[0].atoms = in.leftAtoms;  frags[0].charge = in.leftCharge;
   frags[1].atoms = in.rightAtoms; frags[1].charge = in.rightCharge;
   for (int f = 0; f < 2; f++) {
      const bool single = frags[f].atoms.size() == 1;
      frags[f].kind = single ? FragmentOrbitals::METAL : FragmentOrbitals::LIGAND;
      frags[f].name = single ? in.symbols[frags[f].atoms[0]] : string(f ? "R" : "L");
   }
   FragmentOrbitals::FoSet set;
   if (!FragmentOrbitals::build(in.symbols, in.coordsAng, frags, FragmentOrbitals::PER_LIGAND, set, why))
      return false;
   const size_t nf = set.funcs.size();
   vector<double> E;
   Mat rows;
   if (!FragmentOrbitals::solveWhole(set, E, rows, why)) return false;
   normalise(set.overlap, nf, rows);
   Mat amp;
   {
      vector<EspBasisFunction> own;
      for (size_t i = 0; i < nf; i++) own.push_back(set.funcs[i].fn);
      vector<FragmentOrbitals::MoShares> sh;
      if (!FragmentOrbitals::compose(set, own, set.overlap, rows, sh, why, &amp)) return false;
   }

   vector<Level> cl = clusters(E);
   out.resize(cl.size());
   vector<double> shL(cl.size(), 0.0), shR(cl.size(), 0.0);
   for (size_t c = 0; c < cl.size(); c++) {
      out[c].orbitals = cl[c].mos;
      out[c].energy = cl[c].energy;
      for (size_t k = 0; k < cl[c].mos.size(); k++)
         for (size_t q = 0; q < set.fos.size(); q++) {
            const double a = amp[cl[c].mos[k]][q];
            (set.fos[q].fragment == 0 ? shL[c] : shR[c]) += a*a/cl[c].mos.size();
         }
      out[c].otherShare = min(shL[c], shR[c]);
   }

   const bool diatomic = nAtoms == 2;
   const bool central = in.leftAtoms.size() == 1 && in.rightAtoms.size() >= 2;
   if (!diatomic && !central) return true;      // no clear bond axis: no construction label

   const int a0 = in.leftAtoms[0], a1 = in.rightAtoms[0];
   double axis[3];
   {
      double n = 0.0;
      for (int k = 0; k < 3; k++) { axis[k] = in.coordsAng[3*a1+k] - in.coordsAng[3*a0+k]; n += axis[k]*axis[k]; }
      n = sqrt(n);
      for (int k = 0; k < 3; k++) axis[k] /= (n > 0 ? n : 1.0);
   }

   if (central) {
      const double* pc = &in.coordsAng[3*in.leftAtoms[0]];
      //  sigma/pi content of a coefficient vector on the ligand atoms, about each centre-ligand bond.
      auto ligandSigmaPi = [&](const vector<double>& row, double& sg, double& pp, double& dl) {
         sg = pp = dl = 0.0;
         for (size_t a = 0; a < in.rightAtoms.size(); a++) {
            double u[3], n = 0.0;
            for (int q = 0; q < 3; q++) { u[q] = in.coordsAng[3*in.rightAtoms[a]+q] - pc[q]; n += u[q]*u[q]; }
            n = sqrt(n);
            for (int q = 0; q < 3; q++) u[q] /= (n > 0 ? n : 1.0);
            vector<int> one(1, in.rightAtoms[a]);
            double par, perp, d;
            axisWeights(set, row, one, u, par, perp, d);
            sg += par;  pp += perp;  dl += d;
         }
      };
      //  A level is sigma or pi by the ligand fragment orbitals it is made of
      //  (a degenerate set of them taken whole, as its members mix freely).
      vector< vector<int> > groups;
      FragmentOrbitals::groupFos(set, groups);
      vector< vector<double> > groupFrac(groups.size(), vector<double>(3, 0.0));
      for (size_t g = 0; g < groups.size(); g++) {
         if (set.fos[groups[g][0]].fragment != 1) continue;
         double sg = 0.0, pp = 0.0, dl = 0.0;
         for (size_t q = 0; q < groups[g].size(); q++) {
            double a, b, d;
            ligandSigmaPi(set.fos[groups[g][q]].coef, a, b, d);
            sg += a;  pp += b;  dl += d;
         }
         const double tot = sg + pp + dl;
         if (tot > 1e-12) { groupFrac[g][0] = sg/tot; groupFrac[g][1] = pp/tot; groupFrac[g][2] = dl/tot; }
         else groupFrac[g][0] = 1.0;
      }
      for (size_t c = 0; c < cl.size(); c++) {
         ConstructLabel& L = out[c];
         double W3[3] = {0, 0, 0};
         for (size_t g = 0; g < groups.size(); g++) {
            if (set.fos[groups[g][0]].fragment != 1) continue;
            double share = 0.0;
            for (size_t k = 0; k < cl[c].mos.size(); k++)
               for (size_t q = 0; q < groups[g].size(); q++) {
                  const double a = amp[cl[c].mos[k]][groups[g][q]];
                  share += a*a;
               }
            for (int t = 0; t < 3; t++) W3[t] += share*groupFrac[g][t];
         }
         const vector<double>& lead = rows[cl[c].mos[0]];
         const bool onCentre = shL[c] >= shR[c];
         const string parent = onCentre ? leadingShell(set, lead, in.leftAtoms)
                                        : leadingShell(set, lead, in.rightAtoms);
         if (L.otherShare < NONBONDING_SHARE) {
            L.nonbonding = true;
            L.plain = "nb";
            L.full = "nb (" + parent + ")";
            continue;
         }
         const double tot3 = W3[0] + W3[1] + W3[2];
         int dom = 0;
         for (int t = 1; t < 3; t++) if (W3[t] > W3[dom]) dom = t;
         const char* names[3] = {SIGMA, PI, DELTA};
         const char* kind = names[dom];
         L.nodal = dom;
         L.sigmaShare = tot3 > 1e-12 ? W3[0]/tot3 : 0.5;
         {
            string bond = in.symbols[in.leftAtoms[0]] + "\xE2\x80\x93";
            vector<string> els;
            for (size_t a2 = 0; a2 < in.rightAtoms.size(); a2++)
               if (find(els.begin(), els.end(), in.symbols[in.rightAtoms[a2]]) == els.end()) els.push_back(in.symbols[in.rightAtoms[a2]]);
            for (size_t e = 0; e < els.size(); e++) bond += (e ? "/" : "") + els[e];
            static const char* planes[3] = {"no nodal plane", "a nodal plane", "two nodal planes"};
            const int pct = (int)lround(100.0*W3[dom]/(tot3 > 1e-12 ? tot3 : 1.0));
            L.split = string(kind) + ": " + to_string(pct) + " % with " + planes[dom] + " containing the " + bond + " bond";
            for (int t = 0; t < 3; t++) {
               const int pt = (int)lround(100.0*W3[t]/(tot3 > 1e-12 ? tot3 : 1.0));
               if (t != dom && pt > 0) L.split += ", " + to_string(pt) + " % " + names[t];
            }
            L.split += ".";
         }
         const string centreParent = leadingShell(set, lead, in.leftAtoms);
         //  Phase of the leading centre orbital against the leading ligand
         //  orbital that overlaps it, summed over a degenerate set.
         double phase = 0.0;
         for (size_t k = 0; k < cl[c].mos.size(); k++) {
            const vector<double>& a = amp[cl[c].mos[k]];
            int fl = -1;
            for (size_t q = 0; q < set.fos.size(); q++)
               if (set.fos[q].fragment == 0 && (fl < 0 || fabs(a[q]) > fabs(a[fl]))) fl = (int)q;
            vector<int> rs;
            for (size_t q = 0; q < set.fos.size(); q++)
               if (set.fos[q].fragment == 1 && fabs(a[q]) > 0.2) rs.push_back((int)q);
            sort(rs.begin(), rs.end(), [&](int x, int y) { return fabs(a[x]) > fabs(a[y]); });
            for (size_t r = 0; r < rs.size() && fl >= 0; r++) {
               double ov = 0.0;
               for (size_t i = 0; i < nf; i++)
                  for (size_t j = 0; j < nf; j++)
                     ov += set.fos[fl].coef[i]*set.overlap[i*nf+j]*set.fos[rs[r]].coef[j];
               if (fabs(ov) < 0.02) continue;
               phase += a[fl]*a[rs[r]]*ov;
               break;
            }
         }
         L.antibonding = phase < 0.0;
         const string star = L.antibonding ? "*" : "";
         L.plain = string(kind) + star;
         L.full = string(kind) + star + "(" + centreParent + ")";
      }
      return true;
   }

   //  --- diatomic: correlate with the s-p-unmixed model --------------
   const bool homo = in.symbols[a0] == in.symbols[a1];
   FragmentOrbitals::FoSet un = set;
   for (size_t i = 0; i < nf; i++)
      for (size_t j = 0; j < nf; j++)
         if (set.funcs[i].l != set.funcs[j].l) un.overlap[i*nf+j] = 0.0;
   vector<double> uE;
   Mat uRows;
   if (!FragmentOrbitals::solveWhole(un, uE, uRows, why)) return false;
   normalise(un.overlap, nf, uRows);
   vector<Level> ucl = clusters(uE);

   //  Partner of each function under inversion (homonuclear only).
   vector<int> partner(nf, -1);
   if (homo)
      for (size_t i = 0; i < nf; i++)
         for (size_t j = 0; j < nf; j++)
            if (set.funcs[i].atom != set.funcs[j].atom && set.funcs[i].n == set.funcs[j].n &&
                set.funcs[i].l == set.funcs[j].l && set.funcs[i].component == set.funcs[j].component)
               partner[i] = (int)j;

   struct Kind { int type; int parity; };      // type 0 sigma, 1 pi, 2 unsupported; parity +1 g, -1 u, 0 none
   auto kindOf = [&](const vector<double>& c) {
      Kind k; k.parity = 0;
      double par = 0.0, perp = 0.0, d = 0.0, tot = 0.0;
      vector<int> both(in.leftAtoms);
      both.insert(both.end(), in.rightAtoms.begin(), in.rightAtoms.end());
      axisWeights(set, c, both, axis, par, perp, d);
      tot = par + perp + d;
      k.type = (par >= perp && par >= d) ? 0 : (perp >= d ? 1 : 2);
      (void)tot;
      if (homo) {
         double p = 0.0;
         for (size_t i = 0; i < nf; i++)
            if (partner[i] >= 0) p += c[i]*c[partner[i]]*(set.funcs[i].l % 2 ? -1.0 : 1.0);
         k.parity = p >= 0.0 ? 1 : -1;
      }
      return k;
   };

   //  Names of the unmixed levels.
   struct Named { Kind kind; double energy; bool anti, none; string parL, parR, shell; double rank; };
   vector<Named> named(ucl.size());
   for (size_t c = 0; c < ucl.size(); c++) {
      const vector<double>& row = uRows[ucl[c].mos[0]];
      Named& n = named[c];
      n.kind = kindOf(row);
      n.energy = ucl[c].energy;
      double op = 0.0, lWeight = 0.0;
      for (size_t i = 0; i < nf; i++) {
         if (set.funcs[i].l == 0) lWeight += row[i]*row[i];
         for (size_t j = 0; j < nf; j++)
            if (set.funcs[i].atom == a0 && set.funcs[j].atom == a1) op += 2.0*row[i]*row[j]*un.overlap[i*nf+j];
      }
      n.none = fabs(op) < 1e-6;
      n.anti = op < 0.0;
      n.parL = leadingShell(set, row, in.leftAtoms);
      n.parR = leadingShell(set, row, in.rightAtoms);
      n.shell = n.parL.substr(n.parL.find(' ') + 1);
      {  //  shell order = the model's AO energies: mean H_ii of the functions of the level's l
         int lead = 0; double top = -1.0;
         for (size_t i = 0; i < nf; i++) if (row[i]*row[i] > top) { top = row[i]*row[i]; lead = set.funcs[i].l; }
         double sum = 0.0; int cnt = 0;
         for (size_t i = 0; i < nf; i++) if (set.funcs[i].l == lead) { sum += set.funcs[i].hii; cnt++; }
         n.rank = cnt ? sum/cnt : 0.0;
      }
   }

   //  Real levels, grouped by (type, parity); energy order against energy order.
   map<pair<int,int>, vector<size_t> > realBy, unBy;
   vector<Kind> realKind(cl.size());
   for (size_t c = 0; c < cl.size(); c++) {
      realKind[c] = kindOf(rows[cl[c].mos[0]]);
      realBy[make_pair(realKind[c].type, realKind[c].parity)].push_back(c);
   }
   for (size_t c = 0; c < named.size(); c++) unBy[make_pair(named[c].kind.type, named[c].kind.parity)].push_back(c);
   for (map<pair<int,int>, vector<size_t> >::iterator it = realBy.begin(); it != realBy.end(); ++it) {
      const vector<size_t>& u = unBy[it->first];
      if (u.size() != it->second.size()) continue;       // not a clean correlation: say nothing
      //  The textbook convention: a class's levels in energy order get the names
      //  in the fixed order 2s bonding, 2s anti, 2p bonding, 2p anti, as if the
      //  s shell lay well below the p shell.  No overlap matching.
      vector<size_t> u2(u);
      std::stable_sort(u2.begin(), u2.end(), [&](size_t x, size_t y) {
         if (fabs(named[x].rank - named[y].rank) > 1e-6) return named[x].rank < named[y].rank;
         return !named[x].anti && named[y].anti;
      });
      for (size_t k = 0; k < u.size(); k++) {
         const Named& n = named[u2[k]];
         ConstructLabel& L = out[it->second[k]];
         const char* kind = n.kind.type == 0 ? SIGMA : (n.kind.type == 1 ? PI : DELTA);
         L.nodal = n.kind.type;
         const size_t c = it->second[k];
         if (!homo && L.otherShare < NONBONDING_SHARE) {
            L.nonbonding = true;
            L.plain = "nb";
            L.full = "nb (" + (shL[c] >= shR[c] ? n.parL : n.parR) + ")";
            continue;
         }
         const string star = n.none ? "" : (n.anti ? "*" : "");
         L.antibonding = n.anti && !n.none;
         L.plain = string(kind) + star;
         L.full = homo ? string(kind) + star + n.shell
                       : string(kind) + star + "(" + n.parL + "/" + n.parR + ")";
      }
   }
   return true;
}


bool ModelDiagram::bondSlopes(const Input& in, vector<BondSlope>& out, string& why)
{
   out.clear();
   const size_t nAtoms = in.symbols.size();
   if (nAtoms < 2 || in.coordsAng.size() != 3*nAtoms || in.leftAtoms.empty() || in.rightAtoms.empty()) {
      why = "No structure.";
      return false;
   }
   if (!applicable(in.symbols, why)) return false;
   const bool central = in.leftAtoms.size() == 1 && in.rightAtoms.size() >= 2;
   const double step = 0.02;

   auto shifted = [&](double delta) {
      vector<double> x = in.coordsAng;
      if (central) {
         const double* c = &in.coordsAng[3*in.leftAtoms[0]];
         for (size_t a = 0; a < in.rightAtoms.size(); a++) {
            double u[3], n = 0.0;
            for (int q = 0; q < 3; q++) { u[q] = in.coordsAng[3*in.rightAtoms[a]+q] - c[q]; n += u[q]*u[q]; }
            n = sqrt(n);
            for (int q = 0; q < 3; q++) x[3*in.rightAtoms[a]+q] += delta*u[q]/(n > 0 ? n : 1.0);
         }
      } else {
         double cl[3] = {0, 0, 0}, cr[3] = {0, 0, 0}, u[3], n = 0.0;
         for (size_t a = 0; a < in.leftAtoms.size(); a++)
            for (int q = 0; q < 3; q++) cl[q] += in.coordsAng[3*in.leftAtoms[a]+q]/in.leftAtoms.size();
         for (size_t a = 0; a < in.rightAtoms.size(); a++)
            for (int q = 0; q < 3; q++) cr[q] += in.coordsAng[3*in.rightAtoms[a]+q]/in.rightAtoms.size();
         for (int q = 0; q < 3; q++) { u[q] = cr[q] - cl[q]; n += u[q]*u[q]; }
         n = sqrt(n);
         for (size_t a = 0; a < in.rightAtoms.size(); a++)
            for (int q = 0; q < 3; q++) x[3*in.rightAtoms[a]+q] += delta*u[q]/(n > 0 ? n : 1.0);
      }
      return x;
   };

   struct Run { FragmentOrbitals::FoSet set; vector<double> E; Mat rows; };
   auto solve = [&](const vector<double>& xyz, Run& r) {
      vector<FragmentOrbitals::Fragment> frags(2);
      frags[0].atoms = in.leftAtoms;  frags[0].charge = in.leftCharge;
      frags[1].atoms = in.rightAtoms; frags[1].charge = in.rightCharge;
      for (int f = 0; f < 2; f++) {
         const bool single = frags[f].atoms.size() == 1;
         frags[f].kind = single ? FragmentOrbitals::METAL : FragmentOrbitals::LIGAND;
         frags[f].name = single ? in.symbols[frags[f].atoms[0]] : string(f ? "R" : "L");
      }
      if (!FragmentOrbitals::build(in.symbols, xyz, frags, FragmentOrbitals::PER_LIGAND, r.set, why)) return false;
      if (!FragmentOrbitals::solveWhole(r.set, r.E, r.rows, why)) return false;
      normalise(r.set.overlap, r.set.funcs.size(), r.rows);
      return true;
   };

   Run base, plus, minus;
   if (!solve(in.coordsAng, base) || !solve(shifted(step), plus) || !solve(shifted(-step), minus)) return false;
   const size_t nf = base.set.funcs.size();
   vector<Level> cl = clusters(base.E);
   out.resize(cl.size());
   for (size_t c = 0; c < cl.size(); c++) out[c].orbitals = cl[c].mos;

   //  S times each base orbital, once.
   Mat SB(nf, vector<double>(nf, 0.0));
   for (size_t m = 0; m < nf; m++)
      for (size_t i = 0; i < nf; i++) {
         double sc = 0.0;
         for (size_t j = 0; j < nf; j++) sc += base.set.overlap[i*nf+j]*base.rows[m][j];
         SB[m][i] = sc;
      }
   //  Phases of the displaced orbitals are free, so only squared overlaps are used.
   for (int run = 0; run < 2; run++) {
      const Run& R = run ? minus : plus;
      vector<Level> rc = clusters(R.E);
      struct Cand { double w; size_t b, r; };
      vector<Cand> cands;
      for (size_t b = 0; b < cl.size(); b++)
         for (size_t r = 0; r < rc.size(); r++) {
            if (rc[r].mos.size() != cl[b].mos.size()) continue;
            double w = 0.0;
            for (size_t k = 0; k < cl[b].mos.size(); k++)
               for (size_t l = 0; l < rc[r].mos.size(); l++) {
                  double o = 0.0;
                  for (size_t i = 0; i < nf; i++) o += SB[cl[b].mos[k]][i]*R.rows[rc[r].mos[l]][i];
                  w += o*o;
               }
            Cand cd; cd.w = w/cl[b].mos.size(); cd.b = b; cd.r = r;
            cands.push_back(cd);
         }
      stable_sort(cands.begin(), cands.end(), [](const Cand& x, const Cand& y) { return x.w > y.w; });
      vector<bool> usedB(cl.size(), false), usedR(rc.size(), false);
      for (size_t i = 0; i < cands.size(); i++) {
         if (usedB[cands[i].b] || usedR[cands[i].r]) continue;
         usedB[cands[i].b] = usedR[cands[i].r] = true;
         //  slope accumulates as E(+) at run 0 and -E(-) at run 1
         out[cands[i].b].slope += (run ? -1.0 : 1.0)*rc[cands[i].r].energy*27.2114/(2.0*step);
         out[cands[i].b].ok = run ? out[cands[i].b].ok : true;
      }
      for (size_t b = 0; b < cl.size(); b++) if (!usedB[b]) out[b].ok = false;
   }
   return true;
}
