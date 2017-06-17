#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "SoWxViewer.h"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "inv/SoType.H"
#include "inv/SbBox.H"
#include "inv/SbLinear.H"
#include "inv/SbPList.H"
#include "inv/SbViewportRegion.H"
#include "inv/SoDB.H"
#include "inv/SoPath.H"
#include "inv/SoPickedPoint.H"
#include "inv/actions/SoGetBoundingBoxAction.H"
#include "inv/actions/SoRayPickAction.H"
#include "inv/actions/SoSearchAction.H"
#include "inv/errors/SoDebugError.H"
#include "inv/nodes/SoPackedColor.H"
#include "inv/nodes/SoMaterialBinding.H"
#include "inv/nodes/SoComplexity.H"
#include "inv/nodes/SoDirectionalLight.H"
#include "inv/nodes/SoDrawStyle.H"
#include "inv/nodes/SoGroup.H"
#include "inv/nodes/SoLightModel.H"
#include "inv/nodes/SoOrthographicCamera.H"
#include "inv/nodes/SoPerspectiveCamera.H"
#include "inv/nodes/SoResetTransform.H"
#include "inv/nodes/SoRotation.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/nodes/SoLocateHighlight.H"
#include "inv/sensors/SoFieldSensor.H"
#include "inv/fields/SoSFTime.H"
#include "inv/misc/SoCallbackList.H"

#include "inv/SoWx/SoWx.H"
#include "inv/SoWx/SoWxRenderArea.H"
#include "inv/SoWx/SoWxInputFocus.H"
#include "inv/SoWx/SoWxViewer.H"

#include <iostream>
using std::cerr;

IMPLEMENT_DYNAMIC_CLASS( SoWxViewer, wxPanel )


BEGIN_EVENT_TABLE( SoWxViewer, wxPanel )
END_EVENT_TABLE()
;


/*
 * Defines
 */
// color used in feedback
#define DARK_COLOR	glColor3ub(90, 90, 90)
#define LIGHT_COLOR	glColor3ub(230, 230, 230)

#define LINE_THIN   3	// line thickness used in feedback
#define	LINE_THICK  (LINE_THIN + 2)
#define CROSS 	    8	// size of cross hair at screen center for Roll
#define RADIUS	    15	// radius of center circle (in pix) for Roll
#define ANGLE_LEN   14   // angular size in degrees of Roll anchor


/*
 * Globals
 */
#define ARROW_SIZE  6.0	// size in pix of arrow head

// anchor arrow head description
static float arrow_data[3][3] = {
  {-ARROW_SIZE, 0, 0},
  {0, 2*ARROW_SIZE, 0},
  {ARROW_SIZE, 0, 0}
};


/*
 * Macros
 */
#define	DRAW_ARROW_MACRO    \
    DARK_COLOR;	\
    glBegin(GL_POLYGON);    \
    glVertex3fv(arrow_data[0]);	\
    glVertex3fv(arrow_data[1]);	\
    glVertex3fv(arrow_data[2]);	\
    glEnd();	\
    LIGHT_COLOR;	\
    glLineWidth(1); \
    glBegin(GL_LINE_LOOP);	\
    glVertex3fv(arrow_data[0]);	\
    glVertex3fv(arrow_data[1]);	\
    glVertex3fv(arrow_data[2]);	\
    glEnd();


// keep a pointer to global time, since we are going to access it a lot.
SoSFTime *SoWxViewer::p_viewerRealTime = NULL;


/*!
 * SoWxViewer constructors
 */
SoWxViewer::SoWxViewer( )
{
}


SoWxViewer::SoWxViewer( wxWindow* parent,
                        wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        long style,
                        SoWxViewer::Type type )
{
  Create(parent, id, pos, size, style, type);
}


/*!
 * SoWxViewer destructors
 */
SoWxViewer::~SoWxViewer( )
{
  // detach everything
  if ( p_sceneGraph != NULL )
    setSceneGraph(NULL);
  p_sceneRoot->unref();

  // delete everything
  delete p_inputFocus;
  delete p_seekAnimationSensor;
  ////////////////////////////
  //  delete p_clipboard;
  delete p_autoClipBboxAction;
  delete p_startCBList;
  delete p_finishCBList;
  p_headlightGroup->unref();
}


/*!
 * SoWxViewer creator
 */
SbBool SoWxViewer::Create( wxWindow* parent,
                           wxWindowID id,
                           const wxPoint& pos,
                           const wxSize& size,
                           long style,
                           SoWxViewer::Type type )
{
  // init local vars
  p_topSizer = NULL;
  p_leftRowSizer = NULL;
  p_centerSizer = NULL;
  p_bottomRowSizer = NULL;
  p_rightRowSizer = NULL;
  p_renderArea = NULL;

  wxPanel::Create( parent, id, pos, size, style );

  CreateControls();

  p_type = type;
  p_camera = NULL;
  p_cameraType = SoPerspectiveCamera::getClassTypeId();
  p_createdCamera = false;
  p_viewingFlag = true;
  p_altSwitchBack = false;
  p_cursorEnabledFlag = true;
  p_interactiveFlag = false;
  p_startCBList = new SoCallbackList;
  p_finishCBList = new SoCallbackList;
  p_interactiveCount = 0;
  p_bufferType = isDoubleBuffer() ? BUFFER_DOUBLE : BUFFER_SINGLE;
  p_stereoOffset = 3.0;
#ifdef __sgi
  p_useSGIStereoExt = false;
#endif
  p_sceneSize = 0.0;	// not computed yet.
  p_viewerSpeed = 1.0;	// default. SoXtFullViewer add UI to increase/decrease 

  // add Enter and Leave notify events for the viewers
  p_inputFocus = new SoWxInputFocus;
  registerDevice(p_inputFocus);
  
  if (! p_viewerRealTime)
    p_viewerRealTime = (SoSFTime *) SoDB::getGlobalField("realTime");
  
  // init auto clipping stuff
  p_autoClipFlag = true;
  p_minimumNearPlane = 0.001;
  p_autoClipBboxAction = new SoGetBoundingBoxAction(SbVec2s(1,1));
  
  // copy/paste support
  /////////////////////////////////////////////
  // @todo Need to write SoWxClipboard class
  //  p_clipboard = NULL;
  
  // init seek animation variables
  p_seekDistance = 50.0;
  p_seekDistAsPercentage = TRUE;
  p_seekModeFlag = FALSE;
  p_detailSeekFlag = TRUE;
  p_seekAnimTime = 2.0;
  p_seekAnimationSensor = 
    new SoFieldSensor(SoWxViewer::seekAnimationSensorCB, this);
  
  // build the small internal graph (nodes used for draw style stuff)
  p_sceneRoot = new SoSeparator(4);
  p_drawStyleSwitch = new SoSwitch(6);
  p_drawStyleNode = new SoDrawStyle;
  p_lightModelNode = new SoLightModel;
  p_colorNode = new SoPackedColor;
  p_matBindingNode = new SoMaterialBinding;
  p_complexityNode = new SoComplexity;
  p_sceneGraph = NULL;
  
  // note: we cannot setSceneGraph on the renderArea in the constructor
  // since it calls virtual functions, and all of our members aren't
  // initialized yet. We'll call it the first time our setSceneGraph
  // is called.
  p_sceneRoot->ref();
  p_sceneRoot->renderCaching.setValue(SoSeparator::OFF); // no caching there
  p_sceneRoot->renderCulling.setValue(SoSeparator::OFF); // no culling there
  p_sceneRoot->addChild(p_drawStyleSwitch);
  p_drawStyleSwitch->addChild(p_drawStyleNode);
  p_drawStyleSwitch->addChild(p_lightModelNode);
  p_drawStyleSwitch->addChild(p_colorNode);
  p_drawStyleSwitch->addChild(p_matBindingNode);
  p_drawStyleSwitch->addChild(p_complexityNode);

  // set the draw style vars and fields that don't change - once we
  // have a context, will will use glGetString() to pick a better default
  // draw style.
  p_stillDrawStyle = VIEW_AS_IS;
  p_interactiveDrawStyle = VIEW_SAME_AS_STILL;
  p_checkForDrawStyle = TRUE;
  p_drawStyleSwitch->whichChild = SO_SWITCH_NONE;
    
  p_drawStyleNode->setOverride(true); // only use style field
  p_drawStyleNode->pointSize = 3.0;
  p_drawStyleNode->lineWidth.setIgnored(true);
  p_drawStyleNode->linePattern.setIgnored(true);
    
  p_lightModelNode->setOverride(true);
    
  p_colorNode->setOverride(true);
    
  p_matBindingNode->setOverride(true);
  p_matBindingNode->value = SoMaterialBinding::OVERALL;
    
  p_complexityNode->setOverride(true);
  p_complexityNode->textureQuality = 0; // always turn texture off under switch
  p_complexityNode->value = 0.15;

  addStartCallback(SoWxViewer::drawStyleStartCallback);
  addFinishCallback(SoWxViewer::drawStyleFinishCallback);

  // headlightGroup - we have a rotation which keeps the headlight
  // moving whenever the camera moves,  and a reset xform so
  // that the rest of the scene is not affected by the first rot.
  // these leaves the direction field in the headlight open for the
  // user to edit, allowing for the direction to change w.r.t. the camera.
  p_headlightGroup = new SoGroup(3);
  p_headlightRot = new SoRotation;
  p_headlightNode = new SoDirectionalLight;
  p_headlightGroup->ref();
  p_headlightGroup->addChild(p_headlightRot);
  p_headlightGroup->addChild(p_headlightNode);
  p_headlightGroup->addChild(new SoResetTransform);
  p_headlightNode->direction.setValue(SbVec3f(.2, -.2, -.9797958971));
  p_headlightFlag = true;

  return true;
}


/*!
 * Customized Control creation for SoWxViewer
 */
