#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>
using std::ostringstream;
#include <algorithm>
#include <cmath>
#include <cstddef>

#include "tdat/MoDiagram.H"

MoDiagram::MoDiagram()
{
}


string MoDiagram::canonicalIrrep(const string& label)
{
  string out;
  for (string::size_type i = 0; i < label.size(); i++) {
    const char c = label[i];
    if (isspace((unsigned char)c)) continue;
    if (c == '"') { out += "''"; continue; }
    out += (char)toupper((unsigned char)c);
  }
  return out;
}


/**
 * How many orbitals an irrep holds, read from its name.
 *
 * The dimensions normally come from the character table, but a linear
 * molecule has no finite table to read -- and N2 came out with its pi
 * level split into "1pi" and "2pi" at one energy, which is not how a
 * doubly degenerate level is drawn or thought about.
 *
 * Mulliken's letters say the dimension outright, which is what they
 * are for: A and B are one, E is two, T is three, and for a linear
 * molecule Sigma is one while Pi, Delta and Phi are two.  Falling back
 * to this is better than assuming one, which silently turns every
 * degeneracy into a pile of coincident levels.
 */
string MoDiagram::formula(const vector<string>& elements, int charge)
{
  //  IUPAC'S ELEMENT SEQUENCE, NOT HILL ORDER.
  //
  //  Hill order -- carbon, hydrogen, then alphabetical -- is an index
  //  convention, and it writes ammonia H3N.  Nobody writes ammonia
  //  H3N.  The convention for a formula meant to be read is the
  //  sequence in IUPAC's Table VI, in which hydrogen falls between
  //  nitrogen and tellurium: so NH3 and CH4 and PH3, but H2O and H2S
  //  and HCl, which is what a chemist writes without thinking about
  //  it.  Anything not in the table follows, alphabetically, which is
  //  better than pretending to know.
  static const char* const SEQUENCE[] = {
    "Rn","Xe","Kr","Ar","Ne","He",
    "Fr","Cs","Rb","K","Na","Li",
    "Ra","Ba","Sr","Ca","Mg","Be",
    "Ac","La","Y","Sc",
    "Hf","Zr","Ti", "Ta","Nb","V", "W","Mo","Cr",
    "Re","Tc","Mn", "Os","Ru","Fe", "Ir","Rh","Co",
    "Pt","Pd","Ni", "Au","Ag","Cu", "Hg","Cd","Zn",
    "Tl","In","Ga","Al","B",
    "Pb","Sn","Ge","Si","C",
    "Bi","Sb","As","P","N",
    "H",
    "Po","Te","Se","S",
    "At","I","Br","Cl",
    "O",
    "F",
    0
  };

  map<string,int> count;
  for (size_t i = 0; i < elements.size(); i++) count[elements[i]]++;

  vector<string> order;
  for (int k = 0; SEQUENCE[k] != 0; k++) {
    if (count.count(SEQUENCE[k])) order.push_back(SEQUENCE[k]);
  }
  for (map<string,int>::const_iterator it = count.begin();
       it != count.end(); ++it) {
    bool known = false;
    for (int k = 0; SEQUENCE[k] != 0; k++) {
      if (it->first == SEQUENCE[k]) known = true;
    }
    if (!known) order.push_back(it->first);
  }

  ostringstream out;
  for (size_t i = 0; i < order.size(); i++) {
    out << order[i];
    if (count[order[i]] > 1) out << count[order[i]];
  }

  if (charge != 0) {
    const int size = (charge < 0) ? -charge : charge;
    out << '^';
    if (size > 1) out << size;
    out << ((charge < 0) ? '-' : '+');
  }
  return out.str();
}


int MoDiagram::dimensionFromName(const string& canonical)
{
  if (canonical.empty()) return 1;

  //  Linear molecules, as the codes spell them: SIG/SIU, PIU/PIG, ...
  if (canonical.compare(0, 2, "SI") == 0) return 1;
  if (canonical.compare(0, 2, "PI") == 0) return 2;
  if (canonical.compare(0, 2, "DE") == 0) return 2;
  if (canonical.compare(0, 2, "PH") == 0) return 2;

  switch (canonical[0]) {
    case 'A': case 'B': return 1;
    case 'E':           return 2;
    case 'T':           return 3;
    default:            return 1;
  }
}


bool MoDiagram::group(const vector<double>& energies,
                      const vector<double>& occupancies,
                      const vector<string>& labels,
                      double tolerance,
                      vector<MoLevel>& levels)
{
  levels.clear();

  const size_t n = energies.size();
  if (n == 0) return false;
  if (!occupancies.empty() && occupancies.size() != n) return false;
  if (!labels.empty() && labels.size() != n) return false;

  for (size_t i = 0; i < n; i++) {

    const string label = labels.empty() ? string("?") : labels[i];

    //  Same level as the one before it?  Both conditions matter: two
    //  orbitals of DIFFERENT irreps can be accidentally degenerate and
    //  are not one level, and two of the same irrep at different
    //  energies are two levels.
    bool extend = false;
    if (!levels.empty()) {
      MoLevel& last = levels.back();
      extend = (last.label == label)
               && (fabs(energies[i] - last.energy) <= tolerance);
    }

    if (extend) {
      MoLevel& last = levels.back();
      //  The level sits at the mean of its orbitals, so a set split by
      //  numerical noise is drawn where it belongs rather than at
      //  whichever of them happened to come first.
      last.energy = (last.energy*last.degeneracy + energies[i])
                    / (last.degeneracy + 1);
      last.degeneracy += 1;
      last.occupancy += occupancies.empty() ? 0.0 : occupancies[i];
      last.orbitals.push_back((int)i);
      last.energies.push_back(energies[i]);
    } else {
      MoLevel level;
      level.energy = energies[i];
      level.label = label;
      level.irrep = canonicalIrrep(label);
      level.occupancy = occupancies.empty() ? 0.0 : occupancies[i];
      level.degeneracy = 1;
      level.orbitals.push_back((int)i);
      level.energies.push_back(energies[i]);
      levels.push_back(level);
    }
  }

  //  A SPECTRUM WITH NO SYMMETRY LABELS STILL HAS ORDINALS.
  //
  //  Most calculations report no orbital symmetries at all, and those
  //  levels were labelled "?", which the canvas then declined to
  //  draw: a diagram of unnamed lines.  A plain numbering is honest
  //  and is what a spectrum without symmetry actually gives you.
  //  Numbered from the lowest, so the numbers match the page.
  if (labels.empty()) {
    for (size_t k = 0; k < levels.size(); k++) {
      char text[32];
      snprintf(text, sizeof(text), "%d", (int)k + 1);
      levels[k].label = text;
      levels[k].irrep.clear();
    }
  }
  return true;
}


static bool lowerEnergy(const MoLevel& a, const MoLevel& b)
{
  return a.energy < b.energy;
}


