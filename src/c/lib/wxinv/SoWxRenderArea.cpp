#include "inv/actions/SoHandleEventAction.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/elements/SoWindowElement.H"

#include "inv/SoWx/SoWx.H"
#include "inv/SoWx/SoWxRenderArea.H"
#include "inv/SoWx/SoWxViewer.H"
#include "inv/SoWx/SoWxDevice.H"
#include "inv/SoWx/SoWxMouse.H"
#include "inv/SoWx/SoWxKeyboard.H"


#include <iostream>
using std::cerr;


BEGIN_EVENT_TABLE(SoWxRenderArea, wxGLCanvas)
  EVT_SIZE(SoWxRenderArea::OnSize)
  EVT_PAINT(SoWxRenderArea::OnPaint)
  EVT_ERASE_BACKGROUND(SoWxRenderArea::OnEraseBackground)
  EVT_MOUSE_EVENTS(SoWxRenderArea::OnMouse)
  EVT_KEY_DOWN(SoWxRenderArea::OnKeyboard)
  EVT_KEY_UP(SoWxRenderArea::OnKeyboard)
  EVT_SET_FOCUS(SoWxRenderArea::OnFocus)
  EVT_KILL_FOCUS(SoWxRenderArea::OnFocus)
END_EVENT_TABLE()
  ;

SoWxRenderArea::SoWxRenderArea(wxWindow * parent,
                               wxWindowID id,
                               const wxPoint& pos,
                               const wxSize& size,
                               long style,
                               const char * name,
                               int * attribList,
                               const wxPalette& palette,
                               SbBool buildInSideParent,
                               SbBool getMouseInput,
                               SbBool getKeyboardInput,
                               SbBool buildNow)
  : wxGLCanvas(parent, id, pos, size, style, name, attribList, palette)
{
  // GLWidget stuff
  p_glModes = SO_GLX_RGB | SO_GLX_DOUBLE | SO_GLX_ZBUFFER | SO_GLX_OVERLAY;
  p_waitForExpose = false;
  p_drawToFrontBuffer = false;
  p_enableDrawToFrontBuffer = false;
  p_glxSize.setValue(0, 0);

  p_windowResized = false;
  p_inPaint = false;

  constructorCommon(getMouseInput, getKeyboardInput, buildNow);

  initGraphic();
  initOverlayGraphic();

  p_windowResized = true;
}


SoWxRenderArea::~SoWxRenderArea()
{
  // Remove our callback from old selection node
  if (p_selection != NULL) {
    p_selection->removeChangeCallback(SoWxRenderArea::selectionChangeCB, this);
    
    // Unref the old selection node
    p_selection->unref(); 
  }
  
  // @todo What is this?
  // unregisterWidget(getGlxMgrWidget());
  // if (mapColors != NULL)
  //   free(mapColors);
  // if (overlayMapColors != NULL)
  //   free(overlayMapColors);
  
  if (p_mouseDevice != NULL)
    delete p_mouseDevice;
  if (p_keyboardDevice != NULL)
    delete p_keyboardDevice;
  delete p_sceneMgr;
  delete p_overlaySceneMgr;
  delete p_deviceList;
}


/**
 * Make the new user supplied scene graph the rendering root.
 */
void SoWxRenderArea::setSceneGraph(SoNode *newScene)
{
  // Deactivate while we change the scene so that our sensors
  // get unhooked before the data changes beneath us.
  p_sceneMgr->deactivate();

  p_sceneMgr->setSceneGraph(newScene);

  // draw new scene graphs to the front buffer by default since
  // the scene will be different (we might has well see something
  // happening for the first redraw).
  if (isDrawToFrontBufferEnable())
    p_drawToFrontBuffer = true;

  // we activate only if we are visible.
  // after all, if we're not on screen, the visibility change
  // callback will invoke activate() once we are on screen.
  if (isVisible() && p_autoRedraw) {
    p_sceneMgr->activate();
    p_sceneMgr->scheduleRedraw();
  }
}


/**
 * Return the rendering root.
 */
SoNode * SoWxRenderArea::getSceneGraph()
{
  return p_sceneMgr->getSceneGraph();
}


/**
 * Set the scene graph to render in the overlay bit planes.
 */
void SoWxRenderArea::setOverlaySceneGraph(SoNode *newScene)
{
  // @todo Worry about overlay later.
  //  if (! getOverlayWidget())
  //    return;
  
  // Deactivate while we change the scene so that our sensors
  // get unhooked before the data changes beneath us.
  p_overlaySceneMgr->deactivate();
  
  p_overlaySceneMgr->setSceneGraph(newScene);
  
  // we activate only if we are visible.
  // after all, if we're not on screen, the visibility change
  // callback will invoke activate() once we are on screen.
  if (isVisible() && p_autoRedraw) {
    p_overlaySceneMgr->activate();
    p_overlaySceneMgr->scheduleRedraw();
  }
}


