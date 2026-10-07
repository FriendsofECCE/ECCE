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

#include <wx/display.h>
using std::vector;
  
#include <math.h>
#include <unistd.h>

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
#include "wxgui/WxHelpViewer.H"
#include "util/CancelException.H"
#include "util/CommandWrapper.H"
#include "wxviz/ImageConverter.H"
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
#include "wxgui/ewxWindowUtils.H"
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
#include <wx/bmpcbox.h>
#include <wx/listctrl.h>
#include <wx/generic/filectrlg.h>
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

#include "tdat/SingleGrid.H"
#include "viz/AtomMeasureAngle.H"
#include "viz/AtomMeasureDist.H"
#include "viz/AtomMeasureTorsion.H"
#include "viz/AtomNodesInit.H"
#include "viz/AtomRTDragger.H"
#include "viz/AtomRotDragger.H"
#include "viz/NodesInit.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/PropSGFragment.H"
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
#include "wxviz/SceneScript.H"
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
#include "PropertyIndexPanel.H"
#include "PropertyPanel.H"
#include "Cube.H"
#include "MoPanel.H"
#include "tdat/PropVector.H"
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
    EVT_MENU( ID_TOOL_RESETVIEW, Builder::OnToolResetViewClick )
    EVT_BUTTON( ID_TOOL_SETHOME, Builder::OnToolSethomeClick )
    EVT_MENU( ID_TOOL_STYLE, Builder::OnToolStyleClick )
    EVT_MENU( ID_TOOL_TRANSLATEM, Builder::OnToolTranslatemClick )
    EVT_MENU_RANGE( ID_TOOLMENU_ITEM, ID_TOOLMENU_ITEM+199,
                    Builder::OnToolMenuClick)
    EVT_MENU_RANGE( ID_CONTEXT_HISTORY_BASE, ID_CONTEXT_HISTORY_BASE+99,
                    Builder::OnContextRadioClick )
    EVT_LIST_ITEM_SELECTED( ContextPanel::ID_LIST, Builder::OnContextListClick )
    EVT_EWXAUI_PANE_CLOSE(Builder::OnPaneClose)
    // Stock wx3.2 replacement for the dropped ewxAUI "take focus" caption
    // button (see EwxAuiCompat.H) -- wxChildFocusEvent is a wxCommandEvent
    // and so bubbles up the window hierarchy to Builder (the top-level
    // managed frame) whenever ANY descendant control anywhere in a pane
    // receives keyboard focus (clicking into it, tabbing into it, etc),
    // giving a real, always-available trigger requiring no custom UI.
    // NOTE: this is bound here rather than relying on wxAuiManager's own
    // EVT_AUI_PANE_ACTIVATED/OnChildFocus, because wxAuiManager::GetPane()
    // requires an EXACT match against the pane's own top window
    // (framemanager.cpp's GetPane(wxWindow*): "if (p.window == window)",
    // no ancestor walk) -- so it only ever fires when the pane's own bare
    // panel object receives focus directly, never when focus lands on a
    // control nested inside it (a grid, listbox, etc), which is the
    // overwhelmingly common case. Walking up from the focused window
    // ourselves in OnChildFocus below avoids that exact-match miss.
    EVT_CHILD_FOCUS(Builder::OnChildFocus)
    EVT_EWXAUI_PANE_TAKE_FOCUS(Builder::OnPaneTakeFocus)
    EVT_EWXAUI_PANE_ADD_FOCUS(Builder::OnPaneAddFocus)
    EVT_EWXAUI_PANE_OPTIONS(Builder::OnPaneOptions)
    // ...and the trigger that actually reaches OnPaneOptions' work now
    // that the ewxAUI options caption button is gone: a right-click on
    // any property panel. wxContextMenuEvent propagates up to this
    // frame, and OnPanelContextMenu walks back down the parent chain
    // to find the owning TearableContentProvider, for the same reason
    // OnChildFocus does -- the click lands on a nested control.
    EVT_CONTEXT_MENU(Builder::OnPanelContextMenu)
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

//  Forward declarations: definitions (and their rationale) are with
//  addToolPanel() below, but OnToolMenuClick() needs to call
//  debugPrintPaneSizes() earlier in the file.
static int contentMinWidth(wxWindow *window);
static int contentFixedHeight(wxWindow *window);
static const int FIXED_PANE_HEIGHT_FALLBACK = 150;
static void debugPrintPaneSizes(wxAuiManager &mgr);


namespace {

//  Scoped "we are building the property panels, not the user clicking
//  around in them" flag, for OnChildFocus() to test.  Same idea, and for
//  the same class of bug, as AtomTable's p_internalSelect and
//  PartialCharges' -- a control that fires a genuine event while it is
//  being populated, and a handler with no way to tell that from real user
//  input.  Here the event is wxChildFocusEvent and the "real user input"
//  it is mistaken for is "the user wants this property's overlay in the
//  3-D viewer" (#111).
class PanelBuildGuard
{
  public:
    explicit PanelBuildGuard(int& depth) : p_depth(depth) { ++p_depth; }
    ~PanelBuildGuard() { --p_depth; }
  private:
    PanelBuildGuard(const PanelBuildGuard&);
    PanelBuildGuard& operator=(const PanelBuildGuard&);
    int& p_depth;
};

}  // namespace


const string Builder::NAME_TOOL_CONTEXT(_("Open structures"));
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

