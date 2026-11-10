#include "d3.hpp"
#include <algorithm>
#include <cmath>
#include "common.hpp"
#include "d3_data.inc"

namespace qm {

namespace {

constexpr double kCnSteepness = 16.0;  // k1 of the counting function
constexpr double kWeightExp = 4.0;     // k3 of the Gaussian reference weights

// mctc-lib's Bohr radius (CODATA 2018), the one its radii are converted with.
constexpr double kAngToBohrMctc = 1.0 / 0.529177210903;

int npair_index(int zi, int zj) {  // 0-based element-pair block, zi >= zj (1-based Z)
  return zj + zi * (zi - 1) / 2 - 1;
}

// Reference C6 of reference a of element zi with reference b of element zj.
double ref_c6(int zi, int a, int zj, int b) {
  const int R = d3data::max_ref;
  if (zi > zj) return d3data::reference_c6[a + R * b + R * R * npair_index(zi, zj)];
  return d3data::reference_c6[b + R * a + R * R * npair_index(zj, zi)];
}

double covalent_radius(int Z) {  // D3: 4/3 of Pyykko-Atsumi, bohr
  return 4.0 / 3.0 * d3data::covalent_rad_2009_angstrom[Z - 1] * kAngToBohrMctc;
}

double sqrt_zr4r2(int Z) {  // sqrt(0.5 * <r4>/<r2> * sqrt(Z)), as simple-dftd3
  return std::sqrt(0.5 * d3data::r4_over_r2[Z - 1] * std::sqrt(double(Z)));
}

}  // namespace

bool d3bj_params(const std::string& method, D3Params& p) {
  // simple-dftd3 v1.7.0 src/dftd3/param.f90 (get_rational_damping), the
  // values of Grimme, Ehrlich, Goerigk, JCC 32, 1456 (2011); s6 = 1.
  if (method == "b3lyp") p = {1.0, 1.9889, 0.3981, 4.4211};
  else if (method == "pbe") p = {1.0, 0.7875, 0.4289, 4.4407};
  else if (method == "pbe0") p = {1.0, 1.2177, 0.4145, 4.8593};
  else if (method == "hf") p = {1.0, 0.9171, 0.3385, 2.8830};
  else return false;
  return true;
}

double d3bj_energy(const Molecule& mol, const D3Params& p, std::vector<double>* grad) {
  std::vector<int> at;  // indices of the real atoms
  for (size_t i = 0; i < mol.atoms.size(); ++i) {
    const int Z = mol.atoms[i].Z;
    if (Z == 0) continue;
    if (Z < 1 || Z > d3data::max_elem) throw Error(std::string("D3(BJ) has no reference data for element ") + element_symbol(Z));
    at.push_back(int(i));
  }
  const int n = int(at.size());
  auto Z = [&](int i) { return mol.atoms[at[i]].Z; };
  auto dist = [&](int i, int j, double d[3]) {
    double r2 = 0;
    for (int c = 0; c < 3; ++c) {
      d[c] = mol.atoms[at[i]].x[c] - mol.atoms[at[j]].x[c];
      r2 += d[c] * d[c];
    }
    return std::sqrt(r2);
  };

  // Coordination numbers.  No distance cut-off: a molecule is finite and the
  // energy stays smooth.
  std::vector<double> cn(n, 0.0);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < i; ++j) {
      double d[3];
      const double r = dist(i, j, d);
      const double rc = covalent_radius(Z(i)) + covalent_radius(Z(j));
      const double f = 1.0 / (1.0 + std::exp(-kCnSteepness * (rc / r - 1.0)));
      cn[i] += f;
      cn[j] += f;
    }