bool MoDiagram::groupByIrrep(const vector<double>& energies,
                             const vector<double>& occupancies,
                             const vector<string>& labels,
                             const map<string,int>& dimensions,
                             double tolerance,
                             vector<MoLevel>& levels)
{
  //  Without labels there is nothing to look a dimension up by.
  if (labels.empty() || labels.size() != energies.size()) {
    return group(energies, occupancies, labels, tolerance, levels);
  }

  levels.clear();
  const size_t n = energies.size();
  if (!occupancies.empty() && occupancies.size() != n) return false;

  for (size_t i = 0; i < n; ) {
    const string canonical = canonicalIrrep(labels[i]);

    map<string,int>::const_iterator found = dimensions.find(canonical);
    int wanted = (found == dimensions.end())
                     ? dimensionFromName(canonical) : found->second;
    if (wanted < 1) wanted = 1;

    //  Take up to `wanted` consecutive orbitals with the same label.
    //  Fewer is possible and is not an error: an open shell, or a
    //  calculation whose last printed orbital falls mid-set.
    //  AND AT THE SAME ENERGY.  Partners of a degenerate set are
    //  printed consecutively and agree to the last digit; two
    //  same-irrep orbitals half a Hartree apart are two different
    //  levels that happen to be adjacent in the list.  Without this,
    //  cobalt's occupied bonding t1u and an empty virtual t1u were
    //  swept into one level, which then reported zero electrons in a
    //  bonding orbital and sat out of energy order.
    size_t take = 0;
    while (take < (size_t)wanted && i + take < n &&
           canonicalIrrep(labels[i + take]) == canonical &&
           fabs(energies[i + take] - energies[i]) <= tolerance) {
      take++;
    }
    if (take == 0) take = 1;

    MoLevel level;
    level.irrep      = canonical;
    level.label      = labels[i];
    level.degeneracy = (int)take;
    level.occupancy  = 0.0;

    double sum = 0.0;
    for (size_t k = 0; k < take; k++) {
      sum += energies[i + k];
      if (!occupancies.empty()) level.occupancy += occupancies[i + k];
      level.orbitals.push_back((int)(i + k));
      level.energies.push_back(energies[i + k]);
    }
    //  The level sits at the mean of its orbitals, so a set split by
    //  finite precision is drawn where it belongs.
    level.energy = sum/take;

    levels.push_back(level);
    i += take;
  }

  //  IN ENERGY ORDER, WHATEVER ORDER THEY ARRIVED IN.  Everything
  //  downstream -- the numbering, the bonding/antibonding counting,
  //  the row grouping -- reads this as a spectrum, and a code is free
  //  to print its orbitals grouped by symmetry block instead.
  std::sort(levels.begin(), levels.end(), lowerEnergy);

  //  NUMBER THE LEVELS WITHIN EACH IRREP: 1a1, 2a1, 1t2, 2t2.
  //
  //  This is how a diagram is labelled and how orbitals are referred to
  //  in text, and it is the difference between something that reads as
  //  a diagram and something that reads as a column of output.  MOPAC
  //  prints its own orbitals this way ("1 a1  1 t2 ... 2 a1"); ECCE's
  //  parser keeps only the symbol, so the count is redone here from the
  //  order the levels come in, which is the same rule: counting up from
  //  the lowest.
  //  A SPECTRUM WITH NO SYMMETRY LABELS STILL HAS ORDINALS.
  //
  //  Most calculations report no orbital symmetries at all, and those
  //  levels were labelled "?", which the canvas then declined to
  //  draw: a diagram of unnamed lines, where a plain numbering is
  //  both honest and what a spectrum without symmetry actually gives
  //  you.  Numbered from the lowest drawn level, so the numbers match
  //  what is on the page.
  //  "?" is the placeholder group() uses when the caller supplied no
  //  labels at all, so it is an absence of symmetry and not a name.
  bool anyIrrep = false;
  for (size_t k = 0; k < levels.size(); k++) {
    if (!levels[k].irrep.empty() && levels[k].irrep != "?") anyIrrep = true;
  }

  map<string,int> seen;
  for (size_t k = 0; k < levels.size(); k++) {
    char text[64];
    if (!anyIrrep) {
      snprintf(text, sizeof(text), "%d", (int)k + 1);
    } else {
      const int n = ++seen[levels[k].irrep];
      snprintf(text, sizeof(text), "%d%s", n, levels[k].label.empty()
               ? levels[k].irrep.c_str() : levels[k].label.c_str());
    }
    levels[k].label = text;
  }
  return !levels.empty();
}


static bool equivalentColumns(const vector<MoLevel>& a,
                              const vector<MoLevel>& b);

