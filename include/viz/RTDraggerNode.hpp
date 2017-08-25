/*------------------------------------------------------------
 *  This is an example from the Inventor Toolmaker,
 *  chapter 8, example 14.
 *
 *  Header file for "RTDraggerNode"
 *
 *  This is the source file for the RTDraggerNode.
 *  It is a compound dragger which allows independent rotation 
 *  in the x,y,and z directions as well as translation along a 
 *  line. It is an example of how to write a compound dragger.
 *
 *  It is composed of a TranslateRadialDragger and 
 *  3 SoRotateCylindricalDraggers bound into one dragger.
 *
 *  Clicking on the TranslateRadialDragger piece results in a
 *  translational motion along the line formed by the center of
 *  the dragger and the point on the dragger that was hit.
 *
 *  Clicking a cylinder rotator results in a rotation about 
 *  either the x,y,or z axis.
 *
 *  Geometry resources and part names for this dragger:
 *
 *  Resource Names:                     Part Names:
 *rotTransTranslatorTranslator       translator.translator
 *rotTransTranslatorTranslatorActive translator.translatorActive
 *rotTransTranslatorFeedback         translator.feedback
 *rotTransTranslatorFeedbackActive   translator.feedbackActive
 *
 *rotTransRotatorRotator             XRotator.rotator
 *rotTransRotatorRotatorActive       XRotator.rotatorActive
 *rotTransRotatorFeedback            XRotator.feedback
 *rotTransRotatorFeedbackActive      XRotator.feedbackActive
 *     (and similarly for parts of the YRotator and ZRotator)
 *
 *-----------------------------------------------------------*/

#ifndef  _RTDRAGGERNODE_
#define  _RTDRAGGERNODE_

#include "inv/sensors/SoFieldSensor.H"
#include "inv/draggers/SoDragger.H"
#include "inv/fields/SoSFVec3f.H"
#include "inv/fields/SoSFRotation.H"
#include "inv/nodes/SoTranslation.H"

 class TranslateRadialDragger;
 class SoRotateCylindricalDragger;

 class RTDraggerNode : public SoDragger
{
  SO_KIT_HEADER(RTDraggerNode);

  // Makes the dragger surround other objects
  SO_KIT_CATALOG_ENTRY_HEADER(surroundScale);
  // Keeps the dragger evenly sized in all 3 dimensions
  SO_KIT_CATALOG_ENTRY_HEADER(antiSquish);

  // The translating dragger...
  SO_KIT_CATALOG_ENTRY_HEADER(translator);

  // The X and Z rotators need to be turned so as to orient 
  // correctly. So create a separator part and put an 
  // SoRotation node and the dragger underneath.
  SO_KIT_CATALOG_ENTRY_HEADER(XRotatorSep);
  SO_KIT_CATALOG_ENTRY_HEADER(XRotatorRot);
  SO_KIT_CATALOG_ENTRY_HEADER(XRotator);
  SO_KIT_CATALOG_ENTRY_HEADER(YRotator);
  SO_KIT_CATALOG_ENTRY_HEADER(ZRotatorSep);
  SO_KIT_CATALOG_ENTRY_HEADER(ZRotatorRot);
  SO_KIT_CATALOG_ENTRY_HEADER(ZRotator);

  public:

  // Constructor
  RTDraggerNode();

  SbVec3f old_trans;
  SbVec3f center;
  SbRotation  old_rot;
  float save_scale;
  SoTranslation * tran;
  SoScale  *sca;
  SbBool  Alert;
  SoSeparator *parent;

  // These fields reflect state of the dragger at all times.
  SoSFRotation rotation;
  SoSFVec3f   translation;

  // This should be called once after SoInteraction::init().
  static void initClass();
  void reset();

  protected:

  // These sensors insures that the motionMatrix is updated 
  // when the fields are changed from outside.
  SoFieldSensor *rotFieldSensor;
  SoFieldSensor *translFieldSensor;
  static void fieldSensorCB(void *, SoSensor *);

  //  virtual void  GLRender(SoGLRenderAction *action);


  // This function is invoked by the child draggers when they 
  // change their value.
  static void valueChangedCB(void *, SoDragger *);

  // Called at the beginning and end of each dragging motion.
  // Tells the "surroundScale" part to recalculate.
  static void invalidateSurroundScaleCB(void *, SoDragger *);

  // This will detach/attach the fieldSensor.
  // It is called at the end of the constructor (to attach).
  // and at the start/end of SoBaseKit::readInstance()
  // and on the new copy at the start/end of SoBaseKit::copy()
  // Returns the state of the node when this was called.
  virtual SbBool setUpConnections( SbBool onOff, 
      SbBool doItAlways = FALSE);

  // This allows us to specify that certain parts do not
  // write out. We'll use this on the antiSquish and
  // surroundScale parts.
  virtual void setDefaultOnNonWritingFields();

  private:

  static const char geomBuffer[];

  // Destructor.
  ~RTDraggerNode();
};   

#endif  /* _RTDRAGGERNODE_ */
