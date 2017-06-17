#include <fstream>
using std::ifstream;
using std::ofstream;
#include <iostream>
using std::cout;
using std::endl;
#include <map>
using std::map;
#include <sstream>
using std::ostringstream;
using std::ends;
#include <vector>
using std::vector;
  
#include <math.h>

// Remove some of these three once the calls are in SoWx::init
#include "inv/nodes/SoPerspectiveCamera.H"
#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoDirectionalLight.H"
#include "inv/nodes/SoNode.H"
#include "inv/nodes/SoCone.H"
#include "inv/nodes/SoRotationXYZ.H"
#include "inv/actions/SoGLRenderAction.H"
#include "inv/manips/SoTrackballManip.H"

#include <wx/tglbtn.h>
#include <wx/toolbar.h>
#include <wx/utils.h>

#include "util/Ecce.H"
#include "util/BrowserHelp.H"
#include "util/CancelException.H"
#include "util/CommandWrapper.H"
#include "util/ImageConverter.H"
#include "util/Event.H"
  using ecce::Event;
#include "util/EventDispatcher.H"
  using ecce::EventDispatcher;
#include "util/Preferences.H"
#include "util/ResourceUtils.H"
#include "util/RetryException.H"
#include "util/STLUtil.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"
#include "util/TempStorage.H"

#include "tdat/FragUtil.H"
#include "tdat/ShapeData.H"
#include "tdat/TPerTab.H"
#include "tdat/TBond.H"

#include "dsm/Job.H"
#include "dsm/ChemistryTask.H"
#include "dsm/EDSIFactory.H"
#include "dsm/IPropCalculation.H"
#include "dsm/PropertyTask.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"
#include "dsm/Session.H"
#include "dsm/DirDyVTSTTask.H"
#include "dsm/EDSIServerCentral.H"

#include "comm/RunMgmt.H"

#include "wxgui/ContextHistory.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxCursor.H"
#include "wxgui/ewxGenericFileDialog.H"
#include "wxgui/ewxLogTextCtrl.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/FeedbackSaveHandler.H"
#include "wxgui/PerTabPanel.H"
#include "wxgui/SaveExperimentAsDialog.H"
#include "wxgui/TearableContent.H"
#include "wxgui/TearableContentProvider.H"
#include "wxgui/WindowEvent.H"
  using ecce::WindowEvent;
#include "wxgui/WxMeasurePrompt.H"
#include "wxgui/WxPDBPrompt.H"
#include "wxgui/WxPovrayOptionsDialog.H"
#include "wxgui/WxUnitsPrompt.H"
#include "wxgui/WxCalcImport.H"
#include "wxgui/WxCalcImportClient.H"

#include "inv/ChemKit/ChemInit.H"

#include "inv/SoWx/SoWx.H"

#include "viz/AtomMeasureAngle.H"
#include "viz/AtomMeasureDist.H"
#include "viz/AtomMeasureTorsion.H"
#include "viz/AtomNodesInit.H"
#include "viz/AtomRTDragger.H"
#include "viz/AtomRotDragger.H"
#include "viz/NodesInit.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/MoveAction.H"
#include "viz/SGLattice.H"

// Commands
#include "viz/AddFragmentCmd.H"
#include "viz/AngleEditCmd.H"
#include "viz/AtomEditCmd.H"
#include "viz/AtomLabelsCmd.H"
#include "viz/BondEditCmd.H"
#include "viz/CenterCmd.H"
#include "viz/ClearResidueInfoCmd.H"
#include "viz/CSLoadColorsCmd.H"
#include "viz/CSRadiiCmd.H"
#include "viz/CSStyleCmd.H"
#include "viz/DeleteCmd.H"
#include "viz/DepthCueCmd.H"
#include "viz/HydrogenBondsCmd.H"
#include "viz/HydrogensCmd.H"
#include "viz/InsertResidueCmd.H"
#include "viz/LengthEditCmd.H"
#include "viz/NewFragmentCmd.H"
#include "viz/RingsCmd.H"
#include "viz/RmHydrogensCmd.H"
#include "viz/SelectAllCmd.H"
#include "viz/SelectCmd.H"
#include "viz/TorsionEditCmd.H"
#include "viz/UnselectCmd.H"
#include "viz/FragAssignCmd.H"
#include "viz/ShowFragCmd.H"
#include "viz/PBCCmd.H"
#include "viz/TwoDMoveCmd.H"

#include "wxviz/AtomTable.H"
#include "wxviz/ResidueTable.H"
#include "wxviz/ResidueIndexPrompt.H"
#include "wxviz/SGContainerManager.H"
#include "wxviz/SGSelection.H"
#include "wxviz/SGViewer.H"
#include "wxviz/StyleDropDown.H"
#include "wxviz/VizRender.H"
#include "wxviz/WxVizTool.H"
#include "wxviz/WxVizToolFW.H"
#include "wxviz/MotionData.H"

// Viewer based commands
#include "wxviz/CenterViewCmd.H"
#include "wxviz/AxisCmd.H"
#include "wxviz/SetHomeCmd.H"
#include "wxviz/GoHomeCmd.H"
#include "wxviz/DumpSGCmd.H"
#include "wxviz/ViewerEvtHandler.H"
#include "wxviz/PovrayCmd.H"
#include "wxviz/RenderFileCmd.H"
#include "wxviz/SelectionPanel.H"

#include "BondDropDown.H"
#include "BuilderDockArt.H"
#include "ContextPanel.H"
#include "CoordPanel.H"
#include "DefaultCalculation.H"
#include "Dna.H"
#include "MiniPerTab.H"
#include "OpenCalculationDialog.H"
#include "ImportCalculationDialog.H"
#include "Peptide.H"
#include "PropertyPanel.H"
#include "PropertyPanelFactory.H"
#include "ShapeDropDown.H"
#include "StructLib.H"
#include "SymmetryPanel.H"
#include "TranslatePanel.H"
#include "VizPropertyPanel.H"
#include "PBC.H"
#include "Slice.H"

#include "Builder.H"


IMPLEMENT_CLASS( Builder, BuilderGUI )

BEGIN_EVENT_TABLE( Builder, BuilderGUI )
    EVT_MENU( ID_MODE_SELECT, Builder::OnModeClick )
    EVT_MENU( ID_MODE_ROTATE, Builder::OnModeClick )
    EVT_MENU( ID_MODE_TRANSLATE, Builder::OnModeClick )
    EVT_MENU( ID_MODE_ZOOM, Builder::OnModeClick )
    EVT_MENU( ID_MODE_ATOM, Builder::OnModeElementClick )
    EVT_MENU( ID_MODE_SHAPE, Builder::OnModeShapeClick )
    EVT_MENU( ID_MODE_BOND, Builder::OnModeBondClick )
    EVT_MENU( ID_MODE_STRUCTLIB, Builder::OnModeClick )
    EVT_BUTTON( ID_TOOL_GOHOME, Builder::OnToolGohomeClick )
    EVT_BUTTON( ID_TOOL_SETHOME, Builder::OnToolSethomeClick )
    EVT_MENU( ID_TOOL_STYLE, Builder::OnToolStyleClick )
    EVT_MENU( ID_TOOL_TRANSLATEM, Builder::OnToolTranslatemClick )
    EVT_MENU_RANGE( ID_TOOLMENU_ITEM, ID_TOOLMENU_ITEM+199,
                    Builder::OnToolMenuClick)
    EVT_MENU_RANGE( ID_CONTEXT_HISTORY_BASE, ID_CONTEXT_HISTORY_BASE+99,
                    Builder::OnContextRadioClick )
    EVT_LIST_ITEM_SELECTED( ContextPanel::ID_LIST, Builder::OnContextListClick )
    EVT_EWXAUI_PANE_CLOSE(Builder::OnPaneClose)
    EVT_EWXAUI_PANE_TAKE_FOCUS(Builder::OnPaneTakeFocus)
    EVT_EWXAUI_PANE_ADD_FOCUS(Builder::OnPaneAddFocus)
    EVT_EWXAUI_PANE_OPTIONS(Builder::OnPaneOptions)
    EVT_EWXAUI_PANE_OPEN(Builder::OnPaneOpen)
    EVT_EWXAUI_UPDATE(Builder::OnAuiUpdate)
    EVT_MENU( ID_SHOW_CMD, Builder::OnShowCmdClick )
    EVT_TEXT_ENTER(ID_TEXTCTRL_CMD, Builder::OnTextctrlCmdEnter)
    EVT_SPINCTRL(ID_ROT_X, Builder::OnRotX)
    EVT_SPINCTRL(ID_ROT_Y, Builder::OnRotY)
    EVT_SPINCTRL(ID_ROT_Z, Builder::OnRotZ)
    EVT_BUTTON(ID_CHOICE_VIEWER, Builder::OnViewerChoice)
    EVT_MENU_RANGE(ID_PROPERTY_MENU_BASE, ID_PROPERTY_MENU_BASE+99,
                   Builder::OnPropertyMenuClick )
END_EVENT_TABLE()


static bool internalSelect = false;


const string Builder::NAME_TOOL_CONTEXT(_("Context"));
const string Builder::NAME_TOOL_BUILD(_("Build"));
const string Builder::NAME_TOOL_COORDINATES(_("Coordinates"));
const string Builder::NAME_TOOL_SELECTION(_("Selection"));
const string Builder::NAME_TOOL_ATOM_TABLE(_("Atom Table"));
const string Builder::NAME_TOOL_RESIDUE_TABLE(_("Residue Table"));
const string Builder::NAME_TOOL_SYMMETRY(_("Symmetry"));
const string Builder::NAME_TOOL_DNA_BUILDER(_("DNA Builder"));
const string Builder::NAME_TOOL_PEPTIDE_BUILDER(_("Peptide Builder"));
const string Builder::NAME_TOOL_LOG(_("Log"));
const string Builder::NAME_TOOL_COMMAND_LINE(_("Command Line"));
const string Builder::NAME_TOOL_STRUCTLIB(_("Structure Library"));
const string Builder::NAME_TOOL_PBC(_("Periodic Builder"));
const string Builder::NAME_TOOL_SLICER(_("Slicer"));

const string Builder::NAME_TOOLBAR_FILE(_("File Toolbar"));
const string Builder::NAME_TOOLBAR_MODE(_("Mode Toolbar"));
const string Builder::NAME_TOOLBAR_VIEW(_("View Toolbar"));
const string Builder::NAME_TOOLBAR_STYLE(_("Style Toolbar"));
const string Builder::NAME_TOOLBAR_MANIPULATOR(_("Manipulator Toolbar"));
const string Builder::NAME_TOOLBAR_MEASURE(_("Measure Toolbar"));

const string Builder::NAME_MODE_SELECT(_("Select"));
const string Builder::NAME_MODE_ROTATE(_("Rotate"));
const string Builder::NAME_MODE_TRANSLATE(_("Translate"));
const string Builder::NAME_MODE_ZOOM(_("Zoom"));
const string Builder::NAME_MODE_ATOM(_("Atom"));
const string Builder::NAME_MODE_BOND(_("Bond"));
const string Builder::NAME_MODE_STRUCTLIB(_("Add Structure"));
const string Builder::NAME_MODE_SHAPE(_("Shape"));
const string Builder::s_modeText[] = {
  NAME_MODE_SELECT,
  NAME_MODE_ROTATE,
  NAME_MODE_TRANSLATE,
  NAME_MODE_ZOOM,
  NAME_MODE_ATOM,
  NAME_MODE_BOND,
  NAME_MODE_STRUCTLIB,
  NAME_MODE_SHAPE
};

const string Builder::NAME_LAYOUT_PREFIX("/PaneLayout/");
const string Builder::NAME_LAYOUT_DEFAULT("Default");
const string Builder::NAME_LAYOUT_READONLY("ReadOnly");
const string Builder::NAME_LAYOUT_STRUCTLIB("StructLib");




Builder::Builder()
  : BuilderGUI(),
    WxDavAuth(),
    JMSPublisher(BUILDER),
    WxVizToolFW(),
    Listener("Builder"),
    WxVizTool(),
    FeedbackSaveHandler(),
    p_sgMgr(NULL),
    p_viewer(NULL),
    p_currentMode(ID_MODE_SELECT),
    p_currentElement("C"),
    p_currentShapes(),
    p_currentBond(1.0),
    p_xrot(NULL),
    p_yrot(NULL),
    p_zrot(NULL),
    p_viewerButton(NULL),
    p_rotx(0),
    p_roty(0),
    p_rotz(0),
    p_copyBuffer(NULL),
    p_fileToolbar(NULL),
    p_modeToolbar(NULL),
    p_viewToolbar(NULL),
    p_styleToolbar(NULL),
    p_manipulatorToolbar(NULL),
    p_measureToolbar(NULL),
    p_toolbars(),
    p_fileMenu(NULL),
    p_editMenu(NULL),
    p_optionsMenu(NULL),
    p_renderMenu(NULL),
    p_buildMenu(NULL),
    p_modeMenu(NULL),
    p_measureMenu(NULL),
    p_toolMenu(NULL),
    p_toolbarMenu(NULL),
    p_propertyMenu(NULL),
    p_contextMenu(NULL),
    p_contextHistory(NULL),
    p_contextPanel(NULL),
    p_menus(),
    p_mainSizer(NULL),
    p_rotSizer(NULL),
    p_cmd(NULL),
    p_viewerEvtHandler(NULL),
    p_mgr(),
    p_toolCount(0),
    p_toolbarCount(0),
    p_pertab(NULL),
    p_structLib(NULL),
    p_standalone(false),
    p_readOnlyDisabledIds(),
    p_calculations(),
    p_calculation(NULL),
    p_commandManagers(),
    p_commandManager(NULL),
    p_dirty(),
    p_import(),
    p_lockBitmap("center_view_lock16.png", wxBITMAP_TYPE_PNG),
    p_unlockBitmap("center_view_unlock16.png", wxBITMAP_TYPE_PNG),
    p_orthoBitmap("orthographic16.png", wxBITMAP_TYPE_PNG),
    p_perspBitmap("perspective16.png", wxBITMAP_TYPE_PNG),
    p_centerLockButton(NULL),
    p_cameraButton(NULL),
    p_propertyPanelInfo()
{
}



Builder::Builder( wxWindow* parent, bool standalone, wxWindowID id,
        const wxString& caption, const wxPoint& pos, const wxSize& size,
        long style )
  : BuilderGUI(),
    WxDavAuth(),
    JMSPublisher(BUILDER),
    WxVizToolFW(),
    Listener("Builder"),
    WxVizTool(),
    FeedbackSaveHandler(),
    p_sgMgr(NULL),
    p_viewer(NULL),
    p_currentMode(ID_MODE_SELECT),
    p_currentElement("C"),
    p_currentShapes(),
    p_currentBond(1.0),
    p_xrot(NULL),
    p_yrot(NULL),
    p_zrot(NULL),
    p_viewerButton(NULL),
    p_rotx(0),
    p_roty(0),
    p_rotz(0),
    p_copyBuffer(NULL),
    p_fileToolbar(NULL),
    p_modeToolbar(NULL),
    p_viewToolbar(NULL),
    p_styleToolbar(NULL),
    p_manipulatorToolbar(NULL),
    p_measureToolbar(NULL),
    p_toolbars(),
    p_fileMenu(NULL),
    p_editMenu(NULL),
    p_optionsMenu(NULL),
    p_renderMenu(NULL),
    p_buildMenu(NULL),
    p_modeMenu(NULL),
    p_measureMenu(NULL),
    p_toolMenu(NULL),
    p_toolbarMenu(NULL),
    p_propertyMenu(NULL),
    p_contextMenu(NULL),
    p_contextHistory(NULL),
    p_contextPanel(NULL),
    p_menus(),
    p_mainSizer(NULL),
    p_rotSizer(NULL),
    p_cmd(NULL),
    p_viewerEvtHandler(NULL),
    p_mgr(),
    p_toolCount(0),
    p_toolbarCount(0),
    p_pertab(NULL),
    p_structLib(NULL),
    p_standalone(standalone),
    p_readOnlyDisabledIds(),
    p_calculations(),
    p_calculation(NULL),
    p_commandManagers(),
    p_commandManager(NULL),
    p_dirty(),
    p_import(),
    p_lockBitmap("center_view_lock16.png", wxBITMAP_TYPE_PNG),
    p_unlockBitmap("center_view_unlock16.png", wxBITMAP_TYPE_PNG),
    p_orthoBitmap("orthographic16.png", wxBITMAP_TYPE_PNG),
    p_perspBitmap("perspective16.png", wxBITMAP_TYPE_PNG),
    p_centerLockButton(NULL),
    p_cameraButton(NULL),
    p_propertyPanelInfo()
{
  Create(parent, standalone, id, caption, pos, size, style);
}



bool Builder::Create( wxWindow* parent, bool standalone, wxWindowID id,
        const wxString& caption, const wxPoint& pos, const wxSize& size,
        long style )
{
  if (!BuilderGUI::Create(parent, id, caption, pos, size, style)) {
    wxFAIL_MSG( wxT("Builder creation failed") );
    return false;
  }

  p_standalone = standalone;

  connectToolKitFW(this);
  setSaveHandler(this);

  p_mgr.SetManagedWindow(this);
  p_mgr.SetArtProvider(new BuilderDockArt);
  
  p_viewerEvtHandler = new ViewerEvtHandler(this);
  p_viewerEvtHandler->connectToolKitFW(this);
  PushEventHandler(p_viewerEvtHandler);
  Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(Builder::OnKeyDown));

  TPerTab tpt;
  ShapeData shapes;
  int numAtoms = tpt.numAtoms();
  for (int ele=0; ele<numAtoms; ele++) {
    p_currentShapes[tpt.atomicSymbol(ele)] = shapes.getGeometryName(ele);
  }

  // Initialize inventor classes
  SoWx::init(this);
  ChemInit::initClasses();
  SGSelection::initClass();
  NodesInit::initClasses();
  AtomNodesInit::initClasses();
  SGFragment::initClass();
  SGContainer::initClass();
  SGContainerManager::initClass();

  createMenus();
  createToolbar();
  createMainPanel();
  createToolPanels();
  createReadOnlyIds();

  // Subscribe to internal eventing system
  subscribe(this);

  if (getenv("ECCE_DEVELOPER")) {
    p_cmd->setLeaveAsEnter(true);
    p_cmd->SetValue("> ");
  }
   
  // Get Registry
  ResourceDescriptor rs = ResourceDescriptor::getResourceDescriptor();
  // Set desktop icon
  SetIcon(wxIcon(ewxBitmap::pixmapFile(rs.getTool(BUILDER)->getIcon()),
                                       wxBITMAP_TYPE_XPM));

  // create first (Default) IPropCalculation and setContext
  wxCommandEvent emptyEvent;
  OnNewClick(emptyEvent);

  // Cannot call this until we have an SGContainer, established by the first
  // call to setContext.  OnNewClick above calls setContext.
  restoreSettings();

  if (p_viewer->getSelectModeDrag()) {
    p_viewer->getSel()->lassoType.setValue(SGSelection::NOLASSO);
  } else {
    p_viewer->getSel()->lassoType.setValue(SGSelection::DRAGGER);
  }

  // For importing output files
  // Also called from the Organizer, but needed here for users who have
  // never run the Organizer. Added by GDB 9/22/11
  string msg = "";
  RunMgmt::registerLocalMachine(msg);
  if (msg != "") {
    wxLogMessage(msg.c_str(), 0);
  }


  return true;
}



Builder::~Builder()
{
  unsubscribe();
  PopEventHandler(true);
  
  if (p_copyBuffer)
    delete p_copyBuffer;

  if (p_contextHistory) {
    delete p_contextHistory;
  }

  wxLog::SetActiveTarget(NULL);
  ewxConfig::closeConfigs();
}


bool Builder::Show(bool show)
{
   return BuilderGUI::Show(show);
}


/**
 * Create toolbar in code because dialog blocks doesn't provide enough
 * options and control.
 * This code was copied from generated code and then modified.
 */
