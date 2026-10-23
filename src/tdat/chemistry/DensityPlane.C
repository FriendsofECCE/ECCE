// The density / one orbital on a plane (#215); see tdat/DensityPlane.H.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>

#include "tdat/DensityPlane.H"
#include "tdat/EspField.H"
#include "tdat/Fragment.H"
#include "tdat/MoAoBasis.H"
#include "tdat/PropVector.H"
#include "dsm/IPropCalculation.H"

namespace DensityPlane {

static const double BOHR_PER_ANGSTROM = 1.0 / 0.52917721092;

typedef double V3[3];

static void cross(const double *a, const double *b, double *r)
{
  r[0] = a[1]*b[2] - a[2]*b[1];
  r[1] = a[2]*b[0] - a[0]*b[2];
  r[2] = a[0]*b[1] - a[1]*b[0];
}
static double dot(const double *a, const double *b)
{
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
static bool normalise(double *a)
{
  const double l = sqrt(dot(a, a));
  if (l < 1.0e-8) return false;
  for (int k = 0; k < 3; k++) a[k] /= l;
  return true;
}

Plane::Plane() : rms(0.0)
{
  for (int k = 0; k < 3; k++) origin[k] = u[k] = v[k] = n[k] = 0.0;
}

Map::Map() : quantity(DENSITY), mo(0), beta(false), nu(0), nv(0), u0(0),
             v0(0), step(0.05), zmin(0), zmax(0) {}

//  Eigen-decomposition of the 3x3 covariance (Jacobi): the smallest
//  spread is the plane's normal, the largest its long axis.
static void principalAxes(const vector<double>& c, V3 centroid,
                          double evals[3], V3 evecs[3])
{
  const size_t n = c.size() / 3;
  for (int k = 0; k < 3; k++) {
    centroid[k] = 0;
    for (size_t a = 0; a < n; a++) centroid[k] += c[3*a + k];
    centroid[k] /= n;
  }
  double m[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
  for (size_t a = 0; a < n; a++)
    for (int i = 0; i < 3; i++)
      for (int j = 0; j < 3; j++)
        m[i][j] += (c[3*a+i]-centroid[i]) * (c[3*a+j]-centroid[j]) / n;
  double e[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
  for (int sweep = 0; sweep < 50; sweep++) {
    double off = fabs(m[0][1]) + fabs(m[0][2]) + fabs(m[1][2]);
    if (off < 1.0e-14) break;
    for (int p = 0; p < 2; p++)
      for (int q = p + 1; q < 3; q++) {
        if (fabs(m[p][q]) < 1.0e-18) continue;
        const double th = (m[q][q] - m[p][p]) / (2.0*m[p][q]);
        const double t = (th >= 0 ? 1.0 : -1.0) /
                         (fabs(th) + sqrt(th*th + 1.0));
        const double cs = 1.0 / sqrt(t*t + 1.0), sn = t*cs;
        for (int k = 0; k < 3; k++) {
          const double kp = m[k][p], kq = m[k][q];
          m[k][p] = cs*kp - sn*kq;  m[k][q] = sn*kp + cs*kq;
        }
        for (int k = 0; k < 3; k++) {
          const double pk = m[p][k], qk = m[q][k];
          m[p][k] = cs*pk - sn*qk;  m[q][k] = sn*pk + cs*qk;
        }
        for (int k = 0; k < 3; k++) {
          const double kp = e[k][p], kq = e[k][q];
          e[k][p] = cs*kp - sn*kq;  e[k][q] = sn*kp + cs*kq;
        }
      }
  }
  int order[3] = {0, 1, 2};
  std::sort(order, order + 3, [&](int a, int b) { return m[a][a] < m[b][b]; });
  for (int r = 0; r < 3; r++) {
    evals[r] = std::max(0.0, m[order[r]][order[r]]);
    for (int k = 0; k < 3; k++) evecs[r][k] = e[k][order[r]];
  }
}

bool isPlanar(const vector<double>& c, double tolerance)
{
  if (c.size() < 6) return false;
  V3 centroid, ev[3];
  double l[3];
  principalAxes(c, centroid, l, ev);
  return sqrt(l[0]) < tolerance;
}

bool makePlane(PlaneKind kind, const vector<double>& c,
               const vector<string>& labels, int a, int b, int cc,
               Plane& p, string& why)
{
  const int natoms = (int)(c.size() / 3);
  if (natoms < 2) { why = "A plane needs at least two atoms."; return false; }
  V3 centroid, ev[3];
  double lam[3];
  principalAxes(c, centroid, lam, ev);
  p.rms = sqrt(lam[0]);
  auto name = [&](int i) {
    return i >= 0 && i < (int)labels.size() ? labels[i] : string("?");
  };
  auto bad = [&](int i) { return i < 0 || i >= natoms; };

  if (kind == MOLECULAR) {
    const bool linear = sqrt(lam[1]) < 0.05;
    if (linear) {
      //  Any plane through the axis will do; take the one that holds the
      //  Cartesian axis least parallel to it, so that it is reproducible.
      double *axis = ev[2];
      int best = 0;
      for (int k = 1; k < 3; k++)
        if (fabs(axis[k]) < fabs(axis[best])) best = k;
      V3 e = {0,0,0}, w;
      e[best] = 1;
      const double d = dot(e, axis);
      for (int k = 0; k < 3; k++) w[k] = e[k] - d*axis[k];
      normalise(w);
      for (int k = 0; k < 3; k++) {
        p.origin[k] = centroid[k]; p.u[k] = axis[k]; p.v[k] = w[k];
      }
      cross(p.u, p.v, p.n);
      p.description = "plane through the molecular axis";
      return true;
    }
    if (p.rms >= 0.1) {
      char buf[160];
      snprintf(buf, sizeof buf, "The molecule is not planar (atoms scatter "
               "%.2f A about the best plane): choose three atoms.", p.rms);
      why = buf;
      return false;
    }
    for (int k = 0; k < 3; k++) {
      p.origin[k] = centroid[k]; p.u[k] = ev[2][k]; p.n[k] = ev[0][k];
    }
    cross(p.n, p.u, p.v);
    p.description = "molecular plane";
    return true;
  }

  if (kind == THREE_ATOMS) {
    if (bad(a) || bad(b) || bad(cc) || a == b || b == cc || a == cc) {
      why = "Choose three different atoms."; return false;
    }
    V3 ab, ac;
    for (int k = 0; k < 3; k++) {
      ab[k] = c[3*b+k] - c[3*a+k]; ac[k] = c[3*cc+k] - c[3*a+k];
    }
    V3 n;
    cross(ab, ac, n);
    if (!normalise(n) || !normalise(ab)) {
      why = "The three atoms lie on a line: they do not define a plane.";
      return false;
    }
    V3 v;
    cross(n, ab, v);
    if (dot(v, ac) < 0) { for (int k = 0; k < 3; k++) { v[k] = -v[k]; n[k] = -n[k]; } }
    for (int k = 0; k < 3; k++) {
      p.origin[k] = c[3*a+k]; p.u[k] = ab[k]; p.v[k] = v[k]; p.n[k] = n[k];
    }
    p.description = "plane through " + name(a) + ", " + name(b) + ", " + name(cc);
    return true;
  }

  //  AXIAL: contains a-b, perpendicular to the molecular plane.
  if (bad(a) || bad(b) || a == b) { why = "Choose two different atoms."; return false; }
  V3 u, ref;
  for (int k = 0; k < 3; k++) u[k] = c[3*b+k] - c[3*a+k];
  if (!normalise(u)) { why = "The two atoms coincide."; return false; }
  const bool linear = sqrt(lam[1]) < 0.05;
  if (linear || p.rms >= 0.1) {
    why = linear ? "A linear molecule has no molecular plane to stand on."
                 : "The molecule is not planar, so there is no plane to "
                   "stand on: choose three atoms.";
    return false;
  }
  for (int k = 0; k < 3; k++) ref[k] = ev[0][k];
  const double d = dot(ref, u);
  V3 v;
  for (int k = 0; k < 3; k++) v[k] = ref[k] - d*u[k];
  if (!normalise(v)) {
    why = "The two atoms lie along the molecular normal."; return false;
  }
  for (int k = 0; k < 3; k++) {
    p.origin[k] = 0.5*(c[3*a+k] + c[3*b+k]); p.u[k] = u[k]; p.v[k] = v[k];
  }
  cross(p.u, p.v, p.n);
  p.description = "plane through " + name(a) + "-" + name(b) +
                  ", perpendicular to the molecular plane";
  return true;
}


//----------------------------------------------------------------------
//  Contours

vector<Segment> contour(const Map& m, double level)
{
  vector<Segment> out;
  if (m.nu < 2 || m.nv < 2) return out;
  auto Z = [&](int i, int j) { return m.z[(size_t)j*m.nu + i]; };
  //  The crossing on an edge from (ua,va,za) to (ub,vb,zb).
  auto cut = [&](double ua, double va, double za, double ub, double vb,
                 double zb, double& u, double& v) {
    const double t = (level - za) / (zb - za);
    u = ua + t*(ub - ua);  v = va + t*(vb - va);
  };
  for (int j = 0; j + 1 < m.nv; j++)
    for (int i = 0; i + 1 < m.nu; i++) {
      const double z00 = Z(i, j), z10 = Z(i+1, j), z11 = Z(i+1, j+1),
                   z01 = Z(i, j+1);
      const int code = (z00 >= level ? 1 : 0) | (z10 >= level ? 2 : 0) |
                       (z11 >= level ? 4 : 0) | (z01 >= level ? 8 : 0);
      if (code == 0 || code == 15) continue;
      const double u0 = m.u(i), u1 = m.u(i+1), v0 = m.v(j), v1 = m.v(j+1);
      double e[4][2];   // bottom, right, top, left
      bool have[4] = { (code & 1) != ((code >> 1) & 1),
                       ((code >> 1) & 1) != ((code >> 2) & 1),
                       ((code >> 2) & 1) != ((code >> 3) & 1),
                       ((code >> 3) & 1) != (code & 1) };
      if (have[0]) cut(u0, v0, z00, u1, v0, z10, e[0][0], e[0][1]);
      if (have[1]) cut(u1, v0, z10, u1, v1, z11, e[1][0], e[1][1]);
      if (have[2]) cut(u1, v1, z11, u0, v1, z01, e[2][0], e[2][1]);
      if (have[3]) cut(u0, v1, z01, u0, v0, z00, e[3][0], e[3][1]);
      int ends[4], n = 0;
      for (int k = 0; k < 4; k++) if (have[k]) ends[n++] = k;
      auto add = [&](int p, int q) {
        out.push_back(Segment{ e[p][0], e[p][1], e[q][0], e[q][1] });
      };
      if (n == 2) {
        add(ends[0], ends[1]);
      } else if (n == 4) {
        //  Saddle: the cell's mean decides which corners are joined.
        const bool centreHigh = 0.25*(z00 + z10 + z11 + z01) >= level;
        const bool lowHigh = code == 5;   // 00 and 11 are high
        if (centreHigh == lowHigh) { add(0, 1); add(2, 3); }
        else                       { add(0, 3); add(1, 2); }
      }
    }
  return out;
}

vector<double> densityLevels(double zmax, int maxLevels)
{
  vector<double> l;
  for (int k = 0; k < maxLevels; k++) {
    const double v = 0.001 * pow(2.0, k);
    if (v >= zmax) break;
    l.push_back(v);
  }
  return l;
}

vector<double> orbitalLevels(double zabsmax, int maxLevels)
{
  vector<double> l;
  for (int k = 0; k < maxLevels; k++) {
    const double v = 0.02 * pow(2.0, k);
    if (v >= zabsmax) break;
    l.push_back(v);
  }
  return l;
}

string csv(const Map& m, const string& calcName)
{
  string s;
  char buf[200];
  s += "# " + calcName + ": " + m.title + ", " + m.plane.description + "\n";
  s += m.quantity == DENSITY ? "# value: electron density, e/bohr^3\n"
                             : "# value: orbital amplitude, bohr^-3/2\n";
  s += "u_A,v_A,x_A,y_A,z_A,value\n";
  for (int j = 0; j < m.nv; j++)
    for (int i = 0; i < m.nu; i++) {
      const double u = m.u(i), v = m.v(j);
      snprintf(buf, sizeof buf, "%.4f,%.4f,%.4f,%.4f,%.4f,%.8e\n", u, v,
               m.plane.origin[0] + u*m.plane.u[0] + v*m.plane.v[0],
               m.plane.origin[1] + u*m.plane.u[1] + v*m.plane.v[1],
               m.plane.origin[2] + u*m.plane.u[2] + v*m.plane.v[2],
               m.z[(size_t)j*m.nu + i]);
      s += buf;
    }
  return s;
}


//----------------------------------------------------------------------
//  Evaluator

Evaluator::Evaluator() : unrestricted(false), p_basis(new vector<EspBasisFunction>) {}
Evaluator::~Evaluator() { delete p_basis; }

bool Evaluator::build(IPropCalculation *calc)
{
  complaint.clear();
  std::unique_ptr<Fragment> frag(calc->getFragmentStep(-1));
  if (!frag || frag->numAtoms() == 0) {
    complaint = "The calculation has no chemical system.";
    return false;
  }
  if (!MoAoBasis::fragmentGeometry(frag.get(), symbols, coordsAng)) {
    complaint = "The chemical system has no usable atoms.";
    return false;
  }
  if (!MoAoBasis::coefficients(calc, symbols, false, p_c) || p_c.empty()) {
    complaint = "The calculation has no MO coefficients.";
    return false;
  }
  unrestricted = MoAoBasis::coefficients(calc, symbols, true, p_cBeta) &&
                 !p_cBeta.empty();
  if (!unrestricted) p_cBeta.clear();
  if (!MoAoBasis::flatBasis(calc, symbols, coordsAng, (int)p_c[0].size(),
                            *p_basis)) {
    complaint = "The stored basis set does not match the orbital "
                "coefficients.";
    return false;
  }

  auto fetch = [&](const char *key, vector<double>& out) {
    PropVector *pv = (PropVector*) calc->getProperty(key);
    out.clear();
    if (pv != 0) for (int i = 0; i < pv->rows(); i++) out.push_back(pv->value(i));
  };
  fetch("ORBENG", energy);
  fetch("ORBOCC", occ);
  if (occ.size() < p_c.size()) occ.resize(p_c.size(), 0.0);
  if (unrestricted) {
    fetch("ORBENGBETA", energyBeta);
    fetch("ORBOCCBETA", occBeta);
    if (occBeta.size() < p_cBeta.size()) occBeta.resize(p_cBeta.size(), 0.0);
  }
  return true;
}

int Evaluator::orbitals(bool beta) const
{
  return (int)(beta ? p_cBeta.size() : p_c.size());
}

int Evaluator::homo() const
{
  int h = 0;
  for (size_t m = 0; m < occ.size(); m++) if (occ[m] > 0.5) h = (int)m + 1;
  return h;
}

vector<string> Evaluator::atomLabels() const
{
  vector<string> l;
  for (size_t a = 0; a < symbols.size(); a++)
    l.push_back(symbols[a] + std::to_string(a + 1));
  return l;
}

void Evaluator::functions(double x, double y, double z,
                          vector<double>& phi) const
{
  const double bx = x*BOHR_PER_ANGSTROM, by = y*BOHR_PER_ANGSTROM,
               bz = z*BOHR_PER_ANGSTROM;
  phi.resize(p_basis->size());
  for (size_t i = 0; i < p_basis->size(); i++)
    phi[i] = (*p_basis)[i].value(bx, by, bz);
}

static double combine(const vector<double>& c, const vector<double>& phi)
{
  double s = 0;
  for (size_t i = 0; i < phi.size(); i++) s += c[i]*phi[i];
  return s;
}

double Evaluator::density(double x, double y, double z) const
{
  vector<double> phi;
  functions(x, y, z, phi);
  double rho = 0;
  for (size_t m = 0; m < p_c.size(); m++) {
    if (occ[m] == 0.0) continue;
    const double psi = combine(p_c[m], phi);
    rho += occ[m]*psi*psi;
  }
  for (size_t m = 0; m < p_cBeta.size(); m++) {
    if (occBeta[m] == 0.0) continue;
    const double psi = combine(p_cBeta[m], phi);
    rho += occBeta[m]*psi*psi;
  }
  return rho;
}

double Evaluator::orbital(int mo, bool beta, double x, double y, double z) const
{
  const vector< vector<double> >& c = beta ? p_cBeta : p_c;
  if (mo < 1 || mo > (int)c.size()) return 0.0;
  vector<double> phi;
  functions(x, y, z, phi);
  return combine(c[mo-1], phi);
}

bool Evaluator::map(const Plane& plane, Quantity q, int mo, bool beta,
                    double step, double margin, Map& out,
                    std::function<bool(double)> onRow) const
{
  out = Map();
  out.quantity = q; out.mo = mo; out.beta = beta; out.plane = plane;
  out.step = step;

  const int natoms = (int)symbols.size();
  double umin = 1e30, umax = -1e30, vmin = 1e30, vmax = -1e30;
  const vector<string> labels = atomLabels();
  for (int a = 0; a < natoms; a++) {
    double d[3] = { coordsAng[3*a] - plane.origin[0],
                    coordsAng[3*a+1] - plane.origin[1],
                    coordsAng[3*a+2] - plane.origin[2] };
    Mark mk;
    mk.label = labels[a];
    mk.u = dot(d, plane.u);  mk.v = dot(d, plane.v);  mk.off = dot(d, plane.n);
    out.atoms.push_back(mk);
    umin = std::min(umin, mk.u); umax = std::max(umax, mk.u);
    vmin = std::min(vmin, mk.v); vmax = std::max(vmax, mk.v);
  }
  //  Square-ish window, never thinner than 2*margin, so a diatomic's map
  //  is not a strip.
  const double cu = 0.5*(umin + umax), cv = 0.5*(vmin + vmax);
  const double hu = std::max(0.5*(umax - umin) + margin, margin + 1.0);
  const double hv = std::max(0.5*(vmax - vmin) + margin, margin + 1.0);
  out.nu = (int)ceil(2*hu/step) + 1;
  out.nv = (int)ceil(2*hv/step) + 1;
  if ((long)out.nu*out.nv > 1000000) return false;
  out.u0 = cu - 0.5*(out.nu - 1)*step;
  out.v0 = cv - 0.5*(out.nv - 1)*step;
  out.z.assign((size_t)out.nu*out.nv, 0.0);

  if (q == ORBITAL) {
    const vector<double>& e = beta ? energyBeta : energy;
    const vector<double>& o = beta ? occBeta : occ;
    char buf[160];
    string tag;
    const int h = homo();
    if (!beta && mo == h) tag = " (HOMO)";
    else if (!beta && mo == h + 1) tag = " (LUMO)";
    snprintf(buf, sizeof buf, "MO %d%s%s", mo, beta ? " beta" : "", tag.c_str());
    out.title = buf;
    if (mo >= 1 && mo <= (int)e.size()) {
      snprintf(buf, sizeof buf, "  %.4f Eh", e[mo-1]);
      out.title += buf;
    }
    if (mo >= 1 && mo <= (int)o.size()) {
      snprintf(buf, sizeof buf, "  occ %.2g", o[mo-1]);
      out.title += buf;
    }
  } else {
    out.title = "Total electron density";
  }

  vector<double> phi;
  for (int j = 0; j < out.nv; j++) {
    for (int i = 0; i < out.nu; i++) {
      const double u = out.u(i), v = out.v(j);
      const double x = plane.origin[0] + u*plane.u[0] + v*plane.v[0];
      const double y = plane.origin[1] + u*plane.u[1] + v*plane.v[1];
      const double z = plane.origin[2] + u*plane.u[2] + v*plane.v[2];
      out.z[(size_t)j*out.nu + i] =
          q == DENSITY ? density(x, y, z) : orbital(mo, beta, x, y, z);
    }
    if (onRow && !onRow((j + 1.0)/out.nv)) return false;
  }
  out.zmin = *std::min_element(out.z.begin(), out.z.end());
  out.zmax = *std::max_element(out.z.begin(), out.z.end());
  return true;
}

}  // namespace DensityPlane
