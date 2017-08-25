#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "SoWxExaminerViewer.h"
#endif

#include <wx/panel.h>

#include "inv/SoDB.H"
#include "inv/SoInput.H"
#include "inv/sensors/SoFieldSensor.H"
#include "inv/fields/SoSFTime.H"
#include "inv/nodes/SoOrthographicCamera.H"
#include "inv/nodes/SoPerspectiveCamera.H"
#include "inv/nodes/SoScale.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/nodes/SoTranslation.H"
#include "inv/projectors/SbSphereSheetProjector.H"
#include "inv/actions/SoSearchAction.H"

#include "inv/SoWx/SoWxExaminerViewer.H"
#include "inv/SoWx/SoWx.H"
#include "inv/SoWx/SoWxRenderArea.H"

#include <iostream>
using std::cerr;
using std::endl;

IMPLEMENT_DYNAMIC_CLASS( SoWxExaminerViewer, wxPanel )

BEGIN_EVENT_TABLE( SoWxExaminerViewer, wxPanel )
END_EVENT_TABLE()


#define ROT_BUFF_SIZE 3


// The point of interest geometry description
const char *SoWxExaminerViewer::p_geometryBuffer = "\
#Inventor V2.0 ascii\n\
\
Separator { \
    PickStyle { style UNPICKABLE } \
    LightModel { model BASE_COLOR } \
    MaterialBinding { value PER_PART } \
    DrawStyle { lineWidth 2 } \
    Coordinate3 { point [0 0 0, 1 0 0, 0 1 0, 0 0 1] } \
    BaseColor { rgb [1 0 0, 0 1 0, 0 0 1] } \
    IndexedLineSet { coordIndex [1, 0, 2, -1, 0, 3] } \
     \
    LightModel { model PHONG } \
    MaterialBinding { value OVERALL } \
    Complexity { value .1 } \
    Separator { \
    	Material { \
	    diffuseColor    [ 0.5 0 0 ] \
	    emissiveColor   [ 0.5 0 0 ] \
	} \
	Translation { translation 1 0 0 } \
    	RotationXYZ { axis Z angle -1.570796327 } \
    	Cone { bottomRadius .2 height .3 } \
    } \
    Separator { \
    	Material { \
	    diffuseColor    [ 0 0.5 0 ] \
	    emissiveColor   [ 0 0.5 0 ] \
	} \
	Translation { translation 0 1 0 } \
    	Cone { bottomRadius .2 height .3 } \
    } \
    Material { \
	diffuseColor    [ 0 0 0.5 ] \
	emissiveColor   [ 0 0 0.5 ] \
    } \
    Translation { translation 0 0 1 } \
    RotationXYZ { axis X angle 1.570796327 } \
    Cone { bottomRadius .2 height .3 } \
} ";


SoWxExaminerViewer::SoWxExaminerViewer( )
{
}


SoWxExaminerViewer::SoWxExaminerViewer( wxWindow* parent,
                                        wxWindowID id,
                                        const wxPoint& pos,
                                        const wxSize& size,
                                        long style,
                                        SoWxViewer::Type type ):
  SoWxViewer( parent, id , pos, size, style, type )
{
  constructorCommon();
}


SoWxExaminerViewer::~SoWxExaminerViewer()
{
  delete p_animationSensor;
  
  //  for (int i=0; i<PUSH_NUM; i++)
  //    delete buttonList[i];
  
  delete p_sphereSheet;
  if (p_feedbackRoot)
    p_feedbackRoot->unref();
  
  // @todo Maybe wxCursor?
  // free the viewer cursors
  //   if (getDisplay()) {
  //     Display *display = getDisplay();
  //     if (spinCursor) XFreeCursor(display, spinCursor);
  //     if (panCursor) XFreeCursor(display, panCursor);
  //     if (dollyCursor) XFreeCursor(display, dollyCursor);
  //     if (seekCursor) XFreeCursor(display, seekCursor);
  //   }
  
  delete [] p_rotBuffer;
}


/**
 * Called by the constructors
 */
