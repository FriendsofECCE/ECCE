// Atomic charges beyond Mulliken: Loewdin population analysis, and charges
// fitted to the electrostatic potential (CHELPG, Merz-Kollman).
#pragma once
#include <array>
#include <vector>
#include "basis.hpp"
#include "molecule.hpp"
#include "scf.hpp"

namespace qm {

// Which extra charge sets to compute (input keyword "charges").
struct ChargeOptions {
  bool loewdin = false, chelpg = false, mk = false;
  bool any() const { return loewdin || chelpg || mk; }
};

struct ChargeSets {
  std::vector<double> loewdin, chelpg, mk;  // net charges; empty = not computed
  int chelpg_points = 0, mk_points = 0;
};

// Net atomic charges from S^1/2 P S^1/2 (P the total density matrix).
std::vector<double> loewdin_charges(const Molecule& mol, const Basis& basis, const ScfResult& r);

// Points (bohr) the fits sample.  CHELPG: a cubic grid of 0.3 A spacing,
// outside every CHELPG radius and within 2.8 A of some atom.  Merz-Kollman:
// quasi-uniform points, about one per square Angstrom, on the surfaces at
// 1.4, 1.6, 1.8 and 2.0 times the MK radii.
std::vector<std::array<double, 3>> chelpg_points(const Molecule& mol);
std::vector<std::array<double, 3>> mk_points(const Molecule& mol);

// Electrostatic potential (hartree/e) of nuclei and electrons at the points.
std::vector<double> esp_at_points(Basis& basis, const Molecule& mol, const ScfResult& r,
                                  const std::vector<std::array<double, 3>>& pts);

// Least-squares point charges reproducing the potential, with the sum fixed
// to the molecular charge by a Lagrange multiplier.
std::vector<double> fit_esp_charges(const Molecule& mol, const std::vector<std::array<double, 3>>& pts,
                                    const std::vector<double>& v);

ChargeSets compute_charges(Basis& basis, const Molecule& mol, const ScfResult& r, const ChargeOptions& opt);

}  // namespace qm
