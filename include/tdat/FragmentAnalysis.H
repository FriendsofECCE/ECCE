/**
 * @file
 * Fragment-orbital analysis from real fragment calculations (#201): the
 * complex and one results.h5 per fragment, all at the complex's geometry,
 * same method and basis.  Each complex MO is expanded in the fragments' own
 * orbitals (FOs), compositions are Mulliken or SCPA, and the overlap and
 * density-of-states quantities (#171) come from the complex's S.  No wx,
 * no data store; needs only ResultsH5.H models.  Separate from the extended
 * Hueckel FragmentOrbitals (#301), which has no fragment calculations.
 *
 * Expansion: b = C_FO^T S c (overlaps of the complex MO with every FO),
 * S_FO = C_FO^T S C_FO over all fragments, a = S_FO^-1 b.  The FOs of
 * different fragments overlap, so b alone is not the coefficient vector; a
 * is Multiwfn's coFO.  Mulliken share of FO i: a_i (S_FO a)_i = a_i b_i;
 * SCPA: a_i^2 / sum a^2.  remainder = 1 - a.b is the part of the MO outside
 * the span of the FOs (about 0 when the fragment bases cover the complex's).
 */
#ifndef FRAGMENTANALYSIS_HH
#define FRAGMENTANALYSIS_HH

#include <string>
#include <vector>

#include "tdat/ResultsH5.H"

namespace fragorb {

using std::string;
using std::vector;

struct Options {
  bool allowUncovered = false;   // complex atoms in no fragment
  double atomTolAng = 1e-3;
};

struct NamedModel {
  string name;
  const resultsh5::Model* model = 0;
};

struct Fo {
  int fragment = 0;              // index into Analysis::fragmentNames
  int index = 0;                 // 1-based within the fragment, energy order as stored
  double energy = 0;             // hartree, the fragment's own
  double occupation = 0;         // as stored (restricted 0..2, unrestricted 0..1)
};

// One spin channel (restricted complexes have one, unrestricted two).
struct Channel {
  int spin = 0;
  vector<Fo> fo;
  size_t nmo = 0;
  vector<double> energy, occupation;   // complex MOs
  // all [fo * nmo + mo]
  vector<double> coef, mulliken, scpa;
  vector<double> remainder;            // per complex MO, 1 - a.b
};

struct Analysis {
  vector<string> fragmentNames;
  vector<vector<int> > atomMap;        // fragment atom -> complex atom
  vector<Channel> channel;
  bool restricted = true;
};

// false with err set when the inputs do not describe one geometry and one
// basis, fragments overlap, or the complex is not covered.
bool analyse(const resultsh5::Model& complex, const vector<NamedModel>& frags,
             const Options& opt, Analysis& out, string& err);

struct Connection {
  int spin, mo, fo;                    // mo, fo: 0-based into the channel
  double share;
};
// FO -> complex MO lines with share >= threshold (method "mulliken"|"scpa").
vector<Connection> connections(const Analysis& a, double threshold, const string& method);

// --- OPDOS / PDOS / TDOS ---------------------------------------------------

struct AtomSet {
  string name;
  vector<int> atoms;                   // 0-based complex atoms
};

struct DosOptions {
  vector<AtomSet> sets;
  vector<std::pair<int,int> > pairs;   // indices into sets; empty = all pairs
  double fwhmEv = 0.3;
  bool lorentzian = false;
  double emin = 0, emax = 0, step = 0.05;   // eV; emin == emax: HOMO-20 .. LUMO+10
};

struct DosResult {
  vector<string> setNames, pairNames;
  // per MO of every channel (energy in eV)
  vector<double> moEnergy, moOccupation;
  vector<double> moGross;              // [mo * nset + set], Mulliken gross population
  vector<double> moOverlap;            // [mo * npair + pair], 2 sum c c S
  vector<double> moWeight;             // spin factor: 2 restricted, 1 unrestricted
  // curves on the grid (eV; states per eV)
  vector<double> energy, tdos;
  vector<double> pdos;                 // [point * nset + set]
  vector<double> opdos, intOpdos;      // [point * npair + pair]
  vector<double> occupiedOverlap;      // per pair: sum_i n_i OP_i = Mulliken overlap population
  double fermiEv = 0;                  // midpoint of HOMO and LUMO
};

bool dos(const resultsh5::Model& complex, const DosOptions& opt, DosResult& out, string& err);

// Complex atoms of a fragment, for building AtomSets from fragment files.
bool mapAtoms(const resultsh5::Model& complex, const resultsh5::Model& frag,
              double tolAng, vector<int>& map, string& err);

}
#endif
