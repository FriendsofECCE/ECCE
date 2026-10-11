// Result writers: the ecce-qm text format (see docs/qm/output-format.md) and
// Molden files.
#pragma once
#include <iosfwd>
#include <string>
#include "basis.hpp"
#include "input.hpp"
#include "optimizer.hpp"
#include "scf.hpp"
#include "vibration.hpp"

namespace qm {

// What a frequency run adds to the output.
struct FrequencyOutput {
  VibResult vib;
  Thermo thermo;
  double step = 0;          // finite-difference step, bohr
  double max_gradient = 0;  // at the geometry the Hessian belongs to, hartree/bohr
};

// opt: set for runtype optimize, adds the convergence summary.
void write_text_output(std::ostream& os, const Input& in, const Basis& b, const ScfResult& r, double seconds,
                       const OptResult* opt = nullptr, const FrequencyOutput* freq = nullptr);

// One geometry with its energy and gradient, written as soon as it is known
// and before the final output, so a running job shows its trace.
void write_opt_step(std::ostream& os, const OptStep& st);
void write_molden(std::ostream& os, const Input& in, const Basis& b, const ScfResult& r,
                  const FrequencyOutput* freq = nullptr);

// ORCA-style AO label for each basis function: "0O 1s", "1H 2pz", "0C 1dxy".
std::vector<std::string> ao_labels(const Basis& b);

}  // namespace qm
