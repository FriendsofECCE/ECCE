#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <fstream>
#include <sstream>
#include <map>
#include <unistd.h>
using std::ifstream;
using std::istringstream;
using std::map;
using std::sort;
using std::ostringstream;

#include "util/Ecce.H"

#include "tdat/CharacterTable.H"
#include "tdat/SymmetryAnalysis.H"
#include "tdat/MoFragments.H"


/////////////////////////////////////////////////////////////////////////////
//  The valence orbital ionisation energy table, read once.
//
//  Keyed by element symbol; the value is the valence n and the s and p
//  energies in eV, with a p energy of 0 meaning the element's valence
//  shell has none (hydrogen).
/////////////////////////////////////////////////////////////////////////////
struct VoieEntry
{
   int    n;
   double s;
   double p;
   bool   haveP;
   int    valence;       // electrons this element brings

   //  A transition metal's d shell is one principal quantum number
   //  BELOW its s and p -- cobalt is 3d, 4s, 4p -- so it carries its
   //  own, and a diagram that wrote "4d" for it would be wrong on the
   //  face of it.
   int    dn;
   double d;
   bool   haveD;

   VoieEntry() : n(0), s(0.0), p(0.0), haveP(false), valence(0),
                 dn(0), d(0.0), haveD(false) {}
};

static map<string, VoieEntry> s_voie;
static bool s_voieLoaded = false;


static void loadVoie(void)
{
   if (s_voieLoaded) return;
   s_voieLoaded = true;

   const char *home = getenv("ECCE_HOME");
   string path = string(home ? home : ".") +
                 "/data/client/config/ValenceOrbitalEnergies";

   ifstream in(path.c_str());
   if (!in) return;

   string line;
   while (std::getline(in, line)) {
      const string::size_type hash = line.find('#');
      if (hash != string::npos) line.erase(hash);

      istringstream parse(line);
      string symbol, sText, pText;
      int n = 0;
      if (!(parse >> symbol >> n >> sText)) continue;

      VoieEntry entry;
      entry.n = n;
      entry.s = atof(sText.c_str());
      if ((parse >> pText) && pText != "-") {
         entry.p = atof(pText.c_str());
         entry.haveP = true;
      }
      parse >> entry.valence;

      //  The d shell is optional and comes last, so every line
      //  written before transition metals existed still reads.
      string dnText, dText;
      if ((parse >> dnText) && dnText != "-" &&
          (parse >> dText)  && dText  != "-") {
         entry.dn    = atoi(dnText.c_str());
         entry.d     = atof(dText.c_str());
         entry.haveD = true;
      }

      s_voie[symbol] = entry;
   }
}


int MoFragments::valenceElectrons(const string& element)
{
   loadVoie();
   map<string, VoieEntry>::const_iterator it = s_voie.find(element);
   return (it == s_voie.end()) ? 0 : it->second.valence;
}


/////////////////////////////////////////////////////////////////////////////
//  Description
//     Put a fragment's electrons on its levels.
//
//     Aufbau by energy, and Hund within a set that shares one: four
//     hydrogen 1s orbitals in Td are one a1 and one t2 at the same
//     energy, and their four electrons go one to each orbital, not two
//     into the a1.  Drawing it the other way would say the a1
//     combination is doubly occupied before bonding, which is not what
//     four hydrogen atoms are.
/////////////////////////////////////////////////////////////////////////////
static void fillColumn(vector<MoLevel>& levels, int electrons)
{
   for (size_t i = 0; i < levels.size(); i++) levels[i].occupancy = 0.0;
   if (electrons <= 0) return;

   for (size_t first = 0; first < levels.size() && electrons > 0; ) {
      //  Every level at this energy fills together.
      size_t last = first;
      int orbitals = levels[first].degeneracy > 0 ? levels[first].degeneracy : 1;
      while (last + 1 < levels.size() &&
             fabs(levels[last+1].energy - levels[first].energy) < 1.0e-9) {
         last++;
         orbitals += levels[last].degeneracy > 0 ? levels[last].degeneracy : 1;
      }

      const int take = (electrons < 2*orbitals) ? electrons : 2*orbitals;

      //  Hund properly: one electron into every orbital of the set
      //  before any of them takes a second.  Oxygen's four 2p electrons
      //  are 2, 1, 1 over a1, b1, b2 -- not 1, 1, 2, which is what
      //  sharing them out in proportion gave and which reads as the
      //  wrong orbital being the doubly occupied one.
      int singles = (take < orbitals) ? take : orbitals;
      int pairs   = take - singles;

      size_t i;
      for (i = first; i <= last; i++) {
         const int mine = levels[i].degeneracy > 0 ? levels[i].degeneracy : 1;
         const int one = (singles < mine) ? singles : mine;
         levels[i].occupancy = one;
         singles -= one;
      }
      for (i = first; i <= last && pairs > 0; i++) {
         const int mine = levels[i].degeneracy > 0 ? levels[i].degeneracy : 1;
         const int two = (pairs < mine) ? pairs : mine;
         levels[i].occupancy += two;
         pairs -= two;
      }
      electrons -= take;
      first = last + 1;
   }
}


bool MoFragments::valenceEnergy(const string& element, int l, double& eV)
{
   loadVoie();

   map<string, VoieEntry>::const_iterator it = s_voie.find(element);
   if (it == s_voie.end()) return false;

   if (l == 0) { eV = it->second.s; return true; }
   if (l == 1 && it->second.haveP) { eV = it->second.p; return true; }
   if (l == 2 && it->second.haveD) { eV = it->second.d; return true; }
   return false;
}


/////////////////////////////////////////////////////////////////////////////
//  Description
//     The operations of a named point group, from the `symops` helper.
//
//     Shelled out to rather than reimplemented: gensym already holds
//     the generators and multiplies them out, and every structure tool
//     in ECCE builds from the same matrices.  A second implementation
//     here could disagree with them, and nothing would say so.
/////////////////////////////////////////////////////////////////////////////
bool MoFragments::symmetryOperations(const string& group, vector<SymOp>& ops)
{
   ops.clear();
   if (group.empty()) return false;

   string upper = group;
   for (string::size_type i = 0; i < upper.size(); i++) {
      upper[i] = toupper(upper[i]);
   }

   //  Resolved against $ECCE_HOME/bin, like the other Fortran helpers:
   //  the applications no longer run with their working directory set
   //  to the bin directory, so a bare "./symops" finds nothing.
   string command = Ecce::ecceBinCommand("symops");

   command = "echo " + upper + " | " + command + " 2>/dev/null";

   FILE *pipe = popen(command.c_str(), "r");
   if (pipe == 0) return false;

   int count = 0;
   bool ok = (fscanf(pipe, "%d", &count) == 1) && count > 0 && count <= 192;

   for (int o = 0; ok && o < count; o++) {
      SymOp op;
      for (int row = 0; row < 3; row++) {
         double translation;
         if (fscanf(pipe, "%lf %lf %lf %lf",
                    &op.m[row][0], &op.m[row][1], &op.m[row][2],
                    &translation) != 4) {
            ok = false;
            break;
         }
      }
      if (ok) ops.push_back(op);
   }

   //  A non-zero exit means the group name was not one of the 46 the
   //  generator knows.  Checked, because a partial read looks the same
   //  as an unknown group otherwise.
   if (pclose(pipe) != 0) ok = false;

   if (!ok) ops.clear();
   return ok;
}


void MoFragments::sketchPositions(const vector<double>& coords,
                                  const vector<int>& atoms,
                                  vector<double>& x, vector<double>& y,
                                  int* normal)
{
   x.clear();
   y.clear();
   if (atoms.size() < 2) return;

   //  The centre of the set, so the spread is measured about it.
   double mean[3] = { 0.0, 0.0, 0.0 };
   size_t i;
   int k;
   for (i = 0; i < atoms.size(); i++) {
      for (k = 0; k < 3; k++) mean[k] += coords[atoms[i]*3 + k];
   }
   for (k = 0; k < 3; k++) mean[k] /= atoms.size();

   //  Flatten onto the two axes the atoms spread out in most, dropping
   //  the one they spread out in least.  A perspective view of four
   //  tetrahedral hydrogens hides which are in phase with which, which
   //  is the only thing the sketch is for.
   double spread[3] = { 0.0, 0.0, 0.0 };
   for (i = 0; i < atoms.size(); i++) {
      for (k = 0; k < 3; k++) {
         const double d = coords[atoms[i]*3 + k] - mean[k];
         spread[k] += d*d;
      }
   }
   int flattest = 0;
   for (k = 1; k < 3; k++) if (spread[k] < spread[flattest]) flattest = k;
   if (normal != 0) *normal = flattest;

   const int ax = (flattest == 0) ? 1 : 0;
   const int ay = (flattest == 2) ? 1 : 2;

   double biggest = 0.0;
   for (i = 0; i < atoms.size(); i++) {
      const double dx = coords[atoms[i]*3 + ax] - mean[ax];
      const double dy = coords[atoms[i]*3 + ay] - mean[ay];
      x.push_back(dx);
      y.push_back(dy);
      if (fabs(dx) > biggest) biggest = fabs(dx);
      if (fabs(dy) > biggest) biggest = fabs(dy);
   }
   if (biggest <= 0.0) { x.clear(); y.clear(); return; }
   for (i = 0; i < x.size(); i++) { x[i] /= biggest; y[i] /= biggest; }
}



