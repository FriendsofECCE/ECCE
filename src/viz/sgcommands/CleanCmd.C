/**
 * @file
 *
 *
 */
#include <iostream>
  using namespace std;

#include "util/EventDispatcher.H"
#include "util/Event.H"
using namespace ecce;

#include "tdat/ForceField.H"
#include "tdat/FFCoords.H"
#include "tdat/UFFDesc.H"

#include "tdat/TBond.H"
#include "tdat/TAtm.H"
#include "tdat/BondOrderRule.H"

#include <cmath>
#include <cstdio>
#include <sstream>

#include "viz/CleanCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

CleanCmd::CleanCmd(const string& name, Receiver * receiver,
                   ProgressMonitor *mon)
    : FragCloneCmd(name, receiver)
{
   init();
   p_monitor = mon;
   p_redo = 0;
}



/**
 * Destructor.
 * Deletes undo related data.
 */
CleanCmd::~CleanCmd()
{
   p_monitor = 0;
   if (p_redo != 0) delete p_redo;
}



void CleanCmd::init()
{
   addParameter(new CommandParameter("iterations", 100));
   addParameter(new CommandParameter("nonBondCutoff", 5.0));
   addParameter(new CommandParameter("convergence", .0001));
}





//  UFF needs bond orders; the user never sets them. Derive them from the
//  atoms' neighbour counts (BondOrderRule) and store them on the bonds;
//  returns what was assumed, and the atoms the rule could not settle.
static string assignBondOrders(SGFragment *frag, vector<int>& unresolved)
{
   vector<TBond*> *all = frag->bonds();
   vector<string> el(frag->numAtoms());
   for (size_t i = 0; i < frag->numAtoms(); i++)
      el[i] = frag->atomRef(i)->atomicSymbol();
   vector<pair<int,int> > pairs;
   vector<TBond*> used;
   for (TBond *b : *all) {
      if (b->order() < 1.0) continue;          // metal-pi, hydrogen bond
      pairs.push_back(make_pair(b->atom1()->index(), b->atom2()->index()));
      used.push_back(b);
   }
   delete all;
   BondOrderRule::Result r = BondOrderRule::perceive(el, pairs);
   for (size_t i = 0; i < used.size(); i++) used[i]->order(r.orders[i]);
   unresolved = r.unresolved;

   ostringstream os;
   if (!r.notes.empty()) {
      os << "Clean treated as multiple bonds: ";
      for (size_t i = 0; i < r.notes.size(); i++)
         os << (i ? "; " : "") << r.notes[i];
      os << ". ";
   }
   if (!r.unresolved.empty()) {
      os << "Not resolved, treated as single and selected: ";
      for (size_t i = 0; i < r.unresolved.size(); i++)
         os << (i ? ", " : "") << el[r.unresolved[i]] << r.unresolved[i] + 1;
      os << ". ";
   }
   return os.str();
}

//  Bonds far from the typical length for their elements and order.
static string checkBondLengths(SGFragment *frag)
{
   vector<TBond*> *all = frag->bonds();
   ostringstream os;
   int n = 0;
   for (TBond *b : *all) {
      if (b->order() < 1.0) continue;
      TAtm *a1 = b->atom1(), *a2 = b->atom2();
      double typ = BondOrderRule::typicalLength(a1->atomicSymbol(),
                                  a2->atomicSymbol(), b->order());
      if (typ <= 0.0) continue;
      const double *p = a1->coordinates(), *q = a2->coordinates();
      double d = sqrt((p[0]-q[0])*(p[0]-q[0]) + (p[1]-q[1])*(p[1]-q[1]) +
                      (p[2]-q[2])*(p[2]-q[2]));
      if (fabs(d - typ) <= 0.15) continue;
      if (n++ < 8) {
         char buf[160];
         snprintf(buf, sizeof buf, "%s%d-%s%d %.2f A (typical %.2f for order %g)",
                  a1->atomicSymbol().c_str(), a1->index() + 1,
                  a2->atomicSymbol().c_str(), a2->index() + 1, d, typ,
                  b->order());
         os << (n > 1 ? "; " : "") << buf;
      }
   }
   delete all;
   if (n == 0) return "";
   string s = "Warning: bond lengths outside the typical range after Clean: " +
              os.str();
   if (n > 8) s += "; and more";
   return s + ". Check the geometry.";
}


bool CleanCmd::execute()
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   // Since clean is so slow, use cached redo frag if available
   if (p_redo) {
        
     // Copied from superclass undo
     //hack - should this be copied over in fragment?
     DisplayStyle ds = sg->getFragment()->getMainDisplayStyle();
      sg->getCSTop()->removeAllChildren();
      frag = new SGFragment(*p_redo);
      frag->setMainDisplayStyle(ds);
      sg->getCSTop()->addChild(frag);
      sg->getCSTop()->addChild(sg->getDisplayGroup());
   } else {

      if (frag->numAtoms() > 1 ) {
         ret = true;

         int iterations = getParameter("iterations")->getInteger();

         // not currently using - don't remember why
         double convergence = getParameter("convergence")->getDouble();

         double nonBondCutoff = getParameter("nonBondCutoff")->getDouble();

         vector<int> *highlight = &frag->m_atomHighLight;
         cloneFragment(frag);

         //  Before the force field is set up: it reads the bond orders then.
         vector<int> unresolved;
         string report = assignBondOrders(frag, unresolved);

         UFFDesc desc;
         ForceField forceField(frag, highlight, &desc, p_monitor);

         forceField.optimize( iterations,
               convergence,
               nonBondCutoff
               );
         sg->fixAllNubs();

         report += checkBondLengths(frag);
         //  Unresolved atoms are selected after the optimisation (the
         //  selection restricts it, so not before).
         frag->m_atomHighLight = unresolved;
         if (!report.empty()) {
            cerr << "Clean: " << report << endl;
            EventDispatcher::getDispatcher().publish(
               Event("CleanReport", report));
         }


      }
      frag->touchNumbers();
      sg->touchChemDisplay();

      Event event("GeomChange");
      EventDispatcher::getDispatcher().publish(event);
   }

   return ret;
}



void CleanCmd::undo()
{
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   if (p_undoFrag) {
      p_redo = new SGFragment(*frag);
   }
   FragCloneCmd::undo();
}
