#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <sstream>
#include <utility>

#include "tdat/AtomMap.H"
#include "tdat/MoFragments.H"

using std::map;
using std::pair;
using std::set;

AtomAlignment::AtomAlignment() : rmsdBefore(0.0), rmsdAfter(0.0)
{
   for (int i = 0; i < 9; i++) R[i] = (i % 4 == 0) ? 1.0 : 0.0;
   t[0] = t[1] = t[2] = 0.0;
}

// ---- Hungarian algorithm (shortest augmenting paths, potentials) ----

vector<int> AtomMap::assign(const vector<double>& cost, int n)
{
   const double INF = 1e300;
   vector<double> u(n+1, 0.0), v(n+1, 0.0);
   vector<int> p(n+1, 0), way(n+1, 0);
   for (int i = 1; i <= n; i++) {
      p[0] = i;
      int j0 = 0;
      vector<double> minv(n+1, INF);
      vector<char> used(n+1, 0);
      do {
         used[j0] = 1;
         const int i0 = p[j0];
         double delta = INF;
         int j1 = 0;
         for (int j = 1; j <= n; j++) {
            if (used[j]) continue;
            const double cur = cost[(i0-1)*n + (j-1)] - u[i0] - v[j];
            if (cur < minv[j]) { minv[j] = cur; way[j] = j0; }
            if (minv[j] < delta) { delta = minv[j]; j1 = j; }
         }
         for (int j = 0; j <= n; j++) {
            if (used[j]) { u[p[j]] += delta; v[j] -= delta; }
            else minv[j] -= delta;
         }
         j0 = j1;
      } while (p[j0] != 0);
      do {
         const int j1 = way[j0];
         p[j0] = p[j1];
         j0 = j1;
      } while (j0);
   }
   vector<int> result(n, 0);
   for (int j = 1; j <= n; j++) if (p[j]) result[p[j]-1] = j-1;
   return result;
}

// ---- symmetric eigenproblems (cyclic Jacobi), n = 3 or 4 ----

// Eigenvalues end up on the diagonal of a, eigenvectors in the columns of v.
static void jacobi(double* a, double* v, int n)
{
   for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++) v[i*n+j] = (i == j) ? 1.0 : 0.0;
   for (int sweep = 0; sweep < 60; sweep++) {
      double off = 0.0;
      for (int i = 0; i < n; i++)
         for (int j = i+1; j < n; j++) off += a[i*n+j]*a[i*n+j];
      if (off < 1e-30) break;
      for (int p = 0; p < n-1; p++) {
         for (int r = p+1; r < n; r++) {
            if (fabs(a[p*n+r]) < 1e-300) continue;
            const double th = (a[r*n+r] - a[p*n+p]) / (2.0*a[p*n+r]);
            const double t = (th >= 0 ? 1.0 : -1.0) / (fabs(th) + sqrt(th*th + 1.0));
            const double c = 1.0/sqrt(t*t + 1.0), s = t*c;
            for (int k = 0; k < n; k++) {
               const double x = a[k*n+p], y = a[k*n+r];
               a[k*n+p] = c*x - s*y;
               a[k*n+r] = s*x + c*y;
            }
            for (int k = 0; k < n; k++) {
               const double x = a[p*n+k], y = a[r*n+k];
               a[p*n+k] = c*x - s*y;
               a[r*n+k] = s*x + c*y;
            }
            for (int k = 0; k < n; k++) {
               const double x = v[k*n+p], y = v[k*n+r];
               v[k*n+p] = c*x - s*y;
               v[k*n+r] = s*x + c*y;
            }
         }
      }
   }
}

static double rmsdOf(const vector<double>& ref, const vector<double>& mov,
                     const vector<int>& map)
{
   double s = 0.0;
   for (size_t i = 0; i < map.size(); i++)
      for (int k = 0; k < 3; k++) {
         const double d = mov[3*i+k] - ref[3*map[i]+k];
         s += d*d;
      }
   return map.empty() ? 0.0 : sqrt(s/map.size());
}

