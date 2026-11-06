// 3-D builder for Smiles::Graph (#867).  The chain is placed atom by atom
// from ideal lengths, angles and torsions, rings from polygon and chair
// torsions, stereo by choosing which slot each neighbour takes; a
// distance-based relaxation then closes the rings.  Several attempts with
// different torsion choices are made until the stereo checks pass.
#include "tdat/Smiles.H"

#include <algorithm>
#include <cmath>
#include <map>
#include <queue>
#include <random>
#include <set>

using namespace std;

namespace Smiles {

namespace {

typedef array<double,3> V3;
const double PI = 3.14159265358979323846;
V3 sub(const V3& a, const V3& b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
V3 add(const V3& a, const V3& b) { return {a[0]+b[0], a[1]+b[1], a[2]+b[2]}; }
V3 mul(const V3& a, double s) { return {a[0]*s, a[1]*s, a[2]*s}; }
double dot(const V3& a, const V3& b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
V3 cross(const V3& a, const V3& b)
{ return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]}; }
double norm(const V3& a) { return sqrt(dot(a, a)); }
V3 unit(const V3& a) { double n = norm(a); return n > 1e-12 ? mul(a, 1/n) : a; }
double rad(double d) { return d * PI / 180; }

double torsionOf(const V3& a, const V3& b, const V3& c, const V3& d)
{
   V3 b1 = sub(b, a), b2 = sub(c, b), b3 = sub(d, c);
   V3 n1 = cross(b1, b2), n2 = cross(b2, b3);
   return atan2(dot(cross(n1, n2), unit(b2)), dot(n1, n2));
}

double covRadius(const string& el)
{
   static const map<string, double> r = {
      {"H",.31},{"He",.28},{"Li",1.28},{"Be",.96},{"B",.84},{"C",.76},{"N",.71},
      {"O",.66},{"F",.57},{"Ne",.58},{"Na",1.66},{"Mg",1.41},{"Al",1.21},
      {"Si",1.11},{"P",1.07},{"S",1.05},{"Cl",1.02},{"Ar",1.06},{"K",2.03},
      {"Ca",1.76},{"Sc",1.70},{"Ti",1.60},{"V",1.53},{"Cr",1.39},{"Mn",1.39},
      {"Fe",1.32},{"Co",1.26},{"Ni",1.24},{"Cu",1.32},{"Zn",1.22},{"Ga",1.22},
      {"Ge",1.20},{"As",1.19},{"Se",1.20},{"Br",1.20},{"Kr",1.16},{"Rb",2.20},
      {"Sr",1.95},{"Y",1.90},{"Zr",1.75},{"Nb",1.64},{"Mo",1.54},{"Tc",1.47},
      {"Ru",1.46},{"Rh",1.42},{"Pd",1.39},{"Ag",1.45},{"Cd",1.44},{"In",1.42},
      {"Sn",1.39},{"Sb",1.39},{"Te",1.38},{"I",1.39},{"Xe",1.40},{"Cs",2.44},
      {"Ba",2.15},{"La",2.07},{"Hf",1.75},{"Ta",1.70},{"W",1.62},{"Re",1.51},
      {"Os",1.44},{"Ir",1.41},{"Pt",1.36},{"Au",1.36},{"Hg",1.32},{"Tl",1.45},
      {"Pb",1.46},{"Bi",1.48}};
   auto it = r.find(el);
   return it == r.end() ? 1.5 : it->second;
}

double vdwRadius(const string& el)
{
   if (el == "H") return 1.1;
   if (covRadius(el) > 1.3) return 2.0;
   return 1.7;
}

// Slot geometry of a centre: polar angle from the direction away from the
// parent, and azimuth about it, in degrees.
typedef vector<pair<double,double> > Slots;

struct Builder {
   const Graph& g;
   int nh = 0;                       // heavy atoms
   int n = 0;                        // all atoms
   vector<string> el;
   vector<int> charge, hyb;
   vector<vector<int> > adj;         // all neighbours
   struct BondRec { int a, b, ord; bool arom, closure; char dir; int from; };
   vector<BondRec> bonds;
   vector<vector<int> > bondOf;      // per atom: bond index per adj entry
   vector<int> parent, depth;
   vector<vector<int> > rings;
   vector<int> ringOfBond;
   vector<char> ringPlanar;
   vector<vector<int> > chirOrder;   // per atom, 4 neighbours or empty
   vector<int> chirSign;
   vector<V3> pos;
   vector<char> placed;
   mt19937 rng;
   bool flip = false;
   double jitter = 0;

