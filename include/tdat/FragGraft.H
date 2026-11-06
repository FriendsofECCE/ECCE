// Building from SMILES and grafting a fragment onto a structure (#833).
// No wx and no fragment classes: plain coordinates and bond lists, so the
// geometry can be tested on its own.
#ifndef FRAGGRAFT_H
#define FRAGGRAFT_H

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace FragGraft {

typedef std::array<double,3> Vec;

struct Bond { int a, b; double order; };

/// A molecule as Open Babel writes it: symbols, coordinates (Angstrom),
/// formal charges, bonds, and the atom that stood for [*] (-1 if none).
struct Mol {
   std::vector<std::string> el;
   std::vector<Vec> xyz;
   std::vector<int> charge;
   std::vector<Bond> bonds;
   int dummy = -1;
};

/// Parse an MDL V2000 block as Open Babel writes it.  Atoms with the
/// isotope 2 (M  ISO) become Mol::dummy.
bool parseSdf(const std::string& text, Mol& mol, std::string& err);

/// 3-D structure of a SMILES string through `obabel --gen3d`; stereo and
/// formal charges come with it.  A "[*]" in the SMILES is generated as a
/// deuterium (a real bond direction and length) and returned as
/// Mol::dummy.  err names a missing Open Babel.
bool smilesTo3D(const std::string& smiles, Mol& mol, std::string& err);

/// Translation that puts `add` beside `existing` along +x, a gap apart,
/// so the two do not overlap.  Empty `existing` gives zero.
Vec placeBeside(const std::vector<Vec>& existing, const std::vector<Vec>& add,
                double gap = 3.0);

/// Set every non-ring single bond between two heavy atoms, each with
/// another heavy neighbour, to a torsion of 180 degrees.  Works outward
/// from atom 0 and moves the far side of each bond, so a torsion set
/// stays set.
void extendTorsions(Mol& mol);

/// Torsion a-b-c-d in degrees, (-180, 180].
double torsionDeg(const Vec& a, const Vec& b, const Vec& c, const Vec& d);

/// Atoms on G's side of the bond A-G (G included); false if A is reached
/// from G another way (the bond is in a ring).  adj lists neighbours.
bool groupBehind(const std::vector<std::vector<int> >& adj, int A, int G,
                 std::vector<int>& group);

/// For a group named by its first atom G: the neighbour A to attach it to,
/// the one that leaves the smallest terminal group.  False if G has no
/// neighbour that cuts a group off, or two do equally.
bool chooseAttachment(const std::vector<std::vector<int> >& adj, int G,
                      int& A);

/// Place the fragment `frag` so that its atom `attach` bonds to the anchor
/// A at distance L along u (unit vector away from A), with the direction
/// from `dummy` to `attach` along u, rotated about u by the angle that
/// maximises the smallest distance between the fragment (its dummy left
/// out) and the points `rest`, scanned every stepDeg.  Returns all atoms'
/// new coordinates (the dummy's too) and the best angle and distance.
std::vector<Vec> graft(const std::vector<Vec>& frag, int attach, int dummy,
                       const Vec& anchor, const Vec& u, double L,
                       const std::vector<Vec>& rest, double stepDeg,
                       double *bestAngle = 0, double *bestMinDist = 0);

}

#endif