AtomAlignment AtomMap::align(const vector<double>& ref,
                             const vector<double>& mov,
                             const vector<int>& map)
{
   AtomAlignment al;
   const size_t n = map.size();
   if (n == 0) return al;
   al.rmsdBefore = rmsdOf(ref, mov, map);

   double cm[3] = {0,0,0}, cr[3] = {0,0,0};
   for (size_t i = 0; i < n; i++)
      for (int k = 0; k < 3; k++) {
         cm[k] += mov[3*i+k];
         cr[k] += ref[3*map[i]+k];
      }
   for (int k = 0; k < 3; k++) { cm[k] /= n; cr[k] /= n; }

   double S[3][3] = {{0,0,0},{0,0,0},{0,0,0}};   // S[a][b] = sum m_a r_b
   for (size_t i = 0; i < n; i++)
      for (int a = 0; a < 3; a++)
         for (int b = 0; b < 3; b++)
            S[a][b] += (mov[3*i+a]-cm[a]) * (ref[3*map[i]+b]-cr[b]);

   double N[16] = {
      S[0][0]+S[1][1]+S[2][2], S[1][2]-S[2][1], S[2][0]-S[0][2], S[0][1]-S[1][0],
      S[1][2]-S[2][1], S[0][0]-S[1][1]-S[2][2], S[0][1]+S[1][0], S[2][0]+S[0][2],
      S[2][0]-S[0][2], S[0][1]+S[1][0], -S[0][0]+S[1][1]-S[2][2], S[1][2]+S[2][1],
      S[0][1]-S[1][0], S[2][0]+S[0][2], S[1][2]+S[2][1], -S[0][0]-S[1][1]+S[2][2] };
   double V[16];
   jacobi(N, V, 4);
   int best = 0;
   for (int i = 1; i < 4; i++) if (N[i*4+i] > N[best*4+best]) best = i;
   double q[4];
   double nq = 0.0;
   for (int i = 0; i < 4; i++) { q[i] = V[i*4+best]; nq += q[i]*q[i]; }
   nq = sqrt(nq);
   for (int i = 0; i < 4; i++) q[i] /= nq;
   const double w = q[0], x = q[1], y = q[2], z = q[3];
   const double R[9] = {
      w*w+x*x-y*y-z*z, 2*(x*y-w*z),     2*(x*z+w*y),
      2*(x*y+w*z),     w*w-x*x+y*y-z*z, 2*(y*z-w*x),
      2*(x*z-w*y),     2*(y*z+w*x),     w*w-x*x-y*y+z*z };
   for (int i = 0; i < 9; i++) al.R[i] = R[i];
   for (int a = 0; a < 3; a++)
      al.t[a] = cr[a] - (R[3*a]*cm[0] + R[3*a+1]*cm[1] + R[3*a+2]*cm[2]);

   vector<double> moved(mov);
   apply(al, moved);
   al.rmsdAfter = rmsdOf(ref, moved, map);
   return al;
}

void AtomMap::apply(const AtomAlignment& a, vector<double>& c)
{
   for (size_t i = 0; i + 2 < c.size(); i += 3) {
      const double x = c[i], y = c[i+1], z = c[i+2];
      for (int r = 0; r < 3; r++)
         c[i+r] = a.R[3*r]*x + a.R[3*r+1]*y + a.R[3*r+2]*z + a.t[r];
   }
}

vector<double> AtomMap::renumber(const vector<double>& c, const vector<int>& m)
{
   vector<double> out(c.size());
   for (size_t i = 0; i < m.size(); i++)
      for (int k = 0; k < 3; k++) out[3*m[i]+k] = c[3*i+k];
   return out;
}

vector<string> AtomMap::renumber(const vector<string>& e, const vector<int>& m)
{
   vector<string> out(e.size());
   for (size_t i = 0; i < m.size(); i++) out[m[i]] = e[i];
   return out;
}

// ---- mapping ----

static double dist2(const vector<double>& a, int i, const vector<double>& b, int j)
{
   double s = 0.0;
   for (int k = 0; k < 3; k++) {
      const double d = a[3*i+k] - b[3*j+k];
      s += d*d;
   }
   return s;
}

