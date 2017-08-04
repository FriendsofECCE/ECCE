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

#include "tdat/LatticeDef.H"
#include "viz/SGLattice.H"
#include "viz/SGPlane.H"
#include "viz/MillerPlane.H"
#include "viz/StyledPlane.H"

#include "viz/NewPlaneCmd.H"
#include "viz/SGFragment.H"
#include "viz/SGContainer.H"

NewPlaneCmd::NewPlaneCmd(const string& name, Receiver * receiver)
    : FragCloneCmd(name, receiver)
{
   init();
}



/**
 * Destructor.
 * Deletes undo related data.
 */
NewPlaneCmd::~NewPlaneCmd()
{

}



void NewPlaneCmd::init()
{

   // A plane can be requested for the hkj values, abc vectors or from selection
   addParameter(new CommandParameter("type", "hkl")); 
   addParameter(new CommandParameter("h", 1));
   addParameter(new CommandParameter("k", 1));
   addParameter(new CommandParameter("l", 1));
   addParameter(new CommandParameter("displacement", 0.0));
   addParameter(new CommandParameter("transparent", true));
   addParameter(new CommandParameter("name", "name"));

   // Yellow is the default color
   addParameter(new CommandParameter("r", 255));
   addParameter(new CommandParameter("g", 255));
   addParameter(new CommandParameter("b", 0));
}



bool NewPlaneCmd::execute() throw(EcceException)
{
   bool ret = false;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   SGFragment *frag = sg->getFragment();
   LatticeDef *lattice = frag->getLattice();

   string name = getParameter("name")->getString();
   if (frag->numAtoms() > 0 && lattice) {
      //cloneFragment(frag);

      int r = getParameter("r")->getInteger();
      int g = getParameter("g")->getInteger();
      int b = getParameter("b")->getInteger();
      bool transparent = getParameter("transparent")->getBoolean();

      string type = getParameter("type")->getString();
      if (true) { //(type == "hkl") {
         int h = getParameter("h")->getInteger();
         int k = getParameter("k")->getInteger();
         int l = getParameter("l")->getInteger();
         double displacement = getParameter("displacement")->getDouble();
         
         // Find vector pointing in direction indicated by Miller indices
         // h,k,l and the reciprocal lattice vectors
         MPoint normal = lattice->getPlaneNormal(h,k,l);
         normal.normalize();

         // Displacement along normal vector
         MPoint displ(normal.x(), normal.y(), normal.z());
         double range = lattice->displacementRange(normal);
         //cout<<"displacement range: "<<range<<endl;
         displ.scale(displacement*range);

         // Get intersection of Miller plane with bounding box
         vector<MPoint> corners;
         int ncorner;

         //  cout<<"  displacement x: "<<displ.x()<<" y: "
         //      <<displ.y()<<" z: "<<displ.z()<<endl;
         lattice->planeCorners(displ, normal, ncorner, corners);
         /*
         cout <<"number of corners: "<<ncorner<<endl;
         for (int it=0; it<ncorner; it++) {
           cout<<"  corner["<<it<<"] x: "<<corners[it].x()<<" y: "
               <<corners[it].y()<<" z: "<<corners[it].z()<<endl;
         }
         */

         // Set plane
         MPoint o(0,0,0);
         o = lattice->getLatticeCorner();
         if (ncorner > 2) {
            StyledPlane *splane = (StyledPlane*)sg->getPlane(name);
            if (splane == 0) {
               //splane = sg->addPlane(new MillerPlane(),name);
               splane = sg->addPlane(new SGPlane(),name);
            }
            SGPlane *plane = splane->getPlane();
            //MillerPlane *mplane = dynamic_cast<MillerPlane*>(plane);
            //mplane->setIndices(h,k,l);
            ret = true;
            plane->setLabel(name);
            plane->setCorners(corners);
            splane->setColor(r/255., g/255., b/255.);
            splane->setTransparent(transparent);
            plane->touch();

         } else {
            cerr << "no plane added due to lack of corners" << endl;
         }

      }

   }

   return ret;
}
