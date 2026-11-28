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

/**
 * Bridge to an external Open Babel (obabel), run as a subprocess so that no
 * Open Babel code is linked into ECCE (GPL-2.0).
 */
class DockObabel
{
  public:
    /** Path of obabel: $ECCE_OBABEL, else PATH; empty if none. */
    static std::string find();
    /** Lower-case format ids obabel reads; cached, empty without obabel. */
    static const std::vector<std::string>& readFormats();
    static bool canRead(const std::string& ext);
    /** Convert the first record of @p in to a V2000 SDF at @p outSdf. */
    static bool convertToSdf(const std::string& in, const std::string& type,
                             const std::string& outSdf, bool gen3d,
                             std::string& err);
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
    /** Tripos MOL2, first molecule; LP atoms are skipped, Du takes its
     *  element from the atom name.  Bond type ar becomes order 4. */
    static bool readMol2(const std::string& path, DockMol& mol,
                         std::string& err);
    /** PDBQT as a structure: first MODEL, element from the AutoDock type,
     *  bonds perceived, charges kept in DockAtom::charge. */
    static bool readPdbqt(const std::string& path, DockMol& mol,
                          std::string& err);
    /**
     * Any structure file by @p type (SDF, MOL, MOL2, PDBQT or a format only
     * Open Babel reads), the first record.  V3000 and the Open Babel
     * formats go through DockObabel when it is available.
     */
    static bool readStructure(const std::string& path, const std::string& type,
                              DockMol& mol, std::string& err);
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