const string Builder::NAME_COLUMN_TABS("Panel Tabs");
const string Builder::NAME_PROPERTY_INDEX("Property Index");
const string Builder::NAME_COLUMN_TOGGLE("Column Toggle");

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
    p_propertyPanelInfo(),
    p_panelBuildDepth(0)
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
    p_propertyPanelInfo(),
    p_panelBuildDepth(0)
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
  BuilderDockArt *art = new BuilderDockArt;
  art->setFoldedQuery([this](wxWindow *w) { return p_folded.count(w) > 0; });
  p_mgr.SetArtProvider(art);
  p_mgr.Bind(wxEVT_AUI_PANE_BUTTON, &Builder::OnPaneButton, this);
  
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

  initPanelMode();
  createMenus();
  createViewMenu();
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
  ewxWindowUtils::setToolIcon(this, BUILDER);

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
    wxLogMessage("%s", msg.c_str());
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
    p_fileToolbar->AddTool(wxID_NEW, _T(""), ewxBitmap::bundle("filenew.png", wxBITMAP_TYPE_PNG), _("New"));
    p_fileToolbar->AddTool(wxID_OPEN, _T(""), ewxBitmap::bundle("fileopen.png", wxBITMAP_TYPE_PNG), _("Open"));
    p_fileToolbar->AddTool(wxID_SAVE, _T(""), ewxBitmap::bundle("filesave.png", wxBITMAP_TYPE_PNG), _("Save"));
    p_fileToolbar->AddTool(wxID_SAVEAS, _T(""), ewxBitmap::bundle("filesaveas.png", wxBITMAP_TYPE_PNG), _("Save As"));
    p_fileToolbar->AddTool(wxID_UNDO, _T(""), ewxBitmap::bundle("undo.png", wxBITMAP_TYPE_PNG), _("Undo"));
    p_fileToolbar->AddTool(wxID_REDO, _T(""), ewxBitmap::bundle("redo.png", wxBITMAP_TYPE_PNG), _("Redo"));

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
    wxBitmap resetIcon = ewxBitmap::themedIcon("go-home", wxSize(22, 22));
    if (!resetIcon.IsOk()) resetIcon = ewxBitmap("home_view16.png", wxBITMAP_TYPE_PNG);
    p_viewToolbar->AddTool(ID_TOOL_RESETVIEW, _T(""), resetIcon,
            _("Reset view (Home)"));

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
  {
    // Next to View All; "Home" is the key Open Inventor viewers use for it.
    size_t pos = 0;
    if (p_renderMenu->FindChildItem(wxID_ZOOM_FIT, &pos)) pos++;
    else pos = p_renderMenu->GetMenuItemCount();
    p_renderMenu->Insert(pos, ID_TOOL_RESETVIEW, _("Reset View\tHome"),
                          _("Undo rotation and zoom; atoms are not changed"));
  }

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
  //  Dragging a floating panel back needs a dock target, and that drag is
  //  unreliable under GTK and impossible on Wayland (#53).
  p_toolMenu->Append(ID_DOCK_FLOATING_PANELS, _("Dock Floating Panels"),
                     _("Return every floating panel to its dock"));
  Bind(wxEVT_MENU, &Builder::OnDockFloatingPanels, this,
       ID_DOCK_FLOATING_PANELS);

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

  p_mgr.AddPane(mainPanel, wxAuiPaneInfo().CenterPane().
          Name("Viewer").Caption("Viewer").CaptionVisible().
          Show().MaximizeButton()
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
  // alwaysFixed=false: Build's fixed size (the default) is locked in at
  // construction time and never revisited, but its content (12 element
  // buttons + Add H/Del H row) can need more vertical room than that --
  // seen live as buttons clipped/inaccessible with no way to resize the
  // pane to reveal them. Same fix already applied to Atom Table/Residue
  // Table/Log below, for the same reason.
  addToolPanel(p_pertab, NAME_TOOL_BUILD, true, false);

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
  //  In the one-column layouts the log lives collapsed to its caption bar
  //  and opens by itself when something is written to it.
  logWindow->setMessageHandler([this](wxLogLevel) {
    CallAfter([this]() {
      if (isColumnMode()) {
        wxAuiPaneInfo &pane = p_mgr.GetPane(NAME_TOOL_LOG);
        if (pane.IsOk() && pane.IsShown() && p_folded.count(pane.window)) {
          foldPane(pane.window, false);
          updatePanes();
        }
      }
    });
  });

  if (getenv("ECCE_DEVELOPER")) {
    p_toolMenu->AppendCheckItem(ID_SHOW_CMD, NAME_TOOL_COMMAND_LINE, "");
  }

  p_structLib = new StructLib(this);
  p_structLib->configure();
  wxAuiPaneInfo pinfo;
  pinfo.Name(NAME_TOOL_STRUCTLIB).Caption(NAME_TOOL_STRUCTLIB)
          .Show(false).CaptionVisible(true).Right().Resizable(true);
  p_mgr.AddPane(p_structLib, pinfo);
  p_structureNames.insert(NAME_TOOL_STRUCTLIB);

  //  The two tab buttons that head the one-column layouts, and the list
  //  that heads the Properties tab in the list + detail layout.  Both are
  //  ordinary panes, hidden unless the layout uses them.
  p_tabsPanel = new wxPanel(this, wxID_ANY);
  wxBoxSizer *tabRow = new wxBoxSizer(wxHORIZONTAL);
  const char *tabNames[2] = { "Structure", "Properties" };
  for (int t = 0; t < 2; ++t) {
    p_tabButtons[t] = new wxToggleButton(p_tabsPanel, wxID_ANY, tabNames[t]);
    p_tabButtons[t]->Bind(wxEVT_TOGGLEBUTTON, [this, t](wxCommandEvent&) {
      CallAfter([this, t]() { setColumnTab(t, true); });
    });
    tabRow->Add(p_tabButtons[t], 1, wxEXPAND | wxALL, 2);
  }
  p_tabsPanel->SetSizer(tabRow);
  p_tabsPanel->SetMinSize(wxSize(200, 34));
  wxAuiPaneInfo tabInfo;
  tabInfo.Name(NAME_COLUMN_TABS).CaptionVisible(false).Right().Layer(1)
         .Position(0).Fixed().Floatable(false).Movable(false)
         .CloseButton(false).PaneBorder(false).Gripper(false)
         .MinSize(p_tabsPanel->GetMinSize())
         .BestSize(p_tabsPanel->GetMinSize()).Show(false);
  tabInfo.dock_proportion = 1;
  p_mgr.AddPane(p_tabsPanel, tabInfo);

  p_index = new PropertyIndexPanel(this);
  p_index->setSelectHandler([this](const string& name) {
    CallAfter([this, name]() { selectDetail(name); });
  });
  wxAuiPaneInfo indexInfo;
  indexInfo.Name(NAME_PROPERTY_INDEX).Caption("Properties").CaptionVisible(true)
           .Right().Layer(1).Position(1).Resizable(true).CloseButton(false)
           .Floatable(false).MinSize(wxSize(200, 150))
           .BestSize(wxSize(400, 230)).Show(false);
  indexInfo.dock_proportion = INDEX_PROPORTION;
  p_mgr.AddPane(p_index, indexInfo);

  //  The arrow on the border between the viewer and the column: a thin
  //  fixed pane of its own in the innermost right-hand layer, so it stays
  //  put when the column behind it is hidden.
  p_togglePanel = new wxPanel(this, wxID_ANY);
  p_toggleButton = new wxBitmapButton(p_togglePanel, wxID_ANY,
      wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_BUTTON),
      wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
  p_toggleButton->SetToolTip(_("Hide the side panels"));
  p_toggleButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    CallAfter([this]() { setColumnCollapsed(!p_columnHidden); });
  });
  wxBoxSizer *toggleCol = new wxBoxSizer(wxVERTICAL);
  toggleCol->AddStretchSpacer(1);
  toggleCol->Add(p_toggleButton, 0, wxALIGN_CENTER_HORIZONTAL);
  toggleCol->AddStretchSpacer(1);
  p_togglePanel->SetSizer(toggleCol);
  const wxSize toggleSize(p_toggleButton->GetBestSize().x + 2, 40);
  p_togglePanel->SetMinSize(toggleSize);
  wxAuiPaneInfo toggleInfo;
  toggleInfo.Name(NAME_COLUMN_TOGGLE).CaptionVisible(false).Right().Layer(0)
            .Position(0).Fixed().Floatable(false).Movable(false)
            .CloseButton(false).PaneBorder(false).Gripper(false)
            .MinSize(toggleSize).BestSize(toggleSize);
  p_mgr.AddPane(p_togglePanel, toggleInfo);

  applyGeometry();
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
  // A local calculation is a directory (FileEDSI) anywhere on disk, a
  // structure a file; only a path that is neither is refused here.
  if (url != DefaultCalculation::nextURL() && EcceURL(url).isLocal()) {
    string path = EcceURL(url).getFile();
    if (!wxFileExists(path) && !wxDirExists(path)) return NULL;
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
Command *Builder::createCommand(const string& input)
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
Command * Builder::createModeCommand(int modeId)
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


int Builder::checkResidueEditing()
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
  //  Creating/re-docking this calculation's property panels must not
  //  hand any of them the 3-D viewer -- see OnChildFocus() (#111).
  PanelBuildGuard buildGuard(p_panelBuildDepth);
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
      wxAuiPaneInfo &pane = p_mgr.GetPane(*vizpanel);
      if (pane.IsOk()) {
        // NOTE: wxAuiPaneInfo::Focus() (ewxAUI addition) has no stock
        // wx3.2 equivalent and is dropped here - see EwxAuiCompat.H.
        p_propertyPanelInfo[pane.window] = paneInfoForSave(pane);
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
      wxAuiPaneInfo &info = p_mgr.GetPane(*panels.begin());
      if (info.IsOk()) {
        info.Show();
        if (p_panelMode == PANELS_DETAIL &&
            paneGroup(info) == GROUP_PROPERTIES) {
          setDetail(info.window);
        }
        p_stayCollapsed = true;   // opening a calculation is not a request
        updatePanes(true); // TODO is this needed yet or can it wait?
        p_stayCollapsed = false;
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

   //  saveWindowSettings() above already flushed once, but every write
   //  after that point -- savePaneLayout() included -- stays buffered in
   //  wxConfig's in-memory copy.  quit() below ends in _exit(0), which
   //  skips static destructors (see its own comment: exit() was tried
   //  and segfaulted in libwx_gtk3u_core's global teardown) and with them
   //  wxConfig's flush-on-destroy -- so a user's rearranged pane layout
   //  was never actually reaching wxbuilder.ini.  Flush explicitly.
   config->Flush();
}


void Builder::restoreSettings()
{
   ewxConfig * config = ewxConfig::getConfig("wxbuilder.ini");
   bool firstRun = !config->HasEntry("/Window/Width");
   restoreWindowSettings(config, true);

   //  First run: a share of the screen it opens on, so a desktop monitor
   //  gets a roomy window and a small remote-desktop one still fits.  A
   //  size the user chose is kept, but never larger than the screen.
   int displayIdx = wxDisplay::GetFromWindow(this);
   wxRect area = wxDisplay(displayIdx == wxNOT_FOUND ? 0 : displayIdx)
                   .GetClientArea();
   wxSize size = GetSize();
   if (getenv("ECCE_PANEL_FULLSCREEN")) {
     SetSize(area);
     Move(area.GetPosition());
   } else if (firstRun) {
     size = wxSize(area.width * 8 / 10, area.height * 8 / 10);
     SetSize(size);
     CentreOnScreen();
   } else if (size.x > area.width || size.y > area.height) {
     SetSize(wxSize(wxMin(size.x, area.width), wxMin(size.y, area.height)));
   }
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
     config->DeleteGroup("/PaneLayoutColumn");
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

  string path = dialog.GetPath().ToStdString();
  EcceURL url(path);
  if (CalculationFactory::canOpen(url)) {
    setContext(url);

  } else {
    string msg = "Cannot open " + url.toString() + " (to view the output "
                 "properties of a calculation, use File > Import "
                 "Calculation from Output File...).";
    wxLogWarning("%s", msg.c_str());
  }
}


void Builder::OnImportChemsysClick(wxCommandEvent& event)
{
  ImportCalculationDialog dialog(this);
  if (dialog.ShowModal() == wxID_CANCEL) return;
  importChemicalSystem(dialog.GetPath().ToStdString(), dialog.getType(),
                       dialog.getExt());
}


/**
 * Add Structure from File after the file dialog; ext is the file's
 * extension (case does not matter).
 */
void Builder::importChemicalSystem(const string& path, wxString type,
                                   wxString ext)
{
  ext.MakeUpper();

  EcceURL url(path);
  Resource *resource = EDSIFactory::getResource(url);
  if (resource == 0) {
    wxLogError("Cannot read %s", url.toString().c_str());
    return;
  }

  // create temporary file for fragment reading
  SFile *file = TempStorage::getTempFile();
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
      file->remove();
      delete file;
      return;
    }
  } else {
    if (resource->getDocument(file) == 0) {
      wxLogError("Cannot read %s", url.toString().c_str());
      file->remove();
      delete file;
      return;
    }
    file->move(file->pathroot() + "/" + resource->getName());
  }

  // now do the real work
  if (!readFragmentFromFile(file, type, ext)) {
    wxLogError("Could not import %s", url.toString().c_str());
    file->remove();
    delete file;
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
    wxLogError("%s", msg.c_str());
  } else {
    wxLogWarning("%s", msg.c_str());
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
      p_viewer->refreshRenderArea();
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
     undo->SetItemLabel(undoLabel+"\tCtrl+z");
     undo->Enable(true);
     p_fileToolbar->EnableTool(wxID_UNDO, true);
     p_fileToolbar->SetToolShortHelp(wxID_UNDO, undoLabel);
  } else {
     string undoLabel = "Can\'t Undo\tCtrl+z";
     undo->SetItemLabel(undoLabel);
     undo->Enable(false);
     p_fileToolbar->EnableTool(wxID_UNDO, false);
     p_fileToolbar->SetToolShortHelp(wxID_UNDO, "Can\'t Undo");
  }

  wxMenuItem *redo = GetMenuBar()->FindItem(wxID_REDO,0);
  if (getCM()->isRedoable()) {
    redo->Enable(true);
    string redoLabel = "Redo ";
    redoLabel += getCM()->getRedoLabel();
    redo->SetItemLabel(redoLabel+"\tCtrl+y");
    p_fileToolbar->EnableTool(wxID_REDO, true);
    p_fileToolbar->SetToolShortHelp(wxID_REDO, redoLabel);

  } else {
    redo->SetItemLabel("Can\'t Redo\tCtrl+y");
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
      wxLogWarning("%s", ex.what());
   }

}

void Builder::helpSupportMenuitemClick( wxCommandEvent& event )
{
    BrowserHelp help;
    help.showFeedbackPage();

}

void Builder::helpBuilderMenuitemClick( wxCommandEvent& event )
{
   WxHelpViewer::showKey("WxBuilder");

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
   //  quit() yields to the event loop (wxYieldIfNeeded) while it winds
   //  the property panels down, so a second click on the close box
   //  arrived as a second close event INSIDE the first quit() and ran the
   //  whole shutdown again -- reported as needing two or three clicks and
   //  then "free(): invalid pointer".  One shutdown at a time.
   static bool closing = false;
   if (closing) {
      if (event.CanVeto()) event.Veto();
      return;
   }
   closing = true;
   saveSettings();
   quit();
   closing = false;   // reached only when the user cancelled the quit
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
      p_mgr.GetPane("Residue Table").Show(true).Show(true);

      // If its a viz tool, call virtual refresh to force updating
      // Works around bug where we can't override show because it gets
      // called too much by AUI
      WxVizTool *panel = 
         dynamic_cast<WxVizTool*>(p_mgr.GetPane("Residue Table").window);
      if (panel) {
         panel->refresh();
      }

      updatePanes();
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
  string input = p_cmd->GetValue().ToStdString();
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
  wxAuiPaneInfo pinfo;
  pinfo.Name(p_calculation->getURL().toString()).
          Caption(p_calculation->getURL().toString()).
          PinButton(true).MaximizeButton(true).MinimizeButton(true).
          Left().Show(true);
  p_mgr.AddPane(viewer, pinfo);
  updatePanes();
}


void Builder::OnModeClick( wxCommandEvent& event )
{
  setMode(event.GetId());
  toggleModeButton(event.GetId());
  event.Skip();
}


