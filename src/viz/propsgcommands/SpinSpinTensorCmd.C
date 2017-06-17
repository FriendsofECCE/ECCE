#include <iostream>
  using std::cout;
  using std::endl;

#include <algorithm>
  using std::find;

//#include "inv/nodes/SoSwitch.H"

#include "util/InternalException.H"

#include "dsm/IPropCalculation.H"

#include "tdat/PropTable.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/SpinSpinTensorCmd.H"

SpinSpinTensorCmd::SpinSpinTensorCmd(const string& name, Receiver *receiver,
                         IPropCalculation *calc)
  : TensorCmd(name, receiver, calc)
{
   // Add parameter that holds the selected rows.  Each row contains atom 
   // pairs that are used to compute the vectors.
   // The "Atoms" parameter of the superclass is not used.
   vector<int> rows;
   addParameter(new CommandParameter("rows", rows));
}


SpinSpinTensorCmd::~SpinSpinTensorCmd()
{
}

/**
 * Override superclass just to handle the case where no atoms are selected.
 * In this case, we clear the scene.  Otherwise, invoke super's execute.
 */
bool SpinSpinTensorCmd::execute() throw(EcceException)
{
   bool ret = true;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   p_currentSGC = sg;

   // Get viewable atoms list:
   vector<int> *rows = getParameter("rows")->getIntegerList();

   // TODO is this logic ok?  WHat if selection is cleared but the user
   // is trying to adjust the scale???
   if (rows->size() == 0) {
      // Get the root node and clear it out
      SoSwitch* root = getRootNode();
      root->removeAllChildren();
   } else {
      ret = AddVectorCmd::execute();
   }

   // Always reset to false and require caller to set to true in the
   // case there is new data
   getParameter("NewData")->setBoolean(false);

   return ret;
}

 
bool SpinSpinTensorCmd::getDisplacements(vector<double*>& displacements,
                                          vector<SIGN>& signs)
{
   bool ret = false;

   // Get the calculation
   IPropCalculation *propcalc = getCalculation();
   INTERNALEXCEPTION(propcalc, "No calculation object!");

   // Get the spin-spin property 
   // Each row contains the 9 values of the three by three matrix
   PropTable *ss = (PropTable*) propcalc->getProperty(getPropertyName());

   if (ss != (PropTable*)0) {
      ret = true;

      double xx, yx, zx, xy, yy, zy, xz, yz, zz;
      double emax = 0.0;
      double curE;

      vector<int> *rows = getParameter("rows")->getIntegerList();

      int numTensors = rows->size();

      int thisRow;
      for (int idx = 0; idx < numTensors; idx++) {
         thisRow = (*rows)[idx];

         //cout << "d using " << thisRow << endl;

         xx = ss->value(thisRow , 0);
         yx = ss->value(thisRow , 1);
         zx = ss->value(thisRow , 2);
         xy = ss->value(thisRow , 3);
         yy = ss->value(thisRow , 4);
         zy = ss->value(thisRow , 5);
         xz = ss->value(thisRow , 6);
         yz = ss->value(thisRow , 7);
         zz = ss->value(thisRow , 8);

         // Symmetrize tensor
         xy = (xy + yx) * 0.5;
         xz = (xz + zx) * 0.5;
         yz = (yz + zy) * 0.5;

         curE = addTensor(xx,xy,xz,yy,yz,zz, displacements,signs);
         if (p_newData && curE > emax) {
            emax = curE;
         }
      }
      if (p_newData) {
         if (emax > 0.0) {
            p_preferredScaleFactor = 1.0/emax;

         } else {
            p_preferredScaleFactor = 1.0;
         }
         p_preferredScaleFactor = log10(p_preferredScaleFactor);
      }
   }
   return ret;
}




/**
 * Overrides parent class in that it only gets the centers
 * for atoms that have been selected.
 */
void SpinSpinTensorCmd::getCenters(vector<double*>& centers) 
{
   vector<int> *rows = getParameter("rows")->getIntegerList();

   // Get the calculation
   IPropCalculation *propcalc = getCalculation();
   INTERNALEXCEPTION(propcalc, "No calculation object!");

   // Get the spin-spin property 
   // Each row contains the 9 values of the three by three matrix
   PropTable *ss = (PropTable*) propcalc->getProperty(getPropertyName());

   const vector<string> *rowLabels = ss->rowLabels();
   int numTensors = rows->size();

   SGFragment *sgfrag = p_currentSGC->getFragment();

   int a1, a2;
   if (ss != (PropTable*)0) {
      int thisRow;
      for (int idx=0; idx<numTensors; idx++) {
         thisRow = (*rows)[idx];

         //cout << "center using " << thisRow << endl;

         sscanf((*rowLabels)[thisRow].c_str(), "%d-%d",&a1, &a2);

         // Count atoms from 0, not 1 as in the property
         a1-=1;
         a2-=1;

         TAtm *atm1 = sgfrag->atomRef(a1);
         TAtm *atm2 = sgfrag->atomRef(a2);
         //double distance = atm1->distanceFrom(atm2);


         const double *coords1 = atm1->coordinates();
         const double *coords2 = atm2->coordinates();

         double *values = new double[3];

         values[0] =  (coords1[0]+coords2[0])/2.0;
         values[1] =  (coords1[1]+coords2[1])/2.0;
         values[2] =  (coords1[2]+coords2[2])/2.0;

         centers.push_back(values);

      }

   }
}