void SoWxViewer::CreateControls()
{
  p_topSizer = new wxBoxSizer(wxHORIZONTAL);
  SetSizer(p_topSizer);

  p_leftRowSizer = new wxBoxSizer(wxVERTICAL);
  p_topSizer->Add(p_leftRowSizer, 0, wxGROW, 0);

  p_centerSizer = new wxBoxSizer(wxVERTICAL);
  p_topSizer->Add(p_centerSizer, 1, wxGROW, 0);

  // @todo Should consider buildnow flag here?
#ifdef __WXMSW__
  int *gl_attrib = NULL;
#else
  int gl_attrib[20] = { WX_GL_RGBA, WX_GL_MIN_RED, 1, WX_GL_MIN_GREEN, 1,
                        WX_GL_MIN_BLUE, 1, WX_GL_DEPTH_SIZE, 1,
                        WX_GL_DOUBLEBUFFER,
#  if defined(__WXMAC__) || defined(__WXCOCOA__)
                        GL_NONE };
#  else
                        None };
#  endif
#endif
  p_renderArea = new SoWxRenderArea(this , wxID_ANY, wxDefaultPosition,
                                    wxDefaultSize, 0, "SoWxViewer", gl_attrib);

  p_centerSizer->Add(p_renderArea, 1, wxGROW, 0);
  
  p_bottomRowSizer = new wxBoxSizer(wxHORIZONTAL);
  p_centerSizer->Add(p_bottomRowSizer, 0, wxGROW, 0);

  p_rightRowSizer = new wxBoxSizer(wxVERTICAL);
  p_topSizer->Add(p_rightRowSizer, 0, wxGROW, 0);

  p_leftRowSizer->Add(new wxButton(this, -1, "L1", wxDefaultPosition,
                                   wxSize(30, 30), wxBU_EXACTFIT));
  p_leftRowSizer->Add(new wxButton(this, -1, "L2", wxDefaultPosition,
                                   wxSize(30, 30), wxBU_EXACTFIT));
  p_leftRowSizer->AddStretchSpacer();
  p_leftRowSizer->Add(new wxButton(this, -1, "L3", wxDefaultPosition,
                                   wxSize(30, 30), wxBU_EXACTFIT));
  p_leftRowSizer->Add(new wxButton(this, -1, "L4", wxDefaultPosition,
                                   wxSize(30, 30), wxBU_EXACTFIT));

  p_rightRowSizer->Add(new wxButton(this, -1, "R1", wxDefaultPosition,
                                    wxSize(30, 30), wxBU_EXACTFIT));
  p_rightRowSizer->Add(new wxButton(this, -1, "R2", wxDefaultPosition,
                                    wxSize(30, 30), wxBU_EXACTFIT));
  p_rightRowSizer->Add(new wxButton(this, -1, "R3", wxDefaultPosition,
                                    wxSize(30, 30), wxBU_EXACTFIT));
  p_rightRowSizer->AddStretchSpacer();
  p_rightRowSizer->Add(new wxButton(this, -1, "R4", wxDefaultPosition,
                                    wxSize(30, 30), wxBU_EXACTFIT));


  p_bottomRowSizer->Add(new wxButton(this, -1, "B1", wxDefaultPosition,
                                     wxSize(30, 30), wxBU_EXACTFIT));
  p_bottomRowSizer->AddStretchSpacer();
  p_bottomRowSizer->Add(new wxButton(this, -1, "B2", wxDefaultPosition,
                                     wxSize(30, 30), wxBU_EXACTFIT));
  p_bottomRowSizer->Add(new wxButton(this, -1, "B3", wxDefaultPosition,
                                     wxSize(30, 30), wxBU_EXACTFIT));
  p_bottomRowSizer->Add(new wxButton(this, -1, "B4", wxDefaultPosition,
                                     wxSize(30, 30), wxBU_EXACTFIT));

  p_decorationFlag = true;
}


/**
 * Set a new user supplied scene graph.
 */
void SoWxViewer::setSceneGraph(SoNode *newScene)
{
  // if we haven't already given the render area a scene graph sceneRoot,
  // give it the scene graph now. This is a one shot deal, which
  // cannot be done in the constructor.
  if (p_renderArea->getSceneGraph() == NULL)
    p_renderArea->setSceneGraph(p_sceneRoot);

  /////////////////////////////////////////
  // @todo Not sure if this is needed
  // draw new scene graphs to the front buffer by default since
  // the scene will be different (we might has well see something
  // happening for the first redraw).
  //  if (isDrawToFrontBufferEnable())
  //    p_drawToFrontBuffer = TRUE;

  // detach everything that depends on the old sceneGraph
  if ( p_sceneGraph != NULL ) {
    setCamera(NULL);
    p_sceneRoot->removeChild(p_sceneGraph);
  }

  p_sceneGraph = newScene;
  
  // now assign the new sceneGraph, find or create the new camera 
  // and attach things back.
  if ( p_sceneGraph != NULL ) {
    p_sceneRoot->addChild(p_sceneGraph);
    
    // search for first camera in the scene
    SoSearchAction sa;
    sa.setType(SoCamera::getClassTypeId());
    sa.setSearchingAll(false); // don't look under off switches
    sa.apply(p_sceneGraph);
    
    SoCamera * newCamera = NULL;
    if (sa.getPath())
      newCamera = (SoCamera *)((SoFullPath *)sa.getPath())->getTail();
    
    // if no camera found create one of the right kind...
    if ( newCamera == NULL ) {
      
      newCamera = (SoCamera*) p_cameraType.createInstance();
      if (newCamera == NULL) {
#ifdef DEBUG
        SoDebugError::post("SoWxViewer::setSceneGraph",
                           "unknown camera type!");
#endif
        // ??? what should we do here ?
        p_cameraType = SoPerspectiveCamera::getClassTypeId();
        newCamera = new SoPerspectiveCamera;
      }
      p_createdCamera = TRUE;
      
      if (p_type == SoWxViewer::BROWSER)
        // add camera after drawstyle stuff
        p_sceneRoot->insertChild(newCamera, 1);
      else {
        // check to make sure scene starts with at least a group node
        if ( p_sceneGraph->isOfType(SoGroup::getClassTypeId()) )
          ((SoGroup *)p_sceneGraph)->insertChild(newCamera, 0);
        else {
          // make scene start with a group node
          SoGroup *group = new SoGroup;
          group->addChild(newCamera);
          group->addChild(p_sceneGraph);
          p_sceneRoot->addChild(group);
          p_sceneRoot->removeChild(p_sceneGraph);
          p_sceneGraph = group;
        }
      }
      
      newCamera->viewAll(p_sceneGraph, SbViewportRegion(getGlxSize()));
    }
    
    setCamera(newCamera);
  }

  // recompute the scene size variables...
  recomputeSceneSize();
}


/**
 * Return the user supplied scene graph.
 */
SoNode * SoWxViewer::getSceneGraph()
{
  return p_sceneGraph;
}


/**
 * Sets the camera to use.
 */
void SoWxViewer::setCamera(SoCamera *newCamera)
{
  // check for trivual return
  if (p_camera == newCamera)
    return;
  
  // detach everything that depended on the old camera
  if ( p_camera != NULL ) {
    
    if (p_headlightFlag) {
      setHeadlight(false);
      p_headlightFlag = true;  // can later be turned on
    }
    
    if (p_viewingFlag) {
      setViewing(false);
      p_viewingFlag = true;  // can later be turned on
    }
    
    // remove the camera if we created one outside of the
    // scene graph.
    if (p_createdCamera && p_type == SoWxViewer::BROWSER) {
      if (p_sceneRoot->findChild(p_camera) >= 0)
        p_sceneRoot->removeChild(p_camera);
      p_createdCamera = false;
    }
    
    p_camera->unref();
  }
  
  p_camera = newCamera;
  
  // attach everything that depends on the new camera
  if ( p_camera != NULL) {
    p_camera->ref();
    
    if (p_headlightFlag) {
      p_headlightFlag = false;  // enables the routine to be called
      setHeadlight(true);
    }
    
    if (p_viewingFlag) {
      p_viewingFlag = false;  // enables the routine to be called
      setViewing(true);
    }
    
    saveHomePosition();
  }
}


/**
 * Get the camera object.
 */
SoCamera * SoWxViewer::getCamera()
{
  return p_camera;
}


/**
 * Set the camera type to create.
 */
void SoWxViewer::setCameraType(SoType type)
{
  if (type.isDerivedFrom(SoPerspectiveCamera::getClassTypeId()) ||
      type.isDerivedFrom(SoOrthographicCamera::getClassTypeId()))
    p_cameraType = type;
#ifdef DEBUG
  else
    SoDebugError::post("SoWxViewer::setCameraType",
                       "unknown camera type!");
#endif
}


/**
 * Get the camera type.
 */
SoType SoWxViewer::getCameraType()
{
  return p_cameraType;
}


/**
 * See the whole scene from the camera.
 */
void SoWxViewer::viewAll()
{
  if ( p_camera != NULL )
    p_camera->viewAll(p_sceneGraph,SbViewportRegion(getGlxSize()));
}


/**
 * Saves the camera values for later restore.
 */
void SoWxViewer::saveHomePosition()
{
  if (p_camera == NULL)
    return;
  
  p_origPosition = p_camera->position.getValue();
  p_origOrientation = p_camera->orientation.getValue();
  p_origNearDistance = p_camera->nearDistance.getValue();
  p_origFarDistance = p_camera->farDistance.getValue();
  p_origFocalDistance = p_camera->focalDistance.getValue();
    
  // save camera height (changed by zooming)
  if (p_camera->isOfType(SoPerspectiveCamera::getClassTypeId()))
    p_origHeight = ((SoPerspectiveCamera *)p_camera)->heightAngle.getValue();
  else if (p_camera->isOfType(SoOrthographicCamera::getClassTypeId()))
    p_origHeight = ((SoOrthographicCamera *)p_camera)->height.getValue();
}


/**
 * Reset the camera to it's saved values.
 */
