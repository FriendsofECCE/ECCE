// Shared by the resultsh5 sources only.
#ifndef RESULTSH5_INTERNAL_HH
#define RESULTSH5_INTERNAL_HH

#include <array>
#include "tdat/ResultsH5.H"

namespace resultsh5 {

typedef std::array<int,3> Pow;

// Cartesian powers of one shell in the canonical order
// (a descending, then b descending: xx xy xz yy yz zz).
vector<Pow> cartCanonical(int l);
// The same set in the order Gaussian/Molden write it (xx yy zz xy xz yz ...).
vector<Pow> cartMolden(int l);
// permutation: result[k] = index into the canonical list of the k-th entry of the source order
vector<int> cartPermFromMolden(int l);

int shellSize(const Shell& s);
// Canonical AO from shell component `comp`; coefficients as stored (raw).
EspBasisFunction rawAo(const Shell& s, const double* centerBohr, int comp);

// Source-convention coefficients -> canonical.  `m` holds the basis with
// coefficients as the source printed them and the MO rows in canonical
// AO order but source scaling; picks the normalisation convention that
// makes the occupied MOs orthonormal and rewrites m in place.
bool normaliseSource(Model& m, string& note, string& err);

double nrad(double alpha, int l);   // normalisation of x^l exp(-alpha r^2)

}
#endif
