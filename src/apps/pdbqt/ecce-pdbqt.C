/**
 * ecce-pdbqt - prepare AutoDock PDBQT files for a docking run.
 *
 * The same library backs the Calculation Editor and the job script, so what
 * the command writes and what a Vina calculation writes cannot drift apart.
 */
#include "dock/DockPrep.H"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static void usage()
{
  std::cerr <<
    "usage: ecce-pdbqt --frag FILE.frag --atoms LIST --as ligand|receptor -o OUT.pdbqt\n"
    "       ecce-pdbqt --ligand FILE.sdf [-o OUT.pdbqt]\n"
    "       ecce-pdbqt --receptor FILE.pdb [--keep-residue NAMES] [-o OUT.pdbqt]\n"
    "       ecce-pdbqt --info FILE\n"
    "\n"
    "  --frag FILE        ECCE fragment file; its bond orders are used when\n"
    "                     it has them, otherwise bonds come from the geometry\n"
    "  --atoms LIST       1-based atoms of this part, e.g. 1-5,9,12-20\n"
    "  --as ROLE          ligand (flexible, torsion tree) or receptor (rigid)\n"
    "  --ligand FILE      .sdf/.mol with explicit hydrogens\n"
    "  --ligand-residue N HETATM residue name when --ligand is a .pdb\n"
    "  --receptor FILE    .pdb; polar hydrogens are added at pH 7\n"
    "  --keep-residue NS  cofactors and metals to keep in the receptor\n"
    "  --flexible-amides  let amide C-N bonds rotate (off by default)\n"
    "  --no-add-hydrogens take the receptor's hydrogens as they are\n"
    "  --map FILE         ligand only: PDBQT atom order as 1-based input atoms\n"
    "  --name NAME        ligand name written into the REMARK\n"
    "  --info FILE        print types, rotatable bonds and the tree, for tests\n";
}

/** "1-5,9" -> 0-based indices.  Empty means every atom. */
static std::vector<int> parseAtoms(const std::string& spec, int natoms)
{
  std::vector<int> out;
  if (spec.empty()) {
    for (int i = 0; i < natoms; i++) out.push_back(i);
    return out;
  }
  std::string s = spec;
  for (size_t i = 0; i < s.size(); i++) if (s[i] == ',' || s[i] == ';') s[i] = ' ';
  std::istringstream ss(s);
  std::string tok;
  while (ss >> tok) {
    size_t d = tok.find('-', 1);
    int a, b;
    if (d == std::string::npos) a = b = atoi(tok.c_str());
    else { a = atoi(tok.substr(0,d).c_str()); b = atoi(tok.substr(d+1).c_str()); }
    for (int i = a; i <= b; i++) if (i >= 1 && i <= natoms) out.push_back(i-1);
  }
  return out;
}