void SoWxViewer::resetToHomePosition()
{
  if (p_camera == NULL)
    return;

  p_camera->position = p_origPosition;
  p_camera->orientation = p_origOrientation;
  p_camera->nearDistance = p_origNearDistance;
  p_camera->farDistance = p_origFarDistance;
  p_camera->focalDistance = p_origFocalDistance;

  // restore camera height (changed by zooming)
  if (p_camera->isOfType(SoPerspectiveCamera::getClassTypeId()))
    ((SoPerspectiveCamera *)p_camera)->heightAngle.setValue(p_origHeight);
  else if (p_camera->isOfType(SoOrthographicCamera::getClassTypeId()))
    ((SoOrthographicCamera *)p_camera)->height.setValue(p_origHeight);
}


/**
 * adds a directional light to the scene graph that is a headlight
 * positioned over the left shoulder of the camera.  It has a sensor
 * on the camera, so that it always is pointing in the same direction
 * as the camera.  The sensor is not delayed, so that it is "always"
 * accurate.
 */
void SoWxViewer::setHeadlight(SbBool insertFlag)
{
  // check for trivial return
  if (p_camera == NULL || p_headlightFlag == insertFlag) {
    p_headlightFlag = insertFlag;
    return;
  }
  
  // find the camera parent to insert/remove the headlight
  SoSearchAction sa;
  if (insertFlag)
    sa.setNode(p_camera);
  else {
    sa.setNode(p_headlightGroup);
    sa.setSearchingAll(true); // find under OFF switches for removal
  }
  sa.apply(p_sceneRoot);
  SoFullPath *fullPath = (SoFullPath *) sa.getPath();
  if (!fullPath) {
#if DEBUG
    SoDebugError::post("SoWxViewer::setHeadlight",
                       insertFlag ? "ERROR: cannot find camera in graph" :
                       "ERROR: cannot find headlight in graph");
#endif
    return;
  }
  SoGroup * parent = (SoGroup *) fullPath->getNodeFromTail(1);
  
  p_headlightFlag = insertFlag;
  
  // inserts/remove the headlight group node
  if (p_headlightFlag) {
    int camIndex;
    
    // check to make sure that the camera parent is not a switch node
    // (VRML camera viewpoints are kept under a switch node). Otherwise
    // we will insert the headlight right before the switch node.
    if (parent->isOfType(SoSwitch::getClassTypeId())) {
      SoNode *switchNode = parent;
      parent = (SoGroup *) fullPath->getNodeFromTail(2);
      camIndex = parent->findChild(switchNode);
    }
    else
      camIndex = parent->findChild(p_camera);
	
    // return if headlight is already there (this should be an error !)
    if (parent->findChild(p_headlightGroup) >= 0)
      return;
	
    // insert the light group right after the camera
    if (camIndex >= 0)
      parent->insertChild(p_headlightGroup, camIndex+1);
  }
  else {
    if (parent->findChild(p_headlightGroup) >= 0)
      parent->removeChild(p_headlightGroup);
  }
}


/**
 * Get headlight status.
 */
SbBool SoWxViewer::isHeadlight()
{
  return p_headlightFlag;
}


/**
 * Get headlight object.
 */
SoDirectionalLight * SoWxViewer::getHeadlight()
{
  return p_headlightNode;
}


/**
 * Set the drawing style.
 */
void SoWxViewer::setDrawStyle(SoWxViewer::DrawType type, 
                              SoWxViewer::DrawStyle style)
{
  // prevent us from picking a default draw style if the user already
  // has done so...
  p_checkForDrawStyle = false;
    
  if (type == STILL) {
    if (p_stillDrawStyle == style)
      return;
    if (style == VIEW_SAME_AS_STILL) {
#ifdef DEBUG
      SoDebugError::post("SoWxViewer::setDrawStyle", 
                         "illegal VIEW_SAME_AS_STILL draw style passed for STILL !");
#endif
      return; 
    }
    p_stillDrawStyle = style;
    
    if (! p_interactiveFlag || p_interactiveDrawStyle == VIEW_SAME_AS_STILL
        || (p_interactiveDrawStyle == VIEW_NO_TEXTURE && style != VIEW_AS_IS))
      setCurrentDrawStyle(style);
    else if (p_interactiveFlag && p_interactiveDrawStyle == VIEW_NO_TEXTURE && 
             style == VIEW_AS_IS)
      setCurrentDrawStyle(p_interactiveDrawStyle);
  }
  else { // type == INTERACTIVE
    if (p_interactiveDrawStyle == style)
      return;
    p_interactiveDrawStyle = style;
    
    if (p_interactiveFlag) {
      if (style == VIEW_SAME_AS_STILL || 
          (style == VIEW_NO_TEXTURE && p_stillDrawStyle != VIEW_AS_IS))
        setCurrentDrawStyle(p_stillDrawStyle);
      else
        setCurrentDrawStyle(style);
    }
  }
}


/**
 * Get the drawing style.
 */
SoWxViewer::DrawStyle SoWxViewer::getDrawStyle(SoWxViewer::DrawType type)
{
  return (type == STILL ? p_stillDrawStyle : p_interactiveDrawStyle);
}


/**
 * Set the current buffering type in the main view.
 */
void SoWxViewer::setBufferingType(SoWxViewer::BufferType type)
{
  if (p_bufferType == type)
    return;
  
  // remove interactive callback
  if (p_bufferType == BUFFER_INTERACTIVE) {
    removeStartCallback(SoWxViewer::bufferStartCallback);
    removeFinishCallback(SoWxViewer::bufferFinishCallback);
  }
  
  p_bufferType = type;
  
  switch(p_bufferType) {
  case BUFFER_SINGLE:
    p_renderArea->setDoubleBuffer(false);
    break;
  case BUFFER_DOUBLE:
    p_renderArea->setDoubleBuffer(true);
    break;
  case BUFFER_INTERACTIVE:
    p_renderArea->setDoubleBuffer(false);
    addStartCallback(SoWxViewer::bufferStartCallback);
    addFinishCallback(SoWxViewer::bufferFinishCallback);
    break;
  }
}


/**
 * Get the current buffering type in the main view.
 */
SoWxViewer::BufferType SoWxViewer::getBufferingType()
{
  return p_bufferType;
}


/**
 * redefine this routine from SoWxGLWidget to call the viewer
 * SoWxViewer::setBufferingType() method which is a superset.
 */
void SoWxViewer::setDoubleBuffer(SbBool flag)
{
  setBufferingType(flag ? SoWxViewer::BUFFER_DOUBLE :
                   SoWxViewer::BUFFER_SINGLE);
}


/**
 * Set whether the viewer is turned on or off. When turned off
 * events over the renderArea are sent down the sceneGraph
 * (picking can occurs). (default viewing is ON)
 */
void SoWxViewer::setViewing(SbBool flag)
{
  if (flag == p_viewingFlag)
    return;
    
  p_viewingFlag = flag;
    
  // if we are goind into viewing mode, then de-highlight any
  // currently highlighted nodes (since the object will never receive
  // any motion events).
  if (p_viewingFlag) {
    SoGLRenderAction *glAct = p_renderArea->getGLRenderAction();
    if (glAct)
      SoLocateHighlight::turnOffCurrentHighlight(glAct);
  }
}


/**
 * Get the viewer status.
 */
SbBool SoWxViewer::isViewing() const
{
  return p_viewingFlag;
}


/**
 * Enables/Disable the viewer cursor on the window.
 */
void SoWxViewer::setCursorEnabled(SbBool flag)
{
  p_cursorEnabledFlag = flag;
}


/**
 * Get cursor status.
 */
SbBool SoWxViewer::isCursorEnabled() const
{
  return p_cursorEnabledFlag;
}


/**
 * Set the auto clipping mode.
 */
void SoWxViewer::setAutoClipping(SbBool flag)
{
  if (p_autoClipFlag == flag)
    return;
  
  p_autoClipFlag = flag;
    
  // cause a redraw to correctly place the near and far plane now that
  // auto clipping is on.
  if (p_autoClipFlag)
    p_renderArea->scheduleRedraw();
}


/**
 * Get the auto clipping mode.
 */
SbBool SoWxViewer::isAutoClipping() const
{
  return p_autoClipFlag;
}


/**
 * Set stereo mode.
 */
void SoWxViewer::setStereoViewing(SbBool flag)
{
  if (flag == isStereoViewing())
    return;
    
  // First, check to see if the OpenGL stereo visual can be created
  /////////////////////////////
  // @todo need to work on this
  //  setStereoBuffer(flag);
    
#ifdef __sgi
  // since OpenGL stereo failed, see if the SGI extension will work
  // by checking whether the X server supports it....
  int first_event, first_error;
  if (flag != isStereoViewing() && 
      XSGIStereoQueryExtension(getDisplay(), &first_event, &first_error)) {
	
    if (flag) {
      // make sure the current window will support stereo
      // ??? if we havn't been managed yet, just assume this visual
      // ??? will support stereo viewing (see bug 
      if (! getNormalWindow())
        p_useSGIStereoExt = true;
      else if (XSGIQueryStereoMode(getDisplay(), getNormalWindow()) !=
               X_STEREO_UNSUPPORTED)
        // stereo will be turned on in the rendering....
        p_useSGIStereoExt = true;
	    
      // save the camera original aspect ratio since it will be updated
      // during rendering to strech the objects. We will restore it
      // when stereo if OFF.
      p_camStereoOrigAspect = p_camera->aspectRatio.getValue();
      p_camStereoOrigVPMapping = p_camera->viewportMapping.getValue();
    }
    else {
      // turn stereo off on the window
      p_useSGIStereoExt = false;
	    
      // clear the left/right buffers to prevent gost images from
      // the other view...(until the user resets the monitor with setmon)
      if (isRGBMode()) {
        SbColor color = getBackgroundColor();
        glClearColor(color[0], color[1], color[2], 0);
      }
      else
        glClearIndex(getBackgroundIndex());
	    
      glDrawBuffer(GL_FRONT_AND_BACK);
	    
      XSGISetStereoBuffer(getDisplay(), getNormalWindow(), STEREO_BUFFER_LEFT);
      XSync(getDisplay(), False);
      glClear(GL_COLOR_BUFFER_BIT);
	    
      XSGISetStereoBuffer(getDisplay(), getNormalWindow(), STEREO_BUFFER_RIGHT);
      XSync(getDisplay(), False);
      glClear(GL_COLOR_BUFFER_BIT);
	    
      glDrawBuffer( isDoubleBuffer() ? GL_BACK : GL_FRONT);
	    
      // restore the camera original aspect ratio (saved above)
      p_camera->aspectRatio = p_camStereoOrigAspect;
      p_camera->viewportMapping = p_camStereoOrigVPMapping;
    }
	
    // now cause a redraw to see the affect since we havn't changed
    // the actual visual (unlike OpenGL)
    if (flag == isStereoViewing())
      scheduleRedraw();
  }
#endif
}


