/**
 * @file
 *
 */
//#include <iostream>
// using namespace std;

#include "inv/sensors/SoFieldSensor.H"
#include "inv/fields/SoSFTime.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoGroup.H"
#include "inv/nodes/SoCallback.H"
#include "inv/nodes/SoCamera.H"
#include "inv/nodes/SoPerspectiveCamera.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/elements/SoGLLazyElement.H"

#include "wxgui/ewxCursor.H"

#include "inv/SoWx/SoWxExaminerViewer.H"
#include "inv/SoWx/SoWxRenderArea.H"

#include "viz/freeglut_font_data.H"
#include "viz/EcceDispParam.H"
#include "viz/VR3dXVSmouse.H"

#include "wxviz/SGViewer.H"
#include "wxviz/SGSelection.H"
#include "wxviz/MouseEventListener.H"


#include <iostream>
using std::cerr;
using std::endl;


/////////////////////////////////////////////////////////////////////////////
// Class statics
/////////////////////////////////////////////////////////////////////////////

// list of the different popup choices
enum popupChoices {
  VIEW_ALL = 20, 
  SET_HOME,
  HOME_1,
  HEADLIGHT,
  SEEK,
  PREF,
  VIEWING,
  DECORATION,
  COPY_VIEW,
  PASTE_VIEW,
  HELP
};

enum drawChoices {
  AS_IS,
  HIDDEN_LINE,
  NO_TXT,
  LOW_RES,
  LINE,
  POINT,
  BBOX,

  MOVE_SAME_AS,
  MOVE_NO_TXT,
  MOVE_LOW_RES,
  MOVE_LINE,
  MOVE_LOW_LINE,
  MOVE_POINT,
  MOVE_LOW_POINT,
  MOVE_BBOX,

  DRAW_STYLE_NUM // specify the length
};

// list of the toggle buttons in the popumenu
enum popupToggles {
  HEADLIGHT_WIDGET = 0,    // very convenient to start at 0
  VIEWING_WIDGET,
  DECORATION_WIDGET,

  POPUP_TOGGLE_NUM   // specify the length
};

// list of custom push buttons
enum ViewerPushButtons {
  PICK_PUSH,
  VIEW_PUSH,
  HELP_PUSH,
  HOME_PUSH,
  SET_HOME_PUSH,
  VIEW_ALL_PUSH,
  SEEK_PUSH,

  PUSH_NUM
};

enum ZoomSliderVars {
  ZOOM_LABEL,
  ZOOM_SLIDER,
  ZOOM_FIELD,
  ZOOM_RANGE_LAB1,
  ZOOM_RANGE_FIELD1,
  ZOOM_RANGE_LAB2,
  ZOOM_RANGE_FIELD2,

  ZOOM_NUM
};



/**
 * To be filled in with ecce viewer customized code here
 */
