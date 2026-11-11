// Atomic orbital values (and gradients) on points; used by the DFT code and
// for orbital plotting.  Same normalisation and ordering as Basis/libcint.
#pragma once
#include "basis.hpp"
#include "common.hpp"

namespace qm {

struct AoValues {
  Mat phi;                 // npts x nbf
  Mat dx, dy, dz;          // gradients, filled when deriv >= 1
  Mat dxx, dxy, dxz, dyy, dyz, dzz;  // second derivatives, deriv >= 2
  Mat d3[10];              // third derivatives, deriv >= 3, indexed by ao_d3_index
};

// Index into AoValues::d3 of d^3/(di dj dk), any order of i, j, k (0 = x):
// xxx xxy xxz xyy xyz xzz yyy yyz yzz zzz.
int ao_d3_index(int i, int j, int k);

// xyz: 3*npts doubles (x0,y0,z0,x1,...), bohr.  With `shells`, only those
// shells are evaluated and the columns follow their order (the DFT code passes
// the shells that reach a batch of points).
void eval_ao(const Basis& b, const double* xyz, int npts, int deriv, AoValues& out,
             const std::vector<int>* shells = nullptr);

}  // namespace qm
