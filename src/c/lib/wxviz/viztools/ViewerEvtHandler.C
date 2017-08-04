/**
 * @file
 *
 */

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ViewerEvtHandler.H"
#endif

#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/EventDispatcher.H"
#include "util/Event.H"
#include "util/CancelException.H"

#include "wxgui/CleanParameters.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxColorDialog.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ewxConfig.H"

#include "viz/CustomBBoxNode.H"
#include "viz/AtomLabelsCmd.H"
#include "viz/CenterCmd.H"
#include "viz/CleanCmd.H"
#include "viz/ClearMeasuresCmd.H"
#include "viz/ClearResidueInfoCmd.H"
#include "viz/MakeResidueCmd.H"
#include "viz/CSStyleCmd.H"
#include "viz/DisplayAtomsCmd.H"
#include "viz/DepthCueCmd.H"
#include "viz/ForegroundCmd.H"
#include "viz/GenBondsCmd.H"
#include "viz/HydrogenBondsCmd.H"
#include "viz/HydrogensCmd.H"
#include "viz/MeasureAngleCmd.H"
#include "viz/MeasureDistCmd.H"
#include "viz/MeasureTorsionCmd.H"
#include "viz/OrientCmd.H"
#include "viz/RingsCmd.H"
#include "viz/RmHydrogensCmd.H"
#include "viz/ShowHydrogensCmd.H"
#include "viz/AtomMoveCmd.H"
#include "viz/BondRotateCmd.H"
#include "viz/ClearAtomMoveCmd.H"
#include "viz/ClearBondRotateCmd.H"

#include "wxviz/AxisCmd.H"
#include "wxviz/BackgroundCmd.H"
#include "wxviz/CenterViewCmd.H"
#include "wxviz/ViewerEvtHandler.H"
#include "wxviz/WxVizToolFW.H"
#include "wxviz/VizStyleChooser.H"


IMPLEMENT_CLASS(ViewerEvtHandler, wxEvtHandler)


const wxWindowID ViewerEvtHandler::ID_CLEAR_RESIDUE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_MAKE_RESIDUE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_GENERATE_BOND = wxNewId();
const wxWindowID ViewerEvtHandler::ID_ORIENT = wxNewId();
const wxWindowID ViewerEvtHandler::ID_CENTER_XYZ = wxNewId();
const wxWindowID ViewerEvtHandler::ID_CENTER_X = wxNewId();
const wxWindowID ViewerEvtHandler::ID_CENTER_Y = wxNewId();
const wxWindowID ViewerEvtHandler::ID_CENTER_Z = wxNewId();

const wxWindowID ViewerEvtHandler::ID_STYLE_BALL_WIREFRAME = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE_BALL_STICK = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE_STICK = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE_WIREFRAME = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE_CPK = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE_RESIDUE_WIRE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE_RESIDUE_CYLINDERS = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE_LINE_RIBBON = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE_FLAT_RIBBON = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE_SOLID_RIBBON = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE_CARTOON = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE_CUSTOM = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STYLE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_STEREO = wxNewId();
const wxWindowID ViewerEvtHandler::ID_DEPTH_CUEING = wxNewId();
const wxWindowID ViewerEvtHandler::ID_TRANSPARENCY = wxNewId();
const wxWindowID ViewerEvtHandler::ID_ANTIALIASING = wxNewId();
const wxWindowID ViewerEvtHandler::ID_HIDE_SELECTED = wxNewId();
const wxWindowID ViewerEvtHandler::ID_HIDE_UNSELECTED = wxNewId();
const wxWindowID ViewerEvtHandler::ID_SHOW_HIDDEN = wxNewId();
const wxWindowID ViewerEvtHandler::ID_FOREGROUND_COLOR = wxNewId();
const wxWindowID ViewerEvtHandler::ID_BACKGROUND_COLOR = wxNewId();
const wxWindowID ViewerEvtHandler::ID_CAMERA_TYPE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_CLEAN_COORD = wxNewId();

const wxWindowID ViewerEvtHandler::ID_SHOW_HYDROGEN = wxNewId();
const wxWindowID ViewerEvtHandler::ID_SHOW_HYDROGEN_BOND = wxNewId();
const wxWindowID ViewerEvtHandler::ID_LABEL_NONE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_LABEL_ELEMENT = wxNewId();
const wxWindowID ViewerEvtHandler::ID_LABEL_ATOM_NAME = wxNewId();
const wxWindowID ViewerEvtHandler::ID_LABEL_CHARGE2 = wxNewId();
const wxWindowID ViewerEvtHandler::ID_LABEL_CHARGE3 = wxNewId();
const wxWindowID ViewerEvtHandler::ID_LABEL_CHARGE4 = wxNewId();
const wxWindowID ViewerEvtHandler::ID_LABEL_TYPE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_ATOM_LABEL = wxNewId();
const wxWindowID ViewerEvtHandler::ID_BOND_LABEL = wxNewId();
const wxWindowID ViewerEvtHandler::ID_RESIDUE_LABEL = wxNewId();
const wxWindowID ViewerEvtHandler::ID_CORNER_LABELS = wxNewId();
const wxWindowID ViewerEvtHandler::ID_SHOW_AXES = wxNewId();
const wxWindowID ViewerEvtHandler::ID_SHOW_RING = wxNewId();
const wxWindowID ViewerEvtHandler::ID_SELECT_DRAG = wxNewId();
const wxWindowID ViewerEvtHandler::ID_SELECT_LASSO = wxNewId();
const wxWindowID ViewerEvtHandler::ID_SELECT_ACTION = wxNewId();
const wxWindowID ViewerEvtHandler::ID_AUTO_NORMALIZE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_ENABLE_SPINNING = wxNewId();
const wxWindowID ViewerEvtHandler::ID_AUTO_RESIDUE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_CLEAN_PARAMETER = wxNewId();
const wxWindowID ViewerEvtHandler::ID_RESET_PERSPECTIVE = wxNewId();

