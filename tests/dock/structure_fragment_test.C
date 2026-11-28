// DockFragmentIO: structure files become a Fragment with the file's bond orders.
#include <cstdio>
#include <vector>

#include "tdat/DockFragmentIO.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

static int fails = 0;
static void check(bool c, const char* msg)
{
  if (!c) { printf("FAIL %s\n", msg); fails++; }
}

static int count(Fragment& f, double order)
{
  int n = 0;
  std::vector<TBond*>* b = f.bonds();
  for (size_t i = 0; i < b->size(); i++) if ((*b)[i]->order() == order) n++;
  return n;
}

// "--dump <file> <type>": the Fragment's atoms and bond orders as the Builder
// sees them (after generateBondOrders for PDBQT), for structure_formats.py.
static int dump(const std::string& path, const std::string& type)
{
  Fragment f;
  if (!DockFragmentIO::read(f, path, type)) {
    printf("ERROR %s\n", DockFragmentIO::lastError().c_str());
    return 1;
  }
  for (size_t i = 0; i < f.numAtoms(); i++)
    printf("A %s\n", f.atomRef(i)->atomicSymbol().c_str());
  std::vector<TBond*>* b = f.bonds();
  for (size_t k = 0; k < b->size(); k++) {
    int i = -1, j = -1;
    for (size_t n = 0; n < f.numAtoms(); n++) {
      if (f.atomRef(n) == (*b)[k]->atom1()) i = (int)n;
      if (f.atomRef(n) == (*b)[k]->atom2()) j = (int)n;
    }
    printf("B %d %d %.2f\n", i, j, (*b)[k]->order());
  }
  return 0;
}

int main(int argc, char** argv)
{
  if (argc == 4 && std::string(argv[1]) == "--dump") return dump(argv[2], argv[3]);
  std::string d = argv[1];
  {
    Fragment f;
    check(DockFragmentIO::read(f, d + "/benzamide.mol2", "MOL2"), "mol2 reads");
    check(f.numAtoms() == 16, "mol2: 16 atoms");
    check(count(f, TBond::Aromatic) == 6, "mol2: six aromatic bonds");
    check(f.bonds()->size() == 16, "mol2: 16 bonds");
  }
  {
    Fragment f;
    check(DockFragmentIO::read(f, d + "/benzamide.sdf", "SDF"), "sdf reads");
    check(count(f, TBond::Double) == 4, "sdf: four double bonds");
  }
  {
    Fragment f;
    check(DockFragmentIO::read(f, d + "/poses.pdbqt", "PDBQT"), "pdbqt reads");
    check(f.numAtoms() == 11, "pdbqt: first model only");
    check(count(f, TBond::Aromatic) + count(f, TBond::Double) >= 3,
          "pdbqt: bond orders from distances");
  }
  {
    Fragment f;
    check(!DockFragmentIO::read(f, d + "/benzamide.sdf", "XYZZY"), "unknown type is refused");
  }
  printf(fails ? "FAILED\n" : "ok\n");
  return fails ? 1 : 0;
}
