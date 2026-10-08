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

class XcIntegrator {
 public:
  XcIntegrator(const Basis& b, const FunctionalSpec& f, Grid grid);
  ~XcIntegrator();
  // Da, Db: spin densities.  closed_shell: Da == Db is assumed and only Va
  // is computed (Vb = Va).  Returns E_xc; also reports the integrated electron count.
  double compute(const Mat& Da, const Mat& Db, bool closed_shell, Mat& Va, Mat& Vb, double* nelec = nullptr);
  size_t npoints() const { return grid_.size(); }

 private:
  struct Impl;
  const Basis& basis_;
  FunctionalSpec spec_;
  Grid grid_;
  std::unique_ptr<Impl> impl_;
};

}  // namespace qm
