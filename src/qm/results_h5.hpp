// results.h5 written by ecce-qm itself (#690): orbitals, Hessian and
// vibrations at full precision, and the path of an optimisation or scan.
#pragma once
#include <string>
#include <vector>

#include "basis.hpp"
#include "input.hpp"
#include "molecule.hpp"
#include "output.hpp"
#include "scf.hpp"

namespace qm {

struct TracePoint {
  Molecule mol;                 // with dummies, as the optimiser has it
  double energy = 0;
  std::vector<double> grad;     // 3 x atoms of mol; empty when not known
  double value = 0;             // scan coordinate, bohr or radian
};

struct ResultsTraces {
  std::vector<TracePoint> optimization, scan;
  std::string scan_unit;        // "bohr" | "radian"
};

// False (reason in why) when this build has no HDF5 or the file cannot be
// written.  A file whose orbitals fail tr(PS) = N or C'SC = 1 is written
// with checks = "failed: ...", which the job store replaces.
bool write_results_h5(const std::string& path, const Input& in, const Basis& b, const Molecule& at,
                      const ScfResult& r, const FrequencyOutput* fo, const ResultsTraces& traces,
                      std::string& why);

}  // namespace qm
