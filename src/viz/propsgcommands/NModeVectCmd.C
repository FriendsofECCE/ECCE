#include <iostream>
  using std::cout;
  using std::endl;

//#include "inv/nodes/SoSwitch.H"

#include "util/Color.H"
#include "util/InternalException.H"


#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"

#include "tdat/SingleGrid.H"
#include "tdat/PropVecTable.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/VRVector.H"
#include "viz/NModeVectCmd.H"

NModeVectCmd::NModeVectCmd(const string& name,Receiver *receiver,
                         IPropCalculation *calc)
  : PropCmd(name,receiver,calc)
{
  init();
}

NModeVectCmd::~NModeVectCmd() 
{
}

void NModeVectCmd::init()
{
   addParameter(new CommandParameter("Mode", 0));
   addParameter(new CommandParameter("Amplitude", -1.0));
   addParameter(new CommandParameter("Color", "#cccc00"));
   addParameter(new CommandParameter("Sign", false));
}

bool NModeVectCmd::execute() throw(EcceException)
{
   bool ret = false;
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   // parameter info
   bool sign    = getParameter("Sign")->getBoolean();
   int mode     = getParameter("Mode")->getInteger();
   double factor= getParameter("Amplitude")->getDouble();

   float signMult = (sign ? 1 : -1);
   //cout << "    incoming factor: " << factor << endl ;

   string neutral = getParameter("Color")->getString();
   Color *color = new Color(neutral);


   SGFragment *sgfrag = sg->getFragment() ;

   double maxnorm = 0.0;

   sg->getNMVecRoot()->removeAllChildren();

   IPropCalculation *calc = getCalculation();
   INTERNALEXCEPTION(calc, "No calculation object!");


   // Get the property 
   PropVecTable* vecTable = (PropVecTable*)calc->getProperty("VIB");
   if (vecTable) {
      ret = true;
      vector<TAtm*>* atoms = sgfrag->atoms();

      int natoms = atoms->size();
      float cx,cy,cz, dx,dy,dz;
      double scale ;
      int j ;

      //find max displacement vect
      for (j=0; j< natoms; j++) {
         dx = vecTable->value(mode,j,0);
         dy = vecTable->value(mode,j,1);
         dz = vecTable->value(mode,j,2);

         double norm = sqrt( dx*dx + dy*dy + dz*dz ) ;
         if ( norm > maxnorm )
            maxnorm = norm ;
      }

      // In the old calcviewer, we used statics to keep track of 
      // last request to decide what to do.
      // Instead, use the incoming Amplitude value to decide.
      // If its -1.0, compute the factor.  Otherwise, use what is provided.
      if ( factor == -1.0) {
         scale = 1.0/maxnorm ;
      } else {
         scale = factor;
      }
      getParameter("Amplitude")->setDouble(scale);


      // loop over displaced geometries
      for (j=0; j< natoms; j++) {
         cx=  (*atoms)[j]->coordinates()[0];
         cy=  (*atoms)[j]->coordinates()[1];
         cz=  (*atoms)[j]->coordinates()[2];
         dx = vecTable->value(mode,j,0);
         dy = vecTable->value(mode,j,1);
         dz = vecTable->value(mode,j,2);

         dx = dx * scale * signMult;
         dy = dy * scale * signMult;
         dz = dz * scale * signMult;
         //cout << "atom: " << j << " " << cx << "/" << dx << " " << cy << "/" << dy << " " << cz << "/" << dz << endl;

         // vect mode vector
         VRVector * dip = new VRVector;
         dip->position(cx,cy,cz); 
         dip->direction(dx,dy,dz );
         dip->setColor(color->red(), color->green(), color->blue());
         //dip->scaleFactor(scale, scale, scale) ;
         sg->getNMVecRoot()->addChild(dip);

      }

   }
   delete color;

   return ret;
}
