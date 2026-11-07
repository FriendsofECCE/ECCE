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
  vector<const resultsh5::Model*> fragmentModels;   // as given to analyse()
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

// Does the occupied d set of a one-atom fragment match the complex's?  Each
// occupied complex MO with a Mulliken share of at least dMin on the atom's d
// orbitals must be expanded mainly in the atom's OCCUPIED d orbitals: their
// share at least occMin of its d share.  A bare ion's degenerate d set is
// filled in an arbitrary orientation unless the fragment calculation was
// guided, and the d shares are then not those of the complex's t2g/eg
// split.  applicable is false for a fragment that is not an atom with a d
// set, or when no occupied MO has dMin of d (ok stays true: nothing to say).
struct FieldCheck {
  bool applicable = false;
  bool ok = true;
  int fragment = -1;        // 0-based
  int checked = 0;          // occupied MOs with a d share of at least dMin
  double worst = 1;         // lowest occupied fraction of the d share
  int mo = 0;               // 1-based, the MO that has it
  double dShare = 0;        // its d share
};
FieldCheck fieldCheck(const Analysis& a, double dMin = 0.30, double occMin = 0.80);

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
  int spin = -1;                       // -1 both channels, 0 alpha, 1 beta (open shell)
  bool occupiedOnly = false;           // leave out the empty MOs (diffuse virtuals)
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
  vector<int> moIndex, moSpin;         // 1-based MO number within its spin set; 0 alpha / 1 beta
  double fermiEv = 0;                  // midpoint of HOMO and LUMO
  double homoEv = 0, lumoEv = 0;
  double normError = 0;                // max |c^T S c - 1| over the MOs used
  bool unrestricted = false;
};

// dosOrbitals: S and the per-MO populations (slow); dosCurves: the broadening,
// repeatable with another width on the same DosResult.
bool dosOrbitals(const resultsh5::Model& complex, const DosOptions& opt, DosResult& out, string& err);
bool dosCurves(const DosOptions& opt, DosResult& out, string& err);
bool dos(const resultsh5::Model& complex, const DosOptions& opt, DosResult& out, string& err);

// Complex atoms of a fragment, for building AtomSets from fragment files.
bool mapAtoms(const resultsh5::Model& complex, const resultsh5::Model& frag,
              double tolAng, vector<int>& map, string& err);

// --- Interaction diagram ---------------------------------------------------
//
// Which levels are drawn is one fixed rule on the data: the complex MO levels
// of the window (occupiedLevels from the HOMO level down, virtualLevels from
// the LUMO level up); then, repeated until nothing changes, every MO level
// that shares an FO level (share >= threshold) with a drawn occupied MO level,
// so the partner of a drawn occupied level is never hidden; then every FO
// level with a line to a drawn MO level.  Degenerate orbitals (within
// degenerateEh) form one level; the share of an FO level in an MO level is the
// sum over its FOs averaged over the MOs, which does not depend on how a
// program mixes a degenerate set.

struct DiagramOptions {
  double threshold = 0.10;
  string method = "mulliken";
  int occupiedLevels = 4;
  int virtualLevels = 4;
  double degenerateEh = 1e-4;
  double covered = 0.90;               // an MO level covered less is not representative
  bool symmetry = true;                // label levels by irrep (runs autosym)
};

// Shares of 1 % or more: an MO level's FO levels, or the MO levels an FO
// level goes into.
struct DiagramShare {
  int level = -1;                      // id of the drawn level, -1 when not drawn
  int fragment = -1;                   // FO level: its fragment; MO level: -1
  vector<int> numbers;                 // 1-based MO numbers or FO indices
  double energyEv = 0;
  double share = 0;
  string position;
  string irrep;                        // as DiagramLevel::irrep
  string shell;                        // as DiagramLevel::shell
};

struct DiagramLevel {
  int id = 0;
  int column = 1;                      // 0 first fragment, 1 complex, 2 the other fragments
  int fragment = -1;                   // -1 complex
  int first = 0, last = 0;             // 0-based positions in the channel (MOs or FOs)
  vector<int> numbers;                 // 1-based MO numbers or FO indices
  double energyEv = 0;                 // mean over the level
  vector<double> occupation;           // per orbital, as stored
  string position;                     // "HOMO", "LUMO+3", counted in orbitals
  string irrep;                        // Mulliken label with its count, "2t2g"; "" not determined
  string shell;                        // FO of a one-atom fragment: "Cr 3d" when one l has >= 90 %
  // complex MO levels only
  bool partner = false;                // drawn by the partner rule, outside the window
  double remainder = 0;                // part outside the span of all FOs (mean over the level)
  double covered = 0;                  // sum of the shares of its lines
  bool representative = true;          // covered >= DiagramOptions::covered
  double overlapPopulation = 0;        // fragment-fragment OP per electron, summed over pairs
  string kind;                         // bonding, antibonding, fragment, unconnected; FO levels "fo"
  vector<int> fragments;               // fragments with a line into it
  vector<DiagramShare> shares;
};

struct DiagramLine {
  int fo = 0, mo = 0;                  // level ids
  int fragment = 0;
  double share = 0;
};

struct DiagramChannel {
  int spin = 0;
  vector<DiagramLevel> levels;         // complex MO levels (energy order), then FO levels
  vector<DiagramLine> lines;
  int homo = -1, lumo = -1;            // level ids
  double gapEv = 0;                    // LUMO - HOMO orbital energy
  bool representative = true;          // every drawn MO level covered
  bool windowRepresentative = true;    // every MO level of the window covered (partners aside)
  string verdict;                      // one line: representative or which levels are not
  vector<int> notRepresentative;       // level ids
  vector<int> hiddenFoLevels;          // per fragment: FO levels with no line to a drawn MO
  vector<string> notes;
};

struct Diagram {
  vector<string> fragmentNames;
  // point group of the complex [0] and of each fragment [1..], "" when not
  // determined, with the reason in symmetryWhy
  vector<string> groups, symmetryWhy;
  DiagramOptions options;
  vector<DiagramChannel> channel;
};

// Irreps of a model's orbitals in the point group autosym finds, from the
// coefficients (SymmetryAnalysis::fullLabelSpectrum, the Builder MO
// diagram's machinery).  irrep[i] is "" where no clean label exists; false
// with `why` when no orbital can be labelled.  Needs autosym and symops.
struct OrbitalSymmetry {
  string group, why;
  vector<string> irrep;                // per MO, the table's spelling ("T2g")
};
bool orbitalSymmetry(const resultsh5::Model& m, int spin, OrbitalSymmetry& out);

// `a` must come from analyse() on the same complex model.
bool diagram(const resultsh5::Model& complex, const Analysis& a, const DiagramOptions& opt,
             Diagram& out, string& err);

}
#endif
