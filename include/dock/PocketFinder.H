/**
 * @file PocketFinder.H
 *
 * LIGSITE-style binding-pocket detection (#1320).  A grid is laid over the
 * structure, each point is marked protein or free from van der Waals radii,
 * and each free point is scored by how many of 7 directions (3 axes, 4 cube
 * diagonals) are closed by protein on BOTH sides within maxRay.  Contiguous
 * points scoring at least minScore form a pocket.  Everything here is wx-free.
 *
 * LIMITATION: detection is good but ranking is not.  On 35 diverse complexes
 * the real ligand site is among the top five pockets about 86% of the time
 * but ranked first only about 40% of the time.  Callers must not present the
 * best-ranked pocket as the answer; offer several and let the user choose.
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

enum class Ranking {
  ByVolume,       ///< enclosure-weighted volume (the default)
  ByDescriptors   ///< that score times apolar fraction, enclosure and closedness, each to a weight
};

struct Options {
  double spacing = 1.0;       ///< grid step, Angstrom
  double margin = 4.0;        ///< grid padding around the atoms, Angstrom
  double probe = 1.4;         ///< a pocket point must fit a sphere this big, Angstrom
  double maxRay = 12.0;       ///< how far a ray looks for protein, Angstrom
  // minScore/minVolume: 5/20 discarded real pockets before they could be
  // ranked. 3/10 was chosen against 35 diverse complexes (best pocket within
  // 8 A of the ligand: top-5 63% -> 86%) and coronavirus main-protease
  // structures with the ligand removed (two found no pocket at 5/20).
  int minScore = 3;           ///< of 7 directions
  double minVolume = 10.0;    ///< smaller clusters are dropped, cubic Angstrom
  double liningDistance = 4.0;///< protein atom within this of a pocket point
  bool ignoreWater = true;    ///< waters do not close a pocket
  bool ignoreLigands = true;  ///< bound ligands are removed, so their site shows
  int threads = 0;            ///< 0 = hardware concurrency
  Ranking ranking = Ranking::ByVolume;
  // Exponents of the three descriptors in the alternative ranking; 0 drops one.
  double wApolar = 1.0, wEnclosure = 1.0, wMouth = 1.0;
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
  double score = 0;                ///< enclosure-weighted volume; the default ranking key
  double apolarFraction = 0;       ///< lining atoms that are C or S, 0..1
  double mouthFraction = 0;        ///< pocket points open to free space on a face, 0..1; low = closed
  double rankScore = 0;            ///< the key actually used for ordering under Options::ranking
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