const wxWindowID ViewerEvtHandler::ID_MEASURE_DIST = wxNewId();
const wxWindowID ViewerEvtHandler::ID_MEASURE_ANGLE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_MEASURE_TORSION = wxNewId();
const wxWindowID ViewerEvtHandler::ID_MEASURE_CLEAR = wxNewId();

const wxWindowID ViewerEvtHandler::ID_MANIPULATOR_SPHERE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_MANIPULATOR_NOSPHERE = wxNewId();
const wxWindowID ViewerEvtHandler::ID_MANIPULATOR_WHEEL = wxNewId();
const wxWindowID ViewerEvtHandler::ID_MANIPULATOR_NOWHEEL = wxNewId();

const wxWindowID ViewerEvtHandler::ID_ADD_H = wxNewId();
const wxWindowID ViewerEvtHandler::ID_DEL_H = wxNewId();

const wxWindowID ViewerEvtHandler::ID_RESET_STYLE = wxNewId();

const wxWindowID ViewerEvtHandler::ID_ZOOM_TO = wxNewId();


BEGIN_EVENT_TABLE(ViewerEvtHandler, wxEvtHandler)
  EVT_MENU(ID_CENTER_XYZ,
          ViewerEvtHandler::centerCoord)
  EVT_MENU(ID_CENTER_X,
          ViewerEvtHandler::centerCoord)
  EVT_MENU(ID_CENTER_Y,
          ViewerEvtHandler::centerCoord)
  EVT_MENU(ID_CENTER_Z,
          ViewerEvtHandler::centerCoord)
  EVT_MENU(ID_STYLE_BALL_WIREFRAME,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_STYLE_BALL_STICK,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_STYLE_STICK,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_STYLE_WIREFRAME,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_STYLE_CPK,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_STYLE_RESIDUE_WIRE,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_STYLE_RESIDUE_CYLINDERS,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_STYLE_LINE_RIBBON,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_STYLE_FLAT_RIBBON,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_STYLE_SOLID_RIBBON,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_STYLE_CARTOON,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_STYLE_CUSTOM,
          ViewerEvtHandler::styleChange)
  EVT_MENU(ID_LABEL_NONE,
          ViewerEvtHandler::showAtomLabel)
  EVT_MENU(ID_LABEL_ELEMENT,
          ViewerEvtHandler::showAtomLabel)
  EVT_MENU(ID_LABEL_ATOM_NAME,
          ViewerEvtHandler::showAtomLabel)
  EVT_MENU(ID_LABEL_CHARGE2,
          ViewerEvtHandler::showAtomLabel)
  EVT_MENU(ID_LABEL_CHARGE3,
          ViewerEvtHandler::showAtomLabel)
  EVT_MENU(ID_LABEL_CHARGE4,
          ViewerEvtHandler::showAtomLabel)
  EVT_MENU(ID_LABEL_TYPE,
          ViewerEvtHandler::showAtomLabel)
  EVT_MENU(ID_BOND_LABEL,
           ViewerEvtHandler::showBondLabel)
  EVT_MENU(ID_RESIDUE_LABEL,
           ViewerEvtHandler::showResidueLabel)
  EVT_MENU(ID_CORNER_LABELS,
           ViewerEvtHandler::showCornerLabels)
  EVT_MENU(ID_DEPTH_CUEING,
           ViewerEvtHandler::depthCue)
  EVT_MENU(ID_SHOW_AXES,
           ViewerEvtHandler::showAxes)
  EVT_MENU(ID_ENABLE_SPINNING,
           ViewerEvtHandler::enableAnimation)
  EVT_MENU(ID_CLEAN_PARAMETER,
           ViewerEvtHandler::cleanParameters)
  EVT_MENU(ID_SHOW_RING,
           ViewerEvtHandler::showRing)
  EVT_MENU(ID_HIDE_SELECTED,
           ViewerEvtHandler::hideSelected)
  EVT_MENU(ID_HIDE_UNSELECTED,
           ViewerEvtHandler::hideUnselected)
  EVT_MENU(ID_SHOW_HIDDEN,
           ViewerEvtHandler::showHidden)
  EVT_MENU(wxID_ZOOM_FIT,
           ViewerEvtHandler::zoomFit)
  EVT_MENU(ID_ZOOM_TO,
           ViewerEvtHandler::zoomTo)
  EVT_MENU(wxID_ZOOM_IN,
           ViewerEvtHandler::zoomIn)
  EVT_MENU(wxID_ZOOM_OUT,
           ViewerEvtHandler::zoomOut)
  EVT_MENU(ID_FOREGROUND_COLOR,
           ViewerEvtHandler::foregroundColor)
  EVT_MENU(ID_BACKGROUND_COLOR,
           ViewerEvtHandler::backgroundColor)
  EVT_MENU(ID_CAMERA_TYPE,
           ViewerEvtHandler::toggleCamera)
  EVT_MENU(ID_CLEAN_COORD,
           ViewerEvtHandler::cleanCoord)
  EVT_MENU(ID_SELECT_DRAG,
           ViewerEvtHandler::selectModeAction)
  EVT_MENU(ID_SELECT_LASSO,
           ViewerEvtHandler::selectModeAction)
  EVT_MENU(ID_AUTO_NORMALIZE,
           ViewerEvtHandler::toggleAutoCenter)
  EVT_MENU(ID_MEASURE_DIST,
           ViewerEvtHandler::measureDist)
  EVT_MENU(ID_MEASURE_ANGLE,
           ViewerEvtHandler::measureAngle)
  EVT_MENU(ID_MEASURE_TORSION,
           ViewerEvtHandler::measureTorsion)
  EVT_MENU(ID_MEASURE_CLEAR,
           ViewerEvtHandler::measureClear)
  EVT_MENU(ID_MANIPULATOR_SPHERE,
           ViewerEvtHandler::addSphereManip)
  EVT_MENU(ID_MANIPULATOR_NOSPHERE,
           ViewerEvtHandler::delSphereManip)
  EVT_MENU(ID_MANIPULATOR_WHEEL,
           ViewerEvtHandler::addWheelManip)
  EVT_MENU(ID_MANIPULATOR_NOWHEEL,
           ViewerEvtHandler::delWheelManip)

  EVT_MENU(ID_RESET_PERSPECTIVE,
           ViewerEvtHandler::resetPerspective)
  EVT_MENU(ID_GENERATE_BOND,
           ViewerEvtHandler::generateBond)
  EVT_MENU(ID_ADD_H,
           ViewerEvtHandler::addHydrogen)
  EVT_MENU(ID_DEL_H,
           ViewerEvtHandler::delHydrogen)
  EVT_MENU(ID_SHOW_HYDROGEN,
           ViewerEvtHandler::showHydrogen)
  EVT_MENU(ID_SHOW_HYDROGEN_BOND,
           ViewerEvtHandler::showHydrogenBond)
  EVT_MENU(ID_ORIENT,
           ViewerEvtHandler::orient)
  EVT_MENU(ID_CLEAR_RESIDUE,
           ViewerEvtHandler::clearResidue)
  EVT_MENU(ID_MAKE_RESIDUE,
           ViewerEvtHandler::makeResidue)

  EVT_MENU(ID_RESET_STYLE,
           ViewerEvtHandler::resetStyle)

  EVT_BUTTON(ID_ZOOM_TO,
             ViewerEvtHandler::zoomTo)
  EVT_BUTTON(wxID_ZOOM_IN,
             ViewerEvtHandler::zoomIn)
  EVT_BUTTON(wxID_ZOOM_OUT,
             ViewerEvtHandler::zoomOut)
  EVT_BUTTON(ID_CAMERA_TYPE,
             ViewerEvtHandler::toggleCamera)
  EVT_BUTTON(wxID_ZOOM_FIT,
             ViewerEvtHandler::zoomFit)
  EVT_BUTTON(ID_AUTO_NORMALIZE,
             ViewerEvtHandler::toggleAutoCenter)