/**
 * Get the scene graph to render in the overlay bit planes.
 */
SoNode * SoWxRenderArea::getOverlaySceneGraph()
{
  return p_overlaySceneMgr->getSceneGraph();
}


/**
 * Register interest to handle events for the passed device.
 * If device is already in the list, then make sure it's enabled.
 */
void SoWxRenderArea::registerDevice(SoWxDevice *device)
{
  // Add the device if it's not already in the list of devices
  int index = p_deviceList->find(device);
  // ??? mott, shouldn't we return if the device is found ?
  if (index == -1)
    p_deviceList->append(device);
  
  // tell the device the window size
  device->setWindowSize(getGlxSize());
  
  // Tell the device to register event interest for our widget
  // Widget w = getOverlayWidget() ? getOverlayWidget() : getNormalWidget();
  // if ((w != NULL) && (XtWindow(w) != (Window) NULL))
  device->enable(this);
}


/**
 * Unregister interest in the passed device.
 */
void SoWxRenderArea::unregisterDevice(SoWxDevice *device)
{
  // Find the device in our device list
  int index = p_deviceList->find(device);
  if (index == -1)
    return;
  
  // Remove the device from the list of devices
  p_deviceList->remove(index);
  
  // Tell the device to register event interest for our widget
  // Widget w = getOverlayWidget() ? getOverlayWidget() : getNormalWidget();
  // if (w)
  device->disable(this);
}


/**
 * Set the window background color when in RGB mode.
 * (default to black (0,0,0));
 */
void SoWxRenderArea::setBackgroundColor(const SbColor &c)
{
  p_sceneMgr->setBackgroundColor(c);
}


/**
 * Get the window background color when in RGB mode.
 */
const SbColor & SoWxRenderArea::getBackgroundColor() const
{
  return p_sceneMgr->getBackgroundColor();
}


/**
 * Set the window background color when in color index mode.
 * (default to black (index 0)).
 */
void SoWxRenderArea::setBackgroundIndex(int index)
{
  p_sceneMgr->setBackgroundIndex(index);
}


/**
 * Get the window background color when in color index mode.
 */
int SoWxRenderArea::getBackgroundIndex() const
{
  return p_sceneMgr->getBackgroundIndex();
}


/**
 * Set the overlay window background color index.
 * (default to 0 (clear color)).
 */
void SoWxRenderArea::setOverlayBackgroundIndex(int index)
{
  p_overlaySceneMgr->setBackgroundIndex(index);
}


/**
 * Get the overlay window background color index.
 */
int SoWxRenderArea::getOverlayBackgroundIndex() const
{
  return p_overlaySceneMgr->getBackgroundIndex();
}


/**
 * @todo Worry about color map later.
 * Loads the color map with the given colors.
 void SoWxRenderArea::setColorMap(int startIndex, int num,
 const SbColor *colors)
 {
 if (mapColors != NULL)
 free(mapColors);
 mapColors = (XColor *) malloc(sizeof(XColor) * num);
 mapColorNum = num;
 XColor *xcol = mapColors;
 for (int i=0; i<num; i++, xcol++) {
 xcol->red   = (unsigned short) (colors[i][0] * 65535);
 xcol->green = (unsigned short) (colors[i][1] * 65535);
 xcol->blue  = (unsigned short) (colors[i][2] * 65535);
 xcol->flags = DoRed|DoGreen|DoBlue;
 xcol->pixel = startIndex + i;
 }
  
 // now load those colors into the color map
 if (colorMap != 0)
 XStoreColors(getDisplay(), colorMap, mapColors, mapColorNum);
 }
*/


/**
 * Set current viewport region to use for rendering.
 */
void SoWxRenderArea::setViewportRegion(const SbViewportRegion &newRegion) 
{
  p_sceneMgr->getGLRenderAction()->setViewportRegion(newRegion);
}


/**
 * Get current viewport region to use for rendering.
 */
const SbViewportRegion & SoWxRenderArea::getViewportRegion() const
{
  return p_sceneMgr->getGLRenderAction()->getViewportRegion();
}


/**
 * Set the transparency type.
 */
void SoWxRenderArea::setTransparencyType(SoGLRenderAction::TransparencyType type)
{
  p_sceneMgr->getGLRenderAction()->setTransparencyType(type);
  p_sceneMgr->scheduleRedraw();
}



/**
 * Get the transparency type.
 */
