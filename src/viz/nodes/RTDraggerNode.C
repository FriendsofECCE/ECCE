#include "inv/nodes/SoRotation.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoTransform.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/nodes/SoSurroundScale.H"
#include "inv/nodes/SoAntiSquish.H"
#include "inv/sensors/SoFieldSensor.H"
#include "inv/draggers/SoRotateCylindricalDragger.H"

#include "viz/TranslateRadialDragger.H"

// Include file for our new class.
#include "viz/RTDraggerNode.H"

// This file contains RotTransDragger::geomBuffer, whic 
// describes the default geometry resources for his class.
#include "viz/RTDraggerGeom.H"

SO_KIT_SOURCE(RTDraggerNode);


//  Initializes the type ID for this dragger node. This
//  should be called once after SoInteraction::init().
void RTDraggerNode::initClass()
{
  SO_KIT_INIT_CLASS(RTDraggerNode, SoDragger, "Dragger");	
}

RTDraggerNode::RTDraggerNode()
{

  Alert = FALSE;
  SO_KIT_CONSTRUCTOR(RTDraggerNode);

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
  // $(SO_DRAGGER_DIR)/rotTransDragger.iv"
  if (SO_KIT_IS_FIRST_INSTANCE())
    readDefaultParts("rotTransDragger.iv", geomBuffer, 
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

  // Set rotations in "XRotatorRot" and "ZRotatorRot" parts.
  // These parts will orient the draggers from their default 
  // (rotating about Y) to the desired configurations.
  // By calling 'setAnyPartAsDefault' instead of 'setAnyPart'
  // we insure that they will not be written out, unless
  // they are changed later on.
  SoRotation *XRot = new SoRotation;
  XRot->rotation.setValue( SbRotation(SbVec3f(0,1,0), SbVec3f(1,0,0)));
  setAnyPartAsDefault("XRotatorRot", XRot);

  SoRotation *ZRot = new SoRotation;
  ZRot->rotation.setValue(SbRotation(SbVec3f(0,1,0), SbVec3f(0,0,1)));
  setAnyPartAsDefault("ZRotatorRot", ZRot);

  // Updates the fields when motionMatrix changes 
  addValueChangedCallback(&RTDraggerNode::valueChangedCB);

  // Updates motionMatrix when either field changes.
  rotFieldSensor = new SoFieldSensor(&RTDraggerNode::fieldSensorCB, this);
  rotFieldSensor->setPriority(0);
  translFieldSensor = new SoFieldSensor(&RTDraggerNode::fieldSensorCB,this);
  translFieldSensor->setPriority(0);

  setUpConnections( TRUE, TRUE );

  parent = (SoSeparator*)0;
}

RTDraggerNode::~RTDraggerNode()
{
  if (rotFieldSensor) delete rotFieldSensor;
  if (translFieldSensor) delete translFieldSensor;
}


SbBool RTDraggerNode::setUpConnections( SbBool onOff, 
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
        &RTDraggerNode::invalidateSurroundScaleCB, this);
    tD->addFinishCallback(
        &RTDraggerNode::invalidateSurroundScaleCB, this);
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
        &RTDraggerNode::invalidateSurroundScaleCB, this);
    XD->addFinishCallback(
        &RTDraggerNode::invalidateSurroundScaleCB, this);
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
        &RTDraggerNode::invalidateSurroundScaleCB, this);
    YD->addFinishCallback(
        &RTDraggerNode::invalidateSurroundScaleCB, this);
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
        &RTDraggerNode::invalidateSurroundScaleCB, this);
    ZD->addFinishCallback(
        &RTDraggerNode::invalidateSurroundScaleCB, this);
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
        &RTDraggerNode::invalidateSurroundScaleCB, this);
    tD->removeFinishCallback(
        &RTDraggerNode::invalidateSurroundScaleCB, this);
    unregisterChildDragger(tD);

    SoDragger *XD 
      = (SoDragger *) getAnyPart("XRotator", FALSE);
    XD->removeStartCallback(
        &RTDraggerNode::invalidateSurroundScaleCB, this);
    XD->removeFinishCallback(
        &RTDraggerNode::invalidateSurroundScaleCB, this);
    unregisterChildDragger(XD);

    SoDragger *YD 
      = (SoDragger *) getAnyPart("YRotator", FALSE);
    YD->removeStartCallback(
        &RTDraggerNode::invalidateSurroundScaleCB, this);
    YD->removeFinishCallback(
        &RTDraggerNode::invalidateSurroundScaleCB, this);
    unregisterChildDragger(YD);

    SoDragger *ZD 
      = (SoDragger *) getAnyPart("ZRotator", FALSE);
    ZD->removeStartCallback(
        &RTDraggerNode::invalidateSurroundScaleCB, this);
    ZD->removeFinishCallback(
        &RTDraggerNode::invalidateSurroundScaleCB, this);
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
void
RTDraggerNode::valueChangedCB(void *, SoDragger *inDragger)
{
  RTDraggerNode *myself = (RTDraggerNode *) inDragger;

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
void RTDraggerNode::fieldSensorCB(void *inDragger, SoSensor *)
{
  RTDraggerNode *myself = (RTDraggerNode *) inDragger;

  SbMatrix motMat = myself->getMotionMatrix();
  myself->workFieldsIntoTransform(motMat);

  myself->setMotionMatrix(motMat);
}

// When any child dragger starts or ends a drag, tell the
// "surroundScale" part (if it exists) to invalidate its 
// current bounding box value.
void RTDraggerNode::invalidateSurroundScaleCB(void *parent, SoDragger *)
{
  RTDraggerNode *myParentDragger 
    = (RTDraggerNode *) parent;

  // Invalidate the surroundScale, if it exists.
  SoSurroundScale *mySS = SO_CHECK_PART(
      myParentDragger, "surroundScale", SoSurroundScale);
  if (mySS != NULL)
    mySS->invalidate();
}

void RTDraggerNode::setDefaultOnNonWritingFields()
{
  // These nodes pointed to by these part-fields may 
  // change after construction, but we
  // don't want to write them out.
  surroundScale.setDefault(TRUE);
  antiSquish.setDefault(TRUE);

  SoDragger::setDefaultOnNonWritingFields();
}


void RTDraggerNode::reset()
{
}
