/**
 * @file
 * One point of a relaxed scan or an IRC as a structure (#674).  A point is
 * named by a spec: "" the calculation's final structure, "last", "max" (the
 * highest-energy point) or a 1-based number.  No wx here.
 */
#ifndef SCANPOINT_HH
#define SCANPOINT_HH

#include <string>
#include <vector>

class Fragment;
class PropertyTask;

namespace ScanPoint {

  /// "", "last", "max", a positive number, or "cK": the K-th conformer of a
  /// CREST ensemble by relative energy (1 = lowest).
  bool valid(const std::string& spec);

  /// "last point", "maximum", "point 5"; "" for the final structure.
  std::string words(const std::string& spec);

  /// Number of conformers or docking poses (GEOMTRACE frames with a CONFREL or
  /// POSESCORE each, ranked by that value); 0 if none.
  int ensembleSize(PropertyTask *calc);

  /// True if the ensemble is a docking pose ensemble (POSESCORE), not CREST's.
  bool ensembleIsPoses(PropertyTask *calc);

  /// Relative energy or score (kcal/mol) of the member of rank `rank`, 0 if none.
  double conformerEnergy(PropertyTask *calc, int rank);

  /// One coordination pattern of a CREST ensemble: its label (the key of
  /// ecce-contacts, e.g. "Na: 6×O(H2O)"), the number of conformers that have
  /// it and its lowest-energy member as a rank (1 = lowest) with that
  /// member's relative energy (kcal/mol).
  struct Pattern {
    std::string label;
    int count;
    int rank;
    double relative;
  };

  /// Groups the ensemble by first-shell donor atoms with ecce-contacts (one
  /// implementation of the rule for the CLI and the GUI), lowest energy
  /// first.  False with `why` set if there is no ensemble, no ion in it, or
  /// ecce-contacts cannot be run.
  bool ensemblePatterns(PropertyTask *calc, std::vector<Pattern>& out,
                        std::string& why);

  /// True if the calculation has an energy and a structure per point.
  bool isScan(PropertyTask *calc);

  /// Index of the point (0-based), or -1 with why set.
  int frame(PropertyTask *calc, const std::string& spec, std::string& why);

  /// The point's atoms and coordinates (Angstrom), in the calculation's own
  /// atom order.  False with why set.
  bool structure(PropertyTask *calc, const std::string& spec,
                 std::vector<std::string>& elements, std::vector<double>& xyz,
                 std::string& why);

  /// A copy of the calculation's fragment placed at the point.  Caller
  /// deletes; 0 with why set.
  Fragment *fragment(PropertyTask *calc, const std::string& spec,
                     std::string& why);

  /// Puts x y z per atom onto frag; dummy atoms ("X") are skipped when the
  /// structure has no rows for them.  False, frag unchanged, with why set
  /// if the atom count fits neither way.
  bool apply(Fragment *frag, const std::vector<double>& xyz, std::string& why);
}

#endif
