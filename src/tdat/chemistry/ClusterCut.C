#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include "tdat/ClusterCut.H"
#include "tdat/TPerTab.H"

namespace {

typedef std::array<double,3> V3;
typedef std::array<int,3> I3;
typedef std::pair<int,I3> Key;     // unit-cell atom and its lattice shift

V3 cross(const V3& a, const V3& b)
{
  return V3{{a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]}};
}
double dot(const V3& a, const V3& b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
double norm(const V3& a) { return std::sqrt(dot(a, a)); }
V3 sub(const V3& a, const V3& b) { return V3{{a[0]-b[0], a[1]-b[1], a[2]-b[2]}}; }

// One bond of a unit-cell atom i: atom `to`, displaced by `shift` cells.
struct Edge { int to; I3 shift; };

// Standard element-hydrogen / element-oxygen distances, else the radii sum.
double capLength(const std::string& x, bool hydrogen, double radiusSum)
{
  static const std::map<std::string,double> toH = {
      {"C",1.09},{"O",0.96},{"Si",1.48},{"N",1.01}};
  static const std::map<std::string,double> toO = {{"C",1.43},{"Si",1.63}};
  const std::map<std::string,double>& t = hydrogen ? toH : toO;
  std::map<std::string,double>::const_iterator it = t.find(x);
  return it != t.end() ? it->second : radiusSum;
}

}  // namespace