void MoDiagram::placeFragments(const MoColumn& centre,
                               MoColumn& left, MoColumn& right,
                               const vector<MoConnection>& connections)
{
  if (centre.levels.empty()) return;

  //  A FRAGMENT LEVEL SITS AT THE MEAN OF THE ORBITALS IT BECAME.
  //
  //  Not at its tabulated ionisation energy mapped onto this axis.
  //  That number is a free atom's, measured by somebody else, and
  //  where it lands has no relation to the orbitals the calculation
  //  actually produced: nitrogen's 2s came out ABOVE the 1-sigma-u
  //  built from it, which is impossible -- an antibonding combination
  //  is above both its parents by definition, as a bonding one is
  //  below them.
  //
  //  The calculation already says where the level belongs.  A fragment
  //  orbital is spread over the molecular orbitals it combined into,
  //  so its energy is their mean weighted by how much of each it makes
  //  up -- which is the fragment-orbital energy of a fragment
  //  analysis, and lies inside their range by construction.  The
  //  bonding partners are then below it and the antibonding ones
  //  above, not by arrangement but because that is what the words
  //  mean.
  //
  //  The tabulated energy is kept as the annotation, where it belongs:
  //  it is worth reading, and it is not this axis.
  MoColumn* cols[2] = { &left, &right };
  int c;

  //  TWO EQUIVALENT HALVES CARRY NO PLACEMENT INFORMATION.
  //
  //  A fragment level normally sits at the share-weighted mean of
  //  the orbitals it became, and that works because the shares say
  //  something.  Where the two fragments are equivalent they say
  //  nothing: each half holds exactly half of every orbital, by
  //  symmetry and not by accident, so every level of every shell
  //  gets the same weights and the column comes out scrambled --
  //  ethene drew its carbon 2s above its 2p and its 2p three
  //  Hartree below the lowest molecular orbital.
  //
  //  There the tabulated energies are the only ordering there is,
  //  and they are the ordering a person draws: carbon 2s below
  //  carbon 2p, hydrogen 1s between them.  Mapped onto the range the
  //  molecular orbitals occupy, so the two columns sit beside the
  //  spectrum rather than in eV somewhere off the page.
  if (equivalentColumns(left.levels, right.levels)) {
    double lowTab = 1.0e30, highTab = -1.0e30;
    for (size_t i = 0; i < left.levels.size(); i++) {
      const double e = left.levels[i].energy;
      if (e < lowTab)  lowTab = e;
      if (e > highTab) highTab = e;
    }

    double lowMo = 1.0e30, highMo = -1.0e30;
    for (size_t i = 0; i < centre.levels.size(); i++) {
      const double e = centre.levels[i].energy;
      if (e < lowMo)  lowMo = e;
      if (e > highMo) highMo = e;
    }

    if (highTab > lowTab && highMo > lowMo) {
      //  Inside the spectrum, not spanning it: fragment levels lie
      //  between the bonding orbitals they make and the antibonding
      //  ones, never outside both.
      const double margin = 0.2*(highMo - lowMo);
      const double bottom = lowMo + margin;
      const double top    = highMo - margin;
      const double scale  = (top - bottom)/(highTab - lowTab);

      for (c = 0; c < 2; c++) {
        for (size_t i = 0; i < cols[c]->levels.size(); i++) {
          MoLevel& level = cols[c]->levels[i];

          //  KEEP WHAT THE LEVEL ALREADY SAID ABOUT ITSELF.
          //
          //  This wrote "free atom" over the annotation unconditionally
          //  -- including over "in phase" and "out of phase", which is
          //  the one thing a two-halves diagram is drawn to show. And
          //  it is not true of every column: a fragment level built
          //  from the fragment's OWN orbitals carries that orbital's
          //  energy, not an atom's.
          level.tabulated = level.energy;
          char text[64];
          snprintf(text, sizeof(text), "%.2f Ha", level.energy/27.211386);
          if (level.annotation.empty()) {
            level.annotation = string("free atom ") + text;
          } else {
            level.annotation += string(", ") + text;
          }
          level.energy = bottom + scale*(level.energy - lowTab);
        }
      }
      return;
    }
  }

  //  WHICH LEVELS WERE ACTUALLY PLACED, RECORDED RATHER THAN GUESSED.
  //
  //  A level that connected to nothing keeps the tabulated value it
  //  arrived with, which is in eV -- and the test for "still in eV"
  //  was whether the number was above -100.  Every eV value in the
  //  table is above -100, and so is every Hartree one, so the test
  //  never discriminated: cobalt's 4p, which has no ligand partner,
  //  was drawn at -3.8 on a Hartree axis and dragged the whole scale
  //  with it.
  vector<bool> placed[2];

  for (c = 0; c < 2; c++) {
    for (size_t i = 0; i < cols[c]->levels.size(); i++) {
      MoLevel& level = cols[c]->levels[i];

      //  IN THE AXIS'S UNITS, AND SAID TO BE THE FREE ATOM'S.  An eV
      //  number beside a level on a Hartree axis reads as that level's
      //  energy and disagrees with where it is drawn.  It is the free
      //  atom's tabulated value, which is a different quantity: the
      //  level is placed at the mean of the orbitals it became.
      level.tabulated = level.energy;
      char text[64];
      snprintf(text, sizeof(text), "%.2f Ha", level.energy/27.211386);
      if (level.annotation.empty()) {
        level.annotation = string("free atom ") + text;
      } else {
        level.annotation += string(", ") + text;
      }

      double sum = 0.0, weight = 0.0;
      for (size_t k = 0; k < connections.size(); k++) {
        const MoConnection& link = connections[k];
        const int mine = (c == 0) ? link.leftLevel : link.rightLevel;
        if (mine != (int)i) continue;
        if (link.centreLevel < 0 ||
            link.centreLevel >= (int)centre.levels.size()) continue;

        const MoLevel& mo = centre.levels[link.centreLevel];

        //  Weighted by the share of that orbital this fragment holds,
        //  where a share was computed, and evenly where none was.
        //  THE OVERLAP WITH THIS ORBITAL, WHERE IT IS KNOWN.
        //
        //  This is what a fragment level actually is -- a symmetry
        //  orbital, not a shell -- and the share on a shell is the
        //  same for every symmetry orbital built from it.  Water's
        //  two hydrogen combinations took identical weights and were
        //  placed at one energy; the in-phase one is stabilised more
        //  and the diagram exists to show that.
        double w = 1.0;
        bool weighted = false;
        if (link.centreLevel < (int)level.moWeight.size()) {
          w = level.moWeight[link.centreLevel];
          weighted = true;
        }

        const int which = (level.slot >= 0) ? level.slot : level.shell;
        if (!weighted && which >= 0) {
          const vector<double>& shares = (c == 0) ? mo.shellLeft
                                                  : mo.shellRight;
          if ((int)shares.size() > which) w = shares[which];
        } else if (!weighted) {
          const double share = (c == 0) ? mo.shareLeft : mo.shareRight;
          if (share >= 0.0) w = share;
        }
        if (w <= 0.0) continue;

        //  Each orbital of a degenerate level counts once.
        const double n = (mo.degeneracy > 0) ? mo.degeneracy : 1;
        sum += w*n*mo.energy;
        weight += w*n;
      }

      if (weight > 0.0) {
        level.energy = sum/weight;
        placed[c].push_back(true);
      } else {
        placed[c].push_back(false);
      }
    }
  }

  //  ONE SHELL, ONE LEVEL.
  //
  //  A free atom's 2p set is degenerate -- that is what a shell is.
  //  Placing each of its symmetry components at the mean of the
  //  orbitals IT became splits the set apart by wherever those
  //  orbitals happened to land, which is a construct of the molecule
  //  and not a property of the atom: water's oxygen came out with its
  //  b1, b2 and a1 2p components at three different heights, spread
  //  over a tenth of a Hartree, and the b1 -- whose molecular orbital
  //  is the untouched out-of-plane lone pair, identical in energy --
  //  no longer lined up with it.
  //
  //  Every level built from one shell is placed at their common mean,
  //  weighted the same way.  Drawn, they are one row of lines side by
  //  side, which is what a person draws: "O 2p" with three short
  //  lines labelled a1, b1, b2.
  //
  //  The shell is identified by the energy the level arrived with,
  //  which is the shell's own tabulated value and identical across
  //  its components -- so this needs nothing the levels do not
  //  already carry.
  for (c = 0; c < 2; c++) {
    vector<MoLevel>& levels = cols[c]->levels;
    if (levels.size() < 2) continue;

    //  What each level arrived with, from the level itself rather
    //  than from the caption written for a reader.
    vector<double> tabulated(levels.size(), 0.0);
    for (size_t i = 0; i < levels.size(); i++) {
      tabulated[i] = levels[i].tabulated/27.211386;
    }

    vector<bool> done(levels.size(), false);
    for (size_t i = 0; i < levels.size(); i++) {
      if (done[i]) continue;

      double sum = 0.0;
      int count = 0;
      //  AN ATOM'S SHELL IS DEGENERATE.  A SET OF SYMMETRY ORBITALS
      //  IS NOT.
      //
      //  Snapping a shell to one height is right for a central atom:
      //  its 2p really is threefold degenerate and drawing the three
      //  components apart invents a splitting.  It is wrong for
      //  terminal-atom symmetry orbitals, which are combinations of
      //  several atoms and differ from each other by construction --
      //  water's in-phase hydrogen combination is stabilised and its
      //  out-of-phase one is not, and averaging them put both at
      //  exactly the midpoint and threw the distinction away.
      //
      //  A level carrying a phase pattern is such a combination.
      if (!levels[i].phases.empty()) { done[i] = true; continue; }

      for (size_t j = i; j < levels.size(); j++) {
        if (done[j]) continue;
        if (!levels[j].phases.empty()) continue;
        if (fabs(tabulated[j] - tabulated[i]) > 1.0e-9) continue;
        if (levels[j].shell != levels[i].shell) continue;
        sum += levels[j].energy;
        count++;
      }
      if (count == 0) continue;

      const double common = sum/count;
      for (size_t j = i; j < levels.size(); j++) {
        if (done[j]) continue;
        if (!levels[j].phases.empty()) continue;
        if (fabs(tabulated[j] - tabulated[i]) > 1.0e-9) continue;
        if (levels[j].shell != levels[i].shell) continue;
        levels[j].energy = common;
        done[j] = true;
      }
    }
  }

  //  A COLUMN THAT CONNECTED TO NOTHING AT ALL.
  //
  //  The rescue below places a lone unconnected level relative to the
  //  ones that were placed, which needs at least one placed level to
  //  measure from.  When NOTHING in either column connected there is
  //  no such anchor, and every level kept the tabulated value it
  //  arrived with -- an eV number on a Hartree axis.  Water drew its
  //  oxygen 2s at -32 and its 2p at -15.9 while the molecular orbitals
  //  sat between -1 and 0, putting the fragments off the bottom of the
  //  picture and squashing the spectrum into a line at the top.
  //  Methane did the same.  That is what a calculation with no usable
  //  symmetry labels and no usable composition produces.
  //
  //  Such a diagram is still worth drawing: the tabulated energies are
  //  the ordering a person uses before doing any calculation at all.
  //  They are mapped onto the range the molecular orbitals occupy --
  //  with ONE scale across both columns, not one each.  Scaling each
  //  column to its own range is what makes oxygen's 2p, the highest
  //  level in its column, come out ABOVE hydrogen's 1s: the columns
  //  are at different heights precisely because the elements differ,
  //  and that is the whole content of the picture.
  {
    int placedTotal = 0;
    for (c = 0; c < 2; c++) {
      for (size_t i = 0; i < placed[c].size(); i++) {
        if (placed[c][i]) placedTotal++;
      }
    }

    bool anyLevels = !left.levels.empty() || !right.levels.empty();

    if (placedTotal == 0 && anyLevels) {
      double lowTab = 1.0e30, highTab = -1.0e30;
      for (c = 0; c < 2; c++) {
        for (size_t i = 0; i < cols[c]->levels.size(); i++) {
          const double e = cols[c]->levels[i].energy;
          if (e < lowTab)  lowTab = e;
          if (e > highTab) highTab = e;
        }
      }

      //  ONTO THE OCCUPIED ORBITALS, NOT THE WHOLE SPECTRUM.
      //
      //  The virtuals run far above the valence -- water's reach +0.94
      //  Hartree against a HOMO at -0.31 -- so mapping onto the full
      //  range lifts every fragment level into the empty orbitals.
      //  Oxygen's 2p came out at +0.4, above every occupied orbital
      //  in the molecule, which is not a statement anyone would make.
      //
      //  The occupied set is the window a fragment orbital belongs in:
      //  it is what the fragments' electrons went into.  Mapped there,
      //  oxygen's 2s lands at the bottom of the valence, its 2p among
      //  the lone pairs and the hydrogen combinations just above it --
      //  which is the diagram a person draws.
      double lowMo = 1.0e30, highMo = -1.0e30;
      for (size_t i = 0; i < centre.levels.size(); i++) {
        if (centre.levels[i].occupancy <= 0.0) continue;
        const double e = centre.levels[i].energy;
        if (e < lowMo)  lowMo = e;
        if (e > highMo) highMo = e;
      }
      //  Nothing occupied at all (an empty-shell oddity): fall back to
      //  the whole spectrum rather than to eV.
      if (highMo <= lowMo) {
        lowMo = 1.0e30; highMo = -1.0e30;
        for (size_t i = 0; i < centre.levels.size(); i++) {
          const double e = centre.levels[i].energy;
          if (e < lowMo)  lowMo = e;
          if (e > highMo) highMo = e;
        }
      }

      if (highMo > lowMo) {
        const double bottom = lowMo;
        const double top    = highMo;

        if (highTab > lowTab) {
          const double scale = (top - bottom)/(highTab - lowTab);
          for (c = 0; c < 2; c++) {
            for (size_t i = 0; i < cols[c]->levels.size(); i++) {
              MoLevel& level = cols[c]->levels[i];
              level.energy = bottom + scale*(level.energy - lowTab);
            }
          }
        } else {
          //  Every fragment level at the same tabulated energy: put
          //  them all in the middle rather than leaving them in eV.
          const double middle = 0.5*(bottom + top);
          for (c = 0; c < 2; c++) {
            for (size_t i = 0; i < cols[c]->levels.size(); i++) {
              cols[c]->levels[i].energy = middle;
            }
          }
        }
      }
      return;
    }
  }

  //  A level that connected to nothing keeps its tabulated energy, and
  //  would then be an eV value on a Hartree axis -- off the bottom of
  //  the picture.  Put it at the mean of the levels that were placed,
  //  keeping its order among them, so it is visible and obviously not
  //  bonded to anything.
  for (c = 0; c < 2; c++) {
    vector<MoLevel>& levels = cols[c]->levels;

    double placedSum = 0.0;
    int placedCount = 0;
    for (size_t i = 0; i < levels.size() && i < placed[c].size(); i++) {
      if (placed[c][i]) { placedSum += levels[i].energy; placedCount++; }
    }

    if (placedCount == 0) continue;

    //  Above everything that was placed, not among them: a fragment
    //  orbital with no partner in the molecule did not go down in
    //  energy, and putting it at the mean says it did.
    double highest = -1.0e30;
    for (size_t i = 0; i < levels.size() && i < placed[c].size(); i++) {
      if (placed[c][i] && levels[i].energy > highest) {
        highest = levels[i].energy;
      }
    }
    const double spread = highest - placedSum/placedCount;
    const double fallback = highest + (spread > 0.0 ? 0.25*spread : 0.05);

    for (size_t i = 0; i < levels.size(); i++) {
      if (i >= placed[c].size() || !placed[c][i]) levels[i].energy = fallback;
    }
  }
}