/**
 * Get stereo mode
 */
SbBool SoWxViewer::isStereoViewing()
{
#ifdef __sgi
  return (isStereoBuffer() || useSGIStereoExt);
#else
  // done in SoXtGLWidget
  //////////////////////////////
  // @todo Need to work on this.
  //  return isStereoBuffer();
  return false;
#endif
}


/**
 * Specify what the offset between the two views should be.
 */
void SoWxViewer::setStereoOffset(float dist)
{
  p_stereoOffset = dist;
}


/**
 * Get the offset of the stereo views
 */
float SoWxViewer::getStereoOffset()
{
  return p_stereoOffset;
}


/**
 * Determine whether or not to orient camera on picked point (detail on)
 * or center of the object's bounding box (detail off). Default is detail on.
 */
void SoWxViewer::setDetailSeek(SbBool onOrOff)
{
  p_detailSeekFlag = onOrOff;
}


/**
 * Get the status of detail seek.
 */
SbBool SoWxViewer::isDetailSeek()
{
  return p_detailSeekFlag;
}


/**
 * Set the time a seek takes to change to the new camera location.
 * A value of zero will not animate seek. Default value is 2 seconds.
 */
void SoWxViewer::setSeekTime(float seconds)
{
  p_seekAnimTime = seconds;
}


/**
 * Get the time a seek takes.
 */
float SoWxViewer::getSeekTime()
{
  return p_seekAnimTime;
}


/**
 * Add start callback routines on the viewer.
 */
void SoWxViewer::addStartCallback(SoWxViewerCB *f, void * userData)
{
  p_startCBList->addCallback((SoCallbackListCB *)f, userData);
}


/**
 * Add finish callback routines on the viewer.
 */
void SoWxViewer::addFinishCallback(SoWxViewerCB *f, void * userData)
{
  p_finishCBList->addCallback((SoCallbackListCB *)f, userData);
}


/**
 * Remove start callback routines on the viewer.
 */
void SoWxViewer::removeStartCallback(SoWxViewerCB *f, void * userData)
{
  p_startCBList->removeCallback((SoCallbackListCB *)f, userData);
}


/**
 * Remove finish callback routines on the viewer.
 */
void SoWxViewer::removeFinishCallback(SoWxViewerCB *f, void *userData)
{
  p_finishCBList->removeCallback((SoCallbackListCB *)f, userData);
}


/**
 * Copy the camera onto the clipboard.
 */
void SoWxViewer::copyView(Time eventTime)
{
  if (p_camera == NULL)
    return;
  
  /*
    if (p_clipboard == NULL)
    clipboard = new SoXtClipboard(getWidget());
    
    p_clipboard->copy(p_camera, eventTime);
  */
}


/**
 * Retrieve the selection from the X server and paste it when it
 * arrives (in our pasteDone callback).
 */
void SoWxViewer::pasteView(Time eventTime)
{
  /*
    if (p_clipboard == NULL)
    p_clipboard = new SoXtClipboard(getWidget());
    
    p_clipboard->paste(eventTime, SoXtViewer::pasteDoneCB, this);
  */
}


/**
 * 
 */
void SoWxViewer::setNormalVisual(XVisualInfo * vis)
{
  /*
  // call parent class
  p_renderArea->setNormalVisual(vis);
  
  // now update the buffering type
  if (isDoubleBuffer())
  setBufferingType(BUFFER_DOUBLE);
  else
  setBufferingType(BUFFER_SINGLE);
  */
}


/**
 * Recomputes the scene sizes
 */
void SoWxViewer::recomputeSceneSize()
{
  if (! p_sceneGraph || ! p_sceneRoot) {
    p_sceneSize = 0.0;
    return;
  }
    
  // Use assignment notation to disambiguate from expression (edison)
  SoGetBoundingBoxAction bboxAct =
    SoGetBoundingBoxAction(SbViewportRegion(getGlxSize()));
  bboxAct.apply(p_sceneRoot);
  SbBox3f bbox = bboxAct.getBoundingBox();
    
  if (bbox.isEmpty()) {
    p_sceneSize = 0.0;
    return;
  }
    
  float x, y, z;
  bbox.getSize(x, y, z);
  p_sceneSize = (x > z) ? x : z;
  if (y > p_sceneSize)
    p_sceneSize = y;
  if (p_sceneSize <= 0.0)
    p_sceneSize = 0.0;
}




///////////////////////////////////////////////////
// Protected functions start here
///////////////////////////////////////////////////


/**
 * Process a common set of events which are shared accross all
 * viewers. Returning TRUE if the event was processed.
 */
SbBool SoWxViewer::processCommonEvents(wxEvent * event)
{

  // check if the application wants to handle the event itself
  // instead of giving it to the viewer. This can be used to disable
  // some simple viewer functionality (like the Arrow keys or Esc key).
  // ??? this is a simple work around for bug #113991 - Xt translation
  // ??? tables would be better than dealing with events directly.
  if (p_renderArea->invokeAppCB(event))
    return TRUE;

  // check for special key which turns viewing on/off


  /*
    @todo Skip this mode changing key events for now
  KeySym keysym;

  if (xe->type == KeyPress) {
    XKeyEvent *ke = (XKeyEvent *)xe;
    keysym = XLookupKeysym(ke, 0);
    if (keysym == XK_Escape) {
      setViewing( !isViewing() );  // toggle the viewing mode...
      return TRUE;
    }
    else if (!isViewing() && (keysym == XK_Alt_L || keysym == XK_Alt_R)
             && !(ke->state & Button1Mask || ke->state & Button2Mask)) {
      // Alt-key goes from PICK to VIEW if
      // 1] we are not in VIEW mode already
      // 2] no mouse buttons are pressed
      //
      altSwitchBack = TRUE;   // later return back
      setViewing(TRUE);
      return TRUE;
    }
  }
  else if (xe->type == KeyRelease) {
    keysym = XLookupKeysym((XKeyEvent *)xe, 0);
    if (altSwitchBack && (keysym == XK_Alt_L || keysym == XK_Alt_R)) {
      // if Alt-key, then return to PICK (if we had switched)
      setViewing(FALSE);
      altSwitchBack = FALSE;  // clear the flag
      return TRUE;
    }
  }
  else if (xe->type == EnterNotify) {
    XCrossingEvent *ce = (XCrossingEvent *)xe;
    //
    // because the application might use Alt-key for motif menu
    // accelerators we might not receive a key-up event, so make sure
    // to reset any Alt mode (temporary viewing) when the mouse re-enters
    // the window.
    //
    if (! isViewing() && ce->state & Mod1Mask) {
      altSwitchBack = TRUE;   // later return back
      setViewing(TRUE);
    }
    else if (altSwitchBack && !(ce->state & Mod1Mask)) {
      setViewing(FALSE);
      altSwitchBack = FALSE;  // clear the flag
    }
  }
  */

  // send the event to the scene graph if viewing is off
  if ( !isViewing() ) {
    p_renderArea->processRenderAreaEvent(event);
    return true;
  }

  // if no camera discard events
  if (p_camera == NULL)
    return true;

  SbBool handled = true;

  // ??? workaround what seem to be an Xt bug...
  //  updateEventState(xe);

  /*
    @todo Skip this mode changing key events for now

  switch(xe->type) {
  case KeyPress:
    switch ( keysym ) {
    case XK_Home:
      resetToHomePosition();
      break;
    case XK_s:
      setSeekMode( !isSeekMode() );
      // ??? this is kind of a hack, but it is needed
      // ??? until a better solution is found
      if ( isSeekMode() && interactiveCount != 0 ) {
        interactiveCount = 0;
        finishCBList->invokeCallbacks(this);
      }
      break;
    case XK_Left:
    case XK_Up:
    case XK_Right:
    case XK_Down:
      arrowKeyPressed(keysym);
      break;
    default:
      handled = FALSE;
      break;
    }
    break;

  default:
    handled = FALSE;
    break;
  }
  */

  handled = FALSE;

  return handled;
}


/**
 * Increment the intercative viewing counter.
 */
void SoWxViewer::interactiveCountInc()
{
  p_interactiveCount++;
    
  if (p_interactiveCount == 1)
    p_startCBList->invokeCallbacks(this);
}


/**
 * Decrement the intercative viewing counter.
 */
void SoWxViewer::interactiveCountDec()
{
  if (p_interactiveCount > 0) {
    p_interactiveCount--;
    if (p_interactiveCount == 0)
      p_finishCBList->invokeCallbacks(this);
  }
}


/**
 * Get the intercative viewing counter.
 */
int SoWxViewer::getInteractiveCount()
{
  return p_interactiveCount;
}


/**
 * This routine is used by subclasses to initiate the seek animation. Given a
 * screen mouse location, this routine will return the picked point
 * and the normal at that point. It will also schedule the sensor to animate
 * if necessary. The routine retuns TRUE if something got picked...
 * Note: if detailSeek is on, the point and normal correspond to the exact 
 *       3D location under the cursor.
 *       if detailSeek if off, the object bbox center and the camera 
 *       orientation are instead returned.
 */
