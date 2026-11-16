// Natural population analysis (NPA) from a results.h5 model (#1054).
//
// Reed, Weinstock, Weinhold, J. Chem. Phys. 83, 735 (1985): m-averaged
// atomic pre-NAOs, natural minimal basis (occupied free-atom shells, ECP
// core removed) and Rydberg set, Schmidt of the Rydberg set against the
// minimal set, occupancy-weighted symmetric orthogonalisation of each set,
// re-diagonalised atomic blocks.  The NAOs come from the total density;
// alpha and beta populations are taken in those NAOs.
#ifndef NATURALPOPULATION_HH
#define NATURALPOPULATION_HH

#include <string>
#include <vector>
#include "tdat/ResultsH5.H"

namespace npa {

using std::string;
using std::vector;

// One natural atomic orbital shell: the 2l+1 NAOs of one radial function.
struct NaoShell {
  int n = 0, l = 0;
  string kind;          // "Cor", "Val" or "Ryd"
  double occ = 0;       // electrons in the shell (all m), alpha + beta
  double alpha = 0, beta = 0;
};

struct AtomResult {
  int z = 0;
  string symbol;
  double zeff = 0;      // Z less the ECP core
  int ecpCore = 0;
  double charge = 0, population = 0, alpha = 0, beta = 0;
  double core = 0, valence = 0, rydberg = 0;
  string configuration; // e.g. "[core]2s(1.02)2p(3.10)3p(0.01)"
  vector<NaoShell> shells;
};

struct Result {
  vector<AtomResult> atoms;
  double electrons = 0;   // sum of the natural populations
  bool openShell = false;
  string note;            // e.g. ECP core counts that were inferred
};

// Empty string when NPA applies to the model; else a one-line reason
// (semiempirical, periodic, no orbitals, Slater basis, ...).
string notApplicable(const resultsh5::Model& m);

bool compute(const resultsh5::Model& m, Result& r, string& err);

string toJson(const Result& r);
string toText(const Result& r);

}
#endif