namespace {

struct Problem {
   const vector<string>* refEl;
   const vector<string>* movEl;
   const vector<double>* ref;
   const vector<double>* mov;
   vector< vector<int> > refNb, movNb;
   vector<string> refSig, movSig;
   map<string, vector<int> > refBy, movBy;   // element -> atom indices
   // Weights (A^2) of the bonded-neighbour tie-breakers: small against a real
   // displacement, large against numerical noise between equivalent atoms.
   double wSig, wNb;
};

void neighbours(const vector<string>& el, const vector<double>& c,
                vector< vector<int> >& nb, vector<string>& sig)
{
   vector< pair<int,int> > bonds;
   MoFragments::covalentBonds(c, el, bonds);
   nb.assign(el.size(), vector<int>());
   for (size_t b = 0; b < bonds.size(); b++) {
      nb[bonds[b].first].push_back(bonds[b].second);
      nb[bonds[b].second].push_back(bonds[b].first);
   }
   sig.assign(el.size(), string());
   for (size_t i = 0; i < el.size(); i++) {
      vector<string> s;
      for (size_t k = 0; k < nb[i].size(); k++) s.push_back(el[nb[i][k]]);
      std::sort(s.begin(), s.end());
      for (size_t k = 0; k < s.size(); k++) sig[i] += s[k] + ",";
   }
}

// One assignment, per element, with the movers already placed in the
// reference frame.  `prev` (may be empty) is the mapping the neighbour
// consistency term is judged against.
vector<int> assignStep(const Problem& P, const vector<double>& placed,
                       const vector<int>& prev)
{
   vector<int> out(P.movEl->size(), 0);
   for (map<string, vector<int> >::const_iterator e = P.movBy.begin();
        e != P.movBy.end(); ++e) {
      const vector<int>& mi = e->second;
      const vector<int>& ri = P.refBy.find(e->first)->second;
      const int m = (int)mi.size();
      vector<double> cost((size_t)m*m);
      for (int a = 0; a < m; a++) {
         for (int b = 0; b < m; b++) {
            double c = dist2(placed, mi[a], *P.ref, ri[b]);
            if (P.movSig[mi[a]] != P.refSig[ri[b]]) c += P.wSig;
            if (!prev.empty()) {
               set<int> rn(P.refNb[ri[b]].begin(), P.refNb[ri[b]].end());
               for (size_t k = 0; k < P.movNb[mi[a]].size(); k++)
                  if (!rn.count(prev[P.movNb[mi[a]][k]])) c += P.wNb;
            }
            cost[(size_t)a*m + b] = c;
         }
      }
      const vector<int> sol = AtomMap::assign(cost, m);
      for (int a = 0; a < m; a++) out[mi[a]] = ri[sol[a]];
   }
   return out;
}

double sumSq(const vector<double>& ref, const vector<double>& mov,
             const vector<int>& map)
{
   const double r = rmsdOf(ref, mov, map);
   return r*r*map.size();
}

// Alternate superposition and assignment until the assignment is stable.
vector<int> refine(const Problem& P, vector<int> m)
{
   for (int it = 0; it < 100; it++) {
      const AtomAlignment al = AtomMap::align(*P.ref, *P.mov, m);
      vector<double> placed(*P.mov);
      AtomMap::apply(al, placed);
      const vector<int> next = assignStep(P, placed, m);
      if (next == m) break;
      m = next;
   }
   return m;
}

vector<int> orderedByElement(const Problem& P)
{
   vector<int> m(P.movEl->size(), 0);
   for (map<string, vector<int> >::const_iterator e = P.movBy.begin();
        e != P.movBy.end(); ++e) {
      const vector<int>& ri = P.refBy.find(e->first)->second;
      for (size_t k = 0; k < e->second.size(); k++) m[e->second[k]] = ri[k];
   }
   return m;
}

// Start that needs no superposition: match each atom's sorted distances to
// the atoms of every element, a fingerprint of where it sits in the molecule.
vector<int> fingerprintStart(const Problem& P)
{
   const size_t n = P.movEl->size();
   vector<string> elems;
   for (map<string, vector<int> >::const_iterator e = P.movBy.begin();
        e != P.movBy.end(); ++e) elems.push_back(e->first);
   const size_t ne = elems.size();
   vector< vector<double> > fp[2];
   for (int s = 0; s < 2; s++) {
      const vector<double>& c = s ? *P.ref : *P.mov;
      const vector<string>& el = s ? *P.refEl : *P.movEl;
      fp[s].resize(n);
      for (size_t i = 0; i < n; i++) {
         vector< vector<double> > d(ne);
         for (size_t j = 0; j < n; j++) {
            if (j == i) continue;
            const size_t k = std::find(elems.begin(), elems.end(), el[j]) -
                             elems.begin();
            d[k].push_back(sqrt(dist2(c, (int)i, c, (int)j)));
         }
         for (size_t k = 0; k < ne; k++) {
            std::sort(d[k].begin(), d[k].end());
            fp[s][i].insert(fp[s][i].end(), d[k].begin(), d[k].end());
         }
      }
   }
   vector<int> out(n, 0);
   for (map<string, vector<int> >::const_iterator e = P.movBy.begin();
        e != P.movBy.end(); ++e) {
      const vector<int>& mi = e->second;
      const vector<int>& ri = P.refBy.find(e->first)->second;
      const int m = (int)mi.size();
      vector<double> cost((size_t)m*m);
      for (int a = 0; a < m; a++)
         for (int b = 0; b < m; b++) {
            double s = 0.0;
            for (size_t k = 0; k < fp[0][mi[a]].size(); k++) {
               const double d = fp[0][mi[a]][k] - fp[1][ri[b]][k];
               s += d*d;
            }
            cost[(size_t)a*m+b] = s;
         }
      const vector<int> sol = AtomMap::assign(cost, m);
      for (int a = 0; a < m; a++) out[mi[a]] = ri[sol[a]];
   }
   return out;
}

// Principal axes of a structure: centre and right-handed axes (rows).
void principalFrame(const vector<double>& c, double ctr[3], double ax[3][3])
{
   const size_t n = c.size()/3;
   ctr[0] = ctr[1] = ctr[2] = 0;
   for (size_t i = 0; i < n; i++)
      for (int k = 0; k < 3; k++) ctr[k] += c[3*i+k]/n;
   double T[9] = {0}, V[9];
   for (size_t i = 0; i < n; i++)
      for (int a = 0; a < 3; a++)
         for (int b = 0; b < 3; b++)
            T[3*a+b] += (c[3*i+a]-ctr[a])*(c[3*i+b]-ctr[b]);
   jacobi(T, V, 3);
   int ord[3] = {0,1,2};
   std::sort(ord, ord+3, [&](int a, int b){ return T[4*a] > T[4*b]; });
   for (int k = 0; k < 3; k++)
      for (int a = 0; a < 3; a++) ax[k][a] = V[3*a+ord[k]];
   const double det = ax[0][0]*(ax[1][1]*ax[2][2]-ax[1][2]*ax[2][1])
                    - ax[0][1]*(ax[1][0]*ax[2][2]-ax[1][2]*ax[2][0])
                    + ax[0][2]*(ax[1][0]*ax[2][1]-ax[1][1]*ax[2][0]);
   if (det < 0) for (int a = 0; a < 3; a++) ax[2][a] = -ax[2][a];
}

// Starts from the four proper axis-sign choices, then an assignment.
void principalStarts(const Problem& P, vector< vector<int> >& starts)
{
   const size_t n = P.movEl->size();
   if (n < 3) return;
   double cm[3], cr[3], Am[3][3], Ar[3][3];
   principalFrame(*P.mov, cm, Am);
   principalFrame(*P.ref, cr, Ar);
   const double sg[4][3] = {{1,1,1},{1,-1,-1},{-1,1,-1},{-1,-1,1}};
   for (int s = 0; s < 4; s++) {
      vector<double> placed(P.mov->size());
      for (size_t i = 0; i < n; i++) {
         double x[3];
         for (int k = 0; k < 3; k++) {
            x[k] = 0;
            for (int a = 0; a < 3; a++) x[k] += Am[k][a]*((*P.mov)[3*i+a]-cm[a]);
            x[k] *= sg[s][k];
         }
         for (int a = 0; a < 3; a++) {
            double y = cr[a];
            for (int k = 0; k < 3; k++) y += Ar[k][a]*x[k];
            placed[3*i+a] = y;
         }
      }
      starts.push_back(assignStep(P, placed, vector<int>()));
   }
}

} // namespace