std::vector<ClusterAtom> cutCluster(const std::vector<std::string>& symbols,
    const std::vector<std::array<double,3> >& xyz,
    const std::array<std::array<double,3>,3>& cell,
    const std::array<double,3>& centre, const ClusterOptions& opt)
{
  const int n = static_cast<int>(symbols.size());
  if (!std::isfinite(opt.radius) || opt.radius <= 0.0)
    throw std::invalid_argument("Radius must be positive.");
  const double volume = dot(cross(cell[0], cell[1]), cell[2]);
  if (std::fabs(volume) < 1.e-8)
    throw std::invalid_argument("The structure has no usable lattice.");
  V3 inv[3] = {cross(cell[1], cell[2]), cross(cell[2], cell[0]),
               cross(cell[0], cell[1])};
  for (int k = 0; k < 3; ++k)
    for (int c = 0; c < 3; ++c) inv[k][c] /= volume;

  TPerTab tpt;
  std::vector<double> rad(n);
  double rmax = 0.0;
  for (int i = 0; i < n; ++i) {
    int z = tpt.atomicNumber(symbols[i]);
    if (z <= 0) throw std::invalid_argument("Unknown element " + symbols[i]);
    rad[i] = tpt.covalentRadius(z);
    rmax = std::max(rmax, rad[i]);
  }
  const double slack = 0.4;
  const double cutMax = 2.0 * rmax + slack;

  // Whole-cell shifts t with t.A near d, to within `reach` Angstrom.
  auto range = [&](const V3& d, double reach, int k, int& lo, int& hi) {
    double f = dot(d, inv[k]), b = reach * norm(inv[k]);
    lo = static_cast<int>(std::ceil(f - b - 1.e-10));
    hi = static_cast<int>(std::floor(f + b + 1.e-10));
  };
  auto shifted = [&](int i, const I3& s) {
    V3 p = xyz[i];
    for (int k = 0; k < 3; ++k)
      for (int c = 0; c < 3; ++c) p[c] += s[k] * cell[k][c];
    return p;
  };

  // Bonds of the unit cell, including those through its faces.
  std::vector<std::vector<Edge> > adj(n);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      const double cut = rad[i] + rad[j] + slack;
      V3 d = sub(xyz[i], xyz[j]);
      int lo[3], hi[3];
      for (int k = 0; k < 3; ++k) range(d, cutMax, k, lo[k], hi[k]);
      for (int a = lo[0]; a <= hi[0]; ++a)
        for (int b = lo[1]; b <= hi[1]; ++b)
          for (int c = lo[2]; c <= hi[2]; ++c) {
            I3 t = {{a, b, c}};
            if (i == j && a == 0 && b == 0 && c == 0) continue;
            double dist = norm(sub(shifted(j, t), xyz[i]));
            if (dist > 0.4 && dist <= cut) adj[i].push_back(Edge{j, t});
          }
    }

  // Bonded components, each with the shifts that keep it contiguous; one
  // that meets itself through a different shift is a network (infinite).
  std::vector<int> comp(n, -1);
  std::vector<I3> off(n);
  std::vector<std::vector<int> > members;
  std::vector<bool> infinite;
  for (int s = 0; s < n; ++s) {
    if (comp[s] >= 0) continue;
    int id = static_cast<int>(members.size());
    members.push_back(std::vector<int>());
    infinite.push_back(false);
    std::vector<int> stack(1, s);
    comp[s] = id;
    off[s] = I3{{0, 0, 0}};
    while (!stack.empty()) {
      int i = stack.back();
      stack.pop_back();
      members[id].push_back(i);
      for (const Edge& e : adj[i]) {
        I3 want = {{off[i][0]+e.shift[0], off[i][1]+e.shift[1], off[i][2]+e.shift[2]}};
        if (comp[e.to] < 0) {
          comp[e.to] = id;
          off[e.to] = want;
          stack.push_back(e.to);
        } else if (off[e.to] != want) infinite[id] = true;
      }
    }
  }

  const double r2 = opt.radius * opt.radius;
  std::set<Key> chosen;
  for (int i = 0; i < n; ++i) {
    V3 d = sub(centre, xyz[i]);
    int lo[3], hi[3];
    for (int k = 0; k < 3; ++k) range(d, opt.radius, k, lo[k], hi[k]);
    for (int a = lo[0]; a <= hi[0]; ++a)
      for (int b = lo[1]; b <= hi[1]; ++b)
        for (int c = lo[2]; c <= hi[2]; ++c) {
          I3 t = {{a, b, c}};
          V3 e = sub(shifted(i, t), centre);
          if (dot(e, e) <= r2) chosen.insert(Key(i, t));
        }
  }
  // Both modes keep finite bonded components whole.
  if (opt.whole != ClusterOptions::WholeNone) {
    std::vector<Key> seeds(chosen.begin(), chosen.end());
    for (const Key& k : seeds) {
      if (infinite[comp[k.first]]) continue;
      for (int j : members[comp[k.first]]) {
        I3 t = {{k.second[0]-off[k.first][0]+off[j][0],
                 k.second[1]-off[k.first][1]+off[j][1],
                 k.second[2]-off[k.first][2]+off[j][2]}};
        chosen.insert(Key(j, t));
      }
    }
  }
  // Units: trim the network part to a whole number of formula units.
  if (opt.whole == ClusterOptions::WholeUnits) {
    std::map<std::string,int> cellCount, have;
    for (int i = 0; i < n; ++i)
      if (infinite[comp[i]]) cellCount[symbols[i]]++;
    int g = 0;
    for (auto& kv : cellCount) g = std::gcd(g, kv.second);
    for (const Key& k : chosen)
      if (infinite[comp[k.first]]) have[symbols[k.first]]++;
    int units = -1;
    for (auto& kv : cellCount) {
      int u = have[kv.first] / (kv.second / g);
      units = units < 0 ? u : std::min(units, u);
    }
    if (units > 0) {
      for (auto& kv : cellCount) {
        int surplus = have[kv.first] - units * (kv.second / g);
        std::vector<std::pair<double,Key> > ranked;
        for (const Key& k : chosen)
          if (infinite[comp[k.first]] && symbols[k.first] == kv.first)
            ranked.push_back(std::make_pair(
                norm(sub(shifted(k.first, k.second), centre)), k));
        std::sort(ranked.begin(), ranked.end());
        for (int m = 0; m < surplus; ++m)
          chosen.erase(ranked[ranked.size() - 1 - m].second);
      }
    }
  }

  std::vector<ClusterAtom> out;
  for (const Key& k : chosen) {
    ClusterAtom a;
    a.symbol = symbols[k.first];
    V3 p = shifted(k.first, k.second);
    for (int c = 0; c < 3; ++c) a.xyz[c] = p[c];
    a.cap = false;
    out.push_back(a);
  }
  if (opt.cap == ClusterOptions::CapNone) return out;

  const double rH = tpt.covalentRadius(1), rO = tpt.covalentRadius(8);
  for (const Key& k : chosen) {
    const std::string& x = symbols[k.first];
    if (x == "H") continue;
    V3 p = shifted(k.first, k.second);
    for (const Edge& e : adj[k.first]) {
      I3 t = {{k.second[0]+e.shift[0], k.second[1]+e.shift[1], k.second[2]+e.shift[2]}};
      if (chosen.count(Key(e.to, t))) continue;
      V3 u = sub(shifted(e.to, t), p);
      double len = norm(u);
      for (int c = 0; c < 3; ++c) u[c] /= len;
      ClusterAtom a;
      a.cap = true;
      if (opt.cap == ClusterOptions::CapH) {
        a.symbol = "H";
        double d = capLength(x, true, rad[k.first] + rH);
        for (int c = 0; c < 3; ++c) a.xyz[c] = p[c] + d * u[c];
        out.push_back(a);
      } else {
        double d = capLength(x, false, rad[k.first] + rO);
        V3 o = {{p[0]+d*u[0], p[1]+d*u[1], p[2]+d*u[2]}};
        // The hydroxyl H bends away from the cluster, X-O-H = 109.5 degrees.
        V3 w = sub(p, centre);
        double along = dot(w, u);
        for (int c = 0; c < 3; ++c) w[c] -= along * u[c];
        if (norm(w) < 1.e-6) {
          V3 axis = {{std::fabs(u[0]) < 0.9 ? 1.0 : 0.0,
                      std::fabs(u[0]) < 0.9 ? 0.0 : 1.0, 0.0}};
          w = cross(u, axis);
        }
        double wl = norm(w);
        for (int c = 0; c < 3; ++c) w[c] /= wl;
        a.symbol = "O";
        for (int c = 0; c < 3; ++c) a.xyz[c] = o[c];
        out.push_back(a);
        a.symbol = "H";
        for (int c = 0; c < 3; ++c)
          a.xyz[c] = o[c] + 0.96 * (0.3338 * u[c] + 0.9426 * w[c]);
        out.push_back(a);
      }
    }
  }
  return out;
}
