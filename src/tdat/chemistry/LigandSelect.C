#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "util/STLUtil.H"
#include "tdat/Fragment.H"
#include "tdat/Residue.H"
#include "tdat/TAtm.H"
#include "tdat/TResItem.H"
#include "tdat/TResTab.H"
#include "tdat/LigandSelect.H"

using std::string;
using std::set;
using std::map;
using std::vector;

namespace LigandSelect {

string residueKey(const Residue *res)
{
  string name = res->name();
  STLUtil::stripLeadingAndTrailingWhiteSpace(name);
  for (char& c : name) c = toupper((unsigned char)c);
  return name;
}

bool isWaterName(const string& name)
{
  static const set<string> names = {"SOL", "HOH", "WAT", "TIP3", "TIP4",
      "TIP5", "SPC", "SPCE", "H2O", "T3P", "T4P", "DOD"};
  return names.count(name) > 0;
}

bool isIonName(const string& name)
{
  static const set<string> names = {"NA", "CL", "K", "MG", "CA", "ZN",
      "LI", "RB", "CS", "F", "BR", "I", "BA", "SR", "CD", "MN", "FE", "CO",
      "NI", "CU", "NA+", "CL-", "K+", "LI+", "RB+", "CS+", "F-", "BR-", "I-",
      "MG2+", "CA2+", "ZN2+", "SOD", "CLA", "POT", "CAL", "CES", "IOD"};
  return names.count(name) > 0;
}

// Names only: what a PDB or mmCIF file gives.  Terminal forms are AMBER's
// four-character NALA/CALA; ACE/NME cap a chain and belong to it.
ResidueClass residueClass(const string& residueName)
{
  string key = residueName;
  STLUtil::stripLeadingAndTrailingWhiteSpace(key);
  for (char& c : key) c = toupper((unsigned char)c);
  static const std::set<std::string> extraPolymer = {"ACE", "NME", "NMA", "MSE", "HSD",
      "HSE", "HSP", "CYM", "ASH", "GLH", "LYN"};
  static const std::set<std::string> additives = {"GOL", "EDO", "SO4", "PO4", "ACT",
      "DMS", "DMSO", "PEG", "PGE", "PG4", "P6G", "1PE", "PE4", "MPD", "BME",
      "FMT", "NO3", "TRS", "EPE", "MES", "IPA", "EOH", "ACY", "SO3", "CIT",
      "TAR", "HED", "MRD", "IMD", "BU3", "PGO", "1BO", "DTT", "DTV", "BTB",
      "BCT", "CO3", "SCN", "AZI", "OXL", "MLI", "SUC", "GLC", "PGR", "TLA"};
  // Called per atom: TResTab reads ResidueList from disk when constructed.
  static std::map<std::string, ResidueClass> known;
  std::map<std::string, ResidueClass>::const_iterator hit = known.find(key);
  if (hit != known.end()) return hit->second;
  static TResTab restab;
  // TResTab holds names in PDB column form ("ALA "), as TResItem pads them.
  auto verify = [](const string& name) {
    // TResItem writes past a 4-character buffer for longer names.
    if (name.empty() || name.size() > 4) return TResItem::Other;
    return restab.verify(TResItem(name.c_str(), TResItem::Other).name().c_str());
  };
  ResidueClass c = RES_OTHER;
  if (verify(key) != TResItem::Other || extraPolymer.count(key) > 0)
    c = RES_POLYMER;
  else if (key.size() == 4 && (key[0] == 'N' || key[0] == 'C') &&
           verify(key.substr(1)) == TResItem::AminoAcid)
    c = RES_POLYMER;
  else if (isWaterName(key)) c = RES_WATER;
  else if (isIonName(key)) c = RES_ION;
  else if (additives.count(key) > 0) c = RES_ADDITIVE;
  known[key] = c;
  return c;
}

vector<Candidate> ligandCandidates(const Fragment& frag)
{
  vector<Candidate> out;
  if (frag.numResidues() == 0) return out;
  map<const Residue*, int> index;
  for (int i = 0; i < frag.numAtoms(); i++) {
    const Residue *res = frag.atomRef(i)->getResidue();
    if (!res) continue;
    const ResidueClass c = residueClass(res->name());
    if (c == RES_POLYMER || c == RES_WATER) continue;
    map<const Residue*, int>::iterator it = index.find(res);
    if (it == index.end()) {
      Candidate cand;
      cand.name = residueKey(res);
      cand.number = res->number();
      cand.atoms = 0;
      cand.firstAtom = i;
      cand.cls = c;
      index[res] = out.size();
      out.push_back(cand);
      it = index.find(res);
    }
    out[it->second].atoms++;
    out[it->second].atomIndices.push_back(i);
  }
  auto rank = [](const Candidate& c) {
    if (c.cls == RES_ION || c.atoms < 2) return 2;
    return c.cls == RES_ADDITIVE ? 1 : 0;
  };
  std::stable_sort(out.begin(), out.end(),
      [&rank](const Candidate& a, const Candidate& b) {
        if (rank(a) != rank(b)) return rank(a) < rank(b);
        return a.atoms > b.atoms;
      });
  return out;
}

string atomRangeString(vector<int> idx)
{
  std::sort(idx.begin(), idx.end());
  string ranges;
  for (size_t i = 0; i < idx.size(); ) {
    size_t j = i;
    while (j + 1 < idx.size() && idx[j + 1] == idx[j] + 1) j++;
    if (!ranges.empty()) ranges += ",";
    ranges += std::to_string(idx[i] + 1);
    if (j > i) ranges += "-" + std::to_string(idx[j] + 1);
    i = j + 1;
  }
  return ranges;
}

}