void Builder::OnModeElementClick( wxCommandEvent& event )
{
  // Used to gate the popup on getToolState(ID_MODE_ATOM) && !event.
  // IsChecked() -- but getToolState() reads the mode MENU's radio item,
  // which is only synced by setMode()/toggleModeButton(), themselves
  // only reached via the ElementChanged->ShapeChanged chain published
  // below. So on the click that actually enters Atom mode for the first
  // time, getToolState() still reported the PREVIOUS mode and the
  // condition was false: the click silently entered Atom mode with no
  // chooser shown, and only a second, redundant click (once the toolbar
  // had caught up) passed the guard and popped it up (#158). Do both
  // unconditionally instead: publish ElementChanged (unchanged --
  // still what enters Atom mode, via Builder::eventMCB's ShapeChanged
  // handler) and always show the chooser, so the first click both
  // enters the mode and lets the user pick an element.
  Event evt("ElementChanged", p_currentElement);
  EventDispatcher::getDispatcher().publish(evt);

  PerTabPanel *pertab = new PerTabPanel(this, false,
          ID_ITEM_DEFAULT, false, true, true);
  pertab->SetName("Elements");
  TearableContent *tc = new TearableContent(pertab);
  tc->Position(wxGetMousePosition(), wxSize(1, 1));
  tc->Popup();
  toggleModeButton(event.GetId());
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
  // Same shape as OnModeElementClick's #158 fix above, found by
  // inspection (getToolState(ID_MODE_BOND) has the identical
  // stale-read-before-sync problem) -- fixed alongside it rather than
  // left to reproduce separately.
  Event evt("BondChanged", TBond::orderToString(p_currentBond));
  EventDispatcher::getDispatcher().publish(evt);

  BondDropDown *l = new BondDropDown(p_currentBond, this);
  TearableContent *tc = new TearableContent(l);
  l->connectToolKitFW(this);
  tc->Position(wxGetMousePosition(), wxSize(1, 1));
  tc->Popup();
  toggleModeButton(event.GetId());
}


void Builder::OnContextRadioClick( wxCommandEvent& event )
{
  if (p_contextHistory->SetContext(event.GetId())) {
    setContext(p_contextHistory->GetContext().ToStdString());
  }
}


void Builder::OnContextListClick( wxListEvent& event )
{
  setContext(event.GetText().ToStdString());
}