void MoDiagram::hideBelow(MoColumn& column, double cutoff)
{
  vector<MoLevel> kept;
  column.hiddenCount = 0;
  column.hiddenMaxEnergy = 0.0;
  bool anyHidden = false;

  for (size_t i = 0; i < column.levels.size(); i++) {
    if (column.levels[i].energy < cutoff) {
      column.hiddenCount += column.levels[i].degeneracy;
      if (!anyHidden || column.levels[i].energy > column.hiddenMaxEnergy) {
        column.hiddenMaxEnergy = column.levels[i].energy;
      }
      anyHidden = true;
    } else {
      kept.push_back(column.levels[i]);
    }
  }
  column.levels = kept;
}


void MoDiagram::hideAbove(MoColumn& column, double cutoff)
{
  vector<MoLevel> kept;
  int hidden = 0;

  for (size_t i = 0; i < column.levels.size(); i++) {
    if (column.levels[i].energy > cutoff) {
      hidden += column.levels[i].degeneracy;
    } else {
      kept.push_back(column.levels[i]);
    }
  }
  column.levels = kept;
  column.hiddenAboveCount = hidden;
}


void MoDiagram::hideBeyondValence(MoColumn& centre, int room)
{
  //  A QUALITATIVE DIAGRAM IS THE SIZE OF A MINIMAL VALENCE BASIS.
  //
  //  NO2- and O3 have twelve valence orbitals between them -- four on
  //  the central atom and four on each oxygen -- and that is how many
  //  a person draws.  A split-valence or larger basis produces more:
  //  one calculation reported twenty levels, and the eight extra empty
  //  ones are artefacts of the extra basis functions.  They correspond
  //  to nothing on either fragment column and have no place here.
  //
  //  suggestVirtualCutoff() folds on gaps in the spectrum, which is the
  //  right instinct but cannot know how many levels the picture should
  //  have.  The elements do know, so the count comes from them.
  //  The count is passed in rather than worked out here: asking
  //  MoFragments would make this file depend on the whole symmetry
  //  stack for one integer.
  if (room <= 0) return;

  //  The core has already been folded away, so what is left is the
  //  valence and whatever sits above it.
  int shown = 0;
  size_t keep = centre.levels.size();
  for (size_t i = 0; i < centre.levels.size(); i++) {
    const int here = (centre.levels[i].degeneracy > 0)
                         ? centre.levels[i].degeneracy : 1;
    if (shown + here > room) { keep = i; break; }
    shown += here;
  }

  //  NEVER FOLD AN OCCUPIED LEVEL AWAY.  An orbital with electrons in
  //  it is part of the molecule whatever the basis did, and hiding one
  //  would misstate the electron count the diagram shows.
  for (size_t i = keep; i < centre.levels.size(); i++) {
    if (centre.levels[i].occupancy > 0.0) keep = i + 1;
  }

  if (keep < centre.levels.size()) {
    hideAbove(centre, centre.levels[keep].energy - 1.0e-9);
  }
}


double MoDiagram::suggestVirtualCutoff(const vector<MoLevel>& levels)
{
  if (levels.empty()) return 1.0e30;

  //  Count what is occupied, and where the virtuals start.
  size_t occupied = 0;
  size_t firstVirtual = levels.size();
  for (size_t i = 0; i < levels.size(); i++) {
    if (levels[i].occupancy > 0.0) {
      occupied++;
    } else if (firstVirtual == levels.size()) {
      firstVirtual = i;
    }
  }
  if (occupied == 0 || firstVirtual >= levels.size()) return 1.0e30;

  //  A CHASM AMONG THE VIRTUALS IS A FOLD WHEREVER IT FALLS.
  //
  //  Counting levels alone let CCl4 through: it has as many virtual
  //  levels as occupied ones, so nothing was folded, and its far
  //  virtual block -- two and a half Hartree above the antibonding
  //  orbitals, which is not a chemical distance -- set the scale.
  //  Every bonding level in the molecule was then squeezed into the
  //  bottom sixth of the picture.
  //
  //  Judged against the molecule's own spread rather than an absolute:
  //  a gap several times anything inside the occupied manifold is not
  //  part of the same story.
  double widestOccupied = 0.0;
  for (size_t i = 1; i < firstVirtual; i++) {
    const double gap = levels[i].energy - levels[i-1].energy;
    if (gap > widestOccupied) widestOccupied = gap;
  }
  const double chasm = (widestOccupied > 0.0) ? 3.0*widestOccupied : 0.5;

  double byGap = 1.0e30;
  for (size_t i = firstVirtual + 1; i < levels.size(); i++) {
    if (levels[i].energy - levels[i-1].energy > chasm) {
      byGap = 0.5*(levels[i].energy + levels[i-1].energy);
      break;
    }
  }

  //  One antibonding partner per occupied level, plus two, so a small
  //  molecule keeps a little room above the LUMO.
  const size_t keep = occupied + 2;
  const size_t last = firstVirtual + keep;

  double byCount = 1.0e30;
  if (last < levels.size()) {
    byCount = 0.5*(levels[last].energy + levels[last-1].energy);
  }

  return (byGap < byCount) ? byGap : byCount;
}