void SoWxExaminerViewer::constructorCommon()
{
  // init local vars

  p_cursorEnabledFlag = true;
  p_bottomWheelVal = 0;
  p_leftWheelVal = 0;
  p_rightWheelVal = 0;
  
  
  // @todo
  // addVisibilityChangeCallback(visibilityChangeCB, this);
  p_mode = isViewing() ? VIEW_MODE : PICK_MODE;
  // No need to create a list of cursor
  //  p_createdCursors = FALSE;
  p_spinCursor = p_panCursor = p_dollyCursor = p_seekCursor = 0;
  p_firstBuild = false;
  // @todo
  //  setSize( SbVec2s(500, 390) );  // default size
  //  setClassName(thisClassName);
  
  // axis of rotation feedback vars
  p_feedbackFlag = FALSE;
  p_feedbackRoot = NULL;
  p_feedbackSwitch = NULL;
  p_feedbackSize = 20.0;
  // @todo
  //  p_feedbackSizeWheel = NULL;
  
  // init animation variables
  p_animationEnabled = TRUE;
  p_animatingFlag = FALSE;
  p_rotBuffer = new SbRotation[ROT_BUFF_SIZE];
  p_lastMotionTime = 0;
  p_animationSensor =
    new SoFieldSensor(SoWxExaminerViewer::animationSensorCB, this);
#ifdef DEBUG
  if (SoDebug::GetEnv("IV_DEBUG_SENSORS")) {
    SoDebug::NamePtr("examinerSpinSensor", animationSensor);
  }
#endif
  
  // init the projector class
  SbViewVolume vv;
  vv.ortho(-1, 1, -1, 1, -10, 10);
  p_sphereSheet = new SbSphereSheetProjector;
  p_sphereSheet->setViewVolume( vv );
  p_sphereSheet->setSphere( SbSphere( SbVec3f(0, 0, 0), .7) );
  
  // @todo
  // Initialize buttonList.    
  //  for (int i=0; i<PUSH_NUM; i++)
  //    buttonList[i] = NULL;
  
  // Build the widget tree, and let SoXtComponent know about our base widget.
  //  if (buildNow) {
  //    Widget w = buildWidget(getParentWidget());
  //    setBaseWidget(w);
  //  }
}


/**
 * Shows/hide the point of rotation feedback geometry
 */
void SoWxExaminerViewer::setFeedbackVisibility(SbBool insertFlag)
{
  // check for trivial return
  if (p_camera == NULL || p_feedbackFlag == insertFlag) {
    p_feedbackFlag = insertFlag;
    return;
  }
  
  // find the camera parent to insert/remove the feedback root
  SoSearchAction sa;
  if (insertFlag)
    sa.setNode(p_camera);
  else {
    sa.setNode(p_feedbackRoot);
    sa.setSearchingAll(true); // find under OFF switches for removal
  }
  sa.apply(p_sceneRoot);
  SoFullPath *fullPath = (SoFullPath *) sa.getPath();
  if (!fullPath) {
#if DEBUG
    SoDebugError::post("SoXtExaminerViewer::setFeedbackVisibility",
                       insertFlag ? "ERROR: cannot find camera in graph" :
                       "ERROR: cannot find axis feedback in graph");
#endif
    return;
  }
  SoGroup *parent = (SoGroup *) fullPath->getNodeFromTail(1);
  
  p_feedbackFlag = insertFlag;
  
  // make sure the feedback has been built
  if (!p_feedbackRoot)
    createFeedbackNodes();
  
  // inserts/remove the feedback axis group
  
  if (p_feedbackFlag) {
    int camIndex;
    
    // check to make sure that the camera parent is not a switch node
    // (VRML camera viewpoints are kept under a switch node). Otherwise
    // we will insert the feedback after the switch node.
    if (parent->isOfType(SoSwitch::getClassTypeId())) {
      SoNode *switchNode = parent;
      parent = (SoGroup *) fullPath->getNodeFromTail(2);
      camIndex = parent->findChild(switchNode);
    }
    else
      camIndex = parent->findChild(p_camera);
    
    // return if feedback is already there (this should be an error !)
    if (parent->findChild(p_feedbackRoot) >= 0)
      return;
    
    // insert the feedback right after the camera+headlight (index+2)
    if (camIndex >= 0) {
      if (isHeadlight())
        parent->insertChild(p_feedbackRoot, camIndex+2);
      else
        parent->insertChild(p_feedbackRoot, camIndex+1);
    }
    
    // make sure the feedback switch is turned to the correct state now
    // that the feedback root has been inserted in the scene
    p_feedbackSwitch->whichChild.setValue(p_viewingFlag ? SO_SWITCH_ALL : SO_SWITCH_NONE);
  }
  else {
    if (parent->findChild(p_feedbackRoot) >= 0) {
      parent->removeChild(p_feedbackRoot);
    }
  }
}


SbBool SoWxExaminerViewer::isFeedbackVisible() const
{
  return p_feedbackFlag;
}


/**
 * Set the point of rotation feeedback size in pixels (default 20).
 */
void SoWxExaminerViewer::setFeedbackSize(int newSize)
{
  if (p_feedbackSize == newSize)
    return;
  
  // assign new value and redraw (since it is not a field in the scene)
  p_feedbackSize = newSize;
  if (isFeedbackVisible() && isViewing())
    p_renderArea->scheduleRedraw();
}