SGViewer::SGViewer( wxWindow* parent, wxWindowID id,
                    const wxPoint& pos, const wxSize& size,
                    long style, Type type, SoCamera *cameraToCopy ):
  SoWxExaminerViewer( parent, id, pos, size, style, type )
{
  p_showText = true;

  // Turn off trim
  setDecoration(false);

  p_zWheelFirstDrag = true;
  p_stickyMode = p_mode;

  p_newRoot = new SoGroup;
  p_newRoot->ref();

  // I struggled to figure out how to change the camera.  From looking
  // at MI source, the camera type must be set before the scenegraph is
  // set in order for the setCameraType to have an effect.  So I added
  // this flag to the constructor.  I searched through MI source and
  // could not find the ortho/perspective camera toggle that the viewers
  // have and thus I couldn't figure out how they were doing the change 
  // dynamically.
  if (cameraToCopy != 0) {
    setCameraType(cameraToCopy->getTypeId());
  }
  SoWxExaminerViewer::setSceneGraph(p_newRoot);
  if (cameraToCopy != 0) {
    copyCamera(getCamera(), cameraToCopy);
  }

  //  the top for the 3D Mouse
  p_topMickey = new SoSeparator;
  p_newRoot->addChild(p_topMickey);
  set3DMouse(false);

  //  the top for the Axis
#ifdef corneraxis
  p_topAxis = new NearPlaneSep;
#else
  p_topAxis = new SoSeparator;
#endif
  p_newRoot->addChild(p_topAxis);


  //  SGSelection 
  p_ecceSel = new SGSelection();
  p_newRoot->addChild(p_ecceSel);
  p_ecceSel->parts.setValue(SGSelection::ATOMS | SGSelection::BONDS);
  p_ecceSel->lassoType.setValue(SGSelection::DRAGGER);
  //p_ecceSel->radiusSelect.setValue(TRUE);
  p_ecceSel->radiusSelect.setValue(false);
  // @todo This line was causing core dump. Not sure why. Need fix.
  //  p_ecceSel->radiusSelectSphereDrawStyle.setValue(SGSelection::LINES);
  p_ecceSel->atomLassoPolicy.setValue(SGSelection::THRU_BBOX); 
  p_ecceSel->bondLassoPolicy.setValue(SGSelection::ENTIRE_BBOX);
  p_ecceSel->setViewer(this);
  p_ecceSel->togglePolicy = ChemSelection::SHIFT;
  // @todo
  //setOverlaySceneGraph(p_ecceSel->getSceneGraph());

  p_topRoot = new SoGroup;
  p_ecceSel->addChild(p_topRoot);


  p_myCallback = (SoCallback*)0;

  // Creating the Preferences
  //  setTitle("VR_Viewer");
#ifndef __COIN
  //  setPrefSheetString("Molecular Viewer Preference Sheet");
#endif
  //  setPopupMenuString("");
  //  setPopupMenuEnabled(FALSE);
  //  setBottomWheelString("Y");
  //  setLeftWheelString("X");
  //  setRightWheelString("Zoom");


  // Set up the overlay color map

#if 000
  // Create a lasso color and set the viewer's overlay color
  // table to have this color at index 1.  Take this color
  // from EcceDispParam for easy maintenace.
  float r,g,b;
  EcceDispParam options;
  options.getLasso(r,g,b);
  SbColor lasso(r, g, b);

  //  setOverlayColorMap(1, 1, &lasso);

  //  p_autozclip = 0;

  //  createZRotator();

  //  if (p_autozclip != 0)
  //    autoclipValueChangedCB(p_autozclip, this, 0);
#endif

  initializeText();

}


SGViewer::~SGViewer()
{
}


/**
 * This method SHOULD be in SoWxViewer but its too much trouble 
 * to put it there now.
 */
void SGViewer::setTransparencyType(SoGLRenderAction::TransparencyType type)
{
  //p_renderArea->setTransparencyType(SoGLRenderAction::DELAYED_ADD);
  //p_renderArea->setTransparencyType(SoGLRenderAction::SCREEN_DOOR);
  p_renderArea->setTransparencyType(type);
}



/**
 * Assigns camera attributes of this camera to be the same as src camera.
 * Does not handle the icon for the camera style.
 */
void SGViewer::assignCamera(SGViewer *src)
{
  // NOTE it does no good to call setCameraType here if a call to
  // setSceneGraph has already been made. See constructor documentation
  // for workaround.
  copyCamera(getCamera(), src->getCamera());
}


/**
 * see whether a particular extension is supported.  Lifted from example
 * code.
 */
bool SGViewer::checkExtension(const char *extName, const char *extString)
{
  char *p = (char*) extString;
  int  extNameLen = strlen(extName);
  char *end = p + strlen(p);
  
  while (p < end) {
    int n = strcspn(p, " ");
    if ((extNameLen == n) && (strncmp(extName, p, n) == 0)) {
      return true;
    }
    p += n + 1;
  }
  return false;
}



string SGViewer::commandObjectType() const
{
  return "SGViewer";
}


void SGViewer::setSceneGraph(SoNode* node)
{
  p_topRoot->removeAllChildren();
  p_topRoot->addChild(node);
}


SoNode * SGViewer::getSceneGraph()
{
  return p_topRoot;
}


