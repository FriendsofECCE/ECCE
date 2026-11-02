#include "ThermoGather.H"

#include <algorithm>
#include <cmath>

#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/PropertyTask.H"
#include "tdat/Fragment.H"
#include "tdat/PropTSVecString.H"
#include "tdat/PropValue.H"
#include "tdat/PropVector.H"
#include "tdat/TAtm.H"
#include "util/EcceURL.H"

namespace {

const double PI = 3.14159265358979323846;
const double AMU_KG = 1.66053906660e-27;   // kg per amu
const double ANG2 = 1.0e-20;               // m^2 per angstrom^2

// Standard atomic weights (amu), mass number for the synthetic elements;
// goes to Og (118) so actinides and lanthanides keep their mass and inertia.
double atomicWeight(int z)
{
  static const double w[] = {0.0,
    1.00794, 4.002602, 6.941, 9.012182, 10.811, 12.0107, 14.0067, 15.9994,
    18.9984032, 20.1797, 22.98977, 24.3050, 26.9815386, 28.0855, 30.973762,
    32.065, 35.453, 39.948, 39.0983, 40.078, 44.955912, 47.867, 50.9415,
    51.9961, 54.938045, 55.845, 58.933195, 58.6934, 63.546, 65.38, 69.723,
    72.64, 74.92160, 78.96, 79.904, 83.798, 85.4678, 87.62, 88.90585, 91.224,
    92.90638, 95.96, 98.0, 101.07, 102.90550, 106.42, 107.8682, 112.411,
    114.818, 118.710, 121.760, 127.60, 126.90447, 131.293, 132.9054519,
    137.327, 138.90547, 140.116, 140.90765, 144.242, 145.0, 150.36, 151.964,
    157.25, 158.92535, 162.500, 164.93032, 167.259, 168.93421, 173.054,
    174.9668, 178.49, 180.94788, 183.84, 186.207, 190.23, 192.217, 195.084,
    196.966569, 200.59, 204.3833, 207.2, 208.98040, 209.0, 210.0, 222.0,
    223.0, 226.0, 227.0, 232.03806, 231.03588, 238.02891, 237.0, 244.0,
    243.0, 247.0, 247.0, 251.0, 252.0, 257.0, 258.0, 259.0, 262.0, 267.0,
    268.0, 271.0, 272.0, 270.0, 276.0, 281.0, 280.0, 285.0, 284.0, 289.0,
    288.0, 293.0, 294.0, 294.0};
  const int n = sizeof(w) / sizeof(w[0]);
  return z >= 1 && z < n ? w[z] : 0.0;
}

// Jacobi diagonalisation of a symmetric 3x3; eigenvalues ascending in `ev`.
void eigen3(const double a[3][3], double ev[3])
{
  double m[3][3] = {{a[0][0], a[0][1], a[0][2]},
                    {a[1][0], a[1][1], a[1][2]},
                    {a[2][0], a[2][1], a[2][2]}};
  for (int sweep = 0; sweep < 50; sweep++) {
    double off = std::fabs(m[0][1]) + std::fabs(m[0][2]) + std::fabs(m[1][2]);
    if (off < 1e-15) break;
    for (int p = 0; p < 2; p++)
      for (int q = p + 1; q < 3; q++) {
        if (std::fabs(m[p][q]) < 1e-15) continue;
        const double phi = 0.5 * std::atan2(2.0 * m[p][q], m[q][q] - m[p][p]);
        const double c = std::cos(phi), s = std::sin(phi);
        const double mpp = m[p][p], mqq = m[q][q], mpq = m[p][q];
        m[p][p] = c * c * mpp - 2.0 * s * c * mpq + s * s * mqq;
        m[q][q] = s * s * mpp + 2.0 * s * c * mpq + c * c * mqq;
        m[p][q] = m[q][p] = 0.0;
        for (int r = 0; r < 3; r++)
          if (r != p && r != q) {
            const double mrp = m[r][p], mrq = m[r][q];
            m[r][p] = m[p][r] = c * mrp - s * mrq;
            m[r][q] = m[q][r] = s * mrp + c * mrq;
          }
      }
  }
  ev[0] = m[0][0]; ev[1] = m[1][1]; ev[2] = m[2][2];
  if (ev[0] > ev[1]) std::swap(ev[0], ev[1]);
  if (ev[1] > ev[2]) std::swap(ev[1], ev[2]);
  if (ev[0] > ev[1]) std::swap(ev[0], ev[1]);
}

// Rotational constant (cm-1) from a principal moment of inertia (amu*A^2).
double rotConstant(double inertia)
{
  if (inertia <= 0.0) return 0.0;
  const double I = inertia * AMU_KG * ANG2;   // kg m^2
  return 6.62607015e-34 / (8.0 * PI * PI * 2.99792458e8 * I) * 1.0e-2;
}

}

