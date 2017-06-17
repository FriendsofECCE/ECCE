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

#include "viz/LatticeBuildCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

LatticeBuildCmd::LatticeBuildCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
LatticeBuildCmd::~LatticeBuildCmd()
{

}



void LatticeBuildCmd::init()
{
   addParameter(new CommandParameter("axis1", 1));
   addParameter(new CommandParameter("x1", 1.0));
   addParameter(new CommandParameter("y1", 0.0));
   addParameter(new CommandParameter("z1", 0.0));
   addParameter(new CommandParameter("axis2", 1));
   addParameter(new CommandParameter("x2", 0.0));
   addParameter(new CommandParameter("y2", 1.0));
   addParameter(new CommandParameter("z2", 0.0));
   addParameter(new CommandParameter("axis3", 1));
   addParameter(new CommandParameter("x3", 0.0));
   addParameter(new CommandParameter("y3", 0.0));
   addParameter(new CommandParameter("z3", 1.0));
   addParameter(new CommandParameter("random",false));
   setHelpMessage("Clear the existing structure.");
}



bool LatticeBuildCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();

   SGFragment *baseFrag;
   SGFragment *tmpFrag = (SGFragment*)0;

   if (frag->numAtoms() > 0 ) {
      cloneFragment(frag);
      ret = true;
      double x1, y1, z1;
      double x2, y2, z2;
      double x3, y3, z3;
      double xt, yt, zt;
      int i, j, k;
      int nstep1, nstep2, nstep3;
      int conatom = -1;
      int fragconatom = -1;
      nstep1 = getParameter("axis1")->getInteger();
      nstep2 = getParameter("axis2")->getInteger();
      nstep3 = getParameter("axis3")->getInteger();
      x1 = getParameter("x1")->getDouble();
      y1 = getParameter("y1")->getDouble();
      z1 = getParameter("z1")->getDouble();
      x2 = getParameter("x2")->getDouble();
      y2 = getParameter("y2")->getDouble();
      z2 = getParameter("z2")->getDouble();
      x3 = getParameter("x3")->getDouble();
      y3 = getParameter("y3")->getDouble();
      z3 = getParameter("z3")->getDouble();
      bool randomize = getParameter("random")->getBoolean();
      double pi = 4.0*atan(1.0);
      if (nstep1*nstep2*nstep3>1) {
        baseFrag = new SGFragment(*frag);
        baseFrag->m_atomHighLight.clear();
        for (i=0; i<nstep1; i++) {
          for (j=0; j<nstep2; j++) {
            for (k=0; k<nstep3; k++) {
              tmpFrag = new SGFragment(*baseFrag);
              if (i != 0 || j != 0 || k != 0) {
                xt = ((double)i)*x1 + ((double)j)*x2 + ((double)k)*x3;
                yt = ((double)i)*y1 + ((double)j)*y2 + ((double)k)*y3;
                zt = ((double)i)*z1 + ((double)j)*z2 + ((double)k)*z3;
                if (randomize) {
                  double theta;
                  int irandnum;
                  irandnum = (rand() % 10000) + 1;
                  theta = ((double)irandnum)*0.0002*pi;
                  tmpFrag->rotate(1,theta,true);
                  irandnum = (rand() % 10000) + 1;
                  theta = ((double)irandnum)*0.0002*pi;
                  tmpFrag->rotate(2,theta,true);
                  irandnum = (rand() % 10000) + 1;
                  theta = ((double)irandnum)*0.0002*pi;
                  tmpFrag->rotate(3,theta,true);
                }
                frag->mergeFragment(*tmpFrag, conatom, fragconatom, xt, yt, zt);
              }
            }
          }
        }
        delete baseFrag;
      }
      delete tmpFrag;

      frag->touchNumbers();
      sg->touchChemDisplay();

      Event event("GeomChange");
      EventDispatcher::getDispatcher().publish(event);
   }

   return ret;
}
