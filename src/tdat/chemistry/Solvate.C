#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <dirent.h>

#include "tdat/Solvate.H"
#include "tdat/TPerTab.H"

using std::string;
using std::vector;

namespace solvate {

namespace {

// Element from an atom type: two letters when they name an element (Cl),
// else the first letter (OW -> O, HW -> H).
string elementOf(const string& type, TPerTab& tpt)
{
  if (type.size() >= 2) {
    string two = type.substr(0, 1) + (char)tolower(type[1]);
    if (tpt.isValid(two)) return two;
  }
  return type.substr(0, 1);
}

bool invert(const Cell h, Cell inv)
{
  const double det =
      h[0][0]*(h[1][1]*h[2][2] - h[1][2]*h[2][1]) -
      h[0][1]*(h[1][0]*h[2][2] - h[1][2]*h[2][0]) +
      h[0][2]*(h[1][0]*h[2][1] - h[1][1]*h[2][0]);
  if (!(std::fabs(det) > 1.0e-6)) return false;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      const int i1 = (j+1)%3, i2 = (j+2)%3, j1 = (i+1)%3, j2 = (i+2)%3;
      inv[i][j] = (h[i1][j1]*h[i2][j2] - h[i1][j2]*h[i2][j1]) / det;
    }
  return true;
}

// Row vector times matrix: fractional = x * inv, Cartesian = f * h.
Vec times(const Vec& x, const Cell m)
{
  Vec r = {{0, 0, 0}};
  for (int j = 0; j < 3; ++j)
    for (int k = 0; k < 3; ++k) r[j] += x[k] * m[k][j];
  return r;
}

// Periodic bins in fractional coordinates, each at least `width` wide, so
// every partner within `width` is in one of the 27 bins around an atom.
struct Grid {
  int n[3];
  std::map<long, vector<int> > bins;
  void init(const Cell h, const Cell inv, double width) {
    for (int k = 0; k < 3; ++k) {
      // Distance between the two faces normal to reciprocal vector k.
      const double g = std::sqrt(inv[0][k]*inv[0][k] + inv[1][k]*inv[1][k] +
                                 inv[2][k]*inv[2][k]);
      n[k] = std::max(1, (int)std::floor((1.0 / g) / width));
    }
    (void)h;
  }
  void binOf(const Vec& f, int b[3]) const {
    for (int k = 0; k < 3; ++k) {
      double u = f[k] - std::floor(f[k]);
      b[k] = std::min(n[k] - 1, (int)(u * n[k]));
    }
  }
  long key(int i, int j, int k) const {
    i = ((i % n[0]) + n[0]) % n[0];
    j = ((j % n[1]) + n[1]) % n[1];
    k = ((k % n[2]) + n[2]) % n[2];
    return ((long)i * n[1] + j) * n[2] + k;
  }
  void add(const Vec& f, int idx) {
    int b[3];
    binOf(f, b);
    bins[key(b[0], b[1], b[2])].push_back(idx);
  }
  // Distinct neighbouring bin keys (fewer than 27 when n < 3).
  void around(const Vec& f, vector<long>& keys) const {
    int b[3];
    binOf(f, b);
    keys.clear();
    for (int i = -1; i <= 1; ++i)
      for (int j = -1; j <= 1; ++j)
        for (int k = -1; k <= 1; ++k) {
          long kk = key(b[0]+i, b[1]+j, b[2]+k);
          if (std::find(keys.begin(), keys.end(), kk) == keys.end())
            keys.push_back(kk);
        }
  }
};

}  // namespace