AtomMapResult AtomMap::map(const vector<string>& refEl, const vector<double>& ref,
                           const vector<string>& movEl, const vector<double>& mov,
                           const AtomMapOptions& opt)
{
   AtomMapResult res;
   if (refEl.size() != movEl.size()) {
      std::ostringstream s;
      s << "The structures have different numbers of atoms ("
        << refEl.size() << " and " << movEl.size() << ").";
      res.error = s.str();
      return res;
   }
   if (ref.size() != 3*refEl.size() || mov.size() != 3*movEl.size()) {
      res.error = "Coordinates do not match the element lists.";
      return res;
   }
   std::map<string,int> cr, cm;
   for (size_t i = 0; i < refEl.size(); i++) { cr[refEl[i]]++; cm[movEl[i]]++; }
   if (cr != cm) {
      string diff;
      set<string> all;
      for (std::map<string,int>::iterator i = cr.begin(); i != cr.end(); ++i) all.insert(i->first);
      for (std::map<string,int>::iterator i = cm.begin(); i != cm.end(); ++i) all.insert(i->first);
      for (set<string>::iterator e = all.begin(); e != all.end(); ++e) {
         if (cr[*e] == cm[*e]) continue;
         std::ostringstream s;
         s << (diff.empty() ? "" : ", ") << *e << " " << cr[*e] << " vs " << cm[*e];
         diff += s.str();
      }
      res.error = "The structures have different elements (" + diff +
                  ", reference vs other).";
      return res;
   }

   const size_t n = refEl.size();
   Problem P;
   P.refEl = &refEl; P.movEl = &movEl; P.ref = &ref; P.mov = &mov;
   P.wSig = 0.1; P.wNb = 0.1;
   neighbours(refEl, ref, P.refNb, P.refSig);
   neighbours(movEl, mov, P.movNb, P.movSig);
   for (size_t i = 0; i < n; i++) {
      P.refBy[refEl[i]].push_back((int)i);
      P.movBy[movEl[i]].push_back((int)i);
   }

   vector<int> ident(n);
   for (size_t i = 0; i < n; i++) ident[i] = (int)i;

   vector<int> best;
   const bool ordered = (refEl == movEl);
   double identCost = 1e300;
   if (ordered) {
      const AtomAlignment al = align(ref, mov, ident);
      vector<double> placed(mov);
      apply(al, placed);
      double mx = 0.0;
      for (size_t i = 0; i < n; i++)
         mx = std::max(mx, sqrt(dist2(placed, (int)i, ref, (int)i)));
      identCost = sumSq(ref, placed, ident);
      if (mx <= opt.identityMaxDisp) best = ident;
   }
   if (best.empty()) {
      vector< vector<int> > starts;
      if (ordered) starts.push_back(ident);
      starts.push_back(orderedByElement(P));
      starts.push_back(fingerprintStart(P));
      principalStarts(P, starts);
      double bestCost = 1e300;
      for (size_t s = 0; s < starts.size(); s++) {
         const vector<int> m = refine(P, starts[s]);
         const AtomAlignment al = align(ref, mov, m);
         vector<double> placed(mov);
         apply(al, placed);
         // Starts are ranked by squared displacement alone, so the
         // tie-breakers never outvote geometry.
         const double c = sumSq(ref, placed, m);
         if (c < bestCost - 1e-9) { bestCost = c; best = m; }
      }
      // The given order stays unless a permutation is clearly better.
      if (ordered && identCost <= 1.1*bestCost) best = ident;
   }

   res.map = best;
   res.identity = (best == ident);
   for (size_t i = 0; i < n; i++) if (best[i] != (int)i) res.nChanged++;
   res.alignment = align(ref, mov, best);
   vector<double> placed(mov);
   apply(res.alignment, placed);
   res.displacement.resize(n);
   for (size_t i = 0; i < n; i++) {
      res.displacement[i] = sqrt(dist2(placed, (int)i, ref, best[i]));
      if (res.displacement[i] > opt.farThreshold) res.far.push_back((int)i);
   }
   res.ok = true;
   return res;
}
