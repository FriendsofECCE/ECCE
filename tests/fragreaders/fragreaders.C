// Loads structure files through FragUtil::load with the Builder's
// Add Structure from File arguments and checks atoms, residues and bonds.
// Usage: fragreaders <dir>   (dir holds the sample files)
#include <clocale>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "util/Ecce.H"
#include "tdat/FragUtil.H"
#include "tdat/LatticeDef.H"
#include "util/Transformations.H"
#include "tdat/Fragment.H"
#include "tdat/Residue.H"
#include "tdat/TAtm.H"
#include "tdat/TPerTab.H"
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

  // The startup every ECCE program runs; under LC_ALL=de_DE.utf8 it must
  // still read and print numbers with a '.'.
  if (getenv("LC_ALL") && !setlocale(LC_ALL, "")) return 77;  // not installed
  Ecce::initialize();
  char num[32];
  snprintf(num, sizeof num, "%.1f", 2.5);
  check(string(num) == "2.5", string("printf of 2.5 gives ") + num);

  //  A GROMACS structure attached to a task is stored as PDB and read back
  //  the way MdTask::getFragment reads it (#262): atoms, elements, bonds.
  {
    ifstream gin(dir + "/../../gromacs_md/fixtures/conf.gro");
    Fragment gro;
    if (gin && gro.restoreGRO(gin, true)) {
      ostringstream pdb;
      gro.dumpPDB(pdb, false);
      istringstream pin(pdb.str());
      Fragment back;
      back.restorePDB(pin, 1.0, true, 1, " ", true, false, " ");
      int h0 = 0, h1 = 0;
      for (int i = 0; i < (int)gro.numAtoms(); i++)
        if (gro.atomRef(i)->atomicSymbol() == "H") h0++;
      for (int i = 0; i < (int)back.numAtoms(); i++)
        if (back.atomRef(i)->atomicSymbol() == "H") h1++;
      cout << "gro: atoms " << gro.numAtoms() << " H " << h0 << " bonds "
           << gro.numBonds() << "; stored and read back: atoms "
           << back.numAtoms() << " H " << h1 << " bonds " << back.numBonds()
           << endl;
      check(back.numAtoms() == gro.numAtoms() && h1 == h0 && h0 > 0,
            "attached .gro: every atom and hydrogen survives the task's PDB");
      check(back.numBonds() == gro.numBonds() && gro.numBonds() > 0,
            "attached .gro: the bonds survive the task's PDB");
    } else {
      check(false, "conf.gro fixture read");
    }
  }

  for (const char *f : {"glycine.pdb", "glycine.ent"}) {
    Fragment *frag = load(dir + "/" + f, "PDB");
    if (!frag) continue;
    string tag = f;
    check(frag->numAtoms() == 10, tag + ": 10 atoms");
    if (frag->numAtoms() > 1) {
      const double *c = frag->atomRef(1)->coordinates();
      check(c[0] == 2.457 && c[2] == 0.099, tag + ": atom 2 at 2.457 -0.120 0.099");
    }
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

  // GROMACS .gro: nm in the file, Angstrom out; the element comes from the
  // atom name (MINFF names are mixed case, ions repeat the residue name,
  // a protein CA is carbon); the last line is the cell.
  for (const char *f : {"minff_ortho.gro", "minff_triclinic.gro"}) {
    string tag = f;
    Fragment *frag = load(dir + "/" + f, "GRO");
    if (!frag) continue;
    check(frag->numAtoms() == 13, tag + ": 13 atoms (got "
          + to_string(frag->numAtoms()) + ")");
    const char *want[] = {"Al","Si","Mg","Fe","O","H","O","H","H",
                          "Na","Cl","C","Cu"};
    for (int i = 0; i < frag->numAtoms() && i < 13; i++) {
      check(frag->atomRef(i)->atomicSymbol() == want[i],
            tag + ": atom " + to_string(i+1) + " is " + want[i]
            + " (got " + frag->atomRef(i)->atomicSymbol() + ")");
    }
    if (frag->numAtoms() > 1) {
      const double *c = frag->atomRef(1)->coordinates();
      check(fabs(c[0] - 5.0) < 1e-6 && fabs(c[1] - 2.0) < 1e-6
            && fabs(c[2] - 3.0) < 1e-6, tag + ": atom 2 at 5.0 2.0 3.0 A");
    }
    checkResidues(frag, tag, 6);
    LatticeDef *lat = frag->getLattice();
    check(lat != 0, tag + ": lattice");
    if (lat) {
      vector<MPoint> *v = lat->toVectors();
      double want3[3][3];
      if (tag == "minff_ortho.gro") {
        double w[3][3] = {{20,0,0},{0,20,0},{0,0,30}};
        memcpy(want3, w, sizeof w);
      } else {
        double w[3][3] = {{40,0,0},{10,50,0},{5,2.5,60}};
        memcpy(want3, w, sizeof w);
      }
      bool ok = v->size() == 3;
      for (int k = 0; ok && k < 3; k++) {
        ok = fabs((*v)[k].x() - want3[k][0]) < 1e-6 &&
             fabs((*v)[k].y() - want3[k][1]) < 1e-6 &&
             fabs((*v)[k].z() - want3[k][2]) < 1e-6;
      }
      check(ok, tag + ": cell vectors in Angstrom");
      delete v;
    }
    // dumpGRO then restoreGRO gives the same atoms and cell back.
    if (tag == "minff_triclinic.gro") {
      stringstream ss;
      check(frag->dumpGRO(ss), "dumpGRO writes");
      Fragment back;
      check(back.restoreGRO(ss, false) && back.numAtoms() == 13
            && back.getLattice() != 0, tag + ": dumpGRO output reads back");
      if (back.numAtoms() == 13 && back.getLattice()) {
        vector<MPoint> *v = back.getLattice()->toVectors();
        check(fabs((*v)[2].x() - 5.0) < 1e-3 && fabs((*v)[2].y() - 2.5) < 1e-3,
              tag + ": cell survives dumpGRO");
        delete v;
      }
    }
    delete frag;
  }

  {
    // Elements 110-118 carry IUPAC symbols; the old placeholders written
    // by earlier releases are read as the same elements.
    const char *iupac[] = {"Ds","Rg","Cn","Nh","Fl","Mc","Lv","Ts","Og"};
    const char *old[] = {"Uun","Uuu","Uub","Uut","Uuq","Uup","Uuh","Uus","Uuo"};
    TPerTab pt;
    for (int i = 0; i < 9; i++) {
      string t = string(iupac[i]) + "/" + old[i];
      check(pt.atomicSymbol(110 + i) == iupac[i], t + ": table symbol");
      check(pt.atomicNumber(iupac[i]) == 110 + i, t + ": Z from IUPAC symbol");
      check(pt.atomicNumber(old[i]) == 110 + i, t + ": Z from old symbol");
      check(TPerTab::mixedCase(string("UU") + (char)toupper(old[i][2]))
            == iupac[i], t + ": mixedCase maps old symbol");
    }
    Fragment *frag = load(dir + "/old_symbols.xyz", "XYZ");
    if (frag) {
      check(frag->numAtoms() == 3, "old_symbols.xyz: 3 atoms");
      if (frag->numAtoms() == 3) {
        check(frag->atomRef(0)->atomicNumber() == 110
              && frag->atomRef(1)->atomicNumber() == 118
              && frag->atomRef(2)->atomicNumber() == 117,
              "old_symbols.xyz: Z 110, 118, 117");
        check(string(frag->atomRef(0)->atomicSymbol()) == "Ds"
              && string(frag->atomRef(1)->atomicSymbol()) == "Og",
              "old_symbols.xyz: symbols come out IUPAC");
      }
      delete frag;
    }
  }

  cout << (failures ? "FAILED" : "PASSED") << " (" << failures
       << " failures)" << endl;
  return failures ? 1 : 0;
}