void Builder::createToolbar()
{
    p_fileToolbar = new wxToolBar(this, ID_FILE_TOOLBAR, wxDefaultPosition,
            wxDefaultSize, wxTB_FLAT|wxTB_HORIZONTAL|wxNO_BORDER);
    p_fileToolbar->SetToolSeparation(0);
    p_fileToolbar->SetToolPacking(0);
    p_fileToolbar->SetToolBitmapSize(wxSize(22, 22));
    ewxBitmap newBitmap("filenew.png", wxBITMAP_TYPE_PNG);
    p_fileToolbar->AddTool(wxID_NEW, _T(""), newBitmap, _("New"));
    ewxBitmap openBitmap("fileopen.png", wxBITMAP_TYPE_PNG);
    p_fileToolbar->AddTool(wxID_OPEN, _T(""), openBitmap, _("Open"));
    ewxBitmap saveBitmap("filesave.png", wxBITMAP_TYPE_PNG);
    p_fileToolbar->AddTool(wxID_SAVE, _T(""), saveBitmap, _("Save"));
    ewxBitmap saveasBitmap("filesaveas.png", wxBITMAP_TYPE_PNG);
    p_fileToolbar->AddTool(wxID_SAVEAS, _T(""), saveasBitmap, _("Save As"));
    ewxBitmap undoBitmap("undo.png", wxBITMAP_TYPE_PNG);
    p_fileToolbar->AddTool(wxID_UNDO, _T(""), undoBitmap, _("Undo"));
    ewxBitmap redoBitmap("redo.png", wxBITMAP_TYPE_PNG);
    p_fileToolbar->AddTool(wxID_REDO, _T(""), redoBitmap, _("Redo"));

    p_modeToolbar = new wxToolBar(this, ID_MODE_TOOLBAR, wxDefaultPosition,
            wxDefaultSize, wxTB_FLAT|wxTB_HORIZONTAL|wxNO_BORDER);
    p_modeToolbar->SetToolSeparation(0);
    p_modeToolbar->SetToolPacking(0);
    p_modeToolbar->SetToolBitmapSize(wxSize(22, 22));
    ewxBitmap selBitmap("mode_select.png", wxBITMAP_TYPE_PNG);
    p_modeToolbar->AddTool(ID_MODE_SELECT, _T(""), selBitmap,
                           _("Select"), wxITEM_CHECK);
    ewxBitmap rotBitmap("mode_rotate.png", wxBITMAP_TYPE_PNG);
    p_modeToolbar->AddTool(ID_MODE_ROTATE, _T(""), rotBitmap,
                           _("Rotate"), wxITEM_CHECK);
    ewxBitmap moveBitmap("mode_move.png", wxBITMAP_TYPE_PNG);
    p_modeToolbar->AddTool(ID_MODE_TRANSLATE, _T(""), moveBitmap,
                           _("Translate"), wxITEM_CHECK);
    ewxBitmap zoomBitmap("mode_zoom.png", wxBITMAP_TYPE_PNG);
    p_modeToolbar->AddTool(ID_MODE_ZOOM, _T(""), zoomBitmap,
                           _("Zoom"), wxITEM_CHECK);
    ewxBitmap atomBitmap("mode_select.png", wxBITMAP_TYPE_PNG);
    p_modeToolbar->AddTool(ID_MODE_ATOM, _T(""), atomBitmap,
                           _("Choose Build Element"), wxITEM_CHECK);
    ewxBitmap shapeBitmap(ShapeData::shapeToSmallImage(ShapeData::tetrahedral));
    p_modeToolbar->AddTool(ID_MODE_SHAPE, _T(""), shapeBitmap,
                           _("Geometry"), wxITEM_NORMAL);
    ewxBitmap bondBitmap(TBond::orderToSmallImage(TBond::Single));
    p_modeToolbar->AddTool(ID_MODE_BOND, _T(""), bondBitmap,
                           _("Choose Build Bond"), wxITEM_CHECK);
    ewxBitmap structBitmap("structlib.png");
    p_modeToolbar->AddTool(ID_MODE_STRUCTLIB, _T(""), structBitmap,
                           _("Import from Structure Library"), wxITEM_CHECK);
    updateElementIcon();
    p_modeToolbar->ToggleTool(p_currentMode, true);
   
    p_viewToolbar = new wxToolBar(this, ID_VIEW_TOOLBAR, wxDefaultPosition,
            wxDefaultSize, wxTB_FLAT|wxTB_HORIZONTAL|wxNO_BORDER);
    p_viewToolbar->SetToolSeparation(0);
    p_viewToolbar->SetToolPacking(0);
    p_viewToolbar->SetToolBitmapSize(wxSize(22, 22));
    p_viewToolbar->AddTool(ViewerEvtHandler::ID_DEPTH_CUEING, _T(""),
            ewxBitmap("depthcue.png", wxBITMAP_TYPE_PNG),
            _("Enable/disable depth cueing"), wxITEM_CHECK);
    ewxBitmap spinBitmap("spin.xpm");
    p_viewToolbar->AddTool(ViewerEvtHandler::ID_ENABLE_SPINNING, _T(""),
            spinBitmap, _("Enable/disable spinning"), wxITEM_CHECK);

    p_styleToolbar = new wxToolBar(this, ID_STYLE_TOOLBAR, wxDefaultPosition,
            wxDefaultSize, wxTB_FLAT|wxTB_HORIZONTAL|wxNO_BORDER);
    ewxBitmap itemtool16Bitmap("axis.png", wxBITMAP_TYPE_PNG);
    p_styleToolbar->AddTool(ViewerEvtHandler::ID_SHOW_AXES, _T(""),
            itemtool16Bitmap, _("Show/hide axis"), wxITEM_CHECK);
    ewxBitmap itemtool17Bitmap("rings.xpm");
    p_styleToolbar->AddTool(ViewerEvtHandler::ID_SHOW_RING, _T(""),
            itemtool17Bitmap, _("Display aromatic rings"), wxITEM_CHECK);
    ewxBitmap styleBitmap("styles.xpm");
    p_styleToolbar->AddTool(ID_TOOL_STYLE, _T(""), styleBitmap,
            _("Set global display style")); 
    ewxBitmap bgcolorBitmap("background.png", wxBITMAP_TYPE_PNG);
    p_styleToolbar->AddTool(ViewerEvtHandler::ID_BACKGROUND_COLOR, _T(""),
            bgcolorBitmap, _("Background Color"), wxITEM_NORMAL);
    ewxBitmap fgcolorBitmap("textcolor22.png", wxBITMAP_TYPE_PNG);
    p_styleToolbar->AddTool(ViewerEvtHandler::ID_FOREGROUND_COLOR, _T(""),
            fgcolorBitmap, _("Atom Label Color"), wxITEM_NORMAL);

    p_manipulatorToolbar = new wxToolBar(this, ID_DRAGGER_TOOLBAR,
            wxDefaultPosition, wxDefaultSize,
            wxTB_FLAT|wxTB_HORIZONTAL|wxNO_BORDER);
    p_manipulatorToolbar->SetToolSeparation(0);
    p_manipulatorToolbar->SetToolPacking(0);
    p_manipulatorToolbar->SetToolBitmapSize(wxSize(22, 22));
    p_manipulatorToolbar->AddTool(ViewerEvtHandler::ID_MANIPULATOR_SPHERE,
            _T(""), ewxBitmap("sphere22.png", wxBITMAP_TYPE_PNG),
            _("Create Sphere Manipulator"), wxITEM_NORMAL);
    p_manipulatorToolbar->AddTool(ViewerEvtHandler::ID_MANIPULATOR_NOSPHERE,
            _T(""), ewxBitmap("nosphere22.png", wxBITMAP_TYPE_PNG),
            _("Clear Sphere Manipulator"), wxITEM_NORMAL);
    p_manipulatorToolbar->AddTool(ViewerEvtHandler::ID_MANIPULATOR_WHEEL,
            _T(""), ewxBitmap("rotation22.png", wxBITMAP_TYPE_PNG),
            _("Create Wheel Manipulator"), wxITEM_NORMAL);
    p_manipulatorToolbar->AddTool(ViewerEvtHandler::ID_MANIPULATOR_NOWHEEL,
            _T(""), ewxBitmap("norotation22.png", wxBITMAP_TYPE_PNG),
            _("Clear Wheel Manipulator"), wxITEM_NORMAL);
    
    p_measureToolbar = new wxToolBar(this, ID_MEASURE_TOOLBAR,
            wxDefaultPosition, wxDefaultSize,
            wxTB_FLAT|wxTB_HORIZONTAL|wxNO_BORDER);
    p_measureToolbar->SetToolSeparation(0);
    p_measureToolbar->SetToolPacking(0);
    p_measureToolbar->SetToolBitmapSize(wxSize(22, 22));
    p_measureToolbar->AddTool(ViewerEvtHandler::ID_MEASURE_DIST, _T(""),
            ewxBitmap("measure_length.png",wxBITMAP_TYPE_PNG),
            _("Measure Distance"), wxITEM_NORMAL);
    p_measureToolbar->AddTool(ViewerEvtHandler::ID_MEASURE_ANGLE, _T(""),
            ewxBitmap("measure_angle.png",wxBITMAP_TYPE_PNG),
            _("Measure Angle"), wxITEM_NORMAL);
    p_measureToolbar->AddTool(ViewerEvtHandler::ID_MEASURE_TORSION, _T(""),
            ewxBitmap("measure_torsion.png",wxBITMAP_TYPE_PNG),
            _("Measure Torsion"), wxITEM_NORMAL);
    p_measureToolbar->AddTool(ViewerEvtHandler::ID_MEASURE_CLEAR, _T(""),
            ewxBitmap("measure_clear.png",wxBITMAP_TYPE_PNG),
            _("Measure Clear"), wxITEM_NORMAL);

    addToolBar(p_fileToolbar, NAME_TOOLBAR_FILE);
    addToolBar(p_modeToolbar, NAME_TOOLBAR_MODE);
    addToolBar(p_viewToolbar, NAME_TOOLBAR_VIEW);
    addToolBar(p_styleToolbar, NAME_TOOLBAR_STYLE);
    addToolBar(p_manipulatorToolbar, NAME_TOOLBAR_MANIPULATOR);
    addToolBar(p_measureToolbar, NAME_TOOLBAR_MEASURE);
}



void Builder::createMenus()
{
  p_fileMenu = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("File")));
  if (!getenv("ECCE_DEVELOPER")) {
    p_fileMenu->Delete(ID_DUMPSG);
  }
  if (isStandalone()) {
    p_fileMenu->Delete(ID_IMPORTCALC);
  }

  p_editMenu = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("Edit")));

  p_optionsMenu = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("Options")));
  ViewerEvtHandler::createOptionMenu(p_optionsMenu);

  p_renderMenu = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("Render")));
  ViewerEvtHandler::createRenderMenu(p_renderMenu);

  p_buildMenu = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("Build")));
  ViewerEvtHandler::createBuildMenu(p_buildMenu);

  p_modeMenu = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("Mode")));
  p_modeMenu->AppendRadioItem(ID_MODE_SELECT,
                              NAME_MODE_SELECT    + "\tCTRL+1");
  p_modeMenu->AppendRadioItem(ID_MODE_ROTATE,
                              NAME_MODE_ROTATE    + "\tCTRL+2");
  p_modeMenu->AppendRadioItem(ID_MODE_TRANSLATE,
                              NAME_MODE_TRANSLATE + "\tCTRL+3");
  p_modeMenu->AppendRadioItem(ID_MODE_ZOOM,
                              NAME_MODE_ZOOM      + "\tCTRL+4");
  p_modeMenu->AppendRadioItem(ID_MODE_ATOM,
                              NAME_MODE_ATOM      + "\tCTRL+5");
  p_modeMenu->AppendRadioItem(ID_MODE_BOND,
                              NAME_MODE_BOND      + "\tCTRL+6");
  p_modeMenu->AppendRadioItem(ID_MODE_STRUCTLIB,
                              NAME_MODE_STRUCTLIB + "\tCTRL+7");
  p_modeMenu->Check(p_currentMode, true);

  p_measureMenu = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("Measure")));
  ViewerEvtHandler::createMeasureMenu(p_measureMenu);

  p_toolMenu = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("Tools")));
  p_toolbarMenu = new wxMenu;
  p_toolMenu->AppendSubMenu(p_toolbarMenu, _("Toolbars"),
                            _("Show/Hide Toolbars"));

  p_propertyMenu = GetMenuBar()->GetMenu(
                   GetMenuBar()->FindMenu(_("Properties")));

  p_contextMenu = GetMenuBar()->GetMenu(GetMenuBar()->FindMenu(_("Context")));
  p_contextHistory = new ContextHistory(100, ID_CONTEXT_HISTORY_BASE);
  p_contextHistory->UseMenu(p_contextMenu);

  p_menus.push_back(p_fileMenu);
  p_menus.push_back(p_editMenu);
  p_menus.push_back(p_optionsMenu);
  p_menus.push_back(p_renderMenu);
  p_menus.push_back(p_buildMenu);
  p_menus.push_back(p_modeMenu);
  p_menus.push_back(p_measureMenu);
  p_menus.push_back(p_toolMenu);
  p_menus.push_back(p_toolbarMenu);
  //p_menus.push_back(p_propertyMenu);
  //p_menus.push_back(p_contextMenu);
}



void Builder::createMainPanel()
{
  wxPanel * mainPanel = new wxPanel(this, wxID_ANY);
  p_mainSizer = new wxBoxSizer(wxVERTICAL);
  mainPanel->SetSizer(p_mainSizer);


  p_sgMgr = new SGContainerManager();
  p_sgMgr->ref();

  p_viewer = new SGViewer(mainPanel, wxID_ANY);
  p_viewer->getSel()->addFinishCallback(&Builder::selectionChangeCB, this);
  p_viewer->getSel()->addMotionListener(this);
  p_viewer->setText("","","","");
  p_viewer->setSceneGraph(p_sgMgr);
  p_viewer->addMouseEventListener(this);


  //KLS TODO default should be SCREEN_DOOR with an override to DELAYED_ADD
  //p_viewer->setTransparencyType(SoGLRenderAction::DELAYED_ADD);
  p_viewer->setViewing(false);
  p_mainSizer->Add(p_viewer, 1, wxEXPAND|wxALL, 0);

  createRotators(mainPanel);
  
  if (getenv("ECCE_DEVELOPER")) {
    p_cmd = new ewxTextCtrl(mainPanel, ID_TEXTCTRL_CMD, _T(""),
                            wxDefaultPosition, wxDefaultSize,
                            wxTE_PROCESS_ENTER );
    p_mainSizer->Add(p_cmd, 0, wxEXPAND|wxALL, 0);
  }
   
  setMode(ID_MODE_SELECT);

  p_mgr.AddPane(mainPanel, ewxAuiPaneInfo().CenterPane().
          Name("Viewer").Caption("Viewer").CaptionVisible().
          Show().Open().MaximizeButton()
          );
}



void Builder::createRotators(wxWindow *parent)
{
  p_rotSizer = new wxBoxSizer(wxHORIZONTAL);
  p_mainSizer->Add(p_rotSizer, 0, wxEXPAND|wxALL, 0);
  p_rotSizer->Add(new wxStaticText(parent, -1, "Rotation: X"), 0,
                  wxALIGN_CENTER|wxLEFT, 5);
  p_xrot = new wxSpinCtrl(parent, ID_ROT_X, "0", wxDefaultPosition,
                           wxSize(50, -1), wxSP_WRAP|wxSP_ARROW_KEYS,
                           -179, 180, 0);
  p_rotSizer->Add(p_xrot, 0, wxALIGN_CENTER|wxALL, 0);
  p_rotSizer->Add(new wxStaticText(parent, -1, "Y"), 0,
                  wxALIGN_CENTER|wxLEFT, 5);
  p_yrot = new wxSpinCtrl(parent, ID_ROT_Y, "0", wxDefaultPosition,
                           wxSize(50, -1), wxSP_WRAP|wxSP_ARROW_KEYS,
                           -179, 180, 0);
  p_rotSizer->Add(p_yrot, 0, wxALIGN_CENTER|wxALL, 0);
  p_rotSizer->Add(new wxStaticText(parent, -1, "Z"), 0,
                   wxALIGN_CENTER|wxLEFT, 5);
  p_zrot = new wxSpinCtrl(parent, ID_ROT_Z, "0", wxDefaultPosition,
                           wxSize(50, -1), wxSP_WRAP|wxSP_ARROW_KEYS,
                           -179, 180, 0);
  p_rotz = 0;
  p_rotSizer->Add(p_zrot, 0, wxALIGN_CENTER|wxALL, 0);
  p_rotSizer->AddStretchSpacer(1);

  //p_viewerButton = new ewxButton(parent, ID_CHOICE_VIEWER, "VCB");
  //p_rotSizer->Add(p_viewerButton);

  ewxBitmap centerBitmap("center_view16.png", wxBITMAP_TYPE_PNG);
  ewxBitmapButton * zoomFit = new ewxBitmapButton(parent, wxID_ZOOM_FIT,
          centerBitmap, wxDefaultPosition, wxDefaultSize,
          wxBU_AUTODRAW|wxNO_BORDER);
  zoomFit->SetToolTip(_T("Center system"));
  p_rotSizer->Add(zoomFit, 0, wxALIGN_CENTER|wxALL, 0);

  p_centerLockButton = new ewxBitmapButton(parent,
                                           ViewerEvtHandler::ID_AUTO_NORMALIZE,
                                           p_unlockBitmap,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxBU_AUTODRAW|wxNO_BORDER);
  p_centerLockButton->SetToolTip(_T("Auto center after each operation"));
  p_rotSizer->Add(p_centerLockButton, 0, wxALIGN_CENTER|wxALL, 0);

  ewxBitmap homeBitmap("home_view16.png", wxBITMAP_TYPE_PNG);
  ewxBitmapButton * homeView = new ewxBitmapButton(parent, ID_TOOL_GOHOME,
          homeBitmap, wxDefaultPosition, wxDefaultSize,
          wxBU_AUTODRAW|wxNO_BORDER);
  p_rotSizer->Add(homeView, 0, wxALIGN_CENTER|wxALL, 0);
  homeView->SetToolTip(_T("Go to home view"));

  ewxBitmap setHomeBitmap("set_home_view16.png", wxBITMAP_TYPE_PNG);
  ewxBitmapButton * setHomeView = new ewxBitmapButton(parent, ID_TOOL_SETHOME,
          setHomeBitmap, wxDefaultPosition, wxDefaultSize,
          wxBU_AUTODRAW|wxNO_BORDER);
  p_rotSizer->Add(setHomeView, 0, wxALIGN_CENTER|wxALL, 0);
  setHomeView->SetToolTip(_T("Set the home view"));

  p_cameraButton = new ewxBitmapButton(parent,
                                       ViewerEvtHandler::ID_CAMERA_TYPE,
                                       p_perspBitmap,
                                       wxDefaultPosition, wxDefaultSize,
                                       wxBU_AUTODRAW|wxNO_BORDER);
  p_rotSizer->Add(p_cameraButton, 0, wxALIGN_CENTER|wxALL, 0);
  p_cameraButton->SetToolTip(_T("Toggle between perpective and orthographic cameras "));

  ewxBitmap zoomToBitmap("viewmagfit.png", wxBITMAP_TYPE_PNG);
  ewxBitmapButton * zoomTo = new ewxBitmapButton(parent, 
          ViewerEvtHandler::ID_ZOOM_TO, zoomToBitmap, wxDefaultPosition,
          wxDefaultSize, wxBU_AUTODRAW|wxNO_BORDER);
  zoomTo->SetToolTip(_T("Zoom to fit"));
  p_rotSizer->Add(zoomTo, 0, wxALIGN_CENTER|wxALL, 0);

  ewxBitmap zoomInBitmap("viewmag+.png", wxBITMAP_TYPE_PNG);
  ewxBitmapButton * zoomIn = new ewxBitmapButton(parent, wxID_ZOOM_IN,
          zoomInBitmap, wxDefaultPosition, wxDefaultSize,
          wxBU_AUTODRAW|wxNO_BORDER);
  zoomIn->SetToolTip(_T("Zoom in"));
  p_rotSizer->Add(zoomIn, 0, wxALIGN_CENTER|wxALL, 0);

  ewxBitmap zoomOutBitmap("viewmag-.png", wxBITMAP_TYPE_PNG);
  ewxBitmapButton * zoomOut = new ewxBitmapButton(parent, wxID_ZOOM_OUT,
          zoomOutBitmap, wxDefaultPosition, wxDefaultSize,
          wxBU_AUTODRAW|wxNO_BORDER);
  zoomOut->SetToolTip(_T("Zoom out"));
  p_rotSizer->Add(zoomOut, 0, wxALIGN_CENTER|wxALL, 0);
}



void Builder::createToolPanels()
{
  p_contextPanel = new ContextPanel(this);
  addToolPanel(p_contextPanel, NAME_TOOL_CONTEXT, false);

  p_pertab = new MiniPerTab(this, -1);
  addToolPanel(p_pertab, NAME_TOOL_BUILD);

  CoordPanel * coordtools = new CoordPanel(this);
  addToolPanel(coordtools, NAME_TOOL_COORDINATES);

  SelectionPanel * seltools = new SelectionPanel(this);
  addToolPanel(seltools, NAME_TOOL_SELECTION, false);

  AtomTable *geomtable = new AtomTable(this,-1);
  addToolPanel(geomtable, NAME_TOOL_ATOM_TABLE, false, false);
  p_mgr.GetPane(NAME_TOOL_ATOM_TABLE).Left();

  ResidueTable *residuetable = new ResidueTable(this,-1);
  addToolPanel(residuetable, NAME_TOOL_RESIDUE_TABLE, false, false);
  p_mgr.GetPane(NAME_TOOL_RESIDUE_TABLE).Left();

  SymmetryPanel *symmetry = new SymmetryPanel(this,-1);
  addToolPanel(symmetry, NAME_TOOL_SYMMETRY);

  Dna *dna = new Dna(this, -1);
  addToolPanel(dna, NAME_TOOL_DNA_BUILDER);

  Peptide *peptide = new Peptide(this, -1);
  addToolPanel(peptide, NAME_TOOL_PEPTIDE_BUILDER);

  PBC *pbc = new PBC(this,-1);
  addToolPanel(pbc, NAME_TOOL_PBC, false);

  Slice *slice = new Slice(this,-1);
  addToolPanel(slice, NAME_TOOL_SLICER, false);

  p_toolMenu->AppendSeparator();

  ewxTextCtrl * log = new ewxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
          wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY|wxSUNKEN_BORDER);
  ewxLogTextCtrl *logWindow = new ewxLogTextCtrl(log, GetStatusBar());
  delete wxLog::SetActiveTarget(logWindow);
  addToolPanel(log, NAME_TOOL_LOG, false, false);

  if (getenv("ECCE_DEVELOPER")) {
    p_toolMenu->AppendCheckItem(ID_SHOW_CMD, NAME_TOOL_COMMAND_LINE, "");
  }

  p_structLib = new StructLib(this);
  p_structLib->configure();
  ewxAuiPaneInfo pinfo;
  pinfo.Name(NAME_TOOL_STRUCTLIB).Caption(NAME_TOOL_STRUCTLIB)
          .Show(false).CaptionVisible(true).Right().AlwaysFixed(false);
  p_mgr.AddPane(p_structLib, pinfo);
}



void Builder::createReadOnlyIds()
{
  p_readOnlyDisabledIds.insert(ID_IMPORT);

  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_CLEAN_COORD);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_ADD_H);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_DEL_H);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_GENERATE_BOND);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_CLEAR_RESIDUE);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_MAKE_RESIDUE);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_CENTER_X);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_CENTER_Y);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_CENTER_Z);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_CENTER_XYZ);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_ORIENT);

  p_readOnlyDisabledIds.insert(ID_MODE_ATOM);
  p_readOnlyDisabledIds.insert(ID_MODE_SHAPE);
  p_readOnlyDisabledIds.insert(ID_MODE_BOND);
  p_readOnlyDisabledIds.insert(ID_MODE_STRUCTLIB);
  
  p_readOnlyDisabledIds.insert(ID_DRAGGER_TOOLBAR);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_MANIPULATOR_SPHERE);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_MANIPULATOR_NOSPHERE);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_MANIPULATOR_WHEEL);
  p_readOnlyDisabledIds.insert(ViewerEvtHandler::ID_MANIPULATOR_NOWHEEL);
}



/**
 * Create the IPropCalculation object from the given url.
 */
IPropCalculation* Builder::createCalculation(const string& url)
{
  if (url != DefaultCalculation::nextURL()
          && EcceURL(url).isLocal()
          && !wxFileExists(url)) {
    return NULL;
  }
  return CalculationFactory::open(url);
}



void Builder::reportError(const string& msg)
{
  showMessage(msg, true);
  wxBell();
}


void Builder::interpretCommand(const string& command)
{
   try {
      Command *cmd = createCommand(command);
      execute(cmd);
    } catch (EcceException& ex) {
      reportError(ex.what());
    }
}

/**
 * An initial cut at a command interpreter.
 * The real implementation will:
 *   . rely on a text file of command names/classes to generate some code
 *   . support a list of all commands
 *   . support list of parameter names and types
 * One tricky thing is the use of parameters that have no value.
 *   e.g. select all
 */
