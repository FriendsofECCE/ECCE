// Result writers: the ecce-qm text format (see docs/qm/output-format.md) and
// Molden files.
#pragma once
#include <iosfwd>
#include <string>
#include "basis.hpp"
#include "charges.hpp"
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

// Static polarisability tensor of the final geometry (bohr^3).
struct PolarOutput {
  Mat alpha;
  double step = 0;  // field step, a.u.
};

// opt: set for runtype optimize, adds the convergence summary.  full: the
// molecule with its dummy atoms (null: none); the per-atom blocks then keep
// every atom of it, a dummy with charge 0, so atom numbers match ECCE's.
void write_text_output(std::ostream& os, const Input& in, const Basis& b, const ScfResult& r, double seconds,
                       const OptResult* opt = nullptr, const FrequencyOutput* freq = nullptr,
                       const ChargeSets* charges = nullptr, const Molecule* full = nullptr,
                       const PolarOutput* polar = nullptr);

// One geometry with its energy and gradient, written as soon as it is known
// and before the final output, so a running job shows its trace.
void write_opt_step(std::ostream& os, const OptStep& st);
// Final orbitals as a plain-text file, and back: the starting guess of a later
// run in the same basis.  read_orbitals returns false (reason in why) when the
// file does not belong to this basis and these atoms.
void write_orbitals(std::ostream& os, const Input& in, const Molecule& mol, const ScfResult& r);
bool read_orbitals(std::istream& is, const Input& in, const Molecule& mol, int nbf, ScfResult& r, std::string& why);

void write_molden(std::ostream& os, const Input& in, const Basis& b, const ScfResult& r,
                  const FrequencyOutput* freq = nullptr);

// Relaxed scan.  The table is rewritten whole after every point (the parser
// keeps the last), each point's geometry is its own block.
struct ScanPointOut {
  double value = 0;        // scanned coordinate: angstrom or degrees
  double energy = 0;       // hartree
  bool converged = false;
};
void write_scan_table(std::ostream& os, const std::vector<ScanPointOut>& pts, bool bond);
void write_scan_geometry(std::ostream& os, int point, double value, const Molecule& mol);
void write_progress_scan(std::ostream& os, int point, int points);

// "progress ..." lines for ECCE's job monitor (ecceqm.desc [PROGRESS]):
// one line per SCF cycle, optimisation step and frequency point, flushed at
// once.  Values with a target are written value/target.
void write_progress_scf(std::ostream& os, const ScfCycle& c, const ScfOptions& opt);
void write_progress_scf_failed(std::ostream& os, int cycles);
void write_progress_opt(std::ostream& os, const OptStep& st, const OptOptions& opt);
void write_progress_opt_end(std::ostream& os, int step, bool converged);
void write_progress_freq(std::ostream& os, int point, int points);
void write_progress_freq_end(std::ostream& os, int points, const FrequencyOutput& f);

// ORCA-style AO label for each basis function: "0O 1s", "1H 2pz", "0C 1dxy".
std::vector<std::string> ao_labels(const Basis& b, const Molecule* full = nullptr);

}  // namespace qm
