#include "tdat/LayerStack.H"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

namespace layerstack {

namespace {

const double kPi = 3.14159265358979323846;
// Gas-phase water; the molecule's internal geometry is not an option.
const double kOH = 0.9572, kHOH = 104.52;
// An H of a placed water stays this far from every other atom, outside the
// covalent-bond cutoff, so that no bond to a layer atom is perceived.
const double kHMin = 1.5;

typedef double V3[3];

void cross(const double* a, const double* b, double* r)
{
  r[0] = a[1]*b[2] - a[2]*b[1];
  r[1] = a[2]*b[0] - a[0]*b[2];
  r[2] = a[0]*b[1] - a[1]*b[0];
}

double dot(const double* a, const double* b)
{
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

// Unit normal of the a-b plane on the side of c, and c's height along it.
bool normal(const Cell cell, double* n, double* height, std::string* err)
{
  cross(cell[0], cell[1], n);
  double len = std::sqrt(dot(n, n));
  if (len < 1e-8) {
    if (err) *err = "The cell vectors a and b are parallel.";
    return false;
  }
  for (int i = 0; i < 3; i++) n[i] /= len;
  double h = dot(n, cell[2]);
  if (h < 0.0) {
    for (int i = 0; i < 3; i++) n[i] = -n[i];
    h = -h;
  }
  if (h < 1e-8) {
    if (err) *err = "The cell vector c lies in the a-b plane.";
    return false;
  }
  if (height) *height = h;
  return true;
}

void toFrac(const Cell c, const double* x, double* f)
{
  V3 bc, ca, ab;
  cross(c[1], c[2], bc);
  cross(c[2], c[0], ca);
  cross(c[0], c[1], ab);
  double vol = dot(c[0], bc);
  f[0] = dot(x, bc) / vol;
  f[1] = dot(x, ca) / vol;
  f[2] = dot(x, ab) / vol;
}

void fromFrac(const Cell c, const double* f, double* x)
{
  for (int k = 0; k < 3; k++)
    x[k] = f[0]*c[0][k] + f[1]*c[1][k] + f[2]*c[2][k];
}

void wrapPoint(const Cell c, double* x)
{
  V3 f;
  toFrac(c, x, f);
  for (int k = 0; k < 3; k++) f[k] -= std::floor(f[k]);
  fromFrac(c, f, x);
}

// Add the images of x within `margin` of the height range [lo,hi].
void addImages(const Cell c, const double* n, const double* x, double lo,
               double hi, double margin, std::vector<double>& list)
{
  for (int ia = -1; ia <= 1; ia++)
    for (int ib = -1; ib <= 1; ib++)
      for (int ic = -1; ic <= 1; ic++) {
        V3 y;
        for (int k = 0; k < 3; k++)
          y[k] = x[k] + ia*c[0][k] + ib*c[1][k] + ic*c[2][k];
        double h = dot(y, n);
        if (h > lo - margin && h < hi + margin)
          list.insert(list.end(), y, y + 3);
      }
}

struct Rng {
  std::mt19937 gen;
  explicit Rng(unsigned s) : gen(s) {}
  // Own mapping to [0,1): std::uniform_real_distribution differs between
  // standard libraries and a seed must give the same stack everywhere.
  double next() { return gen() / 4294967296.0; }
};

}  // namespace

bool stackCell(const Cell cell, const Options& opt, Cell out, std::string* err)
{
  if (opt.nLayers < 1) {
    if (err) *err = "The number of layers must be at least 1.";
    return false;
  }
  if (opt.spacing <= 0.0) {
    if (err) *err = "The interlayer spacing must be positive.";
    return false;
  }
  V3 n;
  if (!normal(cell, n, 0, err)) return false;
  double cn = dot(cell[2], n);
  for (int k = 0; k < 3; k++) {
    out[0][k] = cell[0][k];
    out[1][k] = cell[1][k];
    out[2][k] = (cell[2][k] - cn*n[k]) + opt.nLayers * opt.spacing * n[k];
  }
  return true;
}

bool stackLayers(const std::vector<Atom>& layer, const Cell cell,
                 const Options& opt, std::vector<Atom>& out, std::string* err)
{
  Cell nc;
  if (!stackCell(cell, opt, nc, err)) return false;
  if (layer.empty()) {
    if (err) *err = "There are no atoms in the layer.";
    return false;
  }
  V3 n;
  double H;
  normal(cell, n, &H, 0);

  // A layer cut by the c boundary (its extent along n is larger than its
  // extent around the cell) is made one slab by lifting the low part by a
  // c vector; a layer that is already contiguous is left where it is.
  size_t m = layer.size();
  std::vector<double> h(m), mod(m);
  double hmin = 1e300, hmax = -1e300;
  for (size_t i = 0; i < m; i++) {
    h[i] = dot(layer[i].xyz, n);
    hmin = std::min(hmin, h[i]);
    hmax = std::max(hmax, h[i]);
    double r = std::fmod(h[i], H);
    mod[i] = r < 0 ? r + H : r;
  }
  std::vector<double> sorted(mod);
  std::sort(sorted.begin(), sorted.end());
  double gap = sorted[0] + H - sorted[m-1], start = sorted[0];
  for (size_t i = 1; i < m; i++) {
    if (sorted[i] - sorted[i-1] > gap) {
      gap = sorted[i] - sorted[i-1];
      start = sorted[i];
    }
  }
  bool lift = (hmax - hmin) > (H - gap) + 1e-6;

  out.clear();
  for (int i = 0; i < opt.nLayers; i++) {
    for (size_t k = 0; k < m; k++) {
      Atom a = layer[k];
      if (lift) {
        double q = std::floor((h[k] - start) / H);
        for (int j = 0; j < 3; j++) a.xyz[j] -= q * cell[2][j];
      }
      for (int j = 0; j < 3; j++)
        a.xyz[j] += i * (opt.shiftA*cell[0][j] + opt.shiftB*cell[1][j] +
                         opt.spacing*n[j]);
      out.push_back(a);
    }
  }
  return true;
}

void wrapIntoCell(std::vector<Atom>& atoms, const Cell cell)
{
  for (size_t i = 0; i < atoms.size(); i++) {
    V3 x = {atoms[i].xyz[0], atoms[i].xyz[1], atoms[i].xyz[2]};
    wrapPoint(cell, x);
    // A water moves as one unit, chosen by its O.
    size_t n = (atoms[i].kind == WATER_O && i + 2 < atoms.size()) ? 3 : 1;
    V3 shift = {x[0] - atoms[i].xyz[0], x[1] - atoms[i].xyz[1],
                x[2] - atoms[i].xyz[2]};
    for (size_t j = 0; j < n; j++)
      for (int k = 0; k < 3; k++) atoms[i+j].xyz[k] += shift[k];
    i += n - 1;
  }
}

bool fillInterlayers(const std::vector<Atom>& stack, const Cell newCell,
                     const Options& opt, std::vector<Atom>& added,
                     std::string* err)
{
  added.clear();
  int total = opt.waterPerInterlayer;
  for (size_t i = 0; i < opt.ions.size(); i++) total += opt.ions[i].count;
  if (total == 0) return true;

  V3 n;
  double Hnew;
  if (!normal(newCell, n, &Hnew, err)) return false;
  int N = opt.nLayers;
  size_t m = stack.size() / N;

  double margin = 1.5 + std::max(kHMin, opt.waterMinDist);
  for (size_t i = 0; i < opt.ions.size(); i++)
    margin = std::max(margin, 1.5 + opt.ions[i].minDist);

  std::vector<double> wrapped, placed;   // flat x,y,z
  for (size_t i = 0; i < stack.size(); i++) {
    V3 x = {stack[i].xyz[0], stack[i].xyz[1], stack[i].xyz[2]};
    wrapPoint(newCell, x);
    wrapped.insert(wrapped.end(), x, x + 3);
  }

  Rng rng(opt.seed);
  const int kTries = 20000;
  for (int j = 0; j < N; j++) {
    // Slab between the top of layer j and the bottom of the next layer
    // (the periodic image of layer 0 after the last one), from the
    // unwrapped stack.
    double lo = -1e300, hi = 1e300;
    int next = (j + 1) % N;
    for (size_t k = 0; k < m; k++) {
      lo = std::max(lo, dot(stack[j*m + k].xyz, n));
      hi = std::min(hi, dot(stack[next*m + k].xyz, n) +
                        (j + 1 == N ? Hnew : 0.0));
    }
    if (hi - lo < 0.1) {
      if (err) *err = "The interlayer spacing leaves no room between the "
                      "layers for ions or water.";
      return false;
    }

    std::vector<double> cand;
    for (int pass = 0; pass < 2; pass++) {
      const std::vector<double>& src = pass == 0 ? wrapped : placed;
      for (size_t i = 0; i + 2 < src.size(); i += 3)
        addImages(newCell, n, &src[i], lo, hi, margin, cand);
    }

    std::vector<const Ion*> queue;
    for (size_t i = 0; i < opt.ions.size(); i++)
      for (int k = 0; k < opt.ions[i].count; k++)
        queue.push_back(&opt.ions[i]);
    for (int k = 0; k < opt.waterPerInterlayer; k++) queue.push_back(0);

    for (size_t q = 0; q < queue.size(); q++) {
      const Ion* ion = queue[q];
      double minD = ion ? ion->minDist : opt.waterMinDist;
      int nat = ion ? 1 : 3;
      V3 pos[3];
      bool ok = false;
      for (int t = 0; t < kTries && !ok; t++) {
        double u = rng.next(), v = rng.next(), hs = rng.next();
        double hgt = lo + hs * (hi - lo);
        for (int k = 0; k < 3; k++)
          pos[0][k] = u*newCell[0][k] + v*newCell[1][k] + hgt*n[k];
        if (!ion) {
          double u1 = rng.next(), u2 = rng.next(), u3 = rng.next();
          double qa = std::sqrt(1-u1)*std::sin(2*kPi*u2);
          double qb = std::sqrt(1-u1)*std::cos(2*kPi*u2);
          double qc = std::sqrt(u1)*std::sin(2*kPi*u3);
          double qd = std::sqrt(u1)*std::cos(2*kPi*u3);
          double R[3][3] = {
            {1-2*(qc*qc+qd*qd), 2*(qb*qc-qa*qd), 2*(qb*qd+qa*qc)},
            {2*(qb*qc+qa*qd), 1-2*(qb*qb+qd*qd), 2*(qc*qd-qa*qb)},
            {2*(qb*qd-qa*qc), 2*(qc*qd+qa*qb), 1-2*(qb*qb+qc*qc)}};
          double half = kHOH * kPi / 360.0;
          double loc[2][3] = {{ kOH*std::sin(half), 0, kOH*std::cos(half)},
                              {-kOH*std::sin(half), 0, kOH*std::cos(half)}};
          for (int a = 0; a < 2; a++)
            for (int k = 0; k < 3; k++)
              pos[1+a][k] = pos[0][k] + R[k][0]*loc[a][0] +
                            R[k][1]*loc[a][1] + R[k][2]*loc[a][2];
        }
        ok = true;
        for (size_t i = 0; i + 2 < cand.size() && ok; i += 3) {
          for (int a = 0; a < nat && ok; a++) {
            double d2 = 0;
            for (int k = 0; k < 3; k++)
              d2 += (pos[a][k]-cand[i+k])*(pos[a][k]-cand[i+k]);
            double lim = a == 0 ? minD : kHMin;
            if (d2 < lim*lim) ok = false;
          }
        }
      }
      if (!ok) {
        if (err) {
          char buf[256];
          snprintf(buf, sizeof buf,
                   "Interlayer %d: no room for particle %d of %d with the "
                   "minimum distance %.2f A; lower the count or the distance, "
                   "or increase the spacing.", j + 1, (int)q + 1,
                   (int)queue.size(), minD);
          *err = buf;
        }
        return false;
      }

      for (int a = 0; a < nat; a++) {
        Atom at;
        for (int k = 0; k < 3; k++) at.xyz[k] = pos[a][k];
        if (ion) {
          at.symbol = ion->symbol;
          at.kind = ION;
          at.charge = ion->charge;
        } else {
          at.symbol = a == 0 ? "O" : "H";
          at.kind = a == 0 ? WATER_O : WATER_H;
        }
        added.push_back(at);
        V3 x = {pos[a][0], pos[a][1], pos[a][2]};
        wrapPoint(newCell, x);
        placed.insert(placed.end(), x, x + 3);
        addImages(newCell, n, x, lo, hi, margin, cand);
      }
    }
  }
  return true;
}

bool buildLayerStack(const std::vector<Atom>& layer, const Cell cell,
                     const Options& opt, std::vector<Atom>& out, Cell newCell,
                     std::string* err)
{
  if (!stackCell(cell, opt, newCell, err)) return false;
  std::vector<Atom> stack, added;
  if (!stackLayers(layer, cell, opt, stack, err)) return false;
  if (!fillInterlayers(stack, newCell, opt, added, err)) return false;
  out = stack;
  out.insert(out.end(), added.begin(), added.end());
  wrapIntoCell(out, newCell);
  return true;
}

}  // namespace layerstack