SoGLRenderAction::TransparencyType SoWxRenderArea::getTransparencyType() const
{
  return p_sceneMgr->getGLRenderAction()->getTransparencyType();
}


/**
 * Set antialiasing on GL rendering action.
 */
void SoWxRenderArea::setAntialiasing(SbBool smoothing, int numPasses)
{
  SoGLRenderAction *ra = getGLRenderAction();

  if (smoothing == ra->isSmoothing() && numPasses == ra->getNumPasses())
    return;
    
  // The visual needs to change if we are now enabling or disabling
  // the accumulation buffer
  SbBool needToChangeVisual = ((numPasses >  1 && ra->getNumPasses() == 1) ||
                               (numPasses == 1 && ra->getNumPasses() >  1));
  
  // now create a visual with the right info - this tries to
  // preserve as many setting as the original visual, with just
  // the antialiasing changed.
  if (needToChangeVisual) {
    /* @todo Work on visual stuff later.
    
    int n = 0;
    int attribList[20];
    
    if (isRGBMode()) {
    attribList[n++] = GLX_RGBA;
    attribList[n++] = GLX_RED_SIZE;
    attribList[n++] = 1;
    attribList[n++] = GLX_GREEN_SIZE;
    attribList[n++] = 1;
    attribList[n++] = GLX_BLUE_SIZE;
    attribList[n++] = 1;
    }
    // there is always zbuffer...
    attribList[n++] = GLX_DEPTH_SIZE;
    attribList[n++] = 1;
    if (isDoubleBuffer())
    attribList[n++] = GLX_DOUBLEBUFFER;
    
    // now add the antialiasing stuff
    if (numPasses > 1) {
    attribList[n++] = GLX_ACCUM_RED_SIZE;
    attribList[n++] = 1;
    attribList[n++] = GLX_ACCUM_GREEN_SIZE;
    attribList[n++] = 1;
    attribList[n++] = GLX_ACCUM_BLUE_SIZE;
    attribList[n++] = 1;
    }
    
    // check for stencil buffer
    Widget w = getWidget();
    XVisualInfo *normalVis = getNormalVisual();
    if (normalVis) {
    int val;
    glXGetConfig(XtDisplay(w), normalVis, GLX_STENCIL_SIZE, &val);
    if (val) {
    attribList[n++] = GLX_STENCIL_SIZE;
    attribList[n++] = val;
    }
    }
    
    attribList[n++] = (int) None;
    
    // create the visual
    XVisualInfo *vis = glXChooseVisual(XtDisplay(w), 
    XScreenNumberOfScreen(XtScreen(w)), attribList);
    
    if (! vis) {
    #ifdef DEBUG
    SoDebugError::post("SoWxRenderArea::setAntialiasing",
    "could not create ACCUM visual");
    #endif
    return;
    }
    
    // now set this as the current visual
    setNormalVisual(vis);
    
    // GL widget made a copy - we can free this
    XFree(vis);
    */
  }
  else // only the number of passes changed, so just redraw
    p_sceneMgr->scheduleRedraw();
  
  // finally set the stuff on the render action
  p_sceneMgr->setAntialiasing(smoothing, numPasses);
}


/**
 * Get antialiasing on GL rendering action.
 */
void SoWxRenderArea::getAntialiasing(SbBool &smoothing, int &numPasses) const
{
  p_sceneMgr->getAntialiasing(smoothing, numPasses);
}


/**
 * Set clearFirst flag.
 * Enable/prevent window clearing from happening before a rendering
 * starts (default is clearBeforeRender TRUE).
 */
void SoWxRenderArea::setClearBeforeRender(SbBool trueOrFalse)
{
  p_clearFirst = trueOrFalse;
}


/**
 * Get clearFirst flag.
 */
SbBool SoWxRenderArea::isClearBeforeRender() const
{
  return p_clearFirst;
}


/**
 * Set clearOverlayFirst flag.
 */
void SoWxRenderArea::setClearBeforeOverlayRender(SbBool trueOrFalse)
{
  p_clearOverlayFirst = trueOrFalse;
}


/**
 * Get clearOverlayFirst flag.
 */
SbBool SoWxRenderArea::isClearBeforeOverlayRender() const
{
  return p_clearOverlayFirst;
}


/**
 * Turn auto-redraw on or off. If turning on, then activate the scene
 * sensor.
 */
void SoWxRenderArea::setAutoRedraw(SbBool flag)
{
  if (flag == p_autoRedraw)
    return;
  
  p_autoRedraw = flag;
  
  if (p_autoRedraw) {
    if (isVisible())
      activate();
  }
  else
    deactivate();
}