  // Normalised Gaussian weights of each atom's reference systems and their
  // CN derivatives.  Shifting by the smallest exponent keeps far-off CNs from
  // underflowing every weight; the normalisation cancels the shift.
  const int R = d3data::max_ref;
  std::vector<double> w(size_t(n) * R, 0.0), dw(size_t(n) * R, 0.0);
  for (int i = 0; i < n; ++i) {
    const int nr = d3data::number_of_references[Z(i) - 1];
    const double* rcn = d3data::reference_cn + R * (Z(i) - 1);
    double dmin = 1e300;
    for (int a = 0; a < nr; ++a) dmin = std::min(dmin, (rcn[a] - cn[i]) * (rcn[a] - cn[i]));
    double norm = 0, mean = 0;
    for (int a = 0; a < nr; ++a) norm += (w[i * R + a] = std::exp(-kWeightExp * ((rcn[a] - cn[i]) * (rcn[a] - cn[i]) - dmin)));
    for (int a = 0; a < nr; ++a) {
      w[i * R + a] /= norm;
      mean += w[i * R + a] * 2 * kWeightExp * (rcn[a] - cn[i]);
    }
    for (int a = 0; a < nr; ++a) dw[i * R + a] = w[i * R + a] * (2 * kWeightExp * (rcn[a] - cn[i]) - mean);
  }

  double E = 0;
  std::vector<double> dEdcn(n, 0.0), g(3 * size_t(n), 0.0);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < i; ++j) {
      const int ni = d3data::number_of_references[Z(i) - 1], nj = d3data::number_of_references[Z(j) - 1];
      double c6 = 0, dc6i = 0, dc6j = 0;
      for (int a = 0; a < ni; ++a)
        for (int b = 0; b < nj; ++b) {
          const double c = ref_c6(Z(i), a, Z(j), b);
          c6 += w[i * R + a] * w[j * R + b] * c;
          dc6i += dw[i * R + a] * w[j * R + b] * c;
          dc6j += w[i * R + a] * dw[j * R + b] * c;
        }
      const double rr = 3.0 * sqrt_zr4r2(Z(i)) * sqrt_zr4r2(Z(j));  // C8 / C6
      const double r0 = p.a1 * std::sqrt(rr) + p.a2;
      double d[3];
      const double r = dist(i, j, d), r2 = r * r, r6 = r2 * r2 * r2, r8 = r6 * r2;
      const double r06 = std::pow(r0, 6), r08 = r06 * r0 * r0;
      const double t6 = 1.0 / (r6 + r06), t8 = 1.0 / (r8 + r08);
      E -= c6 * (p.s6 * t6 + p.s8 * rr * t8);
      const double dEdc6 = -(p.s6 * t6 + p.s8 * rr * t8);
      dEdcn[i] += dEdc6 * dc6i;
      dEdcn[j] += dEdc6 * dc6j;
      // dE/dr divided by r, times the separation vector
      const double dEdr_r = c6 * (p.s6 * 6.0 * r6 * t6 * t6 + p.s8 * rr * 8.0 * r8 * t8 * t8) / r2;
      for (int c = 0; c < 3; ++c) {
        g[3 * i + c] += dEdr_r * d[c];
        g[3 * j + c] -= dEdr_r * d[c];
      }
    }
  if (!grad) return E;

  // Chain rule through the coordination numbers.
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < i; ++j) {
      double d[3];
      const double r = dist(i, j, d);
      const double rc = covalent_radius(Z(i)) + covalent_radius(Z(j));
      const double f = 1.0 / (1.0 + std::exp(-kCnSteepness * (rc / r - 1.0)));
      const double dfdr = -f * (1.0 - f) * kCnSteepness * rc / (r * r);
      const double s = (dEdcn[i] + dEdcn[j]) * dfdr / r;
      for (int c = 0; c < 3; ++c) {
        g[3 * i + c] += s * d[c];
        g[3 * j + c] -= s * d[c];
      }
    }
  for (int i = 0; i < n; ++i)
    for (int c = 0; c < 3; ++c) (*grad)[3 * size_t(at[i]) + c] += g[3 * i + c];
  return E;
}

}  // namespace qm