bool readSolventBox(const string& path, SolventBox& box, string* err)
{
  std::ifstream in(path.c_str());
  string line;
  int natoms = 0, size = 0;
  if (!in || !std::getline(in, line) ||
      sscanf(line.c_str(), "%d %d", &natoms, &size) != 2 || natoms <= 0 ||
      size <= 0 || natoms % size != 0) {
    if (err) *err = "cannot read the atom and molecule counts of " + path;
    return false;
  }
  box = SolventBox();
  box.molSize = size;
  if (!std::getline(in, line) ||
      sscanf(line.c_str(), "%lf %lf %lf", &box.edge[0], &box.edge[1],
             &box.edge[2]) != 3 || box.edge[0] <= 0 || box.edge[1] <= 0 ||
      box.edge[2] <= 0) {
    if (err) *err = "cannot read the box of " + path;
    return false;
  }
  TPerTab tpt;
  for (int i = 0; i < natoms; ++i) {
    char type[32];
    Vec p;
    if (!std::getline(in, line) ||
        sscanf(line.c_str(), "%31s %lf %lf %lf", type, &p[0], &p[1], &p[2]) != 4) {
      if (err) *err = "too few atoms in " + path;
      return false;
    }
    box.types.push_back(type);
    box.symbols.push_back(elementOf(type, tpt));
    box.xyz.push_back(p);
  }
  // A molecule split by the template's own boundary is made whole around
  // its first atom.
  for (size_t m = 0; m < box.xyz.size(); m += size)
    for (int a = 1; a < size; ++a)
      for (int k = 0; k < 3; ++k) {
        double d = box.xyz[m+a][k] - box.xyz[m][k];
        box.xyz[m+a][k] -= box.edge[k] * std::round(d / box.edge[k]);
      }
  string stem = path.substr(path.find_last_of('/') + 1);
  box.name = stem.substr(0, stem.rfind(".xyz"));
  return true;
}

vector<string> solventNames(const string& dir)
{
  vector<string> names;
  DIR *d = opendir(dir.c_str());
  if (!d) return names;
  while (struct dirent *e = readdir(d)) {
    string f = e->d_name;
    if (f.size() > 4 && f.compare(f.size() - 4, 4, ".xyz") == 0)
      names.push_back(f.substr(0, f.size() - 4));
  }
  closedir(d);
  std::sort(names.begin(), names.end());
  return names;
}

void marginCell(const vector<Vec>& solute, double margin, Cell out, Vec& shift)
{
  Vec lo = {{0, 0, 0}}, hi = {{0, 0, 0}};
  for (size_t i = 0; i < solute.size(); ++i)
    for (int k = 0; k < 3; ++k) {
      if (i == 0 || solute[i][k] < lo[k]) lo[k] = solute[i][k];
      if (i == 0 || solute[i][k] > hi[k]) hi[k] = solute[i][k];
    }
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 3; ++c) out[r][c] = 0.0;
  for (int k = 0; k < 3; ++k) {
    out[k][k] = hi[k] - lo[k] + 2.0 * margin;
    shift[k] = 0.5 * out[k][k] - 0.5 * (lo[k] + hi[k]);
  }
}