/**
 * Get the point of rotation feeedback size in pixels (default 20).
 */
int SoWxExaminerViewer::getFeedbackSize() const
{
  return (int) p_feedbackSize;
}


/**
 * Enable/disable the animation feature of the Examiner
 */
void SoWxExaminerViewer::setAnimationEnabled(SbBool flag)
{
  if (p_animationEnabled == flag)
    return;
  
  p_animationEnabled = flag;
  if ( !p_animationEnabled && isAnimating())
    stopAnimating();
}


SbBool SoWxExaminerViewer::isAnimationEnabled()
{
  return p_animationEnabled;
}


/**
 * Stops ongoing animation (if any)
 */
void SoWxExaminerViewer::stopAnimating()
{
  if (p_animatingFlag) {
    p_animatingFlag = false;
    p_animationSensor->detach();
    p_animationSensor->unschedule();
    interactiveCountDec();
  }
}


SbBool SoWxExaminerViewer::isAnimating()
{
  return p_animatingFlag;
}


/**
 * Remove the extra geometry when doing a viewAll.
 */
void SoWxExaminerViewer::viewAll()
{
  // stop spinning
  if ( isAnimating() )
    stopAnimating();
  
  // temporarily remove the feedback geometry
  if (p_feedbackFlag && isViewing() && p_feedbackSwitch)
    p_feedbackSwitch->whichChild.setValue( SO_SWITCH_NONE );
  
  // call the base class
  SoWxViewer::viewAll();
  
  // now add the geometry back in
  if (p_feedbackFlag && isViewing() && p_feedbackSwitch)
    p_feedbackSwitch->whichChild.setValue( SO_SWITCH_ALL );
}


/**
 * Call the parent class and stop animation if any
 */
void SoWxExaminerViewer::resetToHomePosition()
{
  // stop spinning
  if ( isAnimating() )
    stopAnimating();
  
  // call the base class
  SoWxViewer::resetToHomePosition();
}


/**
 * Call the parent class and insert/remove the feedback root
 */
void SoWxExaminerViewer::setCamera(SoCamera *newCamera)
{
  if (p_camera == newCamera)
    return;
  
  // set the right thumbwheel label and toggle button image based on 
  // the camera type
  if (newCamera != NULL && (p_camera == NULL || 
                            newCamera->getTypeId() != p_camera->getTypeId())) {
    if (newCamera->isOfType(SoOrthographicCamera::getClassTypeId())) {
      /*
        @todo
      if (buttonList[CAM_PUSH])
        buttonList[CAM_PUSH]->setIcon(so_xt_ortho_bits, 
                                      so_xt_icon_width, so_xt_icon_height);
      setRightWheelString(rl.zoom);
      */
    }
    else {
      /*
        @todo
      if (buttonList[CAM_PUSH])
        buttonList[CAM_PUSH]->setIcon(so_xt_persp_bits, 
                                      so_xt_icon_width, so_xt_icon_height);
      setRightWheelString(rl.dolly);
      */
    }
  }
  
  // detach feedback which depends on camera
  if ( p_feedbackFlag ) {
    setFeedbackVisibility(false);
    p_feedbackFlag = true;  // can later be turned on
  }
  
  // call parent class
  SoWxViewer::setCamera(newCamera);
  
  // attach feedback back on
  if ( p_feedbackFlag ) {
    p_feedbackFlag = false; // enables routine to be called
    setFeedbackVisibility(true);
  }
}


/**
 * Call the base class and sets the correct cursors on the window,
 * plus insert/remove the feedback geometry.
 */
void SoWxExaminerViewer::setViewing(SbBool flag)
{
  if (flag == p_viewingFlag)
    return;
  
  // call the parent class
  SoWxViewer::setViewing(flag);
  
  switchMode(isViewing() ? VIEW_MODE : PICK_MODE);
  
  // show/hide the feedback geometry based on the viewing state
  if (p_feedbackFlag && p_feedbackSwitch)
    p_feedbackSwitch->whichChild.setValue(p_viewingFlag ? 
                                          SO_SWITCH_ALL : SO_SWITCH_NONE);
}


/**
 * Enables/Disable the viewer cursor on the window.
 */
void SoWxExaminerViewer::setCursorEnabled(SbBool flag)
{
  if (flag == p_cursorEnabledFlag)
    return;
  
  p_cursorEnabledFlag = flag;
  
  if (! isViewing())
    return;
  
  updateCursor();
}


/**
 * Process the given event to do viewing stuff
 */