bool MoFragments::orbitsOf(const vector<double>& coords,
                           const vector<string>& elements,
                           const string& group,
                           vector< vector<int> >& orbits,
                           string& why)
{
   orbits.clear();
   why.clear();

   const CharacterTable *table = CharacterTable::lookup(group);
   if (table == 0) { why = "no character table for " + group; return false; }

   vector<SymOp> ops;
   if (!symmetryOperations(group, ops)) {
      why = "could not generate the operations of " + group;
      return false;
   }

   vector< vector<int> > images;
   if (!SymmetryAnalysis::atomImages(coords, elements, ops, 1.0e-3, images)) {
      why = "the structure is not in the symmetry frame of " + group;
      return false;
   }

   SymmetryAnalysis::orbits(images, (int)elements.size(), orbits);
   return !orbits.empty();
}


bool MoFragments::partition(const vector< vector<int> >& orbits,
                            const vector<string>& elements,
                            int& central,
                            vector<int>& terminal)
{
   central = -1;
   terminal.clear();

   //  A DIATOMIC IS ONE ORBIT OF TWO, AND IT SPLITS DOWN THE MIDDLE.
   //
   //  N2 and C2 have no central atom at all: both atoms are equivalent,
   //  so they are one orbit and the AXn test below rejects them.  But
   //  the diagram everyone draws for them exists -- one atom's orbitals
   //  on each side, the molecular orbitals between -- so the split is
   //  simply one atom to each column.
   //
   //  A heteronuclear diatomic arrives as two orbits of one and is
   //  handled by the ordinary path, with the heavier atom on the left.
   if (orbits.size() == 1 && orbits[0].size() == 2) {
      central = orbits[0][0];
      terminal.assign(1, orbits[0][1]);
      return true;
   }

   //  THIS DIAGRAM DESCRIBES ONE SHAPE OF MOLECULE: a central atom
   //  surrounded by one set of equivalent terminal atoms.  CH4, H2O,
   //  NH3, BF3, SF6 -- what a textbook writes AXn.
   //
   //  Requiring exactly two orbits is what enforces that, and it has
   //  to be enforced.  Methanol in Cs has orbits {C} {O} {H} {H} {H,H}:
   //  three atoms sit on the mirror plane, so an "atom the group
   //  leaves in place" exists and the oxygen would be nominated as the
   //  centre, with the two out-of-plane methyl hydrogens as its
   //  "terminal atoms".  That is a diagram of nothing, and it would
   //  look like a diagram of methanol.
   if (orbits.size() != 2) return false;

   //  An orbit of one is an atom the whole group leaves in place.  A
   //  molecule can have several -- the C and the O of methanal are both
   //  on the C2 axis -- so the heaviest wins, which is the atom a
   //  correlation diagram is conventionally drawn around.
   size_t centralOrbit = 0;
   for (size_t i = 0; i < orbits.size(); i++) {
      if (orbits[i].size() != 1) continue;

      const int atom = orbits[i][0];
      if (central < 0 || elements[atom].size() > elements[central].size() ||
          (elements[atom].size() == elements[central].size() &&
           elements[atom] > elements[central])) {
         central = atom;
         centralOrbit = i;
      }
   }
   if (central < 0) return false;

   for (size_t i = 0; i < orbits.size(); i++) {
      if (i == centralOrbit) continue;
      if (orbits[i].size() > terminal.size()) terminal = orbits[i];
   }
   return !terminal.empty();
}


/////////////////////////////////////////////////////////////////////////////
//  Description
//     The irreps one shell on one set of atoms spans, and how many of
//     each.
//
//     Expressed as a basis over the WHOLE molecule with every other
//     atom carrying no shells at all, so that the character comes from
//     SymmetryAnalysis::basisCharacter() rather than from a second copy
//     of the angular-character formulae living here.  That function is
//     already checked against the labels the codes themselves report.
/////////////////////////////////////////////////////////////////////////////
static bool shellIrreps(const vector<int>& atoms, int l, int numAtoms,
                        const vector< vector<int> >& images,
                        const vector<int>& classOfOp,
                        const vector<SymOp>& ops,
                        const CharacterTable& table,
                        vector<int>& multiplicity)
{
   vector< vector<int> > shells(numAtoms);
   for (size_t i = 0; i < atoms.size(); i++) {
      shells[atoms[i]].push_back(l);
   }

   vector<double> chi;
   if (!SymmetryAnalysis::basisCharacter(shells, images, classOfOp, ops,
                                         (int)table.classes().size(),
                                         false, chi)) {
      return false;
   }
   return table.reduce(chi, multiplicity);
}


//  U+00D7 MULTIPLICATION SIGN, as the two bytes UTF-8 spells it with.
//
//  It was written as char(0xd7), a single byte, which is not valid
//  UTF-8 at all -- and wxString(..., wxConvUTF8) returns an EMPTY
//  string for invalid input rather than something mangled, so every
//  label and title carrying it came out blank.
static const char *TIMES = "\xc3\x97";


/** Append one level per irrep occurrence, at the given energy. */
static void addLevels(const CharacterTable& table,
                      const vector<int>& multiplicity,
                      double energy, const string& shellName, int shell,
                      vector<MoLevel>& levels, int slot = -1)
{
   const vector<string>& irreps = table.irreps();

   for (size_t i = 0; i < irreps.size() && i < multiplicity.size(); i++) {
      if (multiplicity[i] <= 0) continue;

      MoLevel level;
      level.energy     = energy;
      level.degeneracy = multiplicity[i]*table.dimension(irreps[i]);
      level.irrep      = MoDiagram::canonicalIrrep(irreps[i]);
      level.shell      = shell;
      level.slot       = slot;

      //  The multiplicity is worth saying: two T2 sets are two
      //  different things at the same height in this model, and a
      //  reader who sees one level labelled T2 will count three
      //  orbitals where there are six.
      ostringstream name;
      if (multiplicity[i] > 1) name << multiplicity[i] << TIMES << ' ';
      name << irreps[i] << "  (" << shellName << ')';
      level.label = name.str();

      levels.push_back(level);
   }
}


