// FragGraft (#833): torsion extension, ring handling, grafting a fragment
// on an attachment vector, group finding, and the SMILES entry point.
#include "tdat/FragGraft.H"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace FragGraft;
using namespace std;

static int failures = 0;
static void check(bool ok, const char *what)
{
   printf("%s: %s\n", ok ? "ok  " : "FAIL", what);
   if (!ok) failures++;
}
static double dist(const Vec& a, const Vec& b)
{
   return sqrt((a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1]) + (a[2]-b[2])*(a[2]-b[2]));
}

int main()
{
   // Synthetic butane skeleton with C2-C3 gauche (torsion 60).
   Mol m;
   m.el = {"C", "C", "C", "C"};
   m.xyz = {{0, 0, 0}, {1.5, 0, 0}, {2.0, 1.4, 0}, {3.5, 1.4, 0.0}};
   const double t = 60 * M_PI / 180;
   // Place atom 4 at torsion 60 about bond 2-3.
   m.xyz[3] = {2.0 + 0.4, 1.4 + 1.45 * cos(t), 1.45 * sin(t)};
   m.charge = {0, 0, 0, 0};
   m.bonds = {{0, 1, 1}, {1, 2, 1}, {2, 3, 1}};
   double before = torsionDeg(m.xyz[0], m.xyz[1], m.xyz[2], m.xyz[3]);
   extendTorsions(m);
   double after = torsionDeg(m.xyz[0], m.xyz[1], m.xyz[2], m.xyz[3]);
   check(fabs(fabs(after) - 180) < 1e-6, "extendTorsions sets a chain torsion to 180");
   check(fabs(dist(m.xyz[2], m.xyz[3]) - dist({2.0, 1.4, 0}, {2.4, 1.4 + 1.45*cos(t), 1.45*sin(t)})) < 1e-9,
         "bond lengths unchanged");
   (void)before;

   // Ring bond is left alone: a 4-ring of carbons keeps its torsion.
   Mol r;
   r.el = {"C", "C", "C", "C", "C"};
   r.xyz = {{0, 0, 0}, {1.5, 0, 0}, {1.5, 1.5, 0.3}, {0, 1.5, 0}, {-1, -1, 1}};
   r.charge = {0, 0, 0, 0, 0};
   r.bonds = {{0, 1, 1}, {1, 2, 1}, {2, 3, 1}, {3, 0, 1}, {0, 4, 1}};
   vector<Vec> keep = r.xyz;
   extendTorsions(r);
   bool same = true;
   for (int i = 0; i < 5; i++) same = same && dist(keep[i], r.xyz[i]) < 1e-9;
   check(same, "ring bonds are not rotated");

   // Group finding on a chain A-G-X plus a ring.
   vector<vector<int> > adj = {{1}, {0, 2}, {1, 3, 4}, {2}, {2}};
   vector<int> g;
   check(groupBehind(adj, 1, 2, g) && g.size() == 3, "group behind bond 1-2 has 3 atoms");
   int A = -1;
   check(chooseAttachment(adj, 1, A) && A == 2, "group of atom 1 is the smaller side, attached to atom 2");
   vector<vector<int> > ring = {{1, 2}, {0, 2}, {0, 1, 3}, {2}};
   check(!groupBehind(ring, 0, 1, g), "a ring bond has no terminal group");

   // Graft: a fragment of C(attach)-H(dummy) onto an anchor along +x.
   vector<Vec> frag = {{0, 0, 0}, {-1.09, 0, 0}, {0.5, 1.0, 0}, {0.5, -0.5, 0.9}};
   vector<Vec> rest = {{0, 1.8, 0}, {0.0, -1.8, 0.0}, {-1.5, 0, 0}};
   double ang, md;
   vector<Vec> out = graft(frag, 0, 1, {-1.5, 0, 0}, {1, 0, 0}, 1.8, rest, 10, &ang, &md);
   check(fabs(dist(out[0], {0.3, 0, 0}) ) < 1e-9, "attach atom sits L along u from the anchor");
   check(fabs(out[1][1]) < 1e-9 && out[1][0] < out[0][0], "dummy lies behind the attach atom on the axis");
   double dm = 1e9;
   for (size_t i = 2; i < out.size(); i++)
      for (const Vec& p : rest) dm = fmin(dm, dist(out[i], p));
   check(fabs(dm - md) < 1e-9, "reported minimum distance matches");
   for (double a2 = 0; a2 < 360; a2 += 10) {
      // No scanned angle does better than the chosen one.
      vector<Vec> alt = out;
      double c = cos(a2 * M_PI / 180), s = sin(a2 * M_PI / 180), best = 1e9;
      for (size_t i = 2; i < alt.size(); i++) {
         double y = out[i][1], z = out[i][2];
         Vec p = {out[i][0], y * c - z * s, y * s + z * c};
         for (const Vec& q : rest) best = fmin(best, dist(p, q));
      }
      if (best > md + 1e-6) { check(false, "chosen rotation maximises the minimum distance"); break; }
   }

   // SMILES parts.
   Mol z;
   string err;
   check(smilesTo3D("[NH3+][C@@H](C[S-])C(=O)[O-]", z, err), "cysteine zwitterion builds");
   int q = 0;
   for (int c : z.charge) q += c;
   check(z.el.size() == 13 && q == -1, "cysteine zwitterion: 13 atoms, charge -1");
   // The charge before the hydrogens, as MOE writes it (#961).
   Mol mq;
   check(smilesTo3D("C[N+H](C)C", mq, err), "[N+H] (charge first) reads");
   q = 0;
   for (int c : mq.charge) q += c;
   check(mq.el.size() == 14 && q == 1, "trimethylammonium: 14 atoms, charge +1");
   check(smilesTo3D("C[N+H2]C", mq, err) && mq.el.size() == 11, "[N+H2]: 11 atoms");
   Mol b;
   check(smilesTo3D("CCCCCCCC", b, err), "octane from SMILES");
   extendTorsions(b);
   bool all180 = true;
   vector<vector<int> > ba(b.el.size());
   for (const Bond& bd : b.bonds) { ba[bd.a].push_back(bd.b); ba[bd.b].push_back(bd.a); }
   for (int i = 0; i < (int)b.el.size(); i++) {
      if (b.el[i] != "C") continue;
      for (int j : ba[i]) {
         if (b.el[j] != "C" || j < i) continue;
         int x = -1, y = -1;
         for (int k : ba[i]) if (k != j && b.el[k] == "C") x = k;
         for (int k : ba[j]) if (k != i && b.el[k] == "C") y = k;
         if (x < 0 || y < 0) continue;
         all180 = all180 && fabs(fabs(torsionDeg(b.xyz[x], b.xyz[i], b.xyz[j], b.xyz[y])) - 180) < 1e-3;
      }
   }
   check(all180, "octane: every C-C torsion is 180 after extendTorsions");
   Mol d;
   check(smilesTo3D("[*]C(=O)O", d, err) && d.dummy >= 0 && d.el[d.dummy] == "H",
         "[*] fragment keeps its attachment atom");
   return failures ? 1 : 0;
}