Command *Builder::createCommand(const string& input) throw(EcceException)
{
   Command *cmd = 0;
   string cmdname;
   string myinput;
   if (input[0] == '>') {
      myinput = input.substr(1);
   } else {
      myinput = input;
   }
   myinput = STLUtil::trim(myinput);
   size_t space = myinput.find(" ");
   if (space > 0) {
     cmdname = myinput.substr(0,space);
   } else {
     cmdname = myinput;
   }
   if (cmdname != "") {
     //cout << "cmdname=" << cmdname << "!" << endl;
     if (cmdname == "open" || cmdname == "import") {
        cmd = new AddFragmentCmd(cmdname, getSG());
     } else if (cmdname == "clear") {
        cmd = new NewFragmentCmd(cmdname, getSG());
     } else if (cmdname == "add") {
        cmd = new AtomEditCmd(cmdname, getSG());
     } else if (cmdname == "depthcue") {
        cmd = new DepthCueCmd(cmdname, getSG());
     } else if (cmdname == "select") {
        cmd = new SelectCmd(cmdname, getSG());
     } else if (cmdname == "selectall") {
        cmd = new SelectAllCmd(cmdname, getSG());
     } else if (cmdname == "unselect") {
        cmd = new UnselectCmd(cmdname, getSG());
     } else if (cmdname == "centerview") {
        cmd = new CenterViewCmd(cmdname, p_viewer);
     } else if (cmdname == "center") {
        cmd = new CenterCmd(cmdname, getSG());
     } else if (cmdname == "addh") {
        cmd = new HydrogensCmd(cmdname, getSG());
     } else if (cmdname == "removeh") {
        cmd = new RmHydrogensCmd(cmdname, getSG());
     } else {
        throw EcceException(string("Command not defined: ") + cmdname,WHERE);
     }

     string args;
     if (space != string::npos) {
        args = myinput.substr(space);
     }
     cmd->parse(args);
     //cout << *cmd << endl;
   } else {
      throw EcceException("No command specified."  ,WHERE);
      
   }
   return cmd;
}


/**
 *
 */
Command * Builder::createModeCommand(int modeId) throw(EcceException)
{

  Command * cmd = 0;

  switch (modeId) {
  case ID_MODE_SELECT:
    // No need to do anything here!
    break;
  case ID_MODE_ATOM:
    {
      try {
        int resIdx = checkResidueEditing();
        cmd = new AtomEditCmd("add", getSG());
        float x,y,z;
        p_viewer->getSel()->getPickCoords(x,y,z);
        cmd->getParameter("x")->setDouble(x);
        cmd->getParameter("y")->setDouble(y);
        cmd->getParameter("z")->setDouble(z);
        cmd->getParameter("elem")->setString(p_currentElement);
        cmd->getParameter("geom")->setString(p_currentShapes[p_currentElement]);
        cmd->getParameter("bondOrder")->setDouble(p_currentBond);
        cmd->getParameter("resIndex")->setInteger(resIdx);
        break;
      } catch (CancelException) {}
    }
  case ID_MODE_BOND:
    cmd = new BondEditCmd("bond", getSG());
    cmd->getParameter("order")->setDouble(p_currentBond);
    if (!(getSG()->getFragment()->onSameMolecule())) {
      ewxMessageDialog mdlg(this, 
                            "The current selections are from two molecules.\n"
                            "Do you want to align them along the new bond?",
                            "Align molecules?",
                            wxYES_NO|wxYES_DEFAULT|wxICON_QUESTION);
      cmd->getParameter("orient")->setBoolean(mdlg.ShowModal() == wxID_YES);
    }
    break;
  case ID_MODE_STRUCTLIB:
    {
      Fragment *frag = p_structLib->getSelectedFragment();
      if (!frag) {
        // Report no selection in struct lib error
        ewxMessageDialog mdlg(this,
                              "Please select a structure from the Structure "
                              "Library first.",
                              "No Structure Seleted!",
                              wxOK|wxICON_EXCLAMATION);
        mdlg.ShowModal();
        break;
      }

      cmd = new AddFragmentCmd("Add Structure", getSG());
      ostringstream os;
      frag->dumpMVM(os, true);
      cmd->getParameter("mvmStream")->setString(os.str());
      cmd->getParameter("conatom")->setInteger(p_structLib->getSelectedAtom());
      cmd->getParameter("streamType")->setString("MVM");
      cmd->getParameter("genBondOrders")->setBoolean(false);
      float x,y,z;
      p_viewer->getSel()->getPickCoords(x,y,z);
      cmd->getParameter("x")->setDouble(x);
      cmd->getParameter("y")->setDouble(y);
      cmd->getParameter("z")->setDouble(z);
      break;
    }
    

      
     /*
   case mbFreeBond:
     // @todo Create MetalPiBondCmd here
     break;
   case mbMove:
     break;
   case mbRotate:
     break;
   case mbLength:
     break;
   case mbAngle:
     break;
   case mbTorsion:
     break;
       Should never reach here since createModeCommand won't be called for
       these viewing modes.
       case mbCamRotate:
       case mbCamTranslate:
       case mbCamZoom:
       break;
   case mbGhost:
     break;
   case mbStructure:
     break;
   case mbLine:
     break;
     */
  }
  
  return cmd;
}


int Builder::checkResidueEditing() throw(CancelException)
{
   int resIndex = -1;
   SGFragment *frag = getSG()->getFragment();
   if (frag->numResidues() > 0 && frag->editResidueIndex() < 0) {
     ResidueIndexPrompt prompt(this);
     prompt.connectToolKitFW(this);
     if (p_viewer->getSel()->isFreePick()) {
       prompt.setInsertResidue();  // as default choice
     } else {
       prompt.setSelectResidue();  // as default choice
     }
     if (prompt.ShowModal() == wxID_OK) {
       resIndex = prompt.getResidueIndex();
       char buf[32];
       sprintf(buf,"%d",resIndex);
       if (prompt.isSelectResidue()) {
         getSG()->getFragment()->editResidueMode(true,resIndex);
         Event notify("ResEditIndexChange",buf);
         EventDispatcher::getDispatcher().publish(notify);
       } else if (prompt.isDeleteResidue()) {
         Command * cmd = new ClearResidueInfoCmd("Clear residue info",getSG());
         execute(cmd);
       } else if (prompt.isInsertResidue()) {
         string name = prompt.getResidueName();
         Command * cmd = new InsertResidueCmd("Insert residue",getSG());
         cmd->getParameter("name")->setString(name);
         cmd->getParameter("index")->setInteger(resIndex);
         execute(cmd);
         // Automatically make this new residue our current edit residue
         getSG()->getFragment()->editResidueMode(true,resIndex);
         Event notify("ResEditIndexChange",buf);
         EventDispatcher::getDispatcher().publish(notify);
       }
     } else {
       throw CancelException(WHERE);
     }
   }
   return resIndex;
}


/**
 * Set the builder in stand alone mode
 */
void Builder::setStandalone(bool standalone)
{
  p_standalone = standalone;
}


/**
 * Return true if the builder is in stand alone mode
 */
bool Builder::isStandalone()
{
  return p_standalone;
}


/**
 * Main url context method.
 * Doesn't matter whether the current context has changed since we're not
 * removing it.
 */
void Builder::setContext(const string& url, const bool& force) 
{
  wxBusyCursor busy;
  bool newContext = false;

  // Make sure it is raised/uniconified for popup dialogs
  Raise();

  if (p_calculation) {
    if (p_calculation->getURL()==url && !force) {
      // Bail if already in context
      return;
    }
    // unset focus of all viz prop panels, if we have any for the last context
    // this fixes seg fault if we're animating a trj, geom trace, vib, etc
    set<VizPropertyPanel*> vizpanels =
            VizPropertyPanel::getPanels(p_calculation->getURL());
    set<VizPropertyPanel*>::iterator vizpanel;
    for (vizpanel = vizpanels.begin();
         vizpanel != vizpanels.end();
         ++vizpanel) {
      (*vizpanel)->setFocus(false);
      ewxAuiPaneInfo &pane = p_mgr.GetPane(*vizpanel);
      if (pane.IsOk()) {
        pane.Focus(false);
        p_propertyPanelInfo[pane.window] = p_mgr.SavePaneInfo(pane);
      }
    }
  }

  // We don't allow struct lib mode across context switches
  if (p_currentMode == ID_MODE_STRUCTLIB) {
    setMode(ID_MODE_SELECT);
    toggleModeButton(ID_MODE_SELECT);
  }

  // If experiment was not previously opened, create it
  if (p_calculations.find(url) == p_calculations.end()) {
    newContext = true;
    // create the IPropCalculation and CommandManager objects
    IPropCalculation *calc = createCalculation(url);
    if (!calc) {
      wxLogError("There was an error parsing %s or it does not exist.",
                 url.c_str());
      return;
    }
    p_calculation = p_calculations[url] = calc;
    p_commandManager = p_commandManagers[url] = new CommandManager(url, 1000);
    p_sgMgr->setSceneGraph(url);

    // append to Context menu
    p_contextHistory->SetContext(url);
  } else {
    // Set experiment, command manager, and scene graph appropriately
    p_calculation = p_calculations[url];
    p_commandManager = p_commandManagers[url];
    p_sgMgr->setSceneGraph(url);
  }

  if (newContext || force) {
    // add fragment to scene graph, if one exists
    // NOTE: why do this if its a new context??
    Command *cmd = new NewFragmentCmd("Clear", getSG());
    execute(cmd);
    SGFragment *frag = getSG()->getFragment();
    bool solventFlag = false;

    // Use the ShowFragCmd to load the last geometry
    // Should be ok in normal builder mode since if there are no geometries,
    // we'll get the default geometry.
    cmd = new ShowFragCmd("Load", getSG(), p_calculation);
    cmd->getParameter("index")->setInteger(-1);
    execute(cmd);
    if (getSG()->getFragment()->numAtoms() > 0) {

      solventFlag = createSolventSoluteStyles(*(getSG()->getFragment()));
      getSG()->updateColorNodes();

      // TODO?? this is stuff from v4, not sure if it's relevent now
      // re-execute CmdAtomLabels?

      string ename = "GeomChange";
      if (frag->numResidues() > 0) ename = "ResidueChange";
      Event event(ename);
      EventDispatcher::getDispatcher().publish(event);

      center();
    } else {
      // no fragment
    }

    // restore default styles
    // GDB 3/7/08 conditionalize restoring the display style preference
    // because it will override the solute/solvent style otherwise
    if (!solventFlag) {
      ewxConfig *config = ewxConfig::getConfig("wxbuilder.ini");
      p_viewerEvtHandler->restoreSettings(config);
    }

    // Clear any initialization commands and dirty state
    getCM()->clearCommands();
    setDirty(false);
    setImport(false);
  }

  p_contextPanel->SetContext(url);

  applySceneGraphSettings();

  // Refresh the GUI based on the current IPropCalculation/Job contexts

  ewxTool::setContext(url);

  updateUndoMenus();
  updateSave();
  updateResource();
  updateReadOnly(true);
  updatePropertyMenus();
  updateMenus();
  updateViewerText();

  // Refresh Tool panels
  EventDispatcher::getDispatcher().publish(Event("UpdateUI"));

  // auto-open single panels
  set<PropertyPanel*> panels =
          PropertyPanel::getPanels(p_calculation->getURL().toString());
  if (panels.size() == 1) {
    bool doOpen = false;
    TaskJob * taskjob = dynamic_cast<TaskJob*>(p_calculation);
    if (taskjob) {
      if (taskjob->getState() >= ResourceDescriptor::STATE_COMPLETED) {
        doOpen = true;
      }
    } else {
      doOpen = true;
    }
    if (doOpen) {
      ewxAuiPaneInfo &info = p_mgr.GetPane(*panels.begin());
      if (info.IsOk()) {
        info.Open();
        p_mgr.Update(); // TODO is this needed yet or can it wait?
      }
    }
  }
}



/**
 * Given the NWChem selection string, select/highlight atoms.  This clears any
 * previous selection so that this is more intuitive for the user.
 */
void Builder::showSelectionFromNWChemString(string selection)
{
  Command * cmd;
  cmd = new UnselectCmd("unselect", getSG());
  execute(cmd);
  cmd = new SelectCmd("select", getSG());
  cmd->getParameter("NWChemSelection")->setString(selection);
  execute(cmd);
}


/**
 * Just forward this to a Builder member function.
 */
void Builder::selectionChangeCB(void *  data, ChemSelection * sel)
{
  if (!internalSelect) {
    ((Builder*)data)->processSelectionChange(sel);
  } else {
    internalSelect = false;
  }
}

bool Builder::mouseEvent(wxMouseEvent *event)
{
   // For not, process all button events.  May want to revert to just button up
   //if (event->ButtonUp() && p_viewer->isViewing()) {
   if (p_viewer->isViewing()) {
      SbMatrix mx;
      mx = p_viewer->getCamera()->orientation.getValue();
      //cout << "X"  << mx[0][0] << " " << mx[0][1] << " " << mx[0][2] << " " << mx[0][3] << endl;
      //cout << "Y"  << mx[1][0] << " " << mx[1][1] << " " << mx[1][2] << " " << mx[1][3] << endl;
      //cout << "Z"  << mx[2][0] << " " << mx[2][1] << " " << mx[2][2] << " " << mx[2][3] << endl;
      //cout << "4 " << mx[3][0] << " " << mx[3][1] << " " << mx[3][2] << " " << mx[3][3] << endl;

      // theta: elevation
      // phi: azimuth
      // psi: twist
      double theta = asin(-mx[2][1]);
      double costh = cos(theta);
      double sinphi, cosphi;
      double sinpsi, cospsi;
      double phi, psi;
      if (costh != 0) {
         sinphi = mx[0][1]/costh;
         cosphi = mx[1][1]/costh;
         sinpsi = -mx[2][0]/costh;
         cospsi = mx[2][2]/costh;
         phi = asin(sinphi);
         if (cosphi < 0.0) {
            phi = M_PI - phi;
         }
         psi = asin(sinpsi);
         if (cospsi < 0.0) {
            psi = M_PI - psi;
         }
      } else {
         sinphi = mx[0][2];
         cosphi = mx[0][0];
         psi = 0.0;
      }
      p_rotx = static_cast<int>(theta*180.0/M_PI);
      p_roty = static_cast<int>(phi*180.0/M_PI);
      p_rotz = static_cast<int>(psi*180.0/M_PI);

      p_xrot->SetValue(p_rotx);
      p_yrot->SetValue(p_roty);
      p_zrot->SetValue(p_rotz);
   }
   return false;
}


void Builder::processSelectionChange(ChemSelection *sel)
{
  SGSelection * esel = ( SGSelection *) sel;
  
  //  tu_busy_activate();

  if (esel->isFreePick() &&
      (p_currentMode == ID_MODE_ATOM
       || p_currentMode == ID_MODE_STRUCTLIB)) {
    // || p_currentMode == mbGhost)) {
    // Current mode: adding an atom or stuct.
    // Not all commands necessarilly use x,y,z coordinates but set them
    // for when they are required.  This seems better than stashing them
    // elsewhere in the interest of keeping the commands as autonomous as
    // possible.

    Command * cmd = createModeCommand(p_currentMode);
    if (cmd)
      execute(cmd);

    /*
    UCommand *cmd = getCM()->getCommand(p_currentCommand);
    if (cmd) {
      // This just dumps the command in XML for debugging
      cout << "Freepick: " << *cmd << endl;
    }
    bool status = getCM()->execute(p_currentCommand,(ICommandObject*)p_SG);
    afterCommand();
    revertToSelect(status);

    // This is a special case.  Since the user is making a selection, by
    // definition, MI will have its selection reset.  However, if the command
    // fails, the previous selection should be restored.  Otherwise MI thinks
    // the selection is cleared by ecce things its set.
    if (status == false) {
      internalSelect = True;
      SGFragment *frag = p_SG->getFragment();
      setSelection(frag->m_atomHighLight, frag->m_bondHighLight);
      internalSelect = False;
    }
    */
  } else if (esel->isFreePick() && p_currentMode == ID_MODE_SELECT) {
    // Click in free space means clear all selection;
    Command * cmd = new UnselectCmd("UnselectCmd", getSG());
    execute(cmd);
    // @todo
    //    indicateSelection(false);
  } else if (esel->isDistancePick()) {
    int x,y;
    esel->getPickPosition(x,y);

    lengthPopup(x, y, (AtomMeasureDist*)esel->getSelectedNode());
    // @todo
    //    indicateSelection(false);
  } else if (esel->isAnglePick()) {
    int x,y;
    esel->getPickPosition(x,y);
    anglePopup(x, y, (AtomMeasureAngle*)esel->getSelectedNode());
    // @todo
    //    indicateSelection(false);

  } else if (esel->isTorsionPick()) {
    int x,y;
    esel->getPickPosition(x,y);
    torsionPopup(x, y, (AtomMeasureTorsion*)esel->getSelectedNode());
    // @todo
    //    indicateSelection(false);
  } else if (esel->isRightClick()) {
    //cout << "popup right click edit " << esel->getRightClickAtom() << endl;
  } else {
    int length = esel->getNumDisplaysSelected();
    //cout << "num displays selected " << length << endl;
    // this is a gentle hack to handle the multiple display styles
    // callbacks when the selction list changes.  It's a long story, but
    // the only times we want to actually process a selection change is
    // when we make a pick (and length = 1) or when we lasso select (and
    // length = 6) or when we use the shift key to do multiple selects.
    // So this eliminates most of the callbacks, but there
    // are still times when length = 6 and this callback is hit 3 or 4
    // times.  I don't know why.
    int numDisplayStyles = getSG()->getNumDisplayStyles();
    if ( length == 1  || length == numDisplayStyles
         || esel->shiftActive() || esel->isDoubleClick() ) {
      // Should be safe to dereference the first child here because if
      // there isn't one, we should not have gotten here.
      SGFragment *sgfrag = getSG()->getFragment();
      if (sgfrag) {
        bool changed = esel->readSelection(sgfrag);
        //cout << "read selection " << changed << endl;
        if (changed || esel->isDoubleClick()) {
          if (esel->isDoubleClick() ) {
            int dcatom = esel->getDoubleClickAtom();
            if (dcatom >= 0) {
              TAtm *atm = sgfrag->atomRef(dcatom);
              Residue *res = sgfrag->findResidue(atm);
              if (res) selectResidue(*res,true);
            }
          }
          
          Command *cmd = createModeCommand(p_currentMode);
          execute(cmd);
          /*
          if (cmd) {
            // This just dumps the command in XML for debugging
            //cout << *cmd << endl;
          }
          bool status =
            getCM()->execute(p_currentCommand,(ICommandObject*)p_SG);
          afterCommand();
          revertToSelect(status);
          // This is a special case.  Since the user is making a
          // selection, by definition, MI will have its selection
          // reset.  However, if the command fails, the previous
          // selection should be restored.  Otherwise MI thinks
          // the selection is cleared but ecce thinks its set.
          if (status == false) {
            internalSelect = True;
            SGFragment *frag = p_SG->getFragment();
            setSelection(frag->m_atomHighLight, frag->m_bondHighLight);
            internalSelect = False;
          }
          */
        }
      }
      // messagingNotifySelection();
    }
    else {
      // this else should never happen --happens a lot
      //cout << "not processing " << length << endl;
    } 
  }
  
  // @todo
  //  VizEventManager::getEventManager().notifySelection(this) ;
  EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));
  
  //  tu_busy_deactivate();
}

void Builder::motionChanged(const MotionData& data)
{
   if (data.isButton1() && getMode() == ID_MODE_SELECT) {
      SGFragment *frag = getSG()->getFragment();
      // The command should not be invoked if nothing is highlighted.
      // This helps avoid or at least reduce the possibility of an infinite
      // loop of prompting if the user accidentally drags with a replicated system
      // where the prompting interfers with normal termination of motion
      if (frag && frag->m_atomHighLight.size() > 0 ) {

         Command *cmd = new TwoDMoveCmd("Translate", getSG());
         cmd->getParameter("deltax")->setDouble(data.getDeltaX());
         cmd->getParameter("deltay")->setDouble(data.getDeltaY());
         cmd->getParameter("deltaz")->setDouble(data.getDeltaZ());
         cmd->getParameter("movez")->setBoolean(data.wasShiftDown());
         cmd->getParameter("doundo")->setBoolean(data.wasStartMotion());
         execute(cmd);
      }

      // This may be important in other places...
      // If autonormalize is not on, then even though we think we are updating
      // the selection, we don't see the change.  If autonormalize is on, the
      // deep layers of code call camera->viewAll(...).
      // which apparently causes the selection change to be redrawn.
      // It may be that our setSelection calls should do this but I just hacked
      // this in place for now because I don't really know if/where its a problem.
      // Don't do it if autonormalize is on because afterCommand does this type
      // of thing already.
      /*KLS TODO
      if (!getAutoNormalizeBtni()->getset() ) {
         SoCamera* camera = p_viewer->getCamera() ;
         if ( camera != NULL ) {
            camera->viewAll(p_SG,p_viewer->getViewportRegion());
         }
      }
      */
   }
   getSG()->adjustAtomContainers();

}

/**
 * Set the builder mode.
 *
 * All the builder modes are defined int the header file.
 * All the mbCam* modes set the viewer in viewing mode.
 * All the other modes set the viewer in selecting mode.
 */
void Builder::setMode(int modeId)
{
  // @todo
  //  Pixel gray = EcceColorFactory::getColor(getBuilder(), Color::WINDOW);
  //  Pixel curMode= EcceColorFactory::getColor(getBuilder(), Color::CURRENTMODE);

  // Set the viewer modes.
  if (modeId == ID_MODE_ZOOM) {
    p_viewer->setStickyMode(SGViewer::DOLLY_MODE);
    p_currentMode = modeId;
    updateModeText();
    return;
  }

  if (modeId == ID_MODE_TRANSLATE) {
    p_viewer->setStickyMode(SGViewer::PAN_MODE);
    p_currentMode = modeId;
    updateModeText();
    return;
  }

  if (modeId == ID_MODE_ROTATE) {
    p_viewer->setStickyMode(SGViewer::VIEW_MODE);
    p_currentMode = modeId;
    updateModeText();
    return;
  }

  // Set the pick modes
  p_viewer->setStickyMode(SGViewer::PICK_MODE);
  

  // Load the cursor based on current mode
  // Defaut cursor image for select mode
  string cursorName = "cursor.xpm";
  if (modeId == ID_MODE_ATOM)
    cursorName = p_currentElement + "-" + 
      TBond::orderToString(p_currentBond) + ".xpm";
  else if (modeId == ID_MODE_BOND)
    cursorName = string(TBond::orderToString(p_currentBond)) + ".xpm";
  // @todo Add structlib cursor here
  else if (modeId == ID_MODE_STRUCTLIB)
    cursorName = "cursor.xpm";
  p_viewer->setCursor(cursorName);


  // clear selections if going from select to atom modes or when
  // switching between any two other modes.  If going from select to
  // a mode other than atom, the selection is to be retained.
  // It doesn't make sense to autoExecute the atom command.
  // Also don't execute if nothing is highlighted.  This is mainly
  // to save potential core dumps.
  bool autoExecute = false;
  if (p_currentMode == ID_MODE_SELECT  && modeId == ID_MODE_ATOM) {
    //cout << "why are we doing an unselect" << endl;
    Command * cmd = new UnselectCmd("UnselectCmd", getSG());
    execute(cmd);
  } else if (modeId != ID_MODE_ATOM
          && modeId != ID_MODE_STRUCTLIB
          && getSG()) {
    SGFragment *frag = getSG()->getFragment();
    if (frag && (frag->m_atomHighLight.size() > 0 ||
                 frag->m_bondHighLight.size() > 0)) {
      autoExecute = true;
    }
  }

  // If we are going into atom mode then lets not use the lasso.
  // This avoids the situation where you have to be so careful not
  // to move the mouse while inserting an atom.
  if (!p_viewer->getSelectModeDrag()) {
    if (modeId == ID_MODE_ATOM || modeId == ID_MODE_STRUCTLIB) {
      p_viewer->getSel()->lassoType.setValue(SGSelection::NOLASSO);
    } else {
      p_viewer->getSel()->lassoType.setValue(SGSelection::DRAGGER);
    }
  }

  // save new value for lastMode
  p_currentMode = modeId;
  updateModeText();

  //  instructions(s_modeCmd[mode]);

  // Now execute the command.  This is only down when ... (TODO
  // document)
  if (autoExecute && !p_viewer->isViewing()) {
    //    tu_busy_activate();
    try {
      // @todo This won't be working since createCommand also need params.
      Command * cmd = createModeCommand(modeId);
      execute(cmd);
    } catch (EcceException& ex) {
      reportError(ex.what());
    }

    // @todo Do we do this revertToSelect?
    // bool status = getCM()->execute(s_modeCmd[mode], (ICommandObject*)p_SG);
    // revertToSelect(status);

    //    tu_busy_deactivate();
  }
}