bool MoFragments::basisSpansReported(const vector<double>& coords,
                                     const vector<string>& elements,
                                     const string& group,
                                     const vector< vector<int> >& shellsPerAtom,
                                     const vector<string>& reported,
                                     bool cartesian,
                                     string& detail)
{
   detail.clear();

   const CharacterTable *table = CharacterTable::lookup(group);
   if (table == 0) { detail = "no character table for " + group; return false; }

   vector<SymOp> ops;
   if (!symmetryOperations(group, ops)) {
      detail = "could not generate the operations of " + group;
      return false;
   }

   vector< vector<int> > images;
   if (!SymmetryAnalysis::atomImages(coords, elements, ops, 1.0e-3, images)) {
      detail = "the structure is not in the symmetry frame of " + group;
      return false;
   }

   vector< vector<int> > classes;
   SymmetryAnalysis::conjugacyClasses(ops, classes);

   vector<int> classOfOp;
   if (!SymmetryAnalysis::matchClasses(ops, classes, *table, classOfOp)) {
      detail = "the operations do not match " + group + "'s classes";
      return false;
   }

   vector<double> chi;
   if (!SymmetryAnalysis::basisCharacter(shellsPerAtom, images, classOfOp,
                                         ops, (int)table->classes().size(),
                                         cartesian, chi)) {
      detail = "the basis character could not be formed";
      return false;
   }

   vector<int> multiplicity;
   if (!table->reduce(chi, multiplicity)) {
      detail = "the basis representation does not reduce in " + group +
               " -- the frame or the class matching is wrong";
      return false;
   }

   //  What the basis spans, and what the code says its orbitals span.
   map<string,int> spans, says;
   const vector<string>& irreps = table->irreps();
   size_t i;
   for (i = 0; i < irreps.size() && i < multiplicity.size(); i++) {
      if (multiplicity[i] > 0) {
         spans[MoDiagram::canonicalIrrep(irreps[i])] =
             multiplicity[i]*table->dimension(irreps[i]);
      }
   }
   for (i = 0; i < reported.size(); i++) {
      if (reported[i].empty()) continue;
      says[MoDiagram::canonicalIrrep(reported[i])]++;
   }
   if (says.empty()) {
      detail = "not comparable: the calculation reports no orbital labels";
      return false;
   }

   //  Built here because the tests below need it.
   map<string,int>::const_iterator it;
   ostringstream text;
   text << "basis spans ";
   for (it = spans.begin(); it != spans.end(); ++it) {
      if (it != spans.begin()) text << " + ";
      text << it->second << it->first;
   }
   text << "; the code reports ";
   for (it = says.begin(); it != says.end(); ++it) {
      if (it != says.begin()) text << " + ";
      text << it->second << it->first;
   }

   if (spans == says) {
      detail = text.str() + " -- agree";
      return true;
   }

   //  THE SAME REDUCTION UNDER DIFFERENT NAMES IS NOT A DISAGREEMENT.
   //
   //  A diatomic is the case: autosym reports D4h, the largest finite
   //  group it knows that contains the molecule, while the code labels
   //  the orbitals in D-infinity-h.  So the basis spans
   //  2A1g + 2A2u + 2Eg + 2Eu and the code reports
   //  2sigma-g + 2sigma-u + 2pi-g + 2pi-u -- the same four sets of the
   //  same sizes, named by two different groups.  Calling that a
   //  disagreement would report a naming difference as an error, and
   //  it would do it on every diatomic.
   vector<int> ourCounts, theirCounts;
   for (it = spans.begin(); it != spans.end(); ++it) ourCounts.push_back(it->second);
   for (it = says.begin(); it != says.end(); ++it) theirCounts.push_back(it->second);
   sort(ourCounts.begin(), ourCounts.end());
   sort(theirCounts.begin(), theirCounts.end());

   if (ourCounts == theirCounts) {
      detail = text.str() + " -- the same reduction under different "
               "names, which is what a diatomic gets: the structure is "
               "handled in " + group + " and the code labels its "
               "orbitals in the infinite group";
      return true;
   }


   //  ARE THE TWO SIDES EVEN SPEAKING ABOUT THE SAME GROUP?
   //
   //  Not a disagreement if they are not.  A code may run a job in a
   //  lower group than the structure has -- ORCA labelled an ethane
   //  a', a'' while autosym finds it C3v at the same tolerance -- and
   //  then the two sets of names have nothing to do with each other.
   //  Reporting that as a failure of the symmetry machinery would be
   //  wrong, and reporting it as a pass would be worse.  It is a third
   //  outcome and it says so.
   bool shareNames = false;
   for (it = says.begin(); it != says.end() && !shareNames; ++it) {
      if (spans.find(it->first) != spans.end()) shareNames = true;
   }
   if (!shareNames) {
   ostringstream why;
      why << "not comparable: the calculation labelled its orbitals in a "
             "different group from " << group << " (it reports ";
      for (it = says.begin(); it != says.end(); ++it) {
         if (it != says.begin()) why << " + ";
         why << it->second << it->first;
      }
      why << ", none of which is an irrep of " << group << ")";
      detail = why.str();
      return false;
   }

   //  An exchange of two irreps is the axis convention, not an error.
   vector<string> names;
   for (it = spans.begin(); it != spans.end(); ++it) names.push_back(it->first);
   for (i = 0; i < names.size(); i++) {
      for (size_t j = i+1; j < names.size(); j++) {
         map<string,int> tried = spans;
         const int a = tried[names[i]], b = tried[names[j]];
         tried[names[i]] = b;
         tried[names[j]] = a;
         if (tried == says) {
            detail = text.str() + " -- agree once " + names[i] + " and " +
                     names[j] + " are exchanged, which is the axis "
                     "convention";
            return true;
         }
      }
   }

   detail = text.str() + " -- DISAGREE";
   return false;
}


double MoFragments::share(const vector<double>& coefficients,
                          const vector<int>& functionsPerAtom,
                          const vector<int>& atoms,
                          const vector<double>& overlap)
{
   const size_t nbas = coefficients.size();
   if (nbas == 0 || functionsPerAtom.empty()) return -1.0;

   //  The mapping has to account for every function.  An off-by-one
   //  here gives a number that looks like a population instead of an
   //  error, so it is checked rather than trusted.
   int total = 0;
   for (size_t i = 0; i < functionsPerAtom.size(); i++) {
      total += functionsPerAtom[i];
   }
   if (total != (int)nbas) return -1.0;

   const bool haveOverlap = (overlap.size() == nbas*nbas);

   //  Where each atom's functions start.
   vector<int> first(functionsPerAtom.size(), 0);
   int at = 0;
   for (size_t i = 0; i < functionsPerAtom.size(); i++) {
      first[i] = at;
      at += functionsPerAtom[i];
   }

   //  Which functions belong to the set asked about.
   vector<bool> mine(nbas, false);
   for (size_t k = 0; k < atoms.size(); k++) {
      const int a = atoms[k];
      if (a < 0 || a >= (int)functionsPerAtom.size()) return -1.0;
      for (int f = 0; f < functionsPerAtom[a]; f++) mine[first[a] + f] = true;
   }

   double wanted = 0.0, whole = 0.0;
   for (size_t mu = 0; mu < nbas; mu++) {
      if (coefficients[mu] == 0.0 && !haveOverlap) continue;

      double row = 0.0;
      if (haveOverlap) {
         for (size_t nu = 0; nu < nbas; nu++) {
            row += coefficients[mu]*coefficients[nu]*overlap[mu*nbas + nu];
         }
      } else {
         row = coefficients[mu]*coefficients[mu];
      }
      whole += row;
      if (mine[mu]) wanted += row;
   }

   if (whole <= 0.0) return -1.0;
   return wanted/whole;
}


/////////////////////////////////////////////////////////////////////////////
//  Description
//     Build one column from a set of atoms.
//
//     The set must be a union of orbits, or there is nothing to build:
//     if an operation takes one of its atoms outside it, the set is
//     not mapped onto itself and it has no symmetry orbitals at all.
//     That is what makes "which atoms are the fragment?" a question
//     about grouping ORBITS rather than picking atoms.
//
//     One atom alone is the exception in both directions.  It is
//     always its own orbit when the group fixes it, and then its
//     orbitals are simply its orbitals -- s and p, labelled by the
//     irreps they span.  When the group does NOT fix it, as with
//     either atom of a diatomic, it is not an orbit at all and spans
//     no irreps: half the operations move it onto its partner.  A
//     diatomic's columns are therefore plain atomic orbitals, which is
//     exactly how the textbook draws them.
/////////////////////////////////////////////////////////////////////////////
/**
 * One sigma-donor orbital per ligand, which is how a complex is drawn.
 *
 * A polyatomic ligand has a basis of its own -- ammonia brings a
 * nitrogen 2s and 2p and three hydrogen 1s -- and building symmetry
 * orbitals out of all of it gives a correct and entirely unreadable
 * wall of levels.  What a ligand field diagram uses is the ligand's
 * DONOR orbital: one function per ligand, pointing at the metal.  Six
 * of them in Oh span a1g + eg + t1u whatever the ligand is, which is
 * the whole reason the construction works for ammonia and water and
 * carbon monoxide alike.
 *
 * The reduction is the same one an s shell gets -- one spherically
 * symmetric function per site -- so this is that, on the attachment
 * atoms, with the labels and the electron count saying what it is.
 */
/**
 * The atoms actually bonded to the central one.
 *
 * RELATIVE TO THE NEAREST, not an absolute cutoff.  Hexammine
 * cobalt's nitrogens sit 2.0 Angstrom from the metal and their own
 * hydrogens 2.58, so any fixed distance generous enough for a long
 * metal-ligand bond also swallows the hydrogens behind it -- which
 * made a six-donor ligand set come out with twenty-two donors in it.
 * A quarter again past the closest neighbour separates a bonded shell
 * from what lies behind it without needing a table of radii.
 */
static void bondedTo(int centre, const vector<int>& candidates,
                     const vector<double>& coords, vector<int>& bonded)
{
   bonded.clear();
   if (candidates.empty()) return;

   double closest = 1.0e30;
   vector<double> distance(candidates.size(), 0.0);
   for (size_t i = 0; i < candidates.size(); i++) {
      double d = 0.0;
      for (int k = 0; k < 3; k++) {
         const double t = coords[3*centre+k] - coords[3*candidates[i]+k];
         d += t*t;
      }
      distance[i] = sqrt(d);
      if (distance[i] < closest) closest = distance[i];
   }

   for (size_t i = 0; i < candidates.size(); i++) {
      if (distance[i] <= 1.25*closest) bonded.push_back(candidates[i]);
   }
}


/**
 * Split a molecule into two halves across a single bond.
 *
 * The bond has to be a BRIDGE: cutting it must leave two pieces, not
 * one ring.  Ethene's C=C is one; a benzene C-C is not, which is why
 * benzene is not two halves and has to be done another way.
 *
 * @return false unless the two pieces are the same size and carry the
 *         same elements, since only equivalent halves combine in and
 *         out of phase.
 */
