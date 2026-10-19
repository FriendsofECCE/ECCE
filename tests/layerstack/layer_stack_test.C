// Layer builder geometry (tdat/LayerStack.H), checked against published or
// independently built structures:
//  1. AB graphite (P6_3/mmc: a = 2.464 A, c = 6.71 A, C at 2b (0,0,1/4),
//     (0,0,3/4) and 2c (1/3,2/3,1/4), (2/3,1/3,3/4); Trucano & Chen, Nature
//     258 (1975) 136, interlayer spacing 3.355 A).
//  2. A montmorillonite stack against preem.gro, the system a MATLAB atom
//     toolbox script (buildsystem.m) wrote for the same two layers.  Those
//     files are not in the repository: this part is skipped when the
//     directory given by ECCE_LAYER_CLAY_DIR (or Andy's default) is absent.
//  3. Water and ion placement: molecule geometry, minimum distances,
//     repeatability for a seed, and the error for an impossible request.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "tdat/LayerStack.H"

using namespace layerstack;

static int failures = 0;

static void check(bool ok, const std::string& what)
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) failures++;
}

static void toFrac(const Cell c, const double* x, double* f)
{
  // Solve f*c = x with the 3x3 inverse.
  double det = c[0][0]*(c[1][1]*c[2][2]-c[1][2]*c[2][1])
             - c[0][1]*(c[1][0]*c[2][2]-c[1][2]*c[2][0])
             + c[0][2]*(c[1][0]*c[2][1]-c[1][1]*c[2][0]);
  double inv[3][3];
  inv[0][0] = (c[1][1]*c[2][2]-c[1][2]*c[2][1])/det;
  inv[0][1] = (c[0][2]*c[2][1]-c[0][1]*c[2][2])/det;
  inv[0][2] = (c[0][1]*c[1][2]-c[0][2]*c[1][1])/det;
  inv[1][0] = (c[1][2]*c[2][0]-c[1][0]*c[2][2])/det;
  inv[1][1] = (c[0][0]*c[2][2]-c[0][2]*c[2][0])/det;
  inv[1][2] = (c[0][2]*c[1][0]-c[0][0]*c[1][2])/det;
  inv[2][0] = (c[1][0]*c[2][1]-c[1][1]*c[2][0])/det;
  inv[2][1] = (c[0][1]*c[2][0]-c[0][0]*c[2][1])/det;
  inv[2][2] = (c[0][0]*c[1][1]-c[0][1]*c[1][0])/det;
  for (int j = 0; j < 3; j++)
    f[j] = x[0]*inv[0][j] + x[1]*inv[1][j] + x[2]*inv[2][j];
}

// Distance of two points under the cell's periodicity (nearest of the
// images around the fractionally nearest one).
static double pbcDist(const Cell c, const double* p, const double* q)
{
  double d[3] = {p[0]-q[0], p[1]-q[1], p[2]-q[2]}, f[3];
  toFrac(c, d, f);
  for (int k = 0; k < 3; k++) f[k] -= std::floor(f[k] + 0.5);
  double best = 1e300;
  for (int a = -1; a <= 1; a++)
    for (int b = -1; b <= 1; b++)
      for (int e = -1; e <= 1; e++) {
        double g[3] = {f[0]+a, f[1]+b, f[2]+e}, r = 0;
        for (int k = 0; k < 3; k++) {
          double x = g[0]*c[0][k] + g[1]*c[1][k] + g[2]*c[2][k];
          r += x*x;
        }
        best = std::min(best, std::sqrt(r));
      }
  return best;
}

static Atom mk(const char* s, double x, double y, double z)
{
  Atom a;
  a.symbol = s;
  a.xyz[0] = x; a.xyz[1] = y; a.xyz[2] = z;
  return a;
}

static void graphene(std::vector<Atom>& layer, Cell cell, double z)
{
  const double a = 2.464, h = a * std::sqrt(3.0) / 2;
  double v[3][3] = {{a, 0, 0}, {-a/2, h, 0}, {0, 0, 10.0}};
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) cell[i][j] = v[i][j];
  layer.clear();
  layer.push_back(mk("C", 0, 0, z));
  // (1/3, 2/3) in fractional coordinates
  layer.push_back(mk("C", a/3 + (-a/2)*(2.0/3), h*(2.0/3), z));
}

