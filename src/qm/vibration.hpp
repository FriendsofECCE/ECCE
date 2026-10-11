// Harmonic vibrational analysis: finite-difference Hessian and dipole
// derivatives from the analytic gradient, mass weighting, projection of
// translations and rotations, IR intensities, and ideal-gas / rigid-rotor /
// harmonic-oscillator thermochemistry.
#pragma once
#include <functional>
#include <vector>
#include "molecule.hpp"

namespace qm {

// Gradient (hartree/bohr, 3 per atom) and dipole (a.u.) at one geometry.
struct PointEval {
  std::vector<double> grad;
  double dipole[3] = {0, 0, 0};
};

struct Derivatives {
  Mat hessian;  // 3N x 3N, hartree/bohr^2, symmetrised
  Mat dmu;      // 3 x 3N, d(dipole)/d(Cartesian), a.u. (= e)
};

// Central differences of eval over every Cartesian coordinate, step in bohr.
// on_point(k, n) is called after each of the n displaced geometries.
Derivatives finite_difference_derivatives(const Molecule& mol, double step,
                                          const std::function<PointEval(const Molecule&)>& eval,
                                          const std::function<void(int, int)>& on_point = nullptr);

struct VibResult {
  int ntransrot = 6;                 // 3 for an atom, 5 for a linear molecule
  std::vector<double> masses;        // amu, per atom
  std::vector<double> residual;      // the ntransrot smallest |frequencies| (cm-1) before projection, signed
  // Vibrations, lowest first; imaginary frequencies are negative.
  std::vector<double> freq;          // cm-1
  std::vector<double> ir;            // km/mol
  Mat modes;                         // 3N x nvib Cartesian displacements, each column of unit length
  int n_imaginary = 0;
};

VibResult analyse_vibrations(const Molecule& mol, const Derivatives& d);

struct Thermo {
  double temperature = 298.15, pressure = 1.0;  // K, atm
  int symmetry_number = 1;
  bool linear = false;
  double zpe = 0;                    // hartree
  double e_thermal = 0;              // thermal correction to E incl. ZPE (hartree)
  double enthalpy_corr = 0;          // thermal correction to H incl. ZPE
  double ts = 0;                     // T*S (hartree)
  double gibbs_corr = 0;             // H_corr - T*S
  double s_trans = 0, s_rot = 0, s_vib = 0, s_elec = 0;  // hartree/K
};

// symmetry_number 0: count the proper rotations that map the molecule onto itself.
Thermo thermochemistry(const Molecule& mol, const VibResult& v, double temperature, double pressure_atm,
                       int symmetry_number);

int rotational_symmetry_number(const Molecule& mol);
double isotope_mass(int Z);  // most abundant isotope, amu; throws beyond Kr

}  // namespace qm