int Builder::getMode()
{
  return p_currentMode;
}


/**
 * The current build bond has changed. Reflect the change in the
 * toolbar.
 */
void Builder::updateBondIcon()
{
  string pixmap = TBond::orderToSmallImage(p_currentBond);

  ewxBitmap bitmap(pixmap.c_str());
  p_modeToolbar->SetToolNormalBitmap(ID_MODE_BOND, bitmap);
}



/**
 * The current build element has changed. Reflect the change in the
 * toolbar.
 */
void Builder::updateElementIcon()
{
  wxBitmap bitmap(22, 22);

  wxMemoryDC dc;
  dc.SelectObject(bitmap);

  TPerTab pertab;
  Preferences prefs("PerTable");
  string color;
  ewxColor col;

  if (prefs.getString(p_currentElement+".Color", color))
    col = ewxColor(color);
  else
    col = ewxColor(pertab.color(pertab.atomicNumber(p_currentElement)));

  dc.SetBackground(wxBrush(col, wxSOLID));
  dc.Clear();
  //dc.DrawCircle( 8, 8, 8);
  //dc.SetBrush( *wxWHITE_BRUSH );
  dc.SetPen(*wxBLACK_PEN);
  dc.SetFont(ewxStyledWindow::getBoldFont());
  wxSize size = dc.GetTextExtent(p_currentElement);
  dc.DrawText( p_currentElement, 11-size.GetX()/2, 11-size.GetY()/2);
  dc.SelectObject( wxNullBitmap );

  p_modeToolbar->SetToolNormalBitmap(ID_MODE_ATOM, bitmap);
}



/**
 * The current build shape has changed. Reflect the change in the
 * toolbar.
 */
void Builder::updateShapeIcon()
{
  string pixmap = ShapeData::shapeToSmallImage(ShapeData::stringToShape(p_currentShapes[p_currentElement]));
  
  ewxBitmap bitmap(pixmap.c_str());
  p_modeToolbar->SetToolNormalBitmap(ID_MODE_SHAPE, bitmap);
}


/**
 *
 */
void Builder::toggleModeButton(int modeId)
{
  if (modeId == ID_MODE_STRUCTLIB) {
    savePaneLayout(NAME_LAYOUT_DEFAULT);
    loadPaneLayout(NAME_LAYOUT_STRUCTLIB);
  }
  else if (p_mgr.GetPane(NAME_TOOL_STRUCTLIB).IsShown()) {
    savePaneLayout(NAME_LAYOUT_STRUCTLIB);
    loadPaneLayout(NAME_LAYOUT_DEFAULT);
  }

  // Restore tool menu status based on layout info
  for (int i=0; i<p_toolCount; ++i) {
    wxString label = p_toolMenu->GetLabel(ID_TOOLMENU_ITEM+i);
    p_toolMenu->Check(ID_TOOLMENU_ITEM+i, p_mgr.GetPane(label).IsShown());
  }
  // Restore toolbar menu status based on layout info
  for (int i = 0; i < p_toolbarCount; ++i) {
    wxString label = p_toolbarMenu->GetLabel(ID_TOOLBARMENU_ITEM+i);
    p_toolbarMenu->Check(ID_TOOLBARMENU_ITEM+i,
                         p_mgr.GetPane(label).IsShown());
  }

  p_viewer->actualRedraw();

  p_modeToolbar->ToggleTool(ID_MODE_SELECT, modeId == ID_MODE_SELECT);
  p_modeToolbar->ToggleTool(ID_MODE_ROTATE, modeId == ID_MODE_ROTATE);
  p_modeToolbar->ToggleTool(ID_MODE_TRANSLATE, modeId == ID_MODE_TRANSLATE);
  p_modeToolbar->ToggleTool(ID_MODE_ZOOM, modeId == ID_MODE_ZOOM);
  p_modeToolbar->ToggleTool(ID_MODE_ATOM, modeId == ID_MODE_ATOM);
  p_modeToolbar->ToggleTool(ID_MODE_BOND, modeId == ID_MODE_BOND);
  p_modeToolbar->ToggleTool(ID_MODE_STRUCTLIB, modeId == ID_MODE_STRUCTLIB);

  p_modeMenu->Check(modeId, true);
}


void Builder::saveSettings()
{
   ewxConfig * config = ewxConfig::getConfig("wxbuilder.ini");

   for (int i=0; i<p_toolCount; ++i) {
     wxString label = p_toolMenu->GetLabel(ID_TOOLMENU_ITEM+i);
     WxVizTool* viztool =dynamic_cast<WxVizTool*>(p_mgr.GetPane(label).window);
     if (viztool) {
       viztool->saveSettings(config);
     }
   }

   p_viewerEvtHandler->saveSettings(config);

   config->Write("/AutoResidue", GetMenuBar()->
                 IsChecked(ViewerEvtHandler::ID_AUTO_RESIDUE));
   config->Write("/Axis", getToolState(ViewerEvtHandler::ID_SHOW_AXES));
   config->Write("/Rings", getToolState(ViewerEvtHandler::ID_SHOW_RING));
   config->Write("/TextLabels", getToolState(ViewerEvtHandler::ID_CORNER_LABELS));
   config->Write("/Spin", getToolState(ViewerEvtHandler::ID_ENABLE_SPINNING));
   config->Write("/Fog", getToolState(ViewerEvtHandler::ID_DEPTH_CUEING));
   config->Write("/AutoCenter", isAutoCenter());
   config->Write("/OrthoCamera",getToolState(ViewerEvtHandler::ID_CAMERA_TYPE));
   config->Write("/ShowHydrogen", GetMenuBar()->
                 IsChecked(ViewerEvtHandler::ID_SHOW_HYDROGEN));
   config->Write("/ShowHydrogenBond", GetMenuBar()->
                 IsChecked(ViewerEvtHandler::ID_SHOW_HYDROGEN_BOND));
   config->Write("/ResidueLabels", GetMenuBar()->
                 IsChecked(ViewerEvtHandler::ID_RESIDUE_LABEL));
   config->Write("/BondLabels", GetMenuBar()->
                 IsChecked(ViewerEvtHandler::ID_BOND_LABEL));
   config->Write("/SelectModeDrag", isSelectModeDrag());
   
   if (getenv("ECCE_DEVELOPER")) {
     config->Write("/ShowCmd", p_toolMenu->IsChecked(ID_SHOW_CMD));
   }

   saveWindowSettings(config, true);

   // We don't allow struct lib mode across context switches
   if (p_currentMode == ID_MODE_STRUCTLIB) {
     setMode(ID_MODE_SELECT);
     toggleModeButton(ID_MODE_SELECT);
   }
 
   // Save the last perspective, if we have one
   if (p_calculation) {
     if (isReadOnly()) {
       savePaneLayout(NAME_LAYOUT_READONLY);
     } else {
       savePaneLayout(NAME_LAYOUT_DEFAULT);
     }
   }
}


void Builder::restoreSettings()
{
   ewxConfig * config = ewxConfig::getConfig("wxbuilder.ini");
   restoreWindowSettings(config, true);
   bool resetPerspective;

   wxString curversion = Ecce::ecceVersion();
   wxString version = config->Read("/Version", "None");
   if (version != curversion) {
      config->Read("/ResetPerspective", &resetPerspective, false);
      if (version!="None" &&  !resetPerspective) {
         // Don't warn user if they did the reset or preferences don't exist
         ewxMessageDialog mdlg(this, 
               "Window layout preferences are being reset due to\n"
               "the version upgrade.  New settings will be saved automatically.",
               "Reset Default Tools/Toolbars",wxOK|wxICON_EXCLAMATION,this->GetPosition());
         mdlg.ShowModal();
      }
      // Use existing infrastructure to delete all perspectives
      // by setting ResetPerspective to true
      config->Write("/ResetPerspective", true);
      config->Write("/Version", curversion);
   }


   // Clear the AUI layout perspective info if toggled to do so
   config->Read("/ResetPerspective", &resetPerspective, false);
   if (resetPerspective) {
     config->DeleteEntry("/Perspective");
     config->DeleteEntry("/ReadOnlyPerspective");
     config->DeleteEntry("/StructLibPerspective");
     config->DeleteGroup("/PaneLayout");
   }

   wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED);
   int buffer;
   map<string,wxWindowID> tmp;
   map<string,wxWindowID>::iterator it;

   // these options default to off/false
   tmp["/Axis"] = ViewerEvtHandler::ID_SHOW_AXES;
   tmp["/Rings"] = ViewerEvtHandler::ID_SHOW_RING;
   tmp["/Spin"] = ViewerEvtHandler::ID_ENABLE_SPINNING;
   tmp["/Fog"] = ViewerEvtHandler::ID_DEPTH_CUEING;
   tmp["/OrthoCamera"] = ViewerEvtHandler::ID_CAMERA_TYPE;
   tmp["/ShowHydrogenBond"] = ViewerEvtHandler::ID_SHOW_HYDROGEN_BOND;
   tmp["/ResidueLabels"] = ViewerEvtHandler::ID_RESIDUE_LABEL;
   tmp["/BondLabels"] = ViewerEvtHandler::ID_BOND_LABEL;

   for (it = tmp.begin(); it != tmp.end(); ++it) {
     config->Read(it->first, &buffer, 0);
     if (buffer) {
       evt.SetId(it->second);
       evt.SetInt(buffer);
       //p_viewerEvtHandler->AddPendingEvent(evt);
       p_viewerEvtHandler->ProcessEvent(evt);
     }
     GetMenuBar()->Check(it->second, buffer);
   }

   tmp.clear();
   // these options default to on/true
   tmp["/AutoCenter"] = ViewerEvtHandler::ID_AUTO_NORMALIZE;
   tmp["/TextLabels"] = ViewerEvtHandler::ID_CORNER_LABELS;
   tmp["/AutoResidue"] = ViewerEvtHandler::ID_AUTO_RESIDUE;
   tmp["/ShowHydrogen"] = ViewerEvtHandler::ID_SHOW_HYDROGEN;

   for (it = tmp.begin(); it != tmp.end(); ++it) {
     config->Read(it->first, &buffer, 1);
     if (buffer) {
       evt.SetId(it->second);
       evt.SetInt(buffer);
       //p_viewerEvtHandler->AddPendingEvent(evt);
       p_viewerEvtHandler->ProcessEvent(evt);
     }
     GetMenuBar()->Check(it->second, buffer);
   }

   // these are radio buttons
   tmp["/SelectModeDrag"] = ViewerEvtHandler::ID_SELECT_DRAG;
   config->Read("/SelectModeDrag", &buffer, 1);
   if (buffer)  {
     evt.SetId(ViewerEvtHandler::ID_SELECT_DRAG);
     GetMenuBar()->Check(ViewerEvtHandler::ID_SELECT_DRAG, true);
   } else {
     evt.SetId(ViewerEvtHandler::ID_SELECT_LASSO);
     GetMenuBar()->Check(ViewerEvtHandler::ID_SELECT_LASSO, true);
   }
   evt.SetInt(1);
   p_viewerEvtHandler->ProcessEvent(evt);

   if (getenv("ECCE_DEVELOPER")) {
     config->Read("/ShowCmd", &buffer, 0);
     evt.SetId(ID_SHOW_CMD);
     evt.SetInt(buffer);
     //AddPendingEvent(evt);
     GetEventHandler()->ProcessEvent(evt);
     p_toolMenu->Check(ID_SHOW_CMD, buffer);
   }
   
   int i;
   for (i=0; i<p_toolCount; ++i) {
     wxString label = p_toolMenu->GetLabel(ID_TOOLMENU_ITEM+i);
     WxVizTool* viztool =dynamic_cast<WxVizTool*>(p_mgr.GetPane(label).window);
     if (viztool) {
       viztool->restoreSettings(config);
     }
   }
}



void Builder::applySceneGraphSettings()
{
  vector<wxWindowID> tmp;
  tmp.push_back(ViewerEvtHandler::ID_SHOW_HYDROGEN);
  tmp.push_back(ViewerEvtHandler::ID_SHOW_HYDROGEN_BOND);
  tmp.push_back(ViewerEvtHandler::ID_BOND_LABEL);
  tmp.push_back(ViewerEvtHandler::ID_RESIDUE_LABEL);
  tmp.push_back(ViewerEvtHandler::ID_DEPTH_CUEING);
  tmp.push_back(ViewerEvtHandler::ID_SHOW_RING);

  for (vector<wxWindowID>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED);
    evt.SetId(*it);
    evt.SetInt(GetMenuBar()->IsChecked(*it));
    //p_viewerEvtHandler->AddPendingEvent(evt);
    p_viewerEvtHandler->ProcessEvent(evt);
  }
}



/**
 * Create a new context and open it.
 */
void Builder::OnNewClick( wxCommandEvent& event )
{
  setContext(DefaultCalculation::nextURL());
}



/**
 * Import a fragment.
 */
void Builder::OnOpenClick( wxCommandEvent& event )
{
  OpenCalculationDialog dialog(this);
  if (dialog.ShowModal() == wxID_CANCEL) return;

  string path = dialog.GetPath().c_str();
  EcceURL url(path);
  if (CalculationFactory::canOpen(url)) {
    setContext(url);

  } else {
    string msg = "Cannot open " + url.toString() + " as a new context (for "
                 "viewing output properties from a calculation, you must "
                 "use the Import Calculation menu option).";
    wxLogWarning(msg.c_str(), 0);
  }
}


void Builder::OnImportChemsysClick(wxCommandEvent& event)
{
  ImportCalculationDialog dialog(this);
  if (dialog.ShowModal() == wxID_CANCEL) return;

  // create temporary file for fragment reading
  SFile *file = TempStorage::getTempFile();

  string path = dialog.GetPath().c_str();
  EcceURL url(path);
  wxString type = dialog.getType();
  wxString ext = dialog.getExt().MakeUpper();
  Resource *resource = EDSIFactory::getResource(url);
  ChemistryTask *task;

  // try to open as a ChemistryTask first
  if ((task = dynamic_cast<ChemistryTask*>(resource))) {
    file->move(file->pathroot() + "/" + resource->getName() + ".mvm");
    ofstream os(file->path().c_str());
    Fragment *frag = task->fragment();
    if (frag) {
      frag->dumpMVM(os);
      delete frag;
      os << ends;
      ext = "MVM";
    } else {
      wxLogError("Could not import %s", url.toString().c_str());
      return;
    }
  } else {
    file = resource->getDocument(file);
    file->move(file->pathroot() + "/" + resource->getName());
  }

  // now do the real work
  if (!readFragmentFromFile(file, type, ext)) {
    wxLogError("Could not import %s", url.toString().c_str());
    return;
  }

  file->remove();
  delete file;

  //---------------
  // KLS - copied from the end of setContext to update the UI
  // Need to revisit import vs open
  updateUndoMenus();
  updateSave();
  updateResource();
  updateReadOnly(true);
  updatePropertyMenus();
  updateMenus();
  updateViewerText();

  // Refresh Tool panels
  EventDispatcher::getDispatcher().publish(Event("UpdateUI"));
  // Also add centering
  getViewer().viewAll();
  //---------------

  // GDB added 6/15/11 to finish off ECCE 6.1 changes
  PBC *pbc=dynamic_cast<PBC*>(p_mgr.GetPane(NAME_TOOL_PBC).window);
  if (pbc) {
    pbc->updateSpaceGroup(); 
  }
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_IMPORT
 */
void Builder::OnImportCalcClick( wxCommandEvent& event )
{
  WxCalcImport *dlg = new WxCalcImport(this);
  dlg->registerListener(this);
  dlg->importCalc();
}


CommandManager&  Builder::getCommandManager()
{
   return *(getCM());
}

SGContainer&   Builder::getSceneGraph()
{
   return *(getSG());
}

SGViewer&   Builder::getViewer()
{
   return *p_viewer;
}

SGContainer* Builder::getSG()
{
  return p_sgMgr->getSceneGraph();
}

CommandManager* Builder::getCM()
{
  return p_commandManager;
}

void Builder::showMessage(const string& msg, bool error )
{
  if (error) {
    wxLogError(msg.c_str(), 0);
  } else {
    wxLogWarning(msg.c_str(), 0);
  }
}


void Builder::center()
{
   Command *cmd = new CenterViewCmd("CenterView", p_viewer);
   cmd->execute();
   delete cmd;
}

bool Builder::isAutoCenter() const
{
  return getToolState(ViewerEvtHandler::ID_AUTO_NORMALIZE);
}


bool Builder::isSelectModeDrag() const
{
  return getToolState(ViewerEvtHandler::ID_SELECT_DRAG);
}


/**
 * Execute the specified command and do standard processing
 * we want to do after every command.  This includes all the code
 * from the old builder afterCommand method.
 */
bool Builder::execute(Command *cmd, bool batch)
{
   bool didit = false;
   // Skip empty command
   if (!cmd)
     return false;

   //////////////////cout << "EXECUTE: " << *cmd << endl;
   try {
      int startNumAtoms = -1;
      // KLS 12/08 what the ???
      //if (getSG() && getSG()->getFragment())
      //  getSG()->getFragment()->numAtoms();

      if (handleCommandsIfLattice(cmd)) {

         if (cmd->execute() && cmd->isUndoable()) {
            didit = true;
            getCM()->append(cmd);
            updateUndoMenus();
         } else {
            // I guess false means that the command did nothing
            // See if there is an error message for backwards compatability
            string msg = cmd->getErrorMessage();
            if (msg != "") reportError(msg);
         }

         // Make camera view all after execute.
         if (!batch) {
            if (isAutoCenter() || 
                  (startNumAtoms == 0 && getSG()->getFragment()->numAtoms() > 0)) {
               center();
            }
         }
      }
   } catch (EcceException& ex) {
       reportError(ex.what());
   }

   // @todo Lisong Hack: Somehow the viewer selection got lost after
   // cmd operations like translation. The p_atomhighlightt is still there
   // which caused sync problem. For now just reselect whatever in
   // p_atomhighlight and p_bondhighlight. Should fix it when it became
   // a performance problem.
   //   Event event("SelectionChanged");
   //   EventDispatcher::getDispatcher().publish(event);

   if (!batch) {
      SGFragment *frag = NULL;
      if (getSG() && (frag = getSG()->getFragment()))
         setSelection(frag->m_atomHighLight, frag->m_bondHighLight);
   }
   return didit;
}

bool Builder::handleCommandsIfLattice(Command *cmd)
{
   bool ret = true;  // all clear
   LatticeDef *lattice = getSG()->getFragment()->getLattice();
   if (lattice) {
      // See PBC.C for information on this design injection
      // Most commands can't work on replicated system.
      // The list that can is hardwired here.
      int na1, na2, na3;
      lattice->getReplicationFactors(na1, na2, na3);
      if (na1 > 1 || na2 > 1 || na3 > 1) {
         PBCCmd *pbcclass = dynamic_cast<PBCCmd*>(cmd);
         FragCloneCmd *fragclass =dynamic_cast<FragCloneCmd*>(cmd);
         if (pbcclass == 0 && fragclass) {
            NewFragmentCmd *nfrag=dynamic_cast<NewFragmentCmd*>(cmd);
            if (!nfrag) {

               long flags = wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_QUESTION;
               ewxMessageDialog dlg(this, 
                     "This command cannot be executed on a replicated system. "
                     "You can choose to revert to a UnitCell or convert "
                     "to the SuperCell and proceed or cancel.",
                     //"Do you want to restore to the unit cell and then "
                     "Remove replication?", flags);
               //"Do you want to restore to the unit cell and then "
               //"execute this command?",
               //"Restore to unit cell?", flags);
               wxButton *btn = (wxButton*)dlg.FindWindow(wxID_YES);
               btn->SetLabel("UnitCell");
               btn = (wxButton*)dlg.FindWindow(wxID_NO);
               btn->SetLabel("SuperLattice");
               int status = dlg.ShowModal();
               if (status == wxID_YES) {
                  PBC *pbc=dynamic_cast<PBC*>(p_mgr.GetPane(NAME_TOOL_PBC).window);
                  if (pbc) {
                     Command *tmpcmd = pbc->restoreUnitCell(); 
                     getCM()->append(tmpcmd);
                     updateUndoMenus();
                     ret = true;
                  }
               } else if (status == wxID_NO) {
                  PBC *pbc=dynamic_cast<PBC*>(p_mgr.GetPane(NAME_TOOL_PBC).window);
                  if (pbc) {
                     pbc->makeSuperLattice(); 
                     ret = true;
                  }
               } else if (status == wxID_CANCEL) {
                  ret = false;
               }
            }
         }
      }
   }
   return ret;
}


/** 
 * Updates the undo and redo menu items.
 */
void Builder::updateUndoMenus()
{
  wxMenuItem *undo = GetMenuBar()->FindItem(wxID_UNDO,0);
  if (getCM()->isUndoable()) {
     string undoLabel = "Undo ";
     undoLabel += getCM()->getUndoLabel();
     undo->SetText(undoLabel+"\tCtrl+z");
     undo->Enable(true);
     p_fileToolbar->EnableTool(wxID_UNDO, true);
     p_fileToolbar->SetToolShortHelp(wxID_UNDO, undoLabel);
  } else {
     string undoLabel = "Can\'t Undo\tCtrl+z";
     undo->SetText(undoLabel.c_str());
     undo->Enable(false);
     p_fileToolbar->EnableTool(wxID_UNDO, false);
     p_fileToolbar->SetToolShortHelp(wxID_UNDO, "Can\'t Undo");
  }

  wxMenuItem *redo = GetMenuBar()->FindItem(wxID_REDO,0);
  if (getCM()->isRedoable()) {
    redo->Enable(true);
    string redoLabel = "Redo ";
    redoLabel += getCM()->getRedoLabel();
    redo->SetText(redoLabel+"\tCtrl+y");
    p_fileToolbar->EnableTool(wxID_REDO, true);
    p_fileToolbar->SetToolShortHelp(wxID_REDO, redoLabel);

  } else {
    redo->SetText("Can\'t Redo\tCtrl+y");
    redo->Enable(false);
    p_fileToolbar->EnableTool(wxID_REDO, false);
    p_fileToolbar->SetToolShortHelp(wxID_REDO, "Can\'t Redo");
  }
}


/**
 * Process user Quit menu item.
 */
void Builder::quitMenuItemClickCB( wxCommandEvent& event )
{
   saveSettings();
   quit();
}


/**
 * Process undo
 */
void Builder::OnUndoClick( wxCommandEvent& event )
{
   getCM()->undo();

   // Make camera view all after execute.
   if (isAutoCenter()) {
      center();
   }
   updateUndoMenus();

   // @todo Resotore the previous selection no matter it is needed or not
   // Should fix it so that it is called only when needed.
   SGFragment *frag = getSG()->getFragment();
   setSelection(frag->m_atomHighLight, frag->m_bondHighLight);
}

/**
 * Process redo
 */
void Builder::OnRedoClick( wxCommandEvent& event )
{

   try {
      getCM()->redo();
      // Make camera view all after execute.
      if (isAutoCenter()) {
         center();
      }

      updateUndoMenus();

   } catch (EcceException& ex) {
      wxLogWarning(ex.what(), 0);
   }

}

void Builder::helpSupportMenuitemClick( wxCommandEvent& event )
{
    BrowserHelp help;
    help.showFeedbackPage();

}

void Builder::helpBuilderMenuitemClick( wxCommandEvent& event )
{
   BrowserHelp help;
   help.showPage(help.URL("WxBuilder"));

}


/**
 * Process frame resize.
 */
void Builder::OnSize( wxSizeEvent& event )
{
   BuilderGUI::OnSize(event);
}


void Builder::mainWindowCloseCB( wxCloseEvent& event )
{
   saveSettings();
   quit();
}


/**
 * Clear the entire fragment to start over.
 */
void Builder::OnClearClick( wxCommandEvent& event )
{
   wxBusyCursor busy;

   Command *cmd = new NewFragmentCmd("Clear", getSG());
   execute(cmd);
}


/**
 * Create a copy buffer and remove selected items.
 */
void Builder::OnCutClick( wxCommandEvent& event )
{
   wxBusyCursor busy;

   SGFragment *frag = getSG()->getFragment();

   // Do the copy
   p_copyBuffer = frag->clipFragment(frag->m_atomHighLight);

   deleteSelection();

}


/**
 * Create a copy buffer.
 */
void Builder::OnCopyClick( wxCommandEvent& event )
{
   wxBusyCursor busy;

   SGFragment *frag = getSG()->getFragment();

   // Delete old one if it exists
   if (p_copyBuffer)  delete p_copyBuffer; 

   p_copyBuffer = frag->clipFragment(frag->m_atomHighLight);
}


/**
 * Paste from the copy buffer.
 */
void Builder::OnPasteClick( wxCommandEvent& event )
{
   if (p_copyBuffer) {
      wxBusyCursor busy;
      ostringstream os;
      p_copyBuffer->dumpMVM(os,true);
      os << ends;

      Command *cmd = new AddFragmentCmd("Paste", getSG());
      cmd->getParameter("streamType")->setString("MVM");
      cmd->getParameter("mvmStream")->setString(os.str());
      cmd->getParameter("genBondOrders")->setBoolean(false);
      execute(cmd);
   }
}


void Builder::OnDeleteClick( wxCommandEvent& event )
{
   deleteSelection();
}



/**
 * @todo This function should also update the GUI of Builder to reflect
 * the change.
 */
void Builder::eventMCB(const Event& event)
{
   string name = event.getName();
   string value = event.getValue();

   SGFragment *frag = getSG()->getFragment();

   if (name == "ResidueChange" || name == "GeomChange") {
     setDirty(true);
     updateForAnyEdit();
   }
   else if (name == "StepChange") {
     updateForAnyEdit();
   }
   else if (name == "SelectionChanged") {
      setSelection(frag->m_atomHighLight, frag->m_bondHighLight);
   }
   else if (name == "ElementChanged") {
     // @todo Update the elt button, shape button and
     // unset the viewer state buttons
     p_currentElement = value;
     updateElementIcon();
     Event evt("ShapeChanged", p_currentShapes[p_currentElement]);
     EventDispatcher::getDispatcher().publish(evt);
   }
   else if (name == "ElementColorChanged") {
     pertabPrefsMCB();
   }
   else if (name == "BondChanged") {
     p_currentBond = TBond::strToOrder(value);
     setMode(ID_MODE_BOND);
     updateBondIcon();
     toggleModeButton(ID_MODE_BOND);
   }
   else if (name == "ShapeChanged") {
     p_currentShapes[p_currentElement] = value;
     setMode(ID_MODE_ATOM);
     updateShapeIcon();
     toggleModeButton(ID_MODE_ATOM);
   }
   else if (name == "SyncToggle") {
     wxCommandEvent * evt = (wxCommandEvent *)event.getObject();
     int id = evt->GetId();
     bool isChecked = evt->IsChecked();
     toggleTool(id, isChecked);
     wxMenuItem * menuItem = GetMenuBar()->FindItem(id);
     if (menuItem)
       menuItem->Check(isChecked);
   }
   else if (name == "AddDragger") {
     SoDragger * dragger = (SoDragger *) event.getObject();
     dragger->addMotionCallback(Builder::moveCB, (void *) this);
     dragger->addStartCallback(Builder::moveStartCB, (void *) this);
     dragger->addFinishCallback(Builder::moveEndCB, (void *) this);
   }
   else if (name == "AddWheel") {
     SoDragger * dragger = (SoDragger *) event.getObject();
     dragger->addMotionCallback(Builder::rotateCB, (void *) this);
     dragger->addStartCallback(Builder::rotateStartCB, (void *) this);
     dragger->addFinishCallback(Builder::rotateEndCB, (void *) this);
   }

   updateSelectionText();
   //TODO refresh GUI
   updateSave();
}


/**
 * Updates everything in the UI for any kind of edit.
 * Note that some "edits" aren't user edits such as recomputing bonds
 * during visualization.
 */
void Builder::updateForAnyEdit()
{
   SGFragment *frag = getSG()->getFragment();

   updateAtomResidueText();

   getSG()->adjustAtomContainers();
   if (getToolState(ViewerEvtHandler::ID_SHOW_HYDROGEN_BOND)) {
      WxVizToolFW& fw = getFW();
      Command* cmd=new HydrogenBondsCmd("Show Hydrogen Bond",&fw.getSceneGraph());
      fw.execute(cmd);
   }

   if (frag->numResidues() &&
         GetMenuBar()->IsChecked(ViewerEvtHandler::ID_AUTO_RESIDUE) &&
         !isReadOnly() &&
         !p_mgr.GetPane("Residue Table").IsShown()) {
      p_mgr.GetPane("Residue Table").Show(true).Open(true);

      // If its a viz tool, call virtual refresh to force updating
      // Works around bug where we can't override show because it gets
      // called too much by AUI
      WxVizTool *panel = 
         dynamic_cast<WxVizTool*>(p_mgr.GetPane("Residue Table").window);
      if (panel) {
         panel->refresh();
      }

      p_mgr.Update();
      GetMenuBar()->Check(GetMenuBar()->FindMenuItem("Tools", "Residue Table"), true);
   }

   // If we went from residues to no residues, the current 
   // display style might be invalid and need to be fixed.
   if (frag->numResidues() == 0) {
      if (getSG()->proteinStyleExists()) {
         // Hardwired because we no longer have a "default" style
         wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, 
               ViewerEvtHandler::ID_STYLE_BALL_WIREFRAME);
         //p_viewerEvtHandler->AddPendingEvent(evt);
         p_viewerEvtHandler->ProcessEvent(evt);
      }
   }
   getSG()->updateColorNodes();
}


