#ifndef _SOWXEXAMINERVIEWER_H_
#define _SOWXEXAMINERVIEWER_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "SoWxExaminerViewer.cpp"
#endif

#include <wx/wx.h>
#include "inv/SbLinear.H"

#include "inv/SoWx/SoWxViewer.H"

class SbSphereSheetProjector;
class SoFieldSensor;
class SoSeparator;
class SoSwitch;
class SoTranslation;
class SoScale;

/**
 * Examiner viewer - allows the user to change the camera position
 * by spinning a sphere in front of the viewpoint.
 *
 * Keys used by this viewer:
 * -------------------------
 * Left Mouse: Tumbles the virtual trackball.
 *
 * Middle Mouse: 
 * Ctrl + Left Mouse: Translate up, down, left and right.
 *
 * Ctrl + Middle Mouse: 
 * Left + Middle Mouse: Dolly in and out (gets closer to and further
 *                      away from the object).
 * <s> + click: Alternative to the Seek button. Press (but do not hold 
 *              down) the <s> key, then click on a target object.
 *
 * Right Mouse: Open the popup menu.
 */
class SoWxExaminerViewer: public SoWxViewer
{
DECLARE_DYNAMIC_CLASS( SoWxExaminerViewer )
DECLARE_EVENT_TABLE()

public:

 enum ViewerModes {
   PICK_MODE,
   VIEW_MODE,
   SPIN_MODE_ACTIVE,
   PAN_MODE,
   PAN_MODE_ACTIVE,
   DOLLY_MODE,
   DOLLY_MODE_ACTIVE,
   SEEK_MODE
 };


  /// Constructors
  SoWxExaminerViewer( wxWindow* parent,
                      wxWindowID id = -1,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxSize(400, 300),
                      long style = 0,
                      Type type = BROWSER );

  SoWxExaminerViewer( );

  ~SoWxExaminerViewer( );

  // Show/hide the point of rotation feedback, which only appears while
  // in Viewing mode. (default OFF)
  void setFeedbackVisibility(SbBool onOrOff);
  SbBool isFeedbackVisible() const;
  
  // Set/get the point of rotation feeedback size in pixels (default 20). 
  void setFeedbackSize(int newSize);
  int getFeedbackSize() const;

  // Enable/disable the animation feature of the viewer. 
  // (enabled by default)
  void setAnimationEnabled(SbBool onOrOff);
  SbBool isAnimationEnabled();

  // Stop animation, if it is occurring, and queuery if the viewer is 
  // currently animating.
  void stopAnimating();
  SbBool isAnimating();

  // redefine these to add Examiner viewer functionality
  virtual void viewAll();
  virtual void resetToHomePosition();
  virtual void setCamera(SoCamera *cam);
  virtual void setViewing(SbBool onOrOff);
  virtual void setCursorEnabled(SbBool onOrOff);

// protected:
  // redefine these
  // @todo
  //  virtual const char * getDefaultWidgetName() const;
  //  virtual const char * getDefaultTitle() const;
  //  virtual const char * getDefaultIconTitle() const;
  
  // redefine those routines to do viewer specific stuff
  virtual void	processEvent(wxEvent * event);
  virtual void	setSeekMode(SbBool onOrOff);
  virtual void	actualRedraw();
  
protected:
  // @todo
  // Get X resources for the widget.
  //  Widget 		buildWidget(Widget parent);
  
  // Define those thumb wheels to rotate the object
  virtual void bottomWheelMotion(float newVal);
  virtual void leftWheelMotion(float newVal);
  virtual void rightWheelMotion(float newVal);
  
  // redefine those routines to also stop animation if any
  virtual void bottomWheelStart();
  virtual void leftWheelStart();
  
  // add viewer preference stuff
  virtual void createPrefSheet();
  
  // add some viewer buttons
  virtual void createViewerButtons(wxWindow * parent);
  
  // Define this to bring the viewer help card
  virtual void openViewerHelpCard();
  
  // viewer state variables
  int p_mode;
  SbBool p_createdCursors;
  wxCursor * p_spinCursor, * p_panCursor, * p_dollyCursor, * p_seekCursor;
  SbSphereSheetProjector * p_sphereSheet;
  SbVec2s p_locator; // mouse position
  SbBool p_firstBuild; // set FALSE after buildWidget called once
  
  // point of rotation feedback vars
  SbBool p_feedbackFlag;
  float p_feedbackSize;
  SoSeparator * p_feedbackRoot;
  SoSwitch * p_feedbackSwitch;
  SoTranslation * p_feedbackTransNode;
  SoScale * p_feedbackScaleNode;
  static const char * p_geometryBuffer;
  void createFeedbackNodes();
  
  // variables used for doing spinning animation
  SbBool p_animationEnabled, p_animatingFlag;
  SoFieldSensor * p_animationSensor;
  SbRotation * p_rotBuffer;
  int p_firstIndex, p_lastIndex;
  int32_t p_lastMotionTime;
  SbRotation p_averageRotation;
  SbBool p_computeAverage;
  static void animationSensorCB(void *v, SoSensor *s);
  static void visibilityChangeCB(void *pt, SbBool visible);
  void doSpinAnimation();
  
  // camera panning vars
  SbVec3f p_locator3D;
  SbPlane p_focalplane;
  
  virtual void updateViewerMode(wxMouseEvent * event);
  virtual void switchMode(int newMode);
  virtual void updateCursor();
  virtual void defineCursors();
  void rotateCamera(const SbRotation &rot);
  void panCamera(const SbVec2f &newLocator);
  void spinCamera(const SbVec2f &newLocator);
  void dollyCamera(const SbVec2s &newLocator);
  
  // @todo
  // preference sheet stuff
  //   Widget createExamPrefSheetGuts(Widget parent);
  //   static void animPrefSheetToggleCB(Widget, SoXtExaminerViewer *, void *);
  
  // @todo
  // point of rotation pref sheet stuff
  //   int feedbackSizeWheelVal;
  //   Widget feedbackLabel[2], feedbackField, feedbackSizeWheel;
  //   static void feedbackSizeWheelCB(Widget, SoXtExaminerViewer *p, XtPointer *d);
  //   static void feedbackSizeFieldCB(Widget, SoXtExaminerViewer *, void *);
  //   static void feedbackPrefSheetToggleCB(Widget, SoXtExaminerViewer *, void *);
  //   void toggleFeedbackWheelSize(Widget toggle);
  
  // @todo
  // push button vars and callbacks
  //  SoXtBitmapButton  *buttonList[10];
  //  static void camPushCB(Widget, SoXtExaminerViewer *, void *);
  
  // this is called by both constructors
  void constructorCommon();

  /////////////////////////////////////
  // Full viewer data members
  float p_bottomWheelVal, p_leftWheelVal, p_rightWheelVal;

};

#endif
