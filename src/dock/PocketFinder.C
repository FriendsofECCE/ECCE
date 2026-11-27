#include "dock/PocketFinder.H"

#include "tdat/Fragment.H"
#include "tdat/LigandSelect.H"
#include "tdat/LinkCellList.H"
#include "tdat/Residue.H"
#include "tdat/TAtm.H"
#include "tdat/TPerTab.H"

#include <algorithm>
#include <cmath>
#include <set>
#include <thread>

using std::vector;

namespace PocketFinder {

namespace {

struct Grid {
  int nx, ny, nz;
  double ox, oy, oz, h;
  size_t size() const { return (size_t)nx * ny * nz; }
  size_t idx(int x, int y, int z) const { return ((size_t)z * ny + y) * nx + x; }
  double X(int i) const { return ox + i * h; }
  double Y(int i) const { return oy + i * h; }
  double Z(int i) const { return oz + i * h; }
};

template <class F>
void parallelSlabs(int n, int threads, F f)
{
  threads = std::max(1, std::min(threads, n));
  vector<std::thread> pool;
  for (int t = 0; t < threads; t++)
    pool.emplace_back([=] { for (int k = t; k < n; k += threads) f(k); });
  for (auto& th : pool) th.join();
}

// Spatial lookup over a fixed atom set.  LinkCellList does no clamping, so
// query points are clamped into the atom bounding box first; the exact
// distance test is done on the true point.
struct Lookup {
  vector<TAtm*> atoms;
  vector<double> radius;
  double lo[3], hi[3];
  LinkCellList *cells;

  Lookup(const vector<TAtm*>& a, const vector<double>& r, double cutoff)
    : atoms(a), radius(r), cells(nullptr)
  {
    for (int k = 0; k < 3; k++) { lo[k] = 1e300; hi[k] = -1e300; }
    for (TAtm *t : atoms) {
      const double *c = t->coordinates();
      for (int k = 0; k < 3; k++) { lo[k] = std::min(lo[k], c[k]); hi[k] = std::max(hi[k], c[k]); }
    }
    cells = new LinkCellList(&atoms, cutoff);
  }
  ~Lookup() { delete cells; }

