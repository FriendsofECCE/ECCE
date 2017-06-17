#include <iostream>
using namespace std;
#include "inv/nodes/SoSwitch.H"

#include "util/Color.H"
#include "util/InternalException.H"

#include "dsm/IPropCalculation.H"

#include "inv/ChemKit/ChemDisplayParam.H"

#include "viz/SGFragment.H"
#include "viz/VRVector.H"
#include "viz/cvSGContainer.H"
#include "viz/AddVectorCmd.H"

AddVectorCmd::AddVectorCmd(const string& name, Receiver *receiver,
                         IPropCalculation *calc) 
  : PropCmd(name,receiver, calc)
{
   init();
}

AddVectorCmd::~AddVectorCmd()
{
}

void AddVectorCmd::init()
{
   addParameter(new CommandParameter("Amplitude", 1.0));
   addParameter(new CommandParameter("Center", "charge"));
   addParameter(new CommandParameter("Sign", true)); // do we need this
   addParameter(new CommandParameter("property", ""));
   addParameter(new CommandParameter("NewData", false)); // can we eliminate?

   p_preferredScaleFactor = 1.0;

   // Note the neutral is the default color hardwired into VRVector
   addParameter(new CommandParameter("neutral", "#cccc00"));
   addParameter(new CommandParameter("positive", "#3333ff"));
   addParameter(new CommandParameter("negative", "#ff3333"));
}

bool AddVectorCmd::execute() throw(EcceException)
{
   bool ret = true;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   p_currentSGC = sg;

   // Get the NewData parameter:
   p_newData = getParameter("NewData")->getBoolean();

   // Get the root node and clear it out
   SoSwitch* root = getRootNode();
   root->removeAllChildren();

   // Get the displacements:
   vector<double*> displacements;
   vector<SIGN> signs;

   if (getDisplacements(displacements, signs) && 
         displacements.size() > 0 && signs.size() > 0) { // we found the property
      ret = true;

      // Get the center(s):
      vector <double*> centers;
      getCenters(centers);
      // One center per vector
      bool oneCenter = centers.size() == 1;

      // One center per each group of numVecs vectors
      int numVecs = displacements.size() / centers.size();
      int centerIdx = 0;

      // Get the scale factor (controlled by slider bar):
      double scale = getScaleFactor();

      // Now draw the vector(s) on the scene graph:
      for (int i = 0; i < displacements.size(); i++) {
         if ( ((centerIdx + 1) * numVecs) == i) {
            // Free the memory of the previous center
            if(!oneCenter) {
               delete[] centers[centerIdx];
            }
            centerIdx++;
         }

         VRVector* vec = new VRVector;

         // Handle color coding for sign
         string neutral = getParameter("neutral")->getString();
         string positive = getParameter("positive")->getString();
         string negative = getParameter("negative")->getString();
         Color *color = 0;
         if (signs[i] == NEUTRAL) {
            color = new Color(neutral);
         } else if (signs[i] == POS) {
            color = new Color(positive);
         } else if (signs[i] == NEG) {
            color = new Color(negative);
         }
         vec->setColor(color->red(), color->green(), color->blue());
         delete color;

         if(oneCenter) {
            vec->position(centers[0][0], centers[0][1], centers[0][2]);

         } else {
            vec->position(centers[centerIdx][0], centers[centerIdx][1], 
                  centers[centerIdx][2]);
         }

         vec->direction(displacements[i][0] * scale,
               displacements[i][1] * scale,
               displacements[i][2] * scale);

         root->addChild(vec);

         // free the memory
         delete[] displacements[i];
      }
      if (oneCenter) {
         delete[] centers[0];
      } else {
         delete[] centers[centerIdx++];
      }
   }
   return ret;
}


double AddVectorCmd::getScaleFactor() 
{
   double scale;

   if(p_newData) {
      // we are recomputing, so use our best guess to set the slider bar:
      scale = p_preferredScaleFactor;
      getParameter("Amplitude")->setDouble(scale);

   } else {
      scale = getParameter("Amplitude")->getDouble();
   }

   return scale;
}


void AddVectorCmd::getCenters(vector<double*>& centers) 
{
   string centerChoice = getParameter("Center")->getString();

   if (centerChoice == "atom") {
      getAtomCenters(centers);

   } else if (centerChoice == "charge" || centerChoice == "mass") {
      getSingleCenter(centers);

   } else {
      INTERNALEXCEPTION(0, "Trying to draw vector with  unknown center type: "
            + centerChoice);
   }

}

/**
 * This version gets one center for each atom in the chemical system.
 */
void AddVectorCmd::getAtomCenters(vector<double*>& centers) 
{

   SGFragment *sgfrag = p_currentSGC->getFragment();
   vector<TAtm*>* atoms = sgfrag->atoms();

   // loop over atoms
   for (int j = 0; j < atoms->size(); j++) {
      centers.push_back(new double[3]);  
      centers[j][0] =  (*atoms)[j]->coordinates()[0];
      centers[j][1] =  (*atoms)[j]->coordinates()[1];
      centers[j][2] =  (*atoms)[j]->coordinates()[2];
   }
   delete atoms;
}

/**
 * This version gets one center, which is either the center of
 * mass or the center of charge, depending upon the "Center"  property.
 */
void AddVectorCmd::getSingleCenter(vector<double*>& centers) 
{

   string center = getParameter("Center")->getString();

   SGFragment *frag = p_currentSGC->getFragment();
   int numAtoms = frag->numAtoms();

   MPoint xyz(0.,0.,0.);
   float totalCharge=0.0;

   if (center == "charge") {
      TAtm *atom;
      float charge;
      for (int i=0; i<numAtoms; i++) {
         atom = frag->atomRef(i);
         charge = atom->atomicNumber();
         totalCharge += charge;
         MPoint tmp;
         const double *coords = atom->coordinates();
         tmp.xyz(coords[0], coords[1], coords[2]);
         tmp.scale(charge);
         xyz.add(tmp);
      }

      // 1/Sum of charges
      xyz.scale(1.0/totalCharge);
   } else {
      // assume mass
      TAtm *atom;
      for (int i=0; i<numAtoms; i++) {
         atom = frag->atomRef(i);
         const double *coords = atom->coordinates();
         MPoint tmp(coords[0], coords[1], coords[2]);
         xyz.add(tmp);
      }
      xyz.scale(1.0/numAtoms);
   }
   centers.push_back(new double[3]);
   centers[0][0] = xyz.x();
   centers[0][1] = xyz.y();
   centers[0][2] = xyz.z();
}


/**
 * Default implementation returns the parameter named "property"
 */
string AddVectorCmd::getPropertyName()
{
   return getParameter("property")->getString();
}


/**
 * Default implementation returns the node with the same name
 * as the property (getPropertyName).
 * If not found, a new one is created.
 */
SoSwitch *AddVectorCmd::getRootNode()
{
   return  p_currentSGC->getPropertySwitch(getPropertyName());
}