static void testGraphite()
{
  Cell cell;
  std::vector<Atom> layer, out;
  graphene(layer, cell, 0.25 * 6.71);
  Options opt;
  opt.nLayers = 2;
  opt.spacing = 3.355;
  opt.shiftA = 2.0/3;
  opt.shiftB = 1.0/3;
  Cell nc;
  std::string err;
  check(buildLayerStack(layer, cell, opt, out, nc, &err), "graphite builds " + err);

  double clen = std::sqrt(nc[2][0]*nc[2][0] + nc[2][1]*nc[2][1] + nc[2][2]*nc[2][2]);
  printf("     graphite c = %.4f A (published 6.71)\n", clen);
  check(std::fabs(clen - 6.71) < 1e-9, "c = 2 x 3.355 = 6.71 A");
  check(std::fabs(nc[0][0] - 2.464) < 1e-12 && nc[2][0] == 0 && nc[2][1] == 0,
        "a, b kept, c along the normal (no tilt in, none out)");

  // Bernal positions, fractional (x,y,z)
  const double ref[4][3] = {{0, 0, 0.25}, {1.0/3, 2.0/3, 0.25},
                            {0, 0, 0.75}, {2.0/3, 1.0/3, 0.75}};
  check(out.size() == 4, "four carbons");
  double worst = 0;
  for (int r = 0; r < 4; r++) {
    double best = 1e300;
    for (size_t i = 0; i < out.size(); i++) {
      double f[3];
      toFrac(nc, out[i].xyz, f);
      double d2 = 0;
      for (int k = 0; k < 3; k++) {
        double d = f[k] - ref[r][k];
        d -= std::floor(d + 0.5);
        d2 += d*d;
      }
      best = std::min(best, std::sqrt(d2));
    }
    worst = std::max(worst, best);
  }
  printf("     largest fractional deviation from Bernal positions %.2e\n", worst);
  check(worst < 1e-9, "atoms at the Bernal AB positions");

  // AA (no shift) must not match: the check is not vacuous.
  opt.shiftA = opt.shiftB = 0;
  std::vector<Atom> aa;
  Cell nc2;
  buildLayerStack(layer, cell, opt, aa, nc2, &err);
  double f[3];
  toFrac(nc2, aa[3].xyz, f);
  double d = f[0] - 2.0/3;
  d -= std::floor(d + 0.5);
  check(std::fabs(d) > 0.1, "AA stacking differs from AB");
}

static bool readGro(const std::string& file, std::vector<Atom>& atoms,
                    std::vector<std::string>& names, double box[9])
{
  std::ifstream in(file);
  if (!in) return false;
  std::string line;
  std::getline(in, line);
  std::getline(in, line);
  int n = atoi(line.c_str());
  for (int i = 0; i < n; i++) {
    std::getline(in, line);
    Atom a;
    a.symbol = "X";
    for (int k = 0; k < 3; k++)
      a.xyz[k] = 10.0 * atof(line.substr(20 + 8*k, 8).c_str());
    names.push_back(line.substr(10, 5));
    atoms.push_back(a);
  }
  std::getline(in, line);
  std::istringstream ss(line);
  for (int k = 0; k < 9; k++) box[k] = 0;
  for (int k = 0; k < 9 && (ss >> box[k]); k++) box[k] *= 10.0;
  return true;
}

