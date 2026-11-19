#include <algorithm>
#include <cstdio>
#include <iostream>
  using namespace std;

#include "util/Ecce.H"
#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "tdat/FragUtil.H"
#include "tdat/LatticeDef.H"
#include "tdat/Residue.H"
#include "tdat/Solvate.H"
#include "tdat/TAtm.H"

#include "viz/SolvateCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

SolvateCmd::SolvateCmd(const string& name, Receiver * receiver,
                       SGFragment *base)
    : PBCCmd(name, receiver, base), p_molecules(0), p_atoms(0), p_density(0.0)
{
   init();
}

SolvateCmd::~SolvateCmd()
{
}

void SolvateCmd::init()
{
   addParameter(new CommandParameter("solvent", string("water216")));
   addParameter(new CommandParameter("margin", 10.0));
   addParameter(new CommandParameter("scale", 1.0));
}

bool SolvateCmd::execute()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (!frag) { setErrorMessage("there is no structure"); return false; }

   solvate::SolventBox box;
   string err;
   const string solvent = getParameter("solvent")->getString();
   if (solvent.empty() || solvent.find('/') != string::npos ||
       !solvate::readSolventBox(string(Ecce::ecceDataPath()) +
                                "/client/solvents/" + solvent + ".xyz", box, &err)) {
      setErrorMessage(err.empty() ? "unknown solvent " + solvent : err);
      return false;
   }

   vector<string> symbols;
   vector<solvate::Vec> xyz;
   for (size_t i = 0; i < frag->numAtoms(); ++i) {
      TAtm *a = frag->atomRef(i);
      if (a->atomicSymbol() == "Nub") continue;
      symbols.push_back(a->atomicSymbol());
      const double *p = a->coordinates();
      xyz.push_back(solvate::Vec{{p[0], p[1], p[2]}});
   }

   solvate::Cell cell;
   solvate::Vec shift = {{0, 0, 0}};
   LatticeDef *lattice = frag->getLattice();
   if (lattice) {
      vector<MPoint> *vec = lattice->toVectors();
      for (int k = 0; k < 3; ++k) {
         cell[k][0] = (*vec)[k].x();
         cell[k][1] = (*vec)[k].y();
         cell[k][2] = (*vec)[k].z();
      }
      delete vec;
   } else {
      if (xyz.empty()) {
         setErrorMessage("an empty structure needs a periodic cell to fill");
         return false;
      }
      solvate::marginCell(xyz, getParameter("margin")->getDouble(), cell, shift);
      for (solvate::Vec& p : xyz)
         for (int k = 0; k < 3; ++k) p[k] += shift[k];
   }

   solvate::Options opt;
   opt.scale = getParameter("scale")->getDouble();
   opt.margin = getParameter("margin")->getDouble();
   solvate::Result res;
   if (!solvate::fill(symbols, xyz, cell, box, opt, res, &err)) {
      setErrorMessage(err);
      return false;
   }
   if (res.molecules == 0) {
      setErrorMessage("no solvent molecule fits in the box");
      return false;
   }

   cloneFragment(frag);   // for undo

   if (!lattice) {
      for (size_t i = 0; i < frag->numAtoms(); ++i) {
         TAtm *a = frag->atomRef(i);
         const double *p = a->coordinates();
         a->coordinates(p[0] + shift[0], p[1] + shift[1], p[2] + shift[2]);
      }
      LatticeDef *cellDef = new LatticeDef(1., 1., 1., 90., 90., 90.);
      cellDef->assign(MPoint(cell[0][0], 0, 0), MPoint(0, cell[1][1], 0),
                      MPoint(0, 0, cell[2][2]));
      frag->setLattice(cellDef);
   }

   // Every atom needs a residue, or the PDB written for GROMACS Prepare
   // drops it: give the solute ECCE's own residues before adding solvent.
   if (frag->numAtoms() > 0 && frag->numResidues() == 0) {
      FragUtil util(frag);
      util.createResidues();
   }
   int resNum = 0;
   for (size_t i = 0; i < frag->numResidues(); ++i)
      resNum = std::max(resNum, frag->residue(i)->number());
   const bool water = solvate::isWater(box);
   static const char *waterNames[3] = {" OW ", " HW1", " HW2"};
   const bool firstSolvent = frag->getFirstSolventAtomIndex() < 0;
   const int start = static_cast<int>(frag->numAtoms());
   for (int m = 0; m < res.molecules; ++m) {
      if (++resNum >= 10000) resNum = 1;
      Residue *r = new Residue(water ? "HOH " : "SOL ", resNum);
      frag->addResidue(r);
      for (int a = 0; a < box.molSize; ++a) {
         const int idx = m * box.molSize + a;
         frag->addAtom(res.symbols[idx], res.xyz[idx].data());
         TAtm *atm = frag->atomRef(frag->numAtoms() - 1);
         atm->atomType(res.types[idx].c_str());
         if (water) atm->atomName(waterNames[a]);
         atm->atomResidue(r);
         if (a == 0) r->firstAtomPtr(atm);
      }
   }
   if (firstSolvent) frag->setFirstSolventAtomIndex(start);
   frag->setSolventNumAtoms(box.molSize);
   p_molecules = res.molecules;
   p_atoms = static_cast<int>(res.xyz.size());
   p_density = solvate::density(res.symbols, cell);

   frag->addCovalentBonds();
   frag->touchNumbers();
   sg->touchChemDisplay();
   sg->touchLattice();
   EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
   return true;
}

bool SolvateCmd::shouldAutoReplicate() const
{
   return true;
}

bool SolvateCmd::shouldAutoReplicateOnUndo() const
{
   return true;
}
