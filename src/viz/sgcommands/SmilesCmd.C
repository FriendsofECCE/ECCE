#include <cmath>
#include <iostream>
#include <set>
#include <vector>
  using namespace std;

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "tdat/FragGraft.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"
#include "tdat/TPerTab.H"
#include "viz/CleanCmd.H"
#include "viz/SmilesCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

using namespace FragGraft;

SmilesCmd::SmilesCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}

SmilesCmd::~SmilesCmd()
{
}

void SmilesCmd::init()
{
   addParameter(new CommandParameter("mode", "add"));
   addParameter(new CommandParameter("smiles", ""));
   addParameter(new CommandParameter("extended", false));
   addParameter(new CommandParameter("first", -1));
   addParameter(new CommandParameter("attach", -1));
}

string SmilesCmd::getUndoLabel()
{
   return getParameter("mode")->getString() == "replace" ? "Replace Group"
                                                         : "Add from SMILES";
}

bool SmilesCmd::execute()
{
   return getParameter("mode")->getString() == "replace" ? replaceGroup()
                                                         : addMolecule();
}


// The formal charges are the atoms' charges (the way an ion is stored in
// the Builder), so the structure's charge is their sum.
static void setFragmentCharge(SGFragment *frag)
{
   frag->getTotalCharge();
}

static void touchAll(SGContainer *sg, SGFragment *frag, bool residues = false)
{
   frag->touchNumbers();
   sg->touchChemDisplay();
   EventDispatcher::getDispatcher().publish(Event("GeomChange"));
}


bool SmilesCmd::addMolecule()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (frag->numResidues() > 0) {
      setErrorMessage("A structure with residues cannot take a molecule from SMILES.");
      return false;
   }
   Mol m;
   string err;
   if (!smilesTo3D(getParameter("smiles")->getString(), m, err)) {
      setErrorMessage(err);
      return false;
   }
   if (m.dummy >= 0) {
      setErrorMessage("[*] marks the attachment point of a replacement group; "
                      "use Replace Group for it.");
      return false;
   }
   if (getParameter("extended")->getBoolean()) extendTorsions(m);

   vector<Vec> existing;
   for (size_t i = 0; i < frag->numAtoms(); i++) {
      if (frag->atomRef(i)->isNub()) continue;
      const double *c = frag->atomRef(i)->coordinates();
      existing.push_back({c[0], c[1], c[2]});
   }
   const Vec shift = placeBeside(existing, m.xyz);

   cloneFragment(frag);
   const int n0 = (int)frag->numAtoms();
   for (size_t i = 0; i < m.el.size(); i++) {
      double c[3] = {m.xyz[i][0] + shift[0], m.xyz[i][1] + shift[1],
                     m.xyz[i][2] + shift[2]};
      frag->addAtom(m.el[i], c);
      frag->atomRef(n0 + i)->partialCharge(m.charge[i]);
   }
   for (const Bond& b : m.bonds)
      new TBond(frag->atomRef(n0 + b.a), frag->atomRef(n0 + b.b), b.order);
   setFragmentCharge(frag);

   // The existing atoms stay where they are: only the new ones are cleaned.
   frag->m_bondHighLight.clear();
   frag->m_atomHighLight.clear();
   if (n0 > 0)
      for (size_t i = 0; i < m.el.size(); i++) frag->m_atomHighLight.push_back(n0 + i);
   CleanCmd clean("Force Field Clean", sg, 0);
   clean.execute();
   frag->m_atomHighLight.clear();
   setFragmentCharge(frag);

   touchAll(sg, frag);
   EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));
   return true;
}