bool gatherThermoInput(const std::string& url, ThermoInput& in,
                       std::string& name, std::vector<std::string>& notes,
                       std::string& error)
{
  PropertyTask* t = url.empty() ? 0 : EDSIFactory::getPropertyTask(EcceURL(url));
  if (!t) { error = "the calculation cannot be read."; return false; }
  name = t->getName();

  PropValue* te = dynamic_cast<PropValue*>(t->getProperty("TE"));
  if (!te) { error = "it has no total energy."; return false; }
  in.electronicEnergy = te->value();

  std::vector<double> freqs;
  PropVector* vf = dynamic_cast<PropVector*>(t->getProperty("VIBFREQ"));
  if (vf) freqs = vf->values();
  in.frequencies = realVibrations(freqs, notes);

  int sigma = 1;
  PropTSVecString* pg = dynamic_cast<PropTSVecString*>(t->getProperty("PNTGRP"));
  if (pg && pg->rows() > 0)
    sigma = symmetryNumberForPointGroup(pg->value(pg->rows() - 1));
  in.symmetryNumber = sigma;

  ICalculation* c = dynamic_cast<ICalculation*>(t);
  if (c) {
    const SpinMult::SpinMultEnum sm = c->spinMultiplicity();
    if (sm >= SpinMult::singlet) in.multiplicity = (int)sm;
  }

  Fragment* frag = t->fragment();
  if (frag) {
    std::vector<TAtm*>* atoms = frag->atoms();
    if (atoms) {
      // Centre of mass and inertia tensor from the stored geometry.
      double m = 0.0, cx = 0.0, cy = 0.0, cz = 0.0;
      int unknown = 0;
      for (TAtm* a : *atoms) {
        const double w = atomicWeight(a->atomicNumber());
        if (w <= 0.0) { unknown++; continue; }
        const double* r = a->coordinates();
        m += w; cx += w * r[0]; cy += w * r[1]; cz += w * r[2];
      }
      if (unknown)
        notes.push_back(std::to_string(unknown) + " atom(s) of an unknown "
                        "element were left out of the mass and inertia.");
      if (m > 0.0) {
        in.mass = m;
        cx /= m; cy /= m; cz /= m;
        double t[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
        for (TAtm* a : *atoms) {
          const double w = atomicWeight(a->atomicNumber());
          if (w <= 0.0) continue;
          const double* r = a->coordinates();
          const double x = r[0] - cx, y = r[1] - cy, z = r[2] - cz;
          t[0][0] += w * (y * y + z * z);
          t[1][1] += w * (x * x + z * z);
          t[2][2] += w * (x * x + y * y);
          t[0][1] -= w * x * y; t[0][2] -= w * x * z; t[1][2] -= w * y * z;
        }
        t[1][0] = t[0][1]; t[2][0] = t[0][2]; t[2][1] = t[1][2];
        double ev[3];
        eigen3(t, ev);
        in.linear = ev[0] < 1.0e-3;   // one principal moment ~ 0
        if (in.linear) {
          const double b = 0.5 * (rotConstant(ev[1]) + rotConstant(ev[2]));
          in.rotConstants[0] = b; in.rotConstants[1] = b; in.rotConstants[2] = b;
        } else {
          in.rotConstants[0] = rotConstant(ev[0]);
          in.rotConstants[1] = rotConstant(ev[1]);
          in.rotConstants[2] = rotConstant(ev[2]);
        }
      }
      delete atoms;
    }
    delete frag;
  }
  if (in.mass <= 0.0) notes.push_back("the molecular mass could not be derived "
                                      "from the geometry.");
  return true;
}
