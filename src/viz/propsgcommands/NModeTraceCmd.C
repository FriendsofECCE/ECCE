#include <iostream>
  using std::cout;
  using std::endl;

//#include "inv/nodes/SoSwitch.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoShapeHints.H"

#include "util/Color.H"
#include "util/InternalException.H"


#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"

#include "tdat/SingleGrid.H"
#include "tdat/PropVecTable.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"
#include "dsm/PropFactory.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/PropSGFragment.H"
#include "viz/VRVector.H"
#include "viz/NModeTraceCmd.H"

/* WIN32 math.h does not define M_PI */
#ifndef M_PI
# define M_PI     3.14159265358979323846
#endif /* M_PI */


NModeTraceCmd::NModeTraceCmd(const string& name,Receiver *receiver,
                         IPropCalculation *calc)
  : PropCmd(name,receiver,calc)
{
  init();
}

NModeTraceCmd::~NModeTraceCmd() 
{
}

void NModeTraceCmd::init()
{
   addParameter(new CommandParameter("Mode", 0));
   addParameter(new CommandParameter("NumAnimations", 15));
   addParameter(new CommandParameter("Amplitude", -1.0));
   addParameter(new CommandParameter("Sign", false));

   // NOTE changed name for clarity
   // vizScale = true produces a periodic motion of the atoms,
   // vizScale = false results in a linear displacement along the
   // normal mode coordinate (This second option is useful for looking
   // at transition states).
   //addBoolParam("VizScale", true);  // Actual Displacement toggle
   addParameter(new CommandParameter("ActualDisplacement", false));

}