SoNode * SGViewer::getTopNode()
{
#ifdef __COIN
  return getSceneGraph();
#else
  return p_sceneRoot; 
#endif
}


void SGViewer::set3DMouse(SbBool on)
{
  if (on) {
    p_mouse3d = new VR3dXVSmouse;
    p_topMickey->addChild(p_mouse3d);
  }
  else {
    p_topMickey->removeAllChildren();
  }
}


SoSeparator * SGViewer::getAxisRoot()
{
  return p_topAxis;
}


RTDraggerNode * SGViewer::getMickey()
{
  RTDraggerNode * drg = p_mouse3d->getDragger(); 
  return drg; 
}


SGSelection * SGViewer::getSel()
{
  return p_ecceSel;
}


void SGViewer::initializeText()
{
  p_cornerText.clear();
  p_cornerText.push_back("");
  p_cornerText.push_back("");
  p_cornerText.push_back("");
  p_cornerText.push_back("");
  p_myCallback = new SoCallback;
  p_newRoot->addChild(p_myCallback);
  p_myCallback->setCallback(&SGViewer::myCallbackRoutine, (void*)this);
  showTextLabels(true);
}



void SGViewer::setText(const string& lleft, 
                       const string& lright, 
                       const string& uleft, 
                       const string& uright)
{
   p_cornerText[0] = lleft;
   p_cornerText[1] = lright;
   p_cornerText[2] = uleft;
   p_cornerText[3] = uright;
  if (p_myCallback) {
    p_myCallback->touch();
  }
}


void SGViewer::setLLeftText(const string& txt)
{
   p_cornerText[0] = txt;
  if (p_myCallback) {
    p_myCallback->touch();
    p_newRoot->touch();
  }
}


void SGViewer::setLRightText(const string& txt)
{
   p_cornerText[1] = txt;
  if (p_myCallback) {
    p_myCallback->touch();
    p_newRoot->touch();
  }
}


void SGViewer::setULeftText(const string& txt)
{
   p_cornerText[2] = txt;
  if (p_myCallback) {
    p_myCallback->touch();
    p_newRoot->touch();
  }
}


void SGViewer::setURightText(const string& txt)
{
   p_cornerText[3] = txt;
  if (p_myCallback) {
    p_myCallback->touch();
    p_newRoot->touch();
  }
}


/**
 * Enable custom near/far clipping and generate some defaults.
 */
void SGViewer::enableCustomZClip(bool set)
{
  setAutoClipping(!set);

  //  p_zcenter->setsensitive(set);
  //  p_zdepth->setsensitive(set);

  // Set some defaults when enabling
  // We decided to have depth be a percent so we'll set it to 100
  if (set) {
    // Get values of near/far and convert to center/depth
    //    float near = p_camera->nearDistance.getValue();
    //    float far = p_camera->farDistance.getValue();
    
    //    int center = near + ((far - near)/2.0);
    //    int depth = 100;
    //    p_zcenter->setrange((int)near, (int)far);
    //    p_zcenter->setvalue(center);
    //    p_zdepth->setvalue(depth);
  }
  updateCustomClipPlanes();
}


void SGViewer::updateCustomClipPlanes()
{
  /*
    int center = p_zcenter->getvalue();
    double depthpercent = p_zdepth->getvalue();
    int near, far;
    p_zcenter->getrange(near,far);
    int depth = (far - near)/ 2.0 * (depthpercent/100.0);
    p_camera->nearDistance.setValue(center-depth);
    p_camera->farDistance.setValue(center +depth);
  */
}


/**
 * Update camera with new rotation vector.  Copied EXACTLY from
 * SoXtExaminerViewer - the method is protected. 
 */
