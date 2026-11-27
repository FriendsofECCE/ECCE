/**
 * @file PocketFinder.H
 *
 * LIGSITE-style binding-pocket detection (#1320).  A grid is laid over the
 * structure, each point is marked protein or free from van der Waals radii,
 * and each free point is scored by how many of 7 directions (3 axes, 4 cube
 * diagonals) are closed by protein on BOTH sides within maxRay.  Contiguous
 * points scoring at least minScore form a pocket.  Everything here is wx-free.
 *
 * Two limits of the PDB reader shape the design:
 *  - HETATM/ATOM is not kept on import (Fragment::restorePDB treats them
 *    alike), so "a ligand is already in the file" is decided by residue-name
 *    class (LigandSelect), which misclassifies modified residues such as MSE.
 *  - SITE records are dropped at import; honouring them needs reader work.
 */
#ifndef POCKETFINDER_HH
#define POCKETFINDER_HH

#include <array>
#include <string>
#include <vector>

class Fragment;

namespace PocketFinder {

struct Options {
  double spacing = 1.0;       ///< grid step, Angstrom
  double margin = 4.0;        ///< grid padding around the atoms, Angstrom
  double probe = 1.4;         ///< a pocket point must fit a sphere this big, Angstrom
  double maxRay = 12.0;       ///< how far a ray looks for protein, Angstrom
  int minScore = 5;           ///< of 7 directions; LIGSITE's usual cut
  double minVolume = 20.0;    ///< smaller clusters are dropped, cubic Angstrom
  double liningDistance = 4.0;///< protein atom within this of a pocket point
  bool ignoreWater = true;    ///< waters do not close a pocket
  bool ignoreLigands = true;  ///< bound ligands are removed, so their site shows
  int threads = 0;            ///< 0 = hardware concurrency
};

struct LiningResidue {
  std::string name;
  int number = 0;
  char chain = ' ';
};

struct Pocket {
  std::array<double,3> centre{};   ///< centroid of the points; may lie in protein for a curved pocket
  std::array<double,3> anchor{};   ///< the pocket point nearest the centroid; always free
  std::array<double,3> lo{}, hi{}; ///< bounding box of the points
  int points = 0;
  double volume = 0;               ///< cubic Angstrom
  double meanEnclosure = 0;        ///< mean point score, 0..7
  double score = 0;                ///< enclosure-weighted volume; the ranking key
  std::vector<std::array<double,3>> pointCoords;
  std::vector<LiningResidue> lining; ///< sorted by chain, number
};

struct Result {
  std::vector<Pocket> pockets;     ///< best first
  int atomsUsed = 0;               ///< atoms that closed pockets
  int atomsIgnoredWater = 0;
  int atomsIgnoredLigand = 0;
  // TPerTab::vwr returns 0.0 silently for an element without an oxidation-state
  // entry; those atoms would vanish from the grid, so they get a fallback radius.
  int radiusFromCovalent = 0;      ///< atoms whose vdW radius was missing, covalent + 0.8 used
  int radiusFromDefault = 0;       ///< atoms with no radius at all, 1.8 used
  std::vector<int> fallbackElements; ///< atomic numbers that needed a fallback
};

/** Pockets in the structure, best first; empty result for no atoms. */
Result findPockets(const Fragment& frag, const Options& opt = Options());

}

#endif
