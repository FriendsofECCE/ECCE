#include <iostream>
  using std::cout;
  using std::endl;


#include "util/InternalException.H"
#include "util/Diag3.H"

#include "dsm/IPropCalculation.H"

#include "tdat/PropTable.H"

#include "viz/cvSGContainer.H"
#include "viz/AtomCoordVecCmd.H"

AtomCoordVecCmd::AtomCoordVecCmd(const string& name, Receiver *receiver,
                         IPropCalculation *calc)
  : AddVectorCmd(name, receiver, calc)
{
  // Parent class does everything
}

AtomCoordVecCmd::~AtomCoordVecCmd()
{
}

bool AtomCoordVecCmd::getDisplacements(vector<double*>& displacements,
                                          vector<SIGN>& signs)
{
   bool ret = false;
   // parameter info
   bool sign = getParameter("Sign")->getBoolean();;
   float signMult = (sign ? 1 : -1); 

   // Get the calculation
   IPropCalculation *propcalc = getCalculation();
   INTERNALEXCEPTION(propcalc, "No calculation object!");

   // Get the property 
   PropTable* vecTable = 
      (PropTable*) propcalc->getProperty(getPropertyName());

   if (vecTable != (PropTable*)0) {
      ret = true;
      int i;
      if(p_newData) {
         // Calculate the preferred scale factor (only need to do this 1 time)
         float dx,dy,dz; 
         double norm;
         double maxnorm = 0.0;

         for (i = 0; i < vecTable->rows(); i++) {
            dx = vecTable->value(i,0);
            dy = vecTable->value(i,1);
            dz = vecTable->value(i,2);

            norm = sqrt( dx*dx + dy*dy + dz*dz );
            if ( norm > maxnorm ) {
               maxnorm = norm ;
            }
         }
         p_preferredScaleFactor = log10(1.0/maxnorm);
      }

      // Get the displacements from the PropTable, with preferred sign multiplier
      for (i = 0; i < vecTable->rows(); i++) {

         displacements.push_back(new double[3]);
         displacements[i][0] = vecTable->value(i,0) * signMult;
         displacements[i][1] = vecTable->value(i,1) * signMult;
         displacements[i][2] = vecTable->value(i,2) * signMult;

         signs.push_back(NEUTRAL);
      }      
   }

   return ret;

}


/**
 * We have to append to the superclass's method because 
 * we are using a log scale (we need to convert these from
 * log value to normal value).
 */
double AtomCoordVecCmd::getScaleFactor()
{
   double scale = AddVectorCmd::getScaleFactor();
   return pow(10, scale);
}
