// Multipole surfaces (#428): the angular function and mesh of a Cartesian
// multipole tensor, on model tensors with known symmetry and on parsed
// outputs (stored property files; the ORCA output text).
#include "util/Multipole.H"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using std::vector;

static int fails = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static vector<double> comp(int rank, const char *labels, const char *values)
{
  vector<string> l; vector<double> v;
  std::istringstream a(labels), b(values);
  string s; double d;
  while (a >> s) l.push_back(s);
  while (b >> d) v.push_back(d);
  vector<double> full;
  if (!Multipole::fromComponents(rank, l, v, full)) { printf("bad components\n"); exit(2); }
  return full;
}

static double F(int rank, const vector<double>& t, double x, double y, double z)
{
  double n[3] = {x, y, z};
  double l = sqrt(x*x + y*y + z*z);
  for (int i = 0; i < 3; i++) n[i] /= l;
  return Multipole::evaluate(rank, t, n);
}

static int findRoot(vector<int>& p, int a)
{
  while (p[a] != a) a = p[a] = p[p[a]];
  return a;
}

// Sign regions of the mesh: connected sets of vertices with |f| above a
// small fraction of the maximum and the same sign.
static void lobes(const Multipole::Mesh& m, int *npos, int *nneg)
{
  const size_t nv = m.f.size();
  float fmax = 0;
  for (size_t i = 0; i < nv; i++) fmax = std::max(fmax, fabsf(m.f[i]));
  const float eps = 0.05f*fmax;
  vector<int> p(nv);
  std::iota(p.begin(), p.end(), 0);
  for (size_t t = 0; t < m.tri.size(); t += 3)
    for (int e = 0; e < 3; e++) {
      int a = m.tri[t + e], b = m.tri[t + (e + 1) % 3];
      if (fabsf(m.f[a]) > eps && fabsf(m.f[b]) > eps &&
          (m.f[a] > 0) == (m.f[b] > 0))
        p[findRoot(p, a)] = findRoot(p, b);
    }
  vector<bool> seen(nv, false);
  *npos = *nneg = 0;
  for (size_t i = 0; i < nv; i++) {
    if (fabsf(m.f[i]) <= eps) continue;
    int r = findRoot(p, (int)i);
    if (!seen[r]) { seen[r] = true; (m.f[r] > 0 ? *npos : *nneg)++; }
  }
}

// Norm of the contraction of the first two indices.
static double traceNorm(int rank, const vector<double>& t)
{
  double s = 0;
  int rest = 1;
  for (int i = 2; i < rank; i++) rest *= 3;
  for (int r = 0; r < rest; r++) {
    double c = 0;
    for (int a = 0; a < 3; a++) c += t[(a*3 + a)*rest + r];
    s += c*c;
  }
  return sqrt(s);
}

static bool readStored(const string& file, int *rank, vector<double>& full)
{
  std::ifstream in(file.c_str());
  if (!in) return false;
  std::stringstream ss;
  ss << in.rdbuf();
  string s = ss.str();
  size_t a = s.find("rowLabels=\"");
  if (a == string::npos) return false;
  size_t b = s.find('"', a + 11), c = s.find('>', b), d = s.find("</vector>");
  if (d == string::npos) return false;
  vector<string> labels; vector<double> vals;
  std::istringstream ls(s.substr(a + 11, b - a - 11)), vs(s.substr(c + 1, d - c - 1));
  string w; double x;
  while (ls >> w) labels.push_back(w);
  while (vs >> x) vals.push_back(x);
  *rank = (int)labels[0].size();
  return Multipole::fromComponents(*rank, labels, vals, full);
}

