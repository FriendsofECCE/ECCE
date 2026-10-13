// ecce-qm input file.  Line oriented, case-insensitive keywords, '#' starts
// a comment.  Keywords:
//   title <text>
//   charge <int>                 (default 0)
//   multiplicity <int>           (default 1)
//   method <hf|svwn|pbe|b3lyp|pbe0>   optionally prefixed r, u or ro
//                                (rhf, uhf, rohf, ub3lyp, rob3lyp, ...);
//                                without a prefix: restricted for
//                                multiplicity 1, restricted-open otherwise
//   basis <name>                 ECCE library name (def2-SVP, 6-31G*, ...)
//   spherical <true|false>       default true (spherical d, f, ...)
//   grid <nradial> <nangular>    DFT grid, default 99 590
//   grid_prune <true|false>      thin the angular grid near the nuclei
//                                (default true; false = full grid everywhere)
//   units <angstrom|bohr>        geometry units, default angstrom
//   maxiter <n>, conv_energy <x>, conv_grad <x>
//   charges <loewdin|chelpg|mk ...>   extra charge sets besides Mulliken
//   runtype <energy|gradient|optimize|frequency|optfreq>   default energy;
//                                gradient adds the analytic nuclear gradient,
//                                optimize moves the nuclei to a minimum
//                                (Cartesian BFGS), frequency adds harmonic
//                                frequencies, IR intensities and
//                                thermochemistry, optfreq does both
//   hessian_step <bohr>          frequency: finite-difference step (0.005)
//   temperature <K>, pressure <atm>   thermochemistry (298.15, 1)
//   symmetry_number <n>          rotational symmetry number (default: from the geometry)
//   polarizability <true|false>  static polarisability tensor by finite field
//                                (six extra SCFs) at the final geometry
//   raman <true|false>           frequency/optfreq: Raman activities and
//                                depolarisation ratios (polarisability at every
//                                displaced geometry, about 7x the cost)
//   field_step <au>              finite-field step, default 0.001
//   maxsteps <n>                 optimize: most energy+gradient evaluations (100)
//   opt_conv <normal|tight>      optimize: ORCA's Opt or TightOpt thresholds
//   basis_data ... end           explicit basis, replaces the library file:
//                                "shell <El> <S|P|D|F|G|H|I>" then one
//                                "exponent coefficient" row per primitive
//   geometry ... end             one atom per line: symbol x y z; the symbol X
//                                is a dummy atom (no nucleus, electrons or basis)
//   constraints ... end          optimize: "bond a b [r]", "angle a b c [deg]",
//                                "torsion a b c d [deg]", "fix a"; atoms count
//                                from 1 including dummies, no value = keep the
//                                starting one
#pragma once
#include <iosfwd>
#include <map>
#include <string>
#include <vector>
#include "basislib.hpp"
#include "charges.hpp"
#include "molecule.hpp"
#include "optimizer.hpp"
#include "scf.hpp"

namespace qm {

enum class RunType { Energy, Gradient, Optimize, Frequency, OptFreq };

inline bool does_frequencies(RunType t) { return t == RunType::Frequency || t == RunType::OptFreq; }
inline bool does_optimize(RunType t) { return t == RunType::Optimize || t == RunType::OptFreq; }

struct Input {
  std::string title;
  Molecule mol;
  std::string basis = "STO-3G";
  int spherical = 1;     // 1 spherical, 0 Cartesian
  ScfOptions scf;
  bool molden = false;
  ChargeOptions charges;  // extra charge sets (keyword "charges")
  RunType runtype = RunType::Energy;
  OptOptions opt;
  // Frequency runs: Cartesian step of the finite-difference Hessian (bohr),
  // temperature (K) and pressure (atm) of the thermochemistry, rotational
  // symmetry number (0: worked out from the geometry).
  double hessian_step = 0.005, temperature = 298.15, pressure = 1.0;
  int symmetry_number = 0;
  bool polarizability = false, raman = false;
  double field_step = 0.001;   // a.u.
  // Z -> shells from a basis_data block (empty: use the library).
  std::map<int, std::vector<ShellDef>> explicit_basis;
};

Input parse_input(std::istream& in);
Input parse_input_file(const std::string& path);

}  // namespace qm