void SGViewer::rotateCamera(const SbRotation &rot)
{
  if (p_camera == NULL)
    return;
  
  // get center of rotation
  SbRotation camRot = p_camera->orientation.getValue();
  float radius = p_camera->focalDistance.getValue();
  SbMatrix mx;
  mx = camRot;
  SbVec3f forward( -mx[2][0], -mx[2][1], -mx[2][2]);
  SbVec3f center = p_camera->position.getValue() + radius * forward;
  
  // apply new rotation to the camera
  camRot = rot * camRot;
  p_camera->orientation = camRot;
  
  // reposition camera to look at pt of interest
  mx = camRot;
  forward.setValue( -mx[2][0], -mx[2][1], -mx[2][2]);
  p_camera->position = center - radius * forward;
}


void SGViewer::copyCamera(SoCamera *dest, const SoCamera *src)
{
  if (src->isOfType(SoOrthographicCamera::getClassTypeId())) {
    SoOrthographicCamera *srco = (SoOrthographicCamera*)src;
    SoOrthographicCamera *desto = (SoOrthographicCamera*)dest;
    desto->height.setValue(srco->height.getValue());
  } else {
    SoPerspectiveCamera *srcp = (SoPerspectiveCamera*)src;
    SoPerspectiveCamera *destp = (SoPerspectiveCamera*)dest;
    destp->heightAngle.setValue(srcp->heightAngle.getValue());
  }
  
  dest->position.setValue(src->position.getValue());
  dest->orientation.setValue(src->orientation.getValue());
  dest->aspectRatio.setValue(src->aspectRatio.getValue());
  dest->nearDistance.setValue(src->nearDistance.getValue());
  dest->farDistance.setValue(src->farDistance.getValue());
  dest->focalDistance.setValue(src->focalDistance.getValue());
  
  //camera->viewportMapping.setValue(src->getCamera()->viewportMapping.getValue());
  //did nothing setViewportRegion(src->getViewportRegion());
}


/**
 * Use simple rules to compute a contrasting foreground color.
 * See http://www.codeproject.com/tips/JbColorContrast.asp?df=100&forumid=39709&exp=0&select=875951#xx875951xx
 */
SbColor SGViewer::calculateContrastingColor()
{
  // The tolerance is a hack to correct for colors around the midpoint
  const int TOLERANCE = 0x40;

  SbColor color = getBackgroundColor();
  //printf("%f %f %f \n",color[0],color[1],color[2]);
  int rt = (int)(color[0] * 255);
  int gt = (int)(color[1] * 255);
  int bt = (int)(color[2] * 255);
  if (rt < 5 && gt < 5 && bt < 5) {
     // Lets use our green color if close to black
     rt = 0; gt = 255; bt = 0;
  } else {
     int crBg = (rt << 16) | (gt << 8) | bt;
     //printf("%x %x %x : %x\n",rt,gt,bt,crBg);
     if (abs(((rt ) & 0xFF) - 0x80) <= TOLERANCE &&
           abs(((gt ) & 0xFF) - 0x80) <= TOLERANCE &&
           abs(((bt ) & 0xFF) - 0x80) <= TOLERANCE

        )  {
        crBg = (0x7F7F7F + crBg) & 0xFFFFFF;
     } else  {
        crBg =  crBg ^ 0xFFFFFF;
     }
     //printf("1shift %x %x %x : %x\n",rt,gt,bt,crBg);
     rt = (crBg & 0xFF0000) >> 16;
     gt = (crBg & 0x00FF00) >> 8;
     bt = (crBg & 0x0000FF);
     //printf("final %x %x %x : %x\n",rt,gt,bt,crBg);
  }

  float r = rt / 255.;
  float g = (gt ) / 255.;
  float b = (bt ) / 255.;
  return SbColor(r,g,b);
}