void SoWxExaminerViewer::processEvent( wxEvent * event )
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxExaminerViewer::processEvent\n";
#endif

  if ( processCommonEvents(event) )
    return;

  SbVec2s raSize = getGlxSize();

  wxMouseEvent * mouseEvent = dynamic_cast<wxMouseEvent *>(event);
  if (mouseEvent != 0) {
    int x = mouseEvent->GetX();
    int y = mouseEvent->GetY();
    int timeStamp = mouseEvent->GetTimestamp();

    if (mouseEvent->ButtonDown() || mouseEvent->ButtonUp()) {
      if (mouseEvent->Button(wxMOUSE_BTN_LEFT) ||
          mouseEvent->Button(wxMOUSE_BTN_MIDDLE)) {
        p_locator[0] = x;
        // @todo Not sure if this is still correct
        p_locator[1] = raSize[1] - y;
        
        if (p_mode == SEEK_MODE) {
          if (mouseEvent->ButtonDown())
            seekToPoint(p_locator);
        }
        else {
          if (mouseEvent->ButtonDown())
            interactiveCountInc();
          else { //... ButtonRelease
            
            // check if we need to start spinning
            if (p_mode == SPIN_MODE_ACTIVE && p_animationEnabled
                && p_lastMotionTime == timeStamp) {
              p_animatingFlag = true;
              p_computeAverage = true;
              p_animationSensor->attach(p_viewerRealTime);
              interactiveCountInc();
            }
            
            interactiveCountDec();
          }
          updateViewerMode(mouseEvent);
        }
        
        if (mouseEvent->ButtonDown())
          stopAnimating();
      }
    }
    else if (mouseEvent->Moving() || mouseEvent->Dragging()) {
      switch (p_mode) {
      case SPIN_MODE_ACTIVE:
        p_lastMotionTime = timeStamp;
        spinCamera(SbVec2f(x/float(raSize[0]),
                           (raSize[1] - y)/float(raSize[1])));
        break;
      case PAN_MODE_ACTIVE:
        panCamera(SbVec2f(x/float(raSize[0]),
                          (raSize[1] - y)/float(raSize[1])));
        break;
      case DOLLY_MODE_ACTIVE:
        dollyCamera( SbVec2s(x, raSize[1] - y) );
        break;
      }
    }
  }
  else {
    wxKeyEvent * keyEvent = dynamic_cast<wxKeyEvent *>(event);
    if (keyEvent != 0) {
      if (keyEvent->GetKeyCode() == WXK_CONTROL && !keyEvent->ControlDown()) {
        switchMode(PAN_MODE);
      }
      else if (keyEvent->GetKeyCode() == WXK_CONTROL && keyEvent->ControlDown()) {
        switchMode(VIEW_MODE);
      }
    }
  }
  

#ifdef SOWXDEBUG
  cerr << "Leave SoWxExaminerViewer::processEvent\n";
#endif
  // @todo This wasn't needed. Some callback function is not working!
  //  Refresh(false);
}


/**
 * Call the base class and sets the correct cursors on the window
 */
void SoWxExaminerViewer::setSeekMode(SbBool flag)
{
  if ( !isViewing() )
    return;
  
  // stop spinning
  if (isAnimating())
    stopAnimating();
  
  // call the base class
  SoWxViewer::setSeekMode(flag);
  
  p_mode = isSeekMode() ? SEEK_MODE : VIEW_MODE;
  
  updateCursor();
}


/**
 * Draws viewer feedback during a render area redraw of the scene.
 */
void SoWxExaminerViewer::actualRedraw()
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxExaminerViewer::actualRedraw()\n";
#endif

  // place the feedback at the focal point
  // ??? we really only need to do this when the camera changes
  if (isViewing() && p_feedbackFlag && p_camera != NULL && p_feedbackRoot) {
    
    // adjust the position to be at the focal point
    SbMatrix mx;
    mx = p_camera->orientation.getValue();
    SbVec3f forward(-mx[2][0], -mx[2][1], -mx[2][2]);
    p_feedbackTransNode->translation = p_camera->position.getValue() + 
      p_camera->focalDistance.getValue() * forward;
    
    // adjust the size to be constant on the screen
    float height;
    if (p_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
      float angle = ((SoPerspectiveCamera *)p_camera)->heightAngle.getValue();
      height = p_camera->focalDistance.getValue() * tanf(angle/2);
    }
    else if (p_camera->isOfType(SoOrthographicCamera::getClassTypeId()))
      height = ((SoOrthographicCamera *)p_camera)->height.getValue() / 2;
    
    // ??? getGlxSize[1] == 0 the very first time, so return in that case
    // ??? else the redraws are 3 times slower from now on !! (alain)
    if (getGlxSize()[1] != 0) {
      float size = 2.0 * height * p_feedbackSize / float (getGlxSize()[1]);
      p_feedbackScaleNode->scaleFactor.setValue(size, size, size);
    }
  }
  
  // have the base class draw the scene
  SoWxViewer::actualRedraw();