int main(int argc, char** argv)
{
  std::string frag, atomSpec, role, ligand, receptor, info, out, mapOut;
  std::string hetRes, keepRes, name = "ligand";
  bool flexAmides = false, addH = true;

  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    bool n = (i + 1 < argc);
    if      (a == "--frag" && n)             frag = argv[++i];
    else if (a == "--atoms" && n)            atomSpec = argv[++i];
    else if (a == "--as" && n)               role = argv[++i];
    else if (a == "--ligand" && n)           ligand = argv[++i];
    else if (a == "--receptor" && n)         receptor = argv[++i];
    else if (a == "--info" && n)             info = argv[++i];
    else if ((a == "-o" || a == "--out") && n) out = argv[++i];
    else if (a == "--map" && n)              mapOut = argv[++i];
    else if (a == "--ligand-residue" && n)   hetRes = argv[++i];
    else if (a == "--keep-residue" && n)     keepRes = argv[++i];
    else if (a == "--name" && n)             name = argv[++i];
    else if (a == "--flexible-amides")       flexAmides = true;
    else if (a == "--no-add-hydrogens")      addH = false;
    else { usage(); return 2; }
  }

  bool asReceptor = (role == "receptor") || (!receptor.empty() && role.empty());
  if (!frag.empty() && role.empty()) { usage(); return 2; }

  DockMol whole;
  std::string err;
  bool ok = false;
  if (!frag.empty()) {
    ok = DockReader::readFrag(frag, whole, err);
  } else {
    std::string src = !ligand.empty() ? ligand
                    : (!receptor.empty() ? receptor : info);
    if (src.empty()) { usage(); return 2; }
    bool isPdb = src.size() > 4 && (src.substr(src.size()-4) == ".pdb" ||
                                    src.substr(src.size()-4) == ".ent");
    ok = isPdb
      ? DockReader::readPdb(src, whole,
                            asReceptor ? keepRes
                                       : (hetRes.empty() ? "LIG" : hetRes), err)
      : DockReader::readSdf(src, whole, err);
  }
  if (!ok) { std::cerr << "ecce-pdbqt: " << err << "\n"; return 1; }

  DockMol mol;
  std::vector<int> map;
  if (!frag.empty()) {
    std::vector<int> keep = parseAtoms(atomSpec, whole.numAtoms());
    if (keep.empty()) { std::cerr << "ecce-pdbqt: --atoms selects nothing\n"; return 1; }
    whole.subset(keep, mol, map);
    // The subset keeps the bonds it inherited; rings and aromaticity have to
    // be found again because a split can break a ring that spanned the two.
    mol.perceive(mol.bonds.empty());
  } else {
    mol = whole;
    for (int i = 0; i < mol.numAtoms(); i++) map.push_back(i);
  }

  if (!asReceptor && addH) {
    // A ligand taken from a crystal structure has no hydrogens at all; the
    // polar ones decide the HD typing, so put them back.
    bool anyH = false;
    for (int i = 0; i < mol.numAtoms(); i++) if (mol.atoms[i].z == 1) anyH = true;
    if (!anyH && DockProtonator::completePolarHydrogens(mol) > 0) {
      mol.perceive(false);
      while ((int)map.size() < mol.numAtoms()) map.push_back(-1);
    }
  }
  if (asReceptor && addH) {
    DockProtonator::addPolarHydrogens(mol);
    mol.perceive(false);
    while ((int)map.size() < mol.numAtoms()) map.push_back(-1);
  }

  DockTyper::assign(mol);
  DockGasteiger::assign(mol);
  DockPdbqtWriter::mergeNonpolarHydrogens(mol);

  DockTorsionTree tree;
  if (!asReceptor) tree.build(mol, flexAmides);

  if (!info.empty()) {
    int nrot = 0;
    for (size_t k = 0; k < mol.bonds.size(); k++) if (mol.bonds[k].rotatable) nrot++;
    printf("atoms %d\n", mol.numAtoms());
    printf("torsions %d\n", nrot);
    for (int i = 0; i < mol.numAtoms(); i++)
      printf("atom %d %s %s %s\n", i+1,
             DockMol::symbolFromElement(mol.atoms[i].z).c_str(),
             mol.atoms[i].adType.c_str(),
             mol.atoms[i].merged ? "merged" : "kept");
    for (size_t k = 0; k < mol.bonds.size(); k++)
      if (mol.bonds[k].rotatable)
        printf("rotatable %d %d\n", mol.bonds[k].i+1, mol.bonds[k].j+1);
    for (size_t n = 0; n < tree.nodes.size(); n++)
      printf("node %d parent %d child %d size %d root %d\n", (int)n,
             tree.nodes[n].parentAtom+1, tree.nodes[n].childAtom+1,
             (int)tree.nodes[n].atoms.size(),
             (int)n == tree.rootNode() ? 1 : 0);
    return 0;
  }

  std::string text;
  std::vector<int> order;
  if (asReceptor) text = DockPdbqtWriter::receptor(mol);
  else            text = DockPdbqtWriter::ligand(mol, tree, name, &order);

  if (out.empty()) std::cout << text;
  else {
    std::ofstream os(out.c_str());
    if (!os) { std::cerr << "ecce-pdbqt: cannot write " << out << "\n"; return 1; }
    os << text;
  }

  if (!mapOut.empty() && !asReceptor) {
    std::ofstream ms(mapOut.c_str());
    if (!ms) { std::cerr << "ecce-pdbqt: cannot write " << mapOut << "\n"; return 1; }
    for (size_t k = 0; k < order.size(); k++) {
      int idx = order[k];
      ms << ((idx >= 0 && idx < (int)map.size()) ? map[idx] + 1 : 0) << "\n";
    }
  }
  return 0;
}