  // Calls f(atomIndex) for every atom in the cells around p.
  template <class F> void near(const double p[3], F f) const
  {
    double q[3];
    for (int k = 0; k < 3; k++) q[k] = std::min(std::max(p[k], lo[k]), hi[k]);
    int ix = cells->xIndex(q[0]), iy = cells->yIndex(q[1]), iz = cells->zIndex(q[2]);
    const int *head = cells->getHeader(), *link = cells->getLinkCellList();
    for (int kx = cells->ixMin(ix); kx <= cells->ixMax(ix); kx++)
      for (int ky = cells->iyMin(iy); ky <= cells->iyMax(iy); ky++)
        for (int kz = cells->izMin(iz); kz <= cells->izMax(iz); kz++)
          for (int a = head[cells->getCellIndex(kx, ky, kz)]; a >= 0; a = link[a]) f(a);
  }
};

}  // namespace

Result findPockets(const Fragment& frag, const Options& opt)
{
  Result res;
  const double h = opt.spacing > 0.05 ? opt.spacing : 1.0;

  vector<TAtm*> *all = frag.atoms();
  vector<bool> skip(all->size(), false);
  vector<bool> isWater(all->size(), false);
  {
    for (size_t i = 0; i < all->size(); i++) {
      Residue *r = (*all)[i]->getResidue();
      if (r && LigandSelect::residueClass(r->name()) == LigandSelect::RES_WATER)
        isWater[i] = true;
    }
    if (opt.ignoreLigands) {
      for (const LigandSelect::Candidate& c : LigandSelect::ligandCandidates(frag))
        if (c.cls == LigandSelect::RES_OTHER || c.cls == LigandSelect::RES_ADDITIVE)
          for (int a : c.atomIndices) if (a >= 0 && a < (int)skip.size()) skip[a] = true;
    }
  }

  TPerTab tpt;
  vector<TAtm*> atoms;
  vector<double> radius;
  std::set<int> fallbackZ;
  for (size_t i = 0; i < all->size(); i++) {
    if (opt.ignoreWater && isWater[i]) { res.atomsIgnoredWater++; continue; }
    if (skip[i] && !isWater[i]) { res.atomsIgnoredLigand++; continue; }
    TAtm *t = (*all)[i];
    int z = t->atomicNumber();
    double r = z > 0 ? tpt.vwr(z) : 0.0;
    if (!(r > 0.0)) {
      double c = z > 0 ? tpt.covalentRadius(z) : 0.0;
      if (c > 0.0) { r = c + 0.8; res.radiusFromCovalent++; }
      else { r = 1.8; res.radiusFromDefault++; }
      fallbackZ.insert(z);
    }
    atoms.push_back(t);
    radius.push_back(r);
  }
  delete all;
  res.fallbackElements.assign(fallbackZ.begin(), fallbackZ.end());
  res.atomsUsed = (int)atoms.size();
  if (atoms.empty()) return res;

  int threads = opt.threads > 0 ? opt.threads : (int)std::thread::hardware_concurrency();
  if (threads < 1) threads = 1;

  double rmax = *std::max_element(radius.begin(), radius.end());
  Lookup atomLookup(atoms, radius, rmax + opt.probe);

  Grid g;
  g.h = h;
  g.ox = atomLookup.lo[0] - opt.margin;
  g.oy = atomLookup.lo[1] - opt.margin;
  g.oz = atomLookup.lo[2] - opt.margin;
  g.nx = (int)std::ceil((atomLookup.hi[0] - atomLookup.lo[0] + 2 * opt.margin) / h) + 1;
  g.ny = (int)std::ceil((atomLookup.hi[1] - atomLookup.lo[1] + 2 * opt.margin) / h) + 1;
  g.nz = (int)std::ceil((atomLookup.hi[2] - atomLookup.lo[2] + 2 * opt.margin) / h) + 1;

  // 1. protein mask: 1 inside an atom, 2 closer to an atom than a probe fits.
  // Without the probe test, the voids between atoms percolate into one
  // protein-sized "pocket" on a 1 A grid.
  vector<char> prot(g.size(), 0);
  parallelSlabs(g.nz, threads, [&](int iz) {
    for (int iy = 0; iy < g.ny; iy++)
      for (int ix = 0; ix < g.nx; ix++) {
        double p[3] = { g.X(ix), g.Y(iy), g.Z(iz) };
        char m = 0;
        atomLookup.near(p, [&](int a) {
          if (m == 1) return;
          const double *c = atomLookup.atoms[a]->coordinates();
          double dx = p[0]-c[0], dy = p[1]-c[1], dz = p[2]-c[2];
          double d2 = dx*dx + dy*dy + dz*dz, r = atomLookup.radius[a], rp = r + opt.probe;
          if (d2 < r*r) m = 1;
          else if (d2 < rp*rp) m = 2;
        });
        prot[g.idx(ix, iy, iz)] = m;
      }
  });

  // 2. enclosure: directions closed by protein on both sides
  static const int D[7][3] = { {1,0,0},{0,1,0},{0,0,1},{1,1,1},{1,1,-1},{1,-1,1},{-1,1,1} };
  vector<unsigned char> score(g.size(), 0);
  parallelSlabs(g.nz, threads, [&](int iz) {
    for (int iy = 0; iy < g.ny; iy++)
      for (int ix = 0; ix < g.nx; ix++) {
        if (prot[g.idx(ix, iy, iz)] == 1) continue;
        int s = 0;
        for (const auto& d : D) {
          double len = std::sqrt((double)(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]));
          int steps = (int)(opt.maxRay / (h * len));
          bool closed[2] = { false, false };
          for (int side = 0; side < 2; side++) {
            int sg = side ? -1 : 1;
            for (int k = 1; k <= steps; k++) {
              int x = ix + sg*k*d[0], y = iy + sg*k*d[1], z = iz + sg*k*d[2];
              if (x < 0 || y < 0 || z < 0 || x >= g.nx || y >= g.ny || z >= g.nz) break;
              if (prot[g.idx(x, y, z)] == 1) { closed[side] = true; break; }
            }
          }
          if (closed[0] && closed[1]) s++;
        }
        score[g.idx(ix, iy, iz)] = (unsigned char)s;
      }
  });

