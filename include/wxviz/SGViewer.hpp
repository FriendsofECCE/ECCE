/**
 * @file
 *
 */
#ifndef SGVIEWER_H
#define SGVIEWER_H

#include "inv/actions/SoGLRenderAction.H"
#include <stack>

#include "util/Receiver.H"

#include "inv/SoWx/SoWxExaminerViewer.H"

using std::stack;
#include <vector>
using std::vector;

class SoCallback;
class RTDraggerNode;
class SGSelection;
class VR3dXVSmouse;
class SoAction;
class MouseEventListener;

class SGViewer : public SoWxExaminerViewer, public Receiver
{
public:
  SGViewer( wxWindow* parent,
            wxWindowID id = -1,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxSize(400, 300),
            long style = 0,
            Type type = BROWSER,
            SoCamera *cameraToCopy = 0 );
  ~SGViewer();

  //  Widget getSelectModeWidget();
  //  Widget getManipulateModeWidget();

  void assignCamera(SGViewer *src);
  //  void setOrthographicView();
  
  static bool checkExtension(const char *extName, const char *extString);

  // Redefine SoWxExaminerViewer methods
  virtual void processEvent(wxEvent * event);

  void addMouseEventListener(MouseEventListener *l);
  void rmMouseEventListener(MouseEventListener *l);

  // ICommandObject virtuals
  string commandObjectType() const;

  SbColor calculateContrastingColor();

  void setTransparencyType(SoGLRenderAction::TransparencyType type);

  
  virtual void setSceneGraph(SoNode *); 
  SoNode *getSceneGraph();

  //  Widget getTopForm();
  SoNode *getTopNode(); 
  void set3DMouse(SbBool);
  SoSeparator *getAxisRoot();
  //  SoSeparator *getTopMickey();
  //  void setViewAll(SbBool);
  RTDraggerNode *getMickey();
  
  SGSelection *getSel();
  //  SbBool doViewAll;

  void initializeText(void);
  void showTextLabels(bool show);
  void setText(const string& lleft, const string& lright, const string& uleft, const string& uright);
  void setLLeftText(const string& text);
  void setLRightText(const string& text);
  void setULeftText(const string& text);
  void setURightText(const string& text);

  virtual SbBool processCommonEvents(wxEvent * event);

  virtual void setStickyMode(int mode);

  virtual void updateViewerMode(wxMouseEvent * event);

  virtual void setCursor(string cursorName);

  virtual void rotateCamera(const SbRotation& rot);

  virtual void zoomCamera(float delta);

  virtual void resetToHomePosition();

  void setOrientation(float twist, float elevation, float azimuth);
  void getOrientation(float &twist, float &elevation, float &azimuth) const;
  void touchOrientation();

  void setSelectModeDrag(const bool& dragFlag);
  bool getSelectModeDrag();

protected:

  int p_stickyMode;
  bool p_showText;
  bool p_dragFlag;

  SoGroup * p_newRoot;
  SoGroup * p_topRoot;
  SGSelection * p_ecceSel;
  SoSeparator * p_topMickey;
  SoSeparator * p_topAxis;
  VR3dXVSmouse * p_mouse3d;

  stack<wxCursor> p_tmpCursor;
  
  // All of these variables and methods support the z rotator
  bool p_zWheelFirstDrag;
  float p_zWheelValue;
  //  Widget              p_autozclip;
  //  hScale             *p_zcenter;
  //  hScale             *p_zdepth;
  void enableCustomZClip(bool set);
  void updateCustomClipPlanes();
  //  static void zWheelCB(Widget, VRViewer *v, XtPointer *d);
  //  static void autoclipValueChangedCB(Widget, VRViewer *v, XtPointer *d);
  //  static void zdepthActivateCB(Widget, VRViewer *v, XtPointer *d);
  //  static void zcenterActivateCB(Widget, VRViewer *v, XtPointer *d);
  
  static void copyCamera(SoCamera *dest, const SoCamera *src);

  // For the stationary text 
  vector<string> p_cornerText;
  SoCallback * p_myCallback;
  static void myCallbackRoutine(void *, SoAction *);

  vector<MouseEventListener*> p_mouseListeners;

  void saveCursor();
  void restoreCursor();

  float p_twist, p_azimuth, p_elevation;
};

#endif
