/**
 * @file
 * ECCE's internal results model (results.h5, format_version 1): plain
 * structs, HDF5 read/write, converters, and the invariants that say
 * whether a conversion kept the physics.  No wx, no ECCE object graph.
 * Layout, units and the canonical AO order:
 * docs/claude/codereg/results-h5-format.md
 *
 * Two purposes drive the design: a later calculation in any code can
 * start from what an earlier one left (geometry, Hessian, orbitals as a
 * guess), and one small file replaces bulky ones (.chk, .gbw).
 *
 * Atomic units throughout (bohr, hartree, e).  Frequencies are in cm-1.
 */
#ifndef RESULTSH5_HH
#define RESULTSH5_HH

#include <map>
#include <string>
#include <vector>

struct EspBasisFunction;

namespace resultsh5 {

using std::string;
using std::vector;

const int FORMAT_VERSION = 1;
const char* const CONVERTER_VERSION = "resultsh5-1";

struct Shell {
  int atom = 0;              // 0-based index into Model::atomicNumber
  int l = 0;
  bool cartesian = false;    // false: real spherical (2l+1), true: Cartesian
  int n = 0;                 // STO principal quantum number, 0 for GTO
  vector<double> exponent;   // GTO exponent or STO zeta
  vector<double> coef;       // on UNNORMALISED primitives; each AO is
                             // renormalised to 1, so only ratios matter
};

struct Basis {
  string type = "GTO";       // GTO | STO | none (plane waves)
  bool orthonormal = false;  // semiempirical: S = 1, not computable
  string name;
  vector<Shell> shells;
  size_t nao() const;
};

// ECP, GTH or other pseudopotential (parameters only, never interpreted
// by the invariants).
struct Ecp {
  string kind;               // "ECP" | "GTH" | "PP"
  vector<int> atom;
  vector<string> name;
  vector<int> ncore;
  vector<double> params;     // flattened, kind specific
};

struct Mos {
  int spin = 0;              // 0 alpha, 1 beta
  int geometry = 0;
  size_t nmo = 0, nao = 0;
  vector<double> coef;       // nmo x nao, MO-major, canonical AO order,
                             // AOs unit-normalised
  vector<double> energy, occupation;
  vector<string> symmetry;   // empty when the source had none
};

struct Hessian  { int geometry = 0; string method; vector<double> upper; };  // 3N(3N+1)/2, row-wise upper triangle
struct Gradient { int geometry = 0; vector<double> g; };
struct Charges  { string scheme; int geometry = 0; vector<double> q; };
struct Multipoles { int geometry = 0; vector<double> dipole, quadrupole; };  // 3; 6 (xx xy xz yy yz zz)
struct Tensor   { string name; int geometry = 0; vector<long long> dims; vector<double> data; };

struct Vib {
  vector<double> frequency, irIntensity, reducedMass;
  vector<double> mode;       // nmodes x 3N
  int geometry = 0;
};

// Plane-wave codes: bands only, never wavefunctions.
struct Bands {
  vector<double> kpoint, weight;       // nk*3 (fractional), nk
  size_t nspin = 1, nk = 0, nband = 0;
  vector<double> energy, occupation;   // [spin][k][band]
  double fermi = 0;
  bool hasFermi = false;
};

struct TrajectoryRef { string file, sha256; long long nframes = 0; double dt = 0; };
struct Series { string name, unit; vector<double> time, value; };

struct Model {
  int formatVersion = FORMAT_VERSION;
  std::map<string,string> provenance;   // code, code_version, method,
                                        // basis_name, source_file,
                                        // source_sha256, converter_version
  vector<int> atomicNumber;
  vector<double> nuclearCharge;         // effective (ECP) charge; empty = Z
  vector<string> label;
  // nframes x natoms x 3, bohr; energy per frame (hartree)
  vector<double> coord, geomEnergy;
  bool hasCell = false;
  double lattice[9] = {0,0,0,0,0,0,0,0,0};   // rows a,b,c, bohr
  int pbc[3] = {0,0,0};

  Basis basis;
  Ecp ecp;
  vector<Mos> mos;
  vector<Hessian> hessian;
  vector<Gradient> gradient;
  vector<Charges> charges;
  vector<Multipoles> multipoles;
  vector<Vib> vib;
  vector<Tensor> tensor;
  Bands bands;
  vector<TrajectoryRef> trajectory;
  vector<Series> series;
  int nelectron[2] = {-1,-1};           // alpha, beta; -1 unknown

  size_t natoms() const { return atomicNumber.size(); }
  size_t nframes() const { return natoms() ? coord.size()/(3*natoms()) : 0; }
};

// false only to measure what compression saves (size table in the docs)
extern bool compression;
bool writeFile(const string& path, const Model& m, string& err);
bool readFile(const string& path, Model& m, string& err);

// Converters; false with err set on failure.
bool readMolden(const string& path, Model& m, string& err);
bool writeMolden(const string& path, const Model& m, string& err);
bool readFchk(const string& path, Model& m, string& err);
bool readOrcaHess(const string& path, Model& m, string& err);   // merges into m
string sha256File(const string& path);

// Canonical AOs (unit self-overlap) at one geometry frame.
bool buildAos(const Model& m, int geometry, vector<EspBasisFunction>& aos, string& err);
bool overlapMatrix(const Model& m, int geometry, vector<double>& S, string& err);

struct Invariants {
  bool haveMos = false;
  double traceErr[2] = {0,0};   // |tr(P S) - N| per spin
  double orthoErr = 0;          // max |C^T S C - 1| over occupied MOs
  bool haveHessian = false;
  bool haveFreq = false;
  double freqDiff = 0;          // cm-1, Hessian-derived against stored
  string report;
  bool ok = true;
};
Invariants check(const Model& m);
// cm-1, negative = imaginary; masses chosen from provenance code
bool frequenciesFromHessian(const Model& m, const Hessian& h, vector<double>& freq);

}
#endif