double MoDiagram::suggestCoreCutoff(const vector<MoLevel>& levels,
                                    double minimumGap)
{
  if (levels.size() < 2) return -1.0e30;

  //  ONLY GAPS BETWEEN OCCUPIED LEVELS, and only ones bigger than a
  //  real core/valence separation.
  //
  //  The previous rule looked for the biggest gap in the lower half of
  //  the whole spectrum, judged against the median gap.  In a small
  //  molecule the biggest gap anywhere is the HOMO-LUMO gap, and in the
  //  lower half of a four-level spectrum that is the gap it found: for
  //  methane it put the cutoff just above the 1t2 level and folded away
  //  BOTH bonding levels, leaving a "diagram" of the two antibonding
  //  ones.  A core orbital is occupied and a core gap is enormous, so
  //  say both of those instead of inferring them.
  //
  //  minimumGap is in the same unit as the energies.  Nothing in a
  //  valence spectrum comes close to it: a 2s/2p separation is around
  //  half a Hartree, while carbon's 1s sits ten Hartree below its
  //  valence and oxygen's twenty.
  double bestGap = 0.0;
  size_t bestAt = 0;

  for (size_t i = 1; i < levels.size(); i++) {
    //  Both sides occupied: the gap ABOVE the highest occupied level
    //  is the HOMO-LUMO gap, which is not a core/valence split however
    //  large it is.
    if (levels[i].occupancy <= 0.0 || levels[i-1].occupancy <= 0.0) continue;

    //  THE HIGHEST BIG GAP, NOT THE BIGGEST.
    //
    //  A many-electron atom has a gap at every shell, and the deepest
    //  is always the largest: cobalt's 1s sits 243 Hartree below its
    //  2s, while the 3p/valence separation that actually divides core
    //  from valence is under two.  Taking the biggest folded the 1s
    //  alone and left 2s, 2p, 3s and 3p in the picture -- which then
    //  made the reconciliation compare an all-electron spectrum
    //  against a valence-only fragment set, find 15 t1u against 6,
    //  and give up, so CoH6(2+) drew no correlation lines at all.
    //
    //  Every gap over the threshold is a shell boundary.  The one
    //  that separates core from valence is the last of them.
    const double gap = levels[i].energy - levels[i-1].energy;
    if (gap >= minimumGap) { bestGap = gap; bestAt = i; }
  }

  if (bestGap < minimumGap) return -1.0e30;   // hides nothing

  return 0.5*(levels[bestAt].energy + levels[bestAt-1].energy);
}


/**
 * Does a molecular level draw enough from a fragment level's shell?
 *
 * This is what connects a diatomic.  Its columns carry no irrep, so
 * there is nothing to match on but the shell a fragment level is --
 * and a molecular orbital's share in that shell is exactly the
 * question a diatomic's correlation lines answer: how much of this
 * sigma-g is 2s, and how much is 2pz.
 */
static bool matchesShell(const MoLevel& centre, const MoLevel& fragment,
                         bool onLeft, double cutoff)
{
  if (fragment.shell < 0) return false;

  const vector<double>& shares = onLeft ? centre.shellLeft
                                        : centre.shellRight;
  if ((int)shares.size() <= fragment.shell) return false;
  if (shares[fragment.shell] < cutoff) return false;

  //  AND A REAL SHARE OF THIS ORBITAL, NOT A TRACE OF IT.
  //
  //  An absolute cutoff alone connects a level to every shell it has
  //  any amplitude on, and a molecular orbital has a little amplitude
  //  on nearly everything.  N2's 1-sigma-u is built from the 2s pair
  //  and carries a few per cent of 2p; drawn as a 2p interaction too,
  //  it then sat between its two "parents" and was classified
  //  non-bonding -- a diatomic's 2s antibonding orbital labelled nb,
  //  with the line clutter to match.
  //
  //  So a shell must also hold its own against the one that dominates
  //  this orbital.  A genuinely mixed orbital -- water's a1 from 2s
  //  and 2pz -- keeps both, which is the case the multiple lines
  //  exist for.
  double dominant = 0.0;
  for (size_t i = 0; i < shares.size(); i++) {
    if (shares[i] > dominant) dominant = shares[i];
  }
  return shares[fragment.shell] >= 0.35*dominant;
}


static int countOf(const vector<MoLevel>& levels, const string& irrep)
{
  int n = 0;
  for (size_t i = 0; i < levels.size(); i++) {
    if (levels[i].irrep == irrep) n++;
  }
  return n;
}


/** How many ORBITALS of each irrep a set of columns holds.
 *
 *  Orbitals, not levels.  One fragment level can stand for several --
 *  "2x A1 (2p)" is two a1 orbitals drawn as one row, and nitrite's
 *  oxygen 2p set has two such -- so counting rows made the two sides
 *  disagree on how much of each irrep they held and the reconciliation
 *  gave up on a molecule whose labels only needed b1 and b2 exchanged.
 */
static map<string,int> tally(const vector<MoLevel>& a,
                             const vector<MoLevel>& b)
{
  map<string,int> out;
  const vector<MoLevel>* both[2] = { &a, &b };
  for (int k = 0; k < 2; k++) {
    for (size_t i = 0; i < both[k]->size(); i++) {
      const MoLevel& level = (*both[k])[i];
      if (level.irrep.empty()) continue;
      out[level.irrep] += (level.degeneracy > 0) ? level.degeneracy : 1;
    }
  }
  return out;
}


static void relabel(vector<MoLevel>& levels, const string& from,
                    const string& to)
{
  for (size_t i = 0; i < levels.size(); i++) {
    if (levels[i].irrep != from) continue;
    levels[i].irrep = to;

    //  The drawn label carries the name too ("B1  (2p)").
    const string::size_type at = levels[i].label.find(from);
    if (at != string::npos) levels[i].label.replace(at, from.size(), to);
  }
}


/**
 * Does the calculation cover everything the fragments span?
 *
 * NOT EQUALITY.  The fragment model is minimal-valence: one s and one
 * p shell an atom, five d for a metal.  A real calculation is not --
 * even 3-21G gives cobalt more virtual orbitals than that, so the two
 * multisets cannot agree and never will.  Demanding they agree meant
 * no calculation with a real basis set was ever correlated by
 * symmetry: CoH6(2+) reported 9 t1u against the model's 6 and drew
 * nothing at all.
 *
 * What has to be true is that every irrep the fragments span is there
 * to be matched.  A surplus of virtuals is not a disagreement; a
 * DEFICIT is, and that is what the axis-convention swap below shows
 * up as.
 */
static bool covers(const map<string,int>& have, const map<string,int>& want)
{
  for (map<string,int>::const_iterator it = have.begin();
       it != have.end(); ++it) {
    map<string,int>::const_iterator found = want.find(it->first);
    if (found == want.end() || found->second < it->second) return false;
  }
  return true;
}



/**
 * One irrep's name under an exchange of two axes.
 *
 * In D2h the subscript on a B irrep names which twofold axis the
 * orbital is antisymmetric about, so calling a different perpendicular
 * direction "y" turns every b2 into a b3 AND every b2u into a b3u --
 * the exchange acts on both parities at once.  The A irreps are
 * unaffected: they are symmetric about every axis, which is why the
 * digit is only touched on a B.  (That also keeps C2v honest, where
 * swapping the mirror planes exchanges B1 and B2 but must leave A1 and
 * A2 alone.)
 */
static string axisExchange(const string& name, const char* mapping)
{
  if (name.empty()) return name;
  const char letter = toupper(name[0]);
  if (letter != 'B') return name;

  string renamed = name;
  for (size_t i = 1; i < renamed.size(); i++) {
    if (renamed[i] >= '1' && renamed[i] <= '3') {
      renamed[i] = mapping[renamed[i] - '1'];
    }
  }
  return renamed;
}

