#ifndef _SOWXVIEWER_H_
#define _SOWXVIEWER_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "SoWxViewer.cpp"
#endif

#include <wx/wx.h>

#include <X11/Intrinsic.h>

#include "inv/SoType.H"
#include "inv/SbTime.H"

#define ID_VIEWER 1000000

class SbViewportRegion;
class SbColor;

class SoCamera;
class SoWxInputFocus;
class SoFieldSensor;
class SoSwitch;
class SoDrawStyle;
class SoLightModel;
class SoPackedColor;
class SoMaterialBinding;
class SoComplexity;
class SoDirectionalLight;
class SoRotation;
class SoNode;
class SoSceneManager;
class SoSFTime;
class SoSeparator;
class SoGetBoundingBoxAction;
class SoPathList;
class SoSensor;
class SoGroup;
class SoCallbackList;

class SoWxRenderArea;
class SoWxViewer;
class SoWxDevice;

// callback function prototypes
typedef void SoWxViewerCB(void *userData, SoWxViewer *viewer);


class SoWxViewer: public wxPanel
{
DECLARE_DYNAMIC_CLASS( SoWxViewer )
DECLARE_EVENT_TABLE()

public:

  // An EDITOR viewer will create a camera under the user supplied scene
  // graph (specified in setSceneGraph()) if it cannot find one in the
  // scene and will leave the camera behind when supplied with a new scene.
  //
  // A BROWSER viewer will also create a camera if it cannot find one in
  // the scene, but will place it above the scene graph node (camera will
  // not appear in the user supplied scene graph), and will automatically
  // remove it when another scene is supplied to the viewer.
  enum Type {
    BROWSER, // default viewer type
    EDITOR
  };

  // list of possible drawing styles
  // Note: Refer to the SoWxViewer man pages for a complete
  // description of those draw styles.
  enum DrawStyle {
    VIEW_AS_IS, // unchanged
    VIEW_HIDDEN_LINE, // render only the front most lines
    VIEW_NO_TEXTURE, // render withought textures
    VIEW_LOW_COMPLEXITY, // render low complexity and no texture
    VIEW_LINE, // wireframe draw style
    VIEW_POINT, // point draw style
    VIEW_BBOX, // bounding box draw style
    VIEW_LOW_RES_LINE, // low complexity wireframe + no depth clearing
    VIEW_LOW_RES_POINT, // low complexity point + no depth clearing
    VIEW_SAME_AS_STILL // forces the INTERACTIVE draw style to match STILL
  };

  enum DrawType {
    STILL, // default to VIEW_NO_TEXTURE (or VIEW_AS_IS)
    INTERACTIVE // default to VIEW_SAME_AS_STILL
  };

  // list of different buffering types
  enum BufferType {
    BUFFER_SINGLE,
    BUFFER_DOUBLE,
    BUFFER_INTERACTIVE // changes to double only when interactive
  };

  /// Constructors
  SoWxViewer( );
  SoWxViewer( wxWindow* parent,
              wxWindowID id = -1,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxSize(400, 300),
              long style = 0,
              Type type = BROWSER );

  ~SoWxViewer( );


  /// Creation
  SbBool Create( wxWindow* parent,
                 wxWindowID id = -1,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxSize(400, 300),
                 long style = 0,
                 Type type = BROWSER );

  /// Creates the controls and sizers
  virtual void CreateControls();

  // Sets/gets the scene graph to render. Whenever a new scene is supplied
  // the first camera encountered will be by default used as the edited
  // camera, else a new camera will be created.
  virtual void setSceneGraph(SoNode *newScene);
  virtual SoNode * getSceneGraph();


  // Set and get the edited camera. setCamera() is only needed if the
  // first camera found when setSceneGraph() is called isn't the one
  // the user really wants to edit.
  virtual void setCamera(SoCamera *cam);
  virtual SoCamera * getCamera();

