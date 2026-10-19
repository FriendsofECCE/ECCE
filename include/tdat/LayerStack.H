/**
 * @file
 * Stack copies of one layer (a periodic slab) along the direction normal to
 * its a-b plane and fill the interlayer regions with ions and water.  Pure
 * geometry on coordinate lists: no Fragment, no wx, so it can be tested and
 * driven from a CLI.  Lengths are Angstrom.
 *
 * Definitions:
 *  - n is the unit normal of the a-b plane, on the same side as c.
 *  - spacing d is the perpendicular distance from a layer to the next one:
 *    layer i is the input layer translated by i*(shiftA*a + shiftB*b + d*n).
 *  - the shift accumulates (layer i is displaced i*shift laterally), so an
 *    AB stack of N=2 needs one shift and ABC needs the same shift with N=3.
 *  - the new cell keeps a and b and has c' = N*d*n + c_lateral, c_lateral
 *    being the part of the input c that lies in the a-b plane, so the tilt
 *    of the input cell is kept and its c length is replaced by N*d / cos.
 */
#ifndef LAYERSTACK_HH
#define LAYERSTACK_HH

#include <string>
#include <vector>

namespace layerstack {

enum Kind { LAYER = 0, ION = 1, WATER_O = 2, WATER_H = 3 };

struct Atom {
  std::string symbol;
  double xyz[3];
  int kind;
  double charge;   // formal charge for ions, else 0
  Atom() : kind(LAYER), charge(0.0) { xyz[0] = xyz[1] = xyz[2] = 0.0; }
};

struct Ion {
  std::string symbol;
  int count;        // per interlayer
  double charge;
  double minDist;   // to every atom already there
  Ion() : count(0), charge(0.0), minDist(0.0) {}
};

struct Options {
  int nLayers;
  double spacing;
  double shiftA, shiftB;       // fractions of a and b, per layer
  std::vector<Ion> ions;
  int waterPerInterlayer;
  double waterMinDist;         // water O to every atom already there
  unsigned seed;
  Options() : nLayers(1), spacing(0.0), shiftA(0.0), shiftB(0.0),
              waterPerInterlayer(0), waterMinDist(0.0), seed(1) {}
};

typedef double Cell[3][3];   // rows are a, b, c

/// New cell for the stack; false and *err if the input cell is degenerate.
bool stackCell(const Cell cell, const Options& opt, Cell out,
               std::string* err = 0);

/// The N layers, unwrapped (layer 0 first, input order inside a layer).
/// A layer that straddles the cell's c boundary is first made contiguous.
bool stackLayers(const std::vector<Atom>& layer, const Cell cell,
                 const Options& opt, std::vector<Atom>& out,
                 std::string* err = 0);

/// Ions then water for each of the N interlayers (the last one is the gap
/// between the top layer and the periodic image of the first).  `stack`
/// is stackLayers' result.  Water is O followed by its two H.
bool fillInterlayers(const std::vector<Atom>& stack, const Cell newCell,
                     const Options& opt,
                     std::vector<Atom>& added, std::string* err = 0);

/// Move every atom into [0,1)^3 of the cell; a water keeps its shape
/// (its H follow the O).
void wrapIntoCell(std::vector<Atom>& atoms, const Cell cell);

/// stackLayers + fillInterlayers + wrapIntoCell.
bool buildLayerStack(const std::vector<Atom>& layer, const Cell cell,
                     const Options& opt, std::vector<Atom>& out, Cell newCell,
                     std::string* err = 0);

}  // namespace layerstack

#endif