SbBool SoWxViewer::seekToPoint(const SbVec2s &mouseLocation)
{
  if (p_camera == NULL) {
    setSeekMode(false);
    return false;
  }
  
  // do the picking
  // Use assignment notation to disambiguate from expression (edison)
  SoRayPickAction pick = SoRayPickAction(SbViewportRegion(getGlxSize()));
  pick.setPoint(mouseLocation);
  pick.setRadius(1.0);
  pick.setPickAll(false); // pick only the closest object
  pick.apply(p_sceneRoot);
  
  // makes sure something got picked
  SoPickedPoint *pp = pick.getPickedPoint();
  if ( pp == NULL ) {
    setSeekMode(false);
    return false;
  }
  
  // Get picked point and normal if detailtSeek
  if (p_detailSeekFlag) {
    
    p_seekPoint = pp->getPoint();
    p_seekNormal = pp->getNormal();
    
    // check to make sure normal points torward the camera, else
    // flip the normal around
    if ( p_seekNormal.dot(p_camera->position.getValue() - p_seekPoint) < 0 )
      p_seekNormal.negate();
  }

  // else get object bounding box as the seek point and the camera
  // orientation as the normal.
  else {
    // get center of object's bounding box
    // Use assignment notation to disambiguate from expression (edison)
    SoGetBoundingBoxAction bba = SoGetBoundingBoxAction(SbViewportRegion(getGlxSize()));
    bba.apply(pp->getPath());
    SbBox3f bbox = bba.getBoundingBox();
    p_seekPoint = bbox.getCenter();
	
    // keep the camera oriented the same way
    SbMatrix mx;
    mx = p_camera->orientation.getValue();
    p_seekNormal.setValue(mx[2][0], mx[2][1], mx[2][2]);
  }
  
  // now check if animation sensor needs to be scheduled
    
  p_computeSeekVariables = TRUE;
  if (p_seekAnimTime == 0) {
	
    // jump to new location, no animation needed
    interpolateSeekAnimation(1.0);
  }
  else {
    // schedule sensor and call viewer start callbacks
    if ( ! p_seekAnimationSensor->getAttachedField() ) {
      p_seekAnimationSensor->attach(p_viewerRealTime);
      p_seekAnimationSensor->schedule();
      interactiveCountInc();
    }
	
    p_seekStartTime = p_viewerRealTime->getValue();
  }
  
  return true;    // successfull
}


/**
 * Subclasses CAN redefine this to interpolate camera position/orientation
 * while the seek animation is going on (called by animation sensor).
 * The parameter t is a [0,1] value corresponding to the animation percentage
 * completion. (i.e. a value of 0.25 means that animation is only 1/4 of the
 * way through).
 */
void SoWxViewer::interpolateSeekAnimation(float t)
{
  if (p_camera == NULL)
    return;
  
  // check if camera new and old position/orientation have already
  // been computed.
  if (p_computeSeekVariables) {
    SbMatrix mx;
    SbVec3f viewVector;
    
    // save camera starting point
    p_oldCamPosition = p_camera->position.getValue();
    p_oldCamOrientation = p_camera->orientation.getValue();
    
    // compute the distance the camera will be from the seek point
    // and update the camera focalDistance.
    float dist;
    if ( p_seekDistAsPercentage ) {
      SbVec3f seekVec(p_seekPoint - p_camera->position.getValue());
      dist = seekVec.length() * (p_seekDistance / 100.0);
    }
    else
      dist = p_seekDistance;
    p_camera->focalDistance = dist;
    
    // let subclasses have a chance to redefine what the
    // camera final orientation should be.
    computeSeekFinalOrientation();
    
    // find the camera final position based on orientation and distance
    mx = p_newCamOrientation;
    viewVector.setValue(-mx[2][0], -mx[2][1], -mx[2][2]);
    p_newCamPosition = p_seekPoint - dist * viewVector;
    
    p_computeSeekVariables = false;
  }
  
  // Now position the camera according to the animation time
    
  // use and ease-in ease-out approach
  float cos_t = 0.5 - 0.5 * cosf(t * M_PI);
    
  // get camera new rotation
  p_camera->orientation = SbRotation::slerp(p_oldCamOrientation,
                                            p_newCamOrientation, cos_t);
    
  // get camera new position
  p_camera->position = p_oldCamPosition + 
    (p_newCamPosition - p_oldCamPosition) * cos_t;
}


/**
 *
 */
void SoWxViewer::computeSeekFinalOrientation()
{
  SbMatrix mx;
  SbVec3f viewVector;
  
  // find the camera final orientation
  if ( isDetailSeek() ) {
    
    // get the camera new orientation
    mx = p_camera->orientation.getValue();
    viewVector.setValue(-mx[2][0], -mx[2][1], -mx[2][2]);
    SbRotation changeOrient;
    changeOrient.setValue(viewVector,
                          p_seekPoint - p_camera->position.getValue());
    p_newCamOrientation = p_camera->orientation.getValue() * changeOrient;
  }
  else
    p_newCamOrientation = p_camera->orientation.getValue();
}


/**
 * Externally set the viewer into/out off seek mode (default OFF). Actual
 * seeking will not happen until the viewer decides to (ex: mouse click).
 *
 *  Note: setting the viewer out of seek mode while the camera is being
 *  animated will stop the animation to the current location.
 */
void SoWxViewer::setSeekMode(SbBool flag)
{
  if (!isViewing())
    return;
    
  // check if seek is being turned off while seek animation is happening
  if ( !flag && p_seekAnimationSensor->getAttachedField() ) {
    p_seekAnimationSensor->detach();
    p_seekAnimationSensor->unschedule();
    interactiveCountDec();
  }
    
  p_seekModeFlag = flag;
}


/**
 *
 */
SbBool SoWxViewer::isSeekMode()
{
  return p_seekModeFlag;
}


/**
 * @todo BIG BIG TROUBLE. NOT SUBCLASSING FROM RENDERAREA ANYMORE.
 *       THIS FUNCTION WILL NEVER GET CALLED!!!
 * Adjust the camera clipping planes before a redraw.
 */
void SoWxViewer::actualRedraw()
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxViewer::actualRedraw()\n";
#endif

  if (isAutoClipping() && ! isStereoViewing())
    adjustCameraClippingPlanes();
  
  // update the headlight if necessary
  if (p_headlightFlag && p_camera)
    p_headlightRot->rotation.setValue(p_camera->orientation.getValue());
  
  // make sure that we have a valid sceneSize value - but don't compute
  // a new sceneSize value for every redraw since the walking speed should
  // really be constant.
  if (p_sceneSize == 0.0)
    recomputeSceneSize();
  
  // Check to see if we are in stereo mode, if so draw the scene
  // twice with the camera offseted between the two views, else
  // do a simple redraw.
  
  if ( isStereoViewing() && p_camera != NULL) {
    // Check the camera type, since stereo is different:
    //
    // Ortho camera: setereo is accomplished by simply rorating
    // the camera (around the point of interest) by 6 degree. 
    //
    // Perspective camera: we translate the camera and rotate
    // them to look at the same point of interest (idealy we also would
    // make sure the plane of convergence is exactly the same for
    // both perspective views, unfortunatly we cannot do this with
    // the current symetric view volumes).
    
    // save the camera original values to restore the camera after
    // both views are rendered. This means we will use this in between
    // left and right view for things like picking.
    SbVec3f camOrigPos = p_camera->position.getValue();
    SbRotation camOrigRot = p_camera->orientation.getValue();
    
    // get the camera focal point
    SbMatrix mx;
    mx = camOrigRot;
    SbVec3f forward( -mx[2][0], -mx[2][1], -mx[2][2]);
    float radius = p_camera->focalDistance.getValue();
    SbVec3f center = camOrigPos + radius * forward;
    
#ifdef __sgi
    // if we are splitting the screen in half (loose vertical resolution)
    // then change the aspect ratio to squish the objects to make them 
    // look square again through the stereo glasses. This is done for
    // every redraw since we need to manually update the aspect ourself.
    if (useSGIStereoExt) {
      SbVec2s windowSize = getGlxSize();
      p_camera->aspectRatio = 0.5 * windowSize[0] / (float) windowSize[1];
      p_camera->viewportMapping = SoCamera::LEAVE_ALONE;
    }
#endif
    
    // change the camera for the LEFT eye view, and render
#ifdef __sgi
    if (useSGIStereoExt) {
      XSGISetStereoBuffer(getDisplay(), getNormalWindow(), STEREO_BUFFER_LEFT);
      XSync(getDisplay(), False);
    }
    else
#endif
      glDrawBuffer((isDoubleBuffer() && !p_renderArea->isDrawToFrontBuffer())
                   ? GL_BACK_LEFT : GL_FRONT_LEFT);
    // rotate the camera by - stereoOffset/2 degrees
    p_camera->orientation = 
      SbRotation(SbVec3f(0, 1, 0), -p_stereoOffset*M_PI/360.0) * camOrigRot;
    
    // reposition camera to look at pt of interest
    mx = p_camera->orientation.getValue();
    forward.setValue( -mx[2][0], -mx[2][1], -mx[2][2]);
    p_camera->position = center - radius * forward;
    
    if (isAutoClipping())
      adjustCameraClippingPlanes();
    doRendering();
    
    // change the camera for the RIGHT eye view, and render
#ifdef __sgi
    if (useSGIStereoExt) {
      XSGISetStereoBuffer(getDisplay(), getNormalWindow(), STEREO_BUFFER_RIGHT);
      XSync(getDisplay(), False);
    }
    else
#endif
      glDrawBuffer((isDoubleBuffer() && ! p_renderArea->isDrawToFrontBuffer())
                   ? GL_BACK_RIGHT : GL_FRONT_RIGHT);
    // rotate the camera by + stereoOffset/2 degrees
    p_camera->orientation = 
      SbRotation(SbVec3f(0, 1, 0), p_stereoOffset * M_PI / 360.0) * camOrigRot;
    
    // reposition camera to look at pt of interest
    mx = p_camera->orientation.getValue();
    forward.setValue( -mx[2][0], -mx[2][1], -mx[2][2]);
    p_camera->position = center - radius * forward;
    
    if (isAutoClipping())
      adjustCameraClippingPlanes();
    doRendering();

    // reset the camera original values now that we are done rendering
    // the stereo views (leave aspect ratio to do correct picking).
    p_camera->enableNotify(FALSE); // don't cause a redraw
    p_camera->position = camOrigPos;
    p_camera->orientation = camOrigRot;
    p_camera->enableNotify(TRUE);
    
#ifdef __sgi
    if (! p_useSGIStereoExt)
#endif
      // restore to draw to both buffer (viewer feedback)
      glDrawBuffer((isDoubleBuffer() && !p_renderArea->isDrawToFrontBuffer())
                   ? GL_BACK : GL_FRONT);
  }
  // else not stereo viewing, so do the regular rendering....
  else
    doRendering();