  // Set and get the camera type that will be created by the viewer if no
  // cameras are found in the scene graph. Possible choices are :
  //	    - SoPerspectiveCamera::getClassTypeId() 
  //	    - SoOrthographicCamera::getClassTypeId()
  //
  // NOTE: the set method will only take affect next time a scene graph
  // is specified (and if no camera are found).
  //
  // By default a perspective camera will be created if needed.
  virtual void setCameraType(SoType type);
  virtual SoType getCameraType();
  
  // Camera routines.
  virtual void viewAll();
  virtual void saveHomePosition();
  virtual void resetToHomePosition();

  // Turns the headlight on/off. (default ON)
  virtual void setHeadlight(SbBool onOrOff);
  virtual SbBool isHeadlight();
  virtual SoDirectionalLight * getHeadlight();

  // Sets/gets the current drawing style in the main view - The user
  // can specify the INTERACTIVE draw style (draw style used when 
  // the scene changes) independently from the STILL style.
  //
  // STILL defaults to VIEW_AS_IS.
  // INTERACTIVE defaults to VIEW_NO_TEXTURE on machine that do not support
  // fast texturing, VIEW_SAME_AS_STILL otherwise.
  //
  // Refer to the SoXtViewer man pages for a complete description 
  // of those draw styles.
  virtual void setDrawStyle(SoWxViewer::DrawType type, 
                            SoWxViewer::DrawStyle style);
  virtual SoWxViewer::DrawStyle getDrawStyle(SoWxViewer::DrawType type);
  
  // Sets/gets the current buffering type in the main view.
  // (default BUFFER_DOUBLE)
  virtual void setBufferingType(SoWxViewer::BufferType type);
  virtual SoWxViewer::BufferType getBufferingType();
  
  // redefine this to call the viewer setBufferingType() method instead.
  virtual void setDoubleBuffer(SbBool onOrOff);
  
  // Set/get whether the viewer is turned on or off. When turned off
  // events over the renderArea are sent down the sceneGraph 
  // (picking can occurs). (default viewing is ON)
  virtual void setViewing(SbBool onOrOff);
  virtual SbBool isViewing() const;

  // Set/get whether the viewer is allowed to change the cursor over
  // the renderArea window. When disabled, the cursor is undefined
  // by the viewer and will not change as the mode of the viewer changes.
  // When re-enabled, the viewer will reset it to the appropriate icon.
  //
  // Disabling the cursor enables the application to set the cursor
  // directly on the viewer window or on any parent widget of the viewer.
  // This can be used when setting a busy cursor on the application shell.
  //
  // Subclasses should redefine this routine to call XUndefineCursor()
  // or XDefineCursor() with the appropariate glyth. The base class 
  // routine only sets the flag.
  virtual void setCursorEnabled(SbBool onOrOff);
  virtual SbBool isCursorEnabled() const;

  // Set and get the auto clipping plane. When auto clipping is ON, the 
  // camera near and far planes are dynamically adjusted to be as tight 
  // as possible (least amount of stuff is clipped). When OFF, the user
  // is expected to manually set those planes within the preference sheet.
  // (default is ON).
  virtual void setAutoClipping(SbBool onOrOff);
  virtual SbBool isAutoClipping() const;

  // Turns stereo viewing on/off on the viewer (default off). When in
  // stereo mode, which may not work on all machines, the scene is rendered
  // twice (in the left and right buffers) with an offset between the
  // two views to simulate stereo viewing. Stereo classes have to be used
  // to see the affect and /usr/gfx/setmon needs to be called to set the
  // monitor in stereo mode.
  //
  // The user can also specify what the offset between the two views 
  // should be.
  virtual void setStereoViewing(SbBool onOrOff);
  virtual SbBool isStereoViewing();
  virtual void setStereoOffset(float dist);
  virtual float getStereoOffset();