bool SmilesCmd::replaceGroup()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   const int n = (int)frag->numAtoms();
   if (frag->numResidues() > 0) {
      setErrorMessage("A structure with residues cannot take a replacement group.");
      return false;
   }
   int G = getParameter("first")->getInteger();
   int A = getParameter("attach")->getInteger();
   if (G < 0 || G >= n || frag->atomRef(G)->isNub()) {
      setErrorMessage("Select the first atom of the group, or the bond that joins it.");
      return false;
   }
   // Connectivity without the nubs: they are open bond sites, not neighbours.
   vector<vector<int> > adj(n);
   vector<TBond*> *all = frag->bonds();
   for (TBond *b : *all) {
      int i = b->atom1()->index(), j = b->atom2()->index();
      if (b->atom1()->isNub() || b->atom2()->isNub()) continue;
      adj[i].push_back(j);
      adj[j].push_back(i);
   }
   delete all;
   if (A < 0 && !chooseAttachment(adj, G, A)) {
      setErrorMessage("Cannot tell which bond joins the group; select that bond.");
      return false;
   }
   vector<int> group;
   if (A >= 0 && A < n) {
      // A selected bond: the group is its terminal side, the smaller one.
      vector<int> g2;
      bool ok1 = groupBehind(adj, A, G, group), ok2 = groupBehind(adj, G, A, g2);
      if (ok2 && (!ok1 || g2.size() < group.size())) { swap(A, G); group = g2; ok1 = true; }
      if (!ok1) group.clear();
   }
   if (A < 0 || A >= n || group.empty()) {
      setErrorMessage("The bond is in a ring: there is no terminal group to replace.");
      return false;
   }
   Mol m;
   string err;
   if (!smilesTo3D(getParameter("smiles")->getString(), m, err)) {
      setErrorMessage(err);
      return false;
   }
   int T = -1;
   int nd = 0;
   for (const Bond& b : m.bonds) {
      if (b.a == m.dummy) { T = b.b; nd++; }
      if (b.b == m.dummy) { T = b.a; nd++; }
   }
   if (m.dummy < 0 || nd != 1) {
      setErrorMessage("Write the fragment with one attachment point [*] "
                      "bonded to one atom, such as [*]C(=O)O.");
      return false;
   }

   TPerTab tpt;
   const string sa = frag->atomRef(A)->atomicSymbol();
   const double L = tpt.covalentRadius(tpt.atomicNumber(sa)) +
                    tpt.covalentRadius(tpt.atomicNumber(m.el[T]));
   const double *pa = frag->atomRef(A)->coordinates();
   const double *pg = frag->atomRef(G)->coordinates();
   Vec u = {pg[0]-pa[0], pg[1]-pa[1], pg[2]-pa[2]};
   vector<bool> inGroup(n, false);
   for (int g : group) inGroup[g] = true;
   vector<Vec> rest;
   for (int i = 0; i < n; i++) {
      if (inGroup[i] || i == A || frag->atomRef(i)->isNub()) continue;
      const double *c = frag->atomRef(i)->coordinates();
      rest.push_back({c[0], c[1], c[2]});
   }
   const Vec anchor = {pa[0], pa[1], pa[2]};
   vector<Vec> xyz = graft(m.xyz, T, m.dummy, anchor, u, L, rest, 10.0);

   // Delete the group the way Delete does (this leaves a nub on A where it
   // was), then drop that nub and bond the new group in its place.
   vector<bool> del(n, false);
   frag->m_atomHighLight.clear();
   frag->m_bondHighLight.clear();
   for (int g : group) { del[g] = true; frag->m_atomHighLight.push_back(g); }
   frag->syncNubSelection(del);
   frag->m_atomHighLight.clear();
   for (int i = 0; i < n; i++)
      if (del[i]) frag->m_atomHighLight.push_back(i);
   sg->cleanAtomDeletes();

   TAtm *anchorAtom = frag->atomRef(A);
   cloneFragment(frag);
   frag->deleteSelectedAtoms(del);
   frag->m_atomHighLight.clear();

   vector<TAtm*> nubs;
   for (TBond *b : anchorAtom->bondList()) {
      TAtm *o = b->atom1() == anchorAtom ? b->atom2() : b->atom1();
      if (o->isNub()) nubs.push_back(o);
   }
   int bestNub = -1;
   double bestDot = -2;
   const double un = sqrt(u[0]*u[0] + u[1]*u[1] + u[2]*u[2]);
   for (size_t k = 0; k < nubs.size(); k++) {
      const double *c = nubs[k]->coordinates();
      double d = ((c[0]-pa[0])*u[0] + (c[1]-pa[1])*u[1] + (c[2]-pa[2])*u[2]) / un;
      double l = sqrt((c[0]-pa[0])*(c[0]-pa[0]) + (c[1]-pa[1])*(c[1]-pa[1]) +
                      (c[2]-pa[2])*(c[2]-pa[2]));
      if (l > 0 && d / l > bestDot) { bestDot = d / l; bestNub = k; }
   }
   if (bestNub >= 0) frag->deleteAtom(nubs[bestNub]->index());

   const int n0 = (int)frag->numAtoms();
   vector<int> map(m.el.size(), -1);
   int cnt = 0;
   for (size_t i = 0; i < m.el.size(); i++) {
      if ((int)i == m.dummy) continue;
      double c[3] = {xyz[i][0], xyz[i][1], xyz[i][2]};
      frag->addAtom(m.el[i], c);
      frag->atomRef(n0 + cnt)->partialCharge(m.charge[i]);
      map[i] = n0 + cnt++;
   }
   for (const Bond& b : m.bonds) {
      if (b.a == m.dummy || b.b == m.dummy) continue;
      new TBond(frag->atomRef(map[b.a]), frag->atomRef(map[b.b]), b.order);
   }
   new TBond(anchorAtom, frag->atomRef(map[T]), TBond::Single);
   setFragmentCharge(frag);

   sg->fixAllNubs();
   touchAll(sg, frag);
   EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));
   return true;
}