/**
 * Get autoRedraw flag.
 */
SbBool SoWxRenderArea::isAutoRedraw() const
{
  return p_autoRedraw;
}


/**
 * Set the priority of the redraw sensor.
 */
void SoWxRenderArea::setRedrawPriority(uint32_t priority)
{
  p_sceneMgr->setRedrawPriority(priority);
  p_overlaySceneMgr->setRedrawPriority(priority);
}


/**
 * Get the priority of the redraw sensor.
 */
uint32_t SoWxRenderArea::getRedrawPriority() const
{
  return p_sceneMgr->getRedrawPriority();
}


/**
 * Get the default priority number.
 */
uint32_t SoWxRenderArea::getDefaultRedrawPriority()
{
  return SoSceneManager::getDefaultRedrawPriority();
}


/**
 * Calling this forces the render area to be redrawn now.
 * It is not necessary to call this method if auto redraw is enabled
 * (which is the default).
 */
void SoWxRenderArea::render()
{
  // @todo Replace it with Refresh()?
  redraw();
}


/**
 * Calling this forces the render area to be redrawn now.
 * It is not necessary to call this method if auto redraw is enabled
 * (which is the default).
 */
void SoWxRenderArea::renderOverlay()
{
  // @todo Replace it with Refresh()?
  redrawOverlay();
}


/**
 * Schedule a redraw to happen sometime soon.
 */
void SoWxRenderArea::scheduleRedraw()
{
  if (isAutoRedraw())
    p_sceneMgr->scheduleRedraw();
  else
    //    redraw();
    Refresh(false);
    Update();
}


/**
 * Schedule a redraw to happen sometime soon.
 */
void SoWxRenderArea::scheduleOverlayRedraw()
{
  if (isAutoRedraw())
    p_overlaySceneMgr->scheduleRedraw();
  else
    redrawOverlay();
}


/**
 * Convenience to set up the redraw mechansim whenever the selection changes.
 */
void SoWxRenderArea::redrawOnSelectionChange(SoSelection *s)
{
  // Ref the new selection node
  if (s != NULL)
    s->ref();
  
  // Remove our callback from old selection node
  if (p_selection != NULL) {
    p_selection->removeChangeCallback(SoWxRenderArea::selectionChangeCB, this);
    
    // Unref the old selection node
    p_selection->unref(); 
  }
  
  p_selection = s;
  
  // Add our callback to this selection node. (We've already ref'd this new sel node)
  if (p_selection != NULL)
    p_selection->addChangeCallback(SoWxRenderArea::selectionChangeCB, this);
}


/**
 * Convenience to set up the redraw mechansim whenever the selection changes.
 */
void SoWxRenderArea::redrawOverlayOnSelectionChange(SoSelection *s)
{
  // Remove our callback from old selection node
  if (p_overlaySelection != NULL)
    p_overlaySelection->removeChangeCallback(SoWxRenderArea::overlaySelectionChangeCB, this);

  p_overlaySelection = s;
  
  // Add our callback to this selection node
  if (p_overlaySelection != NULL)
    p_overlaySelection->addChangeCallback(SoWxRenderArea::overlaySelectionChangeCB, this);
}


/**
 * Call this convenience method to have this render area redraw
 * whenever the selection list changes in the passed node. This
 * is useful if using a highlight render action like the
 * SoBoxHighlightRenderAction to correctly render whenever the
 * selection changes. Pass NULL to turn this off.
 */
void SoWxRenderArea::setEventCallback(SoWxRenderAreaEventCB *fcn, void * userData)
{
  p_appEventHandler = fcn;
  p_appEventHandlerData = userData;
}


/**
 * Set the scene manager.
 */
void SoWxRenderArea::setSceneManager(SoSceneManager *sm)
{
  p_sceneMgr = sm;
}


/**
 * Get the scene manager.
 */
SoSceneManager * SoWxRenderArea::getSceneManager() const
{
  return p_sceneMgr;
}


/**
 * Set the overlay scene manager.
 */
void SoWxRenderArea::setOverlaySceneManager(SoSceneManager *sm)
{
  p_overlaySceneMgr = sm;
}


/**
 * Get the overlay scene manager.
 */
SoSceneManager * SoWxRenderArea::getOverlaySceneManager() const
{
  return p_overlaySceneMgr;
}


/**
 * Set the GL render action.
 */
void SoWxRenderArea::setGLRenderAction(SoGLRenderAction *ra)
{
  p_sceneMgr->setGLRenderAction(ra);
}


/**
 * Get the GL render action.
 */
SoGLRenderAction * SoWxRenderArea::getGLRenderAction() const
{
  return p_sceneMgr->getGLRenderAction();
}