void Builder::OnPropertyMenuClick( wxCommandEvent& event )
{
  wxString name = p_propertyMenu->GetLabelText(event.GetId());
  wxAuiPaneInfo &pane = p_mgr.GetPane(name);
  if (!pane.IsOk()) {
    return;
  }
  wxWindow *win = pane.window;
  const bool docked = paneGroup(pane) == GROUP_PROPERTIES;
  if (!event.IsChecked()) {
    unfoldPane(pane);
    p_tabHidden.erase(win);
    if (p_detail == win) {
      p_detail = 0;
    }
    pane.Show(false);
  } else {
    pane.Show(true);
    if (docked && p_panelMode == PANELS_ACCORDION) {
      accordionNormalize(win);
    } else if (docked && p_panelMode == PANELS_DETAIL) {
      setDetail(win);
    }
  }
  updatePanes(true);
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

// Camera only: orientation back to the home orientation and the whole
// system in view.  Atom coordinates and edits are untouched.
void Builder::OnToolResetViewClick( wxCommandEvent& event )
{
  p_xrot->SetValue(0);
  p_yrot->SetValue(0);
  p_zrot->SetValue(0);
  p_viewer->resetToHomePosition();
  p_viewer->viewAll();
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
  wxAuiPaneInfo &pane = p_mgr.GetPane(GetMenuBar()->GetLabel(event.GetId()));
  pane.Show(event.IsChecked());
  if (!event.IsChecked()) {
    p_tabHidden.erase(pane.window);
  }
  // If its a viz tool, call virtual refresh to force updating
  // Works around bug where we can't override show because it gets
  // called too much by AUI
  WxVizTool *panel = dynamic_cast<WxVizTool*>(pane.window);
  if (panel && event.IsChecked()) {
    panel->refresh();
  }
  updatePanes(true);
  debugPrintPaneSizes(p_mgr);


  event.Skip();
}


// The caption's pin button folds a property pane to its caption bar.
// The resize is deferred so it never runs inside AUI's own event handling.
/**
 * Dock every floating pane on the side it came from.  wxAUI recreates a
 * dock side that has emptied, so this works with nothing left docked there.
 */
void Builder::OnDockFloatingPanels(wxCommandEvent& event)
{
  wxAuiPaneInfoArray& panes = p_mgr.GetAllPanes();
  bool changed = false;
  for (size_t i = 0; i < panes.GetCount(); ++i) {
    if (panes[i].IsFloating()) {
      panes[i].Dock();
      changed = true;
    }
  }
  if (changed) updatePanes();
}


void Builder::OnPaneButton(wxAuiManagerEvent& event)
{
  wxAuiPaneInfo *pane = event.GetPane();
  if (event.GetButton() != wxAUI_BUTTON_PIN || pane == 0 ||
      (dynamic_cast<PropertyPanel*>(pane->window) == 0 &&
       pane->name != NAME_TOOL_LOG)) {
    event.Skip();
    return;
  }
  wxWindow *win = pane->window;
  CallAfter([this, win]() { toggleFold(win); });
}


void Builder::toggleFold(wxWindow *win)
{
  wxAuiPaneInfo &pane = p_mgr.GetPane(win);
  if (!pane.IsOk()) {
    return;
  }
  const bool opening = p_folded.count(win) > 0;
  foldPane(win, !opening);
  if (opening && p_panelMode == PANELS_ACCORDION &&
      paneGroup(pane) == GROUP_PROPERTIES) {
    accordionNormalize(win);
  }
  updatePanes();
}


//  Folds a pane to its caption bar or opens it again.  Does not lay out:
//  the caller does, so several panes can change in one Update().
void Builder::foldPane(wxWindow *win, bool fold)
{
  wxAuiPaneInfo &pane = p_mgr.GetPane(win);
  if (!pane.IsOk() || fold == (p_folded.count(win) > 0)) {
    return;
  }
  if (!fold) {
    unfoldPane(pane);
    return;
  }
  FoldState st = { pane.best_size, pane.min_size, pane.IsResizable(),
                   pane.dock_proportion };
  p_folded[win] = st;
  pane.BestSize(wxSize(st.best.x, 1)).MinSize(wxSize(st.min.x, 1)).Fixed();
  pane.dock_proportion = 1;
}


// Puts a folded pane's sizes back without touching the manager's layout.
void Builder::unfoldPane(wxAuiPaneInfo &pane)
{
  map<wxWindow*, FoldState>::iterator it = p_folded.find(pane.window);
  if (it == p_folded.end()) {
    return;
  }
  pane.BestSize(it->second.best).MinSize(it->second.min)
      .Resizable(it->second.resizable);
  pane.dock_proportion = it->second.proportion;
  p_folded.erase(it);
}


// Folded state is never persisted: a saved 1px pane would reopen empty.
wxString Builder::paneInfoForSave(wxAuiPaneInfo &pane)
{
  map<wxWindow*, FoldState>::iterator it = p_folded.find(pane.window);
  if (it == p_folded.end()) {
    if (p_tabHidden.count(pane.window)) {
      wxAuiPaneInfo shown(pane);
      shown.Show(true);
      return p_mgr.SavePaneInfo(shown);
    }
    return p_mgr.SavePaneInfo(pane);
  }
  wxAuiPaneInfo copy(pane);
  if (p_tabHidden.count(pane.window)) {
    copy.Show(true);
  }
  copy.BestSize(it->second.best).MinSize(it->second.min)
      .Resizable(it->second.resizable);
  copy.dock_proportion = it->second.proportion;
  return p_mgr.SavePaneInfo(copy);
}


void Builder::OnPaneClose(wxAuiManagerEvent& event)
{
  if (event.pane->name == NAME_TOOL_STRUCTLIB) {
    setMode(ID_MODE_SELECT);
    toggleModeButton(ID_MODE_SELECT);
    //wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_MODE_SELECT);
    //AddPendingEvent(event);
  } else {
    wxString name(event.pane->name);
    wxWindow *win = event.pane->window;
    unfoldPane(*event.pane);
    p_tabHidden.erase(win);
    if (p_detail == win) {
      p_detail = 0;
    }
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
    wxAuiManagerEvent evt(wxEVT_EWXAUI_UPDATE);
    AddPendingEvent(evt);
  }
}


void Builder::OnChildFocus(wxChildFocusEvent& event)
{
  // See the comment on this event's binding in the event table above.
  // Walk up from whatever control just received focus until we find the
  // VizPropertyPanel that owns it (the focused window is usually a child
  // control nested inside the panel, not the panel itself), then treat
  // that exactly like the old ewxAUI "take focus" caption-button click.
  // setFocus(true) -> doFocus() already clears focus off any other viz
  // panel for the same calc, so no explicit loop is needed here.
  //
  //  TWO THINGS THIS MUST NOT MISTAKE FOR A USER CLICK (#111).  A panel
  //  taking viz focus is not cosmetic: receiveFocus() puts that property's
  //  overlay into the viewer -- vectors, charge colouring, an animated
  //  geometry trace -- so anything that reaches here without the user
  //  having asked for it ends up as a viewer full of overlapping overlays
  //  that have to be switched off one at a time.
  //
  //  1. Focus changes caused by BUILDING the panels.  updatePropertyMenus()
  //     creates a panel per property the calculation has data for, every
  //     time it gains one, and loadPaneLayout() detaches and re-docks the
  //     lot; both end in wxAuiManager::Update(), which shows, hides and
  //     reparents windows.  Whether GTK moves keyboard focus while that
  //     happens is not ours to decide and differs by version and display
  //     server -- measured as not happening under X11/GTK3 here, which is
  //     exactly why it must not be relied on.  p_panelBuildDepth says
  //     "this focus change is ours, not the user's".
  //  2. Focus landing in a pane that is not even on screen.  A hidden
  //     window should not be focusable at all, but AUI hides panes by
  //     hiding their window mid-Update(), so refuse it explicitly rather
  //     than trusting the ordering.
  //
  if (p_panelBuildDepth > 0) {
    event.Skip();
    return;
  }
  wxWindow *win = event.GetWindow();
  VizPropertyPanel *panel = NULL;
  while (win && !(panel = dynamic_cast<VizPropertyPanel*>(win))) {
    win = win->GetParent();
  }
  if (panel && panel->drawsInViewer() && !panel->hasFocus()) {
    wxAuiPaneInfo &pinfo = p_mgr.GetPane(panel);
    if (pinfo.IsOk() && pinfo.IsShown()) {
      panel->setFocus(true);
    }
  }
  event.Skip();
}


void Builder::OnPaneTakeFocus(wxAuiManagerEvent& event)
{
  wxBusyCursor c;

  // let the pane's window handle the event first
  if (event.pane->window && event.pane->window->GetEventHandler()->ProcessEvent(event)) {
    return;
  }
  // this will clear the focus on all viz panels while setting the given one
  VizPropertyPanel *panel;
  if ((panel = dynamic_cast<VizPropertyPanel*>(event.pane->window))) {
    // NOTE: wxAuiPaneInfo has no HasFocus() in stock wx3.2 (it was an
    // ewxAUI addition tied to the removed take-focus caption button, whose
    // click is what used to drive this handler - see EwxAuiCompat.H).
    bool gettingFocus = true;
    panel->setFocus(gettingFocus);
    // toggle it open if not already
    if (gettingFocus && !event.pane->IsShown()) {
      event.pane->Show();
    }
  }
}


void Builder::OnPaneAddFocus(wxAuiManagerEvent& event)
{
  wxBusyCursor c;

  // let the pane's window handle the event first
  if (event.pane->window && event.pane->window->GetEventHandler()->ProcessEvent(event)) {
    return;
  }
  // clear focus on all viz panels that aren't pinned and set the given one
  VizPropertyPanel *panel;
  if ((panel = dynamic_cast<VizPropertyPanel*>(event.pane->window))) {
    // NOTE: wxAuiPaneInfo has no IsPinned() in stock wx3.2 (it was an
    // ewxAUI addition tied to the removed add-focus/pin caption button,
    // whose click is what used to drive this handler - see
    // EwxAuiCompat.H).
    bool gettingFocus = true;
    panel->setPinned(gettingFocus);
    // toggle it open if not already
    if (gettingFocus && !event.pane->IsShown()) {
      event.pane->Show();
    }
  }
}


void Builder::OnPaneOptions(wxAuiManagerEvent& event)
{
  // let the pane's window handle the event first
  if (event.pane->window && event.pane->window->GetEventHandler()->ProcessEvent(event)) {
    return;
  }
  TearableContentProvider * tcp;
  if ((tcp = dynamic_cast<TearableContentProvider*>(event.pane->window))) {
    TearableContent * tc = new TearableContent(tcp->GetTearableContent());
    tc->Position(wxGetMousePosition(), wxSize(1,1));
    tc->Popup();
  }
}


void Builder::OnPanelContextMenu(wxContextMenuEvent& event)
{
  // Every TearableContentProvider's options menu became unreachable when
  // the wx3.2 AUI port dropped the custom ewxAUI pane-caption buttons (see
  // EwxAuiCompat.H): NModePanel's Show Table / Show Graph switch, and the
  // equivalents on MoPanel, PartialCharges, GeomTracePropertyPanel,
  // VecAtomSpectrum, VecAtomTensor and Cube. The menus themselves were
  // never removed -- nothing could open them. Restoring one trigger here
  // revives all of them at once rather than adding a control to each.
  wxWindow *win = wxDynamicCast(event.GetEventObject(), wxWindow);
  TearableContentProvider *tcp = NULL;
  while (win && !(tcp = dynamic_cast<TearableContentProvider*>(win))) {
    win = win->GetParent();
  }
  if (!tcp) {
    // Right-clicks anywhere else -- the 3-D viewer above all -- must keep
    // reaching whatever already handles them.
    event.Skip();
    return;
  }

  wxWindow *content = tcp->GetTearableContent();
  if (!content) {
    event.Skip();
    return;
  }
  TearableContent *tc = new TearableContent(content);
  tc->Position(wxGetMousePosition(), wxSize(1,1));
  tc->Popup();
}


void Builder::OnPaneOpen(wxAuiManagerEvent& event)
{
  wxBusyCursor c;

  // let the pane's window handle the event first
  if (event.pane->window && event.pane->window->GetEventHandler()->ProcessEvent(event)) {
    return;
  }
  VizPropertyPanel *panel;
  if ((panel = dynamic_cast<VizPropertyPanel*>(event.pane->window))) {
    bool gettingFocus = !event.pane->IsShown();
    panel->setFocus(gettingFocus);
  }

  // custom user message for the case of animating trajectories with the
  // atom table open
  if (event.pane->name == "Trajectory" &&
      !event.pane->IsShown() &&
      p_mgr.GetPane(NAME_TOOL_ATOM_TABLE).IsShown()) {
    showMessage("Animating a trajectory with the atom table open will slow the frame rate.");
  }
}


void Builder::OnAuiUpdate(wxAuiManagerEvent& event)
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
        } else if (getSG()->getFragment()->numAtoms() > 0) {
          // An empty structure gives a file no reader accepts (an XYZ
          // with 0 atoms); it was saved, and there is nothing to open.
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
      file->move(file->path() + "." + ext.ToStdString());
  
      if (writeFragmentToImageFile(file, type, ext)) {
        // now 'upload' new image where we really want it to go
        string path = dialog.GetPath().ToStdString();
        EcceURL url(path);
        Resource * resource = EDSIFactory::getResource(url.getParent());
        if (resource == 0) {
          // shouldn't happen
          wxLogError("Could not save as image for %s", url.toString().c_str());
        } else {
          resource = resource->createChild(filename.ToStdString(), file);
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
      cmd->getParameter("type")->setString(type.ToStdString());
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
  cmd->getParameter("streamType")->setString(ext.ToStdString());
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
  wxString layoutPrefix = wxString(this->layoutPrefix()) + layoutName + '/'; 
  // iterate over all panes and save their layout info
  wxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
  for (size_t i = 0, count = panes.GetCount(); i < count; ++i) {
    wxAuiPaneInfo &pane = panes.Item(i);
    if (pane.name == NAME_COLUMN_TOGGLE) {
      continue;
    }
    wxString info = paneInfoForSave(pane);
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
  //  Detaching and re-docking every property panel is not the user
  //  picking one -- see OnChildFocus() (#111).
  PanelBuildGuard buildGuard(p_panelBuildDepth);

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
  wxString layoutPrefix = wxString(this->layoutPrefix()) + layoutName; 

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
    wxAuiPaneInfo &pane = p_mgr.GetPane(*ppanelIt);
    if (pane.IsOk()) {
      p_folded.erase(pane.window);
      p_tabHidden.erase(pane.window);
      p_columnSeen.erase(pane.window);
      pane.window->Hide();
      p_mgr.DetachPane(*ppanelIt);
    }
  }

  layoutPrefix += "/";

  // iterate over all panes and restore their layout info, if any
  wxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
  for (size_t i = 0, count = panes.GetCount(); i < count; ++i) {
    wxAuiPaneInfo &pane = panes.Item(i);
    wxString info;
    if (pane.name != NAME_COLUMN_TOGGLE &&
        config->Read(layoutPrefix + pane.name, &info)) {
      unfoldPane(pane);     // the saved info carries the unfolded sizes
      p_mgr.LoadPaneInfo(info, pane);
      //  A layout saved under an older build carries whatever flat
      //  min_size.x was in force when it was saved (e.g. the old flat
      //  200) -- LoadPaneInfo() just restored it verbatim, overriding
      //  the content-derived floor addToolPanel() computed for this
      //  session. Reapply it here, same as the property-panel restore
      //  below.
      wxSize minSize = pane.min_size;
      minSize.x = contentMinWidth(pane.window);
      //  Likewise a fixed pane's saved height, which may predate a font
      //  change.
      if (pane.IsFixed() && !pane.IsToolbar() && pane.name != NAME_TOOL_CONTEXT &&
          pane.name != NAME_COLUMN_TABS) {
        minSize.y = contentFixedHeight(pane.window);
      }
      pane.MinSize(minSize);
    }
  }

  // Add property panels back in from our internal layout cache
  ppanels = PropertyPanel::getPanels(p_calculation->getURL().toString());
  for (ppanelIt = ppanels.begin(); ppanelIt != ppanels.end(); ++ppanelIt) {
    PropertyPanel *panel = *ppanelIt;
    wxAuiPaneInfo pane;
    addPropertyPanel(panel, panel->getName());
    wxString info = p_propertyPanelInfo[panel];
    if (!info.IsEmpty()) {
      p_mgr.LoadPaneInfo(info, pane);
      wxAuiPaneInfo &p = p_mgr.GetPane(pane.name);
      if (!p.IsOk()) {
        continue;
      }
      p.SafeSet(pane);
      p.PinButton(true);
      //  Same stale-saved-minimum problem as the tool-pane restore
      //  above: SafeSet() just copied the saved min_size.x verbatim.
      wxSize minSize = p.min_size;
      minSize.x = contentMinWidth(p.window);
      minSize.y = std::max(minSize.y, panel->minimumHeight());
      p.MinSize(minSize);
      //  addPropertyPanel() ticked this panel's Property-menu item from
      //  the pane's visibility as it added it, which was BEFORE the line
      //  above put the saved visibility back.  So a panel restored hidden
      //  kept a ticked menu item -- which is exactly what "the Properties
      //  menu items are all pre-selected" looks like from the menu (#111).
      int menuId = p_propertyMenu->FindItem(p.name);
      if (menuId != wxNOT_FOUND) {
        p_propertyMenu->Check(menuId, p.IsShown());
      }
    }
  }

  SGFragment *frag = getSG()->getFragment();

  if (frag->numResidues() &&
        GetMenuBar()->IsChecked(ViewerEvtHandler::ID_AUTO_RESIDUE) &&
        !isReadOnly() &&
        !p_mgr.GetPane("Residue Table").IsShown()) {
     p_mgr.GetPane("Residue Table").Show(true).Show(true);

     // If its a viz tool, call virtual refresh to force updating
     // Works around bug where we can't override show because it gets
     // called too much by AUI
     WxVizTool *panel = 
        dynamic_cast<WxVizTool*>(p_mgr.GetPane("Residue Table").window);
     if (panel) {
        panel->refresh();
     }

     updatePanes();
     GetMenuBar()->Check(GetMenuBar()->FindMenuItem("Tools", "Residue Table"), true);
  } else if (frag->numResidues()==0 &&
               p_mgr.GetPane("Residue Table").IsShown()) {
     p_mgr.GetPane("Residue Table").Show(false).Show(false);

     updatePanes();
     GetMenuBar()->Check(GetMenuBar()->FindMenuItem("Tools", "Residue Table"), false);
  }

  bool symmetryShown = false;
  //  SYMMETRY IS OPEN WHILE A MOLECULE IS BEING DRAWN -- in a saved
  //  layout too, not only the default one (see loadDefaultPaneLayout).
  //  A user who had arranged their own panes before the Symmetry pane
  //  joined the default never saw it open, which is exactly the user
  //  the reminder is for.  Only the editing layout: a finished
  //  calculation opens read-only and has nothing left to symmetrise.
  if (layoutName == NAME_LAYOUT_DEFAULT && !isReadOnly() &&
      p_mgr.GetPane(NAME_TOOL_SYMMETRY).IsOk() &&
      !p_mgr.GetPane(NAME_TOOL_SYMMETRY).IsShown()) {
     p_mgr.GetPane(NAME_TOOL_SYMMETRY).Show(true);
     GetMenuBar()->Check(
         GetMenuBar()->FindMenuItem("Tools", NAME_TOOL_SYMMETRY), true);
     symmetryShown = true;
  }

  if (isColumnMode()) {
    applyGeometry();
    refreshColumn();
    collapseLog();
  }

  if (update || symmetryShown) {
    updatePanes();
    debugPrintPaneSizes(p_mgr);
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
    // All tools are hidden except Build, Coordinate, Symmetry and Log
    names.insert(NAME_TOOL_CONTEXT);
    names.insert(NAME_TOOL_BUILD);
    names.insert(NAME_TOOL_COORDINATES);

    //  SYMMETRY IS OPEN WHILE A MOLECULE IS BEING DRAWN.
    //
    //  A structure that has not been symmetrised is the single most
    //  expensive thing a user can carry forward. The calculation runs
    //  in whatever group the code detects from coordinates that are
    //  almost but not quite symmetric -- Cs for an ethene, C1 for a
    //  benzene -- and the cost only becomes visible afterwards, when
    //  the orbitals have no useful labels and no correlation diagram
    //  can be drawn from them.
    //
    //  The panel told nobody it existed: it was one entry in the
    //  Tools menu, and the advice to use it appeared in the MO
    //  diagram, which is opened AFTER the calculation has run. Advice
    //  that arrives too late to act on is not advice.
    //
    //  This is the default layout only, so anyone who has arranged
    //  their own panes keeps them -- it reaches the people who have
    //  not yet found it, which is the point.
    names.insert(NAME_TOOL_SYMMETRY);

    names.insert(NAME_TOOL_LOG);
    if (isColumnMode()) {
      //  One column is short: Open structures, Build, Selection, Atom
      //  Table and Symmetry fit a 720-pixel screen; Coordinates is one
      //  click away in the Tools menu.
      names.erase(NAME_TOOL_COORDINATES);
      names.insert(NAME_TOOL_SELECTION);
      names.insert(NAME_TOOL_ATOM_TABLE);
    }
  } else if (layoutName == NAME_LAYOUT_STRUCTLIB) {
    // All tools are hidden except Structure Library
    names.insert(NAME_TOOL_CONTEXT);
    names.insert(NAME_TOOL_STRUCTLIB);
  }
  for (int i=0; i<p_toolCount; ++i) {
    name = p_toolMenu->GetLabel(ID_TOOLMENU_ITEM+i);
    show = names.find(name) != names.end();
    p_mgr.GetPane(name).Show(show).Show(show);
  }
 
  // special cases of positioning
  if (layoutName == NAME_LAYOUT_READONLY) {
    // put Atom tool on right, Log on bottom
    p_mgr.GetPane(NAME_TOOL_ATOM_TABLE).Right();
    p_mgr.GetPane(NAME_TOOL_LOG).Position(p_toolCount+1);
  } else if (layoutName == NAME_LAYOUT_DEFAULT) {
    p_mgr.GetPane(NAME_TOOL_SYMMETRY).Right();
  } else if (layoutName == NAME_LAYOUT_STRUCTLIB) {
    p_mgr.GetPane(NAME_TOOL_STRUCTLIB).Show(true).Show(true).Right();
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
    p_mgr.GetPane(name).Show(show).Show(show);
  }

  // save this default layout immediately
  savePaneLayout(layoutName);

  if (update) {
    updatePanes();
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
  PanelBuildGuard buildGuard(p_panelBuildDepth);  // see OnChildFocus (#111)
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
  // Every relevant property panel gets created (so it has a Property-menu
  // entry and is one click away), but only this small default subset is
  // actually shown for a freshly opened calculation -- previously every
  // single relevant panel was force-opened at once, regardless of how
  // many that was, which made the whole property area unusable both from
  // being crowded and from each individual panel getting too little
  // space to lay out properly. See addPropertyPanel() for where this
  // list actually takes effect.
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

  refreshColumn();
  if (needUpate) {
    updatePanes(); // TODO is this needed yet or can it wait?
  }

  //  ECCE_OPEN_PANEL=<name> (#171): headless capture needs a way to open
  //  a specific property panel with no synthetic input.  Done here,
  //  right after every relevant panel for this calc has been created,
  //  by doing exactly what OnPropertyMenuClick() does for a user's
  //  click -- Show() the pane, which is what actually triggers
  //  PropertyPanel::ensureInitialized()/initialize() the first time.
  //  Once only: this runs again on every property the calculation
  //  gains, and a panel the automation (or the user) has since closed
  //  must stay closed. The latch must only be set once the pane is
  //  actually found and shown -- the first call here can run before
  //  any panel exists yet, and latching then means it never retries.
  static bool openPanelDone = false;
  if (!openPanelDone) {
    const char *openPanelName = getenv("ECCE_OPEN_PANEL");
    if (openPanelName != 0) {
      wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(openPanelName));
      if (pane.IsOk()) {
        openPanelDone = true;
        pane.Show(true);
        if (p_panelMode == PANELS_DETAIL &&
            paneGroup(pane) == GROUP_PROPERTIES) {
          setDetail(pane.window);
        } else if (p_panelMode == PANELS_ACCORDION &&
                   paneGroup(pane) == GROUP_PROPERTIES) {
          accordionNormalize(pane.window);
        }
        for (int i = 0; i < p_propertyMenu->GetMenuItemCount(); i++) {
          wxMenuItem *item = p_propertyMenu->FindItemByPosition(i);
          if (item != 0 && item->GetItemLabelText() == openPanelName) {
            item->Check(true);
            break;
          }
        }
        p_stayCollapsed = getenv("ECCE_PANEL_COLLAPSED") != 0;
        updatePanes(true);
        p_stayCollapsed = false;
      } else if (p_propertyMenu->GetMenuItemCount() > 0) {
        // Panels exist now and still no match -- report once rather
        // than on the very first (panel-less) pass.
        openPanelDone = true;
        fprintf(stderr, "ECCE_OPEN_PANEL: no property panel named '%s' "
                        "for this calculation\n", openPanelName);
      }
    }
  }

  //  ECCE_PANEL_METRICS=<file>: once the layout has settled, write the
  //  3-D viewer's width and the window's, and every shown pane's place.
  static bool panelMetricsStarted = false;
  const char *metricsPath = getenv("ECCE_PANEL_METRICS");
  if (metricsPath != 0 && !panelMetricsStarted && p_calculation != 0 &&
      p_propertyMenu->GetMenuItemCount() > 0) {
    panelMetricsStarted = true;
    string path = metricsPath;
    wxTimer *timer = new wxTimer();   // lives until the process exits
    timer->Bind(wxEVT_TIMER, [this, path](wxTimerEvent&) {
      wxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
      FILE *f = fopen(path.c_str(), "w");
      if (f) {
        static const char *modeNames[] = { "classic", "stacked", "accordion",
                                           "detail" };
        fprintf(f, "mode %s\n", modeNames[p_panelMode]);
      }
      for (size_t i = 0; f && i < panes.GetCount(); ++i) {
        if (panes.Item(i).dock_direction == wxAUI_DOCK_CENTER) {
          fprintf(f, "viewer %d of %d px wide, %d of %d px high\n",
                  panes.Item(i).rect.width, GetClientSize().x,
                  panes.Item(i).rect.height, GetClientSize().y);
        }
      }
      //  One line per shown pane: where AUI put it, and whether its window
      //  is still the frame's own child (tests/apps/panel_layouts_test.py).
      for (size_t i = 0; f && i < panes.GetCount(); ++i) {
        const wxAuiPaneInfo &p = panes.Item(i);
        if (!p.IsShown() || p.IsToolbar() || !p.window) continue;
        //  "need": the width the pane's own controls take at this font.
        wxSizer *content = p.window->GetSizer();
        fprintf(f, "pane \"%s\" %d %d %d %d %s %s need %d have %d\n",
                p.name.ToStdString().c_str(), p.rect.x, p.rect.y,
                p.rect.width, p.rect.height,
                p.window->IsShownOnScreen() ? "onscreen" : "hidden",
                p.window->GetParent() == this ? "docked" : "reparented",
                content ? content->GetMinSize().x : 0,
                p.window->GetClientSize().x);
      }
      if (f) fprintf(f, "client %d %d\n", GetClientSize().x,
                     GetClientSize().y);
      if (f) fclose(f);
    });
    timer->StartOnce(12000);
  }

  //  ECCE_PANEL_TEST=<file>: run the panel-layout checks of
  //  BuilderPanels.C once the property panels exist, then exit.
  static bool panelTestStarted = false;
  if (getenv("ECCE_PANEL_TEST") != 0 && !panelTestStarted &&
      p_calculation != 0 && p_propertyMenu->GetMenuItemCount() > 0) {
    panelTestStarted = true;
    wxTimer *timer = new wxTimer();   // lives until the process exits
    timer->Bind(wxEVT_TIMER, [this](wxTimerEvent&) { runPanelLayoutTest(); });
    timer->StartOnce(5000);
  }

  //  ECCE_TEST_IMPORT=<file>: run Add Structure from File on <file> once the
  //  Builder is up, as if picked in the file dialog, press OK in any prompt
  //  it raises, then exit. ECCE_TEST_IMPORT_DELAY (seconds) leaves time to
  //  attach a debugger. Inert unless set; for tests/apps/import_test.py.
  static bool importStarted = false;
  const char *importPath = getenv("ECCE_TEST_IMPORT");
  if (importPath != 0 && !importStarted && p_calculation != 0) {
    importStarted = true;
    string path = importPath;
    const char *delay = getenv("ECCE_TEST_IMPORT_DELAY");
    wxTimer *okTimer = new wxTimer();   // both live until the process exits
    okTimer->Bind(wxEVT_TIMER, [](wxTimerEvent&) {
      for (wxWindow *w : wxTopLevelWindows) {
        wxDialog *dlg = dynamic_cast<wxDialog*>(w);
        if (dlg && dlg->IsModal()) {
          fprintf(stderr, "ECCE_TEST_IMPORT: OK in \"%s\"\n",
                  dlg->GetTitle().ToStdString().c_str());
          dlg->EndModal(wxID_OK);
        }
      }
    });
    wxTimer *timer = new wxTimer();
    timer->Bind(wxEVT_TIMER, [this, path, okTimer](wxTimerEvent&) {
      // Copy errors to stderr, where the test reads them; the old target
      // is the Builder's log panel and must survive.
      new wxLogChain(new wxLogStderr());
      okTimer->Start(500);
      importChemicalSystem(path, "", wxString(path).AfterLast('.'));
      okTimer->Stop();
      fprintf(stderr, "ECCE_TEST_IMPORT: done, %d atoms\n",
              getSG()->getFragment()->numAtoms());
      // Close() would ask to save the imported system.
      fflush(stderr);
      _exit(0);
    });
    timer->StartOnce(1 + 1000 * (delay ? atoi(delay) : 0));
  }

  //  ECCE_TEST_DOCK=1: float every panel docked on the side with the most
  //  of them, so that no dock is left there, run Tools > Dock Floating
  //  Panels, and report what returned to that side; for
  //  tests/apps/dock_test.py.  Inert unless set.
  static bool dockStarted = false;
  if (getenv("ECCE_TEST_DOCK") && !dockStarted) {
    dockStarted = true;
    wxTimer *timer = new wxTimer();   // lives until the process exits
    timer->Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
      auto docked = [this](int side) {
        std::vector<wxString> names;
        wxAuiPaneInfoArray& all = p_mgr.GetAllPanes();
        for (size_t i = 0; i < all.GetCount(); ++i)
          if (all[i].IsDocked() && !all[i].IsToolbar() && all[i].IsShown() &&
              all[i].dock_direction == side)
            names.push_back(all[i].name);
        return names;
      };
      auto floating = [this]() {
        int n = 0;
        wxAuiPaneInfoArray& all = p_mgr.GetAllPanes();
        for (size_t i = 0; i < all.GetCount(); ++i)
          if (all[i].IsShown() && !all[i].IsToolbar() && all[i].IsFloating())
            n++;
        return n;
      };
      int side = wxAUI_DOCK_LEFT;
      for (int s : { wxAUI_DOCK_RIGHT, wxAUI_DOCK_TOP, wxAUI_DOCK_BOTTOM })
        if (docked(s).size() > docked(side).size()) side = s;
      std::vector<wxString> moved = docked(side);
      for (const wxString& name : moved)
        p_mgr.GetPane(name).Float();
      p_mgr.Update();
      int floated = floating(), leftBefore = (int)docked(side).size();
      wxCommandEvent ev;
      OnDockFloatingPanels(ev);
      int floatedAfter = floating(), leftAfter = (int)docked(side).size();
      bool back = true;
      for (const wxString& name : moved) {
        wxAuiPaneInfo& pi = p_mgr.GetPane(name);
        if (!pi.IsOk() || !pi.IsDocked() || pi.dock_direction != side)
          back = false;
      }
      fprintf(stderr, "ECCE_TEST_DOCK: moved %d, floating %d, left docked %d; "
              "after: floating %d, left docked %d, all back %d\n",
              (int)moved.size(), floated, leftBefore, floatedAfter, leftAfter,
              back ? 1 : 0);
      fflush(stderr);
      _exit(0);
    });
    timer->StartOnce(3000);
  }

  //  ECCE_TEST_HELP=<png>: run Help > Builder, save the help window to
  //  <png> and exit (tests/apps/help_test.py).
  static bool helpStarted = false;
  if (getenv("ECCE_TEST_HELP") && !helpStarted) {
    helpStarted = true;
    wxTimer *timer = new wxTimer();   // lives until the process exits
    timer->Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
      wxCommandEvent ev;
      helpBuilderMenuitemClick(ev);
      WxHelpViewer::testSnapshot("builder");
    });
    timer->StartOnce(1000);
  }

  //  ECCE_TEST_SAVEAS=<type>|<path>[|<structure file>]: add the structure,
  //  if given, then open File > Save As, pick the first type whose label
  //  starts with <type>, save to <path> on the Local Filesystem, cancel the
  //  dialog if it stays open, report the context and exit. Inert unless
  //  set; for tests/apps/saveas_test.py.
  static bool saveAsStarted = false;
  const char *saveAsSpec = getenv("ECCE_TEST_SAVEAS");
  if (saveAsSpec != 0 && !saveAsStarted && p_calculation != 0) {
    saveAsStarted = true;
    wxString spec(saveAsSpec);
    wxString type = spec.BeforeFirst('|'), path = spec.AfterFirst('|');
    wxString structure = path.AfterFirst('|');
    path = path.BeforeFirst('|');
    wxTimer *driver = new wxTimer();   // both live until the process exits
    int *step = new int(0);
    driver->Bind(wxEVT_TIMER, [type, path, step](wxTimerEvent&) {
      SaveExperimentAsDialog *dlg = 0;
      for (wxWindow *w : wxTopLevelWindows) {
        dlg = dynamic_cast<SaveExperimentAsDialog*>(w);
        if (dlg && dlg->IsModal()) break;
        dlg = 0;
      }
      if (!dlg) {
        // A prompt on opening the result, e.g. the units of an XYZ file.
        for (wxWindow *w : wxTopLevelWindows) {
          wxDialog *other = dynamic_cast<wxDialog*>(w);
          if (other && other->IsModal()) {
            fprintf(stderr, "ECCE_TEST_SAVEAS: OK in \"%s\"\n",
                    other->GetTitle().ToStdString().c_str());
            other->EndModal(wxID_OK);
          }
        }
        return;
      }
      if ((*step)++ > 0) {
        fprintf(stderr, "ECCE_TEST_SAVEAS: dialog still open\n");
        dlg->EndModal(wxID_CANCEL);
        return;
      }
      wxBitmapComboBox *combo = 0;
      for (wxWindow *c : dlg->GetChildren())
        if ((combo = dynamic_cast<wxBitmapComboBox*>(c)) != 0) break;
      for (unsigned i = 0; combo && i < combo->GetCount(); i++) {
        if (combo->GetString(i).StartsWith(type)) {
          dlg->setSaveAsFilterIndex(i);
          fprintf(stderr, "ECCE_TEST_SAVEAS: type \"%s\"\n",
                  combo->GetString(i).ToStdString().c_str());
          break;
        }
      }
      dlg->setServerChoice(0);           // Local Filesystem
      dlg->HandleAction(path);
    });
    wxTimer *timer = new wxTimer();
    timer->Bind(wxEVT_TIMER, [this, driver, structure](wxTimerEvent&) {
      new wxLogChain(new wxLogStderr());
      if (!structure.empty())
        importChemicalSystem(structure.ToStdString(), "",
                             structure.AfterLast('.'));
      fprintf(stderr, "ECCE_TEST_SAVEAS: open\n");
      driver->Start(1000);
      doSaveAs(false);
      driver->Stop();
      fprintf(stderr, "ECCE_TEST_SAVEAS: done, context %s, %d atoms\n",
              p_calculation->getURL().toString().c_str(),
              getSG()->getFragment()->numAtoms());
      fflush(stderr);
      _exit(0);
    });
    timer->StartOnce(1000);
  }

  //  ECCE_TEST_OPEN=<path>: File > Open..., go to <path>'s folder on the
  //  Local Filesystem, report how its entry is listed, activate it as a
  //  double click does, report the context and exit. Inert unless set;
  //  for tests/apps/saveas_test.py.
  static bool openStarted = false;
  const char *openSpec = getenv("ECCE_TEST_OPEN");
  if (openSpec != 0 && !openStarted && p_calculation != 0) {
    openStarted = true;
    wxString path(openSpec);
    wxTimer *driver = new wxTimer();   // both live until the process exits
    int *step = new int(0);
    driver->Bind(wxEVT_TIMER, [path, step](wxTimerEvent&) {
      OpenCalculationDialog *dlg = 0;
      for (wxWindow *w : wxTopLevelWindows) {
        dlg = dynamic_cast<OpenCalculationDialog*>(w);
        if (dlg && dlg->IsModal()) break;
        dlg = 0;
      }
      if (!dlg) return;
      if ((*step)++ > 0) {
        fprintf(stderr, "ECCE_TEST_OPEN: dialog still open\n");
        dlg->EndModal(wxID_CANCEL);
        return;
      }
      dlg->setServerChoice(0);           // Local Filesystem
      dlg->HandleAction(path.BeforeLast('/'));
      wxString name = path.AfterLast('/');
      wxListCtrl *list = 0;
      for (wxWindow *c : dlg->GetChildren())
        if ((list = dynamic_cast<wxListCtrl*>(c)) != 0) break;
      long item = list ? list->FindItem(-1, name) : -1;
      wxFileData *fd = item >= 0 ? (wxFileData*)list->GetItemData(item) : 0;
      fprintf(stderr, "ECCE_TEST_OPEN: listed as %s\n",
              !fd ? "missing" : fd->IsDir() ? "folder" : "file");
      dlg->HandleAction(name);
    });
    wxTimer *timer = new wxTimer();
    timer->Bind(wxEVT_TIMER, [this, driver](wxTimerEvent&) {
      new wxLogChain(new wxLogStderr());
      driver->Start(1000);
      wxCommandEvent none;
      OnOpenClick(none);
      driver->Stop();
      fprintf(stderr, "ECCE_TEST_OPEN: done, context %s, %d atoms\n",
              p_calculation->getURL().toString().c_str(),
              getSG()->getFragment()->numAtoms());
      fflush(stderr);
      _exit(0);
    });
    timer->StartOnce(1000);
  }

  //  ECCE_VIEWER_SCENE=<script> ECCE_VIEWER_SCENE_OUT=<dir>: render the
  //  scenes of tools/coin/compare.sh in this calculation, then exit. Inert
  //  unless set. Polls until the molecule is loaded, since this method
  //  runs before the fragment is.
  static bool sceneStarted = false;
  const char *sceneScript = getenv("ECCE_VIEWER_SCENE");
  if (sceneScript != 0 && !sceneStarted && p_calculation != 0) {
    sceneStarted = true;
    string script = sceneScript;
    const char *o = getenv("ECCE_VIEWER_SCENE_OUT");
    string outdir = o ? o : ".";
    wxTimer *timer = new wxTimer();   // lives until the process exits
    int *ticks = new int(0), *settled = new int(0);
    timer->Bind(wxEVT_TIMER, [this, timer, ticks, settled, script, outdir](wxTimerEvent&) {
      SGContainer *sg = getSG();
      bool loaded = sg && sg->getFragment() && sg->getFragment()->numAtoms() > 0;
      ++*ticks;
      if (loaded) ++*settled;
      bool ready = *settled >= 6;       // 3 s after the atoms appear
      if (!ready && *ticks < 240) return;
      timer->Stop();
      //  The Builder's own canvas is as large as the pane layout leaves it
      //  (181 px wide in the headless layout), so draw in a second viewer of
      //  fixed size on the same scene graph.
      //  ECCE_VIEWER_SCENE_SIZE=<w>x<h> sizes the capture canvas.
      //  ECCE_VIEWER_SCENE_HOLD=<seconds> instead draws in the Builder's
      //  own canvas and keeps the window open that long, so the whole
      //  window can be photographed with the scene on screen
      //  (tools/screenshots/readme.py).
      const char *hold = getenv("ECCE_VIEWER_SCENE_HOLD");
      int capW = 480, capH = 480;
      if (const char *sz = getenv("ECCE_VIEWER_SCENE_SIZE"))
        sscanf(sz, "%dx%d", &capW, &capH);
      SGViewer *viewer = p_viewer;
      if (!hold) {
        wxFrame *frame = new wxFrame(NULL, wxID_ANY, "scene capture");
        viewer = new SGViewer(frame, wxID_ANY);
        viewer->setText("", "", "", "");
        viewer->setSceneGraph(p_sgMgr);
        viewer->setViewing(false);
        viewer->setDecoration(false);
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        viewer->SetMinSize(wxSize(capW, capH));
        sizer->Add(viewer, 1, wxEXPAND);
        frame->SetSizerAndFit(sizer);
        frame->Show(true);
      }
      for (int i = 0; i < 30; i++) { wxTheApp->Yield(true); wxMilliSleep(10); }
      viewer->viewAll();
      SceneScript run(viewer, sg, p_calculation, outdir);
      //  "mopanel <name>": press the MO panel's Compute, as a user would,
      //  recording every frame the Builder's own canvas paints meanwhile.
      run.setExtension([this, outdir](SceneScript& s, const vector<string>& w) {
        //  "hold <seconds>": keep the event loop running, so the windows
        //  can be captured from outside while the script waits.
        if (w[0] == "hold" && w.size() == 2) {
          wxLongLong end = wxGetLocalTimeMillis() + 1000 * atoi(w[1].c_str());
          while (wxGetLocalTimeMillis() < end) {
            wxTheApp->Yield(true);
            wxMilliSleep(20);
          }
          return true;
        }
        //  "savethumb": the calculation thumbnail, as File > Save writes it.
        if (w[0] == "savethumb" && w.size() == 1) {
          doSaveThumb();
          return true;
        }
        //  "cubegrid <name> <index> <log10 cutoff>": as a click on grid
        //  <index> of the cube panel with its slider at that cutoff.
        if (w[0] == "cubegrid" && w.size() == 4) {
          Cube *cube = 0;
          set<PropertyPanel*> cp =
              PropertyPanel::getPanels(p_calculation->getURL().toString());
          for (set<PropertyPanel*>::iterator it = cp.begin();
               it != cp.end() && !cube; ++it)
            cube = dynamic_cast<Cube*>(*it);
          if (!cube) return s.fail("cubegrid: no cube panel");
          return s.recordFrames(p_viewer, w[1], [cube, w]() {
            cube->selectGrid(atoi(w[2].c_str()), atof(w[3].c_str()));
          });
        }
        //  "gt...": the Geometry Trace stress commands (#217).
        if (w[0].compare(0, 2, "gt") == 0) return traceStressCommand(s, w);
        //  "mogrid <name>": the range of the grid last computed (an MO
        //  Compute), "none" without one -> <name>.txt.
        if (w[0] == "mogrid" && w.size() == 2) {
          std::ofstream out(outdir + "/" + w[1] + ".txt");
          SingleGrid *grid = getSG() ? getSG()->getCurrentGrid() : 0;
          if (grid)
            out << "fieldMin " << grid->fieldMin() << "\n"
                << "fieldMax " << grid->fieldMax() << "\n";
          else
            out << "none\n";
          return true;
        }
        if ((w[0] != "mopanel" && w[0] != "motable") || w.size() != 2)
          return s.fail("unknown command: " + w[0]);
        MoPanel *mo = 0;
        set<PropertyPanel*> panels =
            PropertyPanel::getPanels(p_calculation->getURL().toString());
        for (set<PropertyPanel*>::iterator it = panels.begin();
             it != panels.end() && !mo; ++it)
          mo = dynamic_cast<MoPanel*>(*it);
        if (!mo) return s.fail(w[0] + ": no MO panel");
        //  "motable <name>": the MO table's selection and scroll state, and
        //  the alpha HOMO from the parsed occupations, -> <name>.txt.
        if (w[0] == "motable") {
          std::ofstream out(outdir + "/" + w[1] + ".txt");
          out << mo->tableState();
          PropVector *occ =
              dynamic_cast<PropVector*>(p_calculation->getProperty("ORBOCC"));
          if (occ) {
            int homo = 0;
            double sum = 0.0;
            for (int i = 0; i < occ->rows(); i++) {
              if (occ->value(i) > 0.0) homo = i + 1;
              sum += occ->value(i);
            }
            out << "occHomo " << homo << "\n" << "occElectrons " << sum << "\n";
          }
          return true;
        }
        return s.recordFrames(p_viewer, w[1], [mo]() {
          wxCommandEvent ev(wxEVT_BUTTON, MoGUI::ID_BUTTON_MO_COMPUTE);
          ev.SetEventObject(mo->FindWindow(MoGUI::ID_BUTTON_MO_COMPUTE));
          mo->GetEventHandler()->ProcessEvent(ev);
        });
      });
      if (!ready || !run.run(script)) {
        string msg = ready ? run.message() : "molecule never loaded";
        fprintf(stderr, "ECCE_VIEWER_SCENE: %s\n", msg.c_str());
        std::ofstream(outdir + "/FAILED") << msg << "\n";
      }
      if (hold) {
        wxTimer *closer = new wxTimer();   // lives until the process exits
        closer->Bind(wxEVT_TIMER, [this](wxTimerEvent&) { Close(true); });
        closer->StartOnce(1000 * atoi(hold));
        return;
      }
      Close(true);
    });
    timer->Start(500);
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
  
  updatePanes();
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


//  A docked pane's width floor, derived from its own content rather than
//  a flat constant. wxAUI's LayoutAll() raises a dock's width to the
//  largest shown pane's min_size.x, so giving every pane an honest
//  content-based MinSize().x is enough to widen the whole right-hand
//  dock to fit whichever pane needs the most room (reported live: the
//  Build tool pane's element grid clipped against the dock). Used
//  uniformly by addToolPanel(), addPropertyPanel() and loadPaneLayout()
//  (the last one because a saved wxbuilder.ini layout
//  from an older build carries whatever flat minimum was in force when it
//  was saved, and LoadPaneInfo()/SafeSet() would otherwise silently
//  reimpose that stale value over whatever was just computed here).
//
//  PANE_WIDTH_MIN is the same "unpainted GTK3 panel reports ~0" floor
//  described beside PANEL_HEIGHT_MIN below -- a degenerate answer is not
//  evidence the panel wants to be that narrow. PANE_WIDTH_MAX caps the
//  other end: one unusually wide pane (a wide table, a long combo row)
//  must not let the dock swallow the 3-D viewer -- above the cap the pane
//  is resizable/scrolls internally rather than widening the shared dock.
static const int PANE_WIDTH_MIN = 200;
static const int PANE_WIDTH_MAX = 600;

static int contentMinWidth(wxWindow *window)
{
  if (!window) {
    return PANE_WIDTH_MIN;
  }
  // Lay out first -- GetMinSize()/GetBestSize() on a panel with a sizer
  // needs the sizer to have seen its children, and these can be called
  // before the panel has ever been shown.
  window->Layout();
  int w = window->GetMinSize().x;
  if (w <= 0) {
    w = window->GetBestSize().x;
  }
  if (w < PANE_WIDTH_MIN) {
    w = PANE_WIDTH_MIN;
  }
  if (w > PANE_WIDTH_MAX) {
    w = PANE_WIDTH_MAX;
  }
  return w;
}


//  A fixed pane cannot be resized by the user, so its height is what its
//  sizer needs in the current font.  A degenerate answer from a panel
//  not yet laid out falls back to the old flat 150.
static int contentFixedHeight(wxWindow *window)
{
  wxSizer *sizer = window ? window->GetSizer() : 0;
  if (!sizer) {
    return FIXED_PANE_HEIGHT_FALLBACK;
  }
  int h = sizer->GetMinSize().y;
  return h < FIXED_PANE_HEIGHT_FALLBACK ? FIXED_PANE_HEIGHT_FALLBACK : h;
}


//  ECCE_DEBUG_PANEL_SIZE=1 also prints one [PANESIZE] line per currently
//  shown docked pane, once the frame's layout has actually settled --
//  this is how an affected applet is identified by data rather than by
//  eye. CLIPPED means wxAUI gave the pane less width than its own content
//  asked for.
static void debugPrintPaneSizes(wxAuiManager &mgr)
{
  if (!getenv("ECCE_DEBUG_PANEL_SIZE")) {
    return;
  }
  wxAuiPaneInfoArray &panes = mgr.GetAllPanes();
  for (size_t i = 0, count = panes.GetCount(); i < count; ++i) {
    wxAuiPaneInfo &pane = panes.Item(i);
    if (!pane.IsShown() || !pane.window) {
      continue;
    }
    const wxSize best = pane.window->GetBestSize();
    const bool clipped = pane.rect.width > 0 && pane.rect.width < best.x;
    printf("[PANESIZE] %-28s content=%dx%d min=%d actual=%dx%d%s\n",
           (const char*)pane.name.mb_str(), best.x, best.y, pane.min_size.x,
           pane.rect.width, pane.rect.height,
           clipped ? " CLIPPED" : "");
    fflush(stdout);
  }
}


void Builder::addToolPanel(wxWindow *panel, const string& name,
                           const bool& readOnlyDisabled,
                           const bool& alwaysFixed)
{
  wxAuiPaneInfo pinfo;
  pinfo.Name(name).Caption(name).Show(false).CaptionVisible(true)
          .Right().Position(p_toolCount).Fixed();
  int minWidth = contentMinWidth(panel);
  if (alwaysFixed) {
    pinfo.Fixed().MaximizeButton(false);
    //  The context pane is a list that scrolls, so it keeps the floor.
    int height = name == NAME_TOOL_CONTEXT ? FIXED_PANE_HEIGHT_FALLBACK
                                           : contentFixedHeight(panel);
    pinfo.MinSize(wxSize(minWidth, height));
  } else {
    pinfo.Resizable(true).MaximizeButton(true);
    // A resizable pane is only actually recoverable if there's a resize
    // grip left to grab -- some panels' content reports a near-zero best
    // size (before it's ever been painted/populated), which without a
    // floor lets the pane collapse to the point where docked it's
    // invisible and floating there's no border left to drag back up.
    // Reported live: Atom Table and Log (both already alwaysFixed=false
    // before this session) collapsing the same way as the newly-
    // resizable panels, confirming this is a pane-level floor problem,
    // not something specific to any one panel's own layout.
    pinfo.MinSize(wxSize(minWidth, 150));
  }

  // NOTE: the OptionsButton() caption button (ewxAUI addition) has no
  // stock wx3.2 wxAuiPaneInfo equivalent and is dropped here - see
  // EwxAuiCompat.H.

  p_mgr.AddPane(panel, pinfo);

  WxVizTool * vizTool = dynamic_cast<WxVizTool *>(panel);
  if (vizTool)
    vizTool->connectToolKitFW(this);

  p_toolIndex[name] = p_toolCount;
  if (name != NAME_TOOL_LOG) {
    p_structureNames.insert(name);
  }
  p_toolMenu->AppendCheckItem(ID_TOOLMENU_ITEM+p_toolCount, name, "");
  if (readOnlyDisabled) {
    p_readOnlyDisabledIds.insert(ID_TOOLMENU_ITEM+p_toolCount);
  }
  ++p_toolCount;
}


void Builder::addToolBar(wxToolBar * toolbar, const string& name)
{
  toolbar->Realize();
  
  wxAuiPaneInfo pinfo;
  pinfo.Name(name).Caption(name).ToolbarPane().Top().Position(p_toolbarCount).
          LeftDockable(false).RightDockable(false);
  p_mgr.AddPane(toolbar, pinfo);

  p_toolbarMenu->AppendCheckItem(ID_TOOLBARMENU_ITEM+p_toolbarCount, name, "");
  ++p_toolbarCount;

  p_toolbars.push_back(toolbar);
}


//  Escape hatch for the content-based property-pane heights below.  Set
//  ECCE_UNIFORM_PANEL_HEIGHT=1 to put every pane back on one fixed
//  height, as it was before #112 -- the same pattern as
//  ECCE_GATEWAY_WINDOW, and for the same reason: a display-server-
//  dependent UI change is worth being able to rule out without a
//  rebuild.
static bool uniformPanelHeight()
{
  static int uniform = -1;
  if (uniform < 0) {
    const char *v = getenv("ECCE_UNIFORM_PANEL_HEIGHT");
    uniform = (v != 0 && *v != '\0' && *v != '0') ? 1 : 0;
  }
  return uniform == 1;
}


void Builder::addPropertyPanel(PropertyPanel *panel, const string& name)
{
  PanelBuildGuard buildGuard(p_panelBuildDepth);  // see OnChildFocus (#111)
  // Don't add this panel if it is already being managed
  wxAuiPaneInfo &pinfo = p_mgr.GetPane(panel);
  if (pinfo.IsOk()) {
    //  Deliberately does NOT Show() it.  This used to, which undid the
    //  defaultShownPanels choice below every time the function ran
    //  again -- and it runs again on every property the calculation
    //  gains.  Leave a calculation open while its job finishes and each
    //  arriving property re-showed every panel already created, so the
    //  viewer ended up covered in overlays; reopening the same
    //  calculation afterwards showed only the default three, because
    //  then the panels were being created rather than re-visited.  That
    //  is exactly the reported difference (#111).
    //
    //  Nothing needs a forced show here.  This function's job is to make
    //  sure the panel exists and has a Property-menu entry; the user
    //  showing or hiding one goes through OnPropertyMenuClick(), which
    //  sets the pane's visibility itself, and the layout-cache path at
    //  the other caller restores saved visibility with LoadPaneInfo()
    //  immediately afterwards.  Leaving it alone is what lets a panel
    //  the user closed stay closed.
  } else {
    wxAuiPaneInfo info = wxAuiPaneInfo();
    info.DefaultPane();
    info.Name(name).Caption(name).CaptionVisible(true).
            PinButton(true).Left().Layer(2).Resizable(true);

    //  A panel too wide for the dock opens floating instead.  It is
    //  still an ordinary AUI pane and can be docked by hand; only where
    //  it starts differs, so it rejoins the common path below rather
    //  than returning -- an early return here would skip the Properties
    //  menu entry and leave the panel unreachable once closed.
    const bool floating = panel->prefersFloating();
    if (floating) {
      info.Float().FloatingSize(panel->preferredFloatingSize())
          .MinSize(wxSize(400, 300));
    }
    if (!floating) {
    // Height from the panel's own content, not one number for all of
    // them.  A scalar readout (point group, total energy) wants a couple
    // of lines, a mode table wants height, a spectrum wants width, and
    // giving every pane the same 150px made the small ones mostly empty
    // and the large ones cramped (#112).
    //
    // WHY THE FLOOR AND THE FALLBACK ARE NOT OPTIONAL. The flat 150 was
    // not arbitrary: on GTK3 -- and on Wayland especially -- a panel that
    // has never been painted reports a best size of zero or near-zero,
    // and several of these build their grid or plot after construction,
    // so they do exactly that here. Trusting GetBestSize() unguarded
    // brings back panes with no height at all. So a degenerate answer is
    // rejected outright and falls back to the old constant, and even a
    // plausible one is clamped: a resizable pane is only recoverable if
    // a resize grip survives being collapsed (the same reason
    // addToolPanel() sets a floor).
    //
    // ECCE_UNIFORM_PANEL_HEIGHT=1 restores the old behaviour for a
    // session, so this needs no rebuild to rule in or out if a display
    // server disagrees.
    static const int PANEL_HEIGHT_FALLBACK = 150;  // the old flat value
    static const int PANEL_HEIGHT_MIN      = 80;   // still leaves a grip
    static const int PANEL_HEIGHT_MAX      = 600;  // no pane eats the dock
    static const int PANEL_HEIGHT_PADDING  = 12;

    int paneHeight = PANEL_HEIGHT_FALLBACK;
    if (!uniformPanelHeight()) {
      // Lay out first. GetBestSize() on a panel with a sizer is the
      // sizer's CalcMin(), which needs no paint -- but it does need the
      // sizer to have seen its children, and these panels are measured
      // here before they have ever been shown.
      panel->Layout();
      const wxSize best = panel->GetBestSize();
      if (best.y > PANEL_HEIGHT_MIN) {
        paneHeight = best.y + PANEL_HEIGHT_PADDING;
        if (paneHeight > PANEL_HEIGHT_MAX) {
          paneHeight = PANEL_HEIGHT_MAX;
        }
      }
      // else: zero, negative or implausibly small -- an unpainted or
      // not-yet-populated panel, NOT a panel that genuinely wants to be
      // tiny. Keep the constant.
    }
    //  HOW AUI ACTUALLY SIZES A DOCKED PANE.  Measured, because two
    //  attempts at this were wrong: BestSize and MaxSize are BOTH IGNORED
    //  for panes docked together.  A test harness asking for 90, 200 and
    //  600 with matching MaxSize got back 244, 244, 245 -- an equal split,
    //  which is exactly the reported symptom of a four-line Energies panel
    //  as tall as a mode table.
    //
    //  What AUI does use is dock_proportion: the same three panes with
    //  dock_proportion set to 90, 200 and 600 came back 80, 153 and 500.
    //  MinSize is honoured as a floor -- the smallest landed exactly on
    //  its 80 -- so the pair together give a panel its share of the dock
    //  in proportion to what its content needs, never below the floor.
    //
    //  Proportional rather than absolute, so a taller dock scales all of
    //  them up; a short readout still gets a small share of it instead of
    //  an equal one, which is the point.
    //  The floor's WIDTH comes from the panel, not a constant -- see
    //  contentMinWidth() above, shared with addToolPanel() and
    //  loadPaneLayout(). A hardcoded 200 lets the dock shrink a pane
    //  below what its own content needs, and the controls at the end of
    //  a horizontal row are simply clipped -- reported as the vibration
    //  panel's stop-animation button "not showing, or too narrow", with
    //  the play button beside it visible.  createPropertyPanel() already
    //  gives every property panel SetMinSize(400, -1); honour it.
    int paneMinWidth = contentMinWidth(panel);
    info.MinSize(wxSize(paneMinWidth,
                        std::max(PANEL_HEIGHT_MIN, panel->minimumHeight())));
    info.BestSize(wxSize(paneMinWidth, paneHeight));
    info.dock_proportion = paneHeight;
    p_baseProportion[panel] = paneHeight;

    // ECCE_DEBUG_PANEL_SIZE=1 prints what each panel asked for and what
    // it got, so a pane that comes out wrong can be attributed to the
    // panel's own best size rather than guessed at -- the same reason
    // ECCE_DEBUG_GEOMTRACE exists.
    if (getenv("ECCE_DEBUG_PANEL_SIZE")) {
      const wxSize best = panel->GetBestSize();
      printf("[PANELSIZE] %-28s best=%dx%d -> pane height %d%s\n",
             name.c_str(), best.x, best.y, paneHeight,
             (paneHeight == PANEL_HEIGHT_FALLBACK && !uniformPanelHeight())
               ? "  (fallback: best size not usable)" : "");
      fflush(stdout);
    }
    // Only this small default subset is shown for a freshly opened
    // calculation -- previously every single relevant panel was
    // force-opened at once (updatePropertyMenus() creates a panel for
    // every property the calculation has data for), which made the
    // whole property area unusable both from being crowded and from
    // each individual panel getting too little space to lay out
    // properly. Everything else still gets created here (so it has a
    // Property-menu entry, via AppendCheckItem() below, and is one click
    // away), just not shown by default. Decided here rather than by the
    // caller showing/hiding afterward, so the menu checkbox's initial
    // Check(pinfo.IsShown()) state below is correct from the start.
    // Was .Fixed() with no way to override -- every property panel
    // (Energies, Geometry Trace, MOs, ...) was permanently locked at
    // whatever size it happened to get on first show, both docked (no
    // way to make it taller) and floating (dragged out, it could
    // collapse to a tiny unresizable box). Same root cause/fix as the
    // Build tool panel elsewhere in this file.
    // NOTE: the TakeFocusButton()/AddFocusButton()/OptionsButton() caption
    // buttons (ewxAUI additions) have no stock wx3.2 wxAuiPaneInfo
    // equivalent and are dropped here - see EwxAuiCompat.H.
    }

    //  Outside the docked branch, because it applies to both.  It was
    //  inside, so a floating panel never had its visibility set and
    //  inherited wxAuiPaneInfo's default of SHOWN -- which is why the
    //  MO Diagram opened by itself on every calculation that had
    //  orbital energies.
    static const set<string> defaultShown = {
      "Calculation Summary", "Energies", "MOs"
    };
    bool show = defaultShown.find(name) != defaultShown.end();
    if (p_panelMode == PANELS_DETAIL) {
      show = false;                     // ensureDetail() picks the one
    } else if (p_panelMode == PANELS_ACCORDION && !floating) {
      show = true;                      // all of them, as caption bars
    }
    info.Show(show);

    info.Position(p_propertyMenu->GetMenuItemCount());
    if (!floating) {
      const int order = PropertyPanelFactory::getPropertyPanelFactory()
                          .indexOf(name);
      if (isColumnMode()) {
        info.Right().Layer(1).Row(0).Position(2 + (order < 0 ? 500 : order));
        info.PinButton(p_panelMode != PANELS_DETAIL);
        if (p_panelMode == PANELS_DETAIL) {
          info.dock_proportion = DETAIL_PROPORTION;
        }
      }
    }
    p_mgr.AddPane(panel, info);
  }

  // add to property menu and Check its item -- re-fetch from the manager
  // rather than reusing `pinfo`: when the pane didn't exist yet, `pinfo`
  // above was bound to wxAuiManager's not-found sentinel
  // (wxAuiNullPaneInfo), whose IsShown() defaults to true (state==0, no
  // optionHidden flag) regardless of what Show() was just called with on
  // the separate `info` object passed to AddPane() -- AddPane() copies
  // `info` into its own internal entry, it doesn't mutate the sentinel
  // `pinfo` still refers to. That made every property-menu item show as
  // checked/selected on open, even for panels deliberately left hidden by
  // the defaultShownPanels logic above.
  wxAuiPaneInfo &added = p_mgr.GetPane(panel);
  p_propertyMenu->AppendCheckItem(
          ID_PROPERTY_MENU_BASE+p_propertyMenu->GetMenuItemCount(),
          name, "")->Check(added.IsShown());
}


void Builder::createPropertyPanel(const string& name)
{
  PanelBuildGuard buildGuard(p_panelBuildDepth);  // see OnChildFocus (#111)
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
    // A silent drop here costs a whole investigation (#171) -- the menu
    // entry vanishes with no other trace.
    if (getenv("ECCE_DEBUG_PANELS")) {
      fprintf(stderr, "[PANELS] %s dropped: isRelevant() false\n",
              name.c_str());
    }
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
    p_folded.erase(*panelIt);
    p_tabHidden.erase(*panelIt);
    p_columnSeen.erase(*panelIt);
    p_baseProportion.erase(*panelIt);
    if (p_detail == *panelIt) p_detail = 0;
    if (p_accordionOpen == *panelIt) p_accordionOpen = 0;
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
  //  A running job delivers properties one at a time and each one can
  //  create a panel; none of them may steal the viewer (#111).
  PanelBuildGuard buildGuard(p_panelBuildDepth);
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
    updatePanes();
  }
}


void Builder::deleteSelection()
{

   Command *cmd = new DeleteCmd("Delete Selection", getSG());
   execute(cmd);
}


void Builder::quit(const bool& allowCancel)
{
  int ret = wxID_YES;
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
    // exit() still runs full C++/shared-library static destructor teardown
    // (__cxa_finalize/_dl_fini), which is exactly where the segfault this
    // comment is talking about actually happens (confirmed via a real
    // core dump: crash inside libwx_gtk3u_core's own global destructors,
    // reached from exit() -> __run_exit_handlers -> _dl_fini). _exit()
    // skips all of that and terminates immediately, which is what "exit
    // ungracefully" actually needs.
    _exit(0);
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
         string cpkconfig = config->Read("/CPK").ToStdString();
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
         string style = wxbuilder_ini->Read("/DefaultStyle", "CPK").ToStdString();
         string styledd = wxbuilder_ini->Read(style).ToStdString();
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
      string wireconfig = config->Read("/Wireframe").ToStdString();
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