void SGViewer::myCallbackRoutine(void *userData, SoAction *action)
{
   SGViewer *viewer = (SGViewer*)userData;

  // only render the floor during GLRender actions:
  if(!action->isOfType(SoGLRenderAction::getClassTypeId()))
    return;
  
  static int firstTime = 1;
  if (firstTime) {
    
    GLint curAlignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT,&curAlignment);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    fontOffset = glGenLists (128);
    for (GLuint i = 32; i < 127; i++) {
      glNewList(i+fontOffset, GL_COMPILE);
      // rasters defined in viz/MoGLFonts.h  - a hardwired font array
      //glBitmap(8, 13, 0.0, 2.0, 10.0, 0.0, rasters[i-32]);
      //glBitmap(9, 15, 0.0, 0.0, 8.0, 0.0, Fixed9x15_Character_Map[i]+1);
      glBitmap(8, 13, 0.0, 0.0, 7.0, 0.0, Fixed8x13_Character_Map[i]+1);
      glEndList();
    }
    firstTime = 0;
    glPixelStorei(GL_UNPACK_ALIGNMENT, curAlignment);
  }


  // If this is not done, the color/lighting of objects farther down the
  // scenegraph will not have correct lighting.  calling 
  // SoGLLazyElement::sendAllMaterial(state) works also but might be overkill
  // KLS Sep 07 - deduced this from measures code and now make sure that this
  // method doesn't just return before executing this
  SoState *state = action->getState();
  SoGLLazyElement* lazyElt =
    (SoGLLazyElement*)SoLazyElement::getInstance(state);
  lazyElt->reset(state,
                 (SoLazyElement::DIFFUSE_MASK)|(SoLazyElement::LIGHT_MODEL_MASK));

  if (viewer->p_showText) {

     glPushMatrix();


     glDisable(GL_LIGHTING);  // so we don't have to set normals

     // Put text colors under user control
     float r,g,b;
     EcceDispParam options;
     options.getForeground(r,g,b);
     glColor3f(r,g,b);


     const SbViewVolume &vv = SoViewVolumeElement::get(action->getState());


     for (int idx=0; idx<4; idx++) {
        const char *text = viewer->p_cornerText[idx].c_str();
        if (strlen(text) == 0) continue;
        // The bottom-left is 0,0, the upper right is 1,1
        SbVec2f pos(0.0,0.);
        if (idx==0) pos.setValue(0.010,0.010); //lleft
        else if (idx==1) pos.setValue(0.70,0.010); // lright
        else if (idx==2) pos.setValue(0.010,0.96); // uleft
        else if (idx==3) pos.setValue(0.70,0.96);  //uright


        float x =0;float y =0;float z =0;
        vv.getPlanePoint(vv.getNearDist()+ 0.1 * vv.getDepth(), pos).getValue(x,y,z);

        glRasterPos3f(x,y,z);
        glPushAttrib (GL_LIST_BIT);
        glListBase(fontOffset);  // fontOffset is static global
        glCallLists(strlen(text), GL_UNSIGNED_BYTE, (GLubyte *)text);
        glPopAttrib ();
     }

     glEnable(GL_LIGHTING);


     glPopMatrix();
  }
  
}


/**
 * Process a common set of events which are shared accross all
 * viewers. Returning TRUE if the event was processed.
 */
