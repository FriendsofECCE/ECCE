// Gaussian integrals through libcint, and Coulomb/exchange matrices built
// from the in-core, 8-fold-packed two-electron integrals.
#pragma once
#include <array>
#include <vector>
#include "basis.hpp"
#include "common.hpp"

namespace qm {

struct OneElectron {
  Mat S, T, V;               // overlap, kinetic, nuclear attraction
  std::array<Mat, 3> dip;    // <i|r_k|j> about the origin (bohr)
};

OneElectron one_electron(Basis& b);

// All (ij|kl) with i>=j, k>=l, ij>=kl, ij = i(i+1)/2 + j.
class EriStore {
 public:
  // max_bytes guards against a molecule too large for in-core storage.
  EriStore(Basis& b, double max_bytes = 12e9);
  static double bytes_needed(int nbf);

  // J_ij = sum_kl (ij|kl) D_kl ; K_ik = sum_jl (ij|kl) D_jl, one pair per D.
  // D must be symmetric.  If want_k is false, K is left empty.
  void jk(const std::vector<Mat>& D, std::vector<Mat>& J, std::vector<Mat>& K, bool want_k = true) const;

  double get(int i, int j, int k, int l) const;
  int nbf() const { return n_; }

 private:
  int n_;
  std::vector<double> v_;
};

}  // namespace qm
