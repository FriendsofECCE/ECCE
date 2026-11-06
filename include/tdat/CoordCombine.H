// Take the coordinates of some atoms from one structure and keep the rest
// of another (#620).  No wx, no display; plain vectors so a calculation, a
// file or a test can supply them.
#ifndef COORDCOMBINE_HH
#define COORDCOMBINE_HH

#include <string>
#include <vector>

namespace CoordCombine {

struct Result {
  std::vector<double> xyz;  ///< 3 per atom: A, with the atoms in S taken from B
  double rmsd;              ///< fit of B onto A over the atoms not in S (align only)
  bool aligned;
  std::string error;
};

/// "1-3,7,9-10" (1-based) to sorted unique 0-based indices below n.
/// False, with err set, on a bad token or an atom outside 1..n.
bool parseSet(const std::string& spec, int n, std::vector<int>& out,
              std::string& err);

/// A and B are 3 coordinates per atom (Angstrom) with one element symbol per
/// atom.  Refused unless both have the same number of atoms and the same
/// element sequence (the error names the first mismatch), and S is a
/// non-empty set of 0-based atoms.  With align, B is first moved onto A by a
/// proper rotation (never a mirror) and translation fitted over the atoms
/// outside S, which needs at least three of them; rmsd is that fit.
bool combine(const std::vector<std::string>& elemA,
             const std::vector<double>& A,
             const std::vector<std::string>& elemB,
             const std::vector<double>& B,
             const std::vector<int>& S, bool align, Result& out);

}

#endif
