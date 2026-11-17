#ifndef CLUSTERCUT_H
#define CLUSTERCUT_H

#include <array>
#include <string>
#include <vector>

// Cut a non-periodic cluster out of a periodic structure (#1033).  No wx and
// no Fragment: symbols and Cartesian coordinates in, the same out, in Angstrom.
struct ClusterAtom {
  std::string symbol;
  double xyz[3];
  bool cap;            // a capping atom added on a cut bond
};

struct ClusterOptions {
  enum Whole { WholeNone, WholeMolecules, WholeUnits };
  enum Cap { CapNone, CapH, CapOH };
  double radius = 5.0;
  Whole whole = WholeNone;
  Cap cap = CapNone;
};

// Atoms whose periodic image lies within radius of centre (Cartesian), with
// the images that fall across the cell boundary.  Throws std::invalid_argument
// for a degenerate cell or an unknown element.
std::vector<ClusterAtom> cutCluster(const std::vector<std::string>& symbols,
    const std::vector<std::array<double,3> >& xyz,
    const std::array<std::array<double,3>,3>& cell,
    const std::array<double,3>& centre, const ClusterOptions& opt);

#endif