void Builder::updateModeText()
{
  string modeTxt = "Mode: ";
  modeTxt += s_modeText[p_currentMode-ID_MODE_SELECT];
  Residue* res ;

  // add the residue edit information, if applicable
  if (p_currentMode == ID_MODE_ATOM || p_currentMode == ID_MODE_STRUCTLIB) {
    int resIndex = getSG()->getFragment()->editResidueIndex();
    if (resIndex != -1) {
      char buf[32];
      res = getSG()->getFragment()->findResidue(resIndex);
      sprintf(buf, "%s%s%s%d",
              ", ", (res->name()).c_str(), "-", resIndex+1);
      modeTxt.append(buf) ;
    }
  }
  p_viewer->setULeftText(modeTxt);
}


void Builder::updateAtomResidueText()
{
  SGFragment *frag = getSG()->getFragment();
  char buf[80];
  strcpy(buf,"");

  // NOTE counting non-nubs much more costly than counting atoms
  if (frag->numAtoms() > 0) {
    if (frag->numResidues() > 0) {
      sprintf(buf,"%zu atoms %zu residues",
              frag->numNonNubs(), frag->numResidues());
    } else if (frag->pointGroup() != "C1") {
      sprintf(buf,"%zu atoms, %s",
              frag->numNonNubs(), frag->pointGroup().c_str());
    } else {
      sprintf(buf,"%zu atoms", frag->numNonNubs());
    }
  }
  p_viewer->setLLeftText(buf);
}


void Builder::updateSelectionText()
{
  // Update the selection label info
  SGFragment *frag = getSG()->getFragment();
  char buf[80];
  strcpy(buf,"");

  if (frag->m_atomHighLight.size() == 1) {
    TAtm *atm = frag->atomRef(frag->m_atomHighLight[0]);
    if (frag->numResidues() == 0) {
      sprintf(buf,"%s%d %.2f,%.2f,%.2f",
              atm->atomicSymbol().c_str(), atm->index(),
              atm->coordinates()[0], atm->coordinates()[1],
              atm->coordinates()[2]);
    } else {
      sprintf(buf,"%s %s %s",
              atm->atomicSymbol().c_str(), atm->residueName().c_str(),
              atm->atomName().c_str());
    }
  } else if (frag->m_atomHighLight.size() > 0) {
    // Could get set of selected residues and if 1 report on it
    int nonnubs = 0;
    int numHighlights = frag->m_atomHighLight.size();
    for (int idx=0; idx<numHighlights; idx++) {
      if (frag->atomRef(frag->m_atomHighLight[idx])->atomicSymbol() != "Nub")
        nonnubs++;
    }
    sprintf(buf,"%d atoms selected",nonnubs);
  }
  p_viewer->setLRightText(buf);
}


void Builder::updateViewerText()
{
  updateModeText();
  updateAtomResidueText();
  updateSelectionText();
}


/**
 * Selects all atoms and bonds.
 */
void Builder::OnSelectallClick( wxCommandEvent& event )
{
   Command *cmd = new SelectAllCmd("Select All", getSG());
   execute(cmd);
}

/**
 * Select molecule if one atom already selected.
 */
void Builder::OnSelectMoleculeClick( wxCommandEvent& event )
{
   Command *cmd = new SelectCmd("Select Molecule", getSG());
   cmd->getParameter("molecule")->setBoolean(true);
   execute(cmd);
}


/**
 * Select solvent atoms if present.
 */
void Builder::OnSelectSolventClick( wxCommandEvent& event )
{
   Command *cmd = new SelectCmd("Select Solvent", getSG());
   cmd->getParameter("solvent")->setBoolean(true);
   execute(cmd);
}


/**
 * Select backbone atoms of recognized biological structure (chain of
 * amino acids)
 */
void Builder::OnSelectBackboneClick( wxCommandEvent& event )
{
   Command *cmd = new SelectCmd("Select Backbone", getSG());
   cmd->getParameter("backbone")->setBoolean(true);
   execute(cmd);
}


/**
 * Select sidechain atoms of recognized biological structure (chain of
 * amino acids)
 */
void Builder::OnSelectSidechainsClick( wxCommandEvent& event )
{
   Command *cmd = new SelectCmd("Select Sidechains", getSG());
   cmd->getParameter("sidechains")->setBoolean(true);
   execute(cmd);
}


/**
 * Reverse selection so that all unselected atoms are selected.
 */
void Builder::OnSelectReverseClick( wxCommandEvent& event )
{
   Command *cmd = new SelectCmd("Reverse Selection", getSG());
   cmd->getParameter("reverse")->setBoolean(true);
   execute(cmd);
}


/**
 * Clear selection
 */
void Builder::OnUnselectClick( wxCommandEvent& event )
{
   Command *cmd = new SelectCmd("Clear Selection", getSG());
   cmd->getParameter("clear")->setBoolean(true);
   execute(cmd);
}


/*!
 * Change to previous mode
 */
void Builder::OnModePrevClick( wxCommandEvent& event )
{
  int newmode = p_currentMode - 1;
  if (newmode < ID_MODE_SELECT) {
    newmode = isReadOnly() ? ID_MODE_ZOOM : ID_MODE_STRUCTLIB;
  }
  wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, newmode);
  //AddPendingEvent(evt);
  GetEventHandler()->ProcessEvent(evt);
  event.Skip();
}


/*!
 * Change to next mode
 */
void Builder::OnModeNextClick( wxCommandEvent& event )
{
  int newmode = p_currentMode + 1;
  if (newmode > ID_MODE_STRUCTLIB || (isReadOnly() && newmode > ID_MODE_ZOOM))
    newmode = ID_MODE_SELECT;
  wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, newmode);
  //AddPendingEvent(evt);
  GetEventHandler()->ProcessEvent(evt);
  event.Skip();
}


void Builder::OnTextctrlCmdEnter( wxCommandEvent& event )
{
  string input = p_cmd->GetValue().c_str();
  try {
    Command * cmd = createCommand(input);
    execute(cmd);
  } catch (EcceException& ex) {
    reportError(ex.what());
    
  }
  event.Skip();
}


/*
void Builder::OnTextctrlCmdUpdated( wxCommandEvent& event )
{
   string input = p_cmd->GetValue().c_str();
   if (input[input.size()-1] == '=') {
      input = input.erase(input.size()-1);
      try {
         Command * cmd = createCommand(input);
         execute(cmd);

      } catch (EcceException& ex) {
         reportError(ex.what());

      }
   }
}
*/


/**
 * Rotate the camera to a specific orientation.
 * Values in range -180 to 180
 */
void Builder::rotateTo(float x, float y, float z)
{

  // This code dumps the current rotation matrix
  SbMatrix mx;
  mx = p_viewer->getCamera()->orientation.getValue();

  //cout << "TO: x,y,z" << x << "," << y << "," << z << endl;
  //cout << "X"  << mx[0][0] << " " << mx[0][1] << " " << mx[0][2] << " " << mx[0][3] << endl;
  //cout << "Y"  << mx[1][0] << " " << mx[1][1] << " " << mx[1][2] << " " << mx[1][3] << endl;
  //cout << "Z"  << mx[2][0] << " " << mx[2][1] << " " << mx[2][2] << " " << mx[2][3] << endl;
  //cout << "4 " << mx[3][0] << " " << mx[3][1] << " " << mx[3][2] << " " << mx[3][3] << endl;

  // First rotate back to the origin so we can then rotate to our new view.
  // inverse() provides this starting orientation
  SbRotation origin(mx.inverse());
  p_viewer->rotateCamera(origin);

  mx = p_viewer->getCamera()->orientation.getValue();

  // Bruce put your code here to generate a new matrix from teh values
  // The ui values for x,y,z are;
  // theta: elevation
  // phi: azimuth
  // psi: twist
  double theta = x / 180. * M_PI;
  double phi = y / 180. * M_PI;
  double psi = z / 180. * M_PI;
  double costh = cos(theta);
  double sinth = sin(theta);
  double sinphi, cosphi;
  double sinpsi, cospsi;

  //cout << "theta: "<<theta*180.0/M_PI<<endl;
  //cout << "phi: "<<phi*180.0/M_PI<<endl;
  //cout << "psi: "<<psi*180.0/M_PI<<endl;

  costh = cos(theta);
  sinth = sin(theta);
  cospsi = cos(psi);
  sinpsi = sin(psi);
  cosphi = cos(phi);
  sinphi = sin(phi);
  //
  //Then set the matrix
  SbMatrix mo;
  mo[0][0] = cosphi*cospsi-sinphi*sinpsi*sinth;
  mo[0][1] = costh*sinphi;
  mo[0][2] = cosphi*sinpsi+cospsi*sinphi*sinth;
  mo[0][3] = 0.0;
  mo[1][0] = -cospsi*sinphi-cosphi*sinpsi*sinth;
  mo[1][1] = cosphi*costh;
  mo[1][2] = -sinphi*sinpsi+cosphi*cospsi*sinth;
  mo[1][3] = 0.0;
  mo[2][0] = -costh*sinpsi;
  mo[2][1] = -sinth;
  mo[2][2] = cospsi*costh;
  mo[2][3] = 0.0;
  mo[3][0] = 0.0;
  mo[3][1] = 0.0;
  mo[3][2] = 0.0;
  mo[3][3] = 1.0;
  
  /*
  char mbuf[256];
  sprintf(mbuf,"m00: %12.6f m01: %12.6f m02: %12.6f m03: %12.6f\n",mo[0][0],mo[0][1],mo[0][2],mo[0][3]);
  cout<<mbuf<<endl;
  sprintf(mbuf,"m10: %12.6f m11: %12.6f m12: %12.6f m13: %12.6f\n",mo[1][0],mo[1][1],mo[1][2],mo[1][3]);
  cout<<mbuf<<endl;
  sprintf(mbuf,"m20: %12.6f m21: %12.6f m22: %12.6f m23: %12.6f\n",mo[2][0],mo[2][1],mo[2][2],mo[2][3]);
  cout<<mbuf<<endl;
  sprintf(mbuf,"m30: %12.6f m31: %12.6f m32: %12.6f m33: %12.6f\n",mo[3][0],mo[3][1],mo[3][2],mo[3][3]);
  cout<<mbuf<<endl;
  */


  // Now rotate to the new view
  //SbRotation r(mo);
  p_viewer->rotateCamera(SbRotation(mo));



}

void Builder::OnRotX( wxSpinEvent& event )
{
  // This code just increases the x rotation by the delta of the x value
// REAL CODE
//  SbRotation rot(SbVec3f(1, 0, 0), (event.GetPosition()-p_rotx)*M_PI/180.0);
//  p_viewer->rotateCamera(rot);
  p_rotx = event.GetPosition();
  rotateTo(p_rotx, p_roty, p_rotz);

  event.Skip();
}


void Builder::OnRotY( wxSpinEvent& event )
{
  //SbRotation rot(SbVec3f(0, 1, 0), (event.GetPosition()-p_roty)*M_PI/180.0);
  //p_viewer->rotateCamera(rot);
  p_roty = event.GetPosition();
  rotateTo(p_rotx, p_roty, p_rotz);
  event.Skip();
}


void Builder::OnRotZ( wxSpinEvent& event )
{
  //SbRotation rot(SbVec3f(0, 0, 1), (event.GetPosition()-p_rotz)*M_PI/180.0);
  //p_viewer->rotateCamera(rot);
  p_rotz = event.GetPosition();
  rotateTo(p_rotx, p_roty, p_rotz);
  event.Skip();
}


void Builder::OnViewerChoice( wxCommandEvent& event )
{
  SGContainer *sg = p_sgMgr->getSceneGraph();
  sg->ref();
  SGViewer *viewer = new SGViewer(this, wxID_ANY);
  viewer->setSceneGraph(sg);
  ewxAuiPaneInfo pinfo;
  pinfo.Name(p_calculation->getURL().toString()).
          Caption(p_calculation->getURL().toString()).
          PinButton(true).MaximizeButton(true).MinimizeButton(true).
          Left().Show(true);
  p_mgr.AddPane(viewer, pinfo);
  p_mgr.Update();
}


void Builder::OnModeClick( wxCommandEvent& event )
{
  setMode(event.GetId());
  toggleModeButton(event.GetId());
  event.Skip();
}


void Builder::OnModeElementClick( wxCommandEvent& event )
{
  if (getToolState(ID_MODE_ATOM) && !event.IsChecked()) {
    PerTabPanel *pertab = new PerTabPanel(this, false,
            ID_ITEM_DEFAULT, false, true, true);
    pertab->SetName("Elements");
    TearableContent *tc = new TearableContent(pertab);
    tc->Position(wxGetMousePosition(), wxSize(1, 1));
    tc->Popup();
    toggleModeButton(event.GetId());
  }
  else {
    Event evt("ElementChanged", p_currentElement);
    EventDispatcher::getDispatcher().publish(evt);
  }
}


void Builder::OnModeShapeClick( wxCommandEvent& event )
{
   ShapeDropDown *l = new ShapeDropDown(
           ShapeData::stringToShape(p_currentShapes[p_currentElement]), this);
   TearableContent *tc = new TearableContent(l);
   l->connectToolKitFW(this);
   tc->Position(wxGetMousePosition(), wxSize(1, 1));
   tc->Popup();
}


void Builder::OnModeBondClick( wxCommandEvent& event )
{
  if (getToolState(ID_MODE_BOND) && !event.IsChecked()) {
    BondDropDown *l = new BondDropDown(p_currentBond, this);
    TearableContent *tc = new TearableContent(l);
    l->connectToolKitFW(this);
    tc->Position(wxGetMousePosition(), wxSize(1, 1));
    tc->Popup();
    toggleModeButton(event.GetId());
  }
  else {
    Event evt("BondChanged", TBond::orderToString(p_currentBond));
    EventDispatcher::getDispatcher().publish(evt);
  }
}


void Builder::OnContextRadioClick( wxCommandEvent& event )
{
  if (p_contextHistory->SetContext(event.GetId())) {
    setContext(p_contextHistory->GetContext().c_str());
  }
}


void Builder::OnContextListClick( wxListEvent& event )
{
  setContext(event.GetText().c_str());
}


void Builder::OnPropertyMenuClick( wxCommandEvent& event )
{
  wxString name = p_propertyMenu->GetLabelText(event.GetId());
  ewxAuiPaneInfo &pane = p_mgr.GetPane(name);
  pane.Show(event.IsChecked());
  // If caption panel is toggled closed, open it by default
  if (event.IsChecked() && !pane.IsOpen()) {
    pane.Open(true);
    pane.Position(0);
  }
  p_mgr.Update();
}


/**
 * Process special keyboard events like delete, esc and whatever else we
 * come up with.
 */
void Builder::OnKeyDown(wxKeyEvent& event)
{
   if (event.GetKeyCode() == WXK_DELETE ||
       event.GetKeyCode() == WXK_BACK ||
       event.GetKeyCode() == WXK_NUMPAD_DELETE) {
      deleteSelection();
   } else if (event.GetKeyCode() == WXK_ESCAPE) {
      setMode(ID_MODE_SELECT);
   }
   event.Skip();
}


/**
 * Provide menu of centering options.
 * The callback is currently handled by the generic event
 *    EVT_RADIOBOX( wxID_ANY, Builder::OnRadioClick)
 */
void Builder::OnToolStyleClick( wxCommandEvent& event )
{
   StyleDropDown *dd = new StyleDropDown(this);
   TearableContent *tc = new TearableContent(dd);
   dd->connectToolKitFW(this);
   tc->Position(wxGetMousePosition(), wxSize(1, 1));
   tc->Popup();
}



void Builder::OnToolTranslatemClick( wxCommandEvent& event )
{
   TranslatePanel *tp = new TranslatePanel(this);
   TearableContent *tc = new TearableContent(tp);
   tp->SetName("Translate Coordinates");
   tp->connectToolKitFW(this);
   tc->Position(wxGetMousePosition(), wxSize(1, 1));
   tc->Popup();
}