END_EVENT_TABLE()


map<wxWindowID,wxString> ViewerEvtHandler::p_styleNames;


ViewerEvtHandler::ViewerEvtHandler(wxWindow * parent)
  : wxEvtHandler()
  , WxVizTool()
  , Listener("ViewerEvtHandler")
  , p_parent(parent)
  , p_resetPerspective(false)
  , p_styleChooser(NULL)
  , p_lastStyle("")
  , p_cameraType(0)
  , p_autoCenter(0)
{
  subscribe();

  if (p_styleNames.empty()) {
    p_styleNames[ID_STYLE_BALL_WIREFRAME] = "Ball And Wireframe\tALT+1";
    p_styleNames[ID_STYLE_BALL_STICK] = "Ball And Stick\tALT+2";
    p_styleNames[ID_STYLE_STICK] = "Stick\tALT+3";
    p_styleNames[ID_STYLE_WIREFRAME] = "Wireframe\tALT+4";
    p_styleNames[ID_STYLE_CPK] = "CPK\tALT+5";
    p_styleNames[ID_STYLE_RESIDUE_WIRE] = "Residue Wire\tALT+6";
    p_styleNames[ID_STYLE_RESIDUE_CYLINDERS] = "Residue Cylinders\tALT+7";
    p_styleNames[ID_STYLE_LINE_RIBBON] = "Line Ribbon";
    p_styleNames[ID_STYLE_FLAT_RIBBON] = "Flat Ribbon";
    p_styleNames[ID_STYLE_SOLID_RIBBON] = "Solid Ribbon\tALT+8";
    p_styleNames[ID_STYLE_CARTOON] = "Cartoon\tALT+9";
    p_styleNames[ID_STYLE_CUSTOM] = "Custom Style...\tALT+0";
  }
}


ViewerEvtHandler::~ViewerEvtHandler()
{
  unsubscribe();
  p_parent = 0;
}


void ViewerEvtHandler::centerCoord(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();

  Command *cmd = new CenterCmd("Center", &fw.getSceneGraph());
  wxWindowID id = event.GetId();
  if (id == ID_CENTER_XYZ) {
    cmd->getParameter("axis")->setString("xyz");
  } else if (id == ID_CENTER_X) {
    cmd->getParameter("axis")->setString("x");
  } else if (id == ID_CENTER_Y) {
    cmd->getParameter("axis")->setString("y");
  } else if (id == ID_CENTER_Z) {
    cmd->getParameter("axis")->setString("z");
  }
  fw.execute(cmd);
}


