/**
 * @file
 *
 */

#ifndef _VIEWEREVTHANDLER_H_
#define _VIEWEREVTHANDLER_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "ViewerEvtHandler.C"
#endif

#include "wx/event.h"
#include "wxviz/WxVizTool.H"

#include "util/Event.H"
#include "util/Listener.H"
  using namespace ecce;


class VizStyleChooser;

class ViewerEvtHandler : public wxEvtHandler, public WxVizTool, Listener
{

  DECLARE_CLASS(ViewerEvtHandler)
  DECLARE_EVENT_TABLE()

public:
  static const wxWindowID ID_CLEAR_RESIDUE;
  static const wxWindowID ID_MAKE_RESIDUE;
  static const wxWindowID ID_GENERATE_BOND;
  static const wxWindowID ID_ORIENT;
  static const wxWindowID ID_CENTER_XYZ;
  static const wxWindowID ID_CENTER_X;
  static const wxWindowID ID_CENTER_Y;
  static const wxWindowID ID_CENTER_Z;

  static const wxWindowID ID_STYLE_BALL_WIREFRAME;
  static const wxWindowID ID_STYLE_BALL_STICK;
  static const wxWindowID ID_STYLE_STICK;
  static const wxWindowID ID_STYLE_WIREFRAME;
  static const wxWindowID ID_STYLE_CPK;
  static const wxWindowID ID_STYLE_RESIDUE_WIRE;
  static const wxWindowID ID_STYLE_RESIDUE_CYLINDERS;
  static const wxWindowID ID_STYLE_LINE_RIBBON;
  static const wxWindowID ID_STYLE_FLAT_RIBBON;
  static const wxWindowID ID_STYLE_SOLID_RIBBON;
  static const wxWindowID ID_STYLE_CARTOON;
  static const wxWindowID ID_STYLE_CUSTOM;
  static const wxWindowID ID_STYLE;
  static const wxWindowID ID_STEREO;
  static const wxWindowID ID_DEPTH_CUEING;
  static const wxWindowID ID_TRANSPARENCY;
  static const wxWindowID ID_ANTIALIASING;
  static const wxWindowID ID_HIDE_SELECTED;
  static const wxWindowID ID_HIDE_UNSELECTED;
  static const wxWindowID ID_SHOW_HIDDEN;
  static const wxWindowID ID_FOREGROUND_COLOR;
  static const wxWindowID ID_BACKGROUND_COLOR;
  static const wxWindowID ID_CAMERA_TYPE;
  static const wxWindowID ID_CLEAN_COORD;

  static const wxWindowID ID_SHOW_HYDROGEN;
  static const wxWindowID ID_SHOW_HYDROGEN_BOND;
  static const wxWindowID ID_LABEL_NONE;
  static const wxWindowID ID_LABEL_ELEMENT;
  static const wxWindowID ID_LABEL_ATOM_NAME;
  static const wxWindowID ID_LABEL_CHARGE2;
  static const wxWindowID ID_LABEL_CHARGE3;
  static const wxWindowID ID_LABEL_CHARGE4;
  static const wxWindowID ID_LABEL_TYPE;
  static const wxWindowID ID_ATOM_LABEL;
  static const wxWindowID ID_BOND_LABEL;
  static const wxWindowID ID_RESIDUE_LABEL;
  static const wxWindowID ID_CORNER_LABELS;
  static const wxWindowID ID_SHOW_AXES;
  static const wxWindowID ID_SHOW_RING;
  static const wxWindowID ID_SELECT_DRAG;
  static const wxWindowID ID_SELECT_LASSO;
  static const wxWindowID ID_SELECT_ACTION;
  static const wxWindowID ID_AUTO_NORMALIZE;
  static const wxWindowID ID_ENABLE_SPINNING;
  static const wxWindowID ID_AUTO_RESIDUE;
  static const wxWindowID ID_CLEAN_PARAMETER;
  static const wxWindowID ID_RESET_PERSPECTIVE;

