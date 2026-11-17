#include <cmath>
#include <iostream>
  using namespace std;

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "tdat/ClusterCut.H"
#include "tdat/LatticeDef.H"
#include "tdat/TAtm.H"

#include "viz/ClusterCutCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

ClusterCutCmd::ClusterCutCmd(const string& name, Receiver * receiver,
                             SGFragment *base)
    : PBCCmd(name, receiver, base), p_atoms(0), p_caps(0)
{
   init();
}

ClusterCutCmd::~ClusterCutCmd()
{
}

void ClusterCutCmd::init()
{
   addParameter(new CommandParameter("radius", 5.0));
   addParameter(new CommandParameter("whole", 0));
   addParameter(new CommandParameter("cap", 0));
}

bool ClusterCutCmd::execute()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   LatticeDef *lattice = frag ? frag->getLattice() : 0;
   if (!lattice) { setErrorMessage("the structure has no lattice"); return false; }
   if (frag->m_atomHighLight.empty()) {
      setErrorMessage("select the atom or atoms to centre the cluster on");
      return false;
   }

   array<array<double,3>,3> cell;
   vector<MPoint> *vec = lattice->toVectors();
   for (int k = 0; k < 3; ++k) {
      cell[k][0] = (*vec)[k].x();
      cell[k][1] = (*vec)[k].y();
      cell[k][2] = (*vec)[k].z();
   }
   delete vec;

   vector<string> symbols;
   vector<array<double,3> > xyz;
   vector<int> compact(frag->numAtoms(), -1);
   for (size_t i = 0; i < frag->numAtoms(); ++i) {
      TAtm *a = frag->atomRef(i);
      if (a->atomicSymbol() == "Nub") continue;
      compact[i] = static_cast<int>(symbols.size());
      symbols.push_back(a->atomicSymbol());
      const double *p = a->coordinates();
      xyz.push_back(array<double,3>{{p[0], p[1], p[2]}});
   }

   // Centre: mean of the selected atoms, each taken as the image nearest the
   // first so a selection that straddles the cell boundary stays together.
   array<double,3> centre = {{0, 0, 0}};
   array<double,3> first = {{0, 0, 0}};
   int count = 0;
   for (int idx : frag->m_atomHighLight) {
      if (idx < 0 || idx >= (int)compact.size() || compact[idx] < 0) continue;
      array<double,3> p = xyz[compact[idx]];
      if (count == 0) first = p;
      else {
         // Subtract whole cells while that shortens the offset from first.
         for (int pass = 0; pass < 3; ++pass)
            for (int k = 0; k < 3; ++k)
               for (int sign = -1; sign <= 1; sign += 2) {
                  array<double,3> q = p;
                  for (int c = 0; c < 3; ++c) q[c] += sign * cell[k][c];
                  double d0 = 0, d1 = 0;
                  for (int c = 0; c < 3; ++c) {
                     d0 += (p[c]-first[c])*(p[c]-first[c]);
                     d1 += (q[c]-first[c])*(q[c]-first[c]);
                  }
                  if (d1 < d0 - 1.e-9) p = q;
               }
      }
      for (int c = 0; c < 3; ++c) centre[c] += p[c];
      ++count;
   }
   if (count == 0) { setErrorMessage("the selection holds no atoms"); return false; }
   for (int c = 0; c < 3; ++c) centre[c] /= count;

   ClusterOptions opt;
   opt.radius = getParameter("radius")->getDouble();
   opt.whole = static_cast<ClusterOptions::Whole>(getParameter("whole")->getInteger());
   opt.cap = static_cast<ClusterOptions::Cap>(getParameter("cap")->getInteger());
   vector<ClusterAtom> cluster;
   try {
      cluster = cutCluster(symbols, xyz, cell, centre, opt);
   } catch (const std::exception& e) {
      setErrorMessage(e.what());
      return false;
   }
   if (cluster.empty()) { setErrorMessage("no atoms within the radius"); return false; }

   cloneFragment(frag);   // for undo

   for (int i = static_cast<int>(frag->numAtoms()) - 1; i >= 0; --i)
      frag->deleteAtom(i, false);
   frag->setLattice(0);
   p_caps = 0;
   for (const ClusterAtom& a : cluster) {
      frag->addAtom(a.symbol, a.xyz);
      if (a.cap) ++p_caps;
   }
   p_atoms = static_cast<int>(cluster.size());
   frag->m_atomHighLight.clear();
   frag->m_bondHighLight.clear();
   frag->addCovalentBonds();
   frag->touchNumbers();
   sg->touchChemDisplay();
   sg->touchLattice();
   EventDispatcher::getDispatcher().publish(Event("GeomChange"));
   EventDispatcher::getDispatcher().publish(Event("LatticeChange"));
   return true;
}

bool ClusterCutCmd::shouldAutoReplicate() const
{
   return true;
}

bool ClusterCutCmd::shouldAutoReplicateOnUndo() const
{
   return true;
}