void ViewerEvtHandler::styleChange(wxCommandEvent & event)
{
  doStyleChange(wxStripMenuCodes(p_styleNames[event.GetId()]));
}


void ViewerEvtHandler::doStyleChange(const wxString& style)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  if (p_styleNames[ID_STYLE_CUSTOM].StartsWith(style)) {
     if (p_styleChooser == 0) {
        p_styleChooser = new VizStyleChooser(p_parent);
        ((WxVizTool*)p_styleChooser)->connectToolKitFW(&(fw));
        p_styleChooser->initializeGUI();
     }
    p_styleChooser->Show();
    p_styleChooser->Raise();
  } else {
    p_lastStyle = style;

    // Restore from styles preferences
    string scheme = "Element";
    ewxConfig *config = ewxConfig::getConfig("vizstyles.ini");
    config->Write("/LastStyle", style);
    string styledd = config->Read(style).c_str();
    DisplayDescriptor *dd = 0;
    if (styledd != "")
       dd = new DisplayDescriptor(styledd);
    else
       dd = new DisplayDescriptor("default", style.c_str(), scheme);
    if (!dd->isValid(dd->getStyle(), dd->getColorScheme(),
             sg.getFragment()->numResidues() > 0)) {
       dd->setColorScheme(dd->getDefaultColorScheme(dd->getStyle()));
    }

    Command *cmd = new CSStyleCmd("Style", (Receiver*)&sg);
    cmd->getParameter("descriptor")->setString(dd->toString());
    cmd->getParameter("all")->setBoolean(true);
    fw.execute(cmd);
    
    sg.fixAllNubs();

    delete dd;
    
  }
}


void ViewerEvtHandler::showAtomLabel(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  Command *cmd = new AtomLabelsCmd("AtomLabel", &fw.getSceneGraph());
  cmd->getParameter("type")->setInteger(event.GetId()-ID_LABEL_NONE);
  fw.execute(cmd);
}


void ViewerEvtHandler::showBondLabel(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  sg.setBondLabels(event.IsChecked());
}


void ViewerEvtHandler::selectModeAction(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGViewer& viewer = fw.getViewer();
  bool dragFlag = event.GetId()==ID_SELECT_DRAG;
  viewer.setSelectModeDrag(dragFlag);
}


void ViewerEvtHandler::showResidueLabel(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  sg.setResidueLabels(event.IsChecked());
}


void ViewerEvtHandler::showCornerLabels(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  fw.getViewer().showTextLabels(event.IsChecked());
  //Command *cmd = new CornerLabelsCmd("Corner Text Labels", &fw.getViewer());
  //cmd->getParameter("switch")->setBoolean(event.IsChecked());
  //fw.execute(cmd);
}

void ViewerEvtHandler::depthCue(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  Command *cmd = new DepthCueCmd("Depth Cue", &sg);
  cmd->getParameter("hazeOn")->setBoolean(event.IsChecked());
  fw.execute(cmd);

  syncToggle(event);
}


void ViewerEvtHandler::showAxes(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
 
  Command *cmd = new AxisCmd("Axis", &fw.getViewer());
  cmd->getParameter("switch")->setBoolean(event.IsChecked());
  fw.execute(cmd);

  syncToggle(event);
}


void ViewerEvtHandler::enableAnimation(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();

  fw.getViewer().setAnimationEnabled(event.IsChecked());

  syncToggle(event);
}


void ViewerEvtHandler::cleanParameters(wxCommandEvent & event)
{
  CleanParameters *dialog = new CleanParameters(p_parent);
  dialog->ShowModal();
}


void ViewerEvtHandler::showRing(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  Command *cmd = new RingsCmd("Rings", &sg);
  cmd->getParameter("switch")->setBoolean(event.IsChecked());
  fw.execute(cmd);

  syncToggle(event);
}


void ViewerEvtHandler::hideSelected(wxCommandEvent& event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  Command *cmd = new DisplayAtomsCmd("DisplayAtoms", &sg);
  fw.execute(cmd);
  wxLogWarning("Hiding selected atoms/bonds.  Use Show Hidden "
               "Atoms/Bonds to redisplay.");
}


void ViewerEvtHandler::hideUnselected(wxCommandEvent& event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  Command *cmd = new DisplayAtomsCmd("DisplayAtoms", &sg);
  cmd->getParameter("selected")->setBoolean(false);
  fw.execute(cmd);
  wxLogWarning("Hiding all unselected atoms/bonds.  Use Show Hidden "
               "Atoms/Bonds to redisplay.");
}


void ViewerEvtHandler::showHidden(wxCommandEvent& event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  Command *cmd = new DisplayAtomsCmd("DisplayAtoms", &sg);
  cmd->getParameter("all")->setBoolean(true);
  cmd->getParameter("show")->setBoolean(true);
  fw.execute(cmd);
}


void ViewerEvtHandler::resetStyle(wxCommandEvent & event)
{
  doStyleChange(p_lastStyle);
}


void ViewerEvtHandler::zoomFit( wxCommandEvent& event )
{
  WxVizToolFW& fw = getFW();
  SGViewer& viewer = fw.getViewer();
  Command *cmd = new CenterViewCmd("Center", &viewer);
  fw.execute(cmd);
  event.Skip();
}


