/**
 * ecce-docksplit - the receptor/ligand split for docking (#1335).  The
 * scripts call this instead of carrying their own copy of the rule.
 */
#include "dock/DockSplit.H"

#include <cstring>
#include <iostream>

static void usage()
{
  std::cerr <<
    "usage: ecce-docksplit (FILE.pdb | --frag FILE.frag) [--atoms LIST]\n"
    "         [--print ligand|receptor|both|report] [--no-residues] [--prog NAME]\n"
    "  --atoms LIST   1-based ligand atoms, e.g. 1-5,9; default is automatic\n"
    "  --print WHAT   ligand (default): its atoms as ranges; receptor: the\n"
    "                 receptor's; both: receptor line, then ligand line;\n"
    "                 report: the full tab-separated summary\n"
    "  --frag FILE    read an ECCE fragment file (with its pdb_list residues)\n"
    "  --no-residues  ignore the residue names: element and size rules only\n"
    "  --prog NAME    the name warnings are printed under (default ecce-docksplit)\n"
    "Warnings go to stderr as 'note [LIGAND_ATOMS]: ...'; an error exits 1.\n";
}

int main(int argc, char** argv)
{
  std::string file, frag, spec, prog = "ecce-docksplit", what = "ligand";
  bool residues = true;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--atoms") && i + 1 < argc) spec = argv[++i];
    else if (!strcmp(argv[i], "--print") && i + 1 < argc) what = argv[++i];
    else if (!strcmp(argv[i], "--frag") && i + 1 < argc) frag = argv[++i];
    else if (!strcmp(argv[i], "--prog") && i + 1 < argc) prog = argv[++i];
    else if (!strcmp(argv[i], "--no-residues")) residues = false;
    else if (argv[i][0] != '-' && file.empty()) file = argv[i];
    else { usage(); return 2; }
  }
  if (file.empty() == frag.empty() ||
      (what != "ligand" && what != "receptor" && what != "both" && what != "report")) {
    usage();
    return 2;
  }
  std::vector<DockSplitAtom> atoms;
  std::string err;
  if (!(frag.empty() ? DockSplit::readPdb(file, atoms, err)
                     : DockSplit::readFrag(frag, residues, atoms, err))) { std::cerr << err << "\n"; return 1; }
  DockSplitResult r = DockSplit::split(atoms, spec);
  if (!r.ok) { std::cerr << r.error << "\n"; return 1; }
  for (size_t i = 0; i < r.warnings.size(); i++)
    std::cerr << prog << " note [LIGAND_ATOMS]: " << r.warnings[i] << "\n";
  if (what == "report") std::cout << DockSplit::report(atoms, r);
  else if (what == "both")
    std::cout << DockSplit::ranges(r.receptor) << "\n" << DockSplit::ranges(r.ligand) << "\n";
  else std::cout << DockSplit::ranges(what == "ligand" ? r.ligand : r.receptor) << "\n";
  return 0;
}