  // 3. clusters of enclosed free points (26-connected)
  vector<char> seen(g.size(), 0);
  vector<Pocket> found;
  vector<vector<size_t>> members;
  for (int iz = 0; iz < g.nz; iz++)
    for (int iy = 0; iy < g.ny; iy++)
      for (int ix = 0; ix < g.nx; ix++) {
        size_t i0 = g.idx(ix, iy, iz);
        if (seen[i0] || prot[i0] || score[i0] < opt.minScore) continue;
        vector<size_t> stack(1, i0), pts;
        seen[i0] = 1;
        while (!stack.empty()) {
          size_t i = stack.back(); stack.pop_back();
          pts.push_back(i);
          int x = (int)(i % g.nx), y = (int)((i / g.nx) % g.ny), z = (int)(i / ((size_t)g.nx * g.ny));
          for (int dz = -1; dz <= 1; dz++) for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
            int a = x+dx, b = y+dy, c = z+dz;
            if (a < 0 || b < 0 || c < 0 || a >= g.nx || b >= g.ny || c >= g.nz) continue;
            size_t j = g.idx(a, b, c);
            if (seen[j] || prot[j] || score[j] < opt.minScore) continue;
            seen[j] = 1;
            stack.push_back(j);
          }
        }
        double vol = pts.size() * h * h * h;
        if (vol < opt.minVolume) continue;
        Pocket p;
        p.points = (int)pts.size();
        p.volume = vol;
        double sum = 0, esum = 0;
        for (int k = 0; k < 3; k++) { p.lo[k] = 1e300; p.hi[k] = -1e300; }
        for (size_t i : pts) {
          int x = (int)(i % g.nx), y = (int)((i / g.nx) % g.ny), z = (int)(i / ((size_t)g.nx * g.ny));
          std::array<double,3> c = { g.X(x), g.Y(y), g.Z(z) };
          p.pointCoords.push_back(c);
          for (int k = 0; k < 3; k++) {
            p.centre[k] += c[k];
            p.lo[k] = std::min(p.lo[k], c[k]);
            p.hi[k] = std::max(p.hi[k], c[k]);
          }
          sum += score[i];
        }
        esum = sum;
        for (int k = 0; k < 3; k++) p.centre[k] /= pts.size();
        p.meanEnclosure = esum / pts.size();
        p.score = sum * h * h * h / 7.0;
        double best = 1e300;
        for (const auto& c : p.pointCoords) {
          double d = 0;
          for (int k = 0; k < 3; k++) d += (c[k]-p.centre[k])*(c[k]-p.centre[k]);
          if (d < best) { best = d; p.anchor = c; }
        }
        found.push_back(std::move(p));
      }
  std::sort(found.begin(), found.end(),
            [](const Pocket& a, const Pocket& b) { return a.score > b.score; });

  // 4. lining residues: atoms within liningDistance of any pocket point
  if (!found.empty()) {
    Lookup lin(atoms, radius, std::max(opt.liningDistance, 1.0));
    double d2 = opt.liningDistance * opt.liningDistance;
    for (Pocket& p : found) {
      std::set<const Residue*> seenRes;
      vector<const Residue*> order;
      for (const auto& c : p.pointCoords)
        lin.near(c.data(), [&](int a) {
          const double *q = lin.atoms[a]->coordinates();
          double dx = c[0]-q[0], dy = c[1]-q[1], dz = c[2]-q[2];
          if (dx*dx + dy*dy + dz*dz > d2) return;
          const Residue *r = lin.atoms[a]->getResidue();
          if (r && seenRes.insert(r).second) order.push_back(r);
        });
      for (const Residue *r : order) {
        LiningResidue l;
        l.name = r->name();
        while (!l.name.empty() && l.name.back() == ' ') l.name.pop_back();  // the reader pads to 4
        l.number = r->number(); l.chain = r->chain();
        p.lining.push_back(l);
      }
      std::sort(p.lining.begin(), p.lining.end(),
                [](const LiningResidue& a, const LiningResidue& b) {
                  return a.chain != b.chain ? a.chain < b.chain : a.number < b.number; });
    }
  }
  res.pockets = std::move(found);
  return res;
}

}  // namespace PocketFinder