static void testClay()
{
  const char* env = getenv("ECCE_LAYER_CLAY_DIR");
  std::string dir = env ? env :
    "/home/andy/tmp/Andy/0_MD_gromacs_minff/minff_MD/gmx_minff/buildscripts";
  std::vector<Atom> l1, pre;
  std::vector<std::string> n1, np;
  double b1[9], bp[9];
  if (!readGro(dir + "/MMT_minff_1.gro", l1, n1, b1) ||
      !readGro(dir + "/preem.gro", pre, np, bp)) {
    printf("skip clay: %s not present\n", dir.c_str());
    return;
  }
  // .gro box: v1(x) v2(y) v3(z) v1(y) v1(z) v2(x) v2(z) v3(x) v3(y)
  Cell cell = {{b1[0], b1[3], b1[4]}, {b1[5], b1[1], b1[6]},
               {b1[7], b1[8], b1[2]}};
  // buildsystem.m: d001 = 18.9*1.1, second layer displaced by
  // [5.2/3 9/3 d001], box [30.96 35.8633 2*d001].
  const double d001 = 18.9 * 1.1, sx = 5.2/3, sy = 9.0/3;
  double sb = sy / cell[1][1];
  double sa = (sx - sb * cell[1][0]) / cell[0][0];
  // The script's layer 1 sits at (2.56, 1.95, 0), not at 0,0,0 (its
  // import step moves it); a rigid translation of the whole system, taken
  // from the first atom.
  std::vector<Atom> pmmt;
  // Atoms of the MMT residue only: the first 1920 entries of preem.gro.
  pmmt.assign(pre.begin(), pre.begin() + 2 * l1.size());
  double off[3];
  for (int k = 0; k < 3; k++) off[k] = pmmt[0].xyz[k] - l1[0].xyz[k];
  for (size_t i = 0; i < l1.size(); i++)
    for (int k = 0; k < 3; k++) l1[i].xyz[k] += off[k];

  Options opt;
  opt.nLayers = 2;
  opt.spacing = d001;
  opt.shiftA = sa;
  opt.shiftB = sb;
  std::vector<Atom> out;
  Cell nc;
  std::string err;
  check(buildLayerStack(l1, cell, opt, out, nc, &err), "clay stack builds " + err);
  printf("     new c = (%.4f %.4f %.4f); script box c = %.4f\n",
         nc[2][0], nc[2][1], nc[2][2], 2 * d001);
  check(std::fabs(nc[2][2] - 2 * d001) < 1e-9, "c' height = 2 x d001");

  // The script wraps into an orthogonal box, the stack into the tilted
  // cell: the same atom may differ by lattice vectors of either cell.  An
  // atom matches when mine - preem = n1.(a,b,c') - n2.(A,B,C) for small
  // integers; with no wrapping on either side both are zero.
  Cell oc = {{30.96, 0, 0}, {0, 35.8633, 0}, {0, 0, 2 * d001}};
  int n[2] = {0, 0}, plain[2] = {0, 0}, explained[2] = {0, 0};
  double sum[2] = {0, 0}, rms[2] = {0, 0};
  for (size_t i = 0; i < out.size(); i++) {
    int L = i < l1.size() ? 0 : 1;
    double d[3];
    for (int k = 0; k < 3; k++) d[k] = out[i].xyz[k] - pmmt[i].xyz[k];
    n[L]++;
    sum[L] += d[0]*d[0] + d[1]*d[1] + d[2]*d[2];
    if (std::sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]) < 0.02) plain[L]++;
    bool found = false;
    double best = 1e300;
    for (int m1 = 0; m1 < 27; m1++)
      for (int m2 = 0; m2 < 27; m2++) {
        int a1 = m1 % 3 - 1, b1_ = m1 / 3 % 3 - 1, c1 = m1 / 9 - 1;
        int a2 = m2 % 3 - 1, b2 = m2 / 3 % 3 - 1, c2 = m2 / 9 - 1;
        double r2 = 0;
        for (int k = 0; k < 3; k++) {
          double e = d[k] - (a1*nc[0][k] + b1_*nc[1][k] + c1*nc[2][k])
                          + (a2*oc[0][k] + b2*oc[1][k] + c2*oc[2][k]);
          r2 += e*e;
        }
        best = std::min(best, std::sqrt(r2));
      }
    found = best < 0.02;
    rms[L] += best*best;
    if (found) explained[L]++;
  }
  for (int L = 0; L < 2; L++)
    printf("     layer %d: %d atoms; %d equal to preem.gro within 0.02 A as "
           "wrapped, %d modulo the lattice vectors of the two cells; RMSD "
           "modulo lattice vectors %.4f A (plain RMSD %.3f A, from atoms "
           "wrapped differently)\n",
           L + 1, n[L], plain[L], explained[L], std::sqrt(rms[L]/n[L]),
           std::sqrt(sum[L]/n[L]));
  // .gro has 0.01 A resolution
  check(explained[0] == n[0], "layer 1 matches preem.gro (modulo wrapping)");
  check(explained[1] == n[1], "layer 2 (d001 and [5.2/3 3] shift) matches preem.gro (modulo wrapping)");
}