bool fill(const vector<string>& soluteSymbols, const vector<Vec>& soluteXyz,
          const Cell cell, const SolventBox& box, const Options& opt,
          Result& out, string* err)
{
  out = Result();
  Cell inv;
  if (!invert(cell, inv)) {
    if (err) *err = "the cell has no volume";
    return false;
  }
  if (box.molSize <= 0 || box.xyz.empty()) {
    if (err) *err = "the solvent box is empty";
    return false;
  }
  TPerTab tpt;
  std::map<string, double> radius;
  double rmax = 0.0;
  auto rad = [&](const string& s) {
    std::map<string, double>::iterator it = radius.find(s);
    if (it != radius.end()) return it->second;
    int z = tpt.atomicNumber(s);
    double r = z > 0 ? tpt.vwr(z) : 1.5;
    if (!(r > 0.0)) r = 1.5;
    radius[s] = r;
    rmax = std::max(rmax, r);
    return r;
  };
  for (const string& s : soluteSymbols) rad(s);
  for (const string& s : box.symbols) rad(s);
  const double width = 2.0 * rmax * std::max(opt.scale, opt.solventScale);

  // Placed atoms: solute first, then kept solvent; fractional positions.
  vector<Vec> frac;
  vector<double> r;
  vector<bool> isSolute;
  Grid grid;
  grid.init(cell, inv, width);
  for (size_t i = 0; i < soluteXyz.size(); ++i) {
    frac.push_back(times(soluteXyz[i], inv));
    r.push_back(rad(soluteSymbols[i]));
    isSolute.push_back(true);
    grid.add(frac.back(), (int)frac.size() - 1);
  }

  // Template copies that cover the cell's bounding box.
  Vec lo = {{0, 0, 0}}, hi = {{0, 0, 0}};
  for (int c = 0; c < 8; ++c) {
    Vec f = {{(double)(c & 1), (double)((c >> 1) & 1), (double)((c >> 2) & 1)}};
    Vec x = times(f, cell);
    for (int k = 0; k < 3; ++k) {
      lo[k] = std::min(lo[k], x[k]);
      hi[k] = std::max(hi[k], x[k]);
    }
  }
  int nlo[3], nhi[3];
  for (int k = 0; k < 3; ++k) {
    nlo[k] = (int)std::floor(lo[k] / box.edge[k]) - 1;
    nhi[k] = (int)std::ceil(hi[k] / box.edge[k]);
  }
  const int nmol = box.molecules();
  vector<long> keys;
  for (int i = nlo[0]; i <= nhi[0]; ++i)
    for (int j = nlo[1]; j <= nhi[1]; ++j)
      for (int l = nlo[2]; l <= nhi[2]; ++l) {
        const Vec off = {{i * box.edge[0], j * box.edge[1], l * box.edge[2]}};
        for (int m = 0; m < nmol; ++m) {
          const int first = m * box.molSize;
          Vec cen = {{0, 0, 0}};
          vector<Vec> atoms(box.molSize);
          for (int a = 0; a < box.molSize; ++a)
            for (int k = 0; k < 3; ++k) {
              atoms[a][k] = box.xyz[first + a][k] + off[k];
              cen[k] += atoms[a][k] / box.molSize;
            }
          Vec fc = times(cen, inv);
          if (fc[0] < 0 || fc[0] >= 1 || fc[1] < 0 || fc[1] >= 1 ||
              fc[2] < 0 || fc[2] >= 1) continue;
          bool ok = true;
          vector<Vec> af(box.molSize);
          for (int a = 0; ok && a < box.molSize; ++a) {
            af[a] = times(atoms[a], inv);
            const double ra = rad(box.symbols[first + a]);
            grid.around(af[a], keys);
            for (size_t q = 0; ok && q < keys.size(); ++q) {
              std::map<long, vector<int> >::const_iterator b = grid.bins.find(keys[q]);
              if (b == grid.bins.end()) continue;
              for (int p : b->second) {
                Vec d;
                for (int k = 0; k < 3; ++k) {
                  d[k] = af[a][k] - frac[p][k];
                  d[k] -= std::round(d[k]);
                }
                Vec dx = times(d, cell);
                const double dist = std::sqrt(dx[0]*dx[0] + dx[1]*dx[1] + dx[2]*dx[2]);
                const double s = isSolute[p] ? opt.scale : opt.solventScale;
                if (dist < s * (ra + r[p])) { ok = false; break; }
              }
            }
          }
          if (!ok) continue;
          if ((int)(soluteXyz.size() + out.xyz.size()) + box.molSize > opt.maxAtoms) {
            if (err) {
              std::ostringstream m;
              m << "more than " << opt.maxAtoms << " atoms; use a smaller box";
              *err = m.str();
            }
            out = Result();
            return false;
          }
          for (int a = 0; a < box.molSize; ++a) {
            out.symbols.push_back(box.symbols[first + a]);
            out.types.push_back(box.types[first + a]);
            out.xyz.push_back(atoms[a]);
            frac.push_back(af[a]);
            r.push_back(rad(box.symbols[first + a]));
            isSolute.push_back(false);
            grid.add(af[a], (int)frac.size() - 1);
          }
          ++out.molecules;
        }
      }
  return true;
}

double density(const vector<string>& symbols, const Cell cell)
{
  TPerTab tpt;
  double mass = 0.0;
  for (const string& s : symbols) {
    int z = tpt.atomicNumber(s);
    if (z > 0) mass += tpt.atomicWeight(z);
  }
  const double vol = std::fabs(
      cell[0][0]*(cell[1][1]*cell[2][2] - cell[1][2]*cell[2][1]) -
      cell[0][1]*(cell[1][0]*cell[2][2] - cell[1][2]*cell[2][0]) +
      cell[0][2]*(cell[1][0]*cell[2][1] - cell[1][1]*cell[2][0]));
  // g/mol over N_A, per A^3 -> g/cm3
  return vol > 0 ? mass / 0.60221408 / vol : 0.0;
}

bool isWater(const SolventBox& box)
{
  return box.molSize == 3 && box.symbols.size() >= 3 &&
         box.symbols[0] == "O" && box.symbols[1] == "H" && box.symbols[2] == "H";
}

}  // namespace solvate