  static const wxWindowID ID_MEASURE_DIST;
  static const wxWindowID ID_MEASURE_ANGLE;
  static const wxWindowID ID_MEASURE_TORSION;
  static const wxWindowID ID_MEASURE_CLEAR;

  static const wxWindowID ID_MANIPULATOR_SPHERE;
  static const wxWindowID ID_MANIPULATOR_NOSPHERE;
  static const wxWindowID ID_MANIPULATOR_WHEEL;
  static const wxWindowID ID_MANIPULATOR_NOWHEEL;

  static const wxWindowID ID_ADD_H;
  static const wxWindowID ID_DEL_H;

  static const wxWindowID ID_RESET_STYLE;

  static const wxWindowID ID_ZOOM_TO;


  static void createMeasureMenu(wxMenu * menu);
  static void createBuildMenu(wxMenu * menu);
  static void createRenderMenu(wxMenu * menu);
  static void createOptionMenu(wxMenu * menu);

  ViewerEvtHandler(wxWindow * parent);
  virtual ~ViewerEvtHandler();
  
  virtual void centerCoord(wxCommandEvent & event);
  virtual void styleChange(wxCommandEvent & event);
  virtual void doStyleChange(const wxString& style);
  virtual void showAtomLabel(wxCommandEvent & event);
  virtual void showBondLabel(wxCommandEvent & event);
  virtual void selectModeAction(wxCommandEvent & event);
  virtual void showResidueLabel(wxCommandEvent & event);
  virtual void showCornerLabels(wxCommandEvent & event);
  virtual void depthCue(wxCommandEvent & event);
  virtual void showAxes(wxCommandEvent & event);
  virtual void enableAnimation(wxCommandEvent & event);
  virtual void cleanParameters(wxCommandEvent & event);
  virtual void showRing(wxCommandEvent & event);
  virtual void hideSelected(wxCommandEvent & event);
  virtual void hideUnselected(wxCommandEvent & event);
  virtual void showHidden(wxCommandEvent & event);
  virtual void resetStyle(wxCommandEvent & event);
  virtual void zoomFit(wxCommandEvent & event);
  virtual void zoomTo(wxCommandEvent & event);
  virtual void zoomIn(wxCommandEvent & event);
  virtual void zoomOut(wxCommandEvent & event);
  virtual void foregroundColor(wxCommandEvent & event);
  virtual void backgroundColor(wxCommandEvent & event);
  virtual void toggleCamera(wxCommandEvent & event);
  virtual void cleanCoord(wxCommandEvent & event);
  virtual void toggleAutoCenter(wxCommandEvent & event);
  virtual void resetPerspective(wxCommandEvent & event);
  virtual void generateBond(wxCommandEvent & event);
  virtual void syncToggle(wxCommandEvent & event);

  virtual void measureDist(wxCommandEvent & event);
  virtual void measureAngle(wxCommandEvent & event);
  virtual void measureTorsion(wxCommandEvent & event);
  virtual void measureClear(wxCommandEvent & event);

  virtual void addSphereManip(wxCommandEvent & event);
  virtual void delSphereManip(wxCommandEvent & event);
  virtual void addWheelManip(wxCommandEvent & event);
  virtual void delWheelManip(wxCommandEvent & event);

  virtual void addHydrogen(wxCommandEvent & event);
  virtual void delHydrogen(wxCommandEvent & event);
  virtual void showHydrogen(wxCommandEvent & event);
  virtual void showHydrogenBond(wxCommandEvent & event);
  virtual void orient(wxCommandEvent & event);
  virtual void clearResidue(wxCommandEvent & event);
  virtual void makeResidue(wxCommandEvent & event);

  virtual void eventMCB(const Event& event);

  virtual void saveSettings(wxConfig * config);
  virtual void restoreSettings(wxConfig * config);

  virtual void setBackground(float r, float g, float b);

protected:
  wxWindow * p_parent;
  bool p_resetPerspective;
  VizStyleChooser *p_styleChooser;
  wxString p_lastStyle;
  static map<wxWindowID,wxString> p_styleNames;
  int p_cameraType;
  int p_autoCenter;
};

#endif