void Builder::OnToolGohomeClick( wxCommandEvent& event )
{
  p_xrot->SetValue(0);
  p_yrot->SetValue(0);
  p_zrot->SetValue(0);
  Command *cmd = new GoHomeCmd("Go Home", p_viewer);
  execute(cmd);
}

void Builder::OnToolSethomeClick( wxCommandEvent& event )
{
  p_xrot->SetValue(0);
  p_yrot->SetValue(0);
  p_zrot->SetValue(0);
  Command *cmd = new SetHomeCmd("Set Home", p_viewer);
  execute(cmd);
}


void Builder::OnToolOrientClick( wxCommandEvent& event )
{
}


void Builder::OnToolMenuClick( wxCommandEvent& event )
{
  ewxAuiPaneInfo &pane = p_mgr.GetPane(GetMenuBar()->GetLabel(event.GetId()));
  pane.Show(event.IsChecked());
  // If caption panel is toggled closed, open it by default
  if (event.IsChecked() && !pane.IsOpen()) {
    pane.Open(true);
    pane.Position(0);
  }
  // If its a viz tool, call virtual refresh to force updating
  // Works around bug where we can't override show because it gets
  // called too much by AUI
  WxVizTool *panel = dynamic_cast<WxVizTool*>(pane.window);
  if (panel && event.IsChecked()) {
    panel->refresh();
  }
  p_mgr.Update();


  event.Skip();
}


void Builder::OnPaneClose(ewxAuiManagerEvent& event)
{
  if (event.pane->name == NAME_TOOL_STRUCTLIB) {
    setMode(ID_MODE_SELECT);
    toggleModeButton(ID_MODE_SELECT);
    //wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_MODE_SELECT);
    //AddPendingEvent(event);
  } else {
    wxString name(event.pane->name);
    wxWindow *win = event.pane->window;
    int tool_id = p_toolMenu->FindItem(name);
    int prop_id = p_propertyMenu->FindItem(name);
    if (tool_id != wxNOT_FOUND) {
      p_toolMenu->Check(tool_id, false);
    }
    if (prop_id != wxNOT_FOUND) {
      p_propertyMenu->Check(prop_id, false);
    }
    // turn off focus when panel is not shown
    VizPropertyPanel *panel = dynamic_cast<VizPropertyPanel*>(win);
    if (panel) {
      panel->setFocus(false);
    }

    // Fix for saving the open/close state for floated windows
    // Without explicitly telling it to save the pane layout after processing
    // the close, it won't do it automatically like docked panes
    // GDB 3/26/10
    ewxAuiManagerEvent evt(wxEVT_EWXAUI_UPDATE);
    AddPendingEvent(evt);
  }
}


void Builder::OnPaneTakeFocus(ewxAuiManagerEvent& event)
{
  wxBusyCursor c;

  // let the pane's window handle the event first
  if (event.pane->window && event.pane->window->ProcessEvent(event)) {
    return;
  }
  // this will clear the focus on all viz panels while setting the given one
  VizPropertyPanel *panel;
  if ((panel = dynamic_cast<VizPropertyPanel*>(event.pane->window))) {
    bool gettingFocus = !event.pane->HasFocus();
    panel->setFocus(gettingFocus);
    // toggle it open if not already
    if (gettingFocus && !event.pane->IsOpen()) {
      event.pane->Open();
    }
  }
}


void Builder::OnPaneAddFocus(ewxAuiManagerEvent& event)
{
  wxBusyCursor c;

  // let the pane's window handle the event first
  if (event.pane->window && event.pane->window->ProcessEvent(event)) {
    return;
  }
  // clear focus on all viz panels that aren't pinned and set the given one
  VizPropertyPanel *panel;
  if ((panel = dynamic_cast<VizPropertyPanel*>(event.pane->window))) {
    bool gettingFocus = !event.pane->IsPinned();
    panel->setPinned(gettingFocus);
    // toggle it open if not already
    if (gettingFocus && !event.pane->IsOpen()) {
      event.pane->Open();
    }
  }
}


void Builder::OnPaneOptions(ewxAuiManagerEvent& event)
{
  // let the pane's window handle the event first
  if (event.pane->window && event.pane->window->ProcessEvent(event)) {
    return;
  }
  TearableContentProvider * tcp;
  if ((tcp = dynamic_cast<TearableContentProvider*>(event.pane->window))) {
    TearableContent * tc = new TearableContent(tcp->GetTearableContent());
    tc->Position(wxGetMousePosition(), wxSize(1,1));
    tc->Popup();
  }
}


void Builder::OnPaneOpen(ewxAuiManagerEvent& event)
{
  wxBusyCursor c;

  // let the pane's window handle the event first
  if (event.pane->window && event.pane->window->ProcessEvent(event)) {
    return;
  }
  VizPropertyPanel *panel;
  if ((panel = dynamic_cast<VizPropertyPanel*>(event.pane->window))) {
    bool gettingFocus = !event.pane->IsOpen();
    panel->setFocus(gettingFocus);
  }

  // custom user message for the case of animating trajectories with the
  // atom table open
  if (event.pane->name == "Trajectory" &&
      !event.pane->IsOpen() &&
      p_mgr.GetPane(NAME_TOOL_ATOM_TABLE).IsShown()) {
    showMessage("Animating a trajectory with the atom table open will slow the frame rate.");
  }
}


void Builder::OnAuiUpdate(ewxAuiManagerEvent& event)
{
  savePaneLayout();
}


void Builder::OnShowCmdClick( wxCommandEvent& event )
{
  if (getenv("ECCE_DEVELOPER")) {
    p_cmd->Show(event.IsChecked());
    p_mainSizer->Layout();
  }
}


void Builder::OnCloseClick( wxCommandEvent& event )
{
  if (isDirty(p_calculation)) {
    int ret;
    long buttonFlags = wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION | wxCANCEL;
    ewxMessageDialog dlg(this, "The current calculation has unsaved changes!  "
                        "Do you want to save changes before closing?",
                        "Save Builder Changes?", buttonFlags);
    ret = dlg.ShowModal();
    if (ret == wxID_YES) {
      doSave();
    } else if (ret == wxID_CANCEL) {
      return;
    }
  }

  doClose(p_calculation->getURL());
}


void Builder::OnSaveClick( wxCommandEvent& event )
{
  doSave();
}


void Builder::OnSaveasClick( wxCommandEvent& event )
{
  doSaveAs();
}


void Builder::OnSaveThumbClick( wxCommandEvent& event )
{
  doSaveThumb();
  notifySubject(); // TODO is this abuse of this function?
}


void Builder::OnDumpsgClick( wxCommandEvent& event )
{
   Command *cmd = new DumpSGCmd("Dump scene graph", p_viewer);
   execute(cmd);
}


bool Builder::areLabelsOn() const
{
  return false;
}


/**
 * Updates the statusbar save icon and the menu bar to reflect
 * user's current capability to save the system based on whether or
 * not changes have been made.
 */
void Builder::setUnsavedState(bool flag)
{
  /*
  if (p_task && getEditStatus() != "READONLY") {
    if (flag) {
      // ::modified is tied to Save but not SaveAs
      setEditStatus("MODIFIED");
    } else {
      setEditStatus("EDIT");
    }
    p_fileToolbar->EnableTool(wxID_SAVE, flag);
    p_fileMenu->Enable(wxID_SAVE, flag);
  } else {
    p_fileToolbar->EnableTool(wxID_SAVE, false);
    p_fileMenu->Enable(wxID_SAVE, false);
  }
  */
}


/**
 * Prepare the system and save to calculation.
 */
void Builder::doSave()
{
  wxBusyCursor busy;

   // Move 'em out prior to save since they cannot be properly handled
   // by our scripts or codes
   //TODO
   //execute("CmdRemoveGhosts", (ICommandObject*)p_SG);

   // forcibly set the name and charge here.  Otherwise, a bad name
   // could be generated (new or clone) and never reset.
   //TODO
   //getCM()->setParam("CmdCSName", "name",getnameTexti()->gettextValue());
   //execute("CmdCSName",(ICommandObject*)p_SG));

   //TODO
   //int comboVal = getchargeComboi()->getintegerValue();
   //getCM()->setParam("CmdCSCharge", "charge", comboVal);
   //afterCommand(getCM()->execute("CmdCSCharge",(ICommandObject*)p_SG));

   /*
      char *symmetry = getsymmetryTexti()->gettextValue();
      if (symmetry && strlen(symmetry) != 0) {
      getCM()->setParam("CmdAssignSymmetry", "group", symmetry);
      } else {
      getCM()->setParam("CmdAssignSymmetry", "group", "C1");
      }
      getCM()->execute("CmdAssignSymmetry",(ICommandObject*)p_SG);
    */

  SGFragment *frag = getSG()->getFragment();
  string msg,title;
  bool abortFlag = false;
  if (p_calculation->promptBeforeSave(frag,msg,title)) {
    ewxMessageDialog dialog(this, msg, title,
                            wxYES_NO|wxNO_DEFAULT|wxICON_QUESTION);
    abortFlag = (dialog.ShowModal() == wxID_NO);
  }
  if (!abortFlag) {
    if (p_calculation->fragment(frag)) {
      setDirty(false);
      // Add a check to make warn them if they are saving with nubs
      // Do after save so empirical formula is regenerated
      string formula = frag->formula();
      if (STLUtil::containsString(formula,string("Nub"))) {
        showMessage("This chemical system still contains Nubs.  "
            "Make sure you complete it before trying to run a calculation!",
            false);
      }
      updateSave();
      updateResource();
      doSaveThumb();
      notifySubject();
    } else {
      showMessage("Unable to save changes.", true);
    }
  }
}


void Builder::doSaveAs(const bool& imagesOnly)
{
  SaveExperimentAsDialog dialog(this);
  if (imagesOnly) {
    dialog.setSaveAsFilterIndex(dialog.getImageIndex());
  }
  int returnCode = dialog.ShowModal();
  if (returnCode == wxID_OK) {
    if (dialog.canSave()) {
      string context;
      if (dialog.doSave(getSG()->getFragment(), context)) {
        if (CalculationFactory::canOpen(context)) {
          string lastContext = p_calculation->getURL();
          setContext(context);
          //doClose(lastContext, false);
        } else {
          wxLogWarning("Can't open recently saved %s", context.c_str());
        }
      } else {
        wxLogError("Save failed");
      }
    } else {
      SFile *file = TempStorage::getTempFile();

      wxString filename = dialog.GetFilename();
      wxString type = dialog.getType();
      wxString ext = dialog.getExt();
      wxString exts = dialog.getExts();
  
      // append extension to filename if one doesn't exist
      if (exts.Upper().Find(ext.Upper()) == wxNOT_FOUND) {
        ext = exts.BeforeFirst('|');
        filename << "." << ext;
      }
      
      // normally, temp filename is not changed, but it's needed for image conv
      file->move(file->path() + "." + ext.c_str());
  
      if (writeFragmentToImageFile(file, type, ext)) {
        // now 'upload' new image where we really want it to go
        string path = dialog.GetPath().c_str();
        EcceURL url(path);
        Resource * resource = EDSIFactory::getResource(url.getParent());
        if (resource == 0) {
          // shouldn't happen
          wxLogError("Could not save as image for %s", url.toString().c_str());
        } else {
          resource = resource->createChild(filename.c_str(), file);
          if (resource == 0) {
            wxLogError("Could not save as image for %s",url.toString().c_str());
          } else {
            dialog.notifyCreate(resource->getURL().toString());
          }
        }
      }
    
      // cleaup
      if (file->exists()) file->remove();
      delete file;
    }
  }
}


void Builder::doSaveThumb()
{
  ChemistryTask *task = NULL;
  if ((task = dynamic_cast<ChemistryTask*>(p_calculation))) {
    // temporarily turn off labels, but don't let CommandManager know
    AtomLabelsCmd cmd("AtomLabel", getSG());
    cmd.getParameter("type")->setInteger(AtomLabelsCmd::NONE);
    cmd.execute();
    // create thumbnail (without labels)
    if (!VizRender::thumbnail(p_viewer->getTopNode(), task))
      showMessage(VizRender::msg(), true);
    // turn labels back on
    cmd.undo();
  }
}


/**
 * Closes current IPropCalculation and opens either first availabe
 * IPropCalculation or creates new IPropCalculation.
 */
void Builder::doClose(const string& context, const bool& autoOpen)
{
  string contextToClose = context;
  if (context.empty()) {
    contextToClose = p_calculation->getURL();
  }

  map<string, IPropCalculation*>::iterator iter =
                                           p_calculations.find(contextToClose);
  // fail silent if we're trying to close a context we know nothing about
  if (iter == p_calculations.end()) return;

  // save off the IPropCalc to use it later on
  IPropCalculation *calcToClose = iter->second;

  // unset focus of all viz prop panels
  // this fixes seg fault if we're animating a trj or geom trace
  set<VizPropertyPanel*> panels = VizPropertyPanel::getPanels(contextToClose);
  set<VizPropertyPanel*>::iterator panel;
  for (panel = panels.begin(); panel != panels.end(); ++panel) {
    (*panel)->setFocus(false);
  }

  // create an empty default context in case we're closing the last non-default
  if (p_calculations.size() == 1) {
    wxCommandEvent emptyEvent;
    OnNewClick(emptyEvent);
  } else if (autoOpen) {
    // find the first calculation that isn't the current one
    map<string,IPropCalculation*>::iterator it;
    for (it = p_calculations.begin(); it != p_calculations.end(); ++it) {
      if (it->first != contextToClose) break;
    }
    setContext(it->first);
  }

  // now remove the old context
  p_calculations.erase(contextToClose);
  delete p_commandManagers[contextToClose];
  p_commandManagers.erase(contextToClose);
  p_sgMgr->removeSceneGraph(contextToClose);
  p_contextHistory->RemoveContext(contextToClose);
  p_contextPanel->RemoveContext(contextToClose);
  removePropertyPanels(contextToClose);

  // delete the entire calculation if it was a transient import
  if (isImport(calcToClose)) {
    EDSI *edsi = EDSIFactory::getEDSI(contextToClose);
    edsi->removeResource();
  }
}


bool Builder::isReadOnly() const
{
   return isReadOnly((IPropCalculation*)0);
}

/**
 * Determines the readonly status of the given calc, or the current calc if 
 * no calc is given.
 */
bool Builder::isReadOnly(IPropCalculation *calc) const
{
  if (!calc) {
    calc = p_calculation;
  }

  return calc->isReadOnly();
}


/**
 * Determines the dirty status of the given calc, or the current calc if
 * no calc is given.
 */
bool Builder::isDirty(IPropCalculation *calc)
{
  if (!calc) {
    calc = p_calculation;
  }
  return p_dirty[calc->getURL().toString()];
}


void Builder::setDirty(const bool& val, IPropCalculation *calc)
{
  if (!calc) {
    calc = p_calculation;
  }
  if (isReadOnly(calc)) {
    p_dirty[calc->getURL().toString()] = false;
  } else {
    p_dirty[calc->getURL().toString()] = val;
  }
}


/**
 * Determines the import status (whether it was imported "transiently" in the
 * builder) of the given calc, or the current calc if no calc is given.
 */
bool Builder::isImport(IPropCalculation *calc)
{
  if (!calc) {
    calc = p_calculation;
  }
  return p_import[calc->getURL().toString()];
}


void Builder::setImport(const bool& val, IPropCalculation *calc)
{
  if (!calc) {
    calc = p_calculation;
  }
  p_import[calc->getURL().toString()] = val;
}


/**
 * Method invoked by feedback save button.
 * This simply calls doSave.
 */
void Builder::processSave()
{
   doSave();
}


/**
 * Method invoked by FragmentExportDialog.
 */
bool Builder::writeFragmentToImageFile(SFile *file, wxString type, wxString ext)
{
  bool ret = true;

  if (type.StartsWith("POV-Ray")) {
    WxPovrayOptionsDialog po(this);
    if (po.ShowModal() == wxID_OK) {
      Command * cmd = new PovrayCmd("Render POV-Ray", p_viewer);
      cmd->getParameter("filename")->setString(file->path());
      cmd->getParameter("finishStyle")->setString(po.getFinishStyle());
      cmd->getParameter("bondStyle")->setString(po.getBondStyle());
      cmd->getParameter("isosurfaceStyle")->setString(po.getIsoStyle());
      execute(cmd);

      if (po.isDisplayChecked()) {
        SFile *tmpoutfile = TempStorage::getTempFile();
        ostringstream os;
        os << "povray";
        os << " +W" << po.getWidth();
        os << " +H" << po.getHeight();
        os << " +A";
        os << " +O" << tmpoutfile->path();
        os << " " << file->path();
        os << ends;
        CommandWrapper cw(os.str());
        try {
            cw.execute();
            BrowserHelp().showPage("file:"+tmpoutfile->path()+".png");
        } catch (SystemCommandException& ce) {
            string msg = ce.what();
            msg += "\n\nYou must have POV-Ray installed and in your path.";
            msg += "\n\nSee http://www.povray.org";
            showMessage(msg, true);
            ret = false;
        }
        delete tmpoutfile;
      }
    }
  } else { // not POV-Ray
    wxSize size = p_viewer->GetSize();
    SbColor color = p_viewer->getBackgroundColor();
    Command * cmd = new RenderFileCmd("Render File", p_viewer);
    cmd->getParameter("width")->setInteger(size.GetWidth());
    cmd->getParameter("height")->setInteger(size.GetHeight());
    cmd->getParameter("red")->setDouble(color[0]);
    cmd->getParameter("green")->setDouble(color[1]);
    cmd->getParameter("blue")->setDouble(color[2]);

    if (type.StartsWith("Postscript")) {
      cmd->getParameter("type")->setString(type.c_str());
      cmd->getParameter("filename")->setString(file->path());
      execute(cmd);
    } else { // ok, so create RGB and convert to format we want
      SFile *tmp = TempStorage::getTempFile();
      tmp->move(tmp->pathroot() + "/" + tmp->filename() + ".rgb");
      cmd->getParameter("type")->setString("RGB");
      cmd->getParameter("filename")->setString(tmp->path());
      execute(cmd);

      // check whether site has disabled creation of image files
      if (!getenv("ECCE_NO_VIZIMAGES")) {
        ImageConverter imconv;
        try {
          // Note that the file extension is crucial to success
          imconv.convert(tmp->path(), file->path(), 
                         size.GetWidth(), size.GetHeight(),
                         8 /*image depth*/, true /*remove inFile*/);
        } catch (EcceException& ex) {
          ret = false;
        }
      }
      if (tmp->exists()) tmp->remove();
      delete tmp;
    }
  }

  return ret;
}


/**
 * Method invoked by FragmentImportDialog.
 */
bool Builder::readFragmentFromFile(SFile *file, wxString type, wxString ext)
{
  bool ret = true;

  string units = "angstroms";

  AddFragmentCmd *cmd = new AddFragmentCmd("Add Fragment", getSG());
  cmd->getParameter("streamType")->setString(ext.c_str());
  cmd->getParameter("fileName")->setString(file->path());
  if (ext.IsSameAs("MVM",false)) {
      cmd->getParameter("genBondOrders")->setBoolean(false);
  } else {
      cmd->getParameter("genBondOrders")->setBoolean(true);
  }

  if (ext.IsSameAs("XYZ",false)) {
    WxUnitsPrompt unitsPrompt(this);
    if (unitsPrompt.ShowModal() != wxID_OK) {
      ret = false;
    } else {
      units = unitsPrompt.getUnits();
    }
  }

  string altLoc = " ";  // default alt location
  int selModel = 1;     // default model
  string selChain = " ";

  if (ret) {
    if (ext.IsSameAs("PDB",false)) {
      ret = getPDBOptions(file->path(), altLoc, selModel, selChain);
      cmd->getParameter("selectAltLoc")->setString(altLoc);
      cmd->getParameter("selectModel")->setInteger(selModel);
      cmd->getParameter("selectChainID")->setString(selChain);
    }

    if (ret) {
       cmd->getParameter("units")->setString(units);
       cmd->getParameter("x")->setDouble(0.0);
       cmd->getParameter("y")->setDouble(0.0);
       cmd->getParameter("z")->setDouble(0.0);

       execute(cmd);

       if (ext.IsSameAs("PDB",false)) {
          (void)createSolventSoluteStyles(*(getSG()->getFragment()));
          // Added when we decided on concept of original fragment.
          if (ChemistryTask *task=dynamic_cast<ChemistryTask*>(p_calculation)) {
            ifstream is(file->path().c_str());
            if (is.good()) {
              Fragment *frag = getSG()->getFragment();
              string name;
              if (frag) name = frag->name();
              task->setOriginalFragment(&is,selModel,altLoc,selChain,name);
            }
            is.close();
          }
       }
    }
  }

  return ret;
}


/**
 * @ret false means cancel
 */
bool Builder::getPDBOptions(const string& filename, string& altLoc, int& model, string& chain)
{
   bool ret = true;
   ifstream is(filename.c_str());
   if (is.good()) {
      SGFragment *sgfrag = getSG()->getFragment();
      int numModels = 1;
      vector<string> altLocVec;
      vector<bool> groupFlgVec;
      vector<string> chainVec;
      int totalAtoms;

      if (sgfrag->prescanPDB(is,numModels,
               altLocVec,groupFlgVec,chainVec,totalAtoms)) {
         // TODO note that I commented the last part (groupFlgVec?)
         // out for now because its not supported.  No need
         // to pop up the dialog.
         //bool towire = false;
         //if (setWireFrame(totalAtoms, towire))
         if (true) {
            if (numModels>1 || altLocVec[0]!="" /*|| groupFlgVec[0]*/) {
               WxPDBPrompt prompt(this, -1, "ECCE PDB Reader",
                  wxDefaultPosition, wxDefaultSize );
               prompt.setModelRange(numModels);
               prompt.setAltLocOptions(altLocVec);
               prompt.setChainOptions(chainVec);
               if (prompt.ShowModal() == wxID_OK) {
                  // grab data from ui for return params
                  model = prompt.getPDBModel();
                  altLoc = prompt.getAltLocation();
                  chain = prompt.getChainID();
                  if (chain == "All") chain = " ";

               } else {
                  ret = false;
               }
               prompt.Destroy();

            }
         } else {
            ret = false;
         }

      }
   }
   is.close();
   return ret;
}


/**
 * The AUI SavePerspective is insufficient for our needs.
 *
 * We don't want to save PropertyPanels as part of our layout.  I didn't want
 * to hack up AUI any further, so I've implemented these.  Nobody said we HAD
 * to use AUI's SavePerspective.
 *
 * @param layoutName_ what we'll call this layout in our preferences file
 */
