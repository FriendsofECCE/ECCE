// Exchange-correlation functionals (libxc) integrated on a molecular grid.
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "basis.hpp"
#include "common.hpp"
#include "grid.hpp"

namespace qm {

struct FunctionalSpec {
  std::string name;           // lower case: hf, svwn, pbe, b3lyp, pbe0
  std::vector<int> libxc_ids; // empty for HF
  double exx = 1.0;           // fraction of exact exchange (1 for HF)
  bool gga = false;
  bool is_hf() const { return libxc_ids.empty(); }
};

// Throws on unknown names.  b3lyp = libxc HYB_GGA_XC_B3LYP5 (VWN5 local
// correlation, as ORCA/GAMESS); svwn = Slater + VWN5.
FunctionalSpec functional_by_name(const std::string& name);

// True for functionals the analytic Hessian handles: every
// libxc part LDA or GGA with fxc available and no range separation (global
// hybrids qualify; the exact exchange is done with the integrals).  HF too.
bool xc_analytic_hessian_supported(const FunctionalSpec& f);

class XcIntegrator {
 public:
  XcIntegrator(const Basis& b, const FunctionalSpec& f, Grid grid);
  ~XcIntegrator();
  // Da, Db: spin densities.  closed_shell: Da == Db is assumed and only Va
  // is computed (Vb = Va).  Returns E_xc; also reports the integrated electron count.
  double compute(const Mat& Da, const Mat& Db, bool closed_shell, Mat& Va, Mat& Vb, double* nelec = nullptr);
  // Gradient of E_xc with respect to the nuclei (3 per atom, bohr) from the
  // basis-function derivatives only; the weights' dependence on the nuclei is
  // neglected.  Same density arguments as compute().
  void gradient(const Mat& Da, const Mat& Db, bool closed_shell, std::vector<double>& grad);
  // Pieces of the analytic Hessian with the grid points and weights held
  // fixed.  P: one density, the total, for a closed shell (unpolarised libxc),
  // or the alpha and beta densities (polarised).  H += d2Exc/dx dy at fixed P,
  // and Vx[s][3A+c] += dVxc_s/dx at fixed P, both from the basis functions
  // moving with their atoms (x runs over 3 per atom).
  void hessian_terms(const std::vector<Mat>& P, Mat& H, std::vector<std::vector<Mat>>& Vx);
  // Linear response of Vxc_s at P (as above) to each first-order density in
  // D1[s][k] (symmetric; same channels as P), through the libxc kernel fxc.
  std::vector<std::vector<Mat>> kernel(const std::vector<Mat>& P, const std::vector<std::vector<Mat>>& D1);
  size_t npoints() const { return grid_.size(); }

 private:
  struct Impl;
  const Basis& basis_;
  FunctionalSpec spec_;
  Grid grid_;
  std::unique_ptr<Impl> impl_;
};

}  // namespace qm