   Builder(const Graph& gr) : g(gr), rng(12345) {}

   int bondIndex(int a, int b) const
   {
      for (size_t k = 0; k < adj[a].size(); k++) if (adj[a][k] == b) return bondOf[a][k];
      return -1;
   }
   double ideal(int a, int b) const
   {
      int k = bondIndex(a, b);
      return idealBond(el[a], el[b], bonds[k].arom ? 1.5 : bonds[k].ord);
   }

   void setup();
   void findRings();
   double ringAngle(int ring, int atom) const;
   double idealAngle(int c, int i, int j) const;
   Slots slotsFor(int c) const;
   bool sideOf(int A, int B, map<int,int>& sides) const;
   void placeCenter(int c);
   bool buildOnce();
   void relax();
   int check(bool verbose);
   V3 nerf(const V3& A, const V3& B, const V3& C, double len, double ang, double tor) const;
};

V3 Builder::nerf(const V3& A, const V3& B, const V3& C, double len, double ang, double tor) const
{
   V3 bc = unit(sub(C, B));
   V3 nn = cross(sub(B, A), bc);
   if (norm(nn) < 1e-6) {                          // collinear reference
      V3 t = fabs(bc[0]) < 0.9 ? V3{1, 0, 0} : V3{0, 1, 0};
      nn = cross(t, bc);
   }
   nn = unit(nn);
   V3 m = cross(nn, bc);
   double x = -len * cos(ang), y = len * sin(ang) * cos(tor), z = len * sin(ang) * sin(tor);
   return add(C, add(mul(bc, x), add(mul(m, y), mul(nn, z))));
}

void Builder::setup()
{
   nh = (int)g.atoms.size();
   for (int i = 0; i < nh; i++) {
      el.push_back(g.atoms[i].el);
      charge.push_back(g.atoms[i].charge);
   }
   for (const Bond& b : g.bonds)
      bonds.push_back({b.a, b.b, b.ord, b.arom, b.closure, b.dir, b.dirFrom});
   int hbase = nh;
   vector<vector<int> > hOf(nh);
   for (int i = 0; i < nh; i++)
      for (int h = 0; h < g.atoms[i].hcount; h++) {
         el.push_back("H"); charge.push_back(0);
         hOf[i].push_back(hbase++);
         bonds.push_back({i, hbase - 1, 1, false, false, 0, -1});
      }
   n = hbase;
   adj.assign(n, {}); bondOf.assign(n, {});
   for (size_t k = 0; k < bonds.size(); k++) {
      adj[bonds[k].a].push_back(bonds[k].b); bondOf[bonds[k].a].push_back((int)k);
      adj[bonds[k].b].push_back(bonds[k].a); bondOf[bonds[k].b].push_back((int)k);
   }
   // Hybridisation.
   hyb.assign(n, 3);
   for (int i = 0; i < nh; i++) {
      int dbl = 0, trp = 0; bool arom = false;
      for (int k : bondOf[i]) {
         if (bonds[k].ord == 2) dbl++;
         if (bonds[k].ord == 3) trp++;
         if (bonds[k].arom) arom = true;
      }
      if (trp || (dbl >= 2 && (el[i] == "C" || el[i] == "N"))) hyb[i] = 1;
      else if (dbl || arom) hyb[i] = 2;
   }
   for (int i = 0; i < nh; i++) {                   // amide-type N
      if (el[i] != "N" || hyb[i] != 3 || adj[i].size() != 3) continue;
      for (int j : adj[i]) {
         if (j >= nh || el[j] != "C") continue;
         for (int k : bondOf[j])
            if (bonds[k].ord == 2) {
               int o = bonds[k].a == j ? bonds[k].b : bonds[k].a;
               if (o < nh && (el[o] == "O" || el[o] == "S")) hyb[i] = 2;
            }
      }
   }
   // Build tree: breadth-first over non-closure bonds, component by component.
   parent.assign(n, -2); depth.assign(n, 0);
   // Chirality neighbour order, with the implicit hydrogen after the
   // preceding atom.
   chirOrder.assign(n, {}); chirSign.assign(n, 0);
   for (int i = 0; i < nh; i++) {
      if (!g.atoms[i].chir) continue;
      vector<int> o = g.nbr[i];
      if (g.atoms[i].hcount == 1) o.insert(o.begin() + (g.prior[i] >= 0 ? 1 : 0), hOf[i][0]);
      if (o.size() == 4) { chirOrder[i] = o; chirSign[i] = g.atoms[i].chir == 1 ? -1 : 1; }
   }
   findRings();
}

void Builder::findRings()
{
   ringOfBond.assign(bonds.size(), -1);
   set<vector<int> > seen;
   for (size_t k = 0; k < bonds.size(); k++) {
      int a = bonds[k].a, b = bonds[k].b;
      vector<int> prevAt(n, -1);
      vector<char> vis(n, 0);
      queue<int> q; q.push(a); vis[a] = 1;
      while (!q.empty() && !vis[b]) {
         int x = q.front(); q.pop();
         for (size_t t = 0; t < adj[x].size(); t++) {
            if ((int)k == bondOf[x][t]) continue;
            int y = adj[x][t];
            if (!vis[y]) { vis[y] = 1; prevAt[y] = x; q.push(y); }
         }
      }
      if (!vis[b]) continue;
      vector<int> ring;
      for (int x = b; x >= 0; x = prevAt[x]) ring.push_back(x);
      vector<int> key = ring;
      sort(key.begin(), key.end());
      int idx;
      auto it = find(rings.begin(), rings.end(), key);
      if (it == rings.end()) {
         // Keep rings in path order for nothing: membership is all we need.
         rings.push_back(key); idx = (int)rings.size() - 1;
      } else idx = (int)(it - rings.begin());
      ringOfBond[k] = idx;
   }
   ringPlanar.assign(rings.size(), 0);
   for (size_t r = 0; r < rings.size(); r++) {
      bool sat = true;
      for (int x : rings[r]) if (hyb[x] != 3) sat = false;
      // Saturated six- and seven-rings pucker; everything else starts flat.
      ringPlanar[r] = !(sat && (rings[r].size() == 6 || rings[r].size() == 7));
   }
}

double Builder::ringAngle(int ring, int) const
{
   size_t s = rings[ring].size();
   if (!ringPlanar[ring]) return s == 6 ? 111.0 : 113.0;
   return 180.0 * (s - 2) / s;
}

double Builder::idealAngle(int c, int i, int j) const
{
   int ki = bondIndex(c, i), kj = bondIndex(c, j);
   int ri = ringOfBond[ki], rj = ringOfBond[kj];
   if (ri >= 0 && ri == rj) return ringAngle(ri, c);
   if (hyb[c] == 1) return 180;
   size_t deg = adj[c].size();
   if (hyb[c] == 2 && deg == 3) {
      double sumRing = 0; int nRing = 0, nOther = 0;
      for (size_t a = 0; a < deg; a++)
         for (size_t b = a + 1; b < deg; b++) {
            int ra = ringOfBond[bondOf[c][a]], rb = ringOfBond[bondOf[c][b]];
            if (ra >= 0 && ra == rb) { sumRing += ringAngle(ra, c); nRing++; }
            else nOther++;
         }
      return nOther ? (360 - sumRing) / nOther : 120;
   }
   if (hyb[c] == 2) return 120;
   // Exocyclic angles on a small ring atom open up.
   for (int x : adj[c]) {
      int r = ringOfBond[bondIndex(c, x)];
      if (r >= 0 && rings[r].size() == 3) return 117;
      if (r >= 0 && rings[r].size() == 4) return 112;
   }
   return 109.47;
}

Slots Builder::slotsFor(int c) const
{
   size_t deg = adj[c].size();
   if (deg == 2) {
      if (hyb[c] == 1) return {{0, 0}};
      return {{hyb[c] == 2 ? 60.0 : 70.53, 0}};
   }
   if (deg == 3) {
      if (hyb[c] == 2) return {{60, 0}, {60, 180}};
      return {{70.53, 0}, {70.53, 120}};
   }
   if (deg == 4) return {{70.53, 0}, {70.53, 120}, {70.53, 240}};
   if (deg == 5) return {{0, 0}, {90, 0}, {90, 120}, {90, 240}};
   if (deg == 6) return {{0, 0}, {90, 0}, {90, 90}, {90, 180}, {90, 270}};
   return {};
}

// Sides of the neighbours of A relative to the double bond A=B from the
// '/' and '\' marks: +1 / -1; false when A has none marked.
bool Builder::sideOf(int A, int B, map<int,int>& sides) const
{
   sides.clear();
   for (size_t t = 0; t < adj[A].size(); t++) {
      int X = adj[A][t];
      const BondRec& b = bonds[bondOf[A][t]];
      if (X == B || !b.dir) continue;
      int s = b.dir == '/' ? 1 : -1;
      sides[X] = (X == b.from) ? -s : s;
   }
   if (sides.empty()) return false;
   int first = sides.begin()->second;
   for (int X : adj[A])
      if (X != B && !sides.count(X)) sides[X] = -first;
   return true;
}

void Builder::placeCenter(int c)
{
   const size_t deg = adj[c].size();
   if (deg < 2) return;
   int p = parent[c];
   if (p < 0) for (int x : adj[c]) if (placed[x]) { p = x; break; }
   if (p < 0) return;
   vector<int> kids;                         // to be placed or virtual
   vector<int> fixedOther;
   for (int x : adj[c]) {
      if (x == p) continue;
      if (placed[x]) fixedOther.push_back(x); else kids.push_back(x);
   }
   if (kids.empty()) return;
   Slots slots = slotsFor(c);
   if (slots.empty()) return;
   // Torsion reference: a placed neighbour of p other than c; for a
   // stereo double bond, one whose side is known.
   map<int,int> sidesA, sidesB;
   int bpc = bondIndex(p, c);
   bool stereo = false;
   if (bonds[bpc].ord == 2 && hyb[c] == 2 && hyb[p] == 2 && c < nh && p < nh)
      stereo = sideOf(p, c, sidesA) && sideOf(c, p, sidesB);
   int aref = -1;
   for (int x : adj[p]) if (x != c && placed[x] && (!stereo || sidesA.count(x))) { aref = x; if (!stereo) break; }
   if (aref < 0) for (int x : adj[p]) if (x != c && placed[x]) { aref = x; break; }
   V3 A;
   if (aref >= 0) A = pos[aref];
   else {
      V3 ax = unit(sub(pos[c], pos[p]));
      V3 t = fabs(ax[1]) < 0.9 ? V3{0, 1, 0} : V3{0, 0, 1};
      A = add(pos[p], unit(cross(ax, t)));
   }
   // Ring child and the torsion the ring wants about p-c.
   int ringKid = -1; double tRing = 0, angRing = 0;
   for (int d : kids) {
      if (d >= nh || aref < 0 || aref >= nh || c >= nh || p >= nh) continue;
      int r = ringOfBond[bondIndex(c, d)];
      if (r < 0 || placed[d]) continue;
      if (ringOfBond[bpc] != r) continue;
      const vector<int>& rg = rings[r];
      if (!binary_search(rg.begin(), rg.end(), aref)) continue;
      if (find(g.nbr[c].begin(), g.nbr[c].end(), d) == g.nbr[c].end()) continue;
      ringKid = d;
      double T = ringPlanar[r] ? 0 : (rg.size() == 6 ? 55 : 65);
      tRing = T * ((depth[c] % 2 == 0) ? 1 : -1) * (flip ? -1 : 1);
      angRing = ringAngle(r, c);
      break;
   }
   // Torsion offset of the first slot.
   double t0 = 180 + jitter;
   // Ring children not at the ring's place are virtual closures only.
   bool realKid = false;
   for (int d : kids) if (parent[d] == c) realKid = true;
   (void)realKid;
   if (ringKid >= 0) t0 = tRing;
   // Slots taken by already-placed neighbours.
   vector<int> freeSlot;
   {
      vector<char> taken(slots.size(), 0);
      for (int x : fixedOther) {
         V3 dx = unit(sub(pos[x], pos[c]));
         double best = 1e9; int bi = -1;
         for (size_t s = 0; s < slots.size(); s++) {
            V3 q = nerf(A, pos[p], pos[c], 1.0, rad(180 - slots[s].first), rad(t0 + slots[s].second));
            double d = norm(sub(dx, sub(q, pos[c])));
            if (!taken[s] && d < best) { best = d; bi = (int)s; }
         }
         if (bi >= 0) taken[bi] = 1;
      }
      for (size_t s = 0; s < slots.size(); s++) if (!taken[s]) freeSlot.push_back((int)s);
   }
   if (freeSlot.size() < kids.size()) {
      // More neighbours than the geometry has room for (hypervalent centre
      // beyond the table): take the slots there are.
      kids.resize(freeSlot.size());
      if (kids.empty()) return;
   }
   // Slot direction vectors.
   auto slotDir = [&](int s, double t) {
      V3 q = nerf(A, pos[p], pos[c], 1.0, rad(180 - slots[s].first), rad(t + slots[s].second));
      return unit(sub(q, pos[c]));
   };
   // Candidate assignments: kids -> distinct free slots.
   vector<int> order(kids.size());
   for (size_t i = 0; i < order.size(); i++) order[i] = (int)i;
   vector<int> choice;                       // slot per kid
   // Stereo double bond: the kid with a known side sets the torsion.
   if (stereo && kids.size() >= 1) {
      int sA = sidesA.count(aref) ? sidesA[aref] : 1;
      int known = -1;
      for (int d : kids) if (sidesB.count(d)) { known = d; break; }
      if (known >= 0) {
         double want = sidesB[known] == sA ? 0 : 180;
         t0 = want;                         // slot 0 az 0 -> known kid
         vector<int> ordered = {known};
         for (int d : kids) if (d != known) ordered.push_back(d);
         kids = ordered;
      }
   }
   bool done = false;
   const size_t K = kids.size();
   vector<int> perm(freeSlot.size());
   for (size_t i = 0; i < perm.size(); i++) perm[i] = (int)i;
   do {
      // kid i takes freeSlot[perm[i]].
      if (ringKid >= 0 && kids[0] == ringKid && perm[0] != 0 && !done) {
         // keep the ring child on slot 0 on the first pass only
      }
      vector<int> assign(K);
      for (size_t i = 0; i < K; i++) assign[i] = freeSlot[perm[i]];
      if (chirSign[c] && chirOrder[c].size() == 4) {
         vector<V3> dir(4);
         bool ok = true;
         for (int q = 0; q < 4; q++) {
            int x = chirOrder[c][q];
            if (x == p) { dir[q] = unit(sub(pos[p], pos[c])); continue; }
            auto fo = find(fixedOther.begin(), fixedOther.end(), x);
            if (fo != fixedOther.end()) { dir[q] = unit(sub(pos[x], pos[c])); continue; }
            auto kt = find(kids.begin(), kids.end(), x);
            if (kt == kids.end()) { ok = false; break; }
            dir[q] = slotDir(assign[kt - kids.begin()], t0);
         }
         if (!ok) { choice = assign; done = true; break; }
         V3 a = sub(dir[1], dir[0]), b = sub(dir[2], dir[0]), cc = sub(dir[3], dir[0]);
         double vol = dot(a, cross(b, cc));
         if (vol * chirSign[c] <= 0) continue;
      }
      choice = assign; done = true; break;
   } while (next_permutation(perm.begin(), perm.end()));
   if (!done) { choice.resize(K); for (size_t i = 0; i < K; i++) choice[i] = freeSlot[i]; }
   for (size_t i = 0; i < K; i++) {
      int d = kids[i];
      if (parent[d] != c) continue;           // ring-closure partner: not placed here
      double ang = 180 - slots[choice[i]].first;
      if (d == ringKid) ang = angRing;
      pos[d] = nerf(A, pos[p], pos[c], ideal(c, d), rad(ang), rad(t0 + slots[choice[i]].second));
      placed[d] = 1;
   }
}

bool Builder::buildOnce()
{
   pos.assign(n, {0, 0, 0});
   placed.assign(n, 0);
   parent.assign(n, -2);
   depth.assign(n, 0);
   vector<int> comp(n, -1);
   int ncomp = 0;
   vector<vector<int> > order;
   for (int s = 0; s < n; s++) {
      if (parent[s] != -2) continue;
      parent[s] = -1;
      queue<int> q; q.push(s);
      vector<int> ord;
      while (!q.empty()) {
         int x = q.front(); q.pop();
         ord.push_back(x); comp[x] = ncomp;
         for (size_t t = 0; t < adj[x].size(); t++) {
            int y = adj[x][t];
            if (bonds[bondOf[x][t]].closure || parent[y] != -2) continue;
            parent[y] = x; depth[y] = depth[x] + 1; q.push(y);
         }
      }
      order.push_back(ord);
      ncomp++;
   }
   double xoff = 0;
   for (int cidx = 0; cidx < ncomp; cidx++) {
      const vector<int>& ord = order[cidx];
      int root = ord[0];
      placed[root] = 1;
      pos[root] = {0, 0, 0};
      // First child along +x so the root has a "parent" to place the rest about.
      for (size_t t = 0; t < adj[root].size(); t++) {
         int y = adj[root][t];
         if (parent[y] == root) {
            pos[y] = {ideal(root, y), 0, 0}; placed[y] = 1; break;
         }
      }
      placeCenter(root);
      for (int x : ord) if (x != root) placeCenter(x);
      // Shift this component clear of the previous ones.
      double lo = 1e9, hi = -1e9;
      for (int x : ord) { lo = min(lo, pos[x][0]); hi = max(hi, pos[x][0]); }
      double shift = (cidx == 0) ? 0 : xoff + 3.5 - lo;
      for (int x : ord) pos[x][0] += shift;
      xoff = hi + shift;
   }
   // Any atom left unplaced (should not happen) goes to the origin region.
   return true;
}

struct Term { int i, j; double d0, k; };

void Builder::relax()
{
   vector<Term> terms;
   for (size_t k = 0; k < bonds.size(); k++)
      terms.push_back({bonds[k].a, bonds[k].b, ideal(bonds[k].a, bonds[k].b), 400});
   for (int c = 0; c < n; c++)
      for (size_t a = 0; a < adj[c].size(); a++)
         for (size_t b = a + 1; b < adj[c].size(); b++) {
            int i = adj[c][a], j = adj[c][b];
            double l1 = ideal(c, i), l2 = ideal(c, j), th = rad(idealAngle(c, i, j));
            double d = sqrt(max(0.0, l1*l1 + l2*l2 - 2*l1*l2*cos(th)));
            terms.push_back({i, j, d, 80});
         }
   // Planarity across double and aromatic bonds between sp2 atoms.
   for (size_t k = 0; k < bonds.size(); k++) {
      int b = bonds[k].a, c = bonds[k].b;
      if (!(bonds[k].ord == 2 || bonds[k].arom) || hyb[b] != 2 || hyb[c] != 2) continue;
      for (int a : adj[b]) {
         if (a == c) continue;
         for (int d : adj[c]) {
            if (d == b || d == a) continue;
            double t = fabs(torsionOf(pos[a], pos[b], pos[c], pos[d]));
            bool cis = t < PI / 2;
            double lab = ideal(a, b), lbc = ideal(b, c), lcd = ideal(c, d);
            double t1 = rad(idealAngle(b, a, c)), t2 = rad(idealAngle(c, b, d));
            V3 pa = {lab * cos(t1), lab * sin(t1), 0};
            V3 pd = {lbc - lcd * cos(t2), (cis ? 1 : -1) * lcd * sin(t2), 0};
            terms.push_back({a, d, norm(sub(pa, pd)), 40});
         }
      }
   }
   // Topological distances for the repulsion.
   vector<vector<unsigned char> > topo(n, vector<unsigned char>(n, 255));
   for (int s = 0; s < n; s++) {
      queue<int> q; q.push(s); topo[s][s] = 0;
      while (!q.empty()) {
         int x = q.front(); q.pop();
         if (topo[s][x] >= 5) continue;
         for (int y : adj[x]) if (topo[s][y] == 255) { topo[s][y] = topo[s][x] + 1; q.push(y); }
      }
   }
   vector<Term> rep;
   for (int i = 0; i < n; i++)
      for (int j = i + 1; j < n; j++) {
         int t = topo[i][j];
         if (t < 3) continue;
         double f = t == 3 ? 0.6 : 0.8;
         rep.push_back({i, j, f * (vdwRadius(el[i]) + vdwRadius(el[j])), 30});
      }
   vector<int> chirAtoms;
   for (int i = 0; i < nh; i++) if (chirSign[i] && chirOrder[i].size() == 4) chirAtoms.push_back(i);

   auto energy = [&](const vector<V3>& x, vector<V3>* gr) {
      double E = 0;
      if (gr) fill(gr->begin(), gr->end(), V3{0, 0, 0});
      for (const Term& t : terms) {
         V3 d = sub(x[t.i], x[t.j]);
         double r = max(1e-9, norm(d)), e = r - t.d0;
         E += 0.5 * t.k * e * e;
         if (gr) { V3 f = mul(d, t.k * e / r); (*gr)[t.i] = add((*gr)[t.i], f); (*gr)[t.j] = sub((*gr)[t.j], f); }
      }
      for (const Term& t : rep) {
         V3 d = sub(x[t.i], x[t.j]);
         double r2 = dot(d, d);
         if (r2 >= t.d0 * t.d0) continue;
         double r = max(1e-9, sqrt(r2)), e = r - t.d0;
         E += 0.5 * t.k * e * e;
         if (gr) { V3 f = mul(d, t.k * e / r); (*gr)[t.i] = add((*gr)[t.i], f); (*gr)[t.j] = sub((*gr)[t.j], f); }
      }
      for (int c : chirAtoms) {
         const vector<int>& o = chirOrder[c];
         V3 p0 = x[o[0]], a = sub(x[o[1]], p0), b = sub(x[o[2]], p0), cc = sub(x[o[3]], p0);
         double v = chirSign[c] * dot(a, cross(b, cc)), vt = 3.0;
         if (v >= vt) continue;
         double e = vt - v;
         E += 2.0 * e * e;
         if (gr) {
            double w = -4.0 * e * chirSign[c];
            V3 g1 = cross(b, cc), g2 = cross(cc, a), g3 = cross(a, b);
            (*gr)[o[1]] = add((*gr)[o[1]], mul(g1, w));
            (*gr)[o[2]] = add((*gr)[o[2]], mul(g2, w));
            (*gr)[o[3]] = add((*gr)[o[3]], mul(g3, w));
            V3 g0 = add(g1, add(g2, g3));
            (*gr)[o[0]] = sub((*gr)[o[0]], mul(g0, w));
         }
      }
      return E;
   };
   vector<V3> grad(n), trial(n), gtrial(n);
   double E = energy(pos, &grad), step = 0.002;
   for (int it = 0; it < 4000; it++) {
      double gmax = 0;
      for (int i = 0; i < n; i++) gmax = max(gmax, norm(grad[i]));
      if (gmax < 1e-3) break;
      for (;;) {
         for (int i = 0; i < n; i++) {
            V3 s = mul(grad[i], -step);
            double l = norm(s);
            if (l > 0.2) s = mul(s, 0.2 / l);
            trial[i] = add(pos[i], s);
         }
         double E2 = energy(trial, &gtrial);
         if (E2 <= E) { pos = trial; grad = gtrial; E = E2; step *= 1.3; break; }
         step *= 0.5;
         if (step < 1e-9) return;
      }
   }
}

// Number of violated checks: wrong chirality, wrong E/Z, bad bond
// lengths, close contacts.
int Builder::check(bool)
{
   int bad = 0;
   for (int c = 0; c < nh; c++) {
      if (!chirSign[c] || chirOrder[c].size() != 4) continue;
      const vector<int>& o = chirOrder[c];
      V3 a = sub(pos[o[1]], pos[o[0]]), b = sub(pos[o[2]], pos[o[0]]), cc = sub(pos[o[3]], pos[o[0]]);
      if (chirSign[c] * dot(a, cross(b, cc)) < 1.0) bad++;
   }
   for (size_t k = 0; k < bonds.size(); k++) {
      int A = bonds[k].a, Bt = bonds[k].b;
      if (bonds[k].ord == 2 && A < nh && Bt < nh && hyb[A] == 2 && hyb[Bt] == 2) {
         map<int,int> sa, sb;
         if (sideOf(A, Bt, sa) && sideOf(Bt, A, sb)) {
            int X = sa.begin()->first, Y = sb.begin()->first;
            bool cis = sa[X] == sb[Y];
            double t = fabs(torsionOf(pos[X], pos[A], pos[Bt], pos[Y]));
            if ((t < PI / 2) != cis) bad++;
         }
      }
      double d = norm(sub(pos[A], pos[Bt]));
      if (fabs(d - ideal(A, Bt)) > 0.1) bad++;
   }
   vector<vector<char> > bonded(n, vector<char>(n, 0));
   for (const BondRec& b : bonds) bonded[b.a][b.b] = bonded[b.b][b.a] = 1;
   for (int i = 0; i < n; i++)
      for (int j = i + 1; j < n; j++) {
         if (bonded[i][j]) continue;
         double lim = (el[i] == "H" && el[j] == "H") ? 0.95 : 1.0;
         if (norm(sub(pos[i], pos[j])) < lim) bad++;
      }
   return bad;
}

}

double idealBond(const string& a, const string& b, double order)
{
   double d = covRadius(a == "*" ? "H" : a) + covRadius(b == "*" ? "H" : b);
   if (a == "H" || b == "H" || a == "*" || b == "*") d += 0.02;
   if (order >= 2.9) d -= 0.32;
   else if (order >= 1.9) d -= 0.18;
   else if (order > 1.1) d -= 0.12;
   return d;
}

bool build3D(const Graph& g, Built& out, string& err)
{
   Builder b(g);
   b.setup();
   for (int i = 0; i < b.n; i++)
      if (b.adj[i].size() > 6) { err = "Unsupported coordination number."; return false; }
   vector<V3> best;
   int bestBad = 1 << 30;
   mt19937 rr(7);
   for (int attempt = 0; attempt < 16; attempt++) {
      b.flip = attempt & 1;
      b.jitter = attempt < 2 ? 0 : (int)(rr() % 360);
      b.buildOnce();
      b.relax();
      int bad = b.check(false);
      if (bad < bestBad) { bestBad = bad; best = b.pos; }
      if (bad == 0) break;
   }
   b.pos = best;
   out = Built();
   for (int i = 0; i < b.n; i++) {
      out.el.push_back(i < b.nh && g.atoms[i].el == "*" ? "H" : b.el[i]);
      out.xyz.push_back(b.pos[i]);
      out.charge.push_back(b.charge[i]);
      if (i < b.nh && g.atoms[i].el == "*") out.dummy = i;
   }
   for (const auto& bd : b.bonds) out.bonds.push_back({bd.a, bd.b, (double)bd.ord, bd.arom});
   // Stereo that could not be honoured is an error, not a silent flip.
   for (int c = 0; c < b.nh; c++) {
      if (!b.chirSign[c] || b.chirOrder[c].size() != 4) continue;
      const vector<int>& o = b.chirOrder[c];
      V3 a = sub(b.pos[o[1]], b.pos[o[0]]), bb = sub(b.pos[o[2]], b.pos[o[0]]), cc = sub(b.pos[o[3]], b.pos[o[0]]);
      if (b.chirSign[c] * dot(a, cross(bb, cc)) <= 0) {
         err = "Could not build the stereochemistry at atom " + to_string(c + 1) + ".";
         return false;
      }
   }
   return true;
}

}
