// Site screening (#728): symmetry-distinct interstitial sites and vacancy
// pairs of a periodic host.  Dependency-free, like the driver, so the
// generation can run where ECCE's GUI libraries are not installed.
#pragma once

#include <string>
#include <vector>

#include "mlpdriver.hpp"

namespace mlp {

// One space-group operation in fractional coordinates, x' = W x + t, with
// the atom permutation it induces (perm[i] = image of atom i).
struct SymOp {
  int W[9];
  double t[3];
  std::vector<int> perm;
};

// Every operation (rotation part with entries -1/0/1 that keeps the metric,
// plus a translation) mapping the host onto itself within symprec (A).
// Integer W with small entries covers conventional and supercell settings;
// a strongly sheared cell finds fewer operations (fewer reductions, never
// a wrong one).
std::vector<SymOp> find_symmetry(const Structure& host, double symprec = 0.05);

struct Neighbour {
  std::string symbol;
  double distance;
};

// The shell around a point: sorted neighbours within cutoff, and the
// coordination number = neighbours within 1.1 x the nearest distance.
struct Environment {
  std::vector<Neighbour> neighbours;
  int coordination = 0;
  std::string summary() const;  // "Cl 2.44 x4, Na 2.44 x4"
};
Environment environment(const Structure& host, const double cart[3], double cutoff = 4.0,
                        int skip_a = -1, int skip_b = -1);

struct Site {
  double frac[3];
  double cart[3];
  double clearance;  // distance to the nearest host atom (A)
  int multiplicity;  // equivalent positions in the cell
  Environment env;
};

struct VoidOptions {
  double min_host = 1.6;  // a site at least this far from every host atom
  double min_sep = 1.0;   // candidate sites closer than this are merged
  double grid = 0.2;      // grid spacing of the void search (A)
  double symprec = 0.05;
};

// Local maxima of the distance to the nearest host atom, symmetrised and
// reduced to one representative per orbit; sorted by clearance, largest first.
std::vector<Site> find_voids(const Structure& host, const std::vector<SymOp>& ops,
                             const VoidOptions& o);

struct VacancyPair {
  int a, b;           // atom indices (0-based) of the representative pair
  double distance;    // minimum-image distance
  int multiplicity;   // pairs in the cell equivalent to this one
};

// Pairs of atoms of `element` within max_dist (0 = no limit), one per
// symmetry class, sorted by distance.
std::vector<VacancyPair> vacancy_pairs(const Structure& host, const std::vector<SymOp>& ops,
                                       const std::string& element, double max_dist);

// Helpers shared with the CLI.
void to_frac(const Structure& s, const double cart[3], double frac[3]);
void to_cart(const Structure& s, const double frac[3], double cart[3]);
double min_image_distance(const Structure& s, const double a[3], const double b[3]);
std::string write_extxyz(const Structure& s, const std::string& comment);

}  // namespace mlp