static void testFill()
{
  Cell cell;
  std::vector<Atom> layer, out, out2, out3;
  graphene(layer, cell, 0.0);
  // 4 x 4 cells so the interlayer is large enough for the particles
  double big[3][3];
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) big[i][j] = cell[i][j] * (i < 2 ? 4 : 1);
  std::vector<Atom> sheet;
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      for (size_t k = 0; k < layer.size(); k++) {
        Atom a = layer[k];
        for (int c = 0; c < 3; c++)
          a.xyz[c] += i * cell[0][c] + j * cell[1][c];
        sheet.push_back(a);
      }
  Options opt;
  opt.nLayers = 3;
  opt.spacing = 12.0;
  opt.shiftA = 1.0/3;
  opt.shiftB = 1.0/3;
  opt.waterPerInterlayer = 12;
  opt.waterMinDist = 2.5;
  Ion na;
  na.symbol = "Na";
  na.count = 3;
  na.charge = 1.0;
  na.minDist = 3.5;
  opt.ions.push_back(na);
  opt.seed = 7;
  Cell nc;
  std::string err;
  Cell bigc;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) bigc[i][j] = big[i][j];
  check(buildLayerStack(sheet, bigc, opt, out, nc, &err), "fill builds " + err);
  size_t nl = sheet.size() * 3;
  check(out.size() == nl + 3 * (3 + 3 * 12), "atom count: layers + ions + 3 per water");

  int ions = 0, ow = 0, hw = 0;
  bool inside = true;
  for (size_t i = 0; i < out.size(); i++) {
    if (out[i].kind == ION) { ions++; check(out[i].charge == 1.0, "ion charge kept"); }
    if (out[i].kind == WATER_O) ow++;
    if (out[i].kind == WATER_H) hw++;
    double f[3];
    toFrac(nc, out[i].xyz, f);
    // a water's H follow its O and may stick out of the cell
    if (out[i].kind != WATER_H)
      for (int k = 0; k < 3; k++)
        if (f[k] < -1e-9 || f[k] >= 1 + 1e-9) inside = false;
  }
  check(ions == 9 && ow == 36 && hw == 72, "9 ions, 36 waters");
  check(inside, "all atoms wrapped into the cell");

  bool geom = true;
  double minWater = 1e300, minIon = 1e300;
  for (size_t i = nl; i < out.size(); i++) {
    if (out[i].kind == WATER_O) {
      const Atom &o = out[i], &h1 = out[i+1], &h2 = out[i+2];
      double a[3], b[3], da = 0, db = 0, ab = 0;
      for (int k = 0; k < 3; k++) {
        a[k] = h1.xyz[k] - o.xyz[k];
        b[k] = h2.xyz[k] - o.xyz[k];
        da += a[k]*a[k]; db += b[k]*b[k]; ab += a[k]*b[k];
      }
      // after wrapping the three atoms may be in different images
      double ang = std::acos(ab / std::sqrt(da*db)) * 180 / M_PI;
      if (std::fabs(std::sqrt(da) - 0.9572) > 1e-6 || std::fabs(ang - 104.52) > 1e-4)
        geom = false;
    }
    if (out[i].kind == ION || out[i].kind == WATER_O) {
      double lim = out[i].kind == ION ? 3.5 : 2.5;
      double m = 1e300;
      // each particle keeps its distance from what was there before it
      for (size_t j = 0; j < i; j++) {
        m = std::min(m, pbcDist(nc, out[i].xyz, out[j].xyz));
      }
      (out[i].kind == ION ? minIon : minWater) = std::min(
          out[i].kind == ION ? minIon : minWater, m);
      (void)lim;
    }
  }
  printf("     closest water O to anything %.3f A (asked 2.5), closest ion %.3f A (asked 3.5)\n",
         minWater, minIon);
  check(geom, "every water has O-H 0.9572 A and H-O-H 104.52 deg");
  check(minWater >= 2.5 - 1e-9, "water minimum distance kept");
  check(minIon >= 3.5 - 1e-9, "ion minimum distance kept");

  check(buildLayerStack(sheet, bigc, opt, out2, nc, &err), "second build");
  bool same = out.size() == out2.size();
  for (size_t i = 0; same && i < out.size(); i++)
    for (int k = 0; k < 3; k++)
      if (out[i].xyz[k] != out2[i].xyz[k]) same = false;
  check(same, "same seed gives the same stack");
  opt.seed = 8;
  buildLayerStack(sheet, bigc, opt, out3, nc, &err);
  bool diff = false;
  for (size_t i = nl; i < out3.size(); i++)
    if (out3[i].xyz[0] != out[i].xyz[0]) diff = true;
  check(diff, "another seed gives another placement");

  opt.waterPerInterlayer = 5000;
  err.clear();
  bool built = buildLayerStack(sheet, bigc, opt, out3, nc, &err);
  check(!built && !err.empty(), "an impossible request reports an error: " + err);
}

int main()
{
  testGraphite();
  testClay();
  testFill();
  printf("%s\n", failures ? "FAILED" : "all passed");
  return failures ? 1 : 0;
}