static bool splitAcrossBond(const vector<double>& coords,
                            const vector<string>& elements,
                            vector<int>& halfA, vector<int>& halfB)
{
   const int n = (int)elements.size();
   if (n < 4) return false;

   //  Adjacency, each atom against its own nearest neighbour.
   vector< vector<int> > near(n);
   for (int i = 0; i < n; i++) {
      vector<int> others;
      for (int j = 0; j < n; j++) if (j != i) others.push_back(j);
      bondedTo(i, others, coords, near[i]);
   }

   for (int a = 0; a < n; a++) {
      for (size_t k = 0; k < near[a].size(); k++) {
         const int b = near[a][k];
         if (b < a) continue;

         //  Flood from a, refusing to cross a-b.
         vector<bool> seen(n, false);
         vector<int> stack(1, a);
         seen[a] = true;
         while (!stack.empty()) {
            const int here = stack.back();
            stack.pop_back();
            for (size_t m = 0; m < near[here].size(); m++) {
               const int next = near[here][m];
               if ((here == a && next == b) || (here == b && next == a)) {
                  continue;
               }
               if (!seen[next]) { seen[next] = true; stack.push_back(next); }
            }
         }
         if (seen[b]) continue;              // a ring, not a bridge

         vector<int> A, B;
         for (int i = 0; i < n; i++) (seen[i] ? A : B).push_back(i);
         if (A.size() != B.size() || A.empty()) continue;

         map<string,int> countA, countB;
         for (size_t i = 0; i < A.size(); i++) countA[elements[A[i]]]++;
         for (size_t i = 0; i < B.size(); i++) countB[elements[B[i]]]++;
         if (countA != countB) continue;

         halfA = A;
         halfB = B;
         return true;
      }
   }
   return false;
}


/**
 * The operations that map a set of atoms onto itself.
 *
 * These form a subgroup, and it is the group the fragment's own
 * orbitals belong to -- C2v for a CH2 of ethene, which is the group
 * a person uses for it without thinking about where it came from.
 */
static void subgroupOf(const vector<int>& atoms,
                       const vector< vector<int> >& images,
                       vector<int>& keepOps)
{
   keepOps.clear();
   for (size_t op = 0; op < images.size(); op++) {
      bool closed = true;
      for (size_t i = 0; i < atoms.size() && closed; i++) {
         const int image = images[op][atoms[i]];
         bool inside = false;
         for (size_t j = 0; j < atoms.size(); j++) {
            if (atoms[j] == image) inside = true;
         }
         if (!inside) closed = false;
      }
      if (closed) keepOps.push_back((int)op);
   }
}


/**
 * Which named group a set of operations is.
 *
 * Tried against every table of the right order, and accepted only
 * when the conjugacy classes match -- two groups of the same order
 * are common (C2v and C2h are both four) and telling them apart is
 * the whole point.
 */
static const CharacterTable* nameSubgroup(const vector<SymOp>& ops,
                                          vector<int>& classOfOp)
{
   const vector<string> candidates = CharacterTable::names();

   vector< vector<int> > classes;
   SymmetryAnalysis::conjugacyClasses(ops, classes);

   for (size_t i = 0; i < candidates.size(); i++) {
      const CharacterTable *table = CharacterTable::lookup(candidates[i]);
      if (table == 0 || table->order() != (int)ops.size()) continue;
      if (table->classes().size() != classes.size()) continue;

      vector<int> mapping;
      if (SymmetryAnalysis::matchClasses(ops, classes, *table, mapping)) {
         classOfOp = mapping;
         return table;
      }
   }
   return 0;
}


/**
 * Two halves of a molecule, and what their combinations become.
 *
 * THE FRAGMENTS A PERSON USES ARE NOT UNIONS OF ORBITS.  Ethene's
 * two CH2 groups are the obvious way to build its diagram, and in
 * D2h the orbits are "both carbons" and "all four hydrogens" -- so a
 * CH2 crosses them, the full group maps it outside itself, and it
 * has no symmetry orbitals at all.  That is why choosing fragments
 * by grouping orbits cannot express the one fragmentation the
 * molecule is actually taught with.
 *
 * The way through is the one a person uses without naming it: work
 * in the SUBGROUP that maps each half onto itself -- C2v for a CH2
 * of ethene -- where the half is a union of orbits and the ordinary
 * machinery applies.  Each half-orbital then appears twice in the
 * molecule, in phase and out of phase, and those two combinations
 * span the full group's irreps.
 *
 * Which irreps is the induced representation of the subgroup's
 * irrep, and it is computed rather than looked up: the character is
 * twice the subgroup's on operations that stay inside the subgroup
 * and zero on those that do not, and reducing that in the full group
 * gives the pair.  Checked, not assumed -- a reduction that does not
 * come out as whole numbers of the right total dimension means the
 * assumption behind it failed, and the honest answer is then to say
 * so rather than to round.
 */
static bool inducedIrreps(const CharacterTable& full,
                          const CharacterTable& sub,
                          const vector<int>& keepOps,
                          const vector<int>& classOfOp,
                          const vector<int>& subClassOfOp,
                          const string& subIrrep,
                          vector<string>& produced,
                          vector<bool>& inPhase)
{
   produced.clear();
   inPhase.clear();

   const vector<double> *subChi = sub.characters(subIrrep);
   if (subChi == 0) return false;

   //  One representative operation per class of the FULL group, and
   //  whether it survives into the subgroup.
   const size_t fullClasses = full.classes().size();
   vector<double> chi(fullClasses, 0.0);
   vector<bool> filled(fullClasses, false);

   for (size_t op = 0; op < classOfOp.size(); op++) {
      const int fullClass = classOfOp[op];
      if (fullClass < 0 || (size_t)fullClass >= fullClasses) continue;
      if (filled[fullClass]) continue;

      int where = -1;
      for (size_t k = 0; k < keepOps.size(); k++) {
         if (keepOps[k] == (int)op) where = (int)k;
      }

      if (where < 0) {
         chi[fullClass] = 0.0;            // leaves the subgroup
      } else {
         const int subClass = subClassOfOp[where];
         if (subClass < 0 || (size_t)subClass >= subChi->size()) return false;
         chi[fullClass] = 2.0*(*subChi)[subClass];
      }
      filled[fullClass] = true;
   }

   for (size_t c = 0; c < fullClasses; c++) if (!filled[c]) return false;

   vector<int> multiplicity;
   if (!full.reduce(chi, multiplicity)) return false;

   //  WHICH COMBINATION IS WHICH, DERIVED RATHER THAN ASSUMED.
   //
   //  The in-phase combination is the one that survives the operation
   //  that SWAPS the halves: psi+ = phi + R(phi) is symmetric under R
   //  by construction, psi- = phi - R(phi) is antisymmetric.  So the
   //  sign of an irrep's character on a swapping class says which
   //  combination it is.  Taking the reduction's order instead would
   //  have been a guess dressed as an answer.
   int swapClass = -1;
   for (size_t op = 0; op < classOfOp.size() && swapClass < 0; op++) {
      bool inside = false;
      for (size_t k = 0; k < keepOps.size(); k++) {
         if (keepOps[k] == (int)op) inside = true;
      }
      if (!inside) swapClass = classOfOp[op];
   }
   if (swapClass < 0) return false;

   int dimension = 0;
   const vector<string>& names = full.irreps();
   for (size_t i = 0; i < names.size() && i < multiplicity.size(); i++) {
      const vector<double> *chiFull = full.characters(names[i]);
      if (chiFull == 0 || (size_t)swapClass >= chiFull->size()) return false;

      for (int k = 0; k < multiplicity[i]; k++) {
         produced.push_back(names[i]);
         inPhase.push_back((*chiFull)[swapClass] > 0.0);
         dimension += full.dimension(names[i]);
      }
   }

   //  Two copies of the subgroup orbital go in; the same number of
   //  orbitals must come out.
   return dimension == 2*sub.dimension(subIrrep) && !produced.empty();
}


static void buildSigmaColumn(const vector<int>& attachments,
                             const vector<string>& elements,
                             int numAtoms,
                             const vector< vector<int> >& images,
                             const vector<int>& classOfOp,
                             const vector<SymOp>& ops,
                             const CharacterTable& table,
                             MoColumn& column)
{
   if (attachments.empty()) return;

   ostringstream title;
   title << attachments.size() << " ligand "
         << (attachments.size() == 1 ? "donor" : "donors");
   column.title = title.str();

   vector<int> multiplicity;
   if (!shellIrreps(attachments, 0, numAtoms, images, classOfOp, ops,
                    table, multiplicity)) {
      return;
   }

   //  Placed together; placeFragments() moves the set onto the
   //  molecular axis, and a donor set is one shell so it stays one
   //  row.  The energy here is only an ordering: a donor lone pair
   //  is below the metal's valence orbitals, which is the one thing
   //  about its height that matters.
   addLevels(table, multiplicity, -13.0, "sigma", 0, column.levels);

   //  THE PI SET, BY SUBTRACTION.
   //
   //  Each ligand has two orbitals perpendicular to the metal-ligand
   //  axis as well as the one along it, and together the three
   //  transform as the Cartesian vectors do -- so the p-shell
   //  reduction on the same atoms is sigma AND pi, and subtracting
   //  the sigma piece leaves pi.  That is the trick the course uses
   //  for nitrite's pi system, applied to a coordination sphere.
   //
   //  It is the whole of ligand field theory's advantage over the
   //  electrostatic picture: six sigma donors span a1g + eg + t1u
   //  and leave t2g with no partner, but the pi set contains a t2g,
   //  so t2g stops being non-bonding the moment the ligands have pi
   //  orbitals -- and which way it moves, down for a pi donor or up
   //  for a pi acceptor, is what orders the spectrochemical series.
   //
   //  Drawn empty.  Whether a ligand's pi orbitals are filled
   //  (halide, a pi donor) or empty (CO and cyanide, pi acceptors,
   //  where it is pi* that matters) is a property of the ligand and
   //  cannot be read off the geometry.  Saying so is better than
   //  guessing, and the symmetry -- which is what the diagram is
   //  for -- is the same either way.
   vector<int> withPi;
   if (shellIrreps(attachments, 1, numAtoms, images, classOfOp, ops,
                   table, withPi) &&
       withPi.size() == multiplicity.size()) {

      vector<int> pi(withPi.size(), 0);
      bool any = false;
      for (size_t i = 0; i < withPi.size(); i++) {
         pi[i] = withPi[i] - multiplicity[i];
         if (pi[i] < 0) pi[i] = 0;          // should not happen; do not lie
         if (pi[i] > 0) any = true;
      }
      if (any) addLevels(table, pi, -11.0, "pi", 1, column.levels);
   }
}