void Builder::savePaneLayout(const wxString& layoutName_)
{
  // make a copy since it's a const param
  wxString layoutName(layoutName_);

  if (layoutName.IsEmpty()) {
    if (p_currentMode == ID_MODE_STRUCTLIB) {
      layoutName = NAME_LAYOUT_STRUCTLIB;
    } else if (isReadOnly()) {
      layoutName = NAME_LAYOUT_READONLY;
    } else {
      layoutName = NAME_LAYOUT_DEFAULT;
    }
  }

  ewxConfig *config = ewxConfig::getConfig("wxbuilder.ini");
  wxString layoutPrefix = NAME_LAYOUT_PREFIX + layoutName + '/'; 
  // iterate over all panes and save their layout info
  ewxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
  for (size_t i = 0, count = panes.GetCount(); i < count; ++i) {
    ewxAuiPaneInfo &pane = panes.Item(i);
    wxString info = p_mgr.SavePaneInfo(pane);
    if (dynamic_cast<PropertyPanel*>(pane.window)) {
      // prop panel layouts are save elsewhere
      p_propertyPanelInfo[pane.window] = info;
    } else {
      config->Write(layoutPrefix + pane.name, info);
    }
  }
}


void Builder::loadPaneLayout(const wxString& layoutName_, const bool& update)
{
  // make a copy since it's a const param
  wxString layoutName(layoutName_);

  if (layoutName.IsEmpty()) {
    if (p_currentMode == ID_MODE_STRUCTLIB) {
      layoutName = NAME_LAYOUT_STRUCTLIB;
    } else if (isReadOnly()) {
      layoutName = NAME_LAYOUT_READONLY;
    } else {
      layoutName = NAME_LAYOUT_DEFAULT;
    }
  }

  ewxConfig *config = ewxConfig::getConfig("wxbuilder.ini");
  wxString layoutPrefix = NAME_LAYOUT_PREFIX + layoutName; 

  if (!config->HasGroup(layoutPrefix)) {
    // the following also calls savePaneLayout()
    loadDefaultPaneLayout(layoutName, false);
  } else {
    wxString oldpath = config->GetPath();
    config->SetPath(layoutPrefix);
    if (config->GetNumberOfEntries() <= 1) {
      // the following also calls savePaneLayout()
      loadDefaultPaneLayout(layoutName, false);
    }
    config->SetPath(oldpath);
  }

  // Detach all PropertyPanels from aui management -- we'll restore them later
  set<PropertyPanel*> ppanels;
  set<PropertyPanel*>::iterator ppanelIt;
  ppanels = PropertyPanel::getPanels();
  for (ppanelIt = ppanels.begin(); ppanelIt != ppanels.end(); ++ppanelIt) {
    ewxAuiPaneInfo &pane = p_mgr.GetPane(*ppanelIt);
    if (pane.IsOk()) {
      pane.window->Hide();
      p_mgr.DetachPane(*ppanelIt);
    }
  }

  layoutPrefix += "/";

  // iterate over all panes and restore their layout info, if any
  ewxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
  for (size_t i = 0, count = panes.GetCount(); i < count; ++i) {
    ewxAuiPaneInfo &pane = panes.Item(i);
    wxString info;
    if (config->Read(layoutPrefix + pane.name, &info)) {
      p_mgr.LoadPaneInfo(info, pane);
    }
  }

  // Add property panels back in from our internal layout cache
  ppanels = PropertyPanel::getPanels(p_calculation->getURL().toString());
  for (ppanelIt = ppanels.begin(); ppanelIt != ppanels.end(); ++ppanelIt) {
    PropertyPanel *panel = *ppanelIt;
    ewxAuiPaneInfo pane;
    addPropertyPanel(panel, panel->getName());
    wxString info = p_propertyPanelInfo[panel];
    if (!info.IsEmpty()) {
      p_mgr.LoadPaneInfo(info, pane);
      ewxAuiPaneInfo &p = p_mgr.GetPane(pane.name);
      if (!p.IsOk()) {
        continue;
      }
      p.SafeSet(pane);
    }
  }

  SGFragment *frag = getSG()->getFragment();

  if (frag->numResidues() &&
        GetMenuBar()->IsChecked(ViewerEvtHandler::ID_AUTO_RESIDUE) &&
        !isReadOnly() &&
        !p_mgr.GetPane("Residue Table").IsShown()) {
     p_mgr.GetPane("Residue Table").Show(true).Open(true);

     // If its a viz tool, call virtual refresh to force updating
     // Works around bug where we can't override show because it gets
     // called too much by AUI
     WxVizTool *panel = 
        dynamic_cast<WxVizTool*>(p_mgr.GetPane("Residue Table").window);
     if (panel) {
        panel->refresh();
     }

     p_mgr.Update();
     GetMenuBar()->Check(GetMenuBar()->FindMenuItem("Tools", "Residue Table"), true);
  } else if (frag->numResidues()==0 &&
               p_mgr.GetPane("Residue Table").IsShown()) {
     p_mgr.GetPane("Residue Table").Show(false).Open(false);

     p_mgr.Update();
     GetMenuBar()->Check(GetMenuBar()->FindMenuItem("Tools", "Residue Table"), false);
  }

  if (update) {
    p_mgr.Update();
  }
}


void Builder::loadDefaultPaneLayout(const wxString& layoutName,
                                    const bool& update)
{
  set<string> names;
  string name;
  bool show;

  names.clear();
  if (layoutName == NAME_LAYOUT_READONLY) {
    // All tools are hidden except Atom Table, Selection and Log
    names.insert(NAME_TOOL_CONTEXT);
    names.insert(NAME_TOOL_ATOM_TABLE);
    names.insert(NAME_TOOL_SELECTION);
    names.insert(NAME_TOOL_LOG);
  } else if (layoutName == NAME_LAYOUT_DEFAULT) {
    // All tools are hidden except Build, Coordinate, and Log
    names.insert(NAME_TOOL_CONTEXT);
    names.insert(NAME_TOOL_BUILD);
    names.insert(NAME_TOOL_COORDINATES);
    names.insert(NAME_TOOL_LOG);
  } else if (layoutName == NAME_LAYOUT_STRUCTLIB) {
    // All tools are hidden except Structure Library
    names.insert(NAME_TOOL_CONTEXT);
    names.insert(NAME_TOOL_STRUCTLIB);
  }
  for (int i=0; i<p_toolCount; ++i) {
    name = p_toolMenu->GetLabel(ID_TOOLMENU_ITEM+i);
    show = names.find(name) != names.end();
    p_mgr.GetPane(name).Show(show).Open(show);
  }
 
  // special cases of positioning
  if (layoutName == NAME_LAYOUT_READONLY) {
    // put Atom tool on right, Log on bottom
    p_mgr.GetPane(NAME_TOOL_ATOM_TABLE).Right();
    p_mgr.GetPane(NAME_TOOL_LOG).Position(p_toolCount+1);
  } else if (layoutName == NAME_LAYOUT_DEFAULT) {
    // noop
  } else if (layoutName == NAME_LAYOUT_STRUCTLIB) {
    p_mgr.GetPane(NAME_TOOL_STRUCTLIB).Show(true).Open(true).Right();
  }

  names.clear();
  if (layoutName == NAME_LAYOUT_READONLY) {
    // All toolbars are shown except Manipulator and Measure
    names.insert(NAME_TOOLBAR_MANIPULATOR);
    names.insert(NAME_TOOLBAR_MEASURE);
  } else if (layoutName == NAME_LAYOUT_DEFAULT) {
    // All toolbars are shown except Measure
    names.insert(NAME_TOOLBAR_MEASURE);
  } else if (layoutName == NAME_LAYOUT_STRUCTLIB) {
    // noop
  }
  for (int i=0; i<p_toolbarCount; ++i) {
    name = p_toolbarMenu->GetLabel(ID_TOOLBARMENU_ITEM+i);
    show = names.find(name) == names.end();
    p_mgr.GetPane(name).Show(show).Open(show);
  }

  // save this default layout immediately
  savePaneLayout(layoutName);

  if (update) {
    p_mgr.Update();
  }
}


/**
 * Calls all UI updating functions.
 */
void Builder::updateUI()
{
  updateSave();
  updateMenus();
  updateResource();
  updateReadOnly(false);
  updatePropertyMenus();
  updateUndoMenus();
  updateBondIcon();
  updateElementIcon();
  updateShapeIcon();
}


/**
 * Enable Save, SaveAs, etc based on various IPropCalculation/Job properties.
 */
void Builder::updateSave()
{
  // Save As is always enabled...
  //
  // If we only have a DefaultCalculation,
  //    -save is disabled
  //    -edit status is always EDIT, regardless of dirtiness
  // Else (we have any other IPropCalculation)
  //    -if READONLY
  //        -disable Save
  //        -set edit status READONLY
  //    -else
  //        -set edit status MODIFIED if dirty, EDIT otherwise
  //        -enable Save if dirty

  if (dynamic_cast<DefaultCalculation*>(p_calculation)) {
    // DefaultCalculation
    p_fileMenu->Enable(wxID_SAVE, false);
    p_fileToolbar->EnableTool(wxID_SAVE, false);
    setEditStatus("EDIT");
  } else {
    // not a DefaultCalculation
    if (isReadOnly()) {
      p_fileMenu->Enable(wxID_SAVE, false);
      p_fileToolbar->EnableTool(wxID_SAVE, false);
      setEditStatus("READONLY");
    } else {
      // not read only
      if (isDirty()) {
        // not read only and dirty
        p_fileMenu->Enable(wxID_SAVE, true);
        p_fileToolbar->EnableTool(wxID_SAVE, true);
        setEditStatus("MODIFIED");
      } else {
        // not read only and not dirty
        p_fileMenu->Enable(wxID_SAVE, false);
        p_fileToolbar->EnableTool(wxID_SAVE, false);
        setEditStatus("EDIT");
      }
    }
  }
}

/**
 * Update core menu items based on the fragment.
 * Currently defined as Residues or no residues.
 */
void Builder::updateMenus()
{
   Fragment *frag = getSG()->getFragment();
   bool enable = frag->numResidues() > 0;
   p_editMenu->Enable(ID_SELECT_BACKBONE, enable);
   p_editMenu->Enable(ID_SELECT_SIDECHAINS, enable);
   GetMenuBar()->Enable(GetMenuBar()->FindMenuItem("Tools", "Residue Table"), enable);
}

/**
 * Special UI updates whether we have a part of the Resource hierarchy as our
 * IPropCalculation.
 */
void Builder::updateResource()
{
  TaskJob *taskJob = NULL;
  if ((taskJob = dynamic_cast<TaskJob*>(p_calculation))) {
    setRunState(taskJob->getState());
    if (taskJob->getState() >= ResourceDescriptor::STATE_SUBMITTED) {
      taskJob->setReviewed(true);
    }
  } else {
    setRunState(ResourceDescriptor::NUMBER_OF_STATES);
  }

  if (dynamic_cast<Resource*>(p_calculation)) {
    // When currently in the context of a calculation, users will be confused
    // by the distinction between Open and Load so make life easier in this
    // most common usage case
    p_fileMenu->Enable(wxID_OPEN, false);
    p_fileToolbar->EnableTool(wxID_OPEN, false);

    p_fileMenu->Enable(ID_SAVE_THUMB, true);
  } else {
    p_fileMenu->Enable(wxID_OPEN, true);
    p_fileToolbar->EnableTool(wxID_OPEN, true);

    p_fileMenu->Enable(ID_SAVE_THUMB, false);
  }
}


void Builder::updatePropertyMenus()
{
  set<PropertyPanel*> panels;
  set<PropertyPanel*>::iterator panelIt;
  set<string> names;
  set<string>::iterator name;
  bool needUpate = false;
  string urlstr = p_calculation->getURL().toString();

  // clear out current property menu items
  for (int i = p_propertyMenu->GetMenuItemCount()-1; i >= 0; --i) {
    p_propertyMenu->Destroy(p_propertyMenu->FindItemByPosition(i));
  }

  // look up panels to add to menu
  panels = PropertyPanel::getPanels(p_calculation->getURL().toString());
  for (panelIt = panels.begin(); panelIt != panels.end(); ++panelIt) {
    PropertyPanel *panel = *panelIt;
    addPropertyPanel(panel, panel->getName());
    needUpate = true;
  }

  // create any missing property panels
  PropertyPanelFactory &ppf = PropertyPanelFactory::getPropertyPanelFactory();
  names = ppf.getPanelNamesForProperties(p_calculation->propertyNames());
  for (name = names.begin(); name != names.end(); ++name) {
    if (name->find("Metadynamics Potential")==string::npos) {
      if (!PropertyPanel::panelExists(urlstr, *name)) {
        createPropertyPanel(*name);
        needUpate = true;
      }
    } else if ((*name== "1D Metadynamics Potential" &&
                p_calculation->getProperty("IKCPVEC") &&
                p_calculation->getProperty("METAVEC")) ||
               (*name== "2D Metadynamics Potential" &&
                p_calculation->getProperty("IKCPVEC") &&
                p_calculation->getProperty("METATABLE"))) {
      createPropertyPanel(*name);
      needUpate = true;
    }
  }

  if (needUpate) {
    p_mgr.Update(); // TODO is this needed yet or can it wait?
  }

  // disable menu if no property guis found
  GetMenuBar()->EnableTop(GetMenuBar()->FindMenu(p_propertyMenu->GetTitle()),
                          p_propertyMenu->GetMenuItemCount()>0);
}


/**
 * En/disable Builder-related Tools, Toolbars, etc based on readonly status
 * of current IPropCalculation.
 * Think of this function as setting the UI to look like the Builder (not
 * readonly) vs the Viewer (readonly).
 */
void Builder::updateReadOnly(const bool& loadPaneFlag)
{
  bool isReadOnly = this->isReadOnly();

  if (loadPaneFlag) {
    // before layouts are save/restored
    if (isReadOnly) {
      loadPaneLayout(NAME_LAYOUT_READONLY);
    } else { // !isReadOnly
      loadPaneLayout(NAME_LAYOUT_DEFAULT);
    }
  }

  // We've finished with the layout, now on to other ReadOnly things.

  // Iterate through all menu items looking for disabled ones
  // If we end up disabling all menu items for a given menu,
  // the top level menu item is also disabled
  wxMenuBar *menuBar = GetMenuBar();
  int menuCount = menuBar->GetMenuCount();
  for (int menuIdx = 0; menuIdx < menuCount; ++menuIdx) {
    bool allDisabled = true;
    wxMenu *menu = menuBar->GetMenu(menuIdx);
    wxMenuItemList &items = menu->GetMenuItems();
    wxMenuItemList::iterator itemIter;
    for (itemIter = items.begin(); itemIter != items.end(); ++itemIter) {
      wxMenuItem *item = *itemIter;
      if (p_readOnlyDisabledIds.find(item->GetId())
              != p_readOnlyDisabledIds.end()) {
        item->Enable(!isReadOnly);
      } else if (item->GetId() > 0) {
        allDisabled = false;
      }
    }
    if (allDisabled) {
      menuBar->EnableTop(menuIdx, !isReadOnly);
    }
  }
  // Iterate through all toolbars looking for disabled ones
  // There is no way to determine if an all toolbar children were disabled 
  vector<wxToolBar*>::iterator toolbarIter;
  for (toolbarIter = p_toolbars.begin();
       toolbarIter != p_toolbars.end();
       ++toolbarIter) {
    wxToolBar *toolbar = *toolbarIter;
    set<int>::const_iterator idIter;
    for (idIter = p_readOnlyDisabledIds.begin();
         idIter != p_readOnlyDisabledIds.end();
         ++idIter)
    {
      toolbar->EnableTool(*idIter, !isReadOnly);
    }
  }

  // if in a mode that is not supported in Viewer mode, change to select mode
  if (isReadOnly &&
          (p_currentMode > ID_MODE_ZOOM || p_currentMode < ID_MODE_SELECT)) {
    setMode(ID_MODE_SELECT);
    toggleModeButton(ID_MODE_SELECT);
  }

  // Restore tool menu status based on layout info
  for (int i=0; i<p_toolCount; ++i) {
    wxString label = p_toolMenu->GetLabel(ID_TOOLMENU_ITEM+i);
    p_toolMenu->Check(ID_TOOLMENU_ITEM+i, p_mgr.GetPane(label).IsShown());
  }
  // Restore toolbar menu status based on layout info
  for (int i = 0; i < p_toolbarCount; ++i) {
    wxString label = p_toolbarMenu->GetLabel(ID_TOOLBARMENU_ITEM+i);
    p_toolbarMenu->Check(ID_TOOLBARMENU_ITEM+i,
                         p_mgr.GetPane(label).IsShown());
  }

  // update our window name and help menu item
  wxString title = "ECCE Builder";
  wxString menu_help = "on Builder\tCtrl+h";
  if (isReadOnly) {
    title = "ECCE Viewer";
    menu_help = "on Viewer\tCtrl+h";
  }
  wxMenuItem * help_menu_item = GetMenuBar()->FindItem(wxID_HELP);
  if (help_menu_item) help_menu_item->SetItemLabel(menu_help);
  SetTitle(title);
  
  p_mgr.Update();
}


/**
 * Notify other tools of the builder selection.  Only the first 4
 * selected atoms are broadcast since we have not identified a need
 * to notify tools of full selection.
 */
void Builder::notifySubject() const
{
   JMSMessage *msg = newMessage();
   msg->addProperty("url",getContext());
   publish("ecce_url_subject",*msg);
   delete msg;
}





/** 
 * Translate viewer pick point to screen position for mapping popups.
 * viewer y is at lower left.  widgets start at upper left.
 * The y is off by a little - not sure why; don't care at the moment.
 */
void Builder::viewPosToScreenPos(int& x, int& y)
{
    wxSize size = p_viewer->GetSize();
    y = size.GetHeight() - y;
    ClientToScreen(&x,  &y);
}


/**
 * Popup a dialog to prompt for a new length the move atoms.
 */
void Builder::lengthPopup(int x, int y, AtomMeasureDist *dist)
{
   viewPosToScreenPos(x, y);
   float startValue = dist->getDistance();
   float newValue = startValue;
   wxPoint pos(x,y);
   WxMeasurePrompt txt(this, WxMeasurePrompt::DISTANCE, startValue, wxOK|wxCANCEL, pos);
   txt.ShowModal();
   newValue = txt.GetValue();
   if (startValue != newValue) {

      Fragment *frag = getSG()->getFragment();
      int atom1 = frag->atomIndex(*(dist->atom1()));
      int atom2 = frag->atomIndex(*(dist->atom2()));

      Command *cmd = new LengthEditCmd("Edit Bond Length", getSG());
      cmd->getParameter("length")->setDouble(newValue);
      cmd->getParameter("atom1")->setInteger(atom1);
      cmd->getParameter("atom2")->setInteger(atom2);
      cmd->getParameter("all")->setBoolean(txt.isToggleSet());
      execute(cmd);
   }
}


/**
 * Popup a dialog to prompt for a new angle and the move atoms.
 */
void Builder::anglePopup(int x, int y, AtomMeasureAngle *angle)
{
   viewPosToScreenPos(x, y);
   float startValue = angle->getAngle();
   float newValue = startValue;
   wxPoint pos(x,y);
   WxMeasurePrompt txt(this, WxMeasurePrompt::ANGLE, startValue, wxOK|wxCANCEL, pos);
   txt.ShowModal();
   newValue = txt.GetValue();
   if (startValue != newValue) {

      Fragment *frag = getSG()->getFragment();
      int atom1 = frag->atomIndex(*(angle->atom1()));
      int atom2 = frag->atomIndex(*(angle->atom2()));
      int atom3 = frag->atomIndex(*(angle->atom3()));

      Command *cmd = new AngleEditCmd("Edit Angle", getSG());
      cmd->getParameter("angle")->setDouble(newValue);
      cmd->getParameter("atom1")->setInteger(atom1);
      cmd->getParameter("atom2")->setInteger(atom2);
      cmd->getParameter("atom3")->setInteger(atom3);
      cmd->getParameter("all")->setBoolean(txt.isToggleSet());
      execute(cmd);
   }
}


void Builder::torsionPopup(int x, int y, AtomMeasureTorsion *torsion)
{
   viewPosToScreenPos(x, y);
   float startValue = torsion->getTorsion();
   float newValue = startValue;
   wxPoint pos(x,y);
   WxMeasurePrompt txt(this, WxMeasurePrompt::TORSION, startValue, wxOK|wxCANCEL, pos);
   txt.ShowModal();
   newValue = txt.GetValue();
   if (startValue != newValue) {

      Fragment *frag = getSG()->getFragment();
      int atom1 = frag->atomIndex(*(torsion->atom1()));
      int atom2 = frag->atomIndex(*(torsion->atom2()));
      int atom3 = frag->atomIndex(*(torsion->atom3()));
      int atom4 = frag->atomIndex(*(torsion->atom4()));

      Command *cmd = new TorsionEditCmd("Edit Torsion", getSG());
      cmd->getParameter("torsion")->setDouble(newValue);
      cmd->getParameter("atom1")->setInteger(atom1);
      cmd->getParameter("atom2")->setInteger(atom2);
      cmd->getParameter("atom3")->setInteger(atom3);
      cmd->getParameter("atom4")->setInteger(atom4);
      cmd->getParameter("all")->setBoolean(txt.isToggleSet());
      execute(cmd);
   }
}


void Builder::addToolPanel(wxWindow *panel, const string& name,
                           const bool& readOnlyDisabled,
                           const bool& alwaysFixed)
{
  ewxAuiPaneInfo pinfo;
  pinfo.Name(name).Caption(name).Show(false).CaptionVisible(true)
          .Right().Position(p_toolCount).Fixed().AlwaysFixed();
  if (alwaysFixed) {
    pinfo.AlwaysFixed(true).MaximizeButton(false);
  } else {
    pinfo.AlwaysFixed(false).MaximizeButton(true);
  }

  if (dynamic_cast<TearableContentProvider*>(panel))
    pinfo.OptionsButton();

  p_mgr.AddPane(panel, pinfo);

  WxVizTool * vizTool = dynamic_cast<WxVizTool *>(panel);
  if (vizTool)
    vizTool->connectToolKitFW(this);

  p_toolMenu->AppendCheckItem(ID_TOOLMENU_ITEM+p_toolCount, name, "");
  if (readOnlyDisabled) {
    p_readOnlyDisabledIds.insert(ID_TOOLMENU_ITEM+p_toolCount);
  }
  ++p_toolCount;
}


void Builder::addToolBar(wxToolBar * toolbar, const string& name)
{
  toolbar->Realize();
  
  ewxAuiPaneInfo pinfo;
  pinfo.Name(name).Caption(name).ToolbarPane().Top().Position(p_toolbarCount).
          LeftDockable(false).RightDockable(false);
  p_mgr.AddPane(toolbar, pinfo);

  p_toolbarMenu->AppendCheckItem(ID_TOOLBARMENU_ITEM+p_toolbarCount, name, "");
  ++p_toolbarCount;

  p_toolbars.push_back(toolbar);
}


