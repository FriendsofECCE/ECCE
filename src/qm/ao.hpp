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
};

// xyz: 3*npts doubles (x0,y0,z0,x1,...), bohr.  With `shells`, only those
// shells are evaluated and the columns follow their order (the DFT code passes
// the shells that reach a batch of points).
void eval_ao(const Basis& b, const double* xyz, int npts, int deriv, AoValues& out,
             const std::vector<int>* shells = nullptr);

}  // namespace qm