/**
 * Set the overlay GL render action.
 */
void SoWxRenderArea::setOverlayGLRenderAction(SoGLRenderAction *ra)
{
  p_overlaySceneMgr->setGLRenderAction(ra);
}


/**
 * Get the overlay GL render action.
 */
SoGLRenderAction * SoWxRenderArea::getOverlayGLRenderAction() const
{
  return p_overlaySceneMgr->getGLRenderAction();
}


/**
 * This routine draws the scene graph (called by expose events and the
 * scene graph sensor)
 * @todo Many changeds in this function. Wonder if content should be moved into OnPaint
 */
void SoWxRenderArea::redraw()
{
  if (!isVisible() || p_waitForExpose)
    return;

  wxPaintDC dc(this);

#ifndef __WXMOTIF__
  if (!GetContext()) return;
#endif

  SetCurrent();

  // @todo Not here in Xt version. Wonder if should be done in GLWidget.
  glEnable(GL_DEPTH_TEST);

  if (p_drawToFrontBuffer && isDrawToFrontBufferEnable() && isDoubleBuffer()) {
    if (isRGBMode()) {
      SbColor color = getBackgroundColor();
      glClearColor(color[0], color[1], color[2], 0);
    } else
      glClearIndex(getBackgroundIndex());
    glClear(GL_COLOR_BUFFER_BIT);
    SwapBuffers();
    
    glReadBuffer(GL_FRONT); // Needed for acbuf antialiasing
    glDrawBuffer(GL_FRONT);
  }
  
  if (p_windowResized) {
    sizeChanged(p_glxSize);
    p_windowResized = false;
  }
  
  actualRedraw();

  // swap those buffers!
  if (isDoubleBuffer()) {
    if (p_drawToFrontBuffer && isDrawToFrontBufferEnable()) {
      // no need to swap here - instead restore the buffer and 
      // clear the flag now that we have drawn to the front buffer
      glReadBuffer(GL_BACK); // Needed for acbuf antialiasing
      glDrawBuffer(GL_BACK);
      glFlush();
    }
    else {
      glFlush();
      SwapBuffers();
    }
  }
  else
    glFlush();
  
  // clear this flag now that we have drawn
  p_drawToFrontBuffer = false;

  // load the color map
  //  if (! isRGBMode() && colorMap != 0 && mapColorNum != 0)
  //    XStoreColors(getDisplay(), colorMap, mapColors, mapColorNum);

  //  glEnable(GL_DEPTH_TEST);

  /*
    glDrawBuffer(GL_FRONT);
    glDisable(GL_CULL_FACE);

    glEnable(GL_DITHER);
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);

    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

  */

}


/**
 * Have the scene manager redraw the scene.
 */
void SoWxRenderArea::actualRedraw()
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxRenderArea::actualRedraw()\n";
#endif

  SoWxViewer * viewer = dynamic_cast<SoWxViewer*>(GetParent());
  if (viewer) {
    viewer->actualRedraw();
  }
  else {
    p_sceneMgr->render(p_clearFirst);
  }

#ifdef SOWXDEBUG
  cerr << "Leave SoWxRenderArea::actualRedraw()\n";
#endif
}


/**
 * This routine draws the scene graph for the overlay bit planes (called
 * by the expose event and scene graph sensor).
 */
void SoWxRenderArea::redrawOverlay()
{
  // @todo Need fix.
  /*
    if (!isVisible() || getOverlayWindow() == (Window) NULL)
    return;
    
    // set the window
    glXMakeCurrent(getDisplay(), getOverlayWindow(), getOverlayContext());
    
    // draw that scene! (subclasses may redefine...)
    actualOverlayRedraw();
    
    glFlush();
  */
}


/**
 * Have the scene manager redraw the overlay scene.
 */
void SoWxRenderArea::actualOverlayRedraw()
{
  p_overlaySceneMgr->render(p_clearOverlayFirst);
}


/**
 * Process the passed wx event.
 * Pass to viewer first. Used to go through virtual function.
 */
void SoWxRenderArea::processEvent( wxEvent * event )
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxRenderArea::processEvent\n";
#endif

  // check if app wants the event...
  if (invokeAppCB(event))
    return;

  // Let the viewer handler it if exist. Otherwise pass to scene
  SoWxViewer * viewer = dynamic_cast<SoWxViewer*>(GetParent());
  if (viewer)
    viewer->processEvent(event);
  else
    processRenderAreaEvent( event );

#ifdef SOWXDEBUG
  cerr << "Leave SoWxRenderArea::processEvent\n";
#endif
}


/**
 * Process the passed wx event.
 */
