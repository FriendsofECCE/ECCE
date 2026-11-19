/**
 * @file
 * Fill a periodic cell with copies of a pre-equilibrated solvent box
 * (data/client/solvents/*.xyz) around a solute (#1118).  Pure geometry on
 * coordinate lists: no Fragment, no wx, so it can be tested and driven from
 * a CLI.  Lengths are Angstrom; the cell's origin is (0,0,0).
 *
 * A copy of a solvent molecule is kept when its centroid lies in the cell
 * and none of its atoms comes closer than scale*(r1+r2) (van der Waals
 * radii) to a solute atom, or solventScale*(r1+r2) to an atom of a solvent
 * molecule already kept, both by minimum image.  The second test removes
 * the clashes where a box that is not a whole multiple of the template
 * meets its own periodic image.
 */
#ifndef SOLVATE_HH
#define SOLVATE_HH

#include <array>
#include <string>
#include <vector>

namespace solvate {

typedef std::array<double,3> Vec;
typedef double Cell[3][3];   // rows are a, b, c

struct SolventBox {
  std::string name;
  std::vector<std::string> symbols;   // element symbols
  std::vector<std::string> types;     // atom types as in the file (OW, HW)
  std::vector<Vec> xyz;
  int molSize;                         // atoms per molecule
  double edge[3];                      // orthorhombic template box
  SolventBox() : molSize(0) { edge[0] = edge[1] = edge[2] = 0.0; }
  int molecules() const { return molSize > 0 ? (int)xyz.size() / molSize : 0; }
};

/// Read "<natoms> <atoms per molecule>", "<a> <b> <c>", then TYPE x y z.
bool readSolventBox(const std::string& path, SolventBox& box,
                    std::string* err = 0);
/// Names (file stems) of the solvent boxes in a directory, sorted.
std::vector<std::string> solventNames(const std::string& dir);

struct Options {
  double scale;          // solute-solvent, times the sum of vdW radii
  double solventScale;   // solvent-solvent across the cell boundary
  double margin;         // Angstrom around the solute when there is no cell
  int maxAtoms;          // refuse a fill larger than this
  Options() : scale(1.0), solventScale(0.57), margin(10.0), maxAtoms(2000000) {}
};

struct Result {
  std::vector<std::string> symbols, types;
  std::vector<Vec> xyz;
  int molecules;
  Result() : molecules(0) {}
};

/// Orthorhombic cell holding the solute's bounding box plus margin on every
/// side; shift is what to add to the solute so it sits in the cell's middle.
void marginCell(const std::vector<Vec>& solute, double margin, Cell out,
                Vec& shift);

/// Solvent molecules to add.  False and *err on a degenerate cell, an empty
/// template or a fill above opt.maxAtoms.
bool fill(const std::vector<std::string>& soluteSymbols,
          const std::vector<Vec>& soluteXyz, const Cell cell,
          const SolventBox& box, const Options& opt, Result& out,
          std::string* err = 0);

/// Mass density (g/cm3) of the given atoms in the cell.
double density(const std::vector<std::string>& symbols, const Cell cell);

/// Water template (O, H, H per molecule)?
bool isWater(const SolventBox& box);

}  // namespace solvate

#endif
