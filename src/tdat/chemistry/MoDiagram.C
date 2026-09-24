#include <cmath>
#include <cstddef>

#include "tdat/MoDiagram.H"

MoDiagram::MoDiagram()
{
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
    } else {
      MoLevel level;
      level.energy = energies[i];
      level.label = label;
      level.occupancy = occupancies.empty() ? 0.0 : occupancies[i];
      level.degeneracy = 1;
      level.orbitals.push_back((int)i);
      levels.push_back(level);
    }
  }
  return true;
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


double MoDiagram::suggestCoreCutoff(const vector<MoLevel>& levels)
{
  if (levels.size() < 3) return levels.empty() ? -1.0e30
                                               : levels.front().energy;

  //  Every gap, and the typical one.
  vector<double> gaps;
  for (size_t i = 1; i < levels.size(); i++) {
    gaps.push_back(levels[i].energy - levels[i-1].energy);
  }
  vector<double> sorted(gaps);
  for (size_t i = 0; i < sorted.size(); i++) {
    for (size_t j = i+1; j < sorted.size(); j++) {
      if (sorted[j] < sorted[i]) {
        const double t = sorted[i]; sorted[i] = sorted[j]; sorted[j] = t;
      }
    }
  }
  const double median = sorted[sorted.size()/2];

  //  The biggest gap in the lower half, where a core/valence split
  //  would be.
  const size_t limit = levels.size()/2;
  double bestGap = 0.0;
  size_t bestAt = 0;
  for (size_t i = 1; i <= limit && i < levels.size(); i++) {
    if (gaps[i-1] > bestGap) { bestGap = gaps[i-1]; bestAt = i; }
  }

  //  Judged against the TYPICAL gap, not against the total span.  A
  //  span-based rule is scale-dependent and gets the easy case wrong:
  //  an evenly spaced spectrum of five levels has every gap at exactly
  //  a quarter of the span, so any threshold near that either hides an
  //  orbital for no reason or misses a real core.  A core sits four or
  //  more typical gaps away from the valence; even spacing sits at one.
  //
  //  This also gives the right answer for a semiempirical calculation,
  //  which has no core at all: its gaps are comparable, so nothing is
  //  hidden rather than something arbitrary being hidden.
  if (median <= 0.0 || bestGap < 4.0*median) return levels.front().energy;

  return 0.5*(levels[bestAt].energy + levels[bestAt-1].energy);
}


void MoDiagram::connect(const vector<MoLevel>& left,
                        const vector<MoLevel>& centre,
                        const vector<MoLevel>& right,
                        vector<MoConnection>& connections)
{
  connections.clear();

  for (size_t c = 0; c < centre.size(); c++) {

    MoConnection link;
    link.leftLevel = -1;
    link.centreLevel = (int)c;
    link.rightLevel = -1;

    //  Symmetry is the entire rule: only orbitals of the same irrep may
    //  combine.  A centre level with no partner on a side is
    //  non-bonding from that side, which is a result worth drawing --
    //  water's b2 lone pair is exactly this.
    for (size_t l = 0; l < left.size(); l++) {
      if (left[l].label == centre[c].label) { link.leftLevel = (int)l; break; }
    }
    for (size_t r = 0; r < right.size(); r++) {
      if (right[r].label == centre[c].label) { link.rightLevel = (int)r; break; }
    }
    connections.push_back(link);
  }
}