bool MoDiagram::reconcile(vector<MoLevel>& left, vector<MoLevel>& right,
                          const vector<MoLevel>& centre, string& why)
{
  why.clear();

  const vector<MoLevel> none;
  map<string,int> want = tally(centre, none);
  map<string,int> have = tally(left, right);
  if (want.empty() || have.empty()) return true;
  if (covers(have, want)) return true;

  //  AN EXCHANGE OF AXES FIRST, because that is the one relabelling
  //  with a reason behind it rather than a coincidence of counts.
  //
  //  Which perpendicular direction a code calls y and which it calls z
  //  is a convention, and changing it renames b2 to b3 in EVERY irrep
  //  family at once. Ethene in D2h is the case that forced this: the
  //  calculation reported 1B2G + 2B2U + 3B3U where the symmetry
  //  analysis gave 1B3G + 3B2U + 2B3U, which is the same set under one
  //  exchange of b2 and b3 -- and the single-pair search below could
  //  never find it, because no ONE swap fixes both parities.
  //  EVERY permutation of the three axes, not only the swaps.
  //
  //  Naming the axes differently need not be a transposition: a
  //  fragment analysis and a calculation can differ by a CYCLE, x to
  //  z to y. Ethene built from two CH2 orbitals does exactly that --
  //  the fragments span B1g2 B3g1 B1u1 B2u3 B3u2 where the
  //  calculation reports B3g2 B2g1 B3u1 B1u3 B2u2, the same multiset
  //  under 1->3->2->1 -- and no exchange of a single pair can reach
  //  it. All six permutations are tried; the identity is skipped
  //  because it is the case already handled above.
  static const char* const axisOrder[] =
      { "132", "213", "321", "231", "312" };
  for (int which = 0; which < 5; which++) {
    const char* const mapping = axisOrder[which];

    map<string,int> tried;
    bool changed = false;
    for (map<string,int>::const_iterator it = have.begin();
         it != have.end(); ++it) {
      const string renamed = axisExchange(it->first, mapping);
      if (renamed != it->first) changed = true;
      tried[renamed] += it->second;
    }
    if (!changed || !covers(tried, want)) continue;

    //  Apply it through a marker, so two names cannot overwrite one
    //  another halfway through the permutation.
    for (map<string,int>::const_iterator it = have.begin();
         it != have.end(); ++it) {
      const string renamed = axisExchange(it->first, mapping);
      if (renamed == it->first) continue;
      relabel(left,  it->first, "\x01" + renamed);
      relabel(right, it->first, "\x01" + renamed);
    }
    for (map<string,int>::const_iterator it = have.begin();
         it != have.end(); ++it) {
      const string renamed = axisExchange(it->first, mapping);
      if (renamed == it->first) continue;
      relabel(left,  "\x01" + renamed, renamed);
      relabel(right, "\x01" + renamed, renamed);
    }
    return true;
  }

  //  Every pair of fragment irreps, since only a pair of equal
  //  dimension can be a relabelling.  The dimension is not known here,
  //  but a swap that makes the two multisets agree is evidence enough:
  //  a wrong swap cannot make them agree, because it would have to
  //  produce the same counts by accident.
  vector<string> names;
  for (map<string,int>::const_iterator it = have.begin(); it != have.end(); ++it) {
    names.push_back(it->first);
  }

  for (size_t i = 0; i < names.size(); i++) {
    for (size_t j = i+1; j < names.size(); j++) {
      map<string,int> tried = have;
      const int a = tried[names[i]], b = tried[names[j]];
      tried[names[i]] = b;
      tried[names[j]] = a;
      if (!covers(tried, want)) continue;

      relabel(left,  names[i], "\x01");
      relabel(right, names[i], "\x01");
      relabel(left,  names[j], names[i]);
      relabel(right, names[j], names[i]);
      relabel(left,  "\x01", names[j]);
      relabel(right, "\x01", names[j]);
      return true;
    }
  }

  ostringstream text;
  text << "The fragment orbitals and the calculation's labels do not span "
          "the same representation (";
  bool first = true;
  for (map<string,int>::const_iterator it = want.begin(); it != want.end(); ++it) {
    if (!first) text << " + ";
    text << it->second << it->first;
    first = false;
  }
  text << " reported, ";
  first = true;
  for (map<string,int>::const_iterator it = have.begin(); it != have.end(); ++it) {
    if (!first) text << " + ";
    text << it->second << it->first;
    first = false;
  }
  //  SAY WHAT ACTUALLY HAPPENS NEXT, WHICH IS NOT NOTHING.
  //
  //  Failing to reconcile the labels does not stop the diagram: the
  //  correlation falls back to composition -- which fragment and
  //  which shell each orbital is built from, out of its own
  //  coefficients -- and that needs no labels at all.  CCl4 drew
  //  twenty correlation lines under a note saying it had drawn none,
  //  which is worse than saying nothing.
  text << " from symmetry), so the levels are correlated by what they "
          "are built from rather than by their labels.";
  why = text.str();
  return false;
}



void MoDiagram::classify(const vector<MoLevel>& left,
                         vector<MoLevel>& centre,
                         const vector<MoLevel>& right,
                         bool fromHalves)
{
  //  WITHOUT IRREPS ON THE FRAGMENT SIDE THERE IS NOTHING TO COUNT,
  //  and counting nothing is not the same as counting zero.
  //
  //  A diatomic's columns are one atom's own orbitals, which span no
  //  irrep of the molecular group -- half its operations move that
  //  atom onto its partner.  With p = q = 0 for every irrep the rule
  //  below marks every molecular orbital non-bonding, so nitrogen came
  //  out with "nb" against all eight of its levels including the two
  //  that hold its triple bond.  Say nothing instead.
  bool anyIrrep = false;
  for (size_t i = 0; i < left.size() && !anyIrrep; i++) {
    if (!left[i].irrep.empty()) anyIrrep = true;
  }
  for (size_t i = 0; i < right.size() && !anyIrrep; i++) {
    if (!right[i].irrep.empty()) anyIrrep = true;
  }
  if (!anyIrrep) return;

  int nextPair = 0;

  //  One irrep at a time; orbitals of different irreps cannot combine,
  //  so each is a separate counting problem.
  vector<string> done;
  for (size_t c = 0; c < centre.size(); c++) {
    const string irrep = centre[c].irrep;
    if (irrep.empty()) continue;

    bool already = false;
    for (size_t k = 0; k < done.size(); k++) if (done[k] == irrep) already = true;
    if (already) continue;
    done.push_back(irrep);

    //  The molecular levels of this irrep, in energy order -- which is
    //  the order they are in, since the spectrum arrives sorted.
    vector<size_t> mine;
    for (size_t i = 0; i < centre.size(); i++) {
      if (centre[i].irrep == irrep) mine.push_back(i);
    }

    const int p = countOf(left, irrep);
    const int q = countOf(right, irrep);

    //  IN A TWO-HALVES DIAGRAM THIS COUNT MEANS NOTHING.
    //
    //  The in-phase and out-of-phase combinations of one fragment
    //  orbital carry DIFFERENT irreps -- ethene's CH2 a1 gives ag on
    //  one side and b2u on the other -- so every irrep sits in one
    //  column only, min(p,q) is zero for all of them, and all twelve
    //  molecular levels came out marked "nb", including the two that
    //  hold the C=C. The same trap the empty-irrep guard above was
    //  written for, one level down.
    //
    //  In a CENTRAL-ATOM diagram the identical observation is a real
    //  finding: water's b1 is on the oxygen and nowhere on the
    //  hydrogens, and it genuinely is the non-bonding lone pair. So
    //  the two cases cannot share an answer, and the caller says
    //  which one this is.
    //
    //  Bonding character in a halves diagram follows the phase of the
    //  combination rather than a count across the columns, which is a
    //  rule this does not yet have; saying nothing is the honest form
    //  of not having it.
    if (fromHalves) continue;

    int pairs = (p < q) ? p : q;
    if (pairs*2 > (int)mine.size()) pairs = (int)mine.size()/2;

    for (size_t k = 0; k < mine.size(); k++) {
      MoLevel& level = centre[mine[k]];
      if ((int)k < pairs) {
        level.character = MoLevel::BONDING;
        level.pairing   = nextPair + (int)k;
      } else if ((int)k >= (int)mine.size() - pairs) {
        level.character = MoLevel::ANTIBONDING;
        //  Counting inwards from the top, so the highest antibonding
        //  pairs with the lowest bonding: they are the two ends of the
        //  same interaction.
        level.pairing   = nextPair + (int)(mine.size() - 1 - k);
      } else {
        level.character = MoLevel::NONBONDING;
        level.pairing   = -1;
      }
    }
    nextPair += pairs;
  }

  //  The asterisk on an antibonding level and "nb" on a non-bonding
  //  one, which is how they are written and how they are read.
  for (size_t c = 0; c < centre.size(); c++) {
    if (centre[c].character == MoLevel::ANTIBONDING) {
      centre[c].label += "*";
    } else if (centre[c].character == MoLevel::NONBONDING) {
      centre[c].label += " nb";
    }
  }
}



