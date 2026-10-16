#include <cmath>
#include <iostream>
#include <string>
#include <vector>
  using namespace std;

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "tdat/LatticeDef.H"
#include "tdat/LayerStack.H"
#include "viz/PBCLayerCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

PBCLayerCmd::PBCLayerCmd(const string& name, Receiver * receiver, SGFragment *base)
    : PBCCmd(name, receiver, base)
{
   init();
}

PBCLayerCmd::~PBCLayerCmd()
{
}

void PBCLayerCmd::init()
{
   addParameter(new CommandParameter("layers", 2));
   addParameter(new CommandParameter("spacing", 0.0));
   addParameter(new CommandParameter("shiftA", 0.0));
   addParameter(new CommandParameter("shiftB", 0.0));
   for (int i = 1; i <= 2; i++) {
      string p = i == 1 ? "ion1" : "ion2";
      addParameter(new CommandParameter(p + "sym", ""));
      addParameter(new CommandParameter(p + "n", 0));
      addParameter(new CommandParameter(p + "q", 0.0));
      addParameter(new CommandParameter(p + "min", 3.5));
   }
   addParameter(new CommandParameter("water", 0));
   addParameter(new CommandParameter("waterMin", 2.0));
   addParameter(new CommandParameter("seed", 1));
}

bool PBCLayerCmd::execute()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   LatticeDef *lattice = frag ? frag->getLattice() : 0;
   if (!lattice || frag->numAtoms() == 0) {
      setErrorMessage("The layer builder needs a structure with a cell: "
                      "create a lattice and generate or draw the layer first.");
      return false;
   }

   using namespace layerstack;
   Cell cell;
   int rep[3];
   lattice->getReplicationFactors(rep[0], rep[1], rep[2]);
   vector<MPoint> *vecs = lattice->toVectors();
   for (int i = 0; i < 3; i++) {
      cell[i][0] = rep[i] * (*vecs)[i].x();
      cell[i][1] = rep[i] * (*vecs)[i].y();
      cell[i][2] = rep[i] * (*vecs)[i].z();
   }
   delete vecs;

   vector<Atom> layer;
   const int m = frag->numAtoms();
   for (int i = 0; i < m; i++) {
      Atom a;
      a.symbol = frag->atomRef(i)->atomicSymbol();
      for (int k = 0; k < 3; k++) a.xyz[k] = frag->atomRef(i)->coordinates()[k];
      layer.push_back(a);
   }

   Options opt;
   opt.nLayers = getParameter("layers")->getInteger();
   opt.spacing = getParameter("spacing")->getDouble();
   opt.shiftA = getParameter("shiftA")->getDouble();
   opt.shiftB = getParameter("shiftB")->getDouble();
   for (int i = 1; i <= 2; i++) {
      string p = i == 1 ? "ion1" : "ion2";
      Ion ion;
      ion.symbol = getParameter(p + "sym")->getString();
      ion.count = getParameter(p + "n")->getInteger();
      ion.charge = getParameter(p + "q")->getDouble();
      ion.minDist = getParameter(p + "min")->getDouble();
      if (ion.count > 0 && !ion.symbol.empty()) opt.ions.push_back(ion);
   }
   opt.waterPerInterlayer = getParameter("water")->getInteger();
   opt.waterMinDist = getParameter("waterMin")->getDouble();
   opt.seed = (unsigned)getParameter("seed")->getInteger();

   string err;
   Cell nc;
   vector<Atom> stack, added;
   if (!stackCell(cell, opt, nc, &err) ||
       !stackLayers(layer, cell, opt, stack, &err) ||
       !fillInterlayers(stack, nc, opt, added, &err)) {
      setErrorMessage(err);
      return false;
   }

   // Undo state only now, so a refused request leaves nothing to undo.
   cloneFragment(frag);

   SGFragment *base = new SGFragment(*frag);
   base->m_atomHighLight.clear();
   for (int i = 0; i < m; i++)
      frag->atomRef(i)->coordinates(stack[i].xyz[0], stack[i].xyz[1],
                                    stack[i].xyz[2]);
   for (int L = 1; L < opt.nLayers; L++) {
      SGFragment *copy = new SGFragment(*base);
      for (int i = 0; i < m; i++) {
         const double *x = stack[L*m + i].xyz;
         copy->atomRef(i)->coordinates(x[0], x[1], x[2]);
      }
      frag->absorb(*copy, true);
      delete copy;
   }
   delete base;

   for (size_t i = 0; i < added.size(); i++) {
      frag->addAtom(added[i].symbol, added[i].xyz);
      if (added[i].kind == ION)
         frag->atomRef(frag->numAtoms() - 1)->partialCharge(added[i].charge);
   }

   MPoint a, b, c;
   a.xyz(nc[0]);
   b.xyz(nc[1]);
   c.xyz(nc[2]);
   lattice->assign(a, b, c);
   lattice->setReplicationFactors(1, 1, 1);

   // Wrap as Fold does: a bonded group moves as a unit, by whole cell
   // vectors; per atom only if that is not possible.
   if (!frag->makeMoleculesWhole(true)) {
      // A water (O, H, H as added) moves by its O's cell vector so that it
      // stays one molecule; layer atoms and ions fold one by one.
      frag->convertToFractionalCoords(false);
      const int first = (int)frag->numAtoms() - (int)added.size();
      for (int i = 0; i < (int)frag->numAtoms(); i++) {
         int nmove = 1;
         if (i >= first && added[i - first].kind == WATER_O) nmove = 3;
         double s[3];
         const double *f = frag->atomRef(i)->coordinates();
         for (int k = 0; k < 3; k++) s[k] = floor(f[k]);
         for (int j = i; j < i + nmove; j++) {
            const double *x = frag->atomRef(j)->coordinates();
            frag->atomRef(j)->coordinates(x[0] - s[0], x[1] - s[1],
                                          x[2] - s[2]);
         }
         i += nmove - 1;
      }
      frag->convertFromFractionalCoords();
   }
   frag->deleteAllBonds();
   frag->addCovalentBonds();
   frag->touchNumbers();

   sg->touchChemDisplay();
   sg->touchLattice();
   EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   return true;
}

bool PBCLayerCmd::shouldAutoReplicate() const
{
   return false;
}

bool PBCLayerCmd::shouldAutoReplicateOnUndo() const
{
   return false;
}