#ifdef SOWXDEBUG
  cerr << "Leave SoWxExaminerViewer::actualRedraw()\n";
#endif
}


/**
 * Rotates the object around the screen x axis (called by thumb wheel).
 */
void SoWxExaminerViewer::bottomWheelMotion(float newVal)
{
  // get rotation and apply to camera
  SbVec3f axis(0, 1, 0);
  SbRotation rot(axis, p_bottomWheelVal - newVal);
  rotateCamera(rot);
  
  p_bottomWheelVal = newVal;
}


/**
 * Rotates the object around the screen y axis (called by thumb wheel).
 */
void SoWxExaminerViewer::leftWheelMotion(float newVal)
{
  // get rotation and apply to camera
  SbVec3f axis(1, 0, 0);
  SbRotation rot(axis, newVal - p_leftWheelVal);
  rotateCamera(rot);
  
  p_leftWheelVal = newVal;
}


/**
 * Moves the camera closer/further away from the plane of interest
 * (perspective camera case), else change the camera height (orthographic
 * camera case).
 */
void SoWxExaminerViewer::rightWheelMotion(float newVal)
{
  if (p_camera == NULL)
    return;
  
  if (p_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
    // change the ortho camera height
    SoOrthographicCamera *cam = (SoOrthographicCamera *) p_camera;
    cam->height = cam->height.getValue() * powf(2.0, newVal - p_rightWheelVal);
  }
  else {
    // shorter/grow the focal distance given the wheel rotation
    float focalDistance = p_camera->focalDistance.getValue();
    float newFocalDist = focalDistance;
    newFocalDist *= powf(2.0, newVal - p_rightWheelVal);
    
    // finally reposition the camera
    SbMatrix mx;
    mx = p_camera->orientation.getValue();
    SbVec3f forward(-mx[2][0], -mx[2][1], -mx[2][2]);
    p_camera->position = p_camera->position.getValue() + 
      (focalDistance - newFocalDist) * forward;
    p_camera->focalDistance = newFocalDist;
  }
  
  p_rightWheelVal = newVal;
}


/**
 * Call the base class and stop animating
 */
void SoWxExaminerViewer::bottomWheelStart()
{
  // @todo
  //  SoXtFullViewer::bottomWheelStart();
  stopAnimating();
}


/**
 * Call the base class and stop animating
 */
void SoWxExaminerViewer::leftWheelStart()
{
  // @todo
  //  SoXtFullViewer::bottomWheelStart();
  stopAnimating();
}


/**
 * Redefine this routine to add some viewer specific stuff.
 */
void SoWxExaminerViewer::createPrefSheet()
{
  /*
    @todo
  // create the preference sheet shell and form widget
  Widget shell, form;
  createPrefSheetShellAndForm(shell, form);
  
  // create all of the parts
  Widget widgetList[20];
  int num = 0;
  createDefaultPrefSheetParts(widgetList, num, form);
  widgetList[num++] = createExamPrefSheetGuts(form);
  
  layoutPartsAndMapPrefSheet(widgetList, num, form, shell);
  */
}


/**
 * add our own button to the existing list
 */
void SoWxExaminerViewer::createViewerButtons(wxWindow * parent)
{
  /*
    @todo
  // get the default buttons
  SoXtFullViewer::createViewerButtons(parent);
  
  // allocate our buttons
  buttonList[CAM_PUSH] = new SoXtBitmapButton(parent, FALSE);
  buttonList[CAM_PUSH]->setIcon(so_xt_persp_bits, so_xt_icon_width, so_xt_icon_height);
  Widget w = buttonList[CAM_PUSH]->getWidget();
  XtAddCallback(w, XmNactivateCallback,
                (XtCallbackProc) SoXtExaminerViewer::camPushCB, (XtPointer) this);
  
  // add this button to the list...
  viewerButtonWidgets->append(w);
  */
}


/**
 * Brings the viewer help card (called by "?" push button)
 */
void SoWxExaminerViewer::openViewerHelpCard()
{
  // @todo
  // tell the base class to open the file for us
  //  openHelpCard("SoXtExaminerViewer.help");
}


/**
 * read the point of interest geometry, which will be placed
 * right after the camera node (or right after the headlight
 * if the headlight is turned on).
 */