#ifdef SOWXDEBUG
  cerr << "Leave SoWxViewer::actualRedraw()\n";
#endif
}


/**
 * Change the values of our camera to newCamera.
 *??? animate from old values to new?
 */
void SoWxViewer::changeCameraValues(SoCamera *newCamera)
{
  if (p_camera == NULL)
    return;
  
  // only paste cameras of the same type
  if (p_camera->getTypeId() != newCamera->getTypeId())
    return;
  
  // give our camera the values of the new camera
  p_camera->position = newCamera->position;
  p_camera->orientation = newCamera->orientation;
  p_camera->nearDistance = newCamera->nearDistance;
  p_camera->farDistance = newCamera->farDistance;
  p_camera->focalDistance = newCamera->focalDistance;

  // get the height or heightAngle
  if (p_camera->isOfType(SoPerspectiveCamera::getClassTypeId()))
    ((SoPerspectiveCamera *)p_camera)->heightAngle = 
      ((SoPerspectiveCamera *)newCamera)->heightAngle;
  else
    ((SoOrthographicCamera *)p_camera)->height = 
      ((SoOrthographicCamera *)newCamera)->height;
}


/**
 * Toggles the current camera type (perspective <--> orthographic)
 */
void SoWxViewer::toggleCameraType()
{
  if (p_camera == NULL)
    return;
  
  // create the camera of the opposite kind and compute the wanted height
  // or heightAngle of the new camera.
  SoCamera *newCam;
  if (p_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
    float angle = ((SoPerspectiveCamera *)p_camera)->heightAngle.getValue();
    float height = p_camera->focalDistance.getValue() * tanf(angle/2);
    newCam = new SoOrthographicCamera;
    ((SoOrthographicCamera *)newCam)->height = 2 * height;
  }
  else if (p_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
    float height = ((SoOrthographicCamera *)p_camera)->height.getValue() / 2;
    float angle = atanf(height / p_camera->focalDistance.getValue());
    newCam = new SoPerspectiveCamera;
    ((SoPerspectiveCamera *)newCam)->heightAngle = 2 * angle;
  }
  else {
#ifdef DEBUG
    SoDebugError::post("SoWxViewer::toggleCameraType",
                       "unknown camera type!");
#endif
    return;
  }
  
  newCam->ref();
  
  // copy common stuff from the old to the new camera
  newCam->viewportMapping = p_camera->viewportMapping.getValue();
  newCam->position = p_camera->position.getValue();
  newCam->orientation = p_camera->orientation.getValue();
  newCam->aspectRatio = p_camera->aspectRatio.getValue();
  newCam->focalDistance = p_camera->focalDistance.getValue();
    
  // search for the old camera and replace it by the new camera
  SoSearchAction sa;
  sa.setNode(p_camera);
  sa.apply(p_sceneRoot);
  SoFullPath *fullCamPath = (SoFullPath *) sa.getPath();
  if (fullCamPath) {
    SoGroup *parent = (SoGroup *)fullCamPath->getNode(fullCamPath->getLength() - 2);
    parent->insertChild(newCam, parent->findChild(p_camera));
    SoCamera *oldCam = p_camera;
    setCamera(newCam);
	
    // remove the old camera if it is still there (setCamera() might
    // have removed it) and set the created flag to true (for next time)
    if (parent->findChild(oldCam) >= 0)
      parent->removeChild(oldCam);
    p_createdCamera = true;
  }
#ifdef DEBUG
  else
    SoDebugError::post("SoWxViewer::toggleCameraType",
                       "camera not found!");
#endif
  
  newCam->unref();
}


/**
 * Sets the default ortho projection when doing viewer feedback. The
 * zbuffer/lighting is automatically turned off.
 */
void SoWxViewer::setFeedbackOrthoProjection(const SbVec2s &size)
{
  // push the gl state to revert it back later....
  glPushAttrib(GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT | GL_LINE_BIT);
  
  // ??? should we worry about restoring this matrix later ?
  glViewport(0, 0, size[0], size[1]);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, size[0], 0, size[1], -1, 1);
  
  // disable zbuffer and lighting....
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
}


/**
 * Restores the state that was changed when setFeedbackOrthoProjection()
 * is called.
 */
void SoWxViewer::restoreGLStateAfterFeedback()
{
  // restore the gl state that were saved in setFeedbackOrthoProjection()
  glPopAttrib();
}


/**
 * Draws a simple 2 colored cross at the given location.
 */
void SoWxViewer::drawViewerCrossFeedback(SbVec2s loc)
{
  LIGHT_COLOR;
  glLineWidth(4);
  glBegin(GL_LINES);
  glVertex2s(loc[0]-CROSS, loc[1]);
  glVertex2s(loc[0]+CROSS, loc[1]);
  glVertex2s(loc[0], loc[1]-CROSS);
  glVertex2s(loc[0], loc[1]+CROSS);
  glEnd();
  
  DARK_COLOR;
  glLineWidth(2);
  glBegin(GL_LINES);
  glVertex2s(loc[0]-CROSS+1, loc[1]);
  glVertex2s(loc[0]+CROSS-1, loc[1]);
  glVertex2s(loc[0], loc[1]-CROSS+1);
  glVertex2s(loc[0], loc[1]+CROSS-1);
  glEnd();
}


/**
 * Draws the anchor roll feedback given the point of rotation and the
 * current mouse location.
 */
void SoWxViewer::drawViewerRollFeedback(SbVec2s center, SbVec2s loc)
{
  // get angle and distance of mouse from center of rotation
  float ang, dist;
  float vx = loc[0] - center[0];
  float vy = loc[1] - center[1];
  if (vx==0 && vy==0) {
    ang = 0;
    dist = 3; // minimum size (given the circle thickness)
  }
  else {
    ang = atan2(vy, vx) * 180.0 / M_PI;
    dist = sqrtf(vx*vx + vy*vy);
    if (dist < 3)
      dist = 3; // minimum size (given the circle thickness)
  }
  float cirAng = -ang + 90; // gluPartialDisk() angle is REALLY backward !!
  
  static GLUquadricObj *quad = NULL;
  if (! quad)	quad = gluNewQuadric();
  
  // draw all of the circles (first inner, then outer)
  glTranslatef(center[0], center[1], 0);
  LIGHT_COLOR;
  gluDisk(quad, RADIUS, RADIUS+LINE_THICK, 20, 2);
  gluPartialDisk(quad, dist-2, dist+LINE_THICK-2, 20, 2, cirAng - ANGLE_LEN, 2 * ANGLE_LEN);
  DARK_COLOR;
  gluDisk(quad, RADIUS+1, RADIUS+LINE_THICK-1, 20, 2);
  gluPartialDisk(quad, dist-1, dist+LINE_THICK-3, 20, 2, cirAng - ANGLE_LEN, 2 * ANGLE_LEN);
  glTranslatef(-center[0], -center[1], 0); // undo the translation
  
  // draw connecting line from center to outer circle
  glLineWidth(LINE_THICK);
  LIGHT_COLOR;
  glBegin(GL_LINES);
  glVertex2s(center[0], center[1]);
  glVertex2s(loc[0], loc[1]);
  glEnd();
  glLineWidth(LINE_THIN);
  DARK_COLOR;
  glBegin(GL_LINES);
  glVertex2s(center[0], center[1]);
  glVertex2s(loc[0], loc[1]);
  glEnd();
  
  // draw the CCW arrow
  glPushMatrix();
  glTranslatef(center[0], center[1], 0);
  glRotatef(ang+ANGLE_LEN, 0, 0, 1);
  glTranslatef(dist, 0, 0);
  DRAW_ARROW_MACRO;
  glPopMatrix();
  
  // draw the CW arrow
  glPushMatrix();
  glTranslatef(center[0], center[1], 0);
  glRotatef(ang-ANGLE_LEN, 0, 0, 1);
  glTranslatef(dist, 0, 0);
  glScalef(1, -1, 1);
  DRAW_ARROW_MACRO;
  glPopMatrix();
}


/**
 * @todo When is this called?
 * This is called when we are first mapped - this will check the
 * configuration of the machine we are running on and decide on what
 * draw style we should pick.
 */
void SoWxViewer::afterRealizeHook()
{
  // @todo Is this a GLWidget func?
  // call the base class
  // SoWxRenderArea::afterRealizeHook();
  
  // only do this once and only IF the user hasn't overwritten this
  if (! p_checkForDrawStyle)
    return;
  p_checkForDrawStyle = false;
    
  SbBool useTexture = true; // true by default (and for new machines)
  const char *renderer = (const char *) glGetString(GL_RENDERER);
  
  // On the following SGI machines we don't want texture rendering 
  // to be turned on by default (for speed reason) - machines not listed
  // below are assumed to be fast enough to leave VIEW_AS_IS draw style.
  
  if (strncmp((const char *)glGetString(GL_VENDOR), "SGI", 3) == 0) {
    // Indy and XL
    if (useTexture && strncmp(renderer, "NEWPORT", 7) == 0)
      useTexture = false;
    // Personal Iris
    if (useTexture && strncmp(renderer, "GR1", 3) == 0)
      useTexture = false;
    // VGX and VGXT
    if (useTexture && strncmp(renderer, "VGX", 3) == 0)
      useTexture = false;
    // Indigo Entry
    if (useTexture && (strncmp(renderer, "LG1", 3) == 0 ||
        strncmp(renderer, "LIGHT", 5) == 0))
      useTexture = false;
    // XS, XZ, Elan, and Extreme
    if (useTexture && (strncmp(renderer, "GR2", 3) == 0 || 
                       strncmp(renderer, "GR3", 3) == 0 ||
                       strncmp(renderer, "GU1", 3) == 0))
      useTexture = false;
  }
  
  if (! useTexture)
    setDrawStyle(SoWxViewer::INTERACTIVE, SoWxViewer::VIEW_NO_TEXTURE);
}


