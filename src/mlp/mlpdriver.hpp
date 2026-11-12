// ECCE's driver for machine-learned potentials (#728): optimisers (FIRE,
// BFGS), cell relaxation, molecular dynamics and finite-difference
// vibrations around an energy/force/stress evaluator.  The evaluator is a
// Python worker that only runs the network (mace-torch), or the built-in
// Lennard-Jones used by the tests.  No wx and no third-party code, so the
// whole driver can be compiled on a cluster by ecce-mlp-setup.
#pragma once
#include <array>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace mlp {

// Units as in ASE: eV, Angstrom, amu; stress in eV/A^3 with ASE's sign.
using Mat3 = std::array<double, 9>;  // row-major; rows are lattice vectors

struct Structure {
  std::vector<std::string> symbols;
  std::vector<double> pos;  // 3 per atom
  bool periodic = false;
  Mat3 cell{};
  int natoms() const { return static_cast<int>(symbols.size()); }
  double volume() const;
};

struct Result {
  double energy = 0;
  std::vector<double> forces;  // 3 per atom
  bool has_stress = false;
  std::array<double, 6> stress{};  // Voigt xx yy zz yz xz xy
};

using Evaluator = std::function<Result(const Structure&)>;

// The deck's structure: extended xyz, species and pos columns; a Lattice=
// entry makes it periodic.  Throws std::runtime_error.
Structure read_extxyz(const std::string& text);
std::string extxyz_frame(const Structure& s, double energy, const std::string& extra = "");
double mass_of(const std::string& symbol);  // ASE's table; throws if unknown

// The energy of ASE's LennardJones(sigma, epsilon, rc) with its default
// shift, so the two give the same numbers; for the tests only.
Result lennard_jones(const Structure& s, double sigma = 2.5, double eps = 0.1, double rc = 8.0);

// The Python worker: started once, then one request per structure.
class Worker {
 public:
  // argv: the command (python, script, --worker); init carries the model.
  Worker(const std::vector<std::string>& argv, const Structure& s,
         const std::string& model, const std::string& dtype, const std::string& disp);
  ~Worker();
  Result evaluate(const Structure& s);
  std::string name, version, licence, weights;

 private:
  int pid_ = -1;
  int to_ = -1;
  FILE* from_ = nullptr;
  std::string read_line();
};

// Largest per-row force norm, the convergence measure ASE uses.
double max_row_norm(const std::vector<double>& v);

struct OptOptions {
  std::string method = "FIRE";  // FIRE or BFGS, ASE's defaults
  bool cell = false;            // the cell too, as ASE's FrechetCellFilter
  double pressure = 0;          // eV/A^3, with cell
  double fmax = 0.05;
  int steps = 200;
  double timelimit = 0;  // seconds, 0 = none
};

struct OptOutcome {
  bool converged = false;
  std::string stopped;  // why it stopped short
  int steps = 0;
  Structure s;
  Result r;
};

// on_step(index, structure, result, fmax) is called at every evaluation of
// the optimiser, the first one included; elapsed() returns seconds.
OptOutcome optimize(Structure s, const OptOptions& opt, const Evaluator& eval,
                    const std::function<void(int, const Structure&, const Result&, double)>& on_step,
                    const std::function<double()>& elapsed);

struct MDOptions {
  std::string ensemble = "NVE";  // NVE or NVT (Bussi velocity rescaling)
  double dt_fs = 1.0;
  double temperature = 300;  // K: initial velocities, and the bath for NVT
  double tau_fs = 100;       // thermostat time constant
  int steps = 1000;
  int every = 10;  // frame and progress line interval
  unsigned seed = 1;
  double timelimit = 0;
};

struct MDFrame {
  int step = 0;
  double time_ps = 0, epot = 0, ekin = 0, temperature = 0;
};

struct MDOutcome {
  std::string stopped;
  int steps = 0;
  Structure s;
  Result r;
  double mean_temperature = 0;  // over all steps after the first
  double drift_ev_atom_ps = 0;  // slope of Etot, least squares
};

MDOutcome molecular_dynamics(Structure s, const MDOptions& md, const Evaluator& eval,
                             const std::function<void(const MDFrame&, const Structure&, const Result&)>& on_frame,
                             const std::function<double()>& elapsed);

// Symmetric eigenproblem (Householder + QL): a is n*n row-major; on return
// w ascending and the columns of a the eigenvectors.
void eigh(int n, std::vector<double>& a, std::vector<double>& w);

struct Vibrations {
  int rigid = 0;
  std::vector<double> freq;   // 3N, cm-1, the rigid motions first at 0
  std::vector<double> modes;  // 3N x 3N, row k = mode k, unit Cartesian vector
};

// Central-difference Hessian of the forces, mass-weighted, translations
// (and rotations for a molecule) projected out.  progress(i, n) per row.
Vibrations vibrations(const Structure& s, double delta, const Evaluator& eval,
                      const std::function<void(int, int)>& progress);

}  // namespace mlp