void SoWxExaminerViewer::createFeedbackNodes()
{
  // make sure we havn't built this yet...
  if (p_feedbackRoot)
    return;
  
  p_feedbackRoot = new SoSeparator(1);
  p_feedbackSwitch = new SoSwitch(3);
  p_feedbackTransNode = new SoTranslation;
  p_feedbackScaleNode = new SoScale;
  p_feedbackRoot->ref();
  p_feedbackRoot->addChild( p_feedbackSwitch );
  p_feedbackSwitch->addChild( p_feedbackTransNode );
  p_feedbackSwitch->addChild( p_feedbackScaleNode );
  SoInput in;
  in.setBuffer((void *)p_geometryBuffer, (size_t) strlen(p_geometryBuffer));
  SoNode *node;
  SbBool ok = SoDB::read(&in, node);
  if (ok && node != NULL)
    p_feedbackSwitch->addChild(node);
#ifdef DEBUG
  else
    SoDebugError::post("SoXtExaminerViewer::createFeedbackNodes",
                       "couldn't read feedback axis geometry");
#endif
}


void SoWxExaminerViewer::animationSensorCB(void *v, SoSensor *)
{
  ((SoWxExaminerViewer *) v)->doSpinAnimation();
}


/**
 * called when the viewer becomes visible/hidden - when hidden, make
 * sure to temporary stop any ongoing animation (and restart it as soon
 * as we become visible).
 * @todo
 */
void SoWxExaminerViewer::visibilityChangeCB(void *pt, SbBool visible)
{
  SoWxExaminerViewer *p = (SoWxExaminerViewer *)pt;
  
  // only do this if we are/were spinning....
  if (! p->p_animatingFlag)
    return;
  
  if (visible) {
    // we now are visible again so attach the field sensor
    p->p_animationSensor->attach(p_viewerRealTime);
  }
  else {
    // if hidden, detach the field sensor, but don't change the
    // animatingFlag var to let us know we need to turn it back on
    // when we become visible....
    p->p_animationSensor->detach();
    p->p_animationSensor->unschedule();
  }
}


/**
 * Routine which animates the ball spinning (called by sensor).
 */
void SoWxExaminerViewer::doSpinAnimation()
{
#ifdef SOWXDEBUG
  cerr << "In SoWxExaminerViewer::doSpinAnimation\n";
#endif

  // check if average rotation needs to be computed
  
  if (p_computeAverage) {
    float averageAngle, angle;
    SbVec3f averageAxis, axis;
    
    // get number of samples
    int num = (((p_lastIndex - p_firstIndex) + 1 + 
                ROT_BUFF_SIZE) % ROT_BUFF_SIZE);
    
    // check for not enough samples
    if (num < 2) {
      stopAnimating();
      return;
    }
    
    // get average axis of rotation
    // ??? right now only take one sample
    p_rotBuffer[p_firstIndex].getValue(averageAxis, angle);
    
    // get average angle of rotation
    averageAngle = 0;
    for (int i=0; i<num; i++) {
      int n = (p_firstIndex + i) % ROT_BUFF_SIZE;
      p_rotBuffer[n].getValue(axis, angle);
      averageAngle += angle;
    }
    averageAngle /= float(num);
    
    p_averageRotation.setValue(averageAxis, averageAngle);
    p_computeAverage = false;
  }
  
  // rotate camera by average rotation
  rotateCamera(p_averageRotation);
}


/**
 * sets the viewer mode based on what keys and buttons are being pressed
 */
void SoWxExaminerViewer::updateViewerMode(wxMouseEvent * event)
{
  // ??? WARNING - this routine ONLY works because of 
  // ??? SoXtViewer::updateEventState() which is called for us
  // ??? by SoXtViewer::processCommonEvents(). 
  // ??? (XEvent state for button and modifier keys is not updated
  // ??? until after the event is received. WEIRD)
  
  // LEFT+MIDDLE down
  if ((event->LeftDown() && event->MiddleIsDown()) ||
      (event->LeftIsDown() && event->MiddleDown())) {
    switchMode(DOLLY_MODE_ACTIVE);
  }
  
  // LEFT down
  else if (event->LeftDown()) {
    if (event->ControlDown())
      switchMode(PAN_MODE_ACTIVE);
    else
      switchMode(SPIN_MODE_ACTIVE);
  }
  
  // MIDDLE DOWN
  else if (event->MiddleDown()) {
    if (event->ControlDown())
      switchMode(DOLLY_MODE_ACTIVE);
    else
      switchMode(PAN_MODE_ACTIVE);
  }
  
  // no buttons down... Button up?
  else {
    if (event->ControlDown())
      switchMode(PAN_MODE);
    else
      switchMode(VIEW_MODE);
  }
}


/**
 * switches to the specified viewer mode
 */
