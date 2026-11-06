#include "tdat/FragGraft.H"
#include "tdat/Smiles.H"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <queue>
#include <sstream>

using namespace std;

namespace FragGraft {

namespace {

const double PI = 3.14159265358979323846;

Vec sub(const Vec& a, const Vec& b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
Vec add(const Vec& a, const Vec& b) { return {a[0]+b[0], a[1]+b[1], a[2]+b[2]}; }
Vec mul(const Vec& a, double s) { return {a[0]*s, a[1]*s, a[2]*s}; }
double dot(const Vec& a, const Vec& b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Vec cross(const Vec& a, const Vec& b)
{
   return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}
double norm(const Vec& a) { return sqrt(dot(a, a)); }
Vec unit(const Vec& a) { double n = norm(a); return n > 0 ? mul(a, 1/n) : a; }

// Rodrigues rotation of p about the axis through o along unit k.
Vec rotate(const Vec& p, const Vec& o, const Vec& k, double ang)
{
   Vec v = sub(p, o);
   double c = cos(ang), s = sin(ang);
   Vec r = add(add(mul(v, c), mul(cross(k, v), s)), mul(k, dot(k, v)*(1-c)));
   return add(o, r);
}

vector<vector<int> > adjacency(size_t n, const vector<Bond>& bonds)
{
   vector<vector<int> > adj(n);
   for (const Bond& b : bonds) {
      adj[b.a].push_back(b.b);
      adj[b.b].push_back(b.a);
   }
   return adj;
}

}


bool smilesTo3D(const string& smiles, Mol& mol, string& err)
{
   string smi = smiles;
   smi.erase(remove_if(smi.begin(), smi.end(), [](char c) {
      return c == '"' || c == '\n' || c == '\r'; }), smi.end());
   Smiles::Graph g;
   if (!Smiles::parse(smi, g, err)) return false;
   Smiles::Built b;
   if (!Smiles::build3D(g, b, err)) return false;
   mol = Mol();
   mol.el = b.el;
   mol.charge = b.charge;
   mol.dummy = b.dummy;
   for (const auto& x : b.xyz) mol.xyz.push_back({x[0], x[1], x[2]});
   for (const auto& bd : b.bonds) mol.bonds.push_back({bd.a, bd.b, bd.order});
   return true;
}


Vec placeBeside(const vector<Vec>& existing, const vector<Vec>& addv, double gap)
{
   if (existing.empty() || addv.empty()) return {0, 0, 0};
   double hiX = existing[0][0], loX = addv[0][0];
   Vec c = {0, 0, 0};
   for (const Vec& p : existing) hiX = max(hiX, p[0]);
   for (const Vec& p : addv) loX = min(loX, p[0]);
   for (const Vec& p : existing) c = add(c, p);
   c = mul(c, 1.0 / existing.size());
   Vec cn = {0, 0, 0};
   for (const Vec& p : addv) cn = add(cn, p);
   cn = mul(cn, 1.0 / addv.size());
   // Along x past the existing atoms, centred on the same y and z.
   return {hiX + gap - loX, c[1] - cn[1], c[2] - cn[2]};
}


double torsionDeg(const Vec& a, const Vec& b, const Vec& c, const Vec& d)
{
   Vec b1 = sub(b, a), b2 = sub(c, b), b3 = sub(d, c);
   Vec n1 = cross(b1, b2), n2 = cross(b2, b3);
   double y = dot(cross(n1, n2), unit(b2)), x = dot(n1, n2);
   return atan2(y, x) * 180.0 / PI;
}


bool groupBehind(const vector<vector<int> >& adj, int A, int G, vector<int>& group)
{
   group.clear();
   vector<char> seen(adj.size(), 0);
   queue<int> q;
   q.push(G);
   seen[G] = 1;
   seen[A] = 1;
   while (!q.empty()) {
      int i = q.front();
      q.pop();
      group.push_back(i);
      for (int j : adj[i]) {
         if (j == A && i != G) return false;     // back to A: a ring
         if (!seen[j]) { seen[j] = 1; q.push(j); }
      }
   }
   return true;
}


bool chooseAttachment(const vector<vector<int> >& adj, int G, int& A)
{
   size_t best = 0;
   bool tie = false, found = false;
   for (int a : adj[G]) {
      vector<int> g;
      if (!groupBehind(adj, a, G, g)) continue;
      if (!found || g.size() < best) { best = g.size(); A = a; found = true; tie = false; }
      else if (g.size() == best) tie = true;
   }
   return found && !tie;
}


void extendTorsions(Mol& mol)
{
   const size_t n = mol.el.size();
   vector<vector<int> > adj = adjacency(n, mol.bonds);
   auto heavy = [&](int i) { return mol.el[i] != "H"; };
   // Breadth-first from atom 0, so each bond is met before the bonds
   // beyond it.
   vector<int> order, depth(n, -1);
   queue<int> q;
   if (n) { q.push(0); depth[0] = 0; }
   while (!q.empty()) {
      int i = q.front();
      q.pop();
      order.push_back(i);
      for (int j : adj[i]) if (depth[j] < 0) { depth[j] = depth[i] + 1; q.push(j); }
   }
   for (int p : order) {
      for (int o : adj[p]) {
         if (depth[o] != depth[p] + 1 && !(depth[o] == depth[p] && o > p)) continue;
         if (!heavy(p) || !heavy(o)) continue;
         double ord = 0;
         for (const Bond& b : mol.bonds)
            if ((b.a == p && b.b == o) || (b.a == o && b.b == p)) ord = b.order;
         if (ord != 1.0) continue;
         vector<int> far;
         if (!groupBehind(adj, p, o, far)) continue;      // ring bond
         int ra = -1, rd = -1;
         for (int j : adj[p]) if (j != o && heavy(j)) { ra = j; break; }
         for (int j : adj[o]) if (j != p && heavy(j)) { rd = j; break; }
         if (ra < 0 || rd < 0) continue;
         double phi = torsionDeg(mol.xyz[ra], mol.xyz[p], mol.xyz[o], mol.xyz[rd]);
         double delta = (180.0 - phi) * PI / 180.0;
         Vec k = unit(sub(mol.xyz[o], mol.xyz[p]));
         for (int i : far) mol.xyz[i] = rotate(mol.xyz[i], mol.xyz[p], k, delta);
      }
   }
}


vector<Vec> graft(const vector<Vec>& frag, int attach, int dummy,
                  const Vec& anchor, const Vec& u, double L,
                  const vector<Vec>& rest, double stepDeg,
                  double *bestAngle, double *bestMinDist)
{
   // Turn (attach - dummy) onto u: rotation about their cross product.
   const Vec from = unit(sub(frag[attach], frag[dummy]));
   const Vec to = unit(u);
   Vec axis = cross(from, to);
   double s = norm(axis), c = dot(from, to);
   vector<Vec> out(frag.size());
   const Vec target = add(anchor, mul(to, L));
   for (size_t i = 0; i < frag.size(); i++) {
      Vec v = sub(frag[i], frag[attach]);
      if (s > 1e-9) v = rotate(v, {0, 0, 0}, mul(axis, 1/s), atan2(s, c));
      else if (c < 0) {                 // opposite: half turn about any normal
         Vec perp = fabs(from[0]) < 0.9 ? Vec{1, 0, 0} : Vec{0, 1, 0};
         v = rotate(v, {0, 0, 0}, unit(cross(from, perp)), PI);
      }
      out[i] = add(target, v);
   }
   double bestA = 0, bestD = -1;
   const int steps = max(1, (int)floor(360.0 / stepDeg + 0.5));
   for (int k = 0; k < steps; k++) {
      double ang = 2 * PI * k / steps, dmin = 1e30;
      for (size_t i = 0; i < out.size(); i++) {
         if ((int)i == dummy || (int)i == attach) continue;
         Vec p = rotate(out[i], target, to, ang);
         for (const Vec& r : rest) dmin = min(dmin, norm(sub(p, r)));
      }
      if (dmin > bestD + 1e-9) { bestD = dmin; bestA = ang; }
   }
   for (size_t i = 0; i < out.size(); i++) out[i] = rotate(out[i], target, to, bestA);
   if (bestAngle) *bestAngle = bestA * 180.0 / PI;
   if (bestMinDist) *bestMinDist = bestD;
   return out;
}

}