/** Are the two fragments the same set of orbitals?  A homonuclear
 *  diatomic's are; H2O's oxygen and its two hydrogens are not. */
static bool equivalentColumns(const vector<MoLevel>& a,
                              const vector<MoLevel>& b)
{
  if (a.size() != b.size() || a.empty()) return false;
  for (size_t i = 0; i < a.size(); i++) {
    if (a[i].shell != b[i].shell) return false;
    if (fabs(a[i].energy - b[i].energy) > 1.0e-6) return false;
  }
  return true;
}


/** An irrep without its g/u parity: SIG and SIU are both SI. */
static string withoutParity(const string& irrep)
{
  if (irrep.size() < 2) return irrep;
  const char last = irrep[irrep.size() - 1];
  if (last == 'G' || last == 'U' || last == 'g' || last == 'u') {
    return irrep.substr(0, irrep.size() - 1);
  }
  return irrep;
}


/** Which shell of the fragments this level is mostly built from. */
static int dominantShell(const MoLevel& level)
{
  int best = -1;
  double most = 0.0;
  for (size_t i = 0; i < level.shellLeft.size(); i++) {
    if (level.shellLeft[i] > most) { most = level.shellLeft[i]; best = (int)i; }
  }
  return best;
}


/** The bucket a level pairs within: its irrep, or -- where the two
 *  fragments are equivalent -- its irrep without parity plus the
 *  shell it came from. */
static string pairingKey(const MoLevel& level, bool mirrored)
{
  if (!mirrored) return level.irrep;
  ostringstream out;
  out << withoutParity(level.irrep) << '#' << dominantShell(level);
  return out.str();
}


void MoDiagram::classifyByEnergy(const vector<MoLevel>& left,
                                 vector<MoLevel>& centre,
                                 const vector<MoLevel>& right,
                                 const vector<MoConnection>& connections)
{
  //  WITHOUT IRREPS THERE IS NOTHING TO COUNT, but there is still
  //  something to read: an orbital below every fragment level it is
  //  built from is bonding, one above them all is antibonding, and one
  //  among them is neither.  That is what the words mean, and it needs
  //  no symmetry labels -- only the connections, which composition
  //  supplies.
  //
  //  Run after the fragment levels are placed, so "below its parents"
  //  is read against where they actually sit.
  int nextPair = 0;

  //  A SYMMETRIC DIATOMIC IS DECIDED BY ITS PARTNER, NOT BY ITS
  //  PARENTS.
  //
  //  Reading "is it below everything it is built from" fails here.  A
  //  diatomic's two combinations of one shell straddle that shell, and
  //  mixing moves them: C2's sigma-u sits above the 2s it came from
  //  but below the 2p it borrows from, so it was called non-bonding --
  //  an antibonding orbital, labelled nb, and then excluded from
  //  pairing so its partner lost its colour too.
  //
  //  What is certain is the pair: of the two combinations a shell
  //  gives, the lower IS the bonding one and the higher IS the
  //  antibonding one.  That needs no reference to where the atomic
  //  level sits, and it is what a person draws.
  const bool symmetric = equivalentColumns(left, right);
  if (symmetric) {
    vector<string> keys;
    for (size_t c = 0; c < centre.size(); c++) {
      if (centre[c].character != MoLevel::UNKNOWN) continue;
      const string key = pairingKey(centre[c], true);
      bool seen = false;
      for (size_t k = 0; k < keys.size(); k++) if (keys[k] == key) seen = true;
      if (!seen) keys.push_back(key);
    }

    for (size_t j = 0; j < keys.size(); j++) {
      vector<size_t> mine;
      for (size_t c = 0; c < centre.size(); c++) {
        if (centre[c].character != MoLevel::UNKNOWN) continue;
        if (pairingKey(centre[c], true) == keys[j]) mine.push_back(c);
      }
      //  The spectrum arrives sorted, so mine is in energy order.
      const size_t pairs = mine.size()/2;
      for (size_t k = 0; k < pairs; k++) {
        MoLevel& low  = centre[mine[k]];
        MoLevel& high = centre[mine[mine.size() - 1 - k]];
        low.character  = MoLevel::BONDING;
        high.character = MoLevel::ANTIBONDING;
        high.label += "*";
        low.pairing = high.pairing = nextPair++;
      }
    }
  }

  for (size_t c = 0; c < centre.size(); c++) {
    if (centre[c].character != MoLevel::UNKNOWN) continue;

    double lowest = 1.0e30, highest = -1.0e30;
    bool any = false;
    for (size_t k = 0; k < connections.size(); k++) {
      if (connections[k].centreLevel != (int)c) continue;
      const int l = connections[k].leftLevel;
      const int r = connections[k].rightLevel;
      if (l >= 0 && l < (int)left.size()) {
        if (left[l].energy < lowest)  lowest = left[l].energy;
        if (left[l].energy > highest) highest = left[l].energy;
        any = true;
      }
      if (r >= 0 && r < (int)right.size()) {
        if (right[r].energy < lowest)  lowest = right[r].energy;
        if (right[r].energy > highest) highest = right[r].energy;
        any = true;
      }
    }
    if (!any) continue;

    if (centre[c].energy < lowest) {
      centre[c].character = MoLevel::BONDING;
    } else if (centre[c].energy > highest) {
      centre[c].character = MoLevel::ANTIBONDING;
      centre[c].label += "*";
    } else {
      centre[c].character = MoLevel::NONBONDING;
      centre[c].label += " nb";
    }
  }

  //  Pair each bonding level with an antibonding one, lowest with
  //  highest, so the two ends of an interaction share a colour.
  //
  //  WITHIN ONE IRREP, AND ONLY WHERE classify() LEFT IT OPEN.  This
  //  used to sweep every bonding and antibonding level in the column
  //  and renumber from zero, so it overwrote the pairing classify()
  //  had already worked out by symmetry -- water came out with 1b2
  //  sharing a colour with 3a1*, which is not a bonding/antibonding
  //  pair and cannot be one: they are different irreps and do not
  //  interact.  An orbital's partner is of its own symmetry.
  int highestPair = nextPair - 1;
  for (size_t c = 0; c < centre.size(); c++) {
    if (centre[c].pairing > highestPair) highestPair = centre[c].pairing;
  }
  nextPair = highestPair + 1;

  //  WHERE THE TWO FRAGMENTS ARE THE SAME, PARITY IS NOT A DIVIDER.
  //
  //  A homonuclear diatomic's bonding and antibonding partners have
  //  OPPOSITE parity by construction -- the 2s pair gives sigma-g and
  //  sigma-u -- so bucketing by irrep puts the two ends of one
  //  interaction in different buckets and they never pair.  What they
  //  do share is the shell they came from, so that is the bucket:
  //  parity dropped, parent shell added.  N2 then gives (1sig, 1siu),
  //  (2sig, 2siu) and (1piu, 1pig), which is what is drawn on a board.
  //
  //  Only where the fragments really are equivalent.  Water's a1 set
  //  comes from two different oxygen shells and is right to pair
  //  within its irrep.
  const bool mirrored = equivalentColumns(left, right);

  vector<string> irreps;
  for (size_t c = 0; c < centre.size(); c++) {
    if (centre[c].pairing >= 0) continue;
    const string key = pairingKey(centre[c], mirrored);
    bool seen = false;
    for (size_t k = 0; k < irreps.size(); k++) {
      if (irreps[k] == key) seen = true;
    }
    if (!seen) irreps.push_back(key);
  }

  for (size_t j = 0; j < irreps.size(); j++) {
    vector<size_t> bonding, antibonding;
    for (size_t c = 0; c < centre.size(); c++) {
      if (centre[c].pairing >= 0) continue;
      if (pairingKey(centre[c], mirrored) != irreps[j]) continue;
      if (centre[c].character == MoLevel::BONDING) bonding.push_back(c);
      if (centre[c].character == MoLevel::ANTIBONDING) antibonding.push_back(c);
    }
    for (size_t i = 0; i < bonding.size() && i < antibonding.size(); i++) {
      centre[bonding[i]].pairing = nextPair;
      centre[antibonding[antibonding.size() - 1 - i]].pairing = nextPair;
      nextPair++;
    }
  }
}


