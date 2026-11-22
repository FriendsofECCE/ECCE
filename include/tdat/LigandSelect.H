#ifndef LIGANDSELECT_HH
#define LIGANDSELECT_HH

#include <string>
#include <vector>

class Fragment;
class Residue;

/// Which residues are protein, water, ion, crystallisation additive or a
/// likely ligand; the one rule for the Builder's Select Ligand and the
/// docking ligand choice.  No viewer or GUI dependency, so the Calculation
/// Editor can use it.
namespace LigandSelect {

enum ResidueClass { RES_POLYMER, RES_WATER, RES_ION, RES_ADDITIVE, RES_OTHER };

struct Candidate {
  std::string name;
  int number;
  int atoms;
  int firstAtom;
  ResidueClass cls;
  std::vector<int> atomIndices;   // sorted, 0-based
};

ResidueClass residueClass(const std::string& residueName);
std::string residueKey(const Residue *res);
bool isWaterName(const std::string& name);
bool isIonName(const std::string& name);

/// Non-polymer, non-water residues; the likely ligand first, then additives,
/// then single atoms and ions.
std::vector<Candidate> ligandCandidates(const Fragment& frag);

/// 0-based atom indices as the 1-based list Ligand atoms uses, "1-5,9".
std::string atomRangeString(std::vector<int> indices);

}

#endif