void ViewerEvtHandler::zoomTo(wxCommandEvent& event)
{
  WxVizToolFW& fw = getFW();
  SGViewer& viewer = fw.getViewer();
  SGContainer& sg = fw.getSceneGraph();

  SoCamera *camera = viewer.getCamera();

  SGFragment *frag = sg.getFragment();
  float xmin, ymin, zmin, xmax, ymax, zmax;
  frag->getBoundBox(xmin,ymin,zmin,xmax,ymax,zmax);

  sg.getViewBBox()->enable();
  sg.getViewBBox()->thebox.setBounds(xmin,ymin,zmin,xmax,ymax,zmax);
  camera->viewAll((SoNode*)sg.getViewBBox(),SbViewportRegion(viewer.getGlxSize()));

  SbVec3f center((xmin+xmax)/2.,(ymin+ymax)/2.,(zmin+zmax)/2.);
  camera->pointAt(center);
  sg.getViewBBox()->disable();

  event.Skip();
}

void ViewerEvtHandler::zoomIn(wxCommandEvent& event)
{
  getFW().getViewer().zoomCamera(-1.0);
  event.Skip();
}

void ViewerEvtHandler::zoomOut(wxCommandEvent& event)
{
  getFW().getViewer().zoomCamera(1.0);
  event.Skip();
}


void ViewerEvtHandler::foregroundColor(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  Command *cmd = new ForegroundCmd("Foreground", &sg);

  float red = cmd->getParameter("red")->getDouble();
  float green = cmd->getParameter("green")->getDouble();
  float blue = cmd->getParameter("blue")->getDouble();
  
  ewxColorDialog colorDialog(ewxColor(red, green, blue), p_parent);
  
  if (colorDialog.ShowModal() == wxID_OK) {
    wxColor bgcolor = colorDialog.GetColor();
    cmd->getParameter("red")->setDouble(bgcolor.Red()/255.0);
    cmd->getParameter("green")->setDouble(bgcolor.Green()/255.0);
    cmd->getParameter("blue")->setDouble(bgcolor.Blue()/255.0);
    fw.execute(cmd);
  }
  else
    delete cmd;
}


void ViewerEvtHandler::backgroundColor(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGViewer& viewer = fw.getViewer();

  SbColor color = viewer.getBackgroundColor();
  float red = color[0];
  float green = color[1];
  float blue = color[2];
  
  ewxColorDialog colorDialog(ewxColor(red, green, blue), p_parent);
  
  if (colorDialog.ShowModal() == wxID_OK) {
    wxColor bgcol= colorDialog.GetColor();
    setBackground(bgcol.Red()/255.0, bgcol.Green()/255.0, bgcol.Blue()/255.0);
  }
}


void ViewerEvtHandler::toggleCamera(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGViewer& viewer = fw.getViewer();

  viewer.toggleCameraType();

  p_cameraType = (p_cameraType+1)%2;
  event.SetInt(p_cameraType);
  syncToggle(event);
}


void ViewerEvtHandler::cleanCoord(wxCommandEvent & event)
{
   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   SGFragment *frag = sg.getFragment();

   string msg = "To restrict the clean,\nselect atoms before invoking clean";
   // The above message makes the dialog soooo wide so forget it
   msg = "";
   ewxProgressDialog *dlg = 0;
   
   // Try to avoid flashing a dialog
   if (frag->numAtoms() > 10) {
      dlg = new ewxProgressDialog ("Ecce Force Field Clean",msg.c_str(),1000,0, wxPD_AUTO_HIDE|wxPD_CAN_ABORT|wxPD_ELAPSED_TIME|wxPD_SMOOTH);
      dlg->Show();
   }

   Command *cmd = new CleanCmd("Force Field Clean", &sg, dlg);
   CleanParameters *params = new CleanParameters(p_parent);
   cmd->getParameter("iterations")->setInteger(params->getMaxIterations());
   cmd->getParameter("nonBondCutoff")->setDouble(params->getForceCutoff());
   cmd->getParameter("convergence")->setDouble(params->getConvergence());

   // This could result in a CancelException
   fw.execute(cmd);

   if (dlg) dlg->Destroy();
}


void ViewerEvtHandler::toggleAutoCenter(wxCommandEvent & event)
{
  p_autoCenter = (p_autoCenter+1)%2;
  event.SetInt(p_autoCenter);

  if (event.IsChecked()) {
    WxVizToolFW& fw = getFW();
    SGViewer& viewer = fw.getViewer();

    Command *cmd = new CenterViewCmd("Center", &viewer);
    fw.execute(cmd);
  }
  syncToggle(event);
}



void ViewerEvtHandler::measureDist(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  Command *cmd = new MeasureDistCmd("Measure Distance", (Receiver*)&sg);
  fw.execute(cmd); 
}


void ViewerEvtHandler::measureAngle(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  Command *cmd = new MeasureAngleCmd("Measure Angle", (Receiver*)&sg);
  fw.execute(cmd); 
}


void ViewerEvtHandler::measureTorsion(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  Command *cmd = new MeasureTorsionCmd("Measure Torsion", (Receiver*)&sg);
  fw.execute(cmd); 
}


void ViewerEvtHandler::measureClear(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  Command *cmd = new ClearMeasuresCmd("Clear Measures", (Receiver*)&sg);
  fw.execute(cmd); 
}


void ViewerEvtHandler::addSphereManip(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  Command *cmd = new AtomMoveCmd("Add Dragger", &sg);
  fw.execute(cmd);
}


void ViewerEvtHandler::delSphereManip(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  Command *cmd = new ClearAtomMoveCmd("Remove Dragger", &sg);
  fw.execute(cmd);
}