/**
 * Adjust the camera clipping planes based on the scene bounding 
 * box. (called before every redraws)
 */
void SoWxViewer::adjustCameraClippingPlanes()
{
  if (p_camera == NULL)
    return;
  
  // get the scene bounding box
  p_autoClipBboxAction->setViewportRegion(SbViewportRegion(getGlxSize()));
  p_autoClipBboxAction->apply(p_sceneRoot);

  SbXfBox3f xfbbox = p_autoClipBboxAction->getXfBoundingBox();
    
  // get camera transformation and apply to xfbbox
  // to align the bounding box to camera space.
  // This will enable us to simply use the z values of the
  // transformed bbox for near and far plane values.
  SbMatrix mx;
  mx.setTranslate(- p_camera->position.getValue());
  xfbbox.transform(mx);
  mx = p_camera->orientation.getValue().inverse();
  xfbbox.transform(mx);
  
  // get screen align bbox and figure the near and far plane values
  SbBox3f bbox = xfbbox.project();
  // take negative value and opposite to what one might think 
  // because the camera points down the -Z axis
  float far = - bbox.getMin()[2];
  float near = - bbox.getMax()[2];
  
  // scene is behind the camera so don't change the planes
  if (far < 0)
    return;
  
  // check for minimum near plane value (Value will be negative 
  // when the camera is inside the bounding box).
  // Note: there needs to be a minimum near value for perspective
  // camera because of zbuffer resolution problem (plus the values
  // has to be positive). There is no such restriction for
  // an Orthographic camera (you can see behind you).
  if (! p_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
    if (near < (p_minimumNearPlane * far))
      near = p_minimumNearPlane * far;
  }
  
  // give the near and far distances a little bit of slack in case
  // the object lies against the bounding box, otherwise the object
  // will be poping in and out of view.
  // (example: a cube is the same as it's bbox)
  near *= 0.999;
  far *= 1.001;
  
  // finally assign camera plane values
  if (p_camera->nearDistance.getValue() != near)
    p_camera->nearDistance = near;
  if (p_camera->farDistance.getValue() != far)
    p_camera->farDistance = far;
}


/**
 * Sets the current drawing style. This only changes the nodes to
 * match what is passed (called from multiple places) and doesn't
 * affect the current state.
 */
void SoWxViewer::setCurrentDrawStyle(SoWxViewer::DrawStyle style)
{
  if (style != VIEW_AS_IS)
    p_drawStyleSwitch->whichChild = SO_SWITCH_ALL;
    
  switch(style) {
  case VIEW_AS_IS:
    p_drawStyleSwitch->whichChild = SO_SWITCH_NONE;
    break;
	    
  case VIEW_HIDDEN_LINE:
    // texture is always off under the switch node.
    // List only stuff common to both rendering passes
    // (the rest is done when rendering)
    p_drawStyleNode->style.setIgnored(FALSE);
    p_drawStyleNode->pointSize.setIgnored(TRUE);
    p_lightModelNode->model = SoLightModel::BASE_COLOR;
    p_lightModelNode->model.setIgnored(FALSE);
    p_complexityNode->type.setIgnored(TRUE);
    p_complexityNode->value.setIgnored(TRUE);
    break;
	    
  case VIEW_NO_TEXTURE:
  case VIEW_LOW_COMPLEXITY:
    // texture is always off under the switch node
    p_drawStyleNode->style.setIgnored(TRUE);
    p_drawStyleNode->pointSize.setIgnored(TRUE);
    p_lightModelNode->model.setIgnored(TRUE);
    p_colorNode->orderedRGBA.setIgnored(TRUE);
    p_matBindingNode->value.setIgnored(TRUE);
    p_complexityNode->type.setIgnored(TRUE);
    p_complexityNode->value.setIgnored(style != VIEW_LOW_COMPLEXITY);
    break;
	    
  case VIEW_LINE:
  case VIEW_LOW_RES_LINE:
  case VIEW_POINT:
  case VIEW_LOW_RES_POINT:
    // texture is always off under the switch node
    p_drawStyleNode->style = (style==VIEW_LINE || style==VIEW_LOW_RES_LINE)? 
      SoDrawStyle::LINES : SoDrawStyle::POINTS;
    p_drawStyleNode->style.setIgnored(FALSE);
    p_drawStyleNode->pointSize.setIgnored(style != VIEW_POINT &&
                                          style != VIEW_LOW_RES_POINT);
    p_lightModelNode->model = SoLightModel::BASE_COLOR;
    p_lightModelNode->model.setIgnored(FALSE);
    p_colorNode->orderedRGBA.setIgnored(TRUE);
    p_matBindingNode->value.setIgnored(TRUE);
	    
    // Force a lower complexity for the low res draw styles
    // ??? this only works if the object didn't have
    // ??? something lower in the first place...
    if (style == VIEW_LOW_RES_LINE || style == VIEW_LOW_RES_POINT) {
      p_complexityNode->type = SoComplexity::OBJECT_SPACE;
      p_complexityNode->type.setIgnored(FALSE);
      p_complexityNode->value.setIgnored(FALSE);
    }
    else {
      p_complexityNode->type.setIgnored(TRUE);
      p_complexityNode->value.setIgnored(TRUE);
    }
    break;
	    
  case VIEW_BBOX:
    // texture is always off under the switch node
    p_drawStyleNode->style = SoDrawStyle::LINES;
    p_drawStyleNode->style.setIgnored(FALSE);
    p_drawStyleNode->pointSize.setIgnored(TRUE);
    p_lightModelNode->model = SoLightModel::BASE_COLOR;
    p_lightModelNode->model.setIgnored(FALSE);
    p_colorNode->orderedRGBA.setIgnored(TRUE);
    p_matBindingNode->value.setIgnored(TRUE);
    p_complexityNode->type = SoComplexity::BOUNDING_BOX;
    p_complexityNode->type.setIgnored(FALSE);
    p_complexityNode->value.setIgnored(TRUE);
    break;
	    
  case VIEW_SAME_AS_STILL:
#ifdef DEBUG
    SoDebugError::post("SoWxViewer::setCurrentDrawStyle", "VIEW_SAME_AS_STILL was passed !");
#endif
    break;
  }
    
  setZbufferState();
}


/**
 * Do a multiple pass rendering if necessary, else simply call
 * SoWxRenderAre::actualRedraw() method.
 */
void SoWxViewer::doRendering()
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxViewer::doRendering()\n";
#endif

  // check if we need two pass rendering for hidden line rendering
  SbBool drawHiddenLine = 
    (p_stillDrawStyle == VIEW_HIDDEN_LINE && 
     (! p_interactiveFlag ||
      p_interactiveDrawStyle == VIEW_NO_TEXTURE ||
      p_interactiveDrawStyle == VIEW_LOW_COMPLEXITY ||
      p_interactiveDrawStyle == VIEW_SAME_AS_STILL)) 
    || (p_interactiveFlag && p_interactiveDrawStyle == VIEW_HIDDEN_LINE);
  
  if (p_camera != NULL && drawHiddenLine) {
    
    // ??? what do we do about highlights ??
    
    // the smaller the near clipping plane is relative to the far
    // plane, the smaller the zbuffer offset needs to be (because
    // the granularity will be pretty big). The closer the clipping
    // planes are relative to each other, the bigger the zbuffer offset
    // needs to be (because the zbuffer granularity will be small).
    // The scale factor was found empirically to work best with the
    // current settings of near/far.
    float zOffset = p_camera->nearDistance.getValue() / 
      (40 * p_camera->farDistance.getValue());
    
    // render the first pass as solid, using the background color
    // for the object base color.
    p_drawStyleNode->style = SoDrawStyle::FILLED;
    p_colorNode->orderedRGBA = getBackgroundColor().getPackedValue();
    p_colorNode->orderedRGBA.setIgnored(FALSE);
    p_matBindingNode->value.setIgnored(FALSE);
	
    // ??? this should match the SoXtRenderArea::actualRedraw()
    // ??? method exactly (apart for not clearing the z-buffer)
    glDepthRange(zOffset, 1); // enable wireframe to be draw on top
    getSceneManager()->render(isClearBeforeRender(), TRUE);

    // render the second pass as wireframe
    // (the first pass rendered the objects solid with base color
    // set to the background color to set the zbuffer values)
    p_drawStyleNode->style = SoDrawStyle::LINES;
    p_colorNode->orderedRGBA.setIgnored(TRUE);
    p_matBindingNode->value.setIgnored(TRUE);
	
    // ??? this should match the SoXtRenderArea::actualRedraw()
    // ??? method exactly (apart for not clearing the color and z-buffer)
    glDepthRange(0,1-zOffset); // enable wireframe to be draw on top
    getSceneManager()->render(FALSE, FALSE);
	
    glDepthRange(0, 1); // restore the range
  }
  else {
    // ??? this should match the SoXtRenderArea::actualRedraw()
    // ??? method exactly (apart for not clearing the z-buffer)
    getSceneManager()->render(isClearBeforeRender(), ! isZbufferOff());
  }

#ifdef SOWXDEBUG
  cerr << "Leave SoWxViewer::doRendering()\n";
#endif
}



/**
 * This is called by Xt when the data is ready to be pasted.
 */
void SoWxViewer::pasteDoneCB(void *userData, SoPathList *pathList)
{
  SoCamera *newCamera = NULL;
  
  // search for a camera in the paste data
  for (int i = 0; i < pathList->getLength(); i++) {
    SoFullPath *fullP = (SoFullPath *) (*pathList)[i];
    if (fullP->getTail()->isOfType(SoCamera::getClassTypeId())) {
      newCamera = (SoCamera *) fullP->getTail();
      break;
    }
  }
  
  if (newCamera != NULL)
    ((SoWxViewer *) userData)->changeCameraValues(newCamera);
  
  // We delete the callback data when done with it.
  delete pathList;
}


