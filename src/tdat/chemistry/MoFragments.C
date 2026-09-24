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

   VoieEntry() : n(0), s(0.0), p(0.0), haveP(false) {}
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
      s_voie[symbol] = entry;
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
      level.label      = irreps[i];

      //  The multiplicity is worth saying: two T2 sets are two
      //  different things at the same height in this model, and a
      //  reader who sees one level labelled T2 will count three
      //  orbitals where there are six.
      ostringstream name;
      if (multiplicity[i] > 1) name << multiplicity[i] << char(0xd7) << ' ';
      name << irreps[i] << "  (" << shellName << ')';
      level.label = name.str();

      levels.push_back(level);
   }
}


bool MoFragments::build(const vector<double>& coords,
                        const vector<string>& elements,
                        const string& group,
                        MoColumn& left,
                        MoColumn& right,
                        string& note)
{
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
      note = "This molecule has no single central atom, so a "
             "central-atom / terminal-atom diagram does not describe it. "
             "The molecular orbitals are still shown.";
      return false;
   }

   //  --- the central atom's valence orbitals -----------------------
   vector<int> centralOnly(1, central);
   left.title = elements[central] + " orbitals";

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
   rightTitle << terminal.size() << char(0xd7) << ' ' << elements[terminal[0]]
              << " symmetry orbitals";
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

   if (left.levels.empty() && right.levels.empty()) {
      note = "No valence orbital energies for " + elements[central] +
             " or " + elements[terminal[0]] +
             " (the table is main group only).";
      return false;
   }
   return true;
}
