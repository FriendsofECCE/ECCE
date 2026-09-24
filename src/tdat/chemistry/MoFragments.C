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

   VoieEntry() : n(0), s(0.0), p(0.0), haveP(false), valence(0) {}
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
   const char *home = getenv("ECCE_HOME");
   string command = string(home ? home : ".") + "/bin/symops";
   if (access(command.c_str(), X_OK) != 0) command = "symops";

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
                      vector<MoLevel>& levels)
{
   const vector<string>& irreps = table.irreps();

   for (size_t i = 0; i < irreps.size() && i < multiplicity.size(); i++) {
      if (multiplicity[i] <= 0) continue;

      MoLevel level;
      level.energy     = energy;
      level.degeneracy = multiplicity[i]*table.dimension(irreps[i]);
      level.irrep      = MoDiagram::canonicalIrrep(irreps[i]);
      level.shell      = shell;

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

      for (int l = 0; l <= 1; l++) {
         double eV;
         if (!MoFragments::valenceEnergy(symbol, l, eV)) continue;

         loadVoie();
         ostringstream shell;
         shell << s_voie[symbol].n << (l == 0 ? 's' : 'p');
         if (byElement.size() > 1) shell << " " << symbol;

         if (ownOrbitals) {
            MoLevel level;
            level.energy     = eV;
            level.degeneracy = (l == 0) ? 1 : 3;
            level.label      = shell.str();
            level.shell      = l;
            column.levels.push_back(level);
            continue;
         }

         vector<int> multiplicity;
         if (!shellIrreps(mine, l, numAtoms, images, classOfOp, ops,
                          table, multiplicity)) {
            continue;
         }
         const size_t before = column.levels.size();
         addLevels(table, multiplicity, eV, shell.str(), l, column.levels);

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
            if (got && !vectors.empty()) column.levels[j].phases = vectors[0];
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

   const int numAtoms = (int)elements.size();

   vector< vector<int> > images;
   if (!SymmetryAnalysis::atomImages(coords, elements, ops, 1.0e-3, images)) {
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
      if (!partition(orbits, elements, central, terminal)) {
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

   buildColumn(leftSet, elements, coords, numAtoms, images, classOfOp, ops,
               *table, diatomic, left);
   buildColumn(rightSet, elements, coords, numAtoms, images, classOfOp, ops,
               *table, diatomic, right);

   //  --- electrons -------------------------------------------------
   //
   //  The molecular charge belongs to the fragment that is not the
   //  neutral reference; there is no way to say which from symmetry, so
   //  it goes on the terminal set, where it usually sits chemically
   //  (NO2- is a nitrogen between two oxygens carrying the charge).
   //  The TOTAL is right either way, which is what the electron count
   //  on the diagram has to be.
   //  Each side brings what its atoms bring.  The molecular charge
   //  goes on the right, where it usually sits chemically (NO2- is a
   //  nitrogen between two oxygens carrying the charge); the TOTAL is
   //  right either way, which is what the electron count on the
   //  diagram has to be.
   int leftElectrons = 0, rightElectrons = 0;
   for (size_t i = 0; i < leftSet.size(); i++) {
      leftElectrons += valenceElectrons(elements[leftSet[i]]);
   }
   for (size_t i = 0; i < rightSet.size(); i++) {
      rightElectrons += valenceElectrons(elements[rightSet[i]]);
   }
   fillColumn(left.levels, leftElectrons);
   fillColumn(right.levels, rightElectrons - charge);

   if (left.levels.empty() && right.levels.empty()) {
      note = "No valence orbital energies for " + elements[leftSet[0]] +
             " or " + elements[rightSet[0]] +
             " (the table is main group only).";
      return false;
   }
   return true;
}
