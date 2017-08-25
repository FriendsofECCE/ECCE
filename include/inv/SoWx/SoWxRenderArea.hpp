#ifndef __SOWXRENDERAREA_H
#define __SOWXRENDERAREA_H

#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "inv/SoSceneManager.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/nodes/SoSelection.H"

#include "inv/SoWx/SoWxDevice.H"
#include "inv/SoWx/SoWxMouse.H"
#include "inv/SoWx/SoWxKeyboard.H"

// GLWidget defines
#define SO_GLX_RGB      (1<<0)
#define SO_GLX_DOUBLE   (1<<1)
#define SO_GLX_ZBUFFER  (1<<2)
#define SO_GLX_OVERLAY  (1<<3)
#define SO_GLX_STEREO   (1<<4)


// callback function prototype
typedef SbBool SoWxRenderAreaEventCB(void * userData, wxEvent * anyevent);


class SoWxRenderArea : public wxGLCanvas {
public:
  // Passing FALSE for getMouseInput means the mouse will be ignored.
  // Passing FALSE for getKeyboardInput means the keyboard will be ignored.
  SoWxRenderArea(wxWindow * parent,
                 wxWindowID id = -1,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = 0,
                 const char * name = "SoWxRenderArea",
                 int * attribList = 0,
                 const wxPalette& palette = wxNullPalette,
                 SbBool buildInSideParent = true,
                 SbBool getMouseInput = true,
                 SbBool getKeyboardInput = true,
                 SbBool buildNow = true);

  ~SoWxRenderArea();

  // Sets/gets the scene graph to render.
  virtual void setSceneGraph(SoNode *newScene);
  virtual SoNode * getSceneGraph();
  
  // Sets/gets the scene graph to render in the overlay bit planes.
  virtual void setOverlaySceneGraph(SoNode *newScene);
  virtual SoNode * getOverlaySceneGraph();

  // Handle/Unhandle devices. When registerDevice is called, the 
  // render area will register interest in events which that device
  // generates, and pass those events into the scene graph.
  // unregisterDevice means the render area will no longer listen
  // to events from the passed device.
  virtual void registerDevice(SoWxDevice *device);
  virtual void unregisterDevice(SoWxDevice *device);

  // Sets/gets the window background color when in RGB mode.
  // (default to black (0,0,0));
  virtual void setBackgroundColor(const SbColor &c);
  virtual const SbColor & getBackgroundColor() const;
  
  // Sets/gets the window background color when in color index mode.
  // (default to black (index 0)).
  virtual void setBackgroundIndex(int index);
  virtual int getBackgroundIndex() const;
  
  // Sets/gets the overlay window background color index.
  // (default to 0 (clear color)).
  virtual void setOverlayBackgroundIndex(int index);
  virtual int getOverlayBackgroundIndex() const;

  // Sets the colors to use when displaying in color index mode.
  // @todo Need fix.
  // virtual void setColorMap(int startIndex, int num, const SbColor *colors);

  // Sets/gets current viewport region to use for rendering
  virtual void setViewportRegion(const SbViewportRegion &newRegion);
  virtual const SbViewportRegion & getViewportRegion() const;

  // Transparency level setting methods which specifies how 
  // transparent objects are rendered (quality/speed trade off).
  virtual void setTransparencyType(SoGLRenderAction::TransparencyType type);
  virtual SoGLRenderAction::TransparencyType getTransparencyType() const;

  // Anti-aliasing methods.
  virtual void setAntialiasing(SbBool smoothing, int numPasses);
  virtual void getAntialiasing(SbBool &smoothing, int &numPasses) const;

  // Enable/prevent window clearing from happening before a rendering
  // starts (default is clearBeforeRender TRUE).
  virtual void setClearBeforeRender(SbBool trueOrFalse);
  virtual SbBool isClearBeforeRender() const;
  virtual void setClearBeforeOverlayRender(SbBool trueOrFalse);
  virtual SbBool isClearBeforeOverlayRender() const;

  // The render area will automatically redraw whenever something
  // in the scene graph changes. Passing FALSE to setAutoRedraw()
  // will disable this feature. NOTE: the render area will always
  // redraw in response to window system events (e.g. resize, exposure)
  // regardless of the setting of the auto redraw flag.
  virtual void setAutoRedraw(SbBool trueOrFalse);
  virtual SbBool isAutoRedraw() const;

  // Sets/gets the priority of the redraw sensor and get the
  // default priority number.
  virtual void setRedrawPriority(uint32_t priority);
  virtual uint32_t getRedrawPriority() const;
  static uint32_t getDefaultRedrawPriority();

  // Calling this forces the render area to be redrawn now.
  // It is not necessary to call this method if auto redraw is enabled
  // (which is the default).
  virtual void render();
  virtual void renderOverlay();

  // Schedule a redraw to happen sometime soon. This will schedule the redraw sensor
  // if auto-redraw is TRUE (default) else cause an immediate redraw of the scene if
  // auto-redraw if FALSE.
  virtual void scheduleRedraw();
  virtual void scheduleOverlayRedraw();
  
  // Call this convenience method to have this render area redraw
  // whenever the selection list changes in the passed node. This
  // is useful if using a highlight render action like the
  // SoBoxHighlightRenderAction to correctly render whenever the
  // selection changes. Pass NULL to turn this off.
  virtual void redrawOnSelectionChange(SoSelection *s);
  virtual void redrawOverlayOnSelectionChange(SoSelection *s);