/**
 * The two columns of a fragment diagram built from two halves.
 *
 * Left is the in-phase combination of each half-orbital, right the
 * out-of-phase one -- which is what the two columns of a hand-drawn
 * fragment diagram are, and why the same local label appears on
 * both.  The irrep each carries is the full group's, so the
 * correlation lines find the molecular orbitals by symmetry exactly
 * as they do for a central-atom diagram.
 */
static bool buildHalves(const vector<double>& coords,
                        const vector<string>& elements,
                        const CharacterTable& full,
                        const vector<SymOp>& ops,
                        const vector< vector<int> >& images,
                        const vector<int>& classOfOp,
                        int numAtoms,
                        MoColumn& left, MoColumn& right, string& note)
{
   vector<int> halfA, halfB;
   if (!splitAcrossBond(coords, elements, halfA, halfB)) return false;

   vector<int> keepOps;
   subgroupOf(halfA, images, keepOps);
   if (keepOps.size() < 2 || keepOps.size() >= ops.size()) return false;

   vector<SymOp> subOps;
   vector< vector<int> > subImages;
   for (size_t k = 0; k < keepOps.size(); k++) {
      subOps.push_back(ops[keepOps[k]]);
      subImages.push_back(images[keepOps[k]]);
   }

   vector<int> subClassOfOp;
   const CharacterTable *sub = nameSubgroup(subOps, subClassOfOp);
   if (sub == 0) return false;

   //  Named for what they are: a person says "the two CH2 fragments",
   //  and the local group is worth saying because it is the one the
   //  fragment's own labels belong to.
   map<string,int> count;
   for (size_t i = 0; i < halfA.size(); i++) count[elements[halfA[i]]]++;
   ostringstream formula;
   for (map<string,int>::const_iterator it = count.begin();
        it != count.end(); ++it) {
      formula << it->first;
      if (it->second > 1) formula << it->second;
   }

   left.title  = formula.str() + " (" + sub->name() + ")";
   right.title = left.title;

   //  Each element's shells, in the subgroup, exactly as a
   //  central-atom fragment is built -- the half IS a union of orbits
   //  there, which is the whole reason for dropping into it.
   map< string, vector<int> > byElement;
   for (size_t i = 0; i < halfA.size(); i++) {
      byElement[elements[halfA[i]]].push_back(halfA[i]);
   }

   bool built = false;
   for (map< string, vector<int> >::const_iterator it = byElement.begin();
        it != byElement.end(); ++it) {
      const string& symbol = it->first;

      for (int l = 0; l <= 2; l++) {
         double eV;
         if (!MoFragments::valenceEnergy(symbol, l, eV)) continue;

         vector<int> multiplicity;
         if (!shellIrreps(it->second, l, numAtoms, subImages, subClassOfOp,
                          subOps, *sub, multiplicity)) {
            continue;
         }

         loadVoie();
         ostringstream shell;
         shell << (l == 2 ? s_voie[symbol].dn : s_voie[symbol].n)
               << (l == 0 ? 's' : (l == 1 ? 'p' : 'd'));
         if (byElement.size() > 1) shell << ' ' << symbol;

         const vector<string>& subIrreps = sub->irreps();
         for (size_t i = 0; i < subIrreps.size() &&
                            i < multiplicity.size(); i++) {
            if (multiplicity[i] <= 0) continue;

            vector<string> produced;
            vector<bool> inPhase;
            if (!inducedIrreps(full, *sub, keepOps, classOfOp, subClassOfOp,
                               subIrreps[i], produced, inPhase)) {
               continue;
            }

            for (int copy = 0; copy < multiplicity[i]; copy++) {
               for (size_t k = 0; k < produced.size(); k++) {
                  MoLevel level;
                  level.energy     = eV;
                  level.shell      = l;
                  //  CANONICAL, like every other irrep in the model.
                  //  The character table spells it "Ag"; everything
                  //  that matches on an irrep compares the uppercased
                  //  form, so storing the table's spelling here made
                  //  the correlation silently find nothing.
                  level.irrep      = MoDiagram::canonicalIrrep(produced[k]);
                  level.degeneracy = full.dimension(produced[k]);

                  ostringstream name;
                  name << subIrreps[i] << "  (" << shell.str() << ')';
                  level.label = name.str();

                  //  The combination this one is, said plainly: it is
                  //  the thing the diagram is being drawn to show.
                  const bool symmetric = (k < inPhase.size())
                                         ? inPhase[k] : (k == 0);

                  ostringstream how;
                  how << (symmetric ? "in phase" : "out of phase");
                  level.annotation = how.str();

                  (symmetric ? left : right).levels.push_back(level);
                  built = true;
               }
            }
         }
      }
   }

   if (!built) return false;

   ostringstream said;
   said << "Built from two " << formula.str() << " halves, analysed in "
        << sub->name() << " and combined in phase and out of phase. The "
           "halves are not unions of equivalent atoms in " << full.name()
        << ", so this is the one fragmentation that cannot be chosen by "
           "grouping them.";
   note = said.str();
   return true;
}


static void buildColumn(const vector<int>& atoms,
                        const vector<string>& elements,
                        const vector<double>& coords,
                        int numAtoms,
                        const vector< vector<int> >& images,
                        const vector<int>& classOfOp,
                        const vector<SymOp>& ops,
                        const CharacterTable& table,
                        bool ownOrbitals,
                        MoColumn& column)
{
   if (atoms.empty()) return;

   //  Named for what it holds: "O", "2H TASOs", "C10H10 TASOs".
   ostringstream title;
   if (atoms.size() == 1 || ownOrbitals) {
      title << elements[atoms[0]];
      if (atoms.size() > 1) {
         //  Several atoms whose orbitals are being drawn as their own
         //  can only be a diatomic's partner, which is one atom.  Say
         //  the count anyway rather than imply one.
         title << atoms.size();
      }
   } else {
      map<string,int> counts;
      for (size_t i = 0; i < atoms.size(); i++) counts[elements[atoms[i]]]++;

      if (counts.size() == 1) {
         //  "4H TASOs", which reads as four hydrogens' symmetry
         //  orbitals.  "H4 TASOs" reads as a molecule's.
         title << atoms.size() << counts.begin()->first;
      } else {
         //  Several elements is a formula, where the count belongs
         //  after the symbol: "C10H10 TASOs" for ferrocene's rings.
         for (map<string,int>::const_iterator it = counts.begin();
              it != counts.end(); ++it) {
            title << it->first;
            if (it->second > 1) title << it->second;
         }
      }
      title << " TASOs";
   }
   column.title = title.str();

   if (atoms.size() > 1) {
      MoFragments::sketchPositions(coords, atoms, column.sketchX,
                                   column.sketchY, &column.sketchNormal);
   }

   //  A SHELL AT A TIME, PER ELEMENT.  A fragment need not be one
   //  element -- methanol split as CH4 against O is a carbon and four
   //  hydrogens -- and each brings its own shells and its own
   //  energies.  Taking the first atom's element for the whole set
   //  gave the hydrogens a 2p they do not have, and counted fifteen p
   //  functions where a valence picture has three.
   map< string, vector<int> > byElement;
   for (size_t i = 0; i < atoms.size(); i++) {
      byElement[elements[atoms[i]]].push_back(atoms[i]);
   }

   for (map< string, vector<int> >::const_iterator it = byElement.begin();
        it != byElement.end(); ++it) {
      const string& symbol = it->first;
      const vector<int>& mine = it->second;

      for (int l = 0; l <= 2; l++) {
         double eV;
         if (!MoFragments::valenceEnergy(symbol, l, eV)) continue;

         loadVoie();
         ostringstream shell;
         //  The d shell's own principal number, which is one below
         //  the s and p of the same atom.
         shell << (l == 2 ? s_voie[symbol].dn : s_voie[symbol].n)
               << (l == 0 ? 's' : (l == 1 ? 'p' : 'd'));
         if (byElement.size() > 1) shell << " " << symbol;

         //  One slot per element AND shell, named so the caller can
         //  bin an orbital's composition the same way.
         ostringstream key;
         key << symbol << ':' << l;
         int slot = -1;
         for (size_t k = 0; k < column.shellKeys.size(); k++) {
            if (column.shellKeys[k] == key.str()) slot = (int)k;
         }
         if (slot < 0) {
            slot = (int)column.shellKeys.size();
            column.shellKeys.push_back(key.str());
         }

         if (ownOrbitals) {
            MoLevel level;
            level.energy     = eV;
            level.degeneracy = (l == 0) ? 1 : (l == 1 ? 3 : 5);
            level.label      = shell.str();
            level.shell      = l;
            level.slot       = slot;
            column.levels.push_back(level);
            continue;
         }

         vector<int> multiplicity;
         if (!shellIrreps(mine, l, numAtoms, images, classOfOp, ops,
                          table, multiplicity)) {
            continue;
         }
         const size_t before = column.levels.size();
         addLevels(table, multiplicity, eV, shell.str(), l, column.levels,
                   slot);

         //  The phase patterns.  s and p by different calls: an
         //  operation carries one s orbital per atom along unchanged,
         //  while it takes p_x on an atom to a COMBINATION on its
         //  image.  A d shell would need the five-by-five
         //  transformation and has none here, so those levels carry no
         //  pattern rather than a made-up one.
         //
         //  Only where the sketch positions describe this element's
         //  atoms, which is when the fragment is all one element.
         if (l > 1 || mine.size() < 2 || byElement.size() > 1) continue;

         for (size_t j = before; j < column.levels.size(); j++) {
            string tableName;
            const vector<string>& names = table.irreps();
            for (size_t n = 0; n < names.size(); n++) {
               if (MoDiagram::canonicalIrrep(names[n]) ==
                   column.levels[j].irrep) {
                  tableName = names[n];
                  break;
               }
            }
            if (tableName.empty()) continue;

            vector< vector<double> > vectors;
            const bool got = (l == 0)
                ? SymmetryAnalysis::projectOrbit(mine, images, classOfOp,
                                                 table, tableName, vectors)
                : SymmetryAnalysis::projectVectorOrbit(mine, images,
                                                       classOfOp, ops, table,
                                                       tableName, vectors);
            if (got && !vectors.empty()) {
               column.levels[j].phases = vectors[0];
               column.levels[j].phaseAtoms = mine;
            }
         }
      }
   }

   //  IN ENERGY ORDER.  The levels were built one element at a time,
   //  so they come out grouped by element -- and everything downstream
   //  assumes energy order: the aufbau filling walks the list from the
   //  start, and the drawing groups levels that share a row by looking
   //  at their neighbours.  Methanol split as CH4 against O filled
   //  carbon's 2p before the hydrogens' 1s, which lies below it.
   for (size_t i = 1; i < column.levels.size(); i++) {
      for (size_t j = i; j > 0 &&
           column.levels[j].energy < column.levels[j-1].energy; j--) {
         const MoLevel swap = column.levels[j];
         column.levels[j] = column.levels[j-1];
         column.levels[j-1] = swap;
      }
   }
}

