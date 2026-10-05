// Loads structure files through FragUtil::load with the Builder's
// Import Chemical System arguments and checks atoms, residues and bonds.
// Usage: fragreaders <dir>   (dir holds the sample files)
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "tdat/FragUtil.H"
#include "tdat/Fragment.H"
#include "tdat/Residue.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

using namespace std;

static int failures = 0;

static void check(bool ok, const string& what)
{
  cout << (ok ? "ok   " : "FAIL ") << what << endl;
  if (!ok) failures++;
}

// The Builder: genBondOrders true, model 1, altLoc " ", chain " " unless
// the PDB options window picked something else.
static Fragment *load(const string& path, const string& type,
                      const string& alt = " ", const string& chain = " ",
                      int model = 1)
{
  ifstream is(path.c_str());
  if (!is.good()) { check(false, "open " + path); return 0; }
  Fragment *frag = new Fragment;
  FragUtil util(frag);
  bool ok = util.load(is, type, 1.0, true, model, alt, chain);
  check(ok, "load " + path);
  return frag;
}

// Every atom must carry a residue from the fragment's own list.
static void checkResidues(Fragment *frag, const string& tag, size_t nres)
{
  vector<Residue*> res = frag->residues();
  check(res.size() == nres, tag + ": " + to_string(nres) + " residues (got "
        + to_string(res.size()) + ")");
  set<Residue*> known(res.begin(), res.end());
  bool allOwned = true;
  for (int i = 0; i < frag->numAtoms(); i++) {
    if (known.count(frag->atomRef(i)->getResidue()) == 0) allOwned = false;
  }
  check(allOwned, tag + ": every atom points at one of its residues");
}

int main(int argc, char **argv)
{
  string dir = argc > 1 ? argv[1] : ".";

  for (const char *f : {"glycine.pdb", "glycine.ent"}) {
    Fragment *frag = load(dir + "/" + f, "PDB");
    if (!frag) continue;
    string tag = f;
    check(frag->numAtoms() == 10, tag + ": 10 atoms");
    check(frag->numBonds() == 9, tag + ": 9 bonds (got "
          + to_string(frag->numBonds()) + ")");
    checkResidues(frag, tag, 1);
    delete frag;
  }

  {
    // HETATM, TER, two chains, no CONECT.
    string tag = "chains.pdb";
    Fragment *frag = load(dir + "/chains.pdb", "PDB");
    if (frag) {
      check(frag->numAtoms() == 11, tag + ": 11 atoms");
      checkResidues(frag, tag, 5);
      delete frag;
    }
    frag = load(dir + "/chains.pdb", "PDB", " ", "B");
    if (frag) {
      check(frag->numAtoms() == 4, tag + " chain B: 4 atoms");
      checkResidues(frag, tag + " chain B", 1);
      delete frag;
    }
  }

  {
    string tag = "altloc.pdb";
    Fragment *frag = load(dir + "/altloc.pdb", "PDB", "A");
    if (frag) {
      check(frag->numAtoms() == 5, tag + " A: 5 atoms");
      checkResidues(frag, tag + " A", 1);
      delete frag;
    }
    frag = load(dir + "/altloc.pdb", "PDB", "B");
    if (frag) {
      check(frag->numAtoms() == 5, tag + " B: 5 atoms");
      delete frag;
    }
  }

  {
    string tag = "models.pdb";
    Fragment *frag = load(dir + "/models.pdb", "PDB", " ", " ", 2);
    if (frag) {
      check(frag->numAtoms() == 3, tag + " model 2: 3 atoms");
      check(frag->numAtoms() > 0 && frag->atomRef(0)->coordinates()[0] > 9.0,
            tag + " model 2: model 2 coordinates");
      checkResidues(frag, tag + " model 2", 1);
      delete frag;
    }
  }

  {
    string tag = "short.pdb";
    Fragment *frag = load(dir + "/short.pdb", "PDB");
    if (frag) {
      check(frag->numAtoms() == 3, tag + ": 3 atoms");
      checkResidues(frag, tag, 1);
      delete frag;
    }
  }

  {
    string tag = "benzene.xyz";
    Fragment *frag = load(dir + "/benzene.xyz", "XYZ");
    if (frag) {
      check(frag->numAtoms() == 12, tag + ": 12 atoms");
      check(frag->numBonds() == 12, tag + ": 12 bonds (got "
            + to_string(frag->numBonds()) + ")");
      delete frag;
    }
  }

  {
    string tag = "benzene.car";
    Fragment *frag = load(dir + "/benzene.car", "CAR");
    if (frag) {
      check(frag->numAtoms() == 12, tag + ": 12 atoms (got "
            + to_string(frag->numAtoms()) + ")");
      check(frag->numBonds() == 12, tag + ": 12 bonds (got "
            + to_string(frag->numBonds()) + ")");
      check(frag->getLattice() == 0, tag + ": no lattice");
      delete frag;
    }
  }

  {
    string tag = "nacl_periodic.car";
    Fragment *frag = load(dir + "/nacl_periodic.car", "CAR");
    if (frag) {
      check(frag->numAtoms() == 8, tag + ": 8 atoms (got "
            + to_string(frag->numAtoms()) + ")");
      check(frag->getLattice() != 0, tag + ": lattice");
      delete frag;
    }
  }

  {
    string tag = "blank.car";
    Fragment *frag = load(dir + "/blank.car", "CAR");
    if (frag) {
      check(frag->numAtoms() == 3, tag + ": 3 atoms (got "
            + to_string(frag->numAtoms()) + ")");
      delete frag;
    }
  }

  cout << (failures ? "FAILED" : "PASSED") << " (" << failures
       << " failures)" << endl;
  return failures ? 1 : 0;
}