void ViewerEvtHandler::addWheelManip(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  Command *cmd = new BondRotateCmd("Add Wheel", &sg);
  fw.execute(cmd);
}


void ViewerEvtHandler::delWheelManip(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  Command *cmd = new ClearBondRotateCmd("Remove Wheel", &sg);
  fw.execute(cmd);
}


void ViewerEvtHandler::addHydrogen(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  Command *cmd = new HydrogensCmd("Add Hydrogens", &sg);
  fw.execute(cmd);
}


void ViewerEvtHandler::delHydrogen(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  Command *cmd = new RmHydrogensCmd("Remove Hydrogens", &sg);
  fw.execute(cmd);
}


void ViewerEvtHandler::showHydrogen(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  Command *cmd = new ShowHydrogensCmd("Show Hydrogen", &fw.getSceneGraph());
  cmd->getParameter("switch")->setBoolean(event.IsChecked());
  fw.execute(cmd);
}


void ViewerEvtHandler::showHydrogenBond(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  Command* cmd=new HydrogenBondsCmd("Show Hydrogen Bond",&fw.getSceneGraph());
  cmd->getParameter("switch")->setBoolean(event.IsChecked());
  fw.execute(cmd);
}


void ViewerEvtHandler::orient(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  Command* cmd=new OrientCmd("Orient",&fw.getSceneGraph());
  fw.execute(cmd);
}


void ViewerEvtHandler::clearResidue(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  Command * cmd = new ClearResidueInfoCmd("Clear Residue Info",
                                          &fw.getSceneGraph());
  fw.execute(cmd);
}

void ViewerEvtHandler::makeResidue(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  Command * cmd = new MakeResidueCmd("Make Residue ",
                                          &fw.getSceneGraph());
  fw.execute(cmd);
}

void ViewerEvtHandler::resetPerspective(wxCommandEvent & event)
{
  string app = getenv("ECCE_INVOKE_VIEWER")? "Viewer": "Builder";

  ewxMessageDialog dlg(p_parent, "The tools layout will be reset to default "
                       "after the ECCE " + app + " restarts.\nDo you want to "
                       "reset the tools layout?",
                       "Reset Layout",
                       wxICON_QUESTION|wxYES_NO|wxYES_DEFAULT);
  p_resetPerspective = (dlg.ShowModal() == wxID_YES);
}


void ViewerEvtHandler::generateBond(wxCommandEvent & event)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  Command *cmd = new GenBondsCmd("Generate Bonds", &sg);
  fw.execute(cmd); 
}


void ViewerEvtHandler::syncToggle(wxCommandEvent & event)
{
  EventDispatcher::getDispatcher().publish(Event("SyncToggle", "", "",
                                                 (void *)&event));
}


void ViewerEvtHandler::createMeasureMenu(wxMenu * menu)
{
  menu->Append(ID_MEASURE_DIST, _("Distance"), _T(""), wxITEM_NORMAL);
  menu->Append(ID_MEASURE_ANGLE, _("Angle"), _T(""), wxITEM_NORMAL);
  menu->Append(ID_MEASURE_TORSION, _("Torsion"), _T(""), wxITEM_NORMAL);
  menu->AppendSeparator();
  menu->Append(ID_MEASURE_CLEAR, _("Clear"), _T(""), wxITEM_NORMAL);
}


void ViewerEvtHandler::createBuildMenu(wxMenu * menu)
{
  menu->Append(ID_CLEAN_COORD, _("Clean\tCtrl+k"), _T(""), wxITEM_NORMAL);
  menu->AppendSeparator();
  menu->Append(ID_ADD_H, _("Add Hydrogen\tCtrl+f"), _T(""), wxITEM_NORMAL);
  menu->Append(ID_DEL_H, _("Remove Hydrogen\tCtrl+g"), _T(""), wxITEM_NORMAL);
  menu->Append(ID_GENERATE_BOND, _("Generate Bonds\tCtrl+b"), _T(""), wxITEM_NORMAL);
  menu->AppendSeparator();
  menu->Append(ID_CLEAR_RESIDUE, _("Clear Residue Info"));
  menu->Append(ID_MAKE_RESIDUE, _("Make into Residue-Based System"));

  menu->AppendSeparator();
  menu->Append(ID_CENTER_XYZ, _("Center XYZ"), _T(""), wxITEM_NORMAL);
  menu->Append(ID_CENTER_X, _("Center X"), _T(""), wxITEM_NORMAL);
  menu->Append(ID_CENTER_Y, _("Center Y"), _T(""), wxITEM_NORMAL);
  menu->Append(ID_CENTER_Z, _("Center Z"), _T(""), wxITEM_NORMAL);
  menu->Append(ID_ORIENT, _("Orient"), _T("Orient moment of inertia tensor with axis"), wxITEM_NORMAL);
}