  // Register an application callback for handling events that
  // occurs in the window, instead of sending them down the graph.
  // Callback should return TRUE if it handled the event.
  // If callback returns FALSE, then the event will be sent
  // to the scene graph.
  virtual void setEventCallback(SoWxRenderAreaEventCB *fcn, void * userData = NULL);

  // Access to the scene manager
  virtual void setSceneManager(SoSceneManager *sm);
  virtual SoSceneManager * getSceneManager() const;
  virtual void setOverlaySceneManager(SoSceneManager *sm);
  virtual SoSceneManager * getOverlaySceneManager() const;
  
  // Access to GL render action
  virtual void setGLRenderAction(SoGLRenderAction *ra);
  virtual SoGLRenderAction * getGLRenderAction() const;

  // Access to overlay GL render action
  virtual void setOverlayGLRenderAction(SoGLRenderAction *ra);
  virtual SoGLRenderAction * getOverlayGLRenderAction() const;
  

  /////////////////////////////////////////////////
  // GLWidget funcs
  virtual void setDrawToFrontBufferEnable(SbBool enableFlag);
  virtual SbBool isDrawToFrontBufferEnable() const;
  virtual SbBool isDrawToFrontBuffer() const;
  virtual void setDoubleBuffer(SbBool onOrOff);
  virtual SbBool isDoubleBuffer() const;
  virtual SbBool isRGBMode() const;
  virtual const SbVec2s & getGlxSize() const;
  virtual SbBool isVisible();
  virtual void processRenderAreaEvent( wxEvent * event );

protected:

  // redraw() calls actualRedraw(), followed by swapbuffers if necessary.
  // actualRedraw will have the scene manager render the scene. Rendering
  // is broken up into two routines like this so that subclasses can
  // redefine or simply add to rendering (in actualRedraw) without having
  // to worry about being visible, seting up the window or 
  // single/double buffer swapping.
  virtual void redraw();
  virtual void actualRedraw();
  virtual void redrawOverlay();
  virtual void actualOverlayRedraw();

  // Redefine these to do Inventor-specific things
  virtual void processEvent( wxEvent * event );
  virtual void initGraphic();
  virtual void initOverlayGraphic();
  virtual void sizeChanged(const SbVec2s &newSize);
  // @todo Need fix.
  // virtual void widgetChanged(Widget);

  // @todo Need fix.
  // Widget buildWidget(Widget parent);
  
  // redefine these
  virtual const char * getDefaultWidgetName() const;
  virtual const char * getDefaultTitle() const;
  virtual const char * getDefaultIconTitle() const;
  
  // subclasses have access to the device list for event processing
  SbPList * p_deviceList; // list of devices

  // application event callback variables
  SoWxRenderAreaEventCB * p_appEventHandler;
  void * p_appEventHandlerData;
  // invoke the application event callback - returns what the app cb returns
  virtual SbBool invokeAppCB(wxEvent * event);

  friend class SoWxViewer;

private:
  DECLARE_EVENT_TABLE()

  // these make rendering/redrawing happen
  SoSceneManager * p_sceneMgr; // this manages rendering and events
  SoSceneManager * p_overlaySceneMgr; // manages the overlay scene graph
  SbBool p_clearFirst, p_clearOverlayFirst; // call clear() before rendering
  SbBool p_autoRedraw; // if TRUE, then we automatically redraw
  XColor * p_mapColors, * p_overlayMapColors; // saved colors
  int p_mapColorNum, p_overlayMapColorNum; // number of saved colors

  SoWxMouse * p_mouseDevice;
  SoWxKeyboard * p_keyboardDevice;
  //@todo Need fix.
  // void reinstallDevices(Widget newWidget);

  static void selectionChangeCB(void *userData, SoSelection *s);
  static void overlaySelectionChangeCB(void *userData, SoSelection *s);
  SoSelection * p_selection, * p_overlaySelection;
  //@todo Need fix.
  //  Widget deviceWidget;
  
  // static callbacks
  SbBool p_firstEvent; // used to init the action SoWindowElement
  // @todo Need fix.
  // static void windowEventCB(Widget, SoXtRenderArea *, XAnyEvent *, SbBoolean *);
  static void renderCB(void *, SoSceneManager *);
  static void renderOverlayCB(void *, SoSceneManager *);
  
  static void visibilityChangeCB(void *pt, SbBool visible);
  void activate(); // connects the sensor
  void deactivate(); // disconnects the sensor
  
  // this is called by constructors
  void constructorCommon(SbBool getMouseInput, SbBool getKeyboardInput, SbBool buildNow);



  // wx event handlers which all calls processEvent functions
  void OnMouse( wxMouseEvent& event );
  void OnKeyboard( wxKeyEvent& event );
  void OnFocus( wxFocusEvent& event );

  void OnPaint(wxPaintEvent& event);
  void OnSize(wxSizeEvent& event);
  void OnEraseBackground(wxEraseEvent& event);



  /////////////////////////////////////////////////
  // GLWidget vars
  int p_glModes;
  SbBool p_waitForExpose;
  SbBool p_drawToFrontBuffer;
  SbBool p_enableDrawToFrontBuffer;
  SbVec2s p_glxSize;
  bool p_windowResized;

  bool p_inPaint;
};

#endif