void SoWxExaminerViewer::switchMode(int newMode)
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxExaminerViewer::switchMode( " << newMode << " )\n";
#endif

  SbVec2s raSize = getGlxSize();
  
  // assign new mode
  int prevMode = p_mode;
  p_mode = newMode;
  
  // update the cursor
  updateCursor();

  // switch to new viewer mode
  switch (newMode) {
  case PICK_MODE:
    if (p_renderArea) {
      // ???? is if are going into PICK mode and some of our
      // mouse buttons are still down, make sure to decrement
      // interactive count correctly (correct draw style). One
      // for the LEFT and one for MIDDLE mouse.

      if ((wxGetMouseState().LeftDown()||wxGetMouseState().MiddleDown())
          && prevMode != SEEK_MODE)
        interactiveCountDec();
    }
    stopAnimating();
    break;
    
  case SPIN_MODE_ACTIVE:
    // set the sphere sheet starting point
    p_sphereSheet->project(SbVec2f(p_locator[0]/float(raSize[0]),
                                   p_locator[1]/float(raSize[1])) );
    
    // reset the animation queue
    p_firstIndex = 0;
    p_lastIndex = -1;
    break;
    
  case PAN_MODE_ACTIVE:
    // Figure out the focal plane
    SbMatrix mx;
    mx = p_camera->orientation.getValue();
    SbVec3f forward(-mx[2][0], -mx[2][1], -mx[2][2]);
    SbVec3f fp = p_camera->position.getValue() + 
      forward * p_camera->focalDistance.getValue();
    p_focalplane = SbPlane(forward, fp);
    
    // map mouse starting position onto the panning plane
    SbViewVolume    cameraVolume;
    SbLine	    line;
    cameraVolume = p_camera->getViewVolume(raSize[0]/float(raSize[1]));
    cameraVolume.projectPointToLine(SbVec2f(p_locator[0]/float(raSize[0]),
                                            p_locator[1]/float(raSize[1])), line);
    p_focalplane.intersect(line, p_locator3D);
    break;
  }
}


/**
 * updates the viewer cursor
 */
void SoWxExaminerViewer::updateCursor()
{
#ifdef SOWXDEBUG
  cerr << "In updateCursor(): mode is " << p_mode << endl;
#endif

  //  if (! p_createdCursors)
  //    defineCursors();
  
  // the viewer cursor are not enabled, then we don't set a new cursor.
  // Instead erase the old viewer cursor.
  if (! p_cursorEnabledFlag) {
    SetCursor(*wxSTANDARD_CURSOR);
    return;
  }

  // ...else set the right cursor for the viewer mode....
  int cursorId = wxCURSOR_ARROW;
  switch(p_mode) {
  case PICK_MODE:
    cursorId = wxCURSOR_ARROW;
    break;
	    
  case VIEW_MODE:
  case SPIN_MODE_ACTIVE:
    cursorId = wxCURSOR_HAND;
    break;
	    
  case DOLLY_MODE:
  case DOLLY_MODE_ACTIVE:
    cursorId = wxCURSOR_BULLSEYE; //wxCURSOR_MAGNIFIER;
    break;
	    
  case PAN_MODE:
  case PAN_MODE_ACTIVE:
    cursorId = wxCURSOR_SIZENWSE; 
    break;
	    
  case SEEK_MODE:
    cursorId = wxCURSOR_SIZING;
    //    cursorId = wxCURSOR_SIZENESW;
    break;
  }

  p_renderArea->SetCursor(wxCursor(cursorId));

}


/**
 * This routine is used to define cursors (can only be called after
 * window has been realized).
 */
void SoWxExaminerViewer::defineCursors()
{
  /*
  XColor foreground;
  Pixmap source;
  Display *display = getDisplay();
  Drawable d = DefaultRootWindow(display);
  
  // set color
  foreground.red = 65535;
  foreground.green = foreground.blue = 0;
  
  // spin cursor
  source = XCreateBitmapFromData(display, d, 
                                 so_xt_curved_hand_bits, so_xt_curved_hand_width, so_xt_curved_hand_height);
  spinCursor = XCreatePixmapCursor(display, source, source, 
                                   &foreground, &foreground, so_xt_curved_hand_x_hot, so_xt_curved_hand_y_hot);
  XFreePixmap(display, source);
  
  // panning cursor
  source = XCreateBitmapFromData(display, d, 
                                 so_xt_flat_hand_bits, so_xt_flat_hand_width, so_xt_flat_hand_height);
  panCursor = XCreatePixmapCursor(display, source, source, 
                                  &foreground, &foreground, so_xt_flat_hand_x_hot, so_xt_flat_hand_y_hot);
  XFreePixmap(display, source);
  
  // dolly cursor
  source = XCreateBitmapFromData(display, d, 
                                 so_xt_pointing_hand_bits, so_xt_pointing_hand_width, so_xt_pointing_hand_height);
  dollyCursor = XCreatePixmapCursor(display, source, source, 
                                    &foreground, &foreground, so_xt_pointing_hand_x_hot, so_xt_pointing_hand_y_hot);
  XFreePixmap(display, source);
  
  // seek cursor
  source = XCreateBitmapFromData(display, d, 
                                 so_xt_target_bits, so_xt_target_width, so_xt_target_height);
  seekCursor = XCreatePixmapCursor(display, source, source, 
                                   &foreground, &foreground, so_xt_target_x_hot, so_xt_target_y_hot);
  XFreePixmap(display, source);
  
  createdCursors = TRUE;
  */
}


