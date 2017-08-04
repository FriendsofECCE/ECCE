#include "inv/nodes/SoRotation.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoTransform.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/nodes/SoSurroundScale.H"
#include "inv/nodes/SoAntiSquish.H"
#include "inv/sensors/SoFieldSensor.H"
#include "inv/draggers/SoRotateCylindricalDragger.H"


// Include file for our new class.
#include "viz/TranslateRadialDragger.H"
#include "viz/AtomRTDragger.H"
#include "viz/SGFragment.H"

// This file contains RotTransDragger::geomBuffer, whic 
// describes the default geometry resources for his class.
#include "viz/AtomRTDraggerGeom.H"

SO_KIT_SOURCE(AtomRTDragger);


//  Initializes the type ID for this dragger node. This
//  should be called once after SoInteraction::init().
void AtomRTDragger::initClass()
{
   SO_KIT_INIT_CLASS(AtomRTDragger, SoDragger, "Dragger");	
}

AtomRTDragger::AtomRTDragger()
{
  m_alert = FALSE;
  SO_KIT_CONSTRUCTOR(AtomRTDragger);

  // Don't create "surroundScale" by default. It's only put 
  // to use if this dragger is used within a manipulator.
  SO_KIT_ADD_CATALOG_ENTRY(surroundScale, SoSurroundScale,
      TRUE, topSeparator, geomSeparator,TRUE);
  // Create an anti-squish node by default.
  SO_KIT_ADD_CATALOG_ENTRY(antiSquish, SoAntiSquish,
      FALSE, topSeparator, geomSeparator,TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(translator, TranslateRadialDragger,
      TRUE, topSeparator, geomSeparator,TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(XRotatorSep, SoSeparator,
      FALSE, topSeparator, geomSeparator,FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(XRotatorRot, SoRotation,
      TRUE, XRotatorSep, ,FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(XRotator, 
      SoRotateCylindricalDragger, TRUE, XRotatorSep, ,TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(YRotator, 
      SoRotateCylindricalDragger, TRUE, topSeparator, 
      geomSeparator, TRUE);
  SO_KIT_ADD_CATALOG_ENTRY(ZRotatorSep, SoSeparator,
      FALSE, topSeparator, geomSeparator,FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(ZRotatorRot, SoRotation,
      TRUE, ZRotatorSep, ,FALSE);
  SO_KIT_ADD_CATALOG_ENTRY(ZRotator, 
      SoRotateCylindricalDragger, TRUE, ZRotatorSep, ,TRUE);

  // Read geometry resources. Only do this the first time we
  // construct one. 'geomBuffer' contains our compiled in
  // defaults. The user can override these by specifying new
  // scene graphs in the file:
  // $(SO_DRAGGER_DIR)/AtomRTDragger.iv"
  if (SO_KIT_IS_FIRST_INSTANCE())
    readDefaultParts("AtomRTDragger.iv", geomBuffer, 
        sizeof(geomBuffer));

  // Fields that always show current state of the dragger.
  SO_KIT_ADD_FIELD(rotation, (0.0, 0.0, 0.0, 1.0));
  SO_KIT_ADD_FIELD(translation, (0.0, 0.0, 0.0));

  // Creates parts list and default parts for this nodekit.
  SO_KIT_INIT_INSTANCE();

  // Make the anti-squish node surround the biggest dimension
  SoAntiSquish *myAntiSquish 
    = SO_GET_ANY_PART(this, "antiSquish", SoAntiSquish);
  myAntiSquish->sizing = SoAntiSquish::BIGGEST_DIMENSION;

  // Create the simple draggers that comprise this dragger.
  // This dragger has four simple pieces:  
  //    1 TranslateRadialDragger
  //    3 RotateCylindricalDraggers
  // In the constructor, we just call SO_GET_ANY_PART to
  // build each dragger.
  // Within the setUpConnections() method, we will
  // take care of giving these draggers new geometry and 
  // establishing their callbacks.

  // Create the translator dragger.    
  // Cast output from the following calls to void to avoid compiler warnings
  (void)SO_GET_ANY_PART(this, "translator", TranslateRadialDragger);

  // Create the XRotator dragger.    
  (void)SO_GET_ANY_PART(this, "XRotator", SoRotateCylindricalDragger);

  // Create the YRotator dragger.    
  (void)SO_GET_ANY_PART(this, "YRotator", SoRotateCylindricalDragger);

  // Create the ZRotator dragger.    
  (void)SO_GET_ANY_PART(this, "ZRotator", SoRotateCylindricalDragger);

  // Set rotations in "XRotatorRot" and "ZRotatorRot" parts.
  // These parts will orient the draggers from their default 
  // (rotating about Y) to the desired configurations.
  // By calling 'setAnyPartAsDefault' instead of 'setAnyPart'
  // we insure that they will not be written out, unless
  // they are changed later on.
  SoRotation *XRot = new SoRotation;
  XRot->rotation.setValue(
      SbRotation(SbVec3f(0,1,0), SbVec3f(1,0,0)));
  setAnyPartAsDefault("XRotatorRot", XRot);

  SoRotation *ZRot = new SoRotation;
  ZRot->rotation.setValue(
      SbRotation(SbVec3f(0,1,0), SbVec3f(0,0,1)));
  setAnyPartAsDefault("ZRotatorRot", ZRot);

  // Updates the fields when motionMatrix changes 
  addValueChangedCallback(&AtomRTDragger::valueChangedCB);

  // Updates motionMatrix when either field changes.
  rotFieldSensor = new SoFieldSensor(
      &AtomRTDragger::fieldSensorCB, this);
  rotFieldSensor->setPriority(0);
  translFieldSensor = new SoFieldSensor(
      &AtomRTDragger::fieldSensorCB,this);
  translFieldSensor->setPriority(0);

  setUpConnections( TRUE, TRUE );

  p_parent = (SoSeparator*)0;
}

AtomRTDragger::~AtomRTDragger()
{
  if (rotFieldSensor)
    delete rotFieldSensor;
  if (translFieldSensor)
    delete translFieldSensor;
}


SbBool AtomRTDragger::setUpConnections( SbBool onOff, 
    SbBool doItAlways)
{
  if ( !doItAlways && connectionsSetUp == onOff)
    return onOff;

  if (onOff) {
    // We connect AFTER base class.
    SoDragger::setUpConnections(onOff, doItAlways);

    // For each of the simple draggers that compries this:
    // [a]Call setPart after looking up our replacement parts 
    //    in the global dictionary.
    // [b]Add the invalidateSurroundScaleCB as a start and end
    //    callback. When using a surroundScale node, these 
    //    trigger it to recalculate a bounding box at the 
    //    beginning and end of dragging.
    // [c]Register the dragger as a 'childDragger' of this 
    //    one. This has the following effects: 
    //    [1] This dragger's callbacks will be invoked 
    //        following the child manip's callbacks.  
    //    [2] When the child is dragged, the child's motion 
    //        will be transferred into motion of the entire 
    //        dragger.
    SoDragger *tD 
      = (SoDragger *) getAnyPart("translator", FALSE);
    // [a] Set up the parts in the child dragger...
    tD->setPartAsDefault("translator",
        "rotTransTranslatorTranslator");
    tD->setPartAsDefault("translatorActive",
        "rotTransTranslatorTranslatorActive");
    tD->setPartAsDefault("feedback",
        "rotTransTranslatorFeedback");
    tD->setPartAsDefault("feedbackActive",
        "rotTransTranslatorFeedbackActive");
    // [b] and [c] Add the callbacks and register the child
    tD->addStartCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    tD->addFinishCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    registerChildDragger(tD);

    SoDragger *XD 
      = (SoDragger *) getAnyPart("XRotator", FALSE);
    // [a] Set up the parts in the child dragger...
    XD->setPartAsDefault("rotator",
        "rotTransRotatorRotator");
    XD->setPartAsDefault("rotatorActive",
        "rotTransRotatorRotatorActive");
    XD->setPartAsDefault("feedback",
        "rotTransRotatorFeedback");
    XD->setPartAsDefault("feedbackActive",
        "rotTransRotatorFeedbackActive");
    // [b] and [c] Add the callbacks and register the child
    XD->addStartCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    XD->addFinishCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    registerChildDragger(XD);

    SoDragger *YD 
      = (SoDragger *) getAnyPart("YRotator", FALSE);
    // [a] Set up the parts in the child dragger...
    YD->setPartAsDefault("rotator",
        "rotTransRotatorRotator");
    YD->setPartAsDefault("rotatorActive",
        "rotTransRotatorRotatorActive");
    YD->setPartAsDefault("feedback",
        "rotTransRotatorFeedback");
    YD->setPartAsDefault("feedbackActive",
        "rotTransRotatorFeedbackActive");
    // [b] and [c] Add the callbacks and register the child
    YD->addStartCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    YD->addFinishCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    registerChildDragger(YD);

    SoDragger *ZD 
      = (SoDragger *) getAnyPart("ZRotator", FALSE);
    // [b] Set up the parts in the child dragger...
    ZD->setPartAsDefault("rotator",
        "rotTransRotatorRotator");
    ZD->setPartAsDefault("rotatorActive",
        "rotTransRotatorRotatorActive");
    ZD->setPartAsDefault("feedback",
        "rotTransRotatorFeedback");
    ZD->setPartAsDefault("feedbackActive",
        "rotTransRotatorFeedbackActive");
    // [c] and [d] Add the callbacks and register the child
    ZD->addStartCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    ZD->addFinishCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    registerChildDragger(ZD);


    // Call the sensor CB to make things up-to-date.
    fieldSensorCB(this, NULL);

    // Connect the field sensors
    if (translFieldSensor->getAttachedField() != &translation)
      translFieldSensor->attach( &translation );
    if (rotFieldSensor->getAttachedField() != &rotation)
      rotFieldSensor->attach( &rotation );
  }
  else {
    // We disconnect BEFORE base class.

    // Remove the callbacks from the child draggers,
    // and unregister them as children.
    SoDragger *tD 
      = (SoDragger *) getAnyPart("translator", FALSE);
    tD->removeStartCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    tD->removeFinishCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    unregisterChildDragger(tD);

    SoDragger *XD 
      = (SoDragger *) getAnyPart("XRotator", FALSE);
    XD->removeStartCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    XD->removeFinishCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    unregisterChildDragger(XD);

    SoDragger *YD 
      = (SoDragger *) getAnyPart("YRotator", FALSE);
    YD->removeStartCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    YD->removeFinishCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    unregisterChildDragger(YD);

    SoDragger *ZD 
      = (SoDragger *) getAnyPart("ZRotator", FALSE);
    ZD->removeStartCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    ZD->removeFinishCallback(
        &AtomRTDragger::invalidateSurroundScaleCB, this);
    unregisterChildDragger(ZD);

    // Disconnect the field sensors.
    if (translFieldSensor->getAttachedField())
      translFieldSensor->detach();
    if (rotFieldSensor->getAttachedField())
      rotFieldSensor->detach();

    SoDragger::setUpConnections(onOff, doItAlways);
  }

  return !(connectionsSetUp = onOff);
}


// Called when the motionMatrix changes. Sets the "translation"
// and "rotation" fields based on the new motionMatrix
void AtomRTDragger::valueChangedCB(void *, SoDragger *inDragger)
{
  AtomRTDragger *myself = (AtomRTDragger *) inDragger;

  // Factor the motionMatrix into its parts
  SbMatrix motMat = myself->getMotionMatrix();
  SbVec3f   trans, scale;
  SbRotation rot, scaleOrient;
  motMat.getTransform(trans, rot, scale, scaleOrient);

  // Set the fields. Disconnect the sensors while doing so.
  myself->rotFieldSensor->detach();
  myself->translFieldSensor->detach();
  if (myself->rotation.getValue() != rot)
    myself->rotation = rot;
  if (myself->translation.getValue() != trans)
    myself->translation = trans;
  myself->rotFieldSensor->attach(&(myself->rotation));
  myself->translFieldSensor->attach(&(myself->translation));
}

// If the "translation" or "rotation" field changes, changes
// the motionMatrix accordingly.
void AtomRTDragger::fieldSensorCB(void *inDragger, SoSensor *)
{
  AtomRTDragger *myself = (AtomRTDragger *) inDragger;

  SbMatrix motMat = myself->getMotionMatrix();
  myself->workFieldsIntoTransform(motMat);

  myself->setMotionMatrix(motMat);
}

// When any child dragger starts or ends a drag, tell the
// "surroundScale" part (if it exists) to invalidate its 
// current bounding box value.
void AtomRTDragger::invalidateSurroundScaleCB(void *parent, 
    SoDragger *)
{
  AtomRTDragger *myParentDragger 
    = (AtomRTDragger *) parent;

  // Invalidate the surroundScale, if it exists.
  SoSurroundScale *mySS = SO_CHECK_PART(
      myParentDragger, "surroundScale", SoSurroundScale);
  if (mySS != NULL)
    mySS->invalidate();
}

void AtomRTDragger::setDefaultOnNonWritingFields()
{
  // These nodes pointed to by these part-fields may 
  // change after construction, but we
  // don't want to write them out.
  surroundScale.setDefault(TRUE);
  antiSquish.setDefault(TRUE);

  SoDragger::setDefaultOnNonWritingFields();
}


void AtomRTDragger::reset()
{

  m_center.setValue(0.,0.,0.);
  m_old_trans.setValue(0.,0.,0.);
  m_old_rot.setValue(rotation.getValue().getValue());

  int at,j;
  int counter = 0;
  for (j=0 ; j < m_selected.size()  ; j++)
  {
    at = ((Fragment*)p_frag)->atomIndex(*(m_selected[j]));
    m_center = m_center + p_frag->getAtomCoordinates(at);
    counter ++;
  }

  m_center = ( 1./counter ) *m_center;

  SbVec3f  pt;
  float  radii = 1.0;
  for ( j=0 ; j < m_selected.size()  ; j++)
  { 
    at = ((Fragment*)p_frag)->atomIndex(*(m_selected[j]));
    pt = p_frag->getAtomCoordinates(at) - m_center;
    if (pt.length() > radii) radii = pt.length();
  }
  enableValueChangedCallbacks(FALSE);
  m_save_scale = 0.5*radii;
  m_sca->scaleFactor.setValue(0.5*radii,0.5*radii,0.5*radii);
  m_tran->translation.setValue(m_center);  
  enableValueChangedCallbacks(TRUE);


}

void AtomRTDragger::setFragment(SGFragment *frag)
{ p_frag = frag; }

SGFragment *AtomRTDragger::getFragment()
{ return p_frag; };

void AtomRTDragger::setParent(SoSeparator *parent)
{ p_parent = parent; }

SoSeparator *AtomRTDragger::getParent()
{ return p_parent; };