void MoDiagram::connect(const vector<MoLevel>& left,
                        const vector<MoLevel>& centre,
                        const vector<MoLevel>& right,
                        vector<MoConnection>& connections,
                        double cutoff)
{
  connections.clear();

  for (size_t c = 0; c < centre.size(); c++) {
    //  An unlabelled level is not an unconnectable one.  A calculation
    //  that reports no symmetry labels still has coefficients, and
    //  those say which fragment and which shell each orbital draws
    //  from -- which is the whole question a correlation line answers.
    //  Skipping them left a diagram with no lines at all.
    const bool haveShare = (centre[c].shareLeft >= 0.0 ||
                            !centre[c].shellLeft.empty());
    if (centre[c].irrep.empty() && !haveShare) continue;

    //  EVERY MATCHING FRAGMENT LEVEL, NOT THE FIRST.
    //
    //  This is symmetry mixing, and it is the whole reason a
    //  correlation diagram is more than a list.  In C2v water the
    //  oxygen 2s and 2pz are BOTH a1, so both mix into every a1
    //  molecular orbital; in C3v ammonia the nitrogen 2s and 2pz are
    //  both a1 the same way.  Stopping at the first match drew one
    //  line where there should be two and made the diagram look like
    //  a set of isolated two-orbital interactions, which is exactly
    //  the misconception the diagram exists to correct.
    bool any = false;

    //  A NON-BONDING LEVEL GETS A LINE TO ONE SIDE ONLY.
    //
    //  It exists because one fragment had an orbital of this irrep
    //  that the other could not match, so drawing it to both sides
    //  says the opposite of what it is.  It also halves the number of
    //  lines on a crowded diagram: NO2- drew every a1 to every a1 and
    //  came out as a web.
    const bool nonBonding = (centre[c].character == MoLevel::NONBONDING);
    const bool excessLeft =
        countOf(left, centre[c].irrep) > countOf(right, centre[c].irrep);

    //  A share of -1 means none was computed, which is not zero: fall
    //  back to symmetry rather than deciding the level is connected to
    //  nothing.
    const bool knowShare = (centre[c].shareLeft >= 0.0 ||
                            centre[c].shareRight >= 0.0);
    bool onLeft  = !knowShare || centre[c].shareLeft  >= cutoff;
    bool onRight = !knowShare || centre[c].shareRight >= cutoff;

    //  A NON-BONDING LEVEL GETS ONE LINE, TO THE FRAGMENT IT IS ON.
    //
    //  The cutoff alone does not thin a small molecule's diagram,
    //  because a small molecule's orbitals really are spread over both
    //  fragments: every level of nitrite clears five per cent on both
    //  sides, and rightly.  What is not true is that a non-bonding
    //  level is interacting with both -- it exists because one
    //  fragment had an orbital the other could not match -- so it is
    //  drawn to whichever side carries it, which is the honest line
    //  and half the lines.
    //
    //  Which side that is comes from the composition where there is
    //  one, and from the counting where there is not.  The two agree
    //  where both are available: nitrite's a2 is the one the counting
    //  calls non-bonding for want of a partner, and the coefficients
    //  put it at 100% on the oxygens.
    if (nonBonding) {
      if (knowShare) {
        const bool leftWins = centre[c].shareLeft > centre[c].shareRight;
        onLeft  = onLeft  && leftWins;
        onRight = onRight && !leftWins;
      } else {
        onLeft  = onLeft  && excessLeft;
        onRight = onRight && !excessLeft;
      }
    }

    //  AT MOST TWO LINES A SIDE, THE NEAREST IN ENERGY.
    //
    //  Drawing every matching fragment level is right and is the
    //  point -- symmetry mixing is why a correlation diagram is more
    //  than a list.  But where composition cannot discriminate it
    //  matches everything: two equivalent halves each carry half of
    //  every orbital by symmetry, so ethene drew a hundred and two
    //  lines and nothing could be read.  Two is enough for mixing to
    //  show and few enough to follow.
    //  COMPARED ON EACH COLUMN'S OWN SCALE.
    //
    //  connect() runs BEFORE placeFragments(), so a fragment level
    //  still carries its tabulated energy in eV while the molecular
    //  orbitals are in Hartree.  Ranking by the raw difference then
    //  ranks by magnitude alone: ethene's carbon 2s at -19.4 was
    //  never "nearest" to anything and drew no lines at all, while
    //  its 2p at -10.7 drew eight.  Both columns are mapped onto
    //  nought-to-one first, which is scale-free and is what "nearest"
    //  was meant to mean.
    double lowLeft = 1.0e30, highLeft = -1.0e30;
    for (size_t i = 0; i < left.size(); i++) {
      if (left[i].energy < lowLeft)  lowLeft = left[i].energy;
      if (left[i].energy > highLeft) highLeft = left[i].energy;
    }
    double lowRight = 1.0e30, highRight = -1.0e30;
    for (size_t i = 0; i < right.size(); i++) {
      if (right[i].energy < lowRight)  lowRight = right[i].energy;
      if (right[i].energy > highRight) highRight = right[i].energy;
    }
    double lowMo = 1.0e30, highMo = -1.0e30;
    for (size_t i = 0; i < centre.size(); i++) {
      if (centre[i].energy < lowMo)  lowMo = centre[i].energy;
      if (centre[i].energy > highMo) highMo = centre[i].energy;
    }

    const double mo = (highMo > lowMo)
        ? (centre[c].energy - lowMo)/(highMo - lowMo) : 0.0;

    vector< std::pair<double,size_t> > leftNear, rightNear;

    for (size_t l = 0; l < left.size(); l++) {
      //  Matched on the irrep where there is one, and on the shell
      //  where there is not -- a diatomic's columns carry no irrep,
      //  because a single atom of one spans none.
      const bool bySymmetry = !left[l].irrep.empty() &&
                              !centre[c].irrep.empty();
      if (bySymmetry && left[l].irrep != centre[c].irrep) continue;
      if (!bySymmetry && !matchesShell(centre[c], left[l], true, cutoff)) {
        continue;
      }
      if (bySymmetry && !onLeft) continue;
      const double where = (highLeft > lowLeft)
          ? (left[l].energy - lowLeft)/(highLeft - lowLeft) : 0.0;
      leftNear.push_back(std::make_pair(fabs(where - mo), l));
    }

    std::sort(leftNear.begin(), leftNear.end());
    for (size_t k = 0; k < leftNear.size() && k < 2; k++) {
      MoConnection link;
      link.leftLevel = (int)leftNear[k].second;
      link.centreLevel = (int)c;
      link.rightLevel = -1;
      connections.push_back(link);
      any = true;
    }

    for (size_t r = 0; r < right.size(); r++) {
      const bool bySymmetry = !right[r].irrep.empty() &&
                              !centre[c].irrep.empty();
      if (bySymmetry && right[r].irrep != centre[c].irrep) continue;
      if (!bySymmetry && !matchesShell(centre[c], right[r], false, cutoff)) {
        continue;
      }
      if (bySymmetry && !onRight) continue;
      const double where = (highRight > lowRight)
          ? (right[r].energy - lowRight)/(highRight - lowRight) : 0.0;
      rightNear.push_back(std::make_pair(fabs(where - mo), r));
    }

    std::sort(rightNear.begin(), rightNear.end());
    for (size_t k = 0; k < rightNear.size() && k < 2; k++) {
      MoConnection link;
      link.leftLevel = -1;
      link.centreLevel = (int)c;
      link.rightLevel = (int)rightNear[k].second;
      connections.push_back(link);
      any = true;
    }

    //  A level with no partner on either side is non-bonding, and that
    //  is a result worth recording rather than an absence.
    if (!any) {
      MoConnection link;
      link.leftLevel = -1;
      link.centreLevel = (int)c;
      link.rightLevel = -1;
      connections.push_back(link);
    }
  }
}
