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


bool MoFragments::partition(const vector< vector<int> >& orbits,
                            const vector<string>& elements,
                            int& central,
                            vector<int>& terminal)
{
   central = -1;
   terminal.clear();

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
                      double energy, const string& shellName,
                      vector<MoLevel>& levels)
{
   const vector<string>& irreps = table.irreps();

   for (size_t i = 0; i < irreps.size() && i < multiplicity.size(); i++) {
      if (multiplicity[i] <= 0) continue;

      MoLevel level;
      level.energy     = energy;
      level.degeneracy = multiplicity[i]*table.dimension(irreps[i]);
      level.irrep      = MoDiagram::canonicalIrrep(irreps[i]);

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


bool MoFragments::build(const vector<double>& coords,
                        const vector<string>& elements,
                        const string& group,
                        int charge,
                        MoColumn& left,
                        MoColumn& right,
                        string& note,
                        vector<int>* leftAtoms,
                        vector<int>* rightAtoms)
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

   int central = -1;
   vector<int> terminal;
   if (!partition(orbits, elements, central, terminal)) {
      //  Say what it found, because "not this shape of molecule" and
      //  "the symmetry came out wrong" look identical otherwise.
      ostringstream why;
      why << "This kind of diagram describes a central atom with one set "
             "of equivalent neighbours (CH4, H2O, NH3, BF3). In "
          << group << " this molecule has " << orbits.size()
          << " symmetry-distinct set" << (orbits.size() == 1 ? "" : "s")
          << " of atoms, so there is no such split. The molecular "
             "orbitals are still shown.";
      note = why.str();
      return false;
   }

   //  --- the central atom's valence orbitals -----------------------
   //  THE COLUMNS ARE NAMED BY THE ATOMS THEY BELONG TO: "O" on one
   //  side, "2H TASOs" on the other, which is how the diagram is read
   //  and how the course names them.
   //  Which atoms ended up on each side, for a caller that wants to
   //  ask how much of a molecular orbital sits there.  Reported rather
   //  than left to be re-derived: repeating the orbit analysis outside
   //  is a second chance to disagree with this one.
   if (leftAtoms  != 0) leftAtoms->assign(1, central);
   if (rightAtoms != 0) *rightAtoms = terminal;

   vector<int> centralOnly(1, central);
   left.title = elements[central];

   for (int l = 0; l <= 1; l++) {
      double eV;
      if (!valenceEnergy(elements[central], l, eV)) continue;

      vector<int> multiplicity;
      if (!shellIrreps(centralOnly, l, numAtoms, images, classOfOp, ops,
                       *table, multiplicity)) {
         continue;
      }

      loadVoie();
      ostringstream shell;
      shell << s_voie[elements[central]].n << (l == 0 ? 's' : 'p');
      addLevels(*table, multiplicity, eV, shell.str(), left.levels);
   }

   //  --- the terminal atoms' symmetry orbitals ---------------------
   ostringstream rightTitle;
   rightTitle << terminal.size() << elements[terminal[0]] << " TASOs";
   right.title = rightTitle.str();

   for (int l = 0; l <= 1; l++) {
      double eV;
      if (!valenceEnergy(elements[terminal[0]], l, eV)) continue;

      vector<int> multiplicity;
      if (!shellIrreps(terminal, l, numAtoms, images, classOfOp, ops,
                       *table, multiplicity)) {
         continue;
      }

      loadVoie();
      ostringstream shell;
      shell << s_voie[elements[terminal[0]]].n << (l == 0 ? 's' : 'p');
      addLevels(*table, multiplicity, eV, shell.str(), right.levels);
   }

   //  --- electrons -------------------------------------------------
   //
   //  The molecular charge belongs to the fragment that is not the
   //  neutral reference; there is no way to say which from symmetry, so
   //  it goes on the terminal set, where it usually sits chemically
   //  (NO2- is a nitrogen between two oxygens carrying the charge).
   //  The TOTAL is right either way, which is what the electron count
   //  on the diagram has to be.
   fillColumn(left.levels, valenceElectrons(elements[central]));
   fillColumn(right.levels,
              (int)terminal.size()*valenceElectrons(elements[terminal[0]])
              - charge);

   if (left.levels.empty() && right.levels.empty()) {
      note = "No valence orbital energies for " + elements[central] +
             " or " + elements[terminal[0]] +
             " (the table is main group only).";
      return false;
   }
   return true;
}
