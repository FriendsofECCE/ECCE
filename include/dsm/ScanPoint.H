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

  /// "", "last", "max" or a positive number.
  bool valid(const std::string& spec);

  /// "last point", "maximum", "point 5"; "" for the final structure.
  std::string words(const std::string& spec);

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
