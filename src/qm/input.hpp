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
//   units <angstrom|bohr>        geometry units, default angstrom
//   maxiter <n>, conv_energy <x>, conv_grad <x>
//   geometry ... end             one atom per line: symbol x y z
#pragma once
#include <iosfwd>
#include <string>
#include "molecule.hpp"
#include "scf.hpp"

namespace qm {

struct Input {
  std::string title;
  Molecule mol;
  std::string basis = "STO-3G";
  int spherical = 1;     // 1 spherical, 0 Cartesian
  ScfOptions scf;
  bool molden = false;
};

Input parse_input(std::istream& in);
Input parse_input_file(const std::string& path);

}  // namespace qm