int main()
{
  const string src = ECCE_SOURCE_DIR;

  // Axial quadrupole (CO2-like), given traced: diag(1,1,3) = traceless
  // diag(-2/3,-2/3,4/3) + 5/3 I.
  {
    vector<double> q = comp(2, "XX XY XZ YY YZ ZZ", "1 0 0 1 0 3");
    CHECK(traceNorm(2, q) > 1);
    vector<double> h = Multipole::harmonicPart(2, q);
    CHECK(traceNorm(2, h) < 1e-9);
    CHECK(fabs(h[0] + 2.0/3) < 1e-9 && fabs(h[8] - 4.0/3) < 1e-9);
    vector<double> h2 = Multipole::harmonicPart(2, h);
    for (size_t i = 0; i < h.size(); i++) CHECK(fabs(h[i] - h2[i]) < 1e-9);
    for (int k = 0; k < 24; k++) {
      double ph = 0.7*k, c = cos(ph), s = sin(ph), z = 0.3 + 0.1*k;
      CHECK(fabs(F(2, h, 1, 0.4, z) - F(2, h, c - 0.4*s, s + 0.4*c, z)) < 1e-9);
    }
    Multipole::Mesh m = Multipole::surface(2, h, 1.0, 4);
    int np, nn;
    lobes(m, &np, &nn);
    CHECK(np == 2 && nn == 1);   // caps along +-z, a belt of the other sign
    CHECK(m.xyz.size() == 3*2562 && m.tri.size() == 3*5120);
  }

  // Tetrahedral octupole: T_xyz only, f = 6 x y z.
  {
    vector<double> o = comp(3, "XXX YYY ZZZ XYY XXY XXZ XZZ YZZ YYZ XYZ",
                            "0 0 0 0 0 0 0 0 0 1");
    vector<double> h = Multipole::harmonicPart(3, o);
    for (size_t i = 0; i < o.size(); i++) CHECK(fabs(h[i] - o[i]) < 1e-9);
    CHECK(fabs(F(3, h, 1, 1, 1) - 6/(3*sqrt(3.0))) < 1e-9);
    for (int sx = -1; sx <= 1; sx += 2)
      for (int sy = -1; sy <= 1; sy += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
          double f = F(3, h, sx, sy, sz);
          CHECK((sx*sy*sz > 0) == (f > 0));
          CHECK(F(3, h, -sx, sy, sz)*f < 0);   // alternates across a face
        }
    Multipole::Mesh m = Multipole::surface(3, h, 1.0, 5);
    int np, nn;
    lobes(m, &np, &nn);
    CHECK(np == 4 && nn == 4);
  }

  // Cubic hexadecapole, traced: T_xxxx = T_yyyy = T_zzzz = 1 (f = x4+y4+z4).
  {
    vector<double> hx = comp(4,
        "XXXX YYYY ZZZZ XXXY XXXZ YYYX YYYZ ZZZX ZZZY XXYY XXZZ YYZZ XXYZ YYXZ ZZXY",
        "1 1 1 0 0 0 0 0 0 0 0 0 0 0 0");
    vector<double> h = Multipole::harmonicPart(4, hx);
    CHECK(traceNorm(4, h) < 1e-9);
    CHECK(fabs(F(4, h, 1, 0, 0) - 0.4) < 1e-9);                  // 1 - 3/5
    CHECK(fabs(F(4, h, 1, 1, 1) - (1.0/3 - 0.6)) < 1e-9);
    double a = 0.3, b = 0.5, c = 0.8, ref = F(4, h, a, b, c);
    double p[6][3] = {{a,b,c},{b,c,a},{c,a,b},{b,a,c},{a,c,b},{c,b,a}};
    for (int i = 0; i < 6; i++)
      for (int s = 0; s < 8; s++)
        CHECK(fabs(F(4, h, (s&1 ? -1 : 1)*p[i][0], (s&2 ? -1 : 1)*p[i][1],
                     (s&4 ? -1 : 1)*p[i][2]) - ref) < 1e-9);
    Multipole::Mesh m = Multipole::surface(4, h, 1.0, 5);
    int np, nn;
    lobes(m, &np, &nn);
    CHECK(np == 6 && nn == 1);   // six caps on the axes; the negative part is one connected belt
    CHECK(F(4, h, 1, 1, 1) < F(4, h, 1, 1, 0.3) && F(4, h, 1, 1, 1) < F(4, h, 1.3, 1, 1));  // minima on the diagonals
  }

  // A generic traced octupole: the harmonic part is traceless and idempotent.
  {
    vector<double> o = comp(3, "XXX YYY ZZZ XYY XXY XXZ XZZ YZZ YYZ XYZ",
                            "1.2 -0.7 0.4 0.9 -1.1 0.3 0.8 -0.2 0.6 0.5");
    vector<double> h = Multipole::harmonicPart(3, o);
    CHECK(traceNorm(3, o) > 0.5 && traceNorm(3, h) < 1e-9);
    vector<double> h2 = Multipole::harmonicPart(3, h);
    for (size_t i = 0; i < h.size(); i++) CHECK(fabs(h[i] - h2[i]) < 1e-9);
  }

  // Gaussian output as stored by the parsers (C2v water, C2 along y): each
  // moment is unchanged by the mirror planes x -> -x and z -> -z.
  const char *props[3] = {"QUADPOLE", "OCTAPOLE", "HEXPOLE"};
  for (int r = 0; r < 3; r++) {
    int rank = 0;
    vector<double> t;
    CHECK(readStored(src + "/tests/apps/fixtures/calc-water-vib/Props/" + props[r], &rank, t));
    CHECK(rank == r + 2);
    vector<double> h = Multipole::harmonicPart(rank, t);
    CHECK(Multipole::maxAbs(rank, h) > 0.1);
    for (int k = 0; k < 20; k++) {
      double x = 0.3 + 0.07*k, y = -0.6 + 0.05*k, z = 0.45 - 0.04*k;
      double v = F(rank, h, x, y, z);
      CHECK(fabs(F(rank, h, -x, y, z) - v) < 1e-4);
      CHECK(fabs(F(rank, h, x, y, -z) - v) < 1e-4);
    }
  }

  // NWChem-style stored quadrupole (lower-case labels): C2v water.
  {
    int rank = 0;
    vector<double> t;
    CHECK(readStored(src + "/tests/apps/fixtures/calc-water-opt/Props/QUADPOLE", &rank, t));
    vector<double> h = Multipole::harmonicPart(rank, t);
    double v = F(2, h, 0.3, 0.5, 0.7);
    CHECK(fabs(F(2, h, -0.3, 0.5, 0.7) - v) < 1e-9);
    CHECK(fabs(F(2, h, 0.3, -0.5, 0.7) - v) < 1e-9);
  }

  // ORCA output text: the "TOT" line (a.u.) of the last QUADRUPOLE MOMENT block.
  {
    std::ifstream in((src + "/tests/parsers/fixtures/orca/h2o_optfreq.out").c_str());
    string line;
    bool block = false;
    vector<double> v;
    while (std::getline(in, line)) {
      if (line.find("QUADRUPOLE MOMENT") != string::npos) { block = true; continue; }
      if (block && line.compare(0, 5, "  TOT") == 0) {
        std::istringstream s(line.substr(5));
        double x;
        v.clear();
        while (v.size() < 6 && (s >> x)) v.push_back(x);
        block = false;
      }
    }
    CHECK(v.size() == 6);
    if (v.size() == 6) {   // printed order: xx yy zz xy xz yz
      vector<string> l = {"XX", "YY", "ZZ", "XY", "XZ", "YZ"};
      vector<double> t;
      CHECK(Multipole::fromComponents(2, l, v, t));
      vector<double> h = Multipole::harmonicPart(2, t);
      CHECK(traceNorm(2, h) < 1e-9);
      CHECK(fabs((h[0] - h[8]) - (v[0] - v[2])) < 1e-5);
      CHECK(fabs((h[4] - h[8]) - (v[1] - v[2])) < 1e-5);
    }
  }

  // A missing component or a bad label is rejected.
  {
    vector<double> full;
    vector<string> l(1, "XX");
    vector<double> v(1, 1.0);
    CHECK(!Multipole::fromComponents(2, l, v, full));
    l[0] = "XQ";
    CHECK(!Multipole::fromComponents(2, l, v, full));
  }

  printf(fails ? "multipole_test: failures\n" : "multipole_test: all passed\n");
  return fails ? 1 : 0;
}
