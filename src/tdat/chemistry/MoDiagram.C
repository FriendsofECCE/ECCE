#include <cctype>
#include <cstdio>
#include <map>
#include <sstream>
using std::ostringstream;
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
  return true;
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
    int wanted = (found == dimensions.end()) ? 1 : found->second;
    if (wanted < 1) wanted = 1;

    //  Take up to `wanted` consecutive orbitals with the same label.
    //  Fewer is possible and is not an error: an open shell, or a
    //  calculation whose last printed orbital falls mid-set.
    size_t take = 0;
    while (take < (size_t)wanted && i + take < n &&
           canonicalIrrep(labels[i + take]) == canonical) {
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

  //  NUMBER THE LEVELS WITHIN EACH IRREP: 1a1, 2a1, 1t2, 2t2.
  //
  //  This is how a diagram is labelled and how orbitals are referred to
  //  in text, and it is the difference between something that reads as
  //  a diagram and something that reads as a column of output.  MOPAC
  //  prints its own orbitals this way ("1 a1  1 t2 ... 2 a1"); ECCE's
  //  parser keeps only the symbol, so the count is redone here from the
  //  order the levels come in, which is the same rule: counting up from
  //  the lowest.
  map<string,int> seen;
  for (size_t k = 0; k < levels.size(); k++) {
    const int n = ++seen[levels[k].irrep];
    char text[64];
    snprintf(text, sizeof(text), "%d%s", n, levels[k].label.empty()
             ? levels[k].irrep.c_str() : levels[k].label.c_str());
    levels[k].label = text;
  }
  return !levels.empty();
}


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

  for (c = 0; c < 2; c++) {
    for (size_t i = 0; i < cols[c]->levels.size(); i++) {
      MoLevel& level = cols[c]->levels[i];

      char text[64];
      snprintf(text, sizeof(text), "%.1f eV", level.energy);
      level.annotation = text;

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
        double w = 1.0;
        if (level.shell >= 0) {
          const vector<double>& shares = (c == 0) ? mo.shellLeft
                                                  : mo.shellRight;
          if ((int)shares.size() > level.shell) w = shares[level.shell];
        } else {
          const double share = (c == 0) ? mo.shareLeft : mo.shareRight;
          if (share >= 0.0) w = share;
        }
        if (w <= 0.0) continue;

        //  Each orbital of a degenerate level counts once.
        const double n = (mo.degeneracy > 0) ? mo.degeneracy : 1;
        sum += w*n*mo.energy;
        weight += w*n;
      }

      if (weight > 0.0) level.energy = sum/weight;
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
    for (size_t i = 0; i < levels.size(); i++) {
      if (levels[i].energy > -100.0) { placedSum += levels[i].energy; placedCount++; }
    }
    if (placedCount == 0) continue;

    const double fallback = placedSum/placedCount;
    for (size_t i = 0; i < levels.size(); i++) {
      if (levels[i].energy <= -100.0) levels[i].energy = fallback;
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

  //  One antibonding partner per occupied level, plus two, so a small
  //  molecule keeps a little room above the LUMO.
  const size_t keep = occupied + 2;
  const size_t last = firstVirtual + keep;
  if (last >= levels.size()) return 1.0e30;    // nothing worth folding

  //  Cut between the last kept level and the first dropped one.
  return 0.5*(levels[last].energy + levels[last-1].energy);
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

    const double gap = levels[i].energy - levels[i-1].energy;
    if (gap > bestGap) { bestGap = gap; bestAt = i; }
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
  return shares[fragment.shell] >= cutoff;
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


bool MoDiagram::reconcile(vector<MoLevel>& left, vector<MoLevel>& right,
                          const vector<MoLevel>& centre, string& why)
{
  why.clear();

  const vector<MoLevel> none;
  map<string,int> want = tally(centre, none);
  map<string,int> have = tally(left, right);
  if (want.empty() || have.empty()) return true;
  if (want == have) return true;

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
      if (tried != want) continue;

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
  text << " from symmetry), so nothing is correlated.";
  why = text.str();
  return false;
}



void MoDiagram::classify(const vector<MoLevel>& left,
                         vector<MoLevel>& centre,
                         const vector<MoLevel>& right)
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



void MoDiagram::connect(const vector<MoLevel>& left,
                        const vector<MoLevel>& centre,
                        const vector<MoLevel>& right,
                        vector<MoConnection>& connections,
                        double cutoff)
{
  connections.clear();

  for (size_t c = 0; c < centre.size(); c++) {
    if (centre[c].irrep.empty()) continue;

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
      MoConnection link;
      link.leftLevel = (int)l;
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
      MoConnection link;
      link.leftLevel = -1;
      link.centreLevel = (int)c;
      link.rightLevel = (int)r;
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