void SoWxRenderArea::processRenderAreaEvent( wxEvent * event )
{
  // send it to the scene

  // to do this, translate the wx event to an SoEvent
  // by finding a device to perform the translation 

  const SoEvent *soevent = NULL;
  for (int i = 0; (soevent == NULL) && (i < p_deviceList->getLength()); i++) {
    SoWxDevice *device = (SoWxDevice *) (*p_deviceList)[i];
    soevent = device->translateEvent(event);
  }

  // no device found, so return...
  if (! soevent)
    return;
  
  // now send the event first to the overlay scene graph, elses to
  // the regular scene graph.
  SbBool handled = p_overlaySceneMgr->processEvent(soevent);
  if (! handled) {
    p_sceneMgr->processEvent(soevent);
    
    // now check to make sure that we updated the handle event action
    // with the current window the very first time. This is needed
    // because the SoState does not exists until the action is
    // applied, and we only update those during enter/leave notify.
    if (p_firstEvent) {
      SoState *state = p_sceneMgr->getHandleEventAction()->getState();
      if (state) {
        // @todo Not there yet.
        // SoWindowElement::set(state, getNormalWindow(), 
        //  getNormalContext(), getDisplay(), getGLRenderAction());
        p_firstEvent = false;
      }
    }
  }
}


/**
 * Initialize the GLX window for rendering.
 * glXMakeCurrent() should have been called before invoking this routine.
 * @todo Need fix.
 */
void SoWxRenderArea::initGraphic()
{
  p_sceneMgr->reinitialize();
  p_sceneMgr->setRGBMode(isRGBMode());

  //  SoGLRenderAction *ra = p_sceneMgr->getGLRenderAction();
  //  ra->setCacheContext(getDisplayListShareGroup(getNormalContext()));
  //  ra->setRenderingIsRemote(!glXIsDirect(getDisplay(), getNormalContext()));

  /*
  SoGLRenderAction *ra = sceneMgr->getGLRenderAction();
  ra->setCacheContext(getDisplayListShareGroup(getNormalContext()));
  ra->setRenderingIsRemote(!glXIsDirect(getDisplay(), getNormalContext()));
  
  // load the color map
  if (! isRGBMode() && colorMap != 0 && mapColorNum != 0)
    XStoreColors(getDisplay(), colorMap, mapColors, mapColorNum);
  */
}


/**
 * Initialize the GLX window for rendering.
 * glXMakeCurrent() should have been called before invoking this routine.
 * @todo Need fix.
 */
void SoWxRenderArea::initOverlayGraphic()
{
  p_overlaySceneMgr->reinitialize();
  p_overlaySceneMgr->setRGBMode(false);

  /*
    SoGLRenderAction *ra = overlaySceneMgr->getGLRenderAction();
    ra->setCacheContext(getDisplayListShareGroup(getOverlayContext()));
    ra->setRenderingIsRemote(!glXIsDirect(getDisplay(), getOverlayContext()));

    // load the color map
    if (overlayColorMap != 0 && overlayMapColorNum != 0)
    XStoreColors(getDisplay(), overlayColorMap, overlayMapColors, overlayMapColorNum);

    // enable the devices on the overlay window
    reinstallDevices(getOverlayWidget());
  */
}


/**
 * Called by the base class (SoWxGLWidget) when the widget has changed size.
 */
void SoWxRenderArea::sizeChanged(const SbVec2s &newSize)
{
  p_sceneMgr->setWindowSize(newSize);
  p_overlaySceneMgr->setWindowSize(newSize);
  
  // tell each device the new window size
  for (int i = 0; i < p_deviceList->getLength(); i++) {
    SoWxDevice *device = (SoWxDevice *) (*p_deviceList)[i];
    device->setWindowSize(newSize);
  }
}


/**
 * Redefine those generic virtual functions
 * @todo Are these from SoWxComponent?
 */
const char * SoWxRenderArea::getDefaultWidgetName() const
{
  return GetName().c_str();
}


const char * SoWxRenderArea::getDefaultTitle() const
{
  return "Wx RenderArea";
}


const char * SoWxRenderArea::getDefaultIconTitle() const
{
  return "Wx RenderArea";
}


/**
 * Invoke the app callback and return what the callback returns.
 */
SbBool SoWxRenderArea::invokeAppCB( wxEvent * event )
{
  // if app wants event, send event to application
  if (p_appEventHandler != NULL)
    return (*p_appEventHandler)(p_appEventHandlerData, event);
  else
    return false;
}


/**
 * Selection change callback. Called by inventor
 */
