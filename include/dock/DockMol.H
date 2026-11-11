/**
 * @file DockMol.H
 *
 * A minimal, wx-free molecule used only by the PDBQT preparation.  It is
 * deliberately not TFragment: the preparation has to run from a command line
 * and from a PDB file that never becomes an ECCE structure, so it keeps its
 * own container and gets fed by thin adapters.
 */
#ifndef DOCKMOL_HH
#define DOCKMOL_HH

#include <string>
#include <vector>

/** Hybridisation as far as the AutoDock typing cares. */
enum DockHybrid { DOCK_HYB_UNKNOWN = 0, DOCK_HYB_SP = 1, DOCK_HYB_SP2 = 2,
                  DOCK_HYB_SP3 = 3 };

struct DockAtom
{
  int z = 0;                     ///< atomic number
  double x = 0.0, y = 0.0, zc = 0.0;
  int formalCharge = 0;
  double charge = 0.0;           ///< partial charge written to the PDBQT column
  std::string name = "";         ///< PDB atom name, blank for ligands from SDF
  std::string resName = "UNL";
  std::string chain = "";
  int resSeq = 1;
  std::string adType = "";       ///< AutoDock type, filled by DockTyper
  bool aromatic = false;
  bool inRing = false;
  DockHybrid hybrid = DOCK_HYB_UNKNOWN;
  bool donorOnly = false;      ///< residue says it donates but never accepts
  bool merged = false;           ///< nonpolar H folded into its heavy atom
  int serial = 0;                ///< PDBQT serial, filled by the writer
};

struct DockBond
{
  int i = 0, j = 0;
  int order = 1;                 ///< 1, 2, 3; 4 means "aromatic" as read in
  bool inRing = false;
  bool rotatable = false;
};

class DockMol
{
  public:
    std::vector<DockAtom> atoms;
    std::vector<DockBond> bonds;

    int numAtoms() const { return (int)atoms.size(); }

    void addBond(int i, int j, int order);
    /** Bond index for the i-j pair, or -1. */
    int findBond(int i, int j) const;
    /** Neighbour atom indices of @p i, built on demand. */
    const std::vector<int>& neighbors(int i) const;
    /** Neighbours of @p i that are not hydrogen. */
    std::vector<int> heavyNeighbors(int i) const;

    /** Sum of bond orders, i.e. the SMARTS "v" primitive. */
    int valence(int i) const;
    /** Number of connections, i.e. the SMARTS "X" primitive. */
    int connections(int i) const { return (int)neighbors(i).size(); }

    /** Bonds from interatomic distances; used for PDB input only. */
    void perceiveBonds();
    /** Smallest set of smallest rings, by size, up to ring size 10. */
    void perceiveRings();
    /** Ring flags, hybridisation and aromaticity.  Call after rings. */
    void perceiveAromaticity();
    /**
     * Aromaticity from the ring's own geometry, for input that carries no
     * bond orders: a planar ring of three-connected C/N/O/S.  This is the
     * criterion AutoDock itself has always used on PDB receptors.
     */
    void perceiveAromaticityPlanar();
    /** Copy @p keep (atom indices) into @p out; @p map gives out -> this. */
    void subset(const std::vector<int>& keep, DockMol& out,
                std::vector<int>& map) const;
    /** Everything the typing needs, in the right order. */
    void perceive(bool withBonds);

    const std::vector< std::vector<int> >& rings() const { return p_rings; }

    static int elementFromSymbol(const std::string& sym);
    static std::string symbolFromElement(int z);
    static double covalentRadius(int z);
    static bool isMetal(int z);

  private:
    void buildAdjacency() const;

    mutable std::vector< std::vector<int> > p_adj;
    mutable bool p_adjValid = false;
    bool p_geomBonds = false;   ///< bonds came from distances, not a file
    std::vector< std::vector<int> > p_rings;
};

#endif