bool MoFragments::coordinationSkeleton(const vector<double>& coords,
                                       const vector<string>& elements,
                                       vector<int>& keep)
{
   keep.clear();
   if (elements.size() < 3 || coords.size() != 3*elements.size()) {
      return false;
   }

   int hub = -1, most = 1;
   for (size_t i = 0; i < elements.size(); i++) {
      vector<int> others;
      for (size_t j = 0; j < elements.size(); j++) {
         if (j != i) others.push_back((int)j);
      }
      vector<int> near;
      bondedTo((int)i, others, coords, near);
      if ((int)near.size() > most) { most = (int)near.size(); hub = (int)i; }
   }
   if (hub < 0) return false;

   double unused;
   if (!valenceEnergy(elements[hub], 2, unused)) return false;   // no metal

   vector<int> others;
   for (size_t j = 0; j < elements.size(); j++) {
      if ((int)j != hub) others.push_back((int)j);
   }
   vector<int> near;
   bondedTo(hub, others, coords, near);
   if (near.size() < 2 || near.size() >= others.size()) return false;

   keep.push_back(hub);
   for (size_t k = 0; k < near.size(); k++) keep.push_back(near[k]);
   return true;
}


bool MoFragments::linearGroupName(const vector<double>& coords,
                                  const vector<string>& elements,
                                  string& name)
{
   const size_t n = elements.size();
   if (n < 2 || coords.size() != 3*n) return false;

   //  Collinear?  Take the axis from the two atoms furthest apart and
   //  require every atom to sit on it.
   size_t a = 0, b = 1;
   double furthest = -1.0;
   for (size_t i = 0; i < n; i++) {
      for (size_t j = i + 1; j < n; j++) {
         double d = 0.0;
         for (int k = 0; k < 3; k++) {
            const double t = coords[3*i+k] - coords[3*j+k];
            d += t*t;
         }
         if (d > furthest) { furthest = d; a = i; b = j; }
      }
   }
   if (furthest <= 0.0) return false;

   double axis[3];
   const double length = sqrt(furthest);
   for (int k = 0; k < 3; k++) {
      axis[k] = (coords[3*b+k] - coords[3*a+k])/length;
   }

   for (size_t i = 0; i < n; i++) {
      double v[3], along = 0.0;
      for (int k = 0; k < 3; k++) {
         v[k] = coords[3*i+k] - coords[3*a+k];
         along += v[k]*axis[k];
      }
      double off = 0.0;
      for (int k = 0; k < 3; k++) {
         const double t = v[k] - along*axis[k];
         off += t*t;
      }
      if (sqrt(off) > 1.0e-3) return false;      // not on the axis
   }

   //  A centre of inversion decides between the two: every atom must
   //  have a partner of its own element opposite the midpoint.
   double centre[3] = {0.0, 0.0, 0.0};
   for (size_t i = 0; i < n; i++) {
      for (int k = 0; k < 3; k++) centre[k] += coords[3*i+k]/(double)n;
   }

   bool centrosymmetric = true;
   for (size_t i = 0; i < n && centrosymmetric; i++) {
      bool matched = false;
      for (size_t j = 0; j < n && !matched; j++) {
         if (elements[j] != elements[i]) continue;
         double d = 0.0;
         for (int k = 0; k < 3; k++) {
            const double want = 2.0*centre[k] - coords[3*i+k];
            const double t = coords[3*j+k] - want;
            d += t*t;
         }
         if (sqrt(d) < 1.0e-3) matched = true;
      }
      if (!matched) centrosymmetric = false;
   }

   name = centrosymmetric ? "DINFH" : "CINFV";
   return true;
}