  // Seek methods
  //
  // Routine to determine whether or not to orient camera on
  // picked point (detail on) or center of the object's bounding box
  // (detail off). Default is detail on.
  virtual void setDetailSeek(SbBool onOrOff);
  virtual SbBool isDetailSeek();

  // Set the time a seek takes to change to the new camera location.
  // A value of zero will not animate seek. Default value is 2 seconds.
  virtual void setSeekTime(float seconds);
  virtual float getSeekTime();

  // add/remove start and finish callback routines on the viewer.
  // Start callbacks will be called whenever the user starts doing interactive
  // viewing (ex: mouse down), and finish callbacks are called when user is
  // done doing interactive work (ex: mouse up).
  //
  // Note: The viewer pointer 'this' is passed as callback data
  virtual void addStartCallback(SoWxViewerCB *f, void *userData = NULL);
  virtual void addFinishCallback(SoWxViewerCB *f, void *userData = NULL);
  virtual void removeStartCallback(SoWxViewerCB *f, void *userData = NULL);
  virtual void removeFinishCallback(SoWxViewerCB *f, void *userData = NULL);

  // copy/paste the view. eventTime should be the time of the X event
  // which initiated the copy or paste (e.g. if copy/paste is initiated
  // from a keystroke, eventTime should be the time in the X keyboard event.)
  virtual void copyView(Time eventTime);
  virtual void pasteView(Time eventTime);

  //////////////////////////////////////
  // @todo Not sure about this one
  // redefine this routine to also correctly set the buffering type
  // on the viewer.
  virtual void setNormalVisual(XVisualInfo *);

  // This can be used to let the viewer know that the scene graph
  // has changed so that the viewer can recompute things like speed which
  // depend on the scene graph size. 
  //
  // NOTE: This routine is automatically called whenever setSceneGraph() 
  virtual void recomputeSceneSize();


  // This routine will toggle the current camera from perspective to
  // orthographic, and from orthographic back to perspective.
  virtual void toggleCameraType();

  //////////////////////////////////////////
  // Forwarding funcs from SoWxRenderArea and GLWidget
  virtual const SbVec2s & getGlxSize() const;
  virtual const SbViewportRegion & getViewportRegion() const;
  virtual SbBool isDoubleBuffer();
  virtual void registerDevice(SoWxDevice *device);
  virtual void unregisterDevice(SoWxDevice *device);
  virtual SoSceneManager * getSceneManager() const;
  virtual SbBool isClearBeforeRender() const;

  virtual void actualRedraw();
  virtual void processEvent(wxEvent * event);

  virtual void setAntialiasing(SbBool smoothing, int numPasses);
  virtual void getAntialiasing(SbBool &smoothing, int &numPasses) const;
  virtual void setBackgroundColor(const SbColor &c);
  virtual const SbColor & getBackgroundColor() const;


  ///////////////////////////////////////////
  // SoWxFullViewer func
  // Show/hide the viewer component trims (default ON)
  void setDecoration(bool onOrOff);
  SbBool isDecoration();
  
  void setCameraZoom(float zoom);
  float getCameraZoom();

protected:
  wxBoxSizer * p_topSizer;
  wxBoxSizer * p_leftRowSizer;
  wxBoxSizer * p_centerSizer;
  wxBoxSizer * p_bottomRowSizer;
  wxBoxSizer * p_rightRowSizer;

  SoWxRenderArea * p_renderArea;

  SbBool p_decorationFlag;

  // global vars
  SoWxViewer::Type p_type;
  SoCamera * p_camera; // camera being edited
  SbBool p_viewingFlag; // FALSE when the viewer is off
  SbBool p_altSwitchBack; // flag to return to PICK after an Alt release
  SbBool p_cursorEnabledFlag;
  static SoSFTime * p_viewerRealTime; // pointer to "realTime" global field
  float p_sceneSize; // the larger of the X,Y,Z scene BBox
  float p_viewerSpeed; // default to 1.0 - SoXtFullViewer add UI to inc/dec