void SoWxRenderArea::selectionChangeCB(void *p, SoSelection *)
{
  ((SoWxRenderArea *)p)->scheduleRedraw();
}


/**
 * Overlay selection change callback. Called by inventor
 */
void SoWxRenderArea::overlaySelectionChangeCB(void *p, SoSelection *)
{
  ((SoWxRenderArea *)p)->scheduleOverlayRedraw();
}


/**
 * Render callback. Called by inventor
 */
void SoWxRenderArea::renderCB(void *p, SoSceneManager *)
{
  // @todo Direct calling render won't work. Need to call refresh instead
#ifdef SOWXDEBUG
  cerr << "Enter SoWxRenderArea::renderCB()\n";
#endif

  SoWxRenderArea * renderArea = (SoWxRenderArea *)(p);
  if (renderArea && !renderArea->p_inPaint) {
    renderArea->Refresh(false);
    renderArea->Update();
  }

#ifdef SOWXDEBUG
  cerr << "Leave SoWxRenderArea::renderCB()\n";
#endif
}


/**
 * Overlay render callback. Called by inventor
 */
void SoWxRenderArea::renderOverlayCB(void *p, SoSceneManager *)
{
  ((SoWxRenderArea *)p)->redrawOverlay();
}


/**
 * Called whenever the component becomes visible or not
 */
void SoWxRenderArea::visibilityChangeCB(void *pt, SbBool visible)
{
  SoWxRenderArea *p = (SoWxRenderArea *)pt;
  
  if (visible)
    p->activate();
  else {
    // we are becoming hidden - so wait for an expose event
    // (which we now will come later) before enabling a redraw to
    // prevent too many redraws (scene graph changes + expose)
    p->p_waitForExpose = true;
    
    p->deactivate();
  }
}


/**
 * Attach the sensor (if necessary).
 */
void SoWxRenderArea::activate()
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxRenderArea::activate()\n";
#endif

  // if autoRedraw is off, then don't attach the scene sensor
  if (! p_autoRedraw)
    return;
  
  // Activate the scene manager
  p_sceneMgr->activate();
  p_overlaySceneMgr->activate();

#ifdef SOWXDEBUG
  cerr << "Leave SoWxRenderArea::activate()\n";
#endif
}


/**
 *
 */
void SoWxRenderArea::deactivate()
{
  p_sceneMgr->deactivate();
  p_overlaySceneMgr->deactivate();
}


/**
 *
 */
void SoWxRenderArea::constructorCommon(SbBool getMouseInput,
                                       SbBool getKeyboardInput,
                                       SbBool buildNow)
{
  // @todo May not need these two
  //  addVisibilityChangeCallback(visibilityChangeCB, this);
  //  setClassName(thisClassName);

  p_firstEvent = true;

  // set up the device list
  p_deviceList = new SbPList;

  // default devices
  if (getMouseInput) {
    p_mouseDevice = new SoWxMouse();
    p_deviceList->append(p_mouseDevice);
  }
  else
    p_mouseDevice = NULL;

  if (getKeyboardInput) {
    p_keyboardDevice = new SoWxKeyboard();
    p_deviceList->append(p_keyboardDevice);
  }
  else
    p_keyboardDevice = NULL;

  // local vars
  p_clearFirst = true;
  p_clearOverlayFirst = true;
  p_autoRedraw = true;
  p_selection = NULL;
  p_overlaySelection = NULL;
  // @todo What is it?
  //  p_deviceWidget = NULL;

  //  setSize(SbVec2s(400, 400));  // default size
  //  setBorder(TRUE);

  p_appEventHandler = NULL;

  p_sceneMgr = new SoSceneManager();
  p_sceneMgr->setRenderCallback(SoWxRenderArea::renderCB, this);

  p_overlaySceneMgr = new SoSceneManager();
  p_overlaySceneMgr->setRenderCallback(SoWxRenderArea::renderOverlayCB, this);

  // @todo color map variables
  //  mapColors = overlayMapColors = NULL;
  //  mapColorNum = overlayMapColorNum = 0;

  //   if (buildNow)
  //     setBaseWidget(buildWidget(getParentWidget()));
}


/**
 *
 */
void SoWxRenderArea::OnMouse( wxMouseEvent& event )
{
  processEvent(&event);
}


/**
 *
 */
void SoWxRenderArea::OnKeyboard( wxKeyEvent& event )
{
  processEvent(&event);
}


/**
 *
 */
void SoWxRenderArea::OnFocus( wxFocusEvent& event )
{
  processEvent(&event);
}


/**
 *
 */
void SoWxRenderArea::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxRenderArea::OnPaint\n";
#endif

  p_inPaint = true;

  //  activate();
  redraw();
  // @todo Should also capture EVT_ICONIZE( FRAME::OnIconize )
  // and do deactive() waitForExpose=true etc.
  p_inPaint = false;