/**
 * Rotates the camera around pt of interest by given rotation
 */
void SoWxExaminerViewer::rotateCamera(const SbRotation &rot)
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxExaminerViewer::rotateCamera\n";
#endif

  if (p_camera == NULL)
    return;
  
  // get center of rotation
  SbRotation camRot = p_camera->orientation.getValue();
  float radius = p_camera->focalDistance.getValue();
  SbMatrix mx;
  mx = camRot;
  SbVec3f forward( -mx[2][0], -mx[2][1], -mx[2][2]);
  SbVec3f center = p_camera->position.getValue()
    + radius * forward;
  
  // apply new rotation to the camera
  camRot = rot * camRot;
  p_camera->orientation = camRot;
  
  // reposition camera to look at pt of interest
  mx = camRot;
  forward.setValue( -mx[2][0], -mx[2][1], -mx[2][2]);
  p_camera->position = center - radius * forward;

#ifdef SOWXDEBUG
  cerr << "Leave SoWxExaminerViewer::rotateCamera\n";
#endif
}


/**
 * Moves the camera into the plane defined by the camera forward vector
 * and the focal point to follow the new mouse location.
 */
void SoWxExaminerViewer::panCamera(const SbVec2f &newLocator)
{
  if (p_camera == NULL)
    return;
  
  // map new mouse location into the camera focal plane
  SbViewVolume cameraVolume;
  SbLine line;
  SbVec3f newLocator3D;
  SbVec2s raSize = getGlxSize();
  cameraVolume = p_camera->getViewVolume(raSize[0]/float(raSize[1]));
  cameraVolume.projectPointToLine(newLocator, line);
  p_focalplane.intersect(line, newLocator3D);
  
  // move the camera by the delta 3D position amount
  p_camera->position = p_camera->position.getValue() + 
    (p_locator3D - newLocator3D);
  
  // You would think we would have to set locator3D to
  // newLocator3D here.  But we don't, because moving the camera
  // essentially makes locator3D equal to newLocator3D in the
  // transformed space, and we will project the next newLocator3D in
  // this transformed space.
}


/**
 * rotates the camera using the sheet sphere projector, given the new
 * mouse location.
 */
void SoWxExaminerViewer::spinCamera(const SbVec2f &newLocator)
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxExaminerViewer::spinCamera\n";
#endif

  // find rotation and rotate camera
  SbRotation rot;
  p_sphereSheet->projectAndGetRotation(newLocator, rot);
  rot.invert();
  
  rotateCamera(rot);
  
  // save rotation for animation
  p_lastIndex = ((p_lastIndex+1) % ROT_BUFF_SIZE);
  p_rotBuffer[p_lastIndex] = rot;
  
  // check if queue is full
  if (((p_lastIndex+1) % ROT_BUFF_SIZE) == p_firstIndex)
    p_firstIndex = ((p_firstIndex+1) % ROT_BUFF_SIZE);

#ifdef SOWXDEBUG
  cerr << "Leave SoWxExaminerViewer::spinCamera\n";
#endif
}


/**
 * Moves the camera forward/backward based on the new mouse position.
 * (perspective camera), else change the camera height (orthographic
 * camera case).
 */
void SoWxExaminerViewer::dollyCamera(const SbVec2s &newLocator)
{
  if (p_camera == NULL)
    return;
  
  // moving the mouse up/down will move the camera futher/closer.
  // moving the camera sideway will not move the camera at all
  float d = (newLocator[1] - p_locator[1]) / 40.0;
  
  if (p_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
    // change the ortho camera height
    SoOrthographicCamera *cam = (SoOrthographicCamera *) p_camera;
    cam->height = cam->height.getValue() * powf(2.0, d);
  }
  else {
    // shorter/grow the focal distance given the mouse move
    float focalDistance = p_camera->focalDistance.getValue();;
    float newFocalDist = focalDistance * powf(2.0, d);
    
    // finally reposition the camera
    SbMatrix mx;
    mx = p_camera->orientation.getValue();
    SbVec3f forward(-mx[2][0], -mx[2][1], -mx[2][2]);
    p_camera->position = p_camera->position.getValue() + 
      (focalDistance - newFocalDist) * forward;
    p_camera->focalDistance = newFocalDist;
  }
  
  p_locator = newLocator;
}
