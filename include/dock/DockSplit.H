/**
 * @file DockSplit.H
 *
 * The receptor/ligand split every docking code uses, in one wx-free place:
 * the Builder dialogs, the deck generators and the scripts all ask this and
 * so cannot choose different ligands for the same structure (#1335).  The
 * rule is the one the former scripts/parsers/docking.pl implemented.
 */
#ifndef DOCKSPLIT_HH
#define DOCKSPLIT_HH

#include <string>
#include <vector>

struct DockSplitAtom
{
  std::string element;     ///< "C", "Cl"; anything unknown is treated as r=1.5
  double x = 0.0, y = 0.0, z = 0.0;
  std::string resName;     ///< empty when the input has no residues
  std::string resNum;
  std::string chain;
};

struct DockSplitResult
{
  bool ok = false;
  std::string error;                 ///< "docking error: ...", when !ok
  std::vector<int> receptor, ligand; ///< 0-based, input order
  std::vector<std::string> warnings; ///< advice, never blocking
};

class DockSplit
{
  public:
    /** @p spec is the Ligand atoms text ("1-5,9", 1-based); empty = automatic. */
    static DockSplitResult split(const std::vector<DockSplitAtom>& atoms,
                                 const std::string& spec);

    /** 0-based indices as "1-5,9". */
    static std::string ranges(const std::vector<int>& zeroBased);

    /** ATOM/HETATM records of the first model. */
    static bool readPdb(const std::string& path,
                        std::vector<DockSplitAtom>& atoms, std::string& err);

    /**
     * ECCE .frag, as ai.vina read it: atom_list (symbols, or atomic numbers
     * unless atom_info says "symbol") and, when @p residues, the pdb_list.
     */
    static bool readFrag(const std::string& path, bool residues,
                         std::vector<DockSplitAtom>& atoms, std::string& err);

    /** The tab-separated summary the golden corpus is kept in. */
    static std::string report(const std::vector<DockSplitAtom>& atoms,
                              const DockSplitResult& r);
};

#endif