bool MoFragments::build(const vector<double>& coords,
                        const vector<string>& elements,
                        const string& group,
                        int charge,
                        MoColumn& left,
                        MoColumn& right,
                        string& note,
                        vector<int>* leftAtoms,
                        vector<int>* rightAtoms,
                        const vector<int>* sideOfOrbit)
{
   if (leftAtoms  != 0) leftAtoms->clear();
   if (rightAtoms != 0) rightAtoms->clear();

   left  = MoColumn();
   right = MoColumn();
   note.clear();

   if (group.empty()) {
      note = "The molecule's point group is not known, so there is "
             "nothing to correlate against. Use Find Symmetry first.";
      return false;
   }

   const CharacterTable *table = CharacterTable::lookup(group);
   if (table == 0) {
      note = "No character table for " + group + ".";
      return false;
   }

   vector<SymOp> ops;
   if (!symmetryOperations(group, ops)) {
      note = "Could not generate the symmetry operations of " + group + ".";
      return false;
   }
   if ((int)ops.size() != table->order()) {
      //  Two independent sources for the same number: the generator and
      //  the hand-entered table.  If they disagree, one of them is
      //  wrong and neither should be trusted for a diagram.
      ostringstream why;
      why << "The generator produced " << ops.size() << " operations for "
          << group << " but its character table has order "
          << table->order() << ".";
      note = why.str();
      return false;
   }

   //  THE SKELETON, WHERE THE LIGANDS ARE MOLECULES.
   //
   //  A ligand field diagram is drawn for the coordination skeleton:
   //  the metal and the atoms bonded to it, one sigma donor each.
   //  The ligands' own internal structure takes no part -- and it is
   //  what stops the molecule having the symmetry the diagram is
   //  drawn in.  Hexammine cobalt's donor set is a perfect
   //  octahedron; the complex is not, because six ammonia rotors
   //  cannot all be, so a whole-molecule symmetry search calls it Th
   //  at best and C1 in practice.  Every course draws it in Oh
   //  regardless, and is right to: the thing being classified is the
   //  skeleton.
   //
   //  So where the terminal atoms are not all bonded to the central
   //  one, the analysis runs on the skeleton and the caller's group
   //  is the skeleton's.  Decided here, before anything symmetric is
   //  computed, because the frame check below is exactly what the
   //  rotors fail.
   vector<double> workCoords = coords;
   vector<string> workElements = elements;
   vector<int> skeletonOf;              // index in the original, or empty
   bool ligandField = false;
   {
      int hub = -1, most = 1;
      for (size_t i = 0; i < elements.size(); i++) {
         vector<int> others;
         for (size_t j = 0; j < elements.size(); j++) {
            if (j != i) others.push_back((int)j);
         }
         vector<int> near;
         bondedTo((int)i, others, coords, near);
         if ((int)near.size() > most) { most = (int)near.size(); hub = (int)i; }
      }

      if (hub >= 0) {
         vector<int> others;
         for (size_t j = 0; j < elements.size(); j++) {
            if ((int)j != hub) others.push_back((int)j);
         }
         vector<int> near;
         bondedTo(hub, others, coords, near);

         //  ONLY FOR A COORDINATION COMPLEX, AND ONLY UNASKED.
         //
         //  "Some atoms are not bonded to the hub" is true of almost
         //  every molecule -- methanol's carbon has an O and three H
         //  bonded to it and the hydroxyl hydrogen beyond -- and
         //  reducing those to a skeleton throws away the molecule.
         //  What makes a ligand field a ligand field is a metal at
         //  the centre.  And a caller who has chosen the fragments
         //  has said what it wants; do not renumber the atoms under
         //  it.
         double unusedD;
         const bool metalHub = valenceEnergy(elements[hub], 2, unusedD);

         if (metalHub && sideOfOrbit == 0 &&
             near.size() > 1 && near.size() < others.size()) {
            ligandField = true;
            skeletonOf.push_back(hub);
            for (size_t k = 0; k < near.size(); k++) {
               skeletonOf.push_back(near[k]);
            }

            workCoords.clear();
            workElements.clear();
            for (size_t k = 0; k < skeletonOf.size(); k++) {
               const int a = skeletonOf[k];
               workElements.push_back(elements[a]);
               for (int c = 0; c < 3; c++) {
                  workCoords.push_back(coords[3*a + c]);
               }
            }
         }
      }
   }

   const vector<double>& coordsUsed  = ligandField ? workCoords   : coords;
   const vector<string>& elementsUsed = ligandField ? workElements : elements;
   const int numAtoms = (int)elementsUsed.size();

   vector< vector<int> > images;
   if (!SymmetryAnalysis::atomImages(coordsUsed, elementsUsed, ops, 1.0e-3,
                                     images)) {
      note = "The structure is not in the symmetry frame of " + group +
             ", so the orbitals cannot be classified. Use Find Symmetry "
             "to clean it up first.";
      return false;
   }

   vector< vector<int> > classes;
   SymmetryAnalysis::conjugacyClasses(ops, classes);

   vector<int> classOfOp;
   if (!SymmetryAnalysis::matchClasses(ops, classes, *table, classOfOp)) {
      note = "The generated operations do not match " + group +
             "'s character table classes.";
      return false;
   }

   vector< vector<int> > orbits;
   SymmetryAnalysis::orbits(images, numAtoms, orbits);

   //  --- which atoms are on which side ------------------------------
   //
   //  A CALLER'S CHOICE WHERE THERE IS ONE, and otherwise the
   //  automatic split.  The choice is expressed by grouping orbits
   //  rather than picking atoms, because a fragment the group maps
   //  outside itself has no symmetry orbitals at all -- so this cannot
   //  be handed an impossible fragmentation.
   vector<int> leftSet, rightSet;
   bool diatomic = false;

   if (sideOfOrbit != 0 && sideOfOrbit->size() == orbits.size()) {
      for (size_t i = 0; i < orbits.size(); i++) {
         const int side = (*sideOfOrbit)[i];
         if (side == 0) {
            leftSet.insert(leftSet.end(), orbits[i].begin(), orbits[i].end());
         } else if (side == 1) {
            rightSet.insert(rightSet.end(), orbits[i].begin(), orbits[i].end());
         }
      }
      if (leftSet.empty() || rightSet.empty()) {
         note = "Both sides of the diagram need at least one set of atoms.";
         return false;
      }
   } else {
      int central = -1;
      vector<int> terminal;
      if (!partition(orbits, elementsUsed, central, terminal)) {

         //  THE MOST CONNECTED ATOM AGAINST EVERYTHING ELSE.
         //
         //  partition() handles the shapes a textbook names -- a
         //  central atom with ONE set of equivalent neighbours, or a
         //  diatomic -- and refuses the moment there are two
         //  different kinds of neighbour.  That refuses formaldehyde,
         //  which has a carbon, an oxygen and two hydrogens in three
         //  orbits and is about as standard a diagram as exists.
         //
         //  Where one orbit is a single atom, and that atom has more
         //  neighbours than any other single atom, it is the central
         //  one by the same reasoning a person uses looking at the
         //  structure.  Everything else is the terminal fragment: a
         //  union of whole orbits, so the group maps it onto itself
         //  and it has symmetry orbitals, which is the only thing
         //  that has to be true.
         //
         //  Said in the note, because a fragmentation the program
         //  chose is a claim the reader should be able to overrule --
         //  and the panel's chooser is how.
         int best = -1, bestNeighbours = -1;
         for (size_t i = 0; i < orbits.size(); i++) {
            if (orbits[i].size() != 1) continue;
            const int atom = orbits[i][0];

            int neighbours = 0;
            for (int other = 0; other < numAtoms; other++) {
               if (other == atom) continue;
               double d = 0.0;
               for (int k = 0; k < 3; k++) {
                  const double t = coordsUsed[3*atom+k] - coordsUsed[3*other+k];
                  d += t*t;
               }
               if (sqrt(d) < 2.2) neighbours++;      // Angstrom
            }
            if (neighbours > bestNeighbours) {
               bestNeighbours = neighbours;
               best = atom;
            }
         }

         //  TWO HALVES FIRST, BECAUSE IT IS THE BETTER DIAGRAM.
         //
         //  Where the molecule falls into two equivalent halves
         //  across a single bond, combining them is how the thing is
         //  actually taught -- and it is the one fragmentation that
         //  cannot be chosen by grouping orbits, because the halves
         //  cross them.
         vector<int> halfA, halfB;
         splitAcrossBond(coordsUsed, elementsUsed, halfA, halfB);

         if (buildHalves(coordsUsed, elementsUsed, *table, ops, images,
                         classOfOp, numAtoms, left, right, note)) {
            int total = 0;
            for (size_t i = 0; i < elementsUsed.size(); i++) {
               total += valenceElectrons(elementsUsed[i]);
            }
            total -= charge;

            //  The two columns are the two combinations of one set of
            //  fragment orbitals, at the same energies, so the
            //  molecule's electrons divide between them evenly.
            fillColumn(left.levels,  total/2);
            fillColumn(right.levels, total - total/2);

            //  THE HALVES, NOT NOTHING.
            //
            //  Clearing these zeroed every composition share, and a
            //  fragment level whose share is zero is never placed on
            //  the molecular axis -- the whole column stayed at the
            //  free-atom energies in eV while the molecular orbitals
            //  were in Hartree, twenty units below the picture.
            //
            //  Each half genuinely carries half of every orbital, by
            //  symmetry, which is the right answer to the question
            //  the shares ask.
            if (leftAtoms  != 0) *leftAtoms  = halfA;
            if (rightAtoms != 0) *rightAtoms = halfB;
            return true;
         }

         //  TWO ORBITS ARE ALREADY TWO FRAGMENTS.
         //
         //  Ethene has two: the carbons and the hydrogens.  Neither
         //  is a central atom, so partition() refuses -- and yet the
         //  split is right there, and both halves are unions of whole
         //  orbits, so the group maps each onto itself and both have
         //  symmetry orbitals.  The C2 unit against the four
         //  hydrogens is how the sigma framework is built up.
         //
         //  Not a substitute for combining two CH2 fragments, which
         //  is a different and better diagram for this molecule and
         //  needs a subgroup; it is what can be done without one.
         if (orbits.size() == 2 && best < 0) {
            leftSet  = orbits[0];
            rightSet = orbits[1];

            //  The heavier set on the left, where a central atom
            //  would be: a diagram reads outward from the middle.
            double leftMass = 0.0, rightMass = 0.0;
            for (size_t i = 0; i < leftSet.size(); i++) {
               leftMass += valenceElectrons(elementsUsed[leftSet[i]]);
            }
            for (size_t i = 0; i < rightSet.size(); i++) {
               rightMass += valenceElectrons(elementsUsed[rightSet[i]]);
            }
            if (rightMass > leftMass) leftSet.swap(rightSet);

            ostringstream said;
            said << "Drawn as its two sets of equivalent atoms. This "
                    "molecule has no central atom, so there is no "
                    "central-atom diagram; combining fragments of it "
                    "instead needs a subgroup and a choice.";
            note = said.str();

            if (leftAtoms  != 0) *leftAtoms  = leftSet;
            if (rightAtoms != 0) *rightAtoms = rightSet;

            buildColumn(leftSet, elementsUsed, coordsUsed, numAtoms, images,
                        classOfOp, ops, *table, false, left);
            buildColumn(rightSet, elementsUsed, coordsUsed, numAtoms, images,
                        classOfOp, ops, *table, false, right);

            int le = 0, re = 0;
            for (size_t i = 0; i < leftSet.size(); i++) {
               le += valenceElectrons(elementsUsed[leftSet[i]]);
            }
            for (size_t i = 0; i < rightSet.size(); i++) {
               re += valenceElectrons(elementsUsed[rightSet[i]]);
            }
            fillColumn(left.levels,  le);
            fillColumn(right.levels, re - charge);

            return !(left.levels.empty() && right.levels.empty());
         }

         if (best >= 0 && bestNeighbours > 1 && orbits.size() > 2) {
            central = best;
            terminal.clear();
            for (int a = 0; a < numAtoms; a++) {
               if (a != best) terminal.push_back(a);
            }

            ostringstream chosen;
            chosen << "Drawn as " << elementsUsed[best]
                   << " against the other atoms, which is this program's "
                      "guess at the fragments: it has "
                   << orbits.size() << " symmetry-distinct sets of atoms "
                      "and more than one way to split them. Choose the "
                      "fragments yourself if another split is the one you "
                      "want.";
            note = chosen.str();

            leftSet.assign(1, central);
            rightSet = terminal;

            if (leftAtoms  != 0) *leftAtoms  = leftSet;
            if (rightAtoms != 0) *rightAtoms = rightSet;

            buildColumn(leftSet, elementsUsed, coordsUsed, numAtoms, images,
                        classOfOp, ops, *table, false, left);

            //  Ligands that are molecules give their donor orbital
            //  only -- see the same decision below, which this
            //  branch returns before reaching.
            vector<int> bonded;
            bondedTo(central, rightSet, coordsUsed, bonded);
            //  In a skeleton the ligands ARE their attachment atoms,
            //  so the count test cannot see them -- the skeleton was
            //  built precisely because they were molecules.
            const bool donors = bonded.size() > 1 &&
                                (ligandField ||
                                 bonded.size() < rightSet.size());

            if (donors) {
               buildSigmaColumn(bonded, elementsUsed, numAtoms, images,
                                classOfOp, ops, *table, right);
               note += " Each ligand contributes one sigma donor orbital, "
                       "as a ligand field diagram does.";
            } else {
               buildColumn(rightSet, elementsUsed, coordsUsed, numAtoms, images,
                           classOfOp, ops, *table, false, right);
            }

            int leftElectrons = 0, rightElectrons = 0;
            for (size_t i = 0; i < leftSet.size(); i++) {
               leftElectrons += valenceElectrons(elementsUsed[leftSet[i]]);
            }
            for (size_t i = 0; i < rightSet.size(); i++) {
               rightElectrons += valenceElectrons(elementsUsed[rightSet[i]]);
            }
            //  A DONOR SET'S COUNT IS FIXED, CHARGE AND ALL.  Two
            //  electrons per ligand is what "dative" means, and the
            //  ligands' own charges are already inside that: six
            //  cyanides donate twelve electrons, not fifteen.
            //  Adding the complex's charge on top spilled three into
            //  the pi set, which is meant to be drawn empty.
            if (donors) { rightElectrons = 2*(int)bonded.size(); }
            bool metal = false;
            double unused;
            if (charge > 0 && valenceEnergy(elementsUsed[central], 2, unused)) {
               metal = true;
            }
            fillColumn(left.levels,  leftElectrons  - (metal ? charge : 0));
            fillColumn(right.levels, rightElectrons
                                     - ((metal || donors) ? 0 : charge));

            return !(left.levels.empty() && right.levels.empty());
         }

         //  Say what it found, because "not this shape of molecule" and
         //  "the symmetry came out wrong" look identical otherwise.
         ostringstream why;
         why << "This kind of diagram describes a central atom with one set "
                "of equivalent neighbours (CH4, H2O, NH3, BF3), or two atoms "
                "of a diatomic. In "
             << group << " this molecule has " << orbits.size()
             << " symmetry-distinct set" << (orbits.size() == 1 ? "" : "s")
             << " of atoms, so there is no such split. Choose the fragments "
                "yourself, or read the molecular orbitals on their own.";
         note = why.str();
         return false;
      }
      leftSet.assign(1, central);
      rightSet = terminal;
      diatomic = (elements.size() == 2 && terminal.size() == 1);
   }

   if (leftAtoms  != 0) *leftAtoms  = leftSet;
   if (rightAtoms != 0) *rightAtoms = rightSet;

   //  A POLYATOMIC LIGAND CONTRIBUTES ITS DONOR ORBITAL, NOT ITS
   //  WHOLE BASIS.
   //
   //  Where the terminal fragment contains atoms that are not bonded
   //  to the central one, its ligands are molecules: hexammine
   //  cobalt's terminal set is six nitrogens and eighteen hydrogens,
   //  and symmetry orbitals built from every one of their atomic
   //  orbitals are correct and unreadable.  A ligand field diagram
   //  uses one sigma donor per ligand instead -- which in Oh spans
   //  a1g + eg + t1u whatever the ligand is, and is why the
   //  construction works for ammonia and water and CO alike.
   bool polyatomicLigands = false;
   vector<int> attachments;
   if (leftSet.size() == 1) {
      bondedTo(leftSet[0], rightSet, coordsUsed, attachments);
      //  AT LEAST TWO OF THEM.  One attachment is not a ligand
      //  field, it is a bond: methanol split as oxygen against the
      //  methyl group has a single attachment, and reducing CH4 to
      //  "one sigma donor" throws away the whole fragment.
      //
      //  In a skeleton the ligands ARE their attachment atoms by
      //  construction, so the count test cannot see them; the
      //  skeleton was built because they were molecules.
      polyatomicLigands = attachments.size() > 1 &&
                          (ligandField ||
                           attachments.size() < rightSet.size());
   }

   buildColumn(leftSet, elementsUsed, coordsUsed, numAtoms, images, classOfOp, ops,
               *table, diatomic, left);

   if (polyatomicLigands) {
      buildSigmaColumn(attachments, elementsUsed, numAtoms, images, classOfOp,
                       ops, *table, right);
      if (note.empty()) {
         ostringstream said;
         said << "Each ligand contributes one sigma donor orbital, as a "
                 "ligand field diagram does; the ligands' own internal "
                 "orbitals are not drawn.";
         note = said.str();
      }
   } else {
      buildColumn(rightSet, elementsUsed, coordsUsed, numAtoms, images, classOfOp,
                  ops, *table, diatomic, right);
   }

   //  --- electrons -------------------------------------------------
   //
   //  Each side brings what its atoms bring.  The molecular charge has
   //  to go on one of them, and symmetry cannot say which: the TOTAL
   //  is right either way, which is what the electron count on the
   //  diagram has to be, but WHICH COLUMN it comes off decides what
   //  the picture says about the molecule.
   //
   //  On the terminal set by default, where it usually sits
   //  chemically: nitrite is a nitrogen between two oxygens carrying
   //  the charge.
   //
   //  ON THE METAL WHEN THERE IS ONE.  A transition metal's charge is
   //  its oxidation state, and the d count that follows from it is the
   //  first thing anyone reads off a complex's diagram -- "d6, low
   //  spin" is the whole answer to most questions asked of one.
   //  Taking the charge off the ligands instead left cobalt(II) drawn
   //  with nine d electrons, which is cobalt(-I).
   int leftElectrons = 0, rightElectrons = 0;
   for (size_t i = 0; i < leftSet.size(); i++) {
      leftElectrons += valenceElectrons(elementsUsed[leftSet[i]]);
   }
   for (size_t i = 0; i < rightSet.size(); i++) {
      rightElectrons += valenceElectrons(elementsUsed[rightSet[i]]);
   }

   //  A METAL IS OXIDISED, NOT REDUCED, BY ITS COMPLEX'S CHARGE.
   //
   //  The charge goes on the metal because it is the oxidation state
   //  -- but only when it takes electrons AWAY.  A complex that is
   //  negative overall is negative because its LIGANDS are:
   //  [Co(CN)6]3- is cobalt(III) with six cyanides, not cobalt(-III),
   //  and putting the -3 on the metal drew cobalt with twelve valence
   //  electrons and an occupied 4s.
   bool chargeOnLeft = false;
   if (charge > 0) {
      for (size_t i = 0; i < leftSet.size(); i++) {
         double unused;
         if (valenceEnergy(elementsUsed[leftSet[i]], 2, unused)) {
            chargeOnLeft = true;
         }
      }
   }

   //  A DONOR BRINGS A PAIR.  That is what dative means, and it is
   //  the count a ligand field diagram fills its bonding set with:
   //  six ligands, twelve electrons, before any of the metal's own.
   if (polyatomicLigands) rightElectrons = 2*(int)attachments.size();

   fillColumn(left.levels,  leftElectrons  - (chargeOnLeft ? charge : 0));
   fillColumn(right.levels, rightElectrons
                            - ((chargeOnLeft || polyatomicLigands)
                                   ? 0 : charge));

   if (left.levels.empty() && right.levels.empty()) {
      note = "No valence orbital energies for " + elementsUsed[leftSet[0]] +
             " or " + elementsUsed[rightSet[0]] +
             " (the table is main group only).";
      return false;
   }
   return true;
}
