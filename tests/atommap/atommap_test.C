// Atom mapping and alignment (#419).  The oracles are known by construction
// or independent of the code: a random permutation, rotation and translation
// applied to real library structures; brute-force assignment for the
// Hungarian routine; the real HCN/HNC geometries of the IRC log.
//
//   atommap_test <source dir>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "tdat/AtomMap.H"
#include "xyzio.H"

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; \
   printf("FAIL %s:%d  ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static std::mt19937 rng(20261021);

// Random proper rotation from a normalised random quaternion (own formula,
// not the library's).
static void randomRotation(double R[9])
{
   std::normal_distribution<double> g(0.0, 1.0);
   double q[4], n = 0;
   for (int i = 0; i < 4; i++) { q[i] = g(rng); n += q[i]*q[i]; }
   n = sqrt(n);
   for (int i = 0; i < 4; i++) q[i] /= n;
   const double a = q[0], b = q[1], c = q[2], d = q[3];
   const double M[9] = {
      1-2*(c*c+d*d), 2*(b*c-a*d),   2*(b*d+a*c),
      2*(b*c+a*d),   1-2*(b*b+d*d), 2*(c*d-a*b),
      2*(b*d-a*c),   2*(c*d+a*b),   1-2*(b*b+c*c) };
   for (int i = 0; i < 9; i++) R[i] = M[i];
}

// Moving copy of s: atom i of s becomes atom perm-position; out[k] = s[order[k]].
static Xyz scramble(const Xyz& s, const std::vector<int>& order, bool rotate)
{
   double R[9] = {1,0,0,0,1,0,0,0,1};
   double t[3] = {0,0,0};
   if (rotate) {
      randomRotation(R);
      std::uniform_real_distribution<double> u(-10, 10);
      for (int k = 0; k < 3; k++) t[k] = u(rng);
   }
   Xyz o;
   for (size_t k = 0; k < order.size(); k++) {
      const int i = order[k];
      o.el.push_back(s.el[i]);
      for (int a = 0; a < 3; a++)
         o.c.push_back(R[3*a]*s.c[3*i] + R[3*a+1]*s.c[3*i+1] +
                       R[3*a+2]*s.c[3*i+2] + t[a]);
   }
   return o;
}

static double dist(const std::vector<double>& a, int i, const std::vector<double>& b, int j)
{
   double s = 0;
   for (int k = 0; k < 3; k++) { const double d = a[3*i+k]-b[3*j+k]; s += d*d; }
   return sqrt(s);
}

static void testHungarian()
{
   for (int trial = 0; trial < 50; trial++) {
      const int n = 2 + trial % 6;
      std::uniform_real_distribution<double> u(0, 10);
      std::vector<double> c(n*n);
      for (size_t i = 0; i < c.size(); i++) c[i] = u(rng);
      const std::vector<int> sol = AtomMap::assign(c, n);
      double got = 0;
      for (int i = 0; i < n; i++) got += c[i*n+sol[i]];
      std::vector<int> p(n);
      for (int i = 0; i < n; i++) p[i] = i;
      double best = 1e300;
      do {
         double s = 0;
         for (int i = 0; i < n; i++) s += c[i*n+p[i]];
         best = std::min(best, s);
      } while (std::next_permutation(p.begin(), p.end()));
      CHECK(fabs(got - best) < 1e-9, "Hungarian n=%d: %g vs brute force %g", n, got, best);
   }
}

// Scrambled copies of real molecules: elements and aligned positions must
// agree with the reference atom the mapping names (index may differ inside a
// symmetric group).
static void testPermutations(const std::string& root)
{
   const char* files[] = {
      "Small_organic_molecules/Hydrocarbons/Propane.mvm",
      "Small_organic_molecules/Hydrocarbons/Isobutane.mvm",
      "Small_organic_molecules/Alcohols_ethers_carbonyls_and_acids/Ethanol.mvm",
      "Small_organic_molecules/Arenes/Toluene.mvm",
      "Small_organic_molecules/Alicycles/Cyclobutane.mvm",
      "Biomolecules/Amino_acids/Interior/Alanine.mvm",
      "Biomolecules/Amino_acids/Interior/Phenylalanine.mvm",
      "Inorganic/Ligands/Bipyridine.mvm" };
   for (size_t f = 0; f < sizeof(files)/sizeof(files[0]); f++) {
      Xyz s;
      if (!readMvm(root + "/data/client/StructureLibrary/" + files[f], s)) {
         failures++; printf("FAIL cannot read %s\n", files[f]); continue;
      }
      const int n = (int)s.el.size();
      for (int trial = 0; trial < 20; trial++) {
         std::vector<int> order(n);
         for (int i = 0; i < n; i++) order[i] = i;
         std::shuffle(order.begin(), order.end(), rng);
         const Xyz m = scramble(s, order, true);
         const AtomMapResult r = AtomMap::map(s.el, s.c, m.el, m.c);
         CHECK(r.ok, "%s: %s", files[f], r.error.c_str());
         if (!r.ok) continue;
         std::vector<double> moved = m.c;
         AtomMap::apply(r.alignment, moved);
         double worst = 0;
         bool elems = true, used[512] = {false};
         for (int i = 0; i < n; i++) {
            elems = elems && (m.el[i] == s.el[r.map[i]]);
            worst = std::max(worst, dist(moved, i, s.c, r.map[i]));
            used[r.map[i]] = true;
         }
         int count = 0;
         for (int i = 0; i < n; i++) count += used[i];
         CHECK(count == n, "%s: mapping is not a permutation", files[f]);
         CHECK(elems, "%s: element changed by the mapping", files[f]);
         CHECK(worst < 1e-3, "%s trial %d: worst displacement %g", files[f], trial, worst);
         CHECK(r.alignment.rmsdAfter < 1e-4, "%s: RMSD %g", files[f], r.alignment.rmsdAfter);
         CHECK(r.far.empty(), "%s: pairs flagged far", files[f]);
         // renumbered and aligned, the structure is the reference
         const std::vector<double> back = AtomMap::renumber(moved, r.map);
         double w2 = 0;
         for (int i = 0; i < n; i++) w2 = std::max(w2, dist(back, i, s.c, i));
         CHECK(w2 < 1e-3, "%s: renumbered structure differs by %g", files[f], w2);
      }
   }
}

// Same order, only rotated and a little noisy: the order is kept.
static void testIdentity(const std::string& root)
{
   Xyz s;
   readMvm(root + "/data/client/StructureLibrary/Small_organic_molecules/Hydrocarbons/Isobutane.mvm", s);
   const int n = (int)s.el.size();
   std::vector<int> order(n);
   for (int i = 0; i < n; i++) order[i] = i;
   Xyz m = scramble(s, order, true);
   std::normal_distribution<double> g(0, 0.03);
   for (size_t i = 0; i < m.c.size(); i++) m.c[i] += g(rng);
   const AtomMapResult r = AtomMap::map(s.el, s.c, m.el, m.c);
   CHECK(r.ok && r.identity && r.nChanged == 0, "isobutane: order not kept");
   CHECK(r.ok && r.alignment.rmsdAfter < 0.1, "isobutane: RMSD %g", r.alignment.rmsdAfter);
   CHECK(r.ok && r.alignment.rmsdBefore > 1.0, "isobutane: test rotation too small");
}

// Real geometries from the IRC log (the end points it reaches: C-H 1.12 A, N-H 1.15 A): the heavy atoms and H are all different
// elements, so the right answer is unique; bonds change (H moves from C to N).
static bool irc(const std::string& root, Xyz& hcn, Xyz& hnc)
{
   std::ifstream in(root + "/tests/parsers/fixtures/gaussian-16/hcn_irc.log");
   if (!in) return false;
   std::string line;
   double bestCH = 1e9, bestNH = 1e9;
   while (std::getline(in, line)) {
      if (line.find("Input orientation") == std::string::npos) continue;
      for (int i = 0; i < 4; i++) std::getline(in, line);
      Xyz s;
      while (std::getline(in, line) && line.find("-----") == std::string::npos) {
         int idx, z, t; double x, y, w;
         if (sscanf(line.c_str(), "%d %d %d %lf %lf %lf", &idx, &z, &t, &x, &y, &w) != 6) break;
         s.el.push_back(z == 6 ? "C" : z == 7 ? "N" : "H");
         s.c.push_back(x); s.c.push_back(y); s.c.push_back(w);
      }
      if (s.el.size() != 3 || s.el[0] != "C" || s.el[1] != "N") continue;
      const double ch = dist(s.c, 0, s.c, 2), nh = dist(s.c, 1, s.c, 2);
      if (ch < bestCH) { bestCH = ch; hcn = s; }
      if (nh < bestNH) { bestNH = nh; hnc = s; }
   }
   return bestCH < 1.2 && bestNH < 1.2;
}

static void testReaction(const std::string& root)
{
   // The H moves less than the default 1 A flag in these two cases.
   AtomMapOptions loose;
   loose.farThreshold = 0.6;
   Xyz hcn, hnc;
   if (!irc(root, hcn, hnc)) { failures++; printf("FAIL cannot read HCN/HNC from the IRC log\n"); return; }
   // product written in another order: H, N, C
   const int ord[3] = {2, 1, 0};
   std::vector<int> order(ord, ord+3);
   const Xyz prod = scramble(hnc, order, true);
   const AtomMapResult r = AtomMap::map(hcn.el, hcn.c, prod.el, prod.c, loose);
   CHECK(r.ok, "%s", r.error.c_str());
   if (r.ok) {
      // product atom 0 is H -> reactant atom 2 (H), 1 is N -> 1, 2 is C -> 0
      CHECK(r.map[0] == 2 && r.map[1] == 1 && r.map[2] == 0, "HCN/HNC mapping %d %d %d",
            r.map[0], r.map[1], r.map[2]);
      CHECK(r.nChanged == 2, "nChanged %d", r.nChanged);
   }
   // Textbook geometries (HCN r(CH)=1.065, r(CN)=1.153; HNC r(NH)=0.994,
   // r(NC)=1.169 A, both linear): here the H really changes place.
   Xyz a, b;
   a.el = hcn.el; a.c = {0,0,0, 1.153,0,0, -1.065,0,0};
   b.el = hcn.el; b.c = {0,0,0, 1.169,0,0, 2.163,0,0};
   const Xyz bs = scramble(b, order, true);
   const AtomMapResult t = AtomMap::map(a.el, a.c, bs.el, bs.c);
   CHECK(t.ok && t.map[0] == 2 && t.map[1] == 1 && t.map[2] == 0, "linear HCN/HNC mapping");
   // the H really moved: the fit stays poor and something is flagged far
   CHECK(t.ok && t.alignment.rmsdAfter > 0.5 && !t.far.empty(),
         "bond change not visible: RMSD %g, %d flagged", t.alignment.rmsdAfter,
         t.ok ? (int)t.far.size() : -1);
   // same order: stays identity although H moved
   const AtomMapResult same = AtomMap::map(hcn.el, hcn.c, hnc.el, hnc.c);
   CHECK(same.ok && same.identity, "HCN/HNC in the same order must stay identity");

   // Bond change with equivalent atoms: propane with one H of a CH2 pulled
   // half way to the neighbouring carbon, atoms shuffled.  Every atom but
   // that H must sit on its own counterpart.
   Xyz p;
   readMvm(root + "/data/client/StructureLibrary/Small_organic_molecules/Hydrocarbons/Propane.mvm", p);
   Xyz q = p;
   const int moved = 3, from = 1, to = 0;   // H3 sits on C1; C0 is the central carbon
   for (int k = 0; k < 3; k++) q.c[3*moved+k] += 0.5*(p.c[3*to+k] - p.c[3*moved+k]);
   (void)from;
   for (int trial = 0; trial < 20; trial++) {
      std::vector<int> o(p.el.size());
      for (size_t i = 0; i < o.size(); i++) o[i] = (int)i;
      std::shuffle(o.begin(), o.end(), rng);
      const Xyz m = scramble(q, o, true);
      const AtomMapResult pr = AtomMap::map(p.el, p.c, m.el, m.c, loose);
      CHECK(pr.ok, "%s", pr.error.c_str());
      if (!pr.ok) continue;
      int bad = 0, far = 0;
      for (size_t i = 0; i < o.size(); i++) {
         if (m.el[i] != p.el[pr.map[i]]) bad++;
         if (o[i] == moved) {
            far += (pr.displacement[i] > 0.8);
         } else if (pr.displacement[i] > 0.3) bad++;
      }
      CHECK(bad == 0 && far == 1, "propane H shift: %d atoms off, %d far", bad, far);
      CHECK(pr.far.size() == 1, "propane H shift: %d flagged", (int)pr.far.size());
   }
}

// Alignment rule (#419): with >= 3 heavy atoms the fit uses them alone, so a
// moved H cannot drag the skeleton; with fewer (HCN/HNC above) the fit is
// mass-weighted over all atoms and the H shows up as the misfit.
static void testHeavyAlignment(const std::string& root)
{
   Xyz p;
   readMvm(root + "/data/client/StructureLibrary/Small_organic_molecules/Hydrocarbons/Propane.mvm", p);
   Xyz q = p;
   int h = -1;
   for (size_t i = 0; i < p.el.size(); i++) if (p.el[i] == "H") { h = (int)i; break; }
   for (int k = 0; k < 3; k++) q.c[3*h+k] += 2.0;
   std::vector<int> order(p.el.size());
   for (size_t i = 0; i < order.size(); i++) order[i] = (int)i;
   const Xyz m = scramble(q, order, true);
   const AtomMapResult r = AtomMap::map(p.el, p.c, m.el, m.c);
   CHECK(r.ok && r.identity, "propane with one displaced H keeps its order");
   if (!r.ok) return;
   double heavyMax = 0;
   for (size_t i = 0; i < p.el.size(); i++)
      if (p.el[i] != "H") heavyMax = std::max(heavyMax, r.displacement[i]);
   CHECK(heavyMax < 1e-3, "heavy atoms must superpose exactly, worst %g", heavyMax);
   CHECK(r.far.size() == 1 && r.far[0] == h, "only the displaced H is flagged (%d)", (int)r.far.size());
}

static void testErrors(const std::string& root)
{
   Xyz p, e;
   readMvm(root + "/data/client/StructureLibrary/Small_organic_molecules/Hydrocarbons/Propane.mvm", p);
   readMvm(root + "/data/client/StructureLibrary/Small_organic_molecules/Alcohols_ethers_carbonyls_and_acids/Ethanol.mvm", e);
   AtomMapResult r = AtomMap::map(p.el, p.c, e.el, e.c);
   CHECK(!r.ok && !r.error.empty(), "count mismatch must be an error");
   printf("count mismatch: %s\n", r.error.c_str());
   Xyz q = p;
   q.el[0] = "N";
   r = AtomMap::map(p.el, p.c, q.el, q.c);
   CHECK(!r.ok && r.error.find("different elements") != std::string::npos, "element mismatch must be an error");
   printf("element mismatch: %s\n", r.error.c_str());
   Xyz empty;
   r = AtomMap::map(empty.el, empty.c, empty.el, empty.c);
   CHECK(r.ok && r.map.empty(), "empty structures");
}

int main(int argc, char** argv)
{
   const std::string root = argc > 1 ? argv[1] : ".";
   testHungarian();
   testPermutations(root);
   testIdentity(root);
   testReaction(root);
   testHeavyAlignment(root);
   testErrors(root);
   printf(failures ? "%d FAILURES\n" : "all atommap tests passed\n", failures);
   return failures ? 1 : 0;
}