bool NModeTraceCmd::execute() throw(EcceException)
{
   bool ret = false;
   SGContainer *sg = dynamic_cast<SGContainer*>(getReceiver());

   // parameter info
   bool sign    = getParameter("Sign")->getBoolean();
   bool vizScale    = !getParameter("ActualDisplacement")->getBoolean();
   int mode     = getParameter("Mode")->getInteger();
   double factor= getParameter("Amplitude")->getDouble();
   int numAnimationSteps = getParameter("NumAnimations")->getInteger();
   //cout << "    incoming : " << mode << " " << factor  << sign << endl ;

   float signMult = (sign ? 1 : -1);
   //cout << "    incoming factor: " << factor << endl ;


   SGFragment *sgfrag = sg->getFragment() ;

   IPropCalculation *calc = getCalculation();
   INTERNALEXCEPTION(calc, "No calculation object!");

   sg->getNMRoot()->removeAllChildren();

  // Get a new table; it will store all the animation steps
  PropTSVecTable* animationTable = 
    (PropTSVecTable*)PropFactory::getProperty("VIB") ;


  // Get the property 
  PropTSVecTable* nmodes = (PropTSVecTable*)calc->getProperty("VIB");
  
  if (nmodes) {
    ret = true ;

    // create a propsgfrag to add to the scene graph
    // Need to reset to default coordinates
    // TODO
    // NOTE: I would prefer that this logic were outside the command.
    // However, when the slider value is set, the gui currently doesn't get
    // to intervene in an arbitrary way.
    //CVSGFragment* sgfrag = (CVSGFragment*)cvsg->getFragment() ;
    sgfrag->clear();
    calc->getFragment(*sgfrag);
    calc->getFragmentStep(sgfrag, -1); // hardwired to last step 
    PropSGFragment* propSGFrag = new PropSGFragment(*sgfrag);
    propSGFrag->setName("HiddenNMFrag");

    vector<TAtm*> *atoms = sgfrag->atoms();
    int natoms = atoms->size();
    // debug  - loop over table
    //   for(int a=0; a<natoms; a++){
    //cout << nmodes->value(mode,a,0) << "\t" 
    //   << nmodes->value(mode,a,1) << "\t" 
    //   << nmodes->value(mode,a,2) << endl ;  }
     
    double dx, dy, dz;
    double scale;
    double maxnorm = 0;
    int j;
    // find maximum component of normal mode
    for (j=0; j<natoms; j++) {
      dx = nmodes->value(mode,j,0);
      dy = nmodes->value(mode,j,1);
      dz = nmodes->value(mode,j,2);
      double norm = sqrt(dx*dx + dy*dy + dz*dz);
      if (norm > maxnorm)
        maxnorm = norm;
    }

    // In the old calcviewer, we used statics to keep track of 
    // last request to decide what to do.
    // Instead, use the incoming Amplitude value to decide.
    // If its -1.0, compute the factor.  Otherwise, use what is provided.
    if ( factor == -1.0) {
      scale = 0.25/maxnorm ;
    } else {
      scale = factor;
    }
    getParameter("Amplitude")->setDouble(scale);

    double stepMagnitude = scale/((double)numAnimationSteps);

    // the entire molecule will oscillate, using a sin function and
    // steps of  radians,  along the normal mode indexed by *mode. 
    double phiStep = 2.0*M_PI/(double)numAnimationSteps;

    SoSeparator *sep = new SoSeparator;
    sg->getNMRoot()->addChild(sep);

    // add PropSGFragment
    sep->addChild(propSGFrag);

    // add display
    sep->addChild(sg->getDisplayGroup());

    // add shape hints
    sep->addChild(sg->getShapeHints()) ;

	 // loop over each mode
    double x, y, z ;
    vector<double> listOfAllAtoms;
    for(int idx = 0; idx < numAnimationSteps; idx++)
    {
      double phiDisplace = (double)(idx) * phiStep;  // 0 <= phi < 2 PI

      for (j=0; j < natoms; j++) {

        if (vizScale) {
          x = ((*atoms)[j]->coordinates())[0] 
            + nmodes->value(mode,j,0) * sin(phiDisplace) * scale;
          listOfAllAtoms.push_back(x);

          y = ((*atoms)[j]->coordinates())[1] 
            + nmodes->value(mode,j,1) * sin(phiDisplace) * scale;
          listOfAllAtoms.push_back(y);

          z = ((*atoms)[j]->coordinates())[2] 
            + nmodes->value(mode,j,2) * sin(phiDisplace) * scale;
          listOfAllAtoms.push_back(z);
        } else {
          x = ((*atoms)[j]->coordinates())[0] 
            + nmodes->value(mode,j,0) * idx * signMult * stepMagnitude;
          listOfAllAtoms.push_back(x);

          y = ((*atoms)[j]->coordinates())[1] 
            + nmodes->value(mode,j,1) * idx * signMult * stepMagnitude;
          listOfAllAtoms.push_back(y);

          z = ((*atoms)[j]->coordinates())[2] 
            + nmodes->value(mode,j,2) * idx * signMult * stepMagnitude;
          listOfAllAtoms.push_back(z);
        }
      }
    }
    
    // throw everything into a table of size, steps X atom coords
    animationTable->appendTable(numAnimationSteps, natoms*3, listOfAllAtoms);
    //cout << "Table info:  rows - " << animationTable->rows() 
    //   << " cols - " << animationTable->columns() << endl ;

    // save the table into PropSGFrag
    propSGFrag->setTraceTable(animationTable);

    double coords[3] ;
    int row = 0 ;  // we want the first step
    int table = 0; // we only have one table
    for (j=0; j < natoms; j++) 
    {
      coords[0] = animationTable->value(table, row, j*3);
      coords[1] = animationTable->value(table, row, (j*3)+1);
      coords[2] = animationTable->value(table, row, (j*3)+2);

      //cout << "new coords: " ;
      //for (int t=0; t<3; t++) cout << coords[t] << " " ;
      //cout << endl;

      propSGFrag->atomRef(j)->coordinates(coords);
    }
    sg->touchChemDisplay(propSGFrag) ;
    propSGFrag->touchNumbers() ;
    sg->updateMeasures() ;

  }


   return ret;
}
