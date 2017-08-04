#include "inv/SoDB.H"
#include "inv/nodekits/SoWrapperKit.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/actions/SoPickAction.H"

#include "inv/nodes/SoCone.H"
#include "inv/nodes/SoCylinder.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/nodes/SoTransform.H"
#include "inv/sensors/SoTimerSensor.H"

#include "viz/VR3dXVSmouse.H"

#include "viz/RTDraggerNode.H"

#define	SPACEBALL_PORT	"/dev/ttyd72"
#define	SPACEBALL_BAUD	9600

SO_NODE_SOURCE(VR3dXVSmouse);

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void VR3dXVSmouse::initClass()
{
   SO_NODE_INIT_CLASS(VR3dXVSmouse, SoGroup,"Group");
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
VR3dXVSmouse::VR3dXVSmouse()
{
  SO_NODE_CONSTRUCTOR(VR3dXVSmouse);
  constructor();
}


/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
void VR3dXVSmouse::constructor()
{

     SoSeparator * sep = new SoSeparator;
     addChild(sep);

     SoScale * scal = new SoScale;
     sep->addChild(scal);
     scal->scaleFactor.setValue(0.2,0.2,0.2);
  
     mickeyDragg =  new RTDraggerNode;
     sep->addChild(mickeyDragg);

     SoTransform * transf = new SoTransform;
     sep->addChild(transf);
     transf->translation.connectFrom(&mickeyDragg->translation);
     transf->rotation.connectFrom(&mickeyDragg->rotation);

     SoTranslation * trans = new SoTranslation;
     sep->addChild(trans);
     trans->translation.setValue(0.,4.,0.);
     SoCylinder *  cylinder = new SoCylinder;
     sep->addChild(cylinder);
     cylinder->height.setValue(4.);
     cylinder->radius.setValue(0.5);

     trans = new SoTranslation;
     sep->addChild(trans);
     trans->translation.setValue(0.,3.,0.);
     SoCone *  cone = new SoCone;
     sep->addChild(cone);

#ifdef XVS_OK

     sensor = new XvsSpaceball();
//   sensor = new XvsFlockOfBirds();

#ifdef DEBUG
     if (sensor->open ( SPACEBALL_PORT, SPACEBALL_BAUD, 1) >= 0)
       cout << " sensor is open " << endl;
     else
       cout << " enable to open sensor " << endl;
#endif

     sensor->setRelative ( );

 
     maxToggles = sensor->howManyToggles ();
     maxDeviceData = sensor->howManyDeviceData ();
     tval = new XvsToggles(maxToggles);
     dval = new XvsDeviceData(maxDeviceData);

     SoTimerSensor * animationSensor = 
     new SoTimerSensor(VR3dXVSmouse::animationSensorCB, this);
     animationSensor->setInterval(1/60.0); // 60 frames per second
     animationSensor->schedule();
#endif
}



/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
VR3dXVSmouse::~VR3dXVSmouse()
{
}




//=============================================================================
   void VR3dXVSmouse::animationSensorCB(void *userData, SoSensor *)
//=============================================================================
{ 
   ((VR3dXVSmouse *) userData)->doReposition();
}

//=============================================================================
   void VR3dXVSmouse::doReposition()
//=============================================================================
{
   
#ifdef DEBUG
#ifdef XVS_OK
   // Label the position and orientation columns
   printf("\n    X       Y       Z      Yaw    Pitch    Roll\n\n");
   r = sensor->read (pval, aval, *tval, *dval);

   // Don't print when read has an error
   // Read function will print error messages itself
   if (r >= 0) {
		printf ("%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f ",
			pval [0], pval [1], pval [2], 
			aval [0], aval [1], aval [2]);
		for (int j = 0; j < maxToggles; j++)
		    printf (" %d", (*tval)[j]);
		for (j = 0; j < maxDeviceData; j++)
		    printf ("%.2f", (*dval)[j]);
		printf ("\n");
	
            SbVec3f vec;
              vec.setValue(pval [0], pval [2],-  pval [1]);
	    getDragger()->translation.setValue(
             getDragger()->translation.getValue() + 
             vec );
            }
#endif
#endif

} 

//=============================================================================
   RTDraggerNode * VR3dXVSmouse::getDragger()
//=============================================================================
{
    
   return  mickeyDragg;
} 