#ifdef SOWXDEBUG
  cerr << "Leave SoWxRenderArea::OnPaint\n";
#endif
}


/**
 *
 */
void SoWxRenderArea::OnSize(wxSizeEvent& event)
{
#ifdef SOWXDEBUG
  cerr << "Enter SoWxRenderArea::OnSize\n";
#endif

  // this is also necessary to update the context on some platforms
  wxGLCanvas::OnSize(event);

  int w, h;
  GetClientSize(&w, &h);
  p_glxSize.setValue(w, h);

  p_windowResized = true;
  
  //  cerr << "w: " << w << "    h: " << h << "\n";

  //  sizeChanged(p_glxSize);
  p_sceneMgr->scheduleRedraw();

#ifdef SOWXDEBUG
  cerr << "Leave SoWxRenderArea::OnSize\n";
#endif
}


/**
 *
 */
void SoWxRenderArea::OnEraseBackground(wxEraseEvent& WXUNUSED(event))
{
#ifdef SOWXDEBUG
  cerr << "Enter/Leave SoWxRenderArea::OnEraseBackground\n";
#endif

  /* Do nothing, to avoid flashing on MSW */
}




















//////////////////////////////////////////////////////
// GLWidget stuff

/**
 *
 */
void SoWxRenderArea::setDrawToFrontBufferEnable(SbBool flag)
{
  p_enableDrawToFrontBuffer = flag;

  // clear the flag is disabled
  if (! p_enableDrawToFrontBuffer)
    p_drawToFrontBuffer = false;
}


/**
 *
 */
SbBool SoWxRenderArea::isDrawToFrontBufferEnable() const
{
  return p_enableDrawToFrontBuffer;
}


/**
 *
 */
SbBool SoWxRenderArea::isDrawToFrontBuffer() const
{
  return p_drawToFrontBuffer;
}


/**
 *
 */
void SoWxRenderArea::setDoubleBuffer(SbBool flag)
{
  if (flag == isDoubleBuffer())
    return;
  
  /*
  // special case - if the user calls setDoubleBuffer(FALSE) BEFORE the
  // this component has been realized, then we need to prevent the
  // ginitCB routine for the old buffer to be called - so simply delete
  // the old widget alltogether - see bug 180978
  if (! getNormalContext())
  destroyNormalWindows();
  
  // set the gl mode and update the attribute list
  SET_BIT(glModes, SO_GLX_DOUBLE, flag);

  for (int i=0; attribList[i] != None; i++) {
  if (flag && attribList[i] == GLX_USE_GL) { // search for first no-op
  attribList[i] = GLX_DOUBLEBUFFER;
  break;
  }
  else if (! flag && attribList[i] == GLX_DOUBLEBUFFER) {
  attribList[i] = GLX_USE_GL; // clear with a no-op
  break;
  }
  }

  Widget *newWidget = (flag) ? &doubleBufferWidget : &singleBufferWidget;
  Widget *oldWidget = (flag) ? &singleBufferWidget : &doubleBufferWidget;
  
  // build and map the new window
  if (*newWidget == NULL)
  buildNormalGLXWidget();
  else {
  XtManageChild(*newWidget);
    
  // make sure overlay window is on top
  if (overlayWidget && XtWindow(overlayWidget))
  XRaiseWindow(XtDisplay(overlayWidget), XtWindow(overlayWidget));
    
  ctxNormal = flag ? ctxDouble : ctxSingle;
    
  // let subclasses know we changed widget
  widgetChanged(*newWidget);
  }
  
  // unmaps the old window now
  if (*oldWidget != NULL)
  XtUnmanageChild(*oldWidget);

  // prevent a redraw a redraw from happening until we receive an
  // expose event - this will prevent too many redraws from happening
  // if the scene graph also changes at the same time.
  waitForExpose = TRUE;
  */
}


/**
 *
 */
SbBool SoWxRenderArea::isDoubleBuffer() const
{
  return p_glModes & SO_GLX_DOUBLE;
}


/**
 *
 */
SbBool SoWxRenderArea::isRGBMode() const
{
  return p_glModes & SO_GLX_RGB;
}


/**
 *
 */
const SbVec2s & SoWxRenderArea::getGlxSize() const
{
  return p_glxSize;
}


/**
 *
 */
SbBool SoWxRenderArea::isVisible()
{
  wxTopLevelWindow * topWin = SoWx::getTopLevelWidget(this);
  return topWin != NULL && (!topWin->IsIconized()) && IsShown();
}