void ViewerEvtHandler::createRenderMenu(wxMenu * menu)
{
  wxMenu* styleMenu = new wxMenu;
  styleMenu->Append(ID_STYLE_BALL_WIREFRAME,
          p_styleNames[ID_STYLE_BALL_WIREFRAME]);
  styleMenu->Append(ID_STYLE_BALL_STICK,
          p_styleNames[ID_STYLE_BALL_STICK]);
  styleMenu->Append(ID_STYLE_STICK,
          p_styleNames[ID_STYLE_STICK]);
  styleMenu->Append(ID_STYLE_WIREFRAME,
          p_styleNames[ID_STYLE_WIREFRAME]);
  styleMenu->Append(ID_STYLE_CPK,
          p_styleNames[ID_STYLE_CPK]);
  styleMenu->Append(ID_STYLE_RESIDUE_WIRE,
          p_styleNames[ID_STYLE_RESIDUE_WIRE]);
  styleMenu->Append(ID_STYLE_RESIDUE_CYLINDERS,
          p_styleNames[ID_STYLE_RESIDUE_CYLINDERS]);
  styleMenu->Append(ID_STYLE_LINE_RIBBON,
          p_styleNames[ID_STYLE_LINE_RIBBON]);
  styleMenu->Append(ID_STYLE_FLAT_RIBBON,
          p_styleNames[ID_STYLE_FLAT_RIBBON]);
  styleMenu->Append(ID_STYLE_SOLID_RIBBON,
          p_styleNames[ID_STYLE_SOLID_RIBBON]);
  styleMenu->Append(ID_STYLE_CARTOON,
          p_styleNames[ID_STYLE_CARTOON]);
  styleMenu->Append(ID_STYLE_CUSTOM,
          p_styleNames[ID_STYLE_CUSTOM]);
  styleMenu->Append(ID_STYLE_BALL_WIREFRAME,
          p_styleNames[ID_STYLE_BALL_WIREFRAME]);

  menu->Append(ID_STYLE, _("Style"), styleMenu);

  //menu->Append(ID_STEREO, _("Stereo"), _T(""),
  //         wxITEM_CHECK);
  menu->Append(ID_DEPTH_CUEING, _("Enable Depth Cueing"), _T(""),
               wxITEM_CHECK);
  menu->Append(ID_CAMERA_TYPE, _("Enable Orthographic"), _T(""),
               wxITEM_CHECK);
  //menu->Append(ID_TRANSPARENCY, _("Enable Transparency"), _T(""),
  //             wxITEM_CHECK);
  //menu->Append(ID_ANTIALIASING, _("Use Antialiasing"), _T(""),
  //         wxITEM_CHECK);
  menu->AppendSeparator();

  menu->Append(ID_HIDE_SELECTED, _("Hide Selected Atoms/Bonds\tCtrl+j"), _T(""),
               wxITEM_NORMAL);
  menu->Append(ID_HIDE_UNSELECTED, _("Hide Unselected Atoms/Bonds\tCtrl+i"), _T(""),
               wxITEM_NORMAL);
  menu->Append(ID_SHOW_HIDDEN, _("Show Hidden Atoms/Bonds\tCtrl+l"), _T(""),
               wxITEM_NORMAL);
  menu->AppendSeparator();

  menu->Append(wxID_ZOOM_FIT, _("View All\tCTRL+="), _T(""), wxITEM_NORMAL);
  menu->Append(ID_ZOOM_TO, _("Zoom To Selection\tCTRL+*"), _T("Zoom to selected atoms"), wxITEM_NORMAL);
  menu->Append(wxID_ZOOM_IN, _("Zoom In\tCTRL++"), _T(""), wxITEM_NORMAL);
  menu->Append(wxID_ZOOM_OUT, _("Zoom Out\tCTRL+-"), _T(""), wxITEM_NORMAL);
  menu->AppendSeparator();

  menu->Append(ID_BACKGROUND_COLOR, _("Background Color..."), _T(""),
               wxITEM_NORMAL);
  menu->Append(ID_FOREGROUND_COLOR, _("Foreground Color..."), _T("Set color used for labels, axis, measures, and text."),
               wxITEM_NORMAL);
}


void ViewerEvtHandler::createOptionMenu(wxMenu * menu)
{
  menu->Append(ID_SHOW_HYDROGEN, _("Show Hydrogen Atoms"), _T(""), wxITEM_CHECK);
  menu->Check(ID_SHOW_HYDROGEN, true);
  menu->Append(ID_SHOW_HYDROGEN_BOND, _("Show Hydrogen Bonds"), _T(""), wxITEM_CHECK);
  wxMenu* labelMenu = new wxMenu;
  labelMenu->Append(ID_LABEL_NONE, _("None"), _T(""), wxITEM_NORMAL);
  labelMenu->Append(ID_LABEL_ELEMENT, _("Element with Number"), _T(""), wxITEM_NORMAL);
  labelMenu->Append(ID_LABEL_ATOM_NAME, _("Atom Name"), _T(""), wxITEM_NORMAL);
  labelMenu->Append(ID_LABEL_CHARGE2, _("Charge--2 decimal digits"), _T(""), wxITEM_NORMAL);
  labelMenu->Append(ID_LABEL_CHARGE3, _("Charge--3 decimal digits"), _T(""), wxITEM_NORMAL);
  labelMenu->Append(ID_LABEL_CHARGE4, _("Charge--4 decimal digits"), _T(""), wxITEM_NORMAL);
  labelMenu->Append(ID_LABEL_TYPE, _("Type"), _T(""), wxITEM_NORMAL);
  menu->Append(ID_ATOM_LABEL, _("Atom Labels"), labelMenu);
  menu->Append(ID_BOND_LABEL, _("Show Bond Labels"), _T(""), wxITEM_CHECK);
  menu->Append(ID_RESIDUE_LABEL, _("Show Residue Labels"), _T(""), wxITEM_CHECK);
  menu->Append(ID_CORNER_LABELS, _("Show Summary Text Labels"), _T(""), wxITEM_CHECK);
  menu->Check(ID_CORNER_LABELS, true);
  menu->Append(ID_SHOW_AXES, _("Show Axes"), _T(""), wxITEM_CHECK);
  menu->Append(ID_SHOW_RING, _("Show Aromatic Rings"), _T(""), wxITEM_CHECK);
  menu->Check(ID_SHOW_RING, true);
  menu->AppendSeparator();
  wxMenu* selectMenu = new wxMenu;
  selectMenu->Append(ID_SELECT_DRAG, _("Atom Drag"), _T(""), wxITEM_RADIO);
  selectMenu->Append(ID_SELECT_LASSO, _("Lasso Select"), _T(""), wxITEM_RADIO);
  menu->Append(ID_SELECT_ACTION, _("Select Mode Action"), selectMenu);
  menu->Append(ID_AUTO_NORMALIZE, _("Auto Normalize Display"), _T(""), wxITEM_CHECK);
  menu->Append(ID_ENABLE_SPINNING, _("Enable Spinning"), _T(""), wxITEM_CHECK);
  menu->Append(ID_AUTO_RESIDUE, _("Auto Open Residue Table"), _T(""), wxITEM_CHECK);
  menu->AppendSeparator();
  menu->Append(ID_CLEAN_PARAMETER, _("Clean Parameters..."), _T(""), wxITEM_NORMAL);
  menu->Append(ID_RESET_PERSPECTIVE, _("Reset Default Tools/Toolbars"), _T(""), wxITEM_NORMAL);
}