/**
 * Called whenever the seek animation sensor fires. Finds the amount 
 * of time since we started the seek and call the subclasses routine
 * to do the correct interpolation.
 */
void SoWxViewer::seekAnimationSensorCB(void *p, SoSensor *)
{
  SoWxViewer *v = (SoWxViewer *)p;
  
  // get the time difference
  SbTime time = p_viewerRealTime->getValue();
  float sec = float((time - v->p_seekStartTime).getValue());
  if (sec == 0.0)
    sec = 1.0/72.0;	// at least one frame (needed for first call)
  float t = (sec / v->p_seekAnimTime);
  
  // check to make sure the values are correctly clipped
  if (t > 1.0)
    t = 1.0;
  else if ((1.0 - t) < 0.0001)
    t = 1.0; // this will be the last one...
  
  // call subclasses to interpolate the animation
  v->interpolateSeekAnimation(t);
  
  // stops seek if this was the last interval
  if (t == 1.0)
    v->setSeekMode(FALSE);
}


/**
 *
 */
void SoWxViewer::drawStyleStartCallback(void *, SoWxViewer *v)
{
  v->p_interactiveFlag = TRUE;  // must happen first
  
  // if still and move draw styles are the same, return...
  if (v->p_interactiveDrawStyle == VIEW_SAME_AS_STILL ||
      v->p_interactiveDrawStyle == v->p_stillDrawStyle)
    return;
    
  // if we have "MOVE NO TEXTURE", then we have nothing
  // to do if we have a current draw style (since they all have 
  // texturing turned off in the first place).
  if (v->p_interactiveDrawStyle == VIEW_NO_TEXTURE &&
      v->p_stillDrawStyle != VIEW_AS_IS)
    return;
    
  v->setCurrentDrawStyle(v->p_interactiveDrawStyle);
}


/**
 *
 */
void SoWxViewer::drawStyleFinishCallback(void *, SoWxViewer *v)
{
  v->p_interactiveFlag = FALSE;  // must happen first
  
  // if still and move draw styles are the same, return...
  if (v->p_interactiveDrawStyle == VIEW_SAME_AS_STILL ||
      v->p_interactiveDrawStyle == v->p_stillDrawStyle)
    return;
  
  // if we have "MOVE NO TEXTURE", then we have nothing
  // to do if we have a current draw style (since they all have 
  // texturing turned off in the first place).
  if (v->p_interactiveDrawStyle == VIEW_NO_TEXTURE &&
      v->p_stillDrawStyle != VIEW_AS_IS)
    return;
  
  v->setCurrentDrawStyle(v->p_stillDrawStyle);
}


/**
 *
 */
void SoWxViewer::bufferStartCallback(void *, SoWxViewer *v)
{
  v->p_renderArea->setDoubleBuffer(true);
}


/**
 *
 */
void SoWxViewer::bufferFinishCallback(void *, SoWxViewer *v)
{
  v->p_renderArea->setDoubleBuffer(false);
}


/**
 * Sets the zbuffer state on the current window. This is called whenever
 * the windows changes (called by SoXtGLWidget::widgetChanged()) or when
 * the viewer draw style changes.
 */
void SoWxViewer::setZbufferState()
{
  // @todo Fix it.
  //  if (getNormalWindow() == (Window) NULL)
  //    return;
  
  //  glXMakeCurrent(getDisplay(), getNormalWindow(), getNormalContext());
  
  if (isZbufferOff())
    glDisable(GL_DEPTH_TEST);
  else
    glEnable(GL_DEPTH_TEST);
}


/**
 * Returns TRUE if the zbuffer should be off (based on the viewer
 * draw styles).
 */
SbBool SoWxViewer::isZbufferOff()
{
  DrawStyle style = (p_interactiveFlag ?
                     p_interactiveDrawStyle : p_stillDrawStyle);
  if (p_interactiveFlag && p_interactiveDrawStyle == VIEW_SAME_AS_STILL)
    style = p_stillDrawStyle;
  
  // for these draw styles, turn the zbuffer off
  return (style == VIEW_LOW_RES_LINE || style == VIEW_LOW_RES_POINT 
          || style == VIEW_BBOX);
}


/**
 * Called by the processCommonEvent routine whenever the arrow keys
 * are pressed. Translate the camera in the viewing plane in the arrow
 * direction half a screen at a time.
 */
void SoWxViewer::arrowKeyPressed(KeySym key)
{
  if (p_camera == NULL)
    return;
    
  // get the camera near plane height value
  float dist;
  if (p_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
    float angle = ((SoPerspectiveCamera *)p_camera)->heightAngle.getValue();
    float length = p_camera->nearDistance.getValue();
    dist = length * tanf(angle);
  }
  else if (p_camera->isOfType(SoOrthographicCamera::getClassTypeId()))
    dist = ((SoOrthographicCamera *)p_camera)->height.getValue();
  dist /= 2.0;
    
  // get camera right/left/up/down direction
  SbMatrix mx;
  mx = p_camera->orientation.getValue();
  SbVec3f dir;
  /* @todo Fix it.
     switch(key) {
     case XK_Up:
     dir.setValue(mx[1][0], mx[1][1], mx[1][2]);
     break;
     case XK_Down:
     dir.setValue(-mx[1][0], -mx[1][1], -mx[1][2]); 
     break;
     case XK_Right:
     dir.setValue(mx[0][0], mx[0][1], mx[0][2]);
     dist *= camera->aspectRatio.getValue();
     break;
     case XK_Left:
     dir.setValue(-mx[0][0], -mx[0][1], -mx[0][2]);
     dist *= camera->aspectRatio.getValue();
     break;
     }
  */
  // finally reposition the camera
  p_camera->position = p_camera->position.getValue() + dist * dir;
}





//////////////////////////////////////////
// Forwarding funcs from SoWxRenderArea and GLWidget
const SbVec2s & SoWxViewer::getGlxSize() const
{
  return p_renderArea->getGlxSize();
}


const SbViewportRegion & SoWxViewer::getViewportRegion() const
{
  // What if renderarea is not initialized?
  return p_renderArea->getViewportRegion();
}


SbBool SoWxViewer::isDoubleBuffer()
{
  if (p_renderArea)
    return p_renderArea->isDoubleBuffer();
  else
    return true;
}


void SoWxViewer::registerDevice(SoWxDevice *device)
{
  if (p_renderArea)
    p_renderArea->registerDevice(device);
}


void SoWxViewer::unregisterDevice(SoWxDevice *device)
{
  if (p_renderArea)
    p_renderArea->unregisterDevice(device);
}


SoSceneManager * SoWxViewer::getSceneManager() const
{
  return p_renderArea->getSceneManager();
}


/**
 * Get clearFirst flag.
 */
SbBool SoWxViewer::isClearBeforeRender() const
{
  return p_renderArea->isClearBeforeRender();
}


/**
 * SoWxFullViewer function:
 *
 * Show/hide the viewer component trims (default ON)
 */
void SoWxViewer::setDecoration(bool onOrOff)
{
  if (onOrOff == p_decorationFlag)
    return;

  p_topSizer->Show((size_t)0, onOrOff);
  p_topSizer->Show((size_t)2, onOrOff);
  p_centerSizer->Show((size_t)1, onOrOff);

  p_decorationFlag = onOrOff;
}


/**
 * Component trim status
 */
SbBool SoWxViewer::isDecoration()
{
  return p_decorationFlag;
}


/**
 * Sets the camera given zoom value (in degree for perspective cameras).
 */
void SoWxViewer::setCameraZoom(float zoom)
{
  if (p_camera == NULL)
    return;
  
  if ( p_camera->isOfType(SoPerspectiveCamera::getClassTypeId()) )
    ((SoPerspectiveCamera *)p_camera)->heightAngle = zoom * M_PI / 180.0;
  else if ( p_camera->isOfType(SoOrthographicCamera::getClassTypeId()) )
    ((SoOrthographicCamera *)p_camera)->height = zoom;
#if DEBUG
  else
    SoDebugError::post("SoXtFullViewer::setCameraZoom",
                       "unknown camera type");
#endif
}


/**
 * Gets the camera current zoom value. The value is returned in degrees
 * for a perspective camera.
 */
float SoWxViewer::getCameraZoom()
{
  if (p_camera == NULL)
    return 0;
  
  if ( p_camera->isOfType(SoPerspectiveCamera::getClassTypeId()) )
    return ((SoPerspectiveCamera *)p_camera)->heightAngle.getValue() *
      180.0 / M_PI;
  else if ( p_camera->isOfType(SoOrthographicCamera::getClassTypeId()) )
    return ((SoOrthographicCamera *)p_camera)->height.getValue();
  else {
#if DEBUG
    SoDebugError::post("SoXtFullViewer::getCameraZoom",
                       "unknown camera type");
#endif
    return 0;
  }
}


/**
 * 
 */
void SoWxViewer::processEvent(wxEvent * event)
{
  p_renderArea->processEvent(event);
}


/**
 * 
 */
void SoWxViewer::setAntialiasing(SbBool smoothing, int numPasses)
{
  p_renderArea->setAntialiasing(smoothing, numPasses);
}


/**
 *
 */
void SoWxViewer::getAntialiasing(SbBool &smoothing, int &numPasses) const
{
  p_renderArea->getAntialiasing(smoothing, numPasses);
}


/**
 * Set the window background color when in RGB mode.
 * (default to black (0,0,0));
 */
void SoWxViewer::setBackgroundColor(const SbColor &c)
{
  p_renderArea->setBackgroundColor(c);
}


/**
 * Get the window background color when in RGB mode.
 */
const SbColor & SoWxViewer::getBackgroundColor() const
{
  return p_renderArea->getBackgroundColor();
}