SbBool SGViewer::processCommonEvents(wxEvent * event)
{
  //  cerr << "Enter SGViewer::processCommonEvents\n";
  //  KeySym keysym;

  // check if the application wants to handle the event itself
  // instead of giving it to the viewer. This can be used to disable
  // some simple viewer functionality (like the Arrow keys or Esc key).
  // ??? this is a simple work around for bug #113991 - Xt translation
  // ??? tables would be better than dealing with events directly.
  //  if (p_renderArea->invokeAppCB(event))
  //    return TRUE;

  // check for special key which turns viewing on/off


  /*
    @todo Skip this mode changing key events for now

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


  wxMouseEvent * mouseEvent = dynamic_cast<wxMouseEvent *>(event);
  if (mouseEvent) {
    if (mouseEvent->ButtonDown() || mouseEvent->ButtonUp() ||
        mouseEvent->Dragging() || mouseEvent->GetWheelRotation()) {
      touchOrientation();
    }

    if (p_mouseListeners.size() > 0) {
       for (int idx=0; idx<p_mouseListeners.size(); idx++) {
          MouseEventListener *l = p_mouseListeners[idx];
          l->mouseEvent(mouseEvent);
       }
    }

    int x = mouseEvent->GetX();
    int y = mouseEvent->GetY();
    SbVec2s raSize = getGlxSize();
      
    if ((mouseEvent->ButtonDown() || mouseEvent->ButtonDClick())
        && mouseEvent->Button(wxMOUSE_BTN_RIGHT)) {
      p_locator[0] = x;
      p_locator[1] = raSize[1] - y;
      saveCursor();
      setViewing(true);
      if (mouseEvent->ControlDown())
        switchMode(DOLLY_MODE_ACTIVE);
      else
        switchMode(SPIN_MODE_ACTIVE);
      return true;
    }
    if (mouseEvent->ButtonUp() && mouseEvent->Button(wxMOUSE_BTN_RIGHT)) {
      p_locator[0] = x;
      p_locator[1] = raSize[1] - y;
      switchMode(p_stickyMode);
      setViewing(p_stickyMode != PICK_MODE);
      restoreCursor();
      return true;
    }
    if ((mouseEvent->ButtonDown() || mouseEvent->ButtonDClick())
        && mouseEvent->Button(wxMOUSE_BTN_MIDDLE)) {
      p_locator[0] = x;
      p_locator[1] = raSize[1] - y;
      saveCursor();
      setViewing(true);
      switchMode(PAN_MODE_ACTIVE);
      return true;
    }
    if (mouseEvent->ButtonUp() && mouseEvent->Button(wxMOUSE_BTN_MIDDLE)) {
      p_locator[0] = x;
      p_locator[1] = raSize[1] - y;
      switchMode(p_stickyMode);
      setViewing(p_stickyMode != PICK_MODE);
      restoreCursor();
      return true;
    }
    if (mouseEvent->GetWheelRotation()) {
      zoomCamera(mouseEvent->GetWheelRotation()*1.0/mouseEvent->GetWheelDelta());
      return true;
    }
  }

  // send the event to the scene graph if viewing is off
  if ( !isViewing() ) {
    p_renderArea->processRenderAreaEvent(event);
    if (!p_ecceSel->isFreePick()) {
      return true;
    }
  }

  // if no camera discard events
  if (p_camera == NULL)
    return true;

  //  cerr << "Returning false\n";
  return false;
}


void SGViewer::setStickyMode(int mode)
{
  if (mode == VIEW_MODE || mode == PAN_MODE || mode == DOLLY_MODE) {
    p_stickyMode = mode;
    setViewing(true);
    switchMode(mode);
  }
  else if (mode == PICK_MODE) {
    p_stickyMode = mode;
    setViewing(false);
  }
}


/**
 * sets the viewer mode based on what keys and buttons are being pressed
 */
void SGViewer::updateViewerMode(wxMouseEvent * event)
{
  // LEFT+MIDDLE down
  if ((event->LeftDown() && event->MiddleIsDown()) ||
      (event->LeftIsDown() && event->MiddleDown())) {
    switchMode(DOLLY_MODE_ACTIVE);
  }
  
  // LEFT down
  else if (event->LeftDown()) {
    if (event->ControlDown() || p_stickyMode == PAN_MODE)
      switchMode(PAN_MODE_ACTIVE);
    else if (p_stickyMode == DOLLY_MODE)
      switchMode(DOLLY_MODE_ACTIVE);
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
    if (event->ControlDown() || p_stickyMode == PAN_MODE)
      switchMode(PAN_MODE);
    else if (p_stickyMode == DOLLY_MODE)
      switchMode(DOLLY_MODE);
    else
      switchMode(VIEW_MODE);
  }
}


/**
 * Process the given event to do viewing stuff
 */
void SGViewer::processEvent( wxEvent * event )
{
#ifdef SOWXDEBUG
  cerr << "Enter SGViewer::processEvent\n";
#endif

  if ( processCommonEvents(event) ) {
    return;
  }

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
    else if (mouseEvent->GetWheelRotation()) {
      zoomCamera(mouseEvent->GetWheelRotation()*1.0/mouseEvent->GetWheelDelta());
    }
  }
  else {
    wxKeyEvent * keyEvent = dynamic_cast<wxKeyEvent *>(event);
    if (keyEvent != 0) {
      if (keyEvent->GetKeyCode() == WXK_CONTROL && !keyEvent->ControlDown()) {
        switchMode(PAN_MODE);
      }
      else if (keyEvent->GetKeyCode() == WXK_CONTROL && keyEvent->ControlDown()) {
        switchMode(p_stickyMode);
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
 * 
 */
void SGViewer::setCursor(string cursorName)
{
  ewxCursor cursor(cursorName);
  p_renderArea->SetCursor(cursor);
}


/**
 * 
 */
void SGViewer::saveCursor()
{
  p_tmpCursor.push(p_renderArea->GetCursor());
}


/**
 * 
 */
void SGViewer::restoreCursor()
{
  if (p_tmpCursor.size()) {
    p_renderArea->SetCursor(p_tmpCursor.top());
    p_tmpCursor.pop();
  }
}


/**
 *
 void SGViewer::processEvent(wxEvent * event)
 {
 SoWxExaminerViewer::processEvent(event);
  
 // Detect mouse release event and call this func.
 //   getSel()->stopProcessing2DMotion();
 }
*/


void SGViewer::zoomCamera(float delta)
{
  float zoomSpeed = 8.0;
  delta /= zoomSpeed;
  if (p_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
    // change the ortho camera height
    SoOrthographicCamera *cam = (SoOrthographicCamera *) p_camera;
    cam->height = cam->height.getValue() * powf(2.0, delta);
  }
  else {
    // shorter/grow the focal distance given the mouse move
    float focalDistance = p_camera->focalDistance.getValue();;
    float newFocalDist = focalDistance * powf(2.0, delta);
    
    // finally reposition the camera
    SbMatrix mx;
    mx = p_camera->orientation.getValue();
    SbVec3f forward(-mx[2][0], -mx[2][1], -mx[2][2]);
    p_camera->position = p_camera->position.getValue() +
      (focalDistance - newFocalDist) * forward;
    p_camera->focalDistance = newFocalDist;
  }
}


void SGViewer::showTextLabels(bool show)
{
   p_showText = show;
   p_myCallback->touch();
}


void SGViewer::resetToHomePosition()
{
  SoWxExaminerViewer::resetToHomePosition();
  touchOrientation();
}

void SGViewer::setOrientation(float twist, float elevation, float azimuth)
{
  SbVec3f axis;
  float ax, ay, az;
  ax = cos(azimuth)*sin(elevation);
  ay = sin(azimuth)*sin(elevation);
  az = cos(elevation);
  axis.setValue(ax,ay,az);
  p_camera->orientation.setValue(axis,twist);
}

void SGViewer::getOrientation(float &twist, float &elevation, float &azimuth) const
{
  twist = p_twist;
  elevation = p_elevation;
  azimuth = p_azimuth;
}

void SGViewer::touchOrientation()
{
  SbVec3f axis;
  p_camera->orientation.getValue(axis,p_twist);
  float ax, ay, az;
  axis.getValue(ax,ay,az);
  float anorm = sqrt(ax*ax + ay*ay + az*az);
  ax /= anorm;
  ay /= anorm;
  az /= anorm;
  p_elevation = acos(az);
  float sinth = sin(p_elevation);
  if (sinth != 0.0) {
    p_azimuth = acos(ax/sinth);
    if (ay < 0.0) {
      p_azimuth = 8.0*atan(1.0) - p_azimuth;
    }
  } else {
    p_azimuth = 0.0;
  }
}

void SGViewer::addMouseEventListener(MouseEventListener *l)
{
   p_mouseListeners.push_back(l);
}

void SGViewer::rmMouseEventListener(MouseEventListener *l)
{
   vector<MouseEventListener *>::iterator it = p_mouseListeners.begin();
   while (it != p_mouseListeners.end()) {
      if (*it == l) {
         p_mouseListeners.erase(it);
         break;
      }
      it++;
   }
}

void SGViewer::setSelectModeDrag(const bool& dragFlag)
{
   p_dragFlag = dragFlag;
}

bool SGViewer::getSelectModeDrag()
{
   return p_dragFlag;
}