void ViewerEvtHandler::eventMCB(const Event& event)
{
  string name = event.getName();
  string value = event.getValue();

  if (name == "ResidueChange" || name == "GeomChange") {
    WxVizToolFW& fw = getFW();
    SGContainer& sg = fw.getSceneGraph();
    SGFragment *frag = sg.getFragment();

    bool hasBioResidues = frag->hasBiologicalResidues();
    //NOTE assuming we can cast parent to frame to get the menu bar
    wxMenuBar *menubar = ((wxFrame*)p_parent)->GetMenuBar();
    if (menubar) {
      menubar->Enable(ID_STYLE_RESIDUE_WIRE,hasBioResidues);
      menubar->Enable(ID_STYLE_RESIDUE_CYLINDERS,hasBioResidues);
      menubar->Enable(ID_STYLE_LINE_RIBBON,hasBioResidues);
      menubar->Enable(ID_STYLE_SOLID_RIBBON,hasBioResidues);
      menubar->Enable(ID_STYLE_FLAT_RIBBON,hasBioResidues);
      menubar->Enable(ID_STYLE_CARTOON,hasBioResidues);
      menubar->Enable(ID_STYLE_CARTOON,hasBioResidues);
    }
  }
}


void ViewerEvtHandler::saveSettings(wxConfig * config)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  SGViewer& viewer = fw.getViewer();

  Command *cmd = new ForegroundCmd("Foreground", &sg);
  float red = cmd->getParameter("red")->getDouble();
  float green = cmd->getParameter("green")->getDouble();
  float blue = cmd->getParameter("blue")->getDouble();
  delete cmd;
  ewxColor fcol(red, green, blue);
  config->Write("Foreground", fcol.GetAsString(wxC2S_HTML_SYNTAX));

  SbColor color = viewer.getBackgroundColor();
  ewxColor bcol(color[0], color[1], color[2]);
  config->Write("Background", bcol.GetAsString(wxC2S_HTML_SYNTAX));
  config->Write("DefaultStyle", p_lastStyle);
  config->Write("ResetPerspective", p_resetPerspective);
}


void ViewerEvtHandler::restoreSettings(wxConfig * config)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();

  wxString buffer;

  // Set a background color and compute a default fg color
  config->Read("Background", &buffer, "#000000");
  ewxColor bcol(buffer);
  setBackground(bcol.Red()/255.0, bcol.Green()/255.0, bcol.Blue()/255.0);

  wxString style = config->Read("DefaultStyle", "Ball And Wireframe");
  doStyleChange(style);

  // Now override foreground only if set
  if (config->Read("Foreground", &buffer, "#FFFFFF")) {
     ewxColor fcol(buffer);
     Command *cmd = new ForegroundCmd("Foreground", &sg);
     cmd->getParameter("red")->setDouble(fcol.Red()/255.0);
     cmd->getParameter("green")->setDouble(fcol.Green()/255.0);
     cmd->getParameter("blue")->setDouble(fcol.Blue()/255.0);
     fw.execute(cmd);
  }

}


/**
 * Set background and then auto-choose a foreground which
 * the user can then override.
 */
void ViewerEvtHandler::setBackground(float r, float g, float b)
{
  WxVizToolFW& fw = getFW();
  SGContainer& sg = fw.getSceneGraph();
  SGViewer& viewer = fw.getViewer();

  Command *cmd = new BackgroundCmd("Background", &viewer);
  cmd->getParameter("red")->setDouble(r);
  cmd->getParameter("green")->setDouble(g);
  cmd->getParameter("blue")->setDouble(b);
  fw.execute(cmd);

  // Automatically compute something that contrasts
  SbColor fg = viewer.calculateContrastingColor();
  cmd = new ForegroundCmd("Foreground", &sg);
  cmd->getParameter("red")->setDouble(fg[0]);
  cmd->getParameter("green")->setDouble(fg[1]);
  cmd->getParameter("blue")->setDouble(fg[2]);
  fw.execute(cmd);
}