void Builder::addPropertyPanel(PropertyPanel *panel, const string& name)
{
  // Don't add this panel if it is already being managed
  ewxAuiPaneInfo &pinfo = p_mgr.GetPane(panel);
  if (pinfo.IsOk()) {
    pinfo.Show();
  } else {
    ewxAuiPaneInfo info = ewxAuiPaneInfo();
    info.DefaultPane();
    info.Name(name).Caption(name).CaptionVisible(true).
            Left().Layer(2).Fixed().AlwaysFixed();
    if ((dynamic_cast<VizPropertyPanel*>(panel))) {
      info.TakeFocusButton();
      if (panel->allowPin()) {
        info.AddFocusButton();
      }
    }
    if (dynamic_cast<TearableContentProvider*>(panel))
      info.OptionsButton();
    info.Position(p_propertyMenu->GetMenuItemCount());
    p_mgr.AddPane(panel, info);
  }

  // add to property menu and Check its item
  p_propertyMenu->AppendCheckItem(
          ID_PROPERTY_MENU_BASE+p_propertyMenu->GetMenuItemCount(),
          //name, "")->Check(panel->isDismissed());
          name, "")->Check(pinfo.IsShown());
}


void Builder::createPropertyPanel(const string& name)
{
  PropertyPanelFactory &ppf = PropertyPanelFactory::getPropertyPanelFactory();
  PropertyPanel *panel = ppf.createPropertyPanel(name);
  if (!panel) {
    wxLogError("Could not get PropertyPanel with name %s", name.c_str());
    return;
  }

  // Panel relevance is mostly based on static information found in the
  // PropertyPanelDescriptor.xml and whether the given calculation
  // reports certain property keys.  However, there are some cases where
  // dynamic information is needed to determine relevance.
  if (!panel->isRelevant(p_calculation)) {
    delete panel;
    return;
  }

  // Some of the panels are viz tools
  WxVizTool *viztool = 0;
  if ((viztool = dynamic_cast<WxVizTool*>(panel))) {
    viztool->connectToolKitFW(this);
  }
  
  // the real panel creation
  panel->Create(p_calculation, this, wxID_ANY);
  panel->SetMinSize(wxSize(400,-1));
  panel->Show(false);

  addPropertyPanel(panel, name);
}


void Builder::removePropertyPanels(const string& context)
{
  string contextToClose = context;
  if (context.empty()) {
    contextToClose = p_calculation->getURL();
  }

  set<PropertyPanel*> panels = PropertyPanel::getPanels(contextToClose);
  set<PropertyPanel*>::iterator panelIt;
  for (panelIt = panels.begin(); panelIt != panels.end(); ++panelIt) {
    p_mgr.DetachPane(*panelIt);
  }
  PropertyPanel::removePanels(contextToClose);
}


// @todo
// Move all the swap bitmap code here
void Builder::toggleTool(int id, bool isChecked)
{
  if (id == ViewerEvtHandler::ID_CAMERA_TYPE) {
    p_cameraButton->SetBitmapLabel(isChecked? p_orthoBitmap:p_perspBitmap);
  }
  else if (id == ViewerEvtHandler::ID_AUTO_NORMALIZE) {
    p_centerLockButton->SetBitmapLabel(isChecked? p_lockBitmap:p_unlockBitmap);
  }
  else if (p_modeToolbar->FindById(id)) {
    p_modeToolbar->ToggleTool(id, isChecked);
  }
  else if (p_viewToolbar->FindById(id)) {
    p_viewToolbar->ToggleTool(id, isChecked);

    if (id == ViewerEvtHandler::ID_DEPTH_CUEING) {
      ewxBitmap enableBitmap("depthcue.png", wxBITMAP_TYPE_PNG);
      ewxBitmap disableBitmap("depthcue_disabled.png", wxBITMAP_TYPE_PNG);
      p_viewToolbar->SetToolNormalBitmap(ViewerEvtHandler::ID_DEPTH_CUEING,
                                         isChecked?disableBitmap:enableBitmap);
    }

  }
  else if (p_styleToolbar->FindById(id)) {
    p_styleToolbar->ToggleTool(id, isChecked);
  }
}


bool Builder::getToolState(int id) const
{
  wxMenuItem * item = GetMenuBar()->FindItem(id);
  if (item && item->IsCheckable())
    return item->IsChecked();
  return false;
}


void Builder::pertabPrefsMCB()
{
  updateElementIcon();
  p_sgMgr->loadAtomColors();  
  p_pertab->setElements();
}


void Builder::urlRenameMCB(JMSMessage& msg)
{
  string oldURL = msg.getProperty("oldurl");
  string newURL = msg.getProperty("newurl");
  
  bool needUpdate = false;
  // if any of our calculations were affected by the rename
  // update the resource pool
  map<string, IPropCalculation*>::iterator iter;
  for (iter = p_calculations.begin(); iter != p_calculations.end(); iter++) {
    if (EcceURL(iter->first).isChildOrMe(oldURL)) {
      EDSIFactory::renamePoolResource(oldURL, newURL);
      needUpdate = true;
      break;
    }
  }

  if (!needUpdate) return;

  // one or more of our calculations were affected by the rename
  // note that if we have a Resource*, its URL already reflects the alteration
  for (iter = p_calculations.begin(); iter != p_calculations.end(); /*no inc*/) {
    if (EcceURL(iter->first).isChildOrMe(oldURL)) {
      string oldContext = iter->first;
      setContext(oldContext);
      wxLogWarning("The current calculation or one of its ancestors "
                   "was renamed.");
      if (dynamic_cast<Resource*>(p_calculation)) {
        string newContext = p_calculation->getURL().toString();
        p_calculations[newContext] = p_calculation;
        p_calculations.erase(iter++);
        ewxTool::setContext(newContext);
        p_contextHistory->RenameContext(oldContext, newContext);
        p_contextPanel->RenameContext(oldContext, newContext);
        p_commandManagers[newContext] = p_commandManagers[oldContext];
        p_commandManagers.erase(oldContext);
        p_sgMgr->renameSceneGraph(oldContext, newContext);
      } else {
        // TODO What if we have a non-Resource?  Would we event get a rename MCB?
      }
    } else {
      iter++;
    }
  }
}


void Builder::urlRemoveMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");

  bool needUpdate = false;
  map<string, IPropCalculation*>::iterator iter;
  for (iter = p_calculations.begin(); iter != p_calculations.end(); /*no inc*/) {
    if (EcceURL(iter->first).isChildOrMe(urlstr)) {
      needUpdate = true;
      string oldContext = iter->first;
      setContext(iter->first);
      if (dynamic_cast<Resource*>(p_calculation)) {
        // create a new DefaultCalculation, but maintain the same SG
        // and command manager
        IPropCalculation *calc = new DefaultCalculation();
        string newContext = calc->getURL().toString();
        p_calculations[newContext] = calc;
        p_calculations.erase(iter++);
        p_contextHistory->RenameContext(oldContext, newContext);
        p_contextPanel->RenameContext(oldContext, newContext);
        p_commandManagers[newContext] = p_commandManagers[oldContext];
        p_commandManagers.erase(oldContext);
        p_sgMgr->renameSceneGraph(oldContext, newContext);
        setContext(newContext);
        wxLogWarning("%s or one of its ancestors was removed. "
                     "The chemical system was transferred to %s.",
                     oldContext.c_str(), newContext.c_str());
      } else {
        // TODO What if we have a non-Resource?  Would we event get a remove MCB?
      }
    } else {
      iter++;
    }
  }

  if (needUpdate) EDSIFactory::removePoolResource(urlstr);
}


/**
 * MCB for runstate changes.
 * Only updates UI if the changed IPropCalculation is the current context.
 */
void Builder::urlStateMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");

  // if any of our calculations were affected by the state change
  // update the resource pool
  map<string, IPropCalculation*>::iterator iter = p_calculations.find(urlstr);
  if (iter == p_calculations.end()) return;

  IPropCalculation *calc = iter->second;
  TaskJob * taskjob = dynamic_cast<TaskJob*>(calc);
  if (taskjob) {
    EDSIFactory::changePoolResource(urlstr);
    ResourceDescriptor::RUNSTATE state = taskjob->getState();
    if (state < ResourceDescriptor::STATE_SUBMITTED) {
      removePropertyPanels(urlstr);
    } else if (state > ResourceDescriptor::STATE_READY) {
      set<PropertyPanel*> panels = PropertyPanel::getPanels(urlstr);
      set<PropertyPanel*>::iterator panel;
      for (panel = panels.begin(); panel != panels.end(); ++panel) {
        (*panel)->stateChange(state);
      }
    }
  }

  // let's not forget to refresh the UI if the current experiment changed state
  if (p_calculation->getURL() == urlstr) {
    // overkill?  At the very least it calls updatePropertyMenus
    // which will find and create any missing ones
    updateUI(); // overkill?
  }
}


/**
 * A property was modified or a new property created.  Notify
 * all related PropertyPanels.
 */
void Builder::propertyChangeMCB(JMSMessage& msg)
{
  string urlstr = msg.getProperty("url");
  //cout << "Builder::propertyChangeMCB" << endl;
  //cout << "url   = " << urlstr << endl;
  //cout << "name  = " << msg.getProperty("name") << endl;
  //cout << "value = " << msg.getProperty("value") << endl;

  // Don't process it if we are the ones that sent it.
  // This happens with the "isReviewed" property.
  if (msg.getSender().getID() == getMyID()) return;

  // See if this change applies to one of our current calcs
  map<string, IPropCalculation*>::iterator iter = p_calculations.find(urlstr);
  if (iter == p_calculations.end()) return;

  bool needUpate = false;
  IPropCalculation *calc = iter->second;
  string key = msg.getProperty("name");
  string value = msg.getProperty("value");

  string token;
  StringTokenizer tokenizer(key);
  vector<string> keylist;
  while (!(token=tokenizer.next()).empty()) {
    keylist.push_back(token);
  }

  // if we don't have a property key, no need to continue
  if (keylist.empty()) return;

  key = keylist[0];
  calc->updateProperty(key, value);
  set<PropertyPanel*> panels =
          PropertyPanel::getPanelsForPropertyName(urlstr, key);
  set<PropertyPanel*>::iterator panel;
  for (panel = panels.begin(); panel != panels.end(); ++panel) {
    (*panel)->propertyUpdate(msg.getProperty("name"));
  }
  PropertyPanelFactory &ppf = PropertyPanelFactory::getPropertyPanelFactory();
  set<string> names = ppf.getPanelNamesForProperties(vector<string>(1,key));
  set<string>::iterator name;
  for (name = names.begin(); name != names.end(); ++name) {
    if (!PropertyPanel::panelExists(urlstr, *name)) {
      // In this case, we don't have one of these yet
      // so we'll have to add it.

      // GDB 2/26/13  Don't create a property panel for the Metadynamics plots
      // here at all.  It ends up crashing out and the one in
      // updatePropertyMenus works properly and is called both running a live
      // calculation and invoking the viewer on a completed calculation.  For
      // a live calculation this is because the property panel is only
      // created at the very end of the run since it involves parsing a file
      // other than the primary one.  Lucked out on that as far as simplifying
      // the hack to just this spot.
      if (name->find("Metadynamics Potential")==string::npos &&
          calc->getProperty(key)) {
      //if (calc->getProperty(key)) {
        createPropertyPanel(*name);
        needUpate = true;
      }
    }
  }

  if (needUpate) {
    p_mgr.Update();
  }
}


void Builder::deleteSelection()
{

   Command *cmd = new DeleteCmd("Delete Selection", getSG());
   execute(cmd);
}


void Builder::quit(const bool& allowCancel)
{
  int ret;
  long buttonFlags = wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION;
  buttonFlags |= allowCancel ? wxCANCEL : 0;
  ewxMessageDialog dlg(this, "The current calculation has unsaved changes!  "
                       "Do you want to save changes before quitting?",
                       "Save Builder Changes?", buttonFlags);

  map<string, IPropCalculation*>::iterator iter;
  for (iter = p_calculations.begin(); iter != p_calculations.end(); iter++) {
    IPropCalculation *calc = iter->second;
    if (isDirty(calc)) {
      setContext(iter->first); // to show GUI to user
      ret = dlg.ShowModal();
      if (ret == wxID_YES) {
        if (dynamic_cast<DefaultCalculation*>(p_calculation)) {
          doSaveAs();
        } else {
          doSave();
        }
      }
      if (ret == wxID_CANCEL) {
        break;
      }
    }
    // delete the entire calculation if it was a transient import
    if (isImport(calc)) {
      EDSI *edsi = EDSIFactory::getEDSI(calc->getURL());
      edsi->removeResource();
    }
  }

  // unset focus of all viz prop panels
  // this fixes seg fault if we're animating a trj, geom trace, vib, etc
  set<VizPropertyPanel*> panels = VizPropertyPanel::getPanels();
  set<VizPropertyPanel*>::iterator panel;
  for (panel = panels.begin(); panel != panels.end(); ++panel) {
    (*panel)->setFocus(false);
  }
  ::wxYieldIfNeeded();

  if (ret != wxID_CANCEL) {
    // GDB 1/22/09 Tired of seg faults so lets see if we can exit ungracefully
    // to avoid that annoyance
    // Destroy();
    exit(0);
  }
}

/**
 * Does solvent/solute mixed display style if there is solvent
 */
bool Builder::createSolventSoluteStyles(SGFragment& frag)
{
   bool ret = false;

   int lastSoluteAtom = frag.getFirstSolventAtomIndex() - 1;
   if (lastSoluteAtom > 0) {
      ret = true;

      ewxConfig *config = ewxConfig::getConfig("vizstyles.ini");
      int numAtoms = frag.numAtoms();
      int i;

      for (i=0; i<=lastSoluteAtom; i++) {
         frag.m_atomHighLight.push_back(i);
         const vector<TBond*>& abonds = frag.atomRef(i)->bondList();
         for (unsigned int bdx=0; bdx<abonds.size(); bdx++) {
            frag.m_bondHighLight.push_back(abonds[bdx]->index());
         }
      }

      Command *cmd = new CSStyleCmd("Set style", &getSceneGraph());

      // Solute atoms will be made CPK only if they are wireframe
      // First, check for any wireframe style
      bool isWire = false;
      for (i=0; i<=lastSoluteAtom; i++) {
        if (frag.atomRef(i)->displayStyle().getStyle()
                  == DisplayStyle::WIRE) {
          isWire = true;
          break;
        }
      }
      // Only set the descriptor parameters if the display style is
      // being overriden to CPK.  Otherwise, render in the current style
      if (isWire) {
         string cpkconfig = config->Read("/CPK").c_str();
         DisplayDescriptor *cpk = 0;
         if (cpkconfig != "") {
            cpk = new DisplayDescriptor(cpkconfig);
         } else {
            cpk = new DisplayDescriptor("Solute", "CPK", "Element");
         }
         cmd->getParameter("descriptor")->setString(cpk->toString());
         delete cpk;
      } else {
         // Restore from styles preferences
         ewxConfig *wxbuilder_ini = ewxConfig::getConfig("wxbuilder.ini");
         string style = wxbuilder_ini->Read("/DefaultStyle", "CPK").c_str();
         string styledd = wxbuilder_ini->Read(style).c_str();
         string scheme = "Element";
         DisplayDescriptor *dd = 0;
         if (styledd != "")
           dd = new DisplayDescriptor(styledd);
         else
           dd = new DisplayDescriptor("default", style, scheme);
         if (!dd->isValid(dd->getStyle(),dd->getColorScheme(),frag.numResidues()>0))
         {
           dd->setColorScheme(dd->getDefaultColorScheme(dd->getStyle()));
         }
         cmd->getParameter("descriptor")->setString(dd->toString());
         delete dd;
      }

      cmd->getParameter("all")->setBoolean(false);
      //execute(cmd);
      cmd->execute();
      frag.m_atomHighLight.clear();
      frag.m_bondHighLight.clear();

      //
      // Solvent atoms will be made wireframe
      //
      DisplayDescriptor *wire = 0;
      string wireconfig = config->Read("/Wireframe").c_str();
      if (wireconfig != "") {
         wire = new DisplayDescriptor(wireconfig.c_str());
      } else {
         wire = new DisplayDescriptor("Solvent", "Wireframe", "Element");
      }
      for (i=lastSoluteAtom+1; i<numAtoms; i++) {
         frag.m_atomHighLight.push_back(i);
         const vector<TBond*>& bonds = frag.atomRef(i)->bondList();
         for (unsigned int bdx=0; bdx<bonds.size(); bdx++) {
            frag.m_bondHighLight.push_back(bonds[bdx]->index());
         }
      }
      cmd = new CSStyleCmd("Set style", &getSceneGraph());
      cmd->getParameter("descriptor")->setString(wire->toString());
      cmd->getParameter("all")->setBoolean(false);
      //execute(cmd);
      cmd->execute();
      delete wire;
      frag.m_atomHighLight.clear();
      frag.m_bondHighLight.clear();
   } else {

      /*
      // Not sure we need to do this
      DisplayStyle mainstyle = frag.getMainDisplayStyle();
      ewxConfig *wxbuilder_ini = ewxConfig::getConfig("wxbuilder.ini");
      string style = wxbuilder_ini->Read(style).c_str();
      DisplayDescriptor styledd(style);
      Command *cmd = new CSStyleCmd("Set style", &getSceneGraph());
      cmd->getParameter("descriptor")->setString(styledd.toString());
      cmd->getParameter("all")->setBoolean(true);
      cmd->execute();
      */
   }

   return ret;
}



/**
 */
void Builder::moveStartCB(void *userData, SoDragger *dragger)
{
  Builder *fw = (Builder *)userData;
  FragAssignCmd * cmd = new FragAssignCmd("Move Atoms",
                                          &(fw->getSceneGraph()));
  fw->execute(cmd);
}


/**
 * This is invoked when the user does a MB release (ie they are finished
 * rotating.  This gives us a chance to make sure the values in the table
 * are up to date.  See moveStartMoveCB().  It cannot update the
 * table each time for performance reasons.
 */
void Builder::moveEndCB(void *userData, SoDragger *dragger)
{
  EventDispatcher::getDispatcher().publish(Event("GeomChange"));
}


/**
 * Respond to the move callback from the dragger manipulator.
 */
void Builder::moveCB(void *userData, SoDragger *dragger)
{
  SGContainer & sg =  ((Builder * ) userData)->getSceneGraph();
  SGFragment *frag = sg.getFragment();

  if (frag->getNumberOfAtoms() > 0) {
    AtomRTDragger *  dd = (AtomRTDragger *) dragger;

    // translate atoms
    int at;
    SbVec3f atom, vec_t;
    double coords[3];
    float x,y,z;
    SbRotation rot_t;

    int cnt = dd->m_selected.size();
    for (int j=0 ; j < cnt  ; j++) {
      at = ((Fragment*)frag)->atomIndex(*( dd->m_selected[j]));
      atom = frag->getAtomCoordinates(at);
      vec_t = dd->translation.getValue();

      atom = atom -  dd->m_save_scale * dd->m_old_trans;
      atom  = atom - dd->m_center;

      rot_t =  dd->rotation.getValue();
      dd->m_old_rot.multVec(atom,atom);
      rot_t.multVec(atom,atom);

      atom = atom + dd->m_center;
      atom = atom  +  dd->m_save_scale *  vec_t;

      atom.getValue(x, y, z);
      coords[0] = (double)x;
      coords[1] = (double)y;
      coords[2] = (double)z;
      frag->changeCoords(at, coords);
    }
    dd->m_old_trans = dd->translation.getValue();
    dd->m_old_rot = rot_t.invert();
    sg.touchChemDisplay();

    dd->m_alert = true;
    MoveAction act;
    act.apply(sg.getTopRotManip());
    act.apply(sg.getTopManip());
    act.apply(sg.getTopLabel());
    act.apply(sg.getTopLine());
    dd->m_alert = false;
  }

  // TODO Update the text table
  /*
    static int cnt = 0;
    if (cnt++ > updateInterval) {
    if (vrExpt->getTable()) vrExpt->getTable()->refresh();
    cnt=0;
    }
  */
}


/**
 * Unless this become too costly, add undo object for every rotate
 * action.
 */
void Builder::rotateStartCB(void *userData, SoDragger *dragger)
{
  Builder *fw = (Builder *)userData;
  FragAssignCmd * cmd = new FragAssignCmd("Rotate Atoms",
                                          &(fw->getSceneGraph()));
  fw->execute(cmd);
}


/**
 * This is invoked when the user does a MB release (ie they are finished
 * rotating.  This gives us a chance to make sure the values in the table
 * are up to date.  See insertMoveAtoms_moveCB().  It cannot update the
 * table each time for performance reasons.
 */
void Builder::rotateEndCB(void *userData, SoDragger *dragger)
{
  EventDispatcher::getDispatcher().publish(Event("GeomChange"));
}


/**
 * Respond to the move callback from the dragger manipulator.
 */
void Builder::rotateCB(void *userData, SoDragger *dragger)
{
  SGContainer & sg =  ((Builder * ) userData)->getSceneGraph();
  SGFragment *frag = sg.getFragment();

  if (frag) {
    AtomRotDragger *  dd = (AtomRotDragger *) dragger;

    SbRotation rot_t;
    SbVec3f axis;
    float ang,x,y,z;

    rot_t =  dd->rotation.getValue();

    rot_t.getValue(axis,ang);
    ang = ang*180/M_PI;
    axis.getValue(x,y,z);
    if (z < 0) ang = 360 -ang;

    frag->rotateAboutBond(dd->atom1(),dd->atom2(),
                          (double)(ang-dd->getRotAngle()));

    MoveAction act;
    act.apply(sg.getTopRotManip());
    act.apply(sg.getTopManip());
    act.apply(sg.getTopLabel());
    act.apply(sg.getTopLine());

    dd->setRotAngle(ang);
    sg.touchChemDisplay();
  }
}

///////////////////// WxCalcImportClient virtuals ///////////////////////////

///////////////////////////////////////////////////////////////////////////
// Create an IPropCalculation object from the current url.  This is where
// the imported calc will be located.
///////////////////////////////////////////////////////////////////////////
TaskJob *Builder::getContainer(const string& name)
{
  TaskJob *ret = 0;

  // Create imported calculation within the user's home folder
  EDSIServerCentral servers;
  EcceURL userurl = servers.getDefaultUserHome();
  Resource *parentRes = EDSIFactory::getResource(userurl);

  if (parentRes != 0) {
    Resource *childRes = 0;

    try {
      childRes = parentRes->createChild(name,
              ResourceDescriptor::RT_VIRTUAL_DOCUMENT,
              ResourceDescriptor::CT_CALCULATION,
              ResourceDescriptor::AT_UNDEFINED);
    }
    catch (InvalidException& ex) {
      // create resource failed, let it fall through
      //cerr << "EDSIFactory::createResource failed :: " << ex.what() << endl;
    }

    if (childRes != 0) {
      ret = dynamic_cast<TaskJob*>(childRes);
    }
  }

  return ret;
}


void Builder::importValidationComplete(TaskJob *ipc, bool status,
                                       string message)
{
  if (!status) {
    if (ipc != 0) {
      // remove the problem calculation
      EDSI *edsi = EDSIFactory::getEDSI(ipc->getURL());
      edsi->removeResource();
    }
    showMessage(message, true);
  } else {
    if (message != "")
      showMessage(message, false);

    message = "Calculation output currently being imported--Viewer will be slow to respond.";
    showMessage(message, false);

    // set context to the calculation being imported
    if (CalculationFactory::canOpen(ipc->getURL())) {
      setContext(ipc->getURL());
      // override default import flag setting of false
      setImport(true);
    }
  }
}

