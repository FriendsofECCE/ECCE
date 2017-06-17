#include <algorithm>
  using std::find;

//#include "inv/nodes/SoSwitch.H"

#include "util/Diag3.H"
#include "util/InternalException.H"

#include "dsm/IPropCalculation.H"

#include "tdat/PropVector.H"

#include "viz/SGContainer.H"
#include "viz/TensorCmd.H"

TensorCmd::TensorCmd(const string& name, Receiver *receiver,
                         IPropCalculation *calc)
  : AddVectorCmd(name, receiver, calc)
{
   vector<int> atoms;
   addParameter(new CommandParameter("Atoms", atoms));
}


TensorCmd::~TensorCmd()
{
}

/**
 * Override superclass just to handle the case where no atoms are selected.
 * In this case, we clear the scene.  Otherwise, invoke super's execute.
 */
bool TensorCmd::execute() throw(EcceException)
{
   bool ret = true;

   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());
   p_currentSGC = sg;

   // Get viewable atoms list:
   vector<int> *atoms = getParameter("Atoms")->getIntegerList();

   // TODO is this logic ok?  WHat if selection is cleared but the user
   // is trying to adjust the scale???
   if (atoms->size() == 0) {
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

 
bool TensorCmd::getDisplacements(vector<double*>& displacements,
                                          vector<SIGN>& signs)
{
   bool ret = false;

   // Get viewable atoms list:
   vector<int> *atoms = getParameter("Atoms")->getIntegerList();

   // Get the calculation
   IPropCalculation *propcalc = getCalculation();
   INTERNALEXCEPTION(propcalc, "No calculation object!");

   // Get the property 
   PropVector *vec = (PropVector*) propcalc->getProperty(getPropertyName());

   if (vec != (PropVector*)0) {
      ret = true;

      double xx, yx, zx, xy, yy, zy, xz, yz, zz;
      double emax = 0.0;
      double curE = 0.0;
      int numVals = 9;

      // Calculate the number of atoms;
      int curAtom;

      int nAtoms = static_cast<int>(atoms->size());
      for (int i = 0; i < nAtoms; i++) {
         curAtom = (*atoms)[i];

         xx = vec->value(curAtom*numVals);
         yx = vec->value(curAtom*numVals +1);
         zx = vec->value(curAtom*numVals +2);
         xy = vec->value(curAtom*numVals +3);
         yy = vec->value(curAtom*numVals +4);
         zy = vec->value(curAtom*numVals +5);
         xz = vec->value(curAtom*numVals +6);
         yz = vec->value(curAtom*numVals +7);
         zz = vec->value(curAtom*numVals +8);

         xy = (xy + yx) * 0.5;
         xz = (xz + zx) * 0.5;
         yz = (yz + zy) * 0.5;

         curE = addTensor(xx,xy,xz,yy,yz,zz, displacements, signs);
         if (p_newData && curE > emax) {
            emax = curE;
         }
      }

      // The newdata flag just determines whether or not we recompute
      // the scale.
      if (p_newData) {
         if (emax > 0.0) {
            // set scale so that largest vector will have magnitude 1
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
 * Compute eigenvectors and eigenvalues then use these to create
 * 6 vectors (3 double pointed vectors) color coded to the sign
 * of the eigenvalue.
 */
double TensorCmd::addTensor(double xx, double xy, double xz,
                            double yy, double yz, double zz,
                            vector<double*>& displacements,
                            vector<SIGN>& signs) 
{

  double ew[3] = {0,0,0};        // eigenvalues
  double e0[3] = {0,0,0};        // eigenvectors
  double e1[3] = {0,0,0};        // ..
  double e2[3] = {0,0,0};        // ..
  int i;
  double emax = 0.0;
  
  // Compute the eigenvectors & eigenvalues
  Diag3 diag(xx, xy, xz, yy, yz, zz);
  diag.calculate(ew, e0, e1, e2);

  if(p_newData) {
    // Calculate the preferred scale factor (only need to do this 1 time)
    for(i = 0 ; i < 3; i++) {
      if (fabs(ew[i]) > emax) {
        emax = fabs(ew[i]);
      }
    }      
  }
  int start = displacements.size();
  
  // Our Tensor is 6 vectors but picture it as 3 double pointed
  // vectors.  Assign neutral color here but set true color in code below
  for(i = 0 ; i < 6; i++) {
     displacements.push_back(new double[3]);
     signs.push_back(NEUTRAL);
  }
  
  for(i = 0 ; i < 3; i++) {
    displacements[start][i]   = ew[0] * e0[i];
    displacements[start+1][i] = ew[1] * e1[i];
    displacements[start+2][i] = ew[2] * e2[i];

    // now push their inverses
    displacements[start+3][i] = displacements[start][i] * -1;
    displacements[start+4][i] = displacements[start+1][i] * -1;
    displacements[start+5][i] = displacements[start+2][i] * -1;

  }

  // Assign sign from eigenvalue sign
  if (ew[0] != 0) {
     signs[start] = signs[start+3] = (ew[0] > 0. ? POS : NEG);
  }
  if (ew[1] != 0) {
     signs[start+1] = signs[start+4] = (ew[1] > 0. ? POS : NEG);
  }
  if (ew[2] != 0) {
     signs[start+2] = signs[start+5] = (ew[2] > 0. ? POS : NEG);
  }
 
  return emax;
}


/**
 * We have to append to the superclass's method because
 * we are using a log scale (we need to convert these from
 * log value to normal value).
 */
double TensorCmd::getScaleFactor() 
{

   double scale = AddVectorCmd::getScaleFactor();
   return pow(10, scale);
}



/**
 * Overrides parent class in that it only gets the centers
 * for atoms that have been selected.
 */
void TensorCmd::getCenters(vector<double*>& centers) 
{
   // Get atom centers:
   vector<double*> atomCenters;
   getAtomCenters(atomCenters);

   // Get viewable atoms list:
   // TODO change this so it uses the normal selection model??
   vector<int> *atoms = getParameter("Atoms")->getIntegerList();
   int i;

   // Only add centers for the specified atoms:
   int nAtoms = static_cast<int>(atoms->size());
   for (i = 0; i < nAtoms; i++) {
      centers.push_back(atomCenters[(*atoms)[i]]);
      atomCenters[(*atoms)[i]] = NULL;
   }

   // clean up memory for unused centers
   int numCenters = static_cast<int>(atomCenters.size());
   for (i = 0; i < numCenters; i++) {
      if (atomCenters[i] != NULL) {
         delete atomCenters[i];
      }
   }  
}
