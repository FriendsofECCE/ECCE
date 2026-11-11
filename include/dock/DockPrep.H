/**
 * @file DockPrep.H
 *
 * PDBQT preparation: AutoDock atom types, Gasteiger charges, the ligand
 * torsion tree and the two writers.  Everything here is wx-free so that the
 * same code serves the GUI, the ecce-pdbqt command and the tests.
 */
#ifndef DOCKPREP_HH
#define DOCKPREP_HH

#include "dock/DockMol.H"

#include <string>
#include <vector>

/** Assign the AutoDock 4 atom types Vina reads. */
class DockTyper
{
  public:
    /** Fills DockAtom::adType.  The molecule must have been perceived. */
    static void assign(DockMol& mol);
};

/** Gasteiger-Marsili partial equalisation of orbital electronegativity. */
class DockGasteiger
{
  public:
    /** Fills DockAtom::charge; returns false if an element has no parameters. */
    static bool assign(DockMol& mol);
};

/** One node of the ligand torsion tree. */
struct DockTorsionNode
{
  std::vector<int> atoms;        ///< atom indices of this rigid group
  int parentAtom = -1;           ///< atom in the parent group of the branch bond
  int childAtom = -1;            ///< atom in this group of the branch bond
  std::vector<int> children;     ///< indices into DockTorsionTree::nodes
};

/** ROOT/BRANCH decomposition of a flexible ligand. */
class DockTorsionTree
{
  public:
    /**
     * Marks rotatable bonds and builds the tree.  @p flexibleAmides keeps
     * amide C-N bonds rotatable; off by default, as in AutoDock.
     */
    void build(DockMol& mol, bool flexibleAmides = false);

    int numTorsions() const { return (int)nodes.size() - 1; }
    int rootNode() const { return p_root; }

    std::vector<DockTorsionNode> nodes;

  private:
    int p_root = 0;
};

/** PDBQT output. */
class DockPdbqtWriter
{
  public:
    /** Flexible ligand with its torsion tree. */
    /** @p order, if given, receives the atom indices in written order. */
    static std::string ligand(DockMol& mol, const DockTorsionTree& tree,
                              const std::string& name,
                              std::vector<int>* order = 0);
    /** Rigid receptor: plain ATOM records, no tree. */
    static std::string receptor(DockMol& mol);
    /** Drop nonpolar hydrogens, adding their charge to the heavy atom. */
    static void mergeNonpolarHydrogens(DockMol& mol);
};

/** Readers for the formats the preparation accepts. */
class DockReader
{
  public:
    /** MDL mol/SDF (V2000), first record only.  Bond orders come from the file. */
    static bool readSdf(const std::string& path, DockMol& mol, std::string& err);
    /**
     * PDB.  @p hetSelect picks HETATM residues by name (empty = protein
     * ATOM records only, waters always dropped).  Bonds are perceived.
     */
    /** ECCE .frag; uses its bond_list when the file has one. */
    static bool readFrag(const std::string& path, DockMol& mol,
                         std::string& err);
    static bool readPdb(const std::string& path, DockMol& mol,
                        const std::string& hetSelect, std::string& err);
    /** Read a PDBQT back in; used to turn Vina poses into coordinates. */
    static bool readPdbqtModels(const std::string& path,
                                std::vector< std::vector<double> >& coords,
                                std::vector<double>& scores,
                                std::string& err);
};

/**
 * Standard protonation at pH 7 for a protein read from a PDB file: the polar
 * hydrogens AutoDock wants (N-H, O-H, S-H), placed from residue templates.
 * Nonpolar hydrogens are never added because the receptor is written with
 * them merged anyway.
 */
class DockProtonator
{
  public:
    /** Returns the number of hydrogens added. */
    static int addPolarHydrogens(DockMol& mol);
    /**
     * The same placement for a ligand read without hydrogens: neutral N, O
     * and S get whatever hydrogens their valence is short of.
     */
    static int completePolarHydrogens(DockMol& mol);
};

#endif
