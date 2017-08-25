#include <iostream>
  using std::cout;
  using std::endl;


#include "util/InternalException.H"
#include "util/Diag3.H"

#include "dsm/IPropCalculation.H"

#include "tdat/PropVector.H"

#include "viz/cvSGContainer.H"
#include "viz/AddQuadMomentCmd.H"

AddQuadMomentCmd::AddQuadMomentCmd(const string& name, Receiver *receiver,
                         IPropCalculation *calc)
  : AddVectorCmd(name, receiver, calc)
{
  // Parent class does everything
}

AddQuadMomentCmd::~AddQuadMomentCmd()
{
}

bool AddQuadMomentCmd::getDisplacements(vector<double*>& displacements,
                                          vector<SIGN>& signs)
{
   bool ret = false;
   double xx, xy, xz, yy, yz, zz; // property values
   double ew[3] = {0,0,0};        // eigenvalues
   double e0[3] = {0,0,0};        // eigenvectors
   double e1[3] = {0,0,0};        // ..
   double e2[3] = {0,0,0};        // ..
   int i;

   // Get the calculation
   IPropCalculation *propcalc = getCalculation();
   INTERNALEXCEPTION(propcalc, "No calculation object!");

   // Get the property 
   PropVector *vec = (PropVector*) propcalc->getProperty(getPropertyName());

   if (vec != (PropVector*)0) {
      ret = true;

      xx = vec->value(0);
      xy = vec->value(1);
      xz = vec->value(2);
      yy = vec->value(3);
      yz = vec->value(4);
      zz = vec->value(5);

      // Compute the eigenvectors & eigenvalues
      Diag3 diag(xx, xy, xz, yy, yz, zz);
      diag.calculate(ew, e0, e1, e2);

      if(p_newData) {
         // Calculate the preferred scale factor (only need to do this 1 time)
         double scale;
         double emax = 0;
         for(i = 0 ; i < 3; i++) {
            if (fabs(ew[i]) > emax) {
               emax = fabs(ew[i]);
            }
         }      
         if (emax > 0) {
            scale = 1.0/emax;
         } else {
            scale = 1.0;
         }
         p_preferredScaleFactor = scale;
      }


      // We have 6 vectors for this property
      for(i = 0 ; i < 6; i++) {
         displacements.push_back(new double[3]);
         signs.push_back(NEUTRAL);
      }

      for(i = 0 ; i < 3; i++) {
         // push the regular vectors
         displacements[0][i] = ew[0] * e0[i];
         displacements[1][i] = ew[1] * e1[i];
         displacements[2][i] = ew[2] * e2[i];

         // now push their inverses
         displacements[3][i] = -1 * displacements[0][i];
         displacements[4][i] = -1 * displacements[1][i];
         displacements[5][i] = -1 * displacements[2][i];
      }
   }
   return ret;

}
