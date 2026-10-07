// Atomic orbital values (and gradients) on points; used by the DFT code and
// for orbital plotting.  Same normalisation and ordering as Basis/libcint.
#pragma once
#include "basis.hpp"
#include "common.hpp"

namespace qm {

struct AoValues {
  Mat phi;                 // npts x nbf
  Mat dx, dy, dz;          // gradients, filled when deriv >= 1
};

// xyz: 3*npts doubles (x0,y0,z0,x1,...), bohr.
void eval_ao(const Basis& b, const double* xyz, int npts, int deriv, AoValues& out);

}  // namespace qm
