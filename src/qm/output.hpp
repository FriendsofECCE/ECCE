// Result writers: the ecce-qm text format (see docs/qm/output-format.md) and
// Molden files.
#pragma once
#include <iosfwd>
#include <string>
#include "basis.hpp"
#include "input.hpp"
#include "scf.hpp"

namespace qm {

void write_text_output(std::ostream& os, const Input& in, const Basis& b, const ScfResult& r, double seconds);
void write_molden(std::ostream& os, const Input& in, const Basis& b, const ScfResult& r);

// ORCA-style AO label for each basis function: "0O 1s", "1H 2pz", "0C 1dxy".
std::vector<std::string> ao_labels(const Basis& b);

}  // namespace qm