  // local tree variables
  SoSeparator * p_sceneRoot; // root node given to the RA
  SoNode * p_sceneGraph; // user supplied scene graph

  // @todo Fix it.
  // Subclasses can call this routine to handle a common set of events.
  // A SbBool is returned to specify whether the event was handled by the
  // base class. Currently handled events and functions are :
  //    'Esc' key - toggles viewing on/off
  //    When viewing OFF - send all events down the scene graph
  //    When camera == NULL - Discard all viewing events
  //    'home' Key - calls resetToHomePosition()
  //    's' Key - toggles seek on/off
  //    Arrow Keys - moves the camera up/down/right/left in the viewing plane
  virtual SbBool processCommonEvents(wxEvent * event);
    
  // Invokes the start and finish viewing callbacks. Subclasses NEED to call
  // those routines when they start and finish doing interactive viewing 
  // operations so that correct interactive drawing style and buffering 
  // types, as well as application callbacks, gets set and called properly.
  //
  // Those routines simply increment and decrement a counter. When the counter
  // changes from 0->1 the start viewing callbacks are called. When the counter
  // changes back from 1->0 the finish viewing callbacks are called.
  // The counter approach enables different parts of a viewer to call those 
  // routines withough having to know about each others (which is not 
  void interactiveCountInc();
  void interactiveCountDec();
  int getInteractiveCount();
  
  // This routine is used by subclasses to initiate the seek animation. Given a
  // screen mouse location, this routine will return the picked point
  // and the normal at that point. It will also schedule the sensor to animate
  // if necessary. The routine retuns TRUE if something got picked...
  //
  // Note: if detailSeek is on, the point and normal correspond to the exact 
  //	    3D location under the cursor.
  //	    if detailSeek if off, the object bbox center and the camera 
  //	    orientation are instead returned.
  SbBool seekToPoint(const SbVec2s &mouseLocation);

  // Subclasses CAN redefine this to interpolate camera position/orientation
  // while the seek animation is going on (called by animation sensor).
  // The parameter t is a [0,1] value corresponding to the animation percentage
  // completion. (i.e. a value of 0.25 means that animation is only 1/4 of the way
  // through).
  virtual void interpolateSeekAnimation(float t);
  virtual void computeSeekFinalOrientation();
  
  // variables used for interpolating seek animations
  float p_seekDistance;
  SbBool p_seekDistAsPercentage; // percentage/absolute flag
  SbBool p_computeSeekVariables;
  SbVec3f p_seekPoint, p_seekNormal;
  SbRotation p_oldCamOrientation, p_newCamOrientation;
  SbVec3f p_oldCamPosition, p_newCamPosition;

  // Externally set the viewer into/out off seek mode (default OFF). Actual
  // seeking will not happen until the viewer decides to (ex: mouse click).
  //
  // Note: setting the viewer out of seek mode while the camera is being
  // animated will stop the animation to the current location.
  virtual void setSeekMode(SbBool onOrOff);
  SbBool isSeekMode();
  
  // redefine this routine to adjust the camera clipping planes just
  // before doing a redraw. The sensor will be unschedule after the camera
  // is changed in the base class to prevent a second redraw from occuring.
  //  virtual void actualRedraw();
  
  // This is called during a paste.
  // Subclasses may wish to redefine this in a way that
  // keeps their viewing paradigm intact.
  virtual void changeCameraValues(SoCamera *newCamera);
  

  // Convenience routines which subclasses can use when drawing viewer 
  // feedback which may be common across multiple viewers. There is for 
  // example a convenience routine which sets an orthographics projection
  // and a method to draw common feedback like the roll anchor (used by
  // a couple of viewers).
  //
  // All drawing routines assume that the window and projection is 
  // already set by the caller.
  //
  // set an ortho projection of the glx window size - this also turns
  // zbuffering off and lighting off (if necessary).
  static void setFeedbackOrthoProjection(const SbVec2s &glxSize);
  // restores the zbuffer and lighting state that was changed when
  // setFeedbackOrthoProjection() was last called.
  static void restoreGLStateAfterFeedback();
  // draws a simple 2 colored cross at given position
  static void drawViewerCrossFeedback(SbVec2s loc);
  // draws the anchor feedback given center of rotation and mouse location
  static void drawViewerRollFeedback(SbVec2s center, SbVec2s loc);
  
  // redefine this for a better default draw style (need to wait for context)
  virtual void afterRealizeHook();
  
  // auto clipping vars and routines
  SbBool p_autoClipFlag;
  float p_minimumNearPlane; // minimum near plane as percentage of far
  SoGetBoundingBoxAction * p_autoClipBboxAction;
  virtual void adjustCameraClippingPlanes();



  // @todo The following funcs were declared as private in Xt version
  //       Not sure if it is necessary.
  // private:

  // current state vars
  SoType p_cameraType;
  BufferType p_bufferType;
  SbBool p_interactiveFlag; // TRUE while doing interactive work
  float p_stereoOffset;
  SoWxInputFocus * p_inputFocus;

  // draw style vars
  DrawStyle p_stillDrawStyle, p_interactiveDrawStyle;
  SbBool p_checkForDrawStyle;
  SoSwitch * p_drawStyleSwitch;   // on/off draw styles
  SoDrawStyle * p_drawStyleNode;	    // LINE vs POINT
  SoLightModel * p_lightModelNode;    // BASE_COLOR vs PHONG
  SoPackedColor * p_colorNode;	    // hidden line first pass
  SoMaterialBinding * p_matBindingNode;    // hidden line first pass
  SoComplexity * p_complexityNode;    // low complexity & texture off
  void setCurrentDrawStyle(SoWxViewer::DrawStyle style);
  void doRendering();

  // copy and paste support
  // @todo SoWxClipboard
  //  SoXtClipboard * p_clipboard;
  static void pasteDoneCB(void *userData, SoPathList *pathList);

  // camera original values, used to restore the camera
  SbBool p_createdCamera;
  SbVec3f p_origPosition;
  SbRotation p_origOrientation;
  float p_origNearDistance;
  float p_origFarDistance;
  float p_origFocalDistance;
  float p_origHeight;

#ifdef __sgi
  // set to TRUE when we are using the SGI specific stereo extensions
  // which enables us to emulate OpenGL stereo on most machines.
  // We also save the camera original aspect ratio and viewport mapping
  // since we need to temporary strech the camera aspect ratio.
  SbBool p_useSGIStereoExt;
  float p_camStereoOrigAspect;
  int p_camStereoOrigVPMapping;
#endif
  
  // seek animation vars
  SbBool p_seekModeFlag; // TRUE when seek turned on externally
  SoFieldSensor * p_seekAnimationSensor;
  SbBool p_detailSeekFlag;
  float p_seekAnimTime;
  SbTime p_seekStartTime;
  static void seekAnimationSensorCB(void *p, SoSensor *);

  // headlight variables
  SoDirectionalLight * p_headlightNode;
  SoGroup * p_headlightGroup;
  SoRotation * p_headlightRot;
  SbBool p_headlightFlag; // true when headlight in turned on

  // interactive viewing callbacks
  int p_interactiveCount;
  SoCallbackList * p_startCBList;
  SoCallbackList * p_finishCBList;
  static void drawStyleStartCallback(void *, SoWxViewer *v);
  static void drawStyleFinishCallback(void *, SoWxViewer *v);
  static void bufferStartCallback(void *, SoWxViewer *v);
  static void bufferFinishCallback(void *, SoWxViewer *v);
  
  // set the zbuffer on current window to correct state
  void setZbufferState();
  SbBool isZbufferOff();
  void arrowKeyPressed(KeySym key);
};

#endif
