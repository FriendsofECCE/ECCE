/**
 * @file
 * @author Lisong Sun
 *
 * wxWidgets version of the Calculation Manager plus MD workflow support
 * 
 */

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "CalcMgr.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#include <unistd.h>
#endif
#ifdef __WXMSW__
#include "wx/msw/wrapwin.h"
#endif

#include <filesystem>
#include <fstream>
#include <functional>
#include "ReleaseNotice.H"
#include <memory>
#include <algorithm>
#include <sstream>
#include <cstdint>
#include <ctime>
#include <iostream>
using std::cerr;
using std::endl;

static bool offerStopServer(bool& inUse);
// The outcome of the local machine registration at startup, for "probe".
static std::string s_registerMessage;

#include <wx/dataobj.h>
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/utils.h>
#include <wx/dnd.h>
#include <wx/treectrl.h>

#include "util/LocalData.H"
#include "util/SessionLease.H"
#include "util/WaitingJobs.H"
#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/Host.H"
#include "util/TypedFile.H"
#include "util/InvalidException.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/ResourceUtils.H"
#include "util/StringConverter.H"
#include "util/TDateTime.H"
#include "util/SFile.H"
#include "util/BrowserHelp.H"
#include "util/VizThumbnail.H"
#include "wxgui/WindowShot.H"
#include <wx/display.h>
#include <algorithm>
#include "wxgui/WxHelpViewer.H"
#include "wxgui/ewxReportProblem.H"
#include "util/EditEvent.H"
#include "util/NullPointerException.H"

#include "tdat/AuthCache.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/RefMachine.H"

#include "dsm/NWChemMDModel.H"
#include "dsm/StoredCalcCopy.H"
#include "dsm/CodeFactory.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/EDSIExporter.H"
#include "dsm/EDSI.H"
#include "dsm/HistoryList.H"
#include "dsm/MdTask.H"
#include "dsm/ResourceType.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/Session.H"
#include "dsm/VDoc.H"
#include "dsm/ICalculation.H"
#include "dsm/OrbitalGuess.H"
#include "dsm/ChemistryTask.H"
#include "dsm/TGBSConfig.H"
#include "tdat/Fragment.H"
#include "tdat/SpinMult.H"
#include "dsm/DirDyVTSTTask.H"

#include "comm/EcceShell.H"
#include "comm/JobCatchUp.H"
#include "comm/Launch.H"
#include "comm/RCommand.H"
#include "comm/RunMgmt.H"

#include "wxgui/ewxProgressDialog.H"
#include "LaunchSelected.H"
#include "ReconnectSelected.H"
#include <future>
#include <wx/wupdlock.h>
#include "wxgui/EcceTool.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxFileDialog.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextEntryDialog.H"
#include "wxgui/WxCalcImport.H"
#include "wxgui/WxCalcImportClient.H"
#include "wxgui/WxDavAuth.H"
#include "wxgui/WxEditSessionMgr.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxResourceImageList.H"
#include "wxgui/WxResourceTreeCtrl.H"
#include "wxgui/WxResourceTreeItemData.H"
#include "wxgui/WxTailWindow.H"
#include "wxgui/WxResourceStateImageList.H"
#include "wxgui/WxState.H"
#include "wxgui/WxStateImageList.H"

#include "BookmarkEditor.H"
#include "ContextPanelFactory.H"
#include "CalculationContextPanel.H"
#include "ProjectContextPanel.H"
#include "util/JobProgress.H"
#include "ProgressBox.H"

static string progressKey(const EcceURL & url);
#include "ContextPanelInterface.H"
#include "TreeDropTarget.H"
#include "WxConfigureNwfsArchive.H"
#include "WxFind.H"
#include "wxgui/GlobalPrefs.H"

#include "GromacsImport.H"
#include <sys/stat.h>
#include <chrono>
#include <set>
#include <map>

// The example set (folder name) the first start imports.
static const char *const kFirstStartSet = "getting-started";
#include <wx/dirdlg.h>
#include "CalcMgr.H"
#include "CompareEnergies.H"
#include "CompareEnergiesDialog.H"
#include "AllocationsDialog.H"

IMPLEMENT_CLASS( CalcMgr, wxFrame )


#define ID_BOOKMARK_BASE 80000
#define ID_BOOKMARK_TOP 80200

#define ID_HISTORY_BASE 80201
#define ID_HISTORY_TOP 80400

/**
 * Constructor.
 * Should never be used.
 */
namespace {
// One-shot timer that runs its function and deletes itself.
class OnceTimer : public wxTimer
{
public:
  explicit OnceTimer(std::function<void()> f) : p_f(f) {}
  void Notify() override { p_f(); delete this; }
private:
  std::function<void()> p_f;
};
}

CalcMgr::CalcMgr( )
  : CalcMgrGUI(),
    EditListener(),
    WxDavAuth(),
    ProgressEventListener(),
    WxCalcImportClient(),
    JMSPublisher(ORGANIZER)
{
  throw InvalidException("Organizer: default constructor not supported.",WHERE);
}


/**
 * Constructor.
 * Should always use this constructor. Initialize the application.
 */
CalcMgr::CalcMgr( wxWindow* parent, wxWindowID id, const wxString& caption,
		              const wxPoint& pos, const wxSize& size, long style )
  : CalcMgrGUI(),
    EditListener(),
    WxDavAuth(),
    ProgressEventListener(),
    WxCalcImportClient(),
    JMSPublisher(ORGANIZER)
{
  Create(parent, id, caption, pos, size, style);
}


bool CalcMgr::Create( wxWindow* parent, wxWindowID id, const wxString& caption,
		                  const wxPoint& pos, const wxSize& size, long style )
{
  if (!CalcMgrGUI::Create( parent, id, caption, pos, size, style )) {
    wxFAIL_MSG( wxT("CalcMgr creation failed") );
    return false;
  }

  p_disabler = 0;
  p_disablerCount = 0;
  p_cursor = 0;

  // Issue #150: built once, right after the base class has laid out the
  // toolbar and splitter this inserts itself between.
  p_machineNoticePanel = NULL;
  p_releaseNotice = NULL;
  buildMachineNotice();
  Bind(wxEVT_ACTIVATE, &CalcMgr::OnActivate, this);

  initializeGUI();

  // The version in the title shows in every screenshot and bug report.
  SetTitle(GetTitle() + " " + wxString::FromUTF8(Ecce::ecceVersion()));

  // Under -remote the calculations live on another machine; say which,
  // so a client's window cannot be mistaken for a local session's.
  if (getenv("ECCE_REMOTE_SERVER")) {
    EDSIServerCentral central;
    string host = central.getDefaultUserHome().getHost();
    if (!host.empty())
      SetTitle(GetTitle() + " on " + wxString::FromUTF8(host.c_str()));
  }

  setAuthDialogParent(this);
  EDSIFactory::addAuthEventListener(this);
  EDSIFactory::addProgressEventListener(this);

  // Get Registry
  ResourceDescriptor rs = ResourceDescriptor::getResourceDescriptor();



  // Set desktop icon
  ewxWindowUtils::setToolIcon(this, ORGANIZER);


  p_panelNode = 0;
  p_accessControlDialog = 0;
  p_bookmarkEditor = new BookmarkEditor("Bookmarks", true, this,
                                        wxID_ANY, "Bookmark Editor");


  p_serverListEditor = new BookmarkEditor("ServerList", false, this,
                                          wxID_ANY, "Server List Editor");
  p_find = 0;
  p_prefs = 0;
  Bind(wxEVT_MENU, &CalcMgr::OnPreferencesClick, this, wxID_PREFERENCES);
  {
    // Help > How to Cite ECCE..., before About.
    wxMenuBar* mb = GetMenuBar();
    int help = mb->FindMenu(_("&Help"));
    if (help != wxNOT_FOUND) {
      wxMenu* m = mb->GetMenu(help);
      int about = m->FindItem(_("About ECCE..."));
      size_t pos = m->GetMenuItemCount();
      if (about != wxNOT_FOUND) m->FindChildItem(about, &pos);
      wxWindowID id = wxNewId();
      m->Insert(pos, id, _("How to &Cite ECCE..."));
      Bind(wxEVT_MENU, [](wxCommandEvent&) {
        WxHelpViewer::show("citing-ecce.html");
      }, id);
    }
  }
  updateBookmarkMenu();

  p_nwfs = 0;

  p_hlist = new HistoryList(15);

  p_currentSelection.clear();
  p_clipboard.clear();
  p_toolBar->EnableTool(wxID_PASTE, false);
  GetMenuBar()->Enable(wxID_PASTE, false);
  //  p_toolBar->EnableTool(wxID_STOP, false);
  p_popupOnTree = false;
  p_focusOnTree = true;


  if (!getenv("ECCE_DEVELOPER")) {
    GetMenuBar()->Remove(GetMenuBar()->FindMenu(_("Developer")));
    p_toolBar->RemoveTool(wxID_VIEW_PROPERTY);
  }


  // Setup the event handling function for the context sensitive menu items.
  // Note: the static EVT macro didn't work in this derived class.
  int low, high;
  rs.getResourceIdRange(low, high);
  Connect(low, high,
          wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(CalcMgr::OnNewResourceClick));
  rs.getRmTaskIdRange(low, high);
  Connect(low, high,
          wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(CalcMgr::OnRmTaskClick));
  rs.getToolIdRange(low, high);
  Connect(low, high,
          wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(CalcMgr::OnToolClick));

  Connect(ID_HISTORY_BASE, ID_HISTORY_TOP,
          wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(CalcMgr::OnHistoryClick));

  Connect(ID_BOOKMARK_BASE, ID_BOOKMARK_TOP,
          wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(CalcMgr::OnBookmarkClick));

  rs.getPropertyViewIdRange(low, high);
  Connect(low, high,
          wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(CalcMgr::OnProjectViewClick));


  Connect(wxID_PANEL_SELECTION,
          wxEVT_COMMAND_MENU_SELECTED,
          wxCommandEventHandler(CalcMgr::OnPanelSelection));

  // The get/lose focus event handler, need to put it before all other
  // handlers in order to get the event.
  TreeEvtHandler * treeHandler = new TreeEvtHandler(this);
  p_treeCtrl->PushEventHandler(treeHandler);
  TreeDropTarget * treeDropTarget = new TreeDropTarget(this);
  p_treeCtrl->SetDropTarget(treeDropTarget);

  restoreSettings();

  startDisabler();
  
  initializeTreeData();
  SetAutoLayout(true);

  // This allows imports and local mdprepare invocations before
  // ever running the Job Launcher
  string msg = "";
  RunMgmt::registerLocalMachine(msg);
  s_registerMessage = msg;
  if (msg != "") {
    setMessage(msg, WxFeedback::INFO);
  }

  // Issue #150: checked last, once whatever registerLocalMachine() just
  // did (or failed to do) is already reflected in RefMachine's tables.
  refreshMachineNotice();

  // #273: the bar sits above the machine notice; the check runs after start.
  {
    wxInfoBar* bar = new wxInfoBar(this);
    p_topSizer->Insert(1, bar, 0, wxGROW);
    p_releaseNotice = new ReleaseNotice(bar);
    p_releaseNotice->start();
  }

  // #302: after the window is up, so the import shows in the tree.
  // CallAfter alone runs before the first paint; the delay lets the window
  // appear first.
  (new OnceTimer([this]() {
    Update();
    cerr << "Organizer: window shown" << endl;
    importExamplesOnce();
  }))->StartOnce(300);

  // File > New is offered before the first selection too.
  {
    wxMenuBar * mb = GetMenuBar();
    int fm = mb ? mb->FindMenu(_("File")) : wxNOT_FOUND;
    if (fm != wxNOT_FOUND) getFileMenu(*mb->GetMenu(fm), 0);
  }

  stopDisabler();

  //  ECCE_TEST_CALCIMPORT=<file>: File > Import Calculation from Output File
  //  on <file>, into a project "calcimport-test" in the user's home, as if
  //  picked in the file dialog; prints the outcome and exits. Inert unless
  //  set; for tests/apps/calcimport_test.py.
  if (const char *importPath = getenv("ECCE_TEST_CALCIMPORT")) {
    string path = importPath;
    wxTimer *timer = new wxTimer();   // lives until the process exits
    timer->Bind(wxEVT_TIMER, [this, path](wxTimerEvent&) {
      EDSIServerCentral central;
      EcceURL home = central.getDefaultUserHome();
      EcceURL project = home.getChild("calcimport-test");
      if (!EDSIFactory::getResource(project)) {
        if (Resource *homeRes = EDSIFactory::getResource(home))
          homeRes->createChild("calcimport-test",
                               ResourceDescriptor::RT_COLLECTION,
                               ResourceDescriptor::CT_PROJECT,
                               ResourceDescriptor::AT_UNDEFINED);
      }
      findNode(project, true, true);
      WxCalcImport dlg(this);
      dlg.registerListener(this);
      dlg.importFile(path);
      fflush(stdout);
      fflush(stderr);
      _exit(0);
    });
    timer->StartOnce(2000);
  }

  //  ECCE_TEST_ORGANIZER=<file>: lines appended to <file> are run as
  //  commands (runTestCommand), each answered on stderr with a line
  //  "ECCE_TEST_ORGANIZER: <command>: <outcome>". Inert unless set; for
  //  tests/apps/session_end.py, which uses it to work in one session's
  //  Organizer while another session ends.
  if (const char *cmdPath = getenv("ECCE_TEST_ORGANIZER")) {
    string path = cmdPath;
    auto done = std::make_shared<size_t>(0);
    wxTimer *timer = new wxTimer();   // lives until the process exits
    timer->Bind(wxEVT_TIMER, [this, path, done](wxTimerEvent&) {
      std::ifstream in(path.c_str());
      string line;
      size_t n = 0;
      while (std::getline(in, line)) {
        if (n++ < *done) continue;
        *done = n;
        if (!line.empty() && line.back() == '\r')   // written on Windows
          line.pop_back();
        runTestCommand(line);
      }
    });
    timer->Start(500);
  }

  //  ECCE_TEST_HELP=<png>: open Help > Organizer, save the help window to
  //  <png> and exit (tests/apps/help_test.py).
  if (getenv("ECCE_TEST_HELP")) {
    wxTimer *timer = new wxTimer();   // lives until the process exits
    timer->Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
      wxCommandEvent ev;
      OnHelpClick(ev);
      WxHelpViewer::testSnapshot("organizer");
    });
    timer->StartOnce(3000);
  }

  return true;
}


/**
 * treeflags      the tree with "+" (expander) and "-" (open) on each node
 * examples [set]  import an example set (default general) as the dialog's
 *                 OK does: "ok <project> created|existing"
 * examplesdialog  show File > Open Example Calculations' dialog, non-modal
 * examplesmenu    the menu item's own handler (opens the modal dialog)
 * import <file>   import <file> into session-test, as File > Import
 *                 Calculation from Output File does
 * walk            select and expand every node below the home, as a user
 *                 clicking through the tree does
 * browse          load the user's home and the project "session-test"
 *                 from the data server, as expanding them in the tree does
 * open [name]     make (once) and select session-test/<name> (default
 *                 calc), an NWChem calculation, as picking it does, and
 *                 read its state
 * refresh         reload session-test from the data server, as View >
 *                 Refresh does
 * message <text> show <text> in the message pane, as any status message does
 * publish         publish ecce_activity, as every tool start does
 * reportproblem  Help > Report a Problem... without the dialog: the
 *                 archive path
 * quit-offer      whether the Quit dialog would offer Quit and Stop Server
 * tail <machine> <user> <file>
 *                 Run Mgmt's Tail on <file> of a registered machine, as
 *                 openTail() does for a job (user "-" for none)
 * tail-wait <text> wait up to 60 s for <text> in that Tail window
 * tail-snap <png> save that window as a PNG
 * tail-close      close it, as its Close button does
 * summary <url>   select <url> in the tree and print the summary panel's
 *                 molecule, basis and setup fields, as label=value
 * snap <png>      save the Organizer window as a PNG
 * new <parent url> <type>
 *                 File > New with resource type <type> (gromacs_md_study,
 *                 gromacs_md_energy) in <parent>, as the New menu makes it
 * counts          broker notices, the updates made for them and their
 *                 batches since the last counts
 * newprompt <parent url> <code>
 *                 File > New <code> Calculation with the name prompt,
 *                 answered OK after 1.5 s
 * contextmenu <url> right-click <url> in the tree; the New submenu's items
 * progress <url> <file>
 *                 hand the snapshot in <file> to the Organizer as an
 *                 ecce_url_progress message does (empty file: forget it)
 * duplicate <url> [last]
 *                 right-click <url> > Duplicate for Rerun (or Duplicate
 *                 Setup with Last Geometry); the tree's selection and
 *                 collapse events during it, then the tree
 * size <w> <h>   resize the Organizer window
 * firstvisible    the label of the tree's top visible row (its scroll)
 * scrollsummary   scroll the calculation summary to its bottom and back
 * showbox         scroll the summary panel so that the Progress box is at
 *                 its top
 * allocations    Tools > Allocations: prints "ALLOC: machine|account|used|
 *                 checked" for each row on stderr
 * compareenergies <sideA> <sideB>
 *                 what Tools > Compare Energies shows for the two sides
 *                 (each "[-][N*]url,[-][N*]url"), " | " between lines
 * comparedialog <sideA> <sideB> [png]
 *                 open that dialog preset to the two sides, report its text
 *                 areas and the Copy text, and save a capture of it
 * progressbox     the shown summary's Progress box: shown or not, its
 *                 paints and the lines it draws, separated by " | "
 * contact <url> lost <epoch>|ok
 *                 the job store's out-of-contact notice (#284)
 * checklines <url> <line>|<line>
 *                 what Check Job on Machine said, as if it had run
 * fold open|shut  fold the Progress box, as a click on its heading does
 * treestate <url> the tree icon's index, and those of running and of
 *                 out of contact
 * tablestates     the shown project table's State cells
 * close [n]       n (default 1) window close requests in a row, as clicks on
 *                 the title bar's X, each queued as its own event
 * answer ok|stop  answer the modal dialog that is open (Quit ECCE) with OK
 *                 or Quit and Stop Server
 */
static wxDialog *makeExamplesDialog(wxWindow *parent, int *chosen);
static const int ID_COMPARE_ENERGIES = wxNewId();
static const int ID_ALLOCATIONS = wxNewId();

void CalcMgr::runTestCommand(const string& line)
{
  string outcome;
  string command = line.substr(0, line.find(' '));
  string calcName = line.size() > command.size() + 1
                    ? line.substr(command.size() + 1) : string("calc");
  try {
    EDSIServerCentral central;
    EcceURL home = central.getDefaultUserHome();
    EcceURL project = home.getChild("session-test");
    if (command == "import") {
      // A calculation with results, as File > Import Calculation makes it.
      Resource *homeRes = EDSIFactory::getResource(home);
      if (homeRes && !EDSIFactory::getResource(project))
        homeRes->createChild("session-test",
            ResourceDescriptor::getResourceDescriptor().getResourceType(
                "collection", "ecceProject", ""));
      findNode(project, true, true);
      WxCalcImport dlg(this);
      dlg.registerListener(this);
      dlg.importFile(calcName);
      outcome = "done";
    } else if (command == "examples") {
      // File > Open Example Calculations
      // "examples SET" imports a set without the dialog (default getting-started)
      string message;
      const string set = line.size() > command.size() + 1 ? calcName : kFirstStartSet;
      bool created = false;
      string made = openExampleSet(set, message,
                                   created);
      outcome = made.empty() ? "not made: " + message
                             : "ok " + made + (created ? " created" : " existing");
    } else if (command == "examplesdialog") {
      // the dialog, shown without blocking, for a screenshot
      int unused = -1;
      makeExamplesDialog(this, &unused)->Show();
      outcome = "ok";
    } else if (command == "examplesmenu") {
      // the menu item itself (disabler, message), not openExampleSet() alone
      wxCommandEvent ev(wxEVT_COMMAND_MENU_SELECTED,
                        CalcMgrGUI::wxID_OPENEXAMPLES);
      ProcessEvent(ev);
      outcome = "ok";
    } else if (command == "newproject" || command == "newcalc") {
      // File > New Project / New <code> Calculation, without the name
      // prompt: "newproject NAME" in the home, "newcalc PROJECT NAME TYPE"
      // with the code's applicationType (QuantumESPRESSO).
      std::istringstream args(calcName);
      string a, b, type;
      args >> a >> b >> type;
      ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
      Resource *parentRes = EDSIFactory::getResource(
          command == "newproject" ? home : home.getChild(a));
      ResourceType *rt = command == "newproject"
          ? rd.getResourceType("collection", "ecceProject", "")
          : rd.getResourceType("virtual_document", "ecceCalculation", type);
      string name = command == "newproject" ? a : b;
      if (!parentRes || !rt) {
        outcome = "no parent or type";
      } else {
        Resource *made = parentRes->createChild(name, rt);
        WxResourceTreeItemData *parentNode =
            findNode(parentRes->getURL(), true, true);
        if (parentNode) p_treeCtrl->refresh(parentNode);
        if (made) findNode(made->getURL(), true, true);
        outcome = made ? "ok " + made->getURL().toString() : "not created";
      }
    } else if (command == "new") {
      // "new PARENT-URL TYPE": File > New with TYPE, the resource type's
      // name in ResourceDescriptor.xml (gromacs_md_study), made in PARENT
      // as the New menu does it, named by the type's label.  The tool the
      // Organizer opens for a new resource, if any, starts as after New.
      std::istringstream args(calcName);
      string parentUrl, typeName;
      args >> parentUrl >> typeName;
      ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
      ResourceType *rt = 0;
      vector<ResourceType*> types = rd.getResourceTypes();
      for (size_t i = 0; i < types.size() && !rt; i++)
        if (types[i]->getName() == typeName) rt = types[i];
      WxResourceTreeItemData *parentNode = findNode(EcceURL(parentUrl),
                                                    true, true);
      if (!rt || !parentNode) {
        outcome = !rt ? "no resource type " + typeName : "no parent node";
      } else {
        string error;
        p_testCreateError = &error;
        createResource(rt, parentNode, true);
        p_testCreateError = 0;
        outcome = error.empty() ? "ok" : "error: " + error;
      }
    } else if (command == "gmximport") {
      // "gmximport PARENT-URL RUN-DIR": import a GROMACS run (layout in
      // GromacsImport.H) as a GROMACS MD study in PARENT.
      std::istringstream args(calcName);
      string parentUrl, runDir;
      args >> parentUrl >> runDir;
      WxResourceTreeItemData *parentNode = findNode(EcceURL(parentUrl),
                                                    true, true);
      string message, made;
      if (parentNode)
        made = importGromacsRun(parentNode->getResource(), "GROMACS_Import",
                                runDir, message);
      if (parentNode) p_treeCtrl->refresh(parentNode);
      outcome = !parentNode ? "error: no parent node"
              : (made.empty() || !message.empty()) ? "error: " + message
              : "ok " + made;
    } else if (command == "gmximportsel") {
      // "gmximportsel PROJECT-URL RUN-DIR": File > Import GROMACS Run...
      // with PROJECT selected and RUN-DIR chosen.
      std::istringstream args(calcName);
      string parentUrl, runDir, message;
      args >> parentUrl >> runDir;
      findNode(EcceURL(parentUrl), true, true);
      string made = importGromacsFrom(runDir, message);
      outcome = (made.empty() || !message.empty()) ? "error: " + message
              : "ok " + made;
    } else if (command == "state") {
      Resource *res = EDSIFactory::getResource(EcceURL(calcName));
      ICalculation *icalc = dynamic_cast<ICalculation*>(res);
      outcome = icalc ? ResourceUtils::stateToString(icalc->getState())
                      : "no such calculation";
    } else if (command == "walk") {
      // Select and expand every node below the home as a user clicking
      // through the tree does: the tree's own event handlers run.
      WxResourceTreeItemData *top = findNode(home, true, false);
      int visited = 0;
      std::vector<wxTreeItemId> todo;
      if (top) todo.push_back(top->GetId());
      while (!todo.empty() && visited < 50) {
        wxTreeItemId id = todo.back();
        todo.pop_back();
        p_treeCtrl->SelectItem(id);
        p_treeCtrl->Expand(id);
        wxYield();
        visited++;
        wxTreeItemIdValue cookie;
        for (wxTreeItemId c = p_treeCtrl->GetFirstChild(id, cookie);
             c.IsOk(); c = p_treeCtrl->GetNextChild(id, cookie))
          todo.push_back(c);
      }
      outcome = top ? "ok, " + std::to_string(visited) + " nodes"
                    : "no home in the tree";
    } else if (command == "message") {
      setMessage(calcName, WxFeedback::INFO);
      outcome = "ok";
    } else if (command == "refresh") {
      WxResourceTreeItemData *node = findNode(project, true, true);
      if (node) {
        p_treeCtrl->refresh(node);
        onSelectionChange(true);
        outcome = "ok, " + std::to_string(
            p_treeCtrl->GetChildrenCount(node->GetId(), false)) + " children";
      } else outcome = "not in the tree";
    } else if (command == "browse" || command == "open") {
      Resource *homeRes = EDSIFactory::getResource(home);
      if (!homeRes) {
        outcome = "no home " + home.toString();
      } else {
        ResourceDescriptor& rd = ResourceDescriptor::getResourceDescriptor();
        Resource *projRes = EDSIFactory::getResource(project);
        if (!projRes)
          projRes = homeRes->createChild("session-test",
              rd.getResourceType("collection", "ecceProject", ""));
        Resource *calcRes = 0;
        if (projRes && command == "open") {
          EcceURL calc = project.getChild(calcName);
          calcRes = EDSIFactory::getResource(calc);
          if (!calcRes)
            calcRes = projRes->createChild(calcName,
                rd.getResourceType("virtual_document", "ecceCalculation",
                                   "NWChem"));
        }
        WxResourceTreeItemData *node =
          findNode(calcRes ? calcRes->getURL() : project, true, true);
        if (!projRes || (command == "open" && !calcRes)) outcome = "not created";
        else if (!node) outcome = "not in the tree";
        else if (calcRes) {
          ICalculation *icalc = dynamic_cast<ICalculation*>(calcRes);
          outcome = string("ok, state ") +
                    (icalc ? ResourceUtils::stateToString(icalc->getState())
                           : string("?"));
        } else outcome = "ok";
      }
    } else if (line == "publish") {
      JMSMessage *msg = newMessage();
      msg->addProperty("action", "start");
      outcome = publish("ecce_activity", *msg) ? "sent" : "not sent";
      delete msg;
    } else if (command == "tail") {
      std::istringstream words(calcName);
      string machine, user, file;
      words >> machine >> user >> file;
      if (user == "-") user = "";
      string error;
      p_testTail = WxTailWindow::open(this, "test", machine, "", user, file,
                                      error);
      outcome = p_testTail ? "ok, " + p_testTail->backend()
                           : "error: " + error;
    } else if (command == "tail-wait") {
      bool seen = false;
      for (int i = 0; i < 600 && p_testTail && !seen; i++) {
        seen = p_testTail->text().find(calcName) != string::npos;
        if (!seen) { wxYield(); wxMilliSleep(100); }
      }
      outcome = !p_testTail ? "no tail window"
                : seen ? "ok, " + std::to_string(p_testTail->lineCount()) +
                         " lines" : "timeout";
    } else if (command == "tail-snap") {
      outcome = p_testTail && p_testTail->snapshot(calcName) ? "saved"
                                                             : "not saved";
    } else if (command == "summary") {
      WxResourceTreeItemData *node = findNode(EcceURL(calcName), true, true);
      if (!node) {
        outcome = "not in the tree";
      } else {
        p_treeCtrl->SelectItem(node->GetId());
        for (int i = 0; i < 5; i++) wxYield();
        wxWindow *panel = p_contextPanel ? p_contextPanel->getWidget() : 0;
        static const char *fields[][2] = {
          {"Formula", "empiricalFormula"}, {"Atoms", "numAtoms"},
          {"Electrons", "numElectrons"}, {"Symmetry", "symmetrygroup"},
          {"Basis", "name"}, {"Polarization", "coordsys"},
          {"Functions", "numFunctions"}, {"Primitives", "numPrimitives"},
          {"Theory", "theory"}, {"Runtype", "runtype"},
          {"Charge", "charge"}, {"SpinMult", "spinmultiplicity"},
          {"OpenShells", "openshells"}};
        outcome = panel ? "ok" : "no context panel";
        for (auto& f : fields) {
          wxWindow *w = panel ? panel->FindWindow(f[1]) : 0;
          wxStaticText *t = dynamic_cast<wxStaticText*>(w);
          outcome += string(" ") + f[0] + "=" +
                     (t ? string(t->GetLabel().ToUTF8()) : string("<none>"));
        }
        // The Launch Info rows that exist, as "label@x,y" (#648).
        static const char *launchLabels[] = {"Machine:", "Processors:",
          "Queue:", "Job ID:", "Start Date:", "Comp. Date:", "Cost:", "Allocation:",
          "Remaining on submission:"};
        outcome += " launch=";
        for (const char *lbl : launchLabels) {
          wxWindow *w = panel ? panel->FindWindow(string("launch:") + lbl) : 0;
          if (!w) continue;
          outcome += string(lbl) + "@" + std::to_string(w->GetPosition().x) +
                     "," + std::to_string(w->GetPosition().y) + ";";
        }
        // The space between the formula and the "Charge:" column beside it
        // (negative: they overlap).
        wxWindow *formula = panel ? panel->FindWindow("empiricalFormula") : 0;
        wxWindow *charge = panel ? panel->FindWindow("Charge:") : 0;
        if (formula && charge)
          outcome += " formula-gap=" + std::to_string(charge->GetPosition().x -
              formula->GetPosition().x - formula->GetSize().x);
        // Each tool button's label, and "clipped" when it is wider than
        // the button.
        if (panel) {
          for (wxWindow *w : panel->GetChildren()) {
            EcceTool *tool = dynamic_cast<EcceTool*>(w);
            if (!tool || tool->getLabel().empty()) continue;
            bool clipped = tool->GetTextExtent(tool->getLabel()).x >
                           tool->GetSize().x;
            outcome += " tool=" + string(tool->getLabel().ToUTF8()) +
                       (clipped ? ":clipped" : "");
          }
        }
      }
    } else if (command == "home" || command == "up") {
      // The toolbar's Home and Up buttons, then the selected node's URL.
      wxCommandEvent ev;
      if (command == "home") OnHomeClick(ev); else OnUpClick(ev);
      WxResourceTreeItemData *sel = p_treeCtrl->getSelection();
      outcome = "ok selected=" + (sel ? sel->getUrl().toString() : string("-"));
    } else if (command == "tree") {
      // The tree as drawn: "depth:label" for each node below the hidden
      // root, in order, "[" marking the selected one, then its URL.
      std::function<void(wxTreeItemId, int)> walk =
          [&](wxTreeItemId id, int depth) {
        wxTreeItemIdValue cookie;
        for (wxTreeItemId c = p_treeCtrl->GetFirstChild(id, cookie);
             c.IsOk(); c = p_treeCtrl->GetNextChild(id, cookie)) {
          bool sel = p_treeCtrl->GetSelection() == c;
          outcome += string(" ") + (sel ? "[" : "") + std::to_string(depth) +
                     ":" + string(p_treeCtrl->GetItemText(c).ToUTF8());
          walk(c, depth + 1);
        }
      };
      outcome = "ok";
      walk(p_treeCtrl->GetRootItem(), 0);
      WxResourceTreeItemData *sel = p_treeCtrl->getSelection();
      outcome += " selected=" + (sel ? sel->getUrl().toString() : string("-")) +
                 " up=" + (p_treeCtrl->canSelectParent() ? "yes" : "no");
    } else if (command == "treerects") {
      // "iconw=<normal icon width>", then for each drawn node
      // "depth:label:x:textx:state": where the label starts, in tree pixels,
      // and its run state (-1 none).
      for (int i = 0; i < 5; i++) wxYield();
      std::function<void(wxTreeItemId, int)> walk =
          [&](wxTreeItemId id, int depth) {
        wxTreeItemIdValue cookie;
        for (wxTreeItemId c = p_treeCtrl->GetFirstChild(id, cookie);
             c.IsOk(); c = p_treeCtrl->GetNextChild(id, cookie)) {
          wxRect all, text;
          if (p_treeCtrl->GetBoundingRect(c, all, false) &&
              p_treeCtrl->GetBoundingRect(c, text, true)) {
            outcome += " " + std::to_string(depth) + ":" +
                       string(p_treeCtrl->GetItemText(c).ToUTF8()) + ":" +
                       std::to_string(all.x) + ":" + std::to_string(text.x) +
                       ":" + std::to_string(p_treeCtrl->GetItemState(c));
          }
          if (p_treeCtrl->IsExpanded(c)) walk(c, depth + 1);
        }
      };
      int iconW = 0, iconH = 0;
      if (p_treeCtrl->GetImageList())
        p_treeCtrl->GetImageList()->GetSize(0, iconW, iconH);
      outcome = "ok iconw=" + std::to_string(iconW);
      walk(p_treeCtrl->GetRootItem(), 0);
    } else if (command == "treeflags") {
      // "depth:label" with "+" for a node drawn with an expander and "-"
      // for one that is open, for each node below the hidden root
      std::function<void(wxTreeItemId, int)> walk =
          [&](wxTreeItemId id, int depth) {
        wxTreeItemIdValue cookie;
        for (wxTreeItemId c = p_treeCtrl->GetFirstChild(id, cookie);
             c.IsOk(); c = p_treeCtrl->GetNextChild(id, cookie)) {
          outcome += " " + std::to_string(depth) + ":" +
                     string(p_treeCtrl->GetItemText(c).ToUTF8()) +
                     (p_treeCtrl->ItemHasChildren(c) ? "+" : "") +
                     (p_treeCtrl->IsExpanded(c) ? "-" : "");
          walk(c, depth + 1);
        }
      };
      outcome = "ok";
      walk(p_treeCtrl->GetRootItem(), 0);
    } else if (command == "toolsmenu") {
      // "toolsmenu <url>": the labels of the Tools menu for <url>.
      WxResourceTreeItemData *node = findNode(EcceURL(calcName), false, false);
      if (!node) {
        outcome = "not in the tree";
      } else {
        wxMenu menu;
        getToolsMenu(menu, node);
        outcome = "ok";
        for (size_t i = 0; i < menu.GetMenuItemCount(); i++) {
          wxMenuItem *it = menu.FindItemByPosition(i);
          if (!it->IsSeparator())
            outcome += " item=" + string(it->GetItemLabel().ToUTF8());
        }
      }
    } else if (command == "helpmenu") {
      // "helpmenu": the Help menu's labels, and the page that Help >
      // Running Codes on This Computer opens and whether it is installed.
      wxMenuBar *mb = GetMenuBar();
      wxMenu *hm = mb->GetMenu(mb->FindMenu(_("&Help")));
      outcome = "ok";
      for (size_t i = 0; i < hm->GetMenuItemCount(); i++) {
        wxMenuItem *it = hm->FindItemByPosition(i);
        if (!it->IsSeparator())
          outcome += " item=" + string(it->GetItemLabelText().ToUTF8());
      }
      string page = WxHelpViewer::pageForKey("RunningCodes");
      string dir = WxHelpViewer::helpDir();
      outcome += " page=" + page + " installed=" +
                 (access((dir + "/index.html").c_str(), R_OK) != 0 ? "no" :
                  access((dir + "/" + page).c_str(), R_OK) == 0 ? "yes" :
                  "missing");
    } else if (command == "toolsclick") {
      // "toolsclick <label>": choose that item of the Tools menu for the
      // current selection; the launch it sends is traced by startApp.
      wxMenu menu;
      getToolsMenu(menu, 0);
      outcome = "no such item";
      for (size_t i = 0; i < menu.GetMenuItemCount(); i++) {
        wxMenuItem *it = menu.FindItemByPosition(i);
        if (it->IsSeparator() || !it->IsEnabled() ||
            string(it->GetItemLabelText().ToUTF8()) != calcName) continue;
        wxCommandEvent ev(wxEVT_MENU, it->GetId());
        if (it->GetId() == wxID_NEWSTRUCTURE) OnNewStructureClick(ev);
        else ProcessEvent(ev);
        outcome = "ok";
        break;
      }
    } else if (command == "filemenu" || command == "newfrommenu") {
      // "filemenu": labels of the File menu's New submenu, as the menu is
      // now. "newfrommenu LABEL": click File > New > LABEL, no name
      // prompt (the resource is named by its type label).
      std::istringstream args(calcName);
      string label, name;
      args >> label >> name;
      wxMenuBar *mb = GetMenuBar();
      wxMenu *fm = mb->GetMenu(mb->FindMenu(_("File")));
      outcome = command == "filemenu" ? "ok" : "no such item";
      for (size_t i = 0; i < fm->GetMenuItemCount(); i++) {
        wxMenuItem *it = fm->FindItemByPosition(i);
        if (!it->GetSubMenu() || it->GetItemLabelText() != "New") continue;
        wxMenu *sub = it->GetSubMenu();
        for (size_t j = 0; j < sub->GetMenuItemCount(); j++) {
          wxMenuItem *si = sub->FindItemByPosition(j);
          string text = string(si->GetItemLabelText().ToUTF8());
          if (command == "filemenu") {
            outcome += " new=" + text;
          } else if (text == label) {
            bool ask = GetMenuBar()->IsChecked(wxID_ASKFORNAME);
            GetMenuBar()->Check(wxID_ASKFORNAME, false);
            wxCommandEvent ev(wxEVT_MENU, si->GetId());
            OnNewResourceClick(ev);
            GetMenuBar()->Check(wxID_ASKFORNAME, ask);
            outcome = "ok";
          }
        }
      }
    } else if (command == "contextmenu") {
      // "contextmenu <url>": the tree's item-menu event for <url>, as a
      // right click sends it, then the labels of the menu's New submenu.
      WxResourceTreeItemData *node = findNode(EcceURL(calcName), false, false);
      if (!node) {
        outcome = "not in the tree";
      } else {
        auto newItems = [this]() {
          wxMenu menu;
          getContextMenu(menu);
          wxMenuItem *newItem = menu.FindItem(wxID_NEW);
          wxMenu *sub = newItem ? newItem->GetSubMenu() : 0;
          string items;
          for (size_t i = 0; sub && i < sub->GetMenuItemCount(); i++) {
            wxMenuItem *it = sub->FindItemByPosition(i);
            if (!it->IsSeparator())
              items += string(it->GetItemLabelText().ToUTF8()) + "|";
          }
          return items;
        };
        outcome = "before=" + newItems();
        wxTreeEvent ev(wxEVT_TREE_ITEM_MENU, p_treeCtrl, node->GetId());
        p_treeCtrl->GetEventHandler()->ProcessEvent(ev);
        for (int i = 0; i < 5; i++) wxYield();
        WxResourceTreeItemData *sel = p_treeCtrl->getSelection();
        outcome += " selection=" +
                   (sel ? sel->getUrl().toString() : string("none")) +
                   " new=" + newItems();
      }
    } else if (command == "progress") {
      const string url = calcName.substr(0, calcName.find(' '));
      const string file = calcName.size() > url.size() + 1
                          ? calcName.substr(url.size() + 1) : string();
      std::ifstream in(file.c_str());
      std::stringstream text;
      text << in.rdbuf();
      progressUpdate(EcceURL(url), in ? text.str() : string());
      outcome = in ? "ok" : "no file " + file;
    } else if (command == "contact") {
      // "contact <url> lost <epoch>" or "contact <url> ok", as the job
      // store's contact notice arrives.
      const string url = calcName.substr(0, calcName.find(' '));
      const string value = calcName.size() > url.size() + 1
                           ? calcName.substr(url.size() + 1) : string("ok");
      contactUpdate(EcceURL(url), value, "");
      outcome = "ok";
    } else if (command == "checklines") {
      // "checklines <url> <line>|<line>": what a machine check said.
      const string url = calcName.substr(0, calcName.find(' '));
      vector<string> lines;
      string rest = calcName.size() > url.size() + 1
                    ? calcName.substr(url.size() + 1) : string();
      for (size_t at; (at = rest.find('|')) != string::npos; rest.erase(0, at + 1))
        lines.push_back(rest.substr(0, at));
      if (!rest.empty()) lines.push_back(rest);
      p_machineCheck[progressKey(EcceURL(url))] = lines;
      contactChanged(EcceURL(url));
      outcome = "ok";
    } else if (command == "fold") {
      // "fold open|shut", as a click on the box's heading does.
      CalculationContextPanel *panel =
          dynamic_cast<CalculationContextPanel*>(p_contextPanel);
      ProgressBox *box = panel ? panel->getProgressBox() : 0;
      if (box) box->setCollapsed(calcName == "shut");
      for (int i = 0; i < 5; i++) wxYield();
      outcome = box ? string("folded=") + (box->collapsed() ? "1" : "0")
                    : string("no box");
    } else if (command == "treestate") {
      // "treestate <url>": the tree icon shown, against those of running
      // and of out of contact.
      WxResourceTreeItemData *node = p_treeCtrl->findNode(EcceURL(calcName),
                                                          true, true);
      if (!node) {
        outcome = "no node";
      } else {
        double lastHeard = 0;
        Resource *r = node->getResource();
        outcome = "image=" + std::to_string(p_treeCtrl->GetItemState(node->GetId())) +
            " running=" + std::to_string((int)ResourceDescriptor::STATE_RUNNING) +
            " lost=" + std::to_string((int)WxState::OUT_OF_CONTACT) +
            " outOfContact=" + (outOfContact(r, lastHeard) ? "1" : "0");
      }
    } else if (command == "iconstyle") {
      // "iconstyle flat|shaded": what Preferences > Colours does, message
      // included, then two seconds for the message to come back.
      Preferences prefs(PrefLabels::GLOBALPREFFILE);
      prefs.setString(PrefLabels::STATEICONSTYLE, calcName);
      prefs.saveFile();
      WxState::refreshStyle();
      JMSMessage *msg = newMessage();
      publish("ecce_preferences_misc", *msg);
      delete msg;
      for (int i = 0; i < 40; i++) { wxYield(); wxMilliSleep(50); }
      outcome = "ok";
    } else if (command == "treeicon") {
      // "treeicon <url>": a checksum of the pixels of the state icon shown.
      WxResourceTreeItemData *node = p_treeCtrl->findNode(EcceURL(calcName),
                                                          true, true);
      wxImageList *list = p_treeCtrl->GetStateImageList();
      int state = node ? p_treeCtrl->GetItemState(node->GetId()) : -1;
      if (!node || !list || state < 0) {
        outcome = "no node";
      } else {
        wxImage img = list->GetBitmap(state).ConvertToImage();
        unsigned long sum = 0;
        const size_t n = (size_t)img.GetWidth() * img.GetHeight();
        for (size_t i = 0; i < 3 * n; i++) sum = sum * 31 + img.GetData()[i];
        if (img.HasAlpha())
          for (size_t i = 0; i < n; i++) sum = sum * 31 + img.GetAlpha()[i];
        outcome = "sum=" + std::to_string(sum);
      }
    } else if (command == "size") {
      int w = 0, h = 0;
      std::istringstream(calcName) >> w >> h;
      if (w > 0 && h > 0) SetSize(w, h);
      for (int i = 0; i < 5; i++) wxYield();
      outcome = "ok " + std::to_string(GetSize().x) + "x" +
                std::to_string(GetSize().y);
    } else if (command == "firstvisible") {
      for (int i = 0; i < 5; i++) wxYield();
      wxTreeItemId first = p_treeCtrl->GetFirstVisibleItem();
      outcome = first.IsOk() ? string(p_treeCtrl->GetItemText(first).utf8_str())
                             : string("none");
    } else if (command == "showbox") {
      CalculationContextPanel *panel =
          dynamic_cast<CalculationContextPanel*>(p_contextPanel);
      ProgressBox *box = panel ? panel->getProgressBox() : 0;
      for (int i = 0; i < 5; i++) wxYield();
      if (box) {
        int py = 1;
        panel->GetScrollPixelsPerUnit(0, &py);
        panel->Scroll(-1, panel->CalcUnscrolledPosition(box->GetPosition()).y /
                              std::max(1, py));
        for (int i = 0; i < 5; i++) wxYield();
      }
      outcome = box ? "scrolled" : "no box";
    } else if (command == "scrollsummary") {
      CalculationContextPanel *panel =
          dynamic_cast<CalculationContextPanel*>(p_contextPanel);
      for (int i = 0; i < 5; i++) wxYield();
      if (panel) {
        const wxSize virtualSize = panel->GetVirtualSize();
        const wxSize clientSize = panel->GetClientSize();
        int unitY = 1;
        panel->GetScrollPixelsPerUnit(0, &unitY);
        const int maxY = std::max(0, (virtualSize.y - clientSize.y +
                                      std::max(1, unitY) - 1) /
                                     std::max(1, unitY));
        if (maxY > 0) {
          panel->Scroll(-1, maxY);
          for (int i = 0; i < 5; i++) wxYield();
          panel->Scroll(-1, 0);
          for (int i = 0; i < 5; i++) wxYield();
          outcome = "scrolled to " + std::to_string(maxY) + " and back";
        } else {
          outcome = "no scroll range";
        }
      } else {
        outcome = "no calculation panel";
      }
    } else if (command == "progressbox") {
      CalculationContextPanel *panel =
          dynamic_cast<CalculationContextPanel*>(p_contextPanel);
      ProgressBox *box = panel ? panel->getProgressBox() : 0;
      for (int i = 0; i < 5; i++) wxYield();
      if (!box) {
        outcome = "no box";
      } else {
        // An unwanted box stays GTK-visible at zero size (hidden children
        // get allocated while scrolling), so visible means a non-empty area.
        const bool shown = box->IsShownOnScreen() &&
                           box->GetSize().x > 0 && box->GetSize().y > 0;
        outcome = string("shown=") + (shown ? "1" : "0") +
                  " paints=" + std::to_string(box->paints()) +
                  " panel=" + std::to_string((unsigned long)(uintptr_t)panel) +
                  " size=" + std::to_string(box->GetSize().x) + "x" +
                  std::to_string(box->GetSize().y) +
                  " stateicon=" + (panel->getStateIcon() &&
                                   panel->getStateIcon()->outOfContact()
                                   ? "running?" : "plain") +
                  " copyvalue=" + box->copyText(0) +
                  " energyselectable=" + (box->energySelectable() ? "1" : "0");
        for (const string& l : box->describe()) outcome += " | " + l;
      }
    } else if (command == "allocations") {
      // "allocations": opens Tools > Allocations, waits up to 10 s for the
      // answers and prints each row as "ALLOC: machine|account|used|checked".
      AllocationsDialog *d = new AllocationsDialog(this);
      d->Show();
      for (int i = 0; i < 100 && (i == 0 || d->checking()); i++) {
        wxMilliSleep(100);
        wxYield();
      }
      for (const string& r : d->rowTexts())
        fprintf(stderr, "ALLOC: %s\n", r.c_str());
      outcome = "ok " + std::to_string(d->rowTexts().size()) + " rows";
      d->Destroy();
    } else if (command == "compareenergies" || command == "comparedialog") {
      // "compareenergies <sideA> <sideB>": the table and notes the dialog
      // shows; a side is "[-][N*]url,[-][N*]url".  "comparedialog <sideA>
      // <sideB> [png]": the dialog itself, its text areas, and a capture.
      std::istringstream args(calcName);
      string ua, ub, png;
      args >> ua >> ub >> png;
      if (command == "compareenergies") {
        const SumResult c = sumEnergies(gatherSide(ua), gatherSide(ub));
        outcome = "ok";
        std::istringstream lines(sumText(c, false));
        for (string l; std::getline(lines, l); ) outcome += " | " + l;
      } else {
        // Sides as in compareenergies; the lists are those of the project
        // of the first calculation.
        CompareEnergiesDialog *d = new CompareEnergiesDialog(this,
            calculationsIn(EcceURL(parseSide(ua)[0].first).getParent().toString()),
            parseSide(ua), parseSide(ub));
        d->Show();
        for (int i = 0; i < 5; i++) wxYield();
        if (!png.empty()) ecceClientShot(d, wxString::FromUTF8(png.c_str()));
        outcome = "ok";
        std::istringstream r(d->resultText() + "Notes:\n" + d->notesText() +
                             "Copy:\n" + d->copyText());
        for (string l; std::getline(r, l); ) outcome += " | " + l;
        d->Destroy();
      }
    } else if (command == "table") {
      // "table [PROJECT-URL]": select the project (when given) and report
      // its table's Progress cells, one "name=text" each.
      if (!calcName.empty()) {
        WxResourceTreeItemData *node = findNode(EcceURL(calcName), true, true);
        if (node) p_treeCtrl->SelectItem(node->GetId());
      }
      for (int i = 0; i < 5; i++) wxYield();
      ProjectContextPanel *table = dynamic_cast<ProjectContextPanel*>(p_contextPanel);
      if (!table) {
        outcome = "no table";
      } else {
        outcome = "panel=" + std::to_string((unsigned long)(uintptr_t)table) +
                  " paints=" + std::to_string(table->progressPaints());
        for (const string& c : table->progressCells()) outcome += " | " + c;
      }
    } else if (command == "tablestates") {
      // The shown table's State cells, "name=state" ("running?": #284).
      ProjectContextPanel *table = dynamic_cast<ProjectContextPanel*>(p_contextPanel);
      outcome = table ? "states" : "no table";
      if (table)
        for (const string& c : table->stateCells()) outcome += " | " + c;
    } else if (command == "snap") {
      for (int i = 0; i < 3; i++) {
        Update();
        wxTheApp->Yield(true);
        wxMilliSleep(50);
      }
      outcome = ecceClientShot(this, wxString::FromUTF8(calcName.c_str()))
          ? "saved" : "not saved";
    } else if (command == "tail-close") {
      if (p_testTail) p_testTail->Close();
      outcome = "ok";
    } else if (command == "counts") {
      // Broker notices received, tree/summary updates made for them, and
      // the batches they came in, since the last "counts".
      outcome = "notices=" + std::to_string(p_testNotices) +
                " applied=" + std::to_string(p_testApplied) +
                " batches=" + std::to_string(p_testBatches);
      p_testNotices = p_testApplied = p_testBatches = 0;
    } else if (command == "newprompt") {
      // "newprompt PARENT-URL CODE": File > New <code> Calculation with the
      // name prompt, as the menu does it (disabler included); the prompt is
      // answered OK after 1.5 s.
      std::istringstream args(calcName);
      string parentUrl, code;
      args >> parentUrl >> code;
      ResourceType *rt = ResourceDescriptor::getResourceDescriptor()
          .getResourceType("virtual_document", "ecceCalculation", code);
      WxResourceTreeItemData *parentNode = findNode(EcceURL(parentUrl),
                                                    true, true);
      if (!rt || !parentNode) {
        outcome = !rt ? "no resource type " + code : "no parent node";
      } else {
        wxTimer answer;
        answer.Bind(wxEVT_TIMER, [](wxTimerEvent&) {
          for (wxWindowList::compatibility_iterator n =
                 wxTopLevelWindows.GetFirst(); n; n = n->GetNext()) {
            wxDialog *d = dynamic_cast<wxDialog*>(n->GetData());
            if (d && d->IsModal()) d->EndModal(wxID_OK);
          }
        });
        answer.StartOnce(1500);
        bool ask = GetMenuBar()->IsChecked(wxID_ASKFORNAME);
        GetMenuBar()->Check(wxID_ASKFORNAME, true);
        startDisabler();
        createResource(rt, parentNode, false);
        stopDisabler();
        GetMenuBar()->Check(wxID_ASKFORNAME, ask);
        outcome = "ok";
      }
    } else if (command == "busyprompt") {
      // A text prompt shown under a menu action's busy cursor and window
      // disabler, answered after 1.5 s: ewxModalGuard must take the busy
      // cursor off while it is up and put it back afterwards.
      wxTimer answer;
      answer.Bind(wxEVT_TIMER, [](wxTimerEvent&) {
        for (wxWindowList::compatibility_iterator n =
               wxTopLevelWindows.GetFirst(); n; n = n->GetNext()) {
          wxDialog *d = dynamic_cast<wxDialog*>(n->GetData());
          if (d && d->IsModal()) {
            cerr << "ECCE_TEST_ORGANIZER: busyprompt: up busy="
                 << (int)wxIsBusy() << " parentEnabled="
                 << (d->GetParent() ? (int)d->GetParent()->IsEnabled() : -1)
                 << endl;
            d->EndModal(wxID_OK);
          }
        }
      });
      answer.StartOnce(1500);
      startDisabler();
      ewxTextEntryDialog prompt(this, "Name:", "Busy Prompt", "x",
                                wxOK | wxCANCEL);
      const int rc = prompt.ShowModal();
      outcome = string(rc == wxID_OK ? "ok" : "cancelled") +
                " busyAfter=" + (wxIsBusy() ? "1" : "0");
      stopDisabler();
    } else if (command == "click") {
      // "click URL": selects a row already in the tree as a click does,
      // with no tree loading first (unlike "summary").
      WxResourceTreeItemData *node = findNode(EcceURL(calcName), false, false);
      if (!node) {
        outcome = "not in the tree";
      } else {
        p_treeCtrl->SelectItem(node->GetId());
        outcome = "ok";
      }
    } else if (command == "duplicate") {
      std::istringstream args(calcName);
      string url, how;
      args >> url >> how;
      WxResourceTreeItemData *node = findNode(EcceURL(url), true, true);
      if (!node) {
        outcome = "not in the tree";
      } else {
        p_treeCtrl->SelectItem(node->GetId());
        for (int i = 0; i < 5; i++) wxYield();
        std::vector<string> events;
        p_testTreeEvents = &events;
        wxCommandEvent ev;
        if (how == "last") OnDuplicate2Click(ev);
        else OnDuplicateClick(ev);
        for (int i = 0; i < 5; i++) wxYield();
        p_testTreeEvents = 0;
        outcome = "ok";
        for (size_t i = 0; i < events.size(); i++) outcome += " " + events[i];
        outcome += " |";
        std::function<void(wxTreeItemId, int)> walk =
            [&](wxTreeItemId id, int depth) {
          wxTreeItemIdValue cookie;
          for (wxTreeItemId c = p_treeCtrl->GetFirstChild(id, cookie);
               c.IsOk(); c = p_treeCtrl->GetNextChild(id, cookie)) {
            bool sel = p_treeCtrl->GetSelection() == c;
            outcome += string(" ") + (sel ? "[" : "") + std::to_string(depth)
                       + ":" + string(p_treeCtrl->GetItemText(c).ToUTF8());
            walk(c, depth + 1);
          }
        };
        walk(p_treeCtrl->GetRootItem(), 0);
      }
    } else if (command == "launchselected" || command == "launchselectedshot") {
      // "launchselected URL...": Launch Selected without its dialogs, one
      // "name: result" a line (joined by |); "launchselectedshot PNG URL...":
      // only the summary dialog, saved to PNG.
      std::istringstream args(calcName);
      string png, u;
      if (command == "launchselectedshot") args >> png;
      // "on=MACHINE" and "profile=NAME" before the URLs launch on that machine.
      LaunchBatch::Target target;
      vector<EcceURL> urls;
      while (args >> u) {
        if (u.compare(0, 3, "on=") == 0) target.machine = u.substr(3);
        else if (u.compare(0, 8, "profile=") == 0) target.profile = u.substr(8);
        else urls.push_back(EcceURL(u));
      }
      launchSelected(urls, false, &outcome, png, target);
    } else if (command == "launchselectedasync") {
      // "launchselectedasync URL...": starts the batch and answers at once;
      // "launchselectedstatus" tells how far it is.
      std::istringstream args(calcName);
      string u;
      vector<EcceURL> urls;
      while (args >> u) urls.push_back(EcceURL(u));
      launchSelectedAsync(urls, &outcome);
    } else if (command == "launchselectedstatus") {
      outcome = launchBatchStatus();
    } else if (command == "reconnectselectedlist" || command == "reconnectselected"
               || command == "reconnectselectedshot") {
      // "reconnectselectedlist URL...": what the dialog would list, one
      // "name|machine|state|ok or reason" a line (joined by ;);
      // "reconnectselected URL...": the batch without its dialogs;
      // "reconnectselectedshot PNG URL...": only the dialog, saved to PNG.
      std::istringstream args(calcName);
      string png, u;
      if (command == "reconnectselectedshot") args >> png;
      vector<EcceURL> urls;
      while (args >> u) urls.push_back(EcceURL(u));
      reconnectSelected(urls, false, &outcome, png, command == "reconnectselectedlist");
    } else if (command == "reparseoutputs") {
      // "reparseoutputs URL...": Re-Parse Outputs without dialogs.
      std::istringstream args(calcName);
      string u;
      vector<EcceURL> urls;
      while (args >> u) urls.push_back(EcceURL(u));
      reparseOutputs(urls, false, &outcome);
    } else if (command == "reconnectjob") {
      // "reconnectjob URL": the single Run Mgmt > Reconnect, without dialogs.
      WxResourceTreeItemData *node = findNode(EcceURL(calcName), true, true);
      p_reconnectNote = "no such calculation";
      if (node) reconnectJob(node);
      outcome = p_reconnectNote;
    } else if (command == "launchselectionrows") {
      // "launchselectionrows URL...": selects those rows in the project table
      // with the focus on the tree, then reports what Launch Selected would
      // use, without launching.
      std::istringstream args(calcName);
      string u;
      vector<EcceURL> urls;
      while (args >> u) urls.push_back(EcceURL(u));
      WxResourceTreeItemData *parent = urls.empty() ? 0 :
          findNode(urls[0].getParent(), true, true);
      if (parent) {
        p_treeCtrl->SelectItem(parent->GetId());
        for (int i = 0; i < 40; i++) { wxYield(); wxMilliSleep(50); }
      }
      if (p_contextPanel) p_contextPanel->selectResources(urls);
      p_focusOnTree = true;
      vector<EcceURL> used = launchSelection();
      outcome = "ok " + std::to_string(used.size());
      for (size_t i = 0; i < used.size(); i++) outcome += " " + used[i].toString();
    } else if (command == "projectcontextmenu") {
      // "projectcontextmenu URL...": selects those rows in the project table
      // as launchselectionrows does, then lists the item labels the table's
      // right-click menu offers (joined by |).
      std::istringstream args(calcName);
      string u;
      vector<EcceURL> urls;
      while (args >> u) urls.push_back(EcceURL(u));
      WxResourceTreeItemData *parent = urls.empty() ? 0 :
          findNode(urls[0].getParent(), true, true);
      if (parent) {
        p_treeCtrl->SelectItem(parent->GetId());
        for (int i = 0; i < 40; i++) { wxYield(); wxMilliSleep(50); }
      }
      if (p_contextPanel) p_contextPanel->selectResources(urls);
      wxMenu menu;
      getContextMenu(menu);
      outcome = "ok";
      for (size_t i = 0; i < menu.GetMenuItemCount(); i++) {
        wxMenuItem *it = menu.FindItemByPosition(i);
        if (!it->IsSeparator())
          outcome += "|" + string(it->GetItemLabelText().ToUTF8());
      }
    } else if (command == "treecontextmenu") {
      // "treecontextmenu URL": selects URL in the tree and lists the items
      // its right-click menu offers (joined by |), "~" before a disabled one.
      WxResourceTreeItemData *node = findNode(EcceURL(calcName), true, true);
      if (node) {
        p_treeCtrl->SelectItem(node->GetId());
        for (int i = 0; i < 40; i++) { wxYield(); wxMilliSleep(50); }
      }
      wxMenu menu;
      getContextMenu(menu);
      outcome = "ok";
      for (size_t i = 0; i < menu.GetMenuItemCount(); i++) {
        wxMenuItem *it = menu.FindItemByPosition(i);
        if (!it->IsSeparator())
          outcome += string("|") + (it->IsEnabled() ? "" : "~") +
                     string(it->GetItemLabelText().ToUTF8());
      }
    } else if (command == "rmtask") {
      // "rmtask URL LABEL": selects URL and runs its Run Mgmt menu item
      // LABEL (e.g. Rerun) as the menu does; a prompt it shows is answered
      // Cancel after 1.5 s.
      std::istringstream args(calcName);
      string url, label;
      args >> url;
      std::getline(args >> std::ws, label);
      WxResourceTreeItemData *node = findNode(EcceURL(url), true, true);
      wxMenuBar *bar = GetMenuBar();
      if (!node) {
        outcome = "not in the tree";
      } else {
        p_treeCtrl->SelectItem(node->GetId());
        for (int i = 0; i < 5; i++) wxYield();
        wxMenu *rm = bar->GetMenu(bar->FindMenu(_("Run Mgmt")));
        int id = rm ? rm->FindItem(label) : wxNOT_FOUND;
        if (id == wxNOT_FOUND) {
          outcome = "no Run Mgmt item " + label;
        } else {
          wxTimer answer;
          answer.Bind(wxEVT_TIMER, [](wxTimerEvent&) {
            for (wxWindowList::compatibility_iterator n =
                   wxTopLevelWindows.GetFirst(); n; n = n->GetNext()) {
              wxDialog *d = dynamic_cast<wxDialog*>(n->GetData());
              if (d && d->IsModal()) d->EndModal(wxID_CANCEL);
            }
          });
          answer.StartOnce(1500);
          wxCommandEvent ev(wxEVT_MENU, id);
          GetEventHandler()->ProcessEvent(ev);
          outcome = "ok";
        }
      }
    } else if (command == "prefs") {
      // "prefs open|colours jmol|colours ecce|advanced KEY on|childenv VAR|state|snap PATH": the
      // Preferences window, its Colours tab as a click chooses.
      std::istringstream args(calcName);
      string what, arg;
      args >> what >> arg;
      if (p_prefs == (GlobalPrefs*)0) p_prefs = new GlobalPrefs(this);
      p_prefs->Show(true);
      p_prefs->showColoursPage();
      for (int i = 0; i < 10; i++) wxYield();
      if (what == "colours") {
        outcome = p_prefs->chooseColourScheme(arg) ? "ok" : "unknown scheme";
      } else if (what == "advanced") {
        // "prefs advanced KEY on|off": a click on that Advanced check box.
        string state;
        args >> state;
        outcome = p_prefs->tickAdvanced(arg, state == "on") ? "ok" : "unknown switch";
      } else if (what == "childenv") {
        // "prefs childenv VAR": what a program started now would see.
        string cmd = "printf %s \"${" + arg + "-UNSET}\"";
        FILE *pipe = popen(cmd.c_str(), "r");
        char buf[512];
        outcome = arg + "=";
        while (pipe && fgets(buf, sizeof buf, pipe)) outcome += buf;
        if (pipe) pclose(pipe);
      } else if (what == "state") {
        outcome = "colours=" + p_prefs->colourSchemeShown();
      } else if (what == "snap") {
        outcome = ecceClientShot(p_prefs, wxString::FromUTF8(arg.c_str()))
            ? "saved" : "not saved";
      } else {
        outcome = "ok";
      }
    } else if (command == "start") {
      // "start TOOL [URL]": what a tool button does (startApp), e.g.
      // "start CalculationEditor <calc url>".
      std::istringstream args(calcName);
      string tool, url;
      args >> tool >> url;
      startApp(tool, 0, url);
      outcome = "requested";
    } else if (command == "probe") {
      // What a user sees first: the registration notice, whether the home
      // is in the tree, and the selection the tree's focus handler settles
      // on when nothing was selected yet (wxMSW).
      bool selected = p_treeCtrl->GetSelection().IsOk();
      WxResourceTreeItemData *homeNode = findNode(home, false, false);
      onSelectionChange(true);
      WxResourceTreeItemData *sel = p_treeCtrl->getSelection();
      outcome = "home=" + home.toString() +
                " homenode=" + (homeNode ? "yes" : "no") +
                " selected-before=" + (selected ? "yes" : "no") +
                " selection=" + (sel ? sel->getUrl().toString() : "none") +
                " register=" + (s_registerMessage.empty() ? "known"
                    : s_registerMessage.find("has just been added") !=
                      string::npos ? "added" : "failed");
    } else if (command == "close") {
      int n = atoi(calcName.c_str());
      if (n < 1) n = 1;
      for (int i = 0; i < n; i++) CallAfter([this]() { Close(); });
      outcome = "queued";
    } else if (command == "answer") {
      outcome = "no dialog";
      for (wxWindowList::iterator it = wxTopLevelWindows.begin();
           it != wxTopLevelWindows.end(); ++it) {
        wxDialog *dlg = wxDynamicCast(*it, wxDialog);
        if (dlg && dlg->IsModal()) {
          dlg->EndModal(calcName == "stop" ? (int)ID_ORGANIZER_QUIT_STOP_SERVER
                                           : (int)wxID_OK);
          outcome = "answered";
          break;
        }
      }
    } else if (command == "reportproblem") {
      // Help > Report a Problem... without its dialog: the archive path
      wxString error;
      wxString archive = ewxReportProblem::collect(error, this);
      outcome = archive.empty() ? "failed: " + string(error.utf8_str())
                                : "archive " + string(archive.utf8_str());
    } else if (line == "quit-offer") {
      bool inUse = false;
      outcome = offerStopServer(inUse) ? "stop offered"
                : inUse ? "stop withheld, services in use" : "stop withheld";
    } else {
      outcome = "unknown command";
    }
  } catch (EcceException& ex) {
    outcome = string("exception: ") + ex.what();
  } catch (std::exception& ex) {
    outcome = string("exception: ") + ex.what();
  }
  cerr << "ECCE_TEST_ORGANIZER: " << line << ": " << outcome << endl;
}


/**
 * Destructor.
 * Save preference, disconnect event handling functions, clean memory.
 */
CalcMgr::~CalcMgr( )
{
  delete p_releaseNotice;
  saveSettings();
  int low, high;
  ResourceDescriptor rs = ResourceDescriptor::getResourceDescriptor();
  rs.getResourceIdRange(low, high);
  p_treeCtrl->PopEventHandler(true);
  Disconnect(low, high,
             wxEVT_COMMAND_MENU_SELECTED,
             wxCommandEventHandler(CalcMgr::OnNewResourceClick));
  rs.getRmTaskIdRange(low, high);
  Disconnect(low, high,
             wxEVT_COMMAND_MENU_SELECTED,
             wxCommandEventHandler(CalcMgr::OnRmTaskClick));
  rs.getToolIdRange(low, high);
  Disconnect(low, high,
             wxEVT_COMMAND_MENU_SELECTED,
             wxCommandEventHandler(CalcMgr::OnToolClick));

  if (p_hlist != 0) delete p_hlist;
}


/**
 * Initialize the GUI construction.
 * This function completed the GUI construction where the super class CalcMgrGUI
 * didn't finish. Mainly it creates the legend panel and put legend panel,
 * feedback widget into the right places.
 */
void CalcMgr::initializeGUI()
{
  wxMenu * menu = new wxMenu();

  // Get Registry
  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();

  // Get property view names from registry
  multimap<int, PropertyView*> propViews =
    rd.getPropertyViews();

  // Append menu items
  multimap<int, PropertyView*>::iterator mapIt;
  p_projectViewId = propViews.begin()->second->getId();

  for (mapIt = propViews.begin();
       mapIt != propViews.end();
       mapIt++) {
    PropertyView *pv = ((*mapIt).second);
    menu->AppendRadioItem(pv->getId(), pv->getMenuItem());
  }

  wxMenuBar * menuBar = GetMenuBar();
  menuBar->GetMenu(menuBar->FindMenu(_("Options")))->
    Append(wxID_ANY, _("Project View"), menu);


  p_legend = WxState::createLegend(this);

  p_treeCtrl->setOutOfContactTest([this](Resource *r) {
    double lastHeard = 0;
    return outOfContact(r, lastHeard);
  });

  p_topSizer->Add(p_legend, 0, wxGROW|wxALL, 0);

  // Feedback area at the botton of the page, no need to delete it since
  // wxFrame destructor will delete it.
  p_feedback = new WxFeedback(this);
  p_feedback->hideEditState();
  p_feedback->hideStateIcon();
  p_feedback->hideStateIcon();
  p_topSizer->Add(p_feedback, 0, wxGROW, 0);
  p_contextPanel = 0;
  p_splitterWindow->SetMinimumPaneSize(200);

  SetMinSize(wxSize(500, 400));
}


/**
 * Obtain the access to the data server and initialize the treectrl widget.
 */
void CalcMgr::initializeTreeData()
{
  p_treeCtrl->initialize(p_hlist,
                         GetMenuBar()->IsChecked(wxID_HOMEASROOT),
                         GetMenuBar()->IsChecked(wxID_SHOW_RUNSTATES),
                         p_treeSort);
  p_treeCtrl->mountEDSIProviders();
  p_treeCtrl->mountServerList(p_serverListEditor);
  p_treeCtrl->SetFocus();
}


/**
 *
 */
void CalcMgr::progressNotice(ProgressEvent& event)
{
  event.m_stop = false;
}


/**
 * Save current settings to the preference files.
 */
void CalcMgr::saveSettings()
{
  string tmp = Ecce::realUserPrefPath();
  tmp += "/OrganizerHistory";
  EcceURL url = tmp;
  p_hlist->save(url);

  Preferences prefs("Organizer");

  saveWindowSettings("Organizer", prefs, true);
  

  prefs.setInt("Organizer.TreeWidth",
                  p_splitterWindow->GetSashPosition());
  prefs.setInt("Organizer.TreeSort",
                  p_treeSort);
  prefs.setInt("Organizer.PropertyView",
                  p_projectViewId);
  prefs.setBool("Organizer.ConfirmDeletes",
                   GetMenuBar()->IsChecked(wxID_CONFIRM_DELETE));
  prefs.setBool("Organizer.ConfirmResets",
                   GetMenuBar()->IsChecked(wxID_CONFIRM_RESET));
  prefs.setBool("Organizer.Toolbar",
                   GetMenuBar()->IsChecked(wxID_SHOW_TOOLBAR));
  prefs.setBool("Organizer.RunStates",
                   GetMenuBar()->IsChecked(wxID_SHOW_RUNSTATES));
  prefs.setBool("Organizer.Legend",
                   GetMenuBar()->IsChecked(wxID_SHOW_LEGEND));
  prefs.setBool("Organizer.DeepTraverse",
                   GetMenuBar()->IsChecked(wxID_SHOW_INTERNAL));
  prefs.setBool("Organizer.HomeAsRoot",
                   GetMenuBar()->IsChecked(wxID_HOMEASROOT));
  prefs.setBool("Organizer.AskForName",
                   GetMenuBar()->IsChecked(wxID_ASKFORNAME));
  prefs.setBool("Organizer.RunDefaultTool",
                   GetMenuBar()->IsChecked(wxID_RUNDEFAULTTOOL));
  prefs.setBool("Organizer.ShowLine",
                   GetMenuBar()->IsChecked(wxID_SHOW_LINE));
  /*
    prefs.setBool("Organizer.FreezeColumns",
    GetMenuBar()->IsChecked(wxID_FIX_COLUMN));
  */
  
  prefs.saveFile();
}


/**
 * Restore settings using the saved preferences.
 */
void CalcMgr::restoreSettings()
{
  // Set default and always set width regardless of whether it was
  // overridden in prefs.
  Preferences prefs("Organizer");

  int prefTmp = 250;
  if (prefs.getInt("Organizer.TreeWidth", prefTmp))
    p_splitterWindow->SetSashPosition(prefTmp);

  p_treeSort = 0;
  if (prefs.getInt("Organizer.TreeSort", prefTmp))
    p_treeSort = prefTmp;
  GetMenuBar()->Check(wxID_SORT_TYPE + p_treeSort, true);

  if (prefs.getInt("Organizer.PropertyView", prefTmp)) {
    p_projectViewId = prefTmp;
    GetMenuBar()->Check(p_projectViewId, true);
  }

  bool choice = true;
  if (prefs.getBool("Organizer.ConfirmDeletes", choice)) {
    GetMenuBar()->Check(wxID_CONFIRM_DELETE, choice);
  }

  if (prefs.getBool("Organizer.ConfirmResets", choice)) {
    GetMenuBar()->Check(wxID_CONFIRM_RESET, choice);
    // @todo Action here
  }

  if (prefs.getBool("Organizer.Toolbar", choice)) {
    GetMenuBar()->Check(wxID_SHOW_TOOLBAR, choice);
    p_toolBar->Show(choice);
  }

  if (prefs.getBool("Organizer.RunStates", choice)) {
    GetMenuBar()->Check(wxID_SHOW_RUNSTATES, choice);
  }

  if (prefs.getBool("Organizer.Legend", choice)) {
    GetMenuBar()->Check(wxID_SHOW_LEGEND, choice);
    p_legend->Show(choice);
  }

  if (prefs.getBool("Organizer.DeepTraverse", choice)) {
    GetMenuBar()->Check(wxID_SHOW_INTERNAL, choice);
    Resource::setHideInternal(!choice);
  }

  if (prefs.getBool("Organizer.HomeAsRoot", choice)) {
    GetMenuBar()->Check(wxID_HOMEASROOT, choice);
  }

  if (prefs.getBool("Organizer.AskForName", choice)) {
    GetMenuBar()->Check(wxID_ASKFORNAME, choice);
  }

  if (prefs.getBool("Organizer.RunDefaultTool", choice)) {
    GetMenuBar()->Check(wxID_RUNDEFAULTTOOL, choice);
  }

  if (prefs.getBool("Organizer.ShowLine", choice)) {
    GetMenuBar()->Check(wxID_SHOW_LINE, choice);
  }

  /* @todo Do we still have this?
     if (prefs.getBool("Organizer.FreezeColumns", choice)) {
     GetMenuBar()->Check(wxID_FIX_COLUMN, choice);
     // @todo Action here
     }
  */

  setTreeStyle();

  int savedWidth = -1;
  const bool sized = prefs.getInt("Organizer.Width", savedWidth) &&
                     savedWidth > 0;
  restoreWindowSettings("Organizer", prefs, true);
  if (!sized) {
    // A fresh profile has no size; the sizers' minimum (500x400) clips the
    // summary pane, so start at a share of the display instead.
    int idx = wxDisplay::GetFromWindow(this);
    wxRect area = wxDisplay((unsigned)(idx == wxNOT_FOUND ? 0 : idx))
                    .GetClientArea();
    int w = std::max(860, std::min(1100, area.width * 60 / 100));
    int h = std::max(600, std::min(800, area.height * 80 / 100));
    SetSize(std::min(w, area.width), std::min(h, area.height));
    Centre();
  }

  string tmp = Ecce::realUserPrefPath();
  tmp += "/OrganizerHistory";
  EcceURL url = tmp;
  p_hlist->load(url);
}


/**
 * Setup the context panel using the selected node in the treectrl
 */
void CalcMgr::setContextPanel()
{
  WxResourceTreeItemData * newNode = p_treeCtrl->getSelection();
  
  //  cerr << "\nEnter setContextPanel of: " << newNode->getName() << endl;

  p_splitterWindow->SetAutoLayout(false);

  if (newNode == p_panelNode) {
    p_contextPanel->refresh();
  }
  else {
    ContextPanelInterface * contextPanel =
      ContextPanelFactory::createContextPanel(this, newNode, p_splitterWindow);

    wxWindow * rightPanel = p_splitterWindow->GetWindow2();
    
    p_splitterWindow->ReplaceWindow(rightPanel, contextPanel->getWidget());
    
    if (p_contextPanel)
      delete p_contextPanel;
    else
      rightPanel->Destroy();

    p_contextPanel = contextPanel;
    p_panelNode = newNode;

    // automatic job monitoring check
    // Check a single job if the selection is a calculation/task, otherwise
    // iterate over all children of the selected container
    if (newNode->getResource()->hasAccess()) {
      if (dynamic_cast<TaskJob*>(newNode)) {
        string message;
        (void)checkSingleJob(newNode->getResource(), message);
      } else
        checkNodeJobs(newNode);
    }
  }

  // This will fix a scroll issue. Without it the minsize of the
  // SessionProjectContextPanel is not correct.
  p_contextPanel->getWidget()->SetMinSize(wxSize(1, 1));

  p_splitterWindow->SetAutoLayout(true);

  Layout();

  //  cerr << "\nLeave setContextPanel of: " << p_panelNode->getName() << endl;

  /*
    Layout();
    p_splitterWindow->Layout();
    p_scrollWindow->Layout();
    p_scrollWindow->SetSizer(p_contextSizer);

    p_contextSizer->Detach(0);
  
    if (p_contextPanel) delete p_contextPanel;
  
    p_contextPanel = ContextPanelFactory::createContextPanel(this,
    itemData,
    p_scrollWindow);
  
    p_contextSizer->Add(p_contextPanel->getWidget(), 1, wxGROW, 0);
    p_contextPanel->getWidget()->Layout();
    p_contextPanel->getWidget()->Fit();
  
    p_scrollWindow->SetVirtualSize(p_contextPanel->getWidget()->GetSize());
    p_contextSizer->Layout();
    cerr << "Panel size\n"
    << p_contextPanel->getWidget()->GetSize().GetWidth() << "\t"
    << p_contextPanel->getWidget()->GetSize().GetHeight() << "\n";
    cerr << "Scroll Window size\n"
    << p_scrollWindow->GetSize().GetWidth() << "\t"
    << p_scrollWindow->GetSize().GetHeight() << "\n";
    cerr << "Scroll Window client size\n"
    << p_scrollWindow->GetClientSize().GetWidth() << "\t"
    << p_scrollWindow->GetClientSize().GetHeight() << "\n";
    p_scrollWindow->SetAutoLayout(true);
    p_scrollWindow->Layout();
  */
}


/**
 * Add a server to the tree, used by CalcMgrApp::msgDataServerMCB
 */
void CalcMgr::addRoot(const string& name, const string& url)
{
  p_treeCtrl->addRoot(name, url);
}


// All event handling functions


/*!
 * wxEVT_CONTEXT_MENU event handler for ID_FRAME
 */
void CalcMgr::OnContextMenu( wxContextMenuEvent& event )
{
  wxMenu menu;
  getContextMenu(menu);
  p_popupOnTree = true;
  ewxPopupMenu(this, menu);
  p_popupOnTree = false;
  /*
  wxPoint pt = event.GetPosition();
  
  if (p_treeCtrl->GetRect().Contains(p_treeCtrl->ScreenToClient(pt))) {
    wxTreeItemId item = p_treeCtrl->GetSelection();
    //  if ( !HasFlag(wxTR_MULTIPLE) ) item = GetSelection();
    
    // check if event was generated by keyboard (MSW-specific?)
    if ( pt.x == -1 && pt.y == -1 ) { //(this is how MSW indicates it)
      // attempt to guess where to show the menu
      if ( item.IsOk() ) {
        // if an item was clicked, show menu to the right of it
        wxRect rect;
        p_treeCtrl->GetBoundingRect(item, rect, true);
        pt = wxPoint(rect.GetRight(), rect.GetTop());
      }
      else {
        pt = wxPoint(0, 0);
      }
    }
    else {                // event was generated by mouse, use supplied coords
      pt = ScreenToClient(pt);
    }
    wxMenu menu;
    getContextMenu(menu);
    p_popupOnTree = true;
    PopupMenu(&menu, pt);
    p_popupOnTree = false;
  }
  */
}


// Menu event handling functions
/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_MENU_HELP
 */
void CalcMgr::OnHelpClick( wxCommandEvent& event )
{
  WxHelpViewer::showKey("Organizer");
}


void CalcMgr::OnRunningCodesClick( wxCommandEvent& event )
{
  WxHelpViewer::showKey("RunningCodes");
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_MENU_SUPPORT
 */
void CalcMgr::OnMenuSupportClick( wxCommandEvent& event )
{
  BrowserHelp help;
  help.showFeedbackPage();
}


/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME
 */
void CalcMgr::OnCloseWindow( wxCloseEvent& event )
{
  //  Closing the window with the title-bar X must do what File > Quit
  //  does. It used to just Destroy(), which was survivable while the
  //  Gateway had a window of its own -- you still had something on
  //  screen and an obvious way out. With the Gateway hidden (#93) that
  //  left an invisible process still owning the session: the terminal
  //  never came back and the services kept running, with nothing left
  //  to stop them from. Reported live 2026-09-22.
  //
  //  Shares confirmAndQuit() with the menu path rather than duplicating
  //  the dialog, so the two cannot drift apart -- and vetoes the close
  //  if the user cancels, which a menu Quit has no need to do.
  if (!confirmAndQuit()) {
    event.Veto();
    return;
  }
  event.Skip();
}


/*!
 * Open a Builder with nothing loaded, i.e. start a new molecular
 * structure (issue #93).
 *
 * This is the one Gateway launcher icon with no other way in. The rest
 * are covered: Organizer is self-referential, Machine Browser, Machine
 * Registration and the Periodic Table are on the Tools menu, and Viewer
 * with nothing loaded is not useful because viewing happens from a
 * selected calculation, which already works.
 *
 * forceNew is 1 deliberately: without it an already-open Builder is
 * raised instead, so a second "New Structure" would silently do nothing
 * to the structure you are working on.
 */
void CalcMgr::OnNewStructureClick( wxCommandEvent& event )
{
  startApp("Builder", 1, "");
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 *
 * Issue #97: plain Quit only ever closed the GUI client. The background
 * services (JMS broker, data server) are deliberately left running -- they
 * persist across app launches so you do not pay startup cost every time --
 * but there was no way to also stop them from the Organizer, only from the
 * Gateway window, which #93 is removing. Offer it as a third button on the
 * confirm dialog, matching Gateway::exitGateway() exactly so the behaviour
 * does not depend on which window you quit from.
 */
/**
 * Ask, then quit. Returns false if the user cancelled, so the caller
 * handling a close EVENT can veto it -- a menu Quit has nothing to veto,
 * but the title-bar X does, and without that a cancelled quit closed the
 * window anyway.
 */
/**
 * Whether the Quit dialog offers Quit and Stop Server. inUse is set when
 * it is withheld only because another session or a job still uses the
 * services.
 */
static bool offerStopServer(bool& inUse)
{
  bool canStop = !getenv("ECCE_REMOTE_SERVER") && LocalData::dir().empty();
  inUse = canStop && !SessionLease::othersUsingServices().empty();
  return canStop && !inUse;
}


bool CalcMgr::confirmAndQuit()
{
  // A second close while the dialog is open runs the dialog's own event
  // loop again; the frame's deletion then ends in that nested loop and
  // destroys the first dialog, a stack object, as a child (#291). A close
  // after the answer has nothing left to ask.
  if (p_quitAsking) return false;
  if (p_quitConfirmed) return true;
  if (p_batch) {
    //  Quitting would leave calculations half submitted.
    ewxMessageDialog busy(this, "Launch Selected is still submitting "
        "calculations. Quit when it has finished, or press Stop in its window.",
        "Quit ECCE", wxOK | wxICON_INFORMATION, wxDefaultPosition);
    busy.ShowModal();
    return false;
  }

  // #190: under a central server (ECCE_REMOTE_SERVER) this client's own
  // ecce-dataserver-stop/ecce-gateway-stop can't reach the server's
  // services anyway (different account) -- offering the button is just
  // misleading, so don't.
  // Local data mode has no data server to stop; plain Quit already ends
  // the session and the reaper stops the per-user broker.
  // Another session of this account, or a job, still uses the services
  // (#233); the scripts refuse then too, this only says so up front.
  bool inUse = false;
  bool canStop = offerStopServer(inUse);
  string question = "Do you really want to quit?";
  if (inUse)
    question += "\n\nThe data server and message broker stay running: "
                "another ECCE session or a job of yours still uses them.";
  ewxMessageDialog dlg(this, question, "Quit ECCE",
                       wxOK|wxCANCEL|wxICON_QUESTION, wxDefaultPosition);
  dlg.SetButtonLabel(wxID_OK, "Quit");
  if (canStop)
    dlg.AddButton(ID_ORGANIZER_QUIT_STOP_SERVER, "Quit and Stop Server");
  p_quitAsking = true;
  int result = dlg.ShowModal();
  p_quitAsking = false;

  if (result == wxID_CANCEL)
    return false;

  p_quitConfirmed = true;
  Destroy();

  if (result == ID_ORGANIZER_QUIT_STOP_SERVER) {
    // After Destroy(), which schedules teardown asynchronously rather
    // than freeing `this`, so running more code here is safe -- and this
    // order lets our own JMS disconnect happen while the broker is still
    // up instead of racing it. Same sequencing as the Gateway's.
    //
    // DATA SERVER FIRST, gateway second. ecce-gateway-stop now also
    // stops the gateway application, which is the process the user's
    // `ecce` command is still waiting on -- so the shell prompt returns
    // the moment it dies. With the old order the data server's
    // "stopped data server (pid ...)" line arrived AFTER the prompt had
    // been printed, which reads as a terminal that never came back.
    // Reported live 2026-09-22: "stops the data server, but does not
    // release the terminal". It had in fact returned; the output simply
    // raced it.
    //
    // The session is over, so the shared data server credential goes
    // with it. ecce-gateway-stop removes the file too; doing it here as
    // well means it is gone even if that script is not on PATH or
    // resolves the session key differently than this process did.
    AuthCache::sessionClear();
    // A local-mode session started no data server; one running belongs to
    // a server-mode session elsewhere and is not ours to stop.
#ifndef _WIN32
    // On Windows "ecce-broker-win watch" (ecce.cmd) stops the session's
    // broker after the last window; these sh scripts cannot run in cmd.exe.
    if (LocalData::dir().empty()) (void)system("ecce-dataserver-stop --if-unused");
    (void)system("ecce-gateway-stop");
#endif
  }

  return true;
}


void CalcMgr::OnExitClick( wxCommandEvent& event )
{
  if (confirmAndQuit()) {
    event.Skip();
  }
}


/*!
 * Should save it off to pref
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_TOOLBAR
 */
void CalcMgr::OnShowToolbarClick( wxCommandEvent& event )
{

  p_toolBar->Show(event.IsChecked());
  p_topSizer->Layout();
}


/*!
 * Should save it off to pref
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LEGEND
 */
void CalcMgr::OnShowLegendClick( wxCommandEvent& event )
{
  startDisabler();

  p_legend->Show(event.IsChecked());
  p_topSizer->Layout();

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_INTERNAL
 */
void CalcMgr::OnShowInternalClick( wxCommandEvent& event )
{
  startDisabler();

  Resource::setHideInternal(!GetMenuBar()->IsChecked(wxID_SHOW_INTERNAL));
  p_treeCtrl->initialize(p_hlist,
                         GetMenuBar()->IsChecked(wxID_HOMEASROOT),
                         GetMenuBar()->IsChecked(wxID_SHOW_RUNSTATES),
                         p_treeSort);
  p_treeCtrl->mountEDSIProviders();
  p_treeCtrl->mountServerList(p_serverListEditor);

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_RUNSTATES
 */
void CalcMgr::OnShowRunstatesClick( wxCommandEvent& event )
{
  startDisabler();

  p_treeCtrl->initialize(p_hlist,
                         GetMenuBar()->IsChecked(wxID_HOMEASROOT),
                         GetMenuBar()->IsChecked(wxID_SHOW_RUNSTATES),
                         p_treeSort);
  p_treeCtrl->mountEDSIProviders();
  p_treeCtrl->mountServerList(p_serverListEditor);

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LINE
 */
void CalcMgr::OnShowLineClick( wxCommandEvent& event )
{
  startDisabler();

  setTreeStyle();

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SORT
 */
void CalcMgr::OnSortTreeClick( wxCommandEvent& event )
{
  startDisabler();

  p_treeSort = event.GetId() - wxID_SORT_TYPE;
  p_treeCtrl->setTreeSort(p_treeSort);

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_BOOKMARK
 */
void CalcMgr::OnAddBookmarkClick( wxCommandEvent& event )
{
  startDisabler();

  EcceURL url =  p_treeCtrl->getSelection()->getUrl();

  EDSIServerCentral servers;
  string name = servers.mapURLtoName(url);

  string urlstr = url.toString();

  p_bookmarkEditor->addBookmark(name, urlstr);

  updateBookmarkMenu();

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_MANAGE_BOOKMARK
 */
void CalcMgr::OnManageBookmarkClick( wxCommandEvent& event )
{
  p_bookmarkEditor->updateListBox();
  p_bookmarkEditor->ShowModal();

  updateBookmarkMenu();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_TEST_SUITE
 */
void CalcMgr::OnTestSuiteClick( wxCommandEvent& event )
{
  startDisabler();

  setMessage("Please be patient.  Generating test suites is a very "
             "time-intensive operation...", WxFeedback::INFO);

  getSelection();

  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    createTestSuite(findNode(*itor, false, true));
  }

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_VIEW_PROPERTY
 */
void CalcMgr::OnViewPropertyClick( wxCommandEvent& event )
{
  // ##REAL BEGIN
  getSelection();

  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    viewProperties(findNode((*itor), false, true));
  }
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ANNOTATION
 */
void CalcMgr::OnAnnotationClick( wxCommandEvent& event )
{
  startDisabler();

  startPropertyEdit("annotation");

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_SERVER
 */
void CalcMgr::OnAddServerClick( wxCommandEvent& event )
{
  startDisabler();

  p_serverListEditor->updateListBox();
  p_serverListEditor->ShowModal();

  p_treeCtrl->initialize(p_hlist,
                         GetMenuBar()->IsChecked(wxID_HOMEASROOT),
                         GetMenuBar()->IsChecked(wxID_SHOW_RUNSTATES),
                         p_treeSort);
  p_treeCtrl->mountEDSIProviders();
  p_treeCtrl->mountServerList(p_serverListEditor);

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOMEASROOT
 */
void CalcMgr::OnHomeasrootClick( wxCommandEvent& event )
{
  startDisabler();

  p_treeCtrl->initialize(p_hlist,
                         GetMenuBar()->IsChecked(wxID_HOMEASROOT),
                         GetMenuBar()->IsChecked(wxID_SHOW_RUNSTATES),
                         p_treeSort);
  p_treeCtrl->mountEDSIProviders();
  p_treeCtrl->mountServerList(p_serverListEditor);

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CHANGE_ACCESS
 */
void CalcMgr::OnChangeAccessClick( wxCommandEvent& event )
{
  if(p_accessControlDialog == (WxAccessControl*)0) {
    p_accessControlDialog = new WxAccessControl(this);
  }
  p_accessControlDialog->Show(true);
  updateAccessCtrlDlg();
  p_accessControlDialog->Raise();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CONFIGURE_NWFS_ARCHIVE
 */
void CalcMgr::OnConfigureNwfsArchiveClick( wxCommandEvent& event )
{
  if(p_nwfs == (WxConfigureNwfsArchive*)0) {
    p_nwfs = new WxConfigureNwfsArchive(this);
  }
  p_nwfs->Show(true);
  updateFindDlg();
  p_nwfs->Raise();
//  if(p_accessControlDialog == (WxAccessControl*)0) {
//    p_accessControlDialog = new WxAccessControl(this);
//  }
//  p_accessControlDialog->Show(true);
//  updateAccessCtrlDlg();
//  p_accessControlDialog->Raise();
}


/**
 * The item, or its nearest ancestor, whose type can contain resType (any
 * type when resType is 0); 0 if none. A calculation holds nothing, so New
 * with one selected means "next to it", in its project.
 */
WxResourceTreeItemData * CalcMgr::nearestContainer(
    WxResourceTreeItemData * node, ResourceType * resType)
{
  while (node) {
    if (node->getResource() && node->getResource()->getDescriptor()) {
      vector<ResourceType *> c =
          node->getResource()->getDescriptor()->getContains();
      if (resType == 0 && !c.empty()) return node;
      for (size_t i = 0; i < c.size(); i++) if (c[i] == resType) return node;
    }
    WxResourceTreeItemData * up = p_treeCtrl->getParent(node);
    if (up == node) break;
    node = up;
  }
  return 0;
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for all ID_RESOURCE
 */
void CalcMgr::OnNewResourceClick( wxCommandEvent& event )
{
  startDisabler();

  ResourceType * resType = ResourceDescriptor::getResourceDescriptor().
    getResourceType(event.GetId());
  // The nearest ancestor of the selection (or the selection itself) that
  // can contain this type, else the user's home.
  WxResourceTreeItemData * parent = nearestContainer(
      p_treeCtrl->getSelection(), resType);
  if (!parent) {
    EDSIServerCentral central;
    parent = findNode(central.getDefaultUserHome(), true, true);
  }
  createResource(resType, parent);

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for all ID_RMTASK
 */
void CalcMgr::OnRmTaskClick( wxCommandEvent& event )
{
  if (refuseDuringBatch("Run Mgmt actions")) return;
  startDisabler();

  ResourceRmTask * rmTask = ResourceDescriptor::getResourceDescriptor().
    getRmTask(event.GetId());
  
  if        (rmTask->getName() == TAILOUTPUTFILE) {
    tailOutputFile();
  } else if (rmTask->getName() == SHELLINRUNDIR) {
    shellInRunDir();
  } else if (rmTask->getName() == MANAGEFILES) {
    manageFiles();
  } else if (rmTask->getName() == TERMINATECALC) {
    terminateCalc();
  } else if (rmTask->getName() == CHECKJOB) {
    checkJob();
  } else if (rmTask->getName() == CHECKJOBONMACHINE) {
    checkJobOnMachine();
  } else if (rmTask->getName() == RECONNECTJOB) {
    reconnectJob();
  } else if (rmTask->getName() == FORCEFAIL) {
    forceFail();
  } else if (rmTask->getName() == RESETFORRERUN) {
    resetForRerun();
  } else if (rmTask->getName() == RESETFORRESTART) {
    resetForRestart();
  } else if (rmTask->getName() == RESETCOMPLETED) {
    resetCompleted();
  } else if (rmTask->getName() == RESETINCOMPLETE) {
    resetIncomplete();
  } else if (rmTask->getName() == RESETIMPORTED) {
    resetImported();
  } else if (rmTask->getName() == RESETKILLED) {
    resetKilled();
  } else if (rmTask->getName() == MARKREVIEWED) {
    markReviewed();
  } else if (rmTask->getName() == MARKUNREVIEWED) {
    markUnreviewed();
  } else if (rmTask->getName() == CLEANRUNDIR) {
    cleanRunDir();
  } else if (rmTask->getName() == VIEWRUNLOG) {
    viewRunLog();
  } else if (rmTask->getName() == VIEWOUTPUTFILE) {
    viewOutputFile();
  } else if (rmTask->getName() == VIEWINPUTFILE) {
    viewInputFile();
  }

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for all ID_TOOL
 */
void CalcMgr::OnToolClick( wxCommandEvent& event )
{
  startDisabler();

  ResourceTool * tool = ResourceDescriptor::getResourceDescriptor().
    getTool(event.GetId());
  //  cerr << "Clicked " << tool->getMenuItem() << endl;

  int forceNew = 0;
  InvokeParam * param = dynamic_cast<InvokeParam *>(event.GetClientObject());
  if (param)
    forceNew = param->forceNew;

  WxResourceTreeItemData * itemData;
  if (getSelection()) {
    itemData = findNode(p_currentSelection[0], false, true);
  }
  else {
    // @todo Do we allow tools invoke in multi selection mode?
  }
  startApp(tool->getName(), forceNew, itemData->getUrl().toString());

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for all ID_HISTORY
 */
void CalcMgr::OnHistoryClick( wxCommandEvent& event )
{
  startDisabler();

  int pos = event.GetId()-ID_HISTORY_BASE;

  findNode(p_hlist->jumpTo(pos), true, true);

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for all ID_BOOKMARK
 */
void CalcMgr::OnBookmarkClick( wxCommandEvent& event )
{
  startDisabler();

  int pos = event.GetId()-ID_BOOKMARK_BASE;

  if (!findNode(p_bookmarkEditor->getBookmarkUrl(pos), true, true)) {

    string msg = "The following URL no longer exists:\n" +
      p_bookmarkEditor->getBookmarkUrl(pos) +
      "\nDo you want to remove it from the bookmark list?";
    
    ewxMessageDialog dlg(this, msg, "Delete Bookmark?",
                         wxOK|wxCANCEL|wxICON_QUESTION, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
      p_bookmarkEditor->removeBookmark(pos);
      updateBookmarkMenu();
    }
  }

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for all ID_PROJECTVIEW
 */
void CalcMgr::OnProjectViewClick( wxCommandEvent& event )
{
  startDisabler();

  p_projectViewId = event.GetId();
  GetMenuBar()->Check(p_projectViewId, true);
  p_contextPanel->refresh();

  stopDisabler();
}



// Toolbar event handling functions

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOME
 */
void CalcMgr::OnHomeClick( wxCommandEvent& event )
{
  startDisabler();

  p_treeCtrl->openHome();

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UP
 */
void CalcMgr::OnUpClick( wxCommandEvent& event )
{
  startDisabler();

  p_treeCtrl->selectParent();

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_BACKWARD
 */
void CalcMgr::OnBackwardClick( wxCommandEvent& event )
{
  startDisabler();

  findNode(p_hlist->back(), true, true);

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FORWARD
 */
void CalcMgr::OnForwardClick( wxCommandEvent& event )
{
  startDisabler();

  EcceURL url = p_hlist->forward();
  findNode(url, true, true);

  stopDisabler();
}


//  Holds the frame's redraws until a change is complete, then paints all
//  its children at once (the MSW thaw only invalidates).  MSW only: GTK
//  paints once per idle anyway, and GTK3 paints a frozen widget black when
//  the frame is drawn before the thaw (#523).
struct FrameHold {
  wxWindow *win;
#ifdef __WXMSW__
  explicit FrameHold(wxWindow *w) : win(w) { win->Freeze(); }
  ~FrameHold() {
    win->Thaw();
    ::RedrawWindow((HWND)win->GetHWND(), NULL, NULL,
                   RDW_UPDATENOW | RDW_ALLCHILDREN);
  }
#else
  explicit FrameHold(wxWindow *w) : win(w) {}
#endif
};


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE
 */
void CalcMgr::OnDuplicateClick( wxCommandEvent& event )
{
  startDisabler();

  //  The copy is added in place and selected; on MSW every step in between
  //  (an item added, the selection moved, the panels refilled) was painted.
  FrameHold hold(this);

  // get selection
  if (getSelection()) {
    // single selection
    WxResourceTreeItemData * sel = findNode(p_currentSelection[0], false, true);
    if (dynamic_cast<TaskJob*>(sel->getResource())) {
      // it is a calculation, so duplicate it and select it in tree
      WxResourceTreeItemData *dup = 0;
      dup = duplicate(sel, true);
      if (dup) {
        // duplicate was successful, now reset for rerun
        TaskJob *task = dynamic_cast<TaskJob*>(dup->getResource());
        if (task) {
          task->resetForRerun();
          // some props changed, so we need to update the panel.  However, the 
          // state also changed so we also need to update the menubar's dynamic 
          // menus.
          // 08/07/06 Lisong: Since CalcMgr is listening to itself for pool
          //                  state change messages, wonder if this refresh
          //                  is still necessary or not.
          onSelectionChange(true);
        }
      }
    } else {
      p_feedback->clearMessage();
      setMessage("Duplicate is only applicable to calculations.");
    }
  }

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE2
 */
void CalcMgr::OnDuplicate2Click( wxCommandEvent& event )
{
  startDisabler();

  //  The copy is added in place and selected; on MSW every step in between
  //  (an item added, the selection moved, the panels refilled) was painted.
  FrameHold hold(this);

  // get selection
  if (getSelection()) {
    // single selection
    WxResourceTreeItemData * sel = findNode(p_currentSelection[0], false, true);
    if (dynamic_cast<TaskJob*>(sel->getResource())) {
      // it is a calculation, so duplicate it and select it in tree
      WxResourceTreeItemData *dup = 0;
      dup = duplicate(sel, true);
      if (dup) {
        // duplicate was successful, now reset with last geom
        resetWithLastGeom(dup);
        // some props changed, so we need to update the panel.  However, the 
        // state also changed so we also need to update the menubar's dynamic 
        // menus.
        // 08/07/06 Lisong: Since CalcMgr is listening to itself for pool state
        //                  change messages, wonder if this refresh is still
        //                  necessary or not.
        onSelectionChange(true);
      }
    } else {
      p_feedback->clearMessage();
      setMessage("Duplicate is only applicable to calculations.");
    }
  }

  stopDisabler();
}


/*!
 * Run Mgmt > Launch Selected: the calculations selected in the tree or the
 * project table, each with the launch settings it already stores.
 */
void CalcMgr::OnLaunchSelectedClick( wxCommandEvent& event )
{
  if (refuseDuringBatch("Launch Selected")) return;
  launchSelected(launchSelection(), true);
}


// The menu can fire while the focus flag still points at the tree, which
// getSelection() then answers with the one tree item; several selected table
// rows are what the user means by "selected".
vector<EcceURL> CalcMgr::launchSelection()
{
  if (p_contextPanel) {
    vector<EcceURL> rows = p_contextPanel->getSelections();
    if (rows.size() > 1) return rows;
  }
  getSelection();
  return p_currentSelection;
}


void CalcMgr::launchSelected(const vector<EcceURL>& urls, bool interactive,
                             string *outcome, const string& shot,
                             const LaunchBatch::Target& initial)
{
  vector<TaskJob*> tasks;
  for (size_t i = 0; i < urls.size(); i++) {
    WxResourceTreeItemData *item = findNode(urls[i], true, true);
    TaskJob *task = item ? dynamic_cast<TaskJob*>(item->getResource()) : 0;
    if (task) tasks.push_back(task);
  }
  if (tasks.empty()) {
    if (outcome) *outcome = "no calculations";
    return;
  }
  LaunchBatch::Target target = initial;
  vector<LaunchSelected::Row> rows = LaunchSelected::plan(tasks, target);
  bool go = true;
  if (interactive || !shot.empty()) {
    go = LaunchSelected::confirm(this, rows, target,
        [this](const string& url) { startApp("Launcher", 0, url); }, shot);
  }
  if (!go) {
    if (outcome) *outcome = "not launched";
    return;
  }
  if (interactive) {
    startBatch(rows, true);
    return;
  }
  LaunchSelected::run(rows);
  if (outcome) {
    string r = LaunchSelected::report(rows);
    for (size_t i = 0; i < r.size(); i++) if (r[i] == '\n') r[i] = '|';
    *outcome = "ok " + r;
  }
}


bool CalcMgr::refuseDuringBatch(const string& what)
{
  if (!p_batch) return false;
  setMessage(what + " is not available while Launch Selected is submitting.");
  return true;
}


// Timer-driven, so the Organizer keeps handling events during the batch (#640).
void CalcMgr::startBatch(const vector<LaunchSelected::Row>& rows, bool showReport)
{
  p_batchReport.clear();
  p_batch = new LaunchSelected::Batch(this, rows,
    [this, showReport](const vector<LaunchSelected::Row>& done) {
      p_batchReport = LaunchSelected::report(done);
      //  The batch object is deleted from the event loop, not from its own tick.
      CallAfter([this, done, showReport]() {
        delete p_batch;
        p_batch = 0;
        if (showReport) LaunchSelected::showResults(this, done);
      });
    });
  p_batch->start();
}


bool CalcMgr::launchSelectedAsync(const vector<EcceURL>& urls, string *outcome)
{
  if (p_batch) { if (outcome) *outcome = "busy"; return false; }
  vector<TaskJob*> tasks;
  for (size_t i = 0; i < urls.size(); i++) {
    WxResourceTreeItemData *item = findNode(urls[i], true, true);
    TaskJob *task = item ? dynamic_cast<TaskJob*>(item->getResource()) : 0;
    if (task) tasks.push_back(task);
  }
  if (tasks.empty()) { if (outcome) *outcome = "no calculations"; return false; }
  vector<LaunchSelected::Row> rows = LaunchSelected::plan(tasks);
  startBatch(rows, false);
  if (outcome) *outcome = "started";
  return true;
}


string CalcMgr::launchBatchStatus() const
{
  if (p_batch)
    return "running " + std::to_string(p_batch->number()) + "/" +
           std::to_string(p_batch->total());
  string r = p_batchReport;
  for (size_t i = 0; i < r.size(); i++) if (r[i] == '\n') r[i] = '|';
  return "idle " + r;
}


/*!
 * Run Mgmt > Reconnect Selected: Reconnect for each selected calculation that
 * can have it. Same selection as Launch Selected.
 */
void CalcMgr::OnReconnectSelectedClick( wxCommandEvent& event )
{
  if (refuseDuringBatch("Reconnect Selected")) return;
  reconnectSelected(launchSelection(), true);
}


void CalcMgr::OnReparseOutputsClick( wxCommandEvent& event )
{
  reparseOutputs(launchSelection(), true);
}


void CalcMgr::reparseOutputs(const vector<EcceURL>& urls, bool interactive,
                             string *outcome)
{
  //  The progress dialog appears only when the copying of the stored
  //  outputs has taken a moment; a single small calculation never shows it.
  const auto t0 = std::chrono::steady_clock::now();
  wxProgressDialog *pd = 0;
  string report;
  int ok = 0;
  for (size_t i = 0; i < urls.size(); i++) {
    WxResourceTreeItemData *item = findNode(urls[i], true, true);
    TaskJob *task = item ? dynamic_cast<TaskJob*>(item->getResource()) : 0;
    if (!task) continue;
    if (interactive && urls.size() > 1 && !pd &&
        std::chrono::steady_clock::now() - t0 > std::chrono::seconds(1))
      pd = new wxProgressDialog("Re-Parse Outputs", "Starting...",
                                (int)urls.size(), this,
                                wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_SMOOTH);
    if (pd && !pd->Update((int)i, wxString::FromUTF8(("Re-parsing " + task->getName()).c_str())))
      break;
    string message;
    const bool done = JobCatchUp::reparse(task, message);
    if (done) {
      ok++;
      p_reparsing[task->getURL().toString()] = task->getName();
    }
    report += task->getName() + (done ? ": started: " : ": skipped: ") +
              message + "|";
  }
  if (pd) { pd->Update((int)urls.size()); delete pd; }
  if (ok)
    setMessage("Re-parsing " + std::to_string(p_reparsing.size()) +
               (p_reparsing.size() == 1 ? " calculation..." : " calculations..."),
               WxFeedback::INFO);
  else if (interactive)
    setMessage("No calculation was re-parsed", WxFeedback::ERROR);
  if (outcome) *outcome = "ok " + report;
}


void CalcMgr::reparseFinished(const EcceURL& url, const string& result)
{
  auto it = p_reparsing.find(url.toString());
  if (it == p_reparsing.end()) return;
  if (result == "done") p_reparseDone++;
  else p_reparseFailed.push_back(it->second);
  p_reparsing.erase(it);
  if (!p_reparsing.empty()) return;

  string msg = "Re-parse finished: " + std::to_string(p_reparseDone) + " done";
  if (!p_reparseFailed.empty()) {
    msg += ", " + std::to_string(p_reparseFailed.size()) + " failed (";
    for (size_t i = 0; i < p_reparseFailed.size(); i++)
      msg += (i ? ", " : "") + p_reparseFailed[i];
    msg += "; see their run logs)";
  }
  setMessage(msg, p_reparseFailed.empty() ? WxFeedback::INFO
                                          : WxFeedback::WARNING);
  if (getenv("ECCE_TEST_ORGANIZER"))
    fprintf(stderr, "REPARSE: %s\n", msg.c_str());
  p_reparseDone = 0;
  p_reparseFailed.clear();
}


void CalcMgr::reconnectSelected(const vector<EcceURL>& urls, bool interactive,
                                string *outcome, const string& shot, bool listOnly)
{
  vector<TaskJob*> tasks;
  for (size_t i = 0; i < urls.size(); i++) {
    WxResourceTreeItemData *item = findNode(urls[i], true, true);
    TaskJob *task = item ? dynamic_cast<TaskJob*>(item->getResource()) : 0;
    if (task) tasks.push_back(task);
  }
  if (tasks.empty()) {
    if (outcome) *outcome = "no calculations";
    return;
  }
  vector<ReconnectSelected::Row> rows = ReconnectSelected::plan(tasks);
  if (listOnly) {
    if (outcome) {
      string r = ReconnectSelected::listing(rows);
      for (size_t i = 0; i < r.size(); i++) if (r[i] == '\n') r[i] = ';';
      *outcome = "ok " + r;
    }
    return;
  }
  bool go = true;
  if (interactive || !shot.empty())
    go = ReconnectSelected::confirm(this, rows, shot);
  if (!go) {
    if (outcome) *outcome = "not reconnected";
    return;
  }
  // One at a time, rows of a machine together: the first login makes the
  // connection (and the per-host lock) that the later jobs and their job
  // stores reuse, so one machine asks once.
  ReconnectSelected::run(this, rows, interactive,
      [this](ReconnectSelected::Row& r) {
        EcceURL url(r.plan.url);
        if (r.task->getState() != ResourceDescriptor::STATE_WAITING) {
          JMSMessage *msg = newMessage();
          msg->addProperty("url", url.toString());
          publish("ecce_ejs_kill", *msg);
          delete msg;
          if (!stopOldMonitor(url)) {
            r.outcome = "previous monitor still running";
            return false;
          }
        }
        bool ok = JobCatchUp::reconnect(r.task, r.outcome);
        if (ok) {
          p_contact.erase(progressKey(url));
          p_machineCheck.erase(progressKey(url));
        }
        updateUrl(url);
        return ok;
      });
  if (interactive) ReconnectSelected::showResults(this, rows);
  if (outcome) {
    string r = ReconnectSelected::report(rows);
    for (size_t i = 0; i < r.size(); i++) if (r[i] == '\n') r[i] = '|';
    *outcome = "ok " + r;
  }
}


// Asked from a launch step's worker thread: the window opens on this one.
bool CalcMgr::getAuthorization(AuthEvent& event)
{
  if (wxThread::IsMain())
    return WxDavAuth::getAuthorization(event);
  std::promise<bool> answer;
  wxTheApp->CallAfter([this, &event, &answer]() {
    answer.set_value(WxDavAuth::getAuthorization(event));
  });
  return answer.get_future().get();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE3
 *
 * Like Duplicate Setup with Last Geometry, and the duplicate also carries
 * the source's final orbitals, which belong to that geometry.
 */
void CalcMgr::OnDuplicate3Click( wxCommandEvent& event )
{
  startDisabler();
  FrameHold hold(this);

  if (getSelection()) {
    WxResourceTreeItemData * sel = findNode(p_currentSelection[0], false, true);
    TaskJob *source = dynamic_cast<TaskJob*>(sel->getResource());
    string why;
    if (source == 0) {
      p_feedback->clearMessage();
      setMessage("Duplicate is only applicable to calculations.");
    } else if (!OrbitalGuess::available(source, why)) {
      p_feedback->clearMessage();
      setMessage(why, WxFeedback::WARNING);
    } else {
      WxResourceTreeItemData *dup = duplicate(sel, true);
      if (dup) {
        resetWithLastGeom(dup);
        TaskJob *task = dynamic_cast<TaskJob*>(dup->getResource());
        if (task) {
          //  A deck copied from the source has no MORead in it: have the
          //  Calculation Editor generate it again.
          if (task->getState() != ResourceDescriptor::STATE_CREATED)
            task->resetForRerun(ResourceDescriptor::STATE_CREATED);
          if (!OrbitalGuess::attach(task, source, why)) {
            p_feedback->clearMessage();
            setMessage("The duplicate was made without the orbitals. " + why,
                       WxFeedback::WARNING);
          }
        }
        onSelectionChange(true);
      }
    }
  }

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CUT
 *
 * The cut implementation will only support paste once after cut.
 * The reason for doing this is that the clipboard will only save
 * the reference to the real objects and once the original objects
 * are deleted after first paste, it is not easy to update the
 * clipboard content to do another paste. Windows file explorer
 * will paste only once after each cut. Also files will not be deleted
 * instantly after cut unless paste is clicked.
 * @todo Need to think about the representation of the nodes after
 * they were cut. Windows will dime out the icon and font, what do
 * we do?
 */
void CalcMgr::OnCutClick( wxCommandEvent& event )
{
  if (refuseDuringBatch("Cut")) return;
  if (selectionHasSystemFolder()) return;
  startDisabler();

  copySelectionToClipboard(true);

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_COPY
 */
void CalcMgr::OnCopyClick( wxCommandEvent& event )
{
  startDisabler();

  copySelectionToClipboard(false);

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PASTE
 *
 * Copy or move over the files that are in clipboard.
 * The destination is the current selection in the treectrl and
 * it will ignore the selection in the right hand side panel.
 */
void CalcMgr::OnPasteClick( wxCommandEvent& event )
{
  if (refuseDuringBatch("Paste")) return;
  startDisabler();

  paste();

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DELETE
 *
 * To get the current selection is pretty tricky, it is possible that user
 * select some node in the treectrl, it is also possible that user select
 * some node from the table view on the right hand side panel. Should have
 * a general util to return the selection.
 * Need to think about multi-selection problem too.
 */
void CalcMgr::OnDeleteClick( wxCommandEvent& event )
{
  if (refuseDuringBatch("Delete")) return;
  if (selectionHasSystemFolder()) return;
  startDisabler();

  deleteSelection();

  stopDisabler();
}


/**
 * Start rename the currently selected file or folder
 *
 * Ignore multi-selection in panel
 */
void CalcMgr::OnRenameClick( wxCommandEvent& event )
{
  if (selectionHasSystemFolder()) return;
  startDisabler();

  // It needs to be a single selection
  if (getSelection()) {
    EcceURL oldUrl = p_currentSelection[0];
    bool selectFromTree = (oldUrl == p_treeCtrl->getSelection()->getUrl());
    WxResourceTreeItemData * node = findNode(oldUrl, false);
    Resource * res = node->getResource();
    ResourceType * resType = res->getDescriptor();
    string newName;

    // special STTR reaction rate study logic to prevent renaming the
    // reactant/product/transition-state calculations because that corrupts
    // the reaction study. getResource() can legitimately return null
    // (same class of gap fixed elsewhere in run management this
    // session) -- guard rather than crash renaming over inapplicable
    // reaction-study logic.
    Resource *parent = EDSIFactory::getResource(res->getURL().getParent());
    if (parent != 0 &&
        parent->getApplicationType() == ResourceDescriptor::AT_REACTION_STUDY) {
      string name = res->getName();
      if (name=="Transition-State" ||
          name=="Reactant1" || name=="Reactant2" ||
          name=="Product1" || name=="Product2") {
        string msg = "Can not rename reaction study calculation \"" +
                     name + "\".";
        ewxMessageDialog dlg(this, msg, "Rename Not Allowed!",
                             wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
        dlg.ShowModal();

        stopDisabler();
        return; 
      }
    }

    // Prompt for new legal name
    bool showInvalidMsg = false;
    string invalidMsg = "A name may contain only letters, digits, "
      "'.', '_' and '-'.\n\n";
    while (true) {
      newName = oldUrl.getFilePathTail();
      string infoStr = "Please enter the new name for " +
        resType->getLabel() + ":";
      if (showInvalidMsg)
        infoStr = invalidMsg + infoStr;
      ewxTextEntryDialog dialog(this, infoStr, "Rename " + resType->getLabel(),
                                newName, wxOK | wxCANCEL);
      dialog.spacesToUnderscores();
      if (dialog.ShowModal() == wxID_OK) {
        newName = dialog.GetValue();
        // Quit the loop if it is a legal name
        if (resType->isValidResourceName(newName))
          break;
        else
          showInvalidMsg = true;
      }
      else {
        // Cancel clicked, return without doing anything
        stopDisabler();
        return; 
      }
    }

    // If the new name is different from old name, make the move..
    if (newName != oldUrl.getFilePathTail()) {
      EcceURL newUrl = oldUrl.getParent().getChild(newName);
      if (res->move(newUrl)) {
        // Refresh the moved node
        p_treeCtrl->refresh(node);

        WxResourceTreeItemData * parent = findNode(oldUrl.getParent(),
                                                   false, true);
        p_treeCtrl->SortChildren(parent->GetId());

        setContextPanel();
        if (!selectFromTree) {
          p_contextPanel->selectResource(newUrl);
        }
        else {
          EDSIServerCentral servers;
          p_feedback->setContextLabel(servers.mapURLtoName(newUrl));
          p_hlist->pop();
          p_hlist->add(newUrl.toString());
        }

        // tell others about it
        notifyRename(oldUrl.toString(),
                     newUrl.toString(),
                     resType->getContentType());
      } else {
        ewxMessageDialog dlg(this, res->messages(),
                             "Rename Failed!", wxOK|wxICON_EXCLAMATION, 
                             wxDefaultPosition);
        dlg.ShowModal();
      }
    }
  }

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FIND
 */
void CalcMgr::OnFindClick( wxCommandEvent& event )
{
  if(p_find == (WxFind*)0) {
    p_find = new WxFind(this);
  }
  p_find->Show(true);
  updateFindDlg();
  p_find->Raise();
}


/**
 * Edit > Preferences.  Built on first use; closing only hides it, and the
 * frame is owned by this one.
 */
void CalcMgr::OnPreferencesClick( wxCommandEvent& event )
{
  if (p_prefs == (GlobalPrefs*)0) {
    p_prefs = new GlobalPrefs(this);
  }
  p_prefs->Show(true);
  p_prefs->Raise();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_REFRESH
 */
void CalcMgr::OnRefreshClick( wxCommandEvent& event )
{
  startDisabler();

  refresh();

  stopDisabler();
}


/*!
 * Prompt user for a file to upload.
 * Multiple selection in this case will pick one file to upload, and place it
 * in all of the selected places in the tree.
 */
void CalcMgr::OnUploadClick( wxCommandEvent& event )
{
  startDisabler();

  // get last location as preference
  Preferences prefs("Organizer");

  string dir, file;
  prefs.getString("Organizer.UploadDir", dir);
  prefs.getString("Organizer.UploadFile", file);

  if (dir != "") {
    SFile dirfile(dir);
    if (!dirfile.exists()) dir = Ecce::realUserHome();
  } else {
    dir = Ecce::realUserHome();
  }
  if (file != "") {
    SFile fileFile(dir + "/" + file);
    if (!fileFile.exists()) file = "";
  }

  ewxFileDialog *dlg = new ewxFileDialog(this,
                                         "Select file to upload", dir, 
                                         file,
                                         "All Files|*",
                                         wxFD_OPEN, wxDefaultPosition);
  dlg->SetFilterIndex(1);
  if (dlg->ShowModal() == wxID_OK) {
    // access the file to upload through an SFile
    string path = dlg->GetPath().ToStdString();
    SFile *file = new SFile(path);

    // Upload to selected folder in the tree
    EcceURL url = upload(p_treeCtrl->getSelection(), file);

    if (url != EcceURL())
      updateAddNode(url);

    // clean up the SFile pointer before going out of scope
    delete file;
  }

  // save the last directory and filename to calcmgr preferences
  string directory = dlg->GetDirectory().ToStdString();
  prefs.setString("Organizer.UploadDir", directory);
  string filename = dlg->GetFilename().ToStdString();
  prefs.setString("Organizer.UploadFile", filename.c_str());
  prefs.saveFile();

  dlg->Destroy();

  stopDisabler();
}


/*!
 * Prompt user for a file to download.
 * If multiple selections, user is prompted each time for where they'd like to
 * put the file currently being downloaded.
 * @todo do we allow the recursive download of children?
 * @todo does multiple selection make sense?
 */
void CalcMgr::OnDownloadClick( wxCommandEvent& event )
{
  startDisabler();

  // get tree item data
  getSelection();
  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    download(findNode(*itor, false, true));
  }

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_IMPORT
 */
void CalcMgr::OnImportClick( wxCommandEvent& event )
{
  startDisabler();

  WxCalcImport *dlg = new WxCalcImport(this);
  dlg->registerListener(this);
  dlg->importCalc();

  stopDisabler();
}


// The study goes into the project enclosing the selection, named after
// the directory (project names allow no spaces).
string CalcMgr::importGromacsFrom(const string& runDir, string& message)
{
  WxResourceTreeItemData *node = p_treeCtrl->getSelection();
  while (node && node->getResource() &&
         (!node->getResource()->getDescriptor() ||
          node->getResource()->getDescriptor()->getName() != "project")) {
    WxResourceTreeItemData *up = p_treeCtrl->getParent(node);
    node = (up == node) ? 0 : up;
  }
  if (!node || !node->getResource()) {
    message = "Select a project to import the GROMACS run into.";
    return "";
  }
  string name = runDir;
  while (name.size() > 1 && name[name.size() - 1] == '/')
    name.erase(name.size() - 1);
  name = name.substr(name.find_last_of('/') + 1);
  for (size_t i = 0; i < name.size(); i++)
    if (!isalnum((unsigned char)name[i]) && name[i] != '_' && name[i] != '-')
      name[i] = '_';
  if (name.empty()) name = "GROMACS_Import";
  // Asked of the data server: the tree's cached children may not list a
  // study imported a moment ago.
  EDSI *edsi = EDSIFactory::getEDSI(node->getResource()->getURL());
  name = edsi->uniqueName(name, "_%d");
  delete edsi;
  string made = importGromacsRun(node->getResource(), name, runDir, message);
  p_treeCtrl->refresh(node);
  return made;
}


void CalcMgr::OnImportGromacsClick( wxCommandEvent& event )
{
  wxDirDialog dlg(this, _("Choose the directory of the GROMACS run"));
  if (dlg.ShowModal() != wxID_OK) return;
  startDisabler();
  string message;
  string made = importGromacsFrom(string(dlg.GetPath().mb_str()), message);
  if (made.empty() || !message.empty())
    setMessage(message, WxFeedback::ERROR);
  else
    setMessage("The GROMACS run is imported; its properties appear shortly.",
               WxFeedback::INFO);
  stopDisabler();
}


/**
 * The shipped example calculations are program outputs (data/client/examples/
 * <set>/[<topic>/]<example>), imported as any output is.  Each set goes into
 * its own project below the project "Examples", its topics into projects
 * below that: Examples/<Set>/<Topic>/<calculation>.  Asking again selects
 * the set, so a user's own copy is never overwritten.  The set project's
 * "ecce:exampleSet" metadata says which version of the example sets it was
 * made from.
 *
 * What is shipped is data: a set is a folder with a set.txt (line 1 the
 * name, line 2 one line of description, then one "folder|Topic name" line
 * per topic); an example is a folder in a topic folder (or directly in the
 * set, then without a topic) holding one output and its stored/ copy.
 * Project names allow no spaces, so spaces become underscores in the tree.
 */
namespace {
struct ExampleOut {
  string rel;      // output (or GROMACS run folder), relative to the set folder
  string dirRel;   // the example's folder, relative to the set folder
  string name;     // the example's folder name, e.g. "orca-benzene"
  string calc;     // the calculation's name once imported, e.g. "benzene"
  bool run;        // a GROMACS run folder
};
struct ExampleTopic {
  string name;                 // "" = directly in the set's project
  vector<ExampleOut> outputs;
};
struct ExampleSet {
  string id, title, line, project;
  vector<ExampleTopic> topics;
  size_t size() const {
    size_t n = 0;
    for (const ExampleTopic &t : topics) n += t.outputs.size();
    return n;
  }
};
const char *const kExamplesTop = "Examples";
const char *const kExampleSetVersion = "14";

string treeName(string s)
{
  std::replace(s.begin(), s.end(), ' ', '_');
  return s;
}

string examplesDir()
{
  return string(Ecce::ecceHome()) + "/data/client/examples/";
}

void collectExamples(const string &setDir, const string &topicDir,
                     ExampleTopic &topic)
{
  namespace fs = std::filesystem;
  vector<string> names;
  std::error_code ec;
  for (const auto &e : fs::directory_iterator(
           setDir + (topicDir.empty() ? "" : topicDir + "/"), ec))
    if (e.is_directory()) names.push_back(e.path().filename().string());
  std::sort(names.begin(), names.end());
  for (const string &n : names) {
    const string dirRel = topicDir.empty() ? n : topicDir + "/" + n;
    ExampleOut o;
    o.dirRel = dirRel;
    o.name = n;
    o.run = n.compare(0, 8, "gromacs-") == 0;
    if (o.run) {
      o.rel = dirRel;
      o.calc = n.substr(8);
    } else {
      // the output is the first file with one of these extensions
      // (tools/examples/preparse.py does the same)
      string found;
      for (const char *ext : {".qmout", ".eprint", ".out"}) {
        vector<string> files;
        for (const auto &f : fs::directory_iterator(setDir + dirRel, ec))
          if (f.is_regular_file() && f.path().extension() == ext)
            files.push_back(f.path().filename().string());
        std::sort(files.begin(), files.end());
        if (!files.empty()) { found = files[0]; break; }
      }
      if (found.empty()) continue;
      o.rel = dirRel + "/" + found;
      o.calc = found.substr(0, found.rfind('.'));
    }
    topic.outputs.push_back(o);
  }
}

const vector<ExampleSet>& exampleSets()
{
  static vector<ExampleSet> sets;
  static bool loaded = false;
  if (loaded) return sets;
  namespace fs = std::filesystem;
  vector<string> ids;
  std::error_code ec;
  for (const auto &e : fs::directory_iterator(examplesDir(), ec))
    if (e.is_directory() && fs::exists(e.path() / "set.txt"))
      ids.push_back(e.path().filename().string());
  std::sort(ids.begin(), ids.end());
  for (const string &id : ids) {
    ExampleSet set;
    set.id = id;
    const string dir = examplesDir() + id + "/";
    std::ifstream in(dir + "set.txt");
    if (!std::getline(in, set.title) || set.title.empty()) continue;
    std::getline(in, set.line);
    set.project = treeName(set.title);
    vector<std::pair<string, string>> topics;   // folder, name
    string l;
    while (std::getline(in, l)) {
      size_t bar = l.find('|');
      if (bar != string::npos && l[0] != '#')
        topics.push_back({l.substr(0, bar), l.substr(bar + 1)});
    }
    ExampleTopic top;
    collectExamples(dir, "", top);
    // folders of the set that are topics are not examples
    top.outputs.erase(std::remove_if(top.outputs.begin(), top.outputs.end(),
        [&](const ExampleOut &o) {
          for (const auto &t : topics) if (t.first == o.name) return true;
          return false;
        }), top.outputs.end());
    if (!top.outputs.empty()) set.topics.push_back(top);
    for (const auto &t : topics) {
      ExampleTopic topic;
      topic.name = t.second;
      collectExamples(dir, t.first, topic);
      if (!topic.outputs.empty()) set.topics.push_back(topic);
    }
    sets.push_back(set);
  }
  loaded = true;
  return sets;
}

// The flat projects earlier versions made, and the set each one became.
const struct { const char *project, *setId; } kOldExampleProjects[] = {
  {"Examples", "getting-started"},
  {"Examples_Computational", "computational"},
  {"Examples_Organic", "organic"},
  {"Examples_Spectroscopy", "spectroscopy"}};

bool exampleSetText(const ExampleSet& set, string& title, string& line)
{
  title = set.title;
  line = set.line;
  return true;
}
}

// The dialog behind File > Open Example Calculations: the sets in a
// drop-down, the chosen one's description beneath it.  *chosen is the
// index into exampleSets() after OK.
static wxDialog *makeExamplesDialog(wxWindow *parent, int *chosen)
{
  wxDialog *dlg = new wxDialog(parent, wxID_ANY, _("Open Example Calculations"));
  vector<wxString> names, lines;
  vector<int> index;
  const vector<ExampleSet>& sets = exampleSets();
  for (size_t i = 0; i < sets.size(); i++) {
    string title, line;
    if (!exampleSetText(sets[i], title, line)) continue;
    names.push_back(wxString::FromUTF8(title.c_str()));
    lines.push_back(wxString::FromUTF8(line.c_str()));
    index.push_back((int)i);
  }
  wxChoice *choice = new wxChoice(dlg, wxID_ANY);
  for (const wxString& n : names) choice->Append(n);
  if (!names.empty()) choice->SetSelection(0);
  wxStaticText *text = new wxStaticText(dlg, wxID_ANY,
      names.empty() ? wxString() : lines[0]);
  text->Wrap(dlg->FromDIP(380));
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  top->Add(new wxStaticText(dlg, wxID_ANY, _("Example set:")), 0,
           wxLEFT | wxRIGHT | wxTOP, 12);
  top->Add(choice, 0, wxEXPAND | wxALL, 12);
  top->Add(text, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
  top->Add(dlg->CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
  dlg->SetSizerAndFit(top);
  //  Capped: on macOS the best size kept the unwrapped line after a new
  //  description, and the dialog grew as wide as the screen.
  dlg->SetMinSize(wxSize(dlg->FromDIP(420), -1));
  dlg->SetMaxSize(wxSize(dlg->FromDIP(460), -1));
  dlg->Fit();
  choice->Bind(wxEVT_CHOICE, [=](wxCommandEvent&) {
    int sel = choice->GetSelection();
    if (sel >= 0 && sel < (int)lines.size()) {
      text->SetLabel(lines[sel]);
      text->Wrap(dlg->FromDIP(380));
      text->InvalidateBestSize();
      dlg->Layout();
      dlg->Fit();
    }
  });
  dlg->Bind(wxEVT_BUTTON, [=](wxCommandEvent& ev) {
    int sel = choice->GetSelection();
    if (chosen) *chosen = (sel >= 0 && sel < (int)index.size()) ? index[sel] : -1;
    ev.Skip();
  }, wxID_OK);
  return dlg;
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_OPENEXAMPLES
 */
void CalcMgr::OnOpenExamplesClick( wxCommandEvent& event )
{
  int chosen = -1;
  wxDialog *dlg = makeExamplesDialog(this, &chosen);
  const int rc = dlg->ShowModal();
  dlg->Destroy();
  if (rc != wxID_OK || chosen < 0) return;
  startDisabler();
  string message;
  bool created = false;
  if (openExampleSet(exampleSets()[chosen].id, message, created).empty())
    setMessage(message, WxFeedback::ERROR);
  else
    setMessage(message, WxFeedback::INFO);
  stopDisabler();
}

/**
 * Earlier versions imported each set as a flat project (Examples,
 * Examples_Organic, ...).  A project that carries the example-set marker
 * of an older version loses the calculations whose names are shipped
 * examples; anything else in it is the user's and stays.  A project left
 * empty is removed.  Returns the sets whose project was found, which the
 * first start after an upgrade imports again in the new layout.
 */
vector<string> CalcMgr::removeOldExamples()
{
  vector<string> found;
  EDSIServerCentral central;
  EcceURL home = central.getDefaultUserHome();
  const string key = VDoc::getEcceNamespace() + ":exampleSet";
  std::set<string> shipped;
  for (const ExampleSet &set : exampleSets())
    for (const ExampleTopic &t : set.topics)
      for (const ExampleOut &o : t.outputs) shipped.insert(o.calc);
  bool changed = false;
  for (const auto &old : kOldExampleProjects) {
    Resource *res = EDSIFactory::getResource(home.getChild(old.project));
    if (!res) continue;
    const string version = res->getProp(key);
    if (version.empty() || version == kExampleSetVersion) continue;
    found.push_back(old.setId);
    //  The tree's items point at these resources: drop them first, or the
    //  refresh below follows them into freed memory (a crash at the first
    //  start after an upgrade from 9.2/9.3).
    WxResourceTreeItemData *node = findNode(res->getURL(), false, false);
    if (node) p_treeCtrl->DeleteChildren(node->GetId());
    size_t left = 0;
    vector<Resource *> children;
    if (vector<Resource *> *c = res->getChildren(true)) children = *c;
    for (Resource *child : children) {
      if (child && shipped.count(child->getName()) && child->getDescriptor() &&
          child->getDescriptor()->getName() != "project") {
        child->remove();
        changed = true;
      } else {
        left++;
      }
    }
    if (left == 0) {
      if (node) p_treeCtrl->Delete(node->GetId());
      res->remove();
    } else {
      // The user's own calculations stay; the project is theirs now.
      vector<MetaDataResult> mark(1);
      mark[0].name = key;
      mark[0].value = kExampleSetVersion;
      res->addProps(mark);
    }
    changed = true;
  }
  if (changed)
    if (WxResourceTreeItemData *homeNode = findNode(home, false, false))
      p_treeCtrl->refresh(homeNode);
  return found;
}


string CalcMgr::openExampleSet(const string& setId, string& message,
                               bool& created)
{
  created = false;
  const ExampleSet *set = nullptr;
  for (const ExampleSet& s : exampleSets())
    if (setId == s.id) set = &s;
  if (!set) {
    message = "There is no example set " + setId + ".";
    return "";
  }
  const string dir = examplesDir() + set->id + "/";
  EDSIServerCentral central;
  EcceURL home = central.getDefaultUserHome();
  Resource *homeRes = EDSIFactory::getResource(home);
  if (!homeRes) {
    message = "Cannot reach your home folder for the examples.";
    return "";
  }
  for (const ExampleTopic &t : set->topics) {
    for (const ExampleOut &o : t.outputs) {
      if (!SFile(dir + o.rel).exists()) {
        message = "The example calculations are not installed (" + dir +
                  o.rel + " is missing).";
        return "";
      }
    }
  }

  // Flat projects of an earlier version must not sit beside or inside the
  // new ones.
  removeOldExamples();
  ResourceType *projectType =
      ResourceDescriptor::getResourceDescriptor().getResourceType(
          "collection", "ecceProject", "");
  Resource *topRes = EDSIFactory::getResource(home.getChild(kExamplesTop));
  if (!topRes) {
    topRes = homeRes->createChild(kExamplesTop, projectType);
    if (WxResourceTreeItemData *homeNode = findNode(home, false, false))
      p_treeCtrl->refresh(homeNode);
  }
  if (!topRes) {
    message = string("Cannot create the project ") + kExamplesTop + ".";
    return "";
  }
  const EcceURL topUrl = topRes->getURL();
  const string name = string(kExamplesTop) + "/" + set->project;
  // An existing project is topped up: only topics and calculations that
  // are missing are imported, nothing the user has is touched.
  Resource *proj = EDSIFactory::getResource(topUrl.getChild(set->project));
  const bool topUp = proj != nullptr;
  if (!proj) {
    created = true;
    proj = topRes->createChild(set->project, projectType);
  }
  if (!proj) {
    message = "Cannot create the project " + name + ".";
    return "";
  }
  vector<MetaDataResult> mark(1);
  mark[0].name = VDoc::getEcceNamespace() + ":exampleSet";
  mark[0].value = kExampleSetVersion;
  proj->addProps(mark);
  auto childOf = [](Resource *parent, const string &child) -> Resource * {
    return parent ? EDSIFactory::getResource(parent->getURL().getChild(child))
                  : nullptr;
  };
  std::map<const ExampleTopic *, vector<const ExampleOut *>> missing;
  int nExamples = 0;
  for (const ExampleTopic &topic : set->topics) {
    Resource *folder = topic.name.empty() ? proj
                                          : childOf(proj, treeName(topic.name));
    for (const ExampleOut &ex : topic.outputs)
      if (!childOf(folder, ex.calc)) {
        missing[&topic].push_back(&ex);
        nExamples++;
      }
  }
  if (topUp && nExamples == 0) {
    findNode(topUrl.getChild(set->project), true, true);
    setContextPanel();
    message = "The example calculations are in the project " + name + ".";
    return name;
  }

  EcceURL project = proj->getURL();
  if (WxResourceTreeItemData *topNode = findNode(topUrl, false, false))
    p_treeCtrl->refresh(topNode);

  // The topic's project is selected while its calculations are imported
  // (getContainer files each calculation under the selection's project);
  // the tree is rebuilt once afterwards, not per calculation.
  p_batchImport = true;
  std::set<string> added;           // calculations imported now
  vector<string> addedTopics;
  const auto t0 = std::chrono::steady_clock::now();
  double gmxSeconds = 0;
  {
    WxCalcImport dlg(this);
    dlg.registerListener(this);
    // Shown at once: the import is always slow, and the window was
    // otherwise silent for its whole length.
    ewxProgressDialog progress(_("Example calculations"),
        _("Importing example calculations"), nExamples, this,
        wxPD_APP_MODAL | wxPD_SMOOTH);
    int step = 0;
    for (const ExampleTopic &topic : set->topics) {
    auto todo = missing.find(&topic);
    if (todo == missing.end()) continue;
    Resource *parent = proj;
    if (!topic.name.empty()) {
      parent = childOf(proj, treeName(topic.name));
      if (!parent) {
        parent = proj->createChild(treeName(topic.name), projectType);
        if (parent) addedTopics.push_back(topic.name);
      }
      if (!parent) {
        p_batchError = "Cannot create the topic " + topic.name + ".";
        continue;
      }
      if (WxResourceTreeItemData *n = findNode(project, false, false))
        p_treeCtrl->refresh(n);
    }
    findNode(parent->getURL(), true, true);
    for (const ExampleOut *exp : todo->second) {
      const ExampleOut &ex = *exp;
      added.insert(ex.calc);
      const char *rel = ex.rel.c_str();
      // "mopac-ammonia" is shown as "MOPAC ammonia"
      string label;
      {
        static const struct { const char *key, *shown; } kCodes[] = {
          {"ecceqm", "ECCE-QM"}, {"mopac", "MOPAC"}, {"nwchem", "NWChem"},
          {"orca", "ORCA"}, {"gromacs", "GROMACS"}};
        size_t dash = ex.name.find('-');
        size_t from = dash == string::npos ? 0 : dash + 1;
        string code = ex.name.substr(0, dash);
        string what = ex.name.substr(from);
        for (const auto &c : kCodes)
          if (code == c.key) code = c.shown;
        label = code + " " + what;
      }
      step++;
      // Update() also pumps events: the window keeps painting and answering.
      auto show = [&](const string &detail) {
        string text = string("Importing example calculations: ") +
            std::to_string(step) + " of " + std::to_string(nExamples) + " - " +
            label + detail;
        progress.Update(step - 1, wxString::FromUTF8(text.c_str()));
        cerr << "Organizer: example progress: " << step << " of "
             << nExamples << " - " << label << detail << endl;
      };
      show("");
      // A directory is a GROMACS run (GromacsImport.H), a study named
      // after the example.
      if (ex.run) {
        string why;
        const auto g0 = std::chrono::steady_clock::now();
        if (importGromacsRun(parent, ex.calc, dir + rel, why,
                             [&](const string &task) {
                               show(" (" + task + ")");
                             }).empty() || !why.empty())
          p_batchError = why;
        gmxSeconds += std::chrono::duration<double>(
            std::chrono::steady_clock::now() - g0).count();
      } else {
        // A copy of what the parsers made of this very output when the
        // package was built is much faster than parsing it again; the
        // parse stays as the fallback.
        const string stored = dir + ex.dirRel + "/stored";
        const string calcName = StoredCalcCopy::calcName(stored);
        string why;
        const auto c0 = std::chrono::steady_clock::now();
        if (!getenv("ECCE_NO_PREPARSED") && !calcName.empty() &&
            StoredCalcCopy::isCurrent(stored, dir + rel) &&
            StoredCalcCopy::copy(parent, stored + "/" + calcName, calcName,
                                 why)) {
          cerr << "Organizer: example copied: " << calcName << " in "
               << std::chrono::duration<double>(
                      std::chrono::steady_clock::now() - c0).count()
               << " s" << endl;
        } else {
          if (!why.empty())
            cerr << "Organizer: example copy failed (" << why
                 << "), parsing" << endl;
          dlg.importFile(dir + rel);
        }
      }
    }
    }
    // Not the maximum: wxProgressDialog then waits for a click on Close.
    progress.Update(nExamples - 1, _("Reading results..."));
  }
  p_batchImport = false;
  cerr << "Organizer: examples " << set->id << " imported in "
       << std::chrono::duration<double>(
              std::chrono::steady_clock::now() - t0).count()
       << " s, of which the GROMACS run " << gmxSeconds << " s" << endl;

  // What each example is for, in its Annotation field.
  static const struct { const char *name, *text; } kNotes[] = {
    {"water", "ECCE-QM, B3LYP/6-31G*. This calculation shows how a frequency "
              "calculation gives the IR spectrum of a molecule, and how the "
              "computed atomic charges depend on the partitioning scheme "
              "(Mulliken, Löwdin, CHELPG, Merz-Kollman)."},
    {"ethanol", "ECCE-QM, B3LYP/6-31G*. This calculation shows the IR and "
                "Raman spectra of the same molecule, whose intensities follow "
                "different selection rules: a change in dipole moment for IR, "
                "a change in polarisability for Raman."},
    {"benzene", "ORCA, B3LYP/6-31G*. This calculation shows the delocalised "
                "pi orbitals of an aromatic ring: six pi orbitals in a "
                "1-2-2-1 pattern of levels with an increasing number of nodal "
                "planes."},
    {"ammonia", "MOPAC, PM7. This calculation shows a semiempirical "
                "optimisation and frequency calculation, including the "
                "umbrella inversion mode, for comparison with the DFT "
                "examples in cost and accuracy."},
    {"methane", "NWChem, B3LYP/6-31G*. This calculation shows the effect of "
                "tetrahedral symmetry: degenerate orbitals (a1, t2) and "
                "degenerate vibrational modes."},
    // organic chemistry and spectroscopy sets
    {"ethane",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals localise"
     "d. This calculation shows a carbon-carbon single bond: in the MOs pane"
     "l, field type Localised MO, one sigma C1-C2 orbital. The optimised C-C"
     " distance is 1.531 Å and the totally symmetric C-C stretch is a"
     "t 1009.19 cm-1 (IR inactive); ethene and ethyne use the same method fo"
     "r comparison."},
    {"ethene",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals localise"
     "d. This calculation shows a carbon-carbon double bond: in the MOs pane"
     "l, field type Localised MO, one sigma C1-C2 and one pi C1-C2 orbital. "
     "The optimised C-C distance is 1.332 Å, shorter than in ethane, "
     "and the totally symmetric C-C stretch is at 1719.02 cm-1 (IR inactive)"
     ", higher than in ethane."},
    {"ethyne",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals localise"
     "d. This calculation shows a carbon-carbon triple bond: in the MOs pane"
     "l, field type Localised MO, one sigma C1-C2 and two pi C1-C2 orbitals."
     " The optimised C-C distance is 1.205 Å, shorter than in ethene,"
     " and the totally symmetric C-C stretch is at 2085.58 cm-1 (IR inactive"
     "), higher than in ethene."},
    {"ethane_scan",
     "ECCE-QM, B3LYP/6-31G*, relaxed scan of the H-C-C-H torsion, -180 to 180"
     " degrees in 25 points. This calculation shows the internal rotation of "
     "ethane: in the Relaxed Scan panel the energy is lowest, 0.0 kJ/mol, for"
     " the staggered geometries at -180, -60, 60 and 180 degrees and highest,"
     " 11.8 kJ/mol, for the eclipsed ones at -120, 0 and 120 degrees, so the "
     "rotation barrier is 11.8 kJ/mol (electronic energies, no zero-point cor"
     "rection)."},
    {"butane_scan",
     "ECCE-QM, B3LYP/6-31G*, relaxed scan of the C-C-C-C torsion, -180 to 180"
     " degrees in 25 points. This calculation shows the conformers of butane:"
     " in the Relaxed Scan panel the anti conformer at +-180 degrees is the m"
     "inimum, 0.0 kJ/mol; the gauche conformers at +-60 degrees lie 3.7 and 3"
     ".8 kJ/mol above it, and the syn (eclipsed) geometry at 0 degrees is 23."
     "8 kJ/mol above it, the top of the barrier between the two gauche confor"
     "mers (electronic energies, no zero-point correction)."},
    {"nma_scan",
     "ECCE-QM, B3LYP/6-31G*, relaxed scan of the O=C-N-C torsion of N-methyla"
     "cetamide, 0 to 360 degrees in 25 points. This calculation shows the ro"
     "tation barrier of the amide bond, which has partial double-bond charact"
     "er: the planar trans amide at 0 and 360 degrees is the minimum, the cis"
     " amide at 180 degrees is 10.6 kJ/mol above it, and the energy rises ste"
     "eply on twisting. The curve jumps from 92.0 kJ/mol at 120 degrees to 20"
     ".3 kJ/mol at 135 degrees, and again from 76.2 kJ/mol at 300 degrees to "
     "20.4 kJ/mol at 315 degrees: a relaxed scan follows one continuous path,"
     " and the nitrogen stays pyramidal on the higher-energy branch until it "
     "flips. Compare nma_scan_rev, which runs the other way (hysteresis); the"
     " rotation barrier is below both curves."},
    {"nma_scan_rev",
     "ECCE-QM, B3LYP/6-31G*, relaxed scan of the O=C-N-C torsion of N-methyla"
     "cetamide, 360 to 0 degrees in 25 points. This calculation shows that th"
     "e relaxed scan of nma_scan depends on its direction (hysteresis): run b"
     "ackwards, the energy rises to 92.0 kJ/mol at 240 degrees and drops to 2"
     "0.3 kJ/mol at 225 degrees, the mirror image of the forward scan, becaus"
     "e the nitrogen stays pyramidal on the higher-energy branch until it fli"
     "ps. The lower envelope of the two scans peaks near 58 kJ/mol at 90 and "
     "270 degrees, and the true rotation barrier is below both curves."},
    {"butadiene",
     "ORCA, B3LYP-D3BJ/def2-SVP. Optimisation and frequencies. This calculat"
     "ion shows the diene of the Diels-Alder reaction in its s-gauche confor"
     "mation, the reactant geometry used here, with an electronic energy of "
     "-155.776738 Eh. With ethylene it is the reactant side of the reaction "
     "energy profile."},
    {"ethylene",
     "ORCA, B3LYP-D3BJ/def2-SVP. Optimisation and frequencies. This calculat"
     "ion shows the dienophile of the Diels-Alder reaction, with an electron"
     "ic energy of -78.477034 Eh. Together with s-gauche butadiene (-155.776"
     "738 Eh) it gives the reactant energy -234.253772 Eh."},
    {"cyclohexene",
     "ORCA, B3LYP-D3BJ/def2-SVP. Optimisation and frequencies. This calculat"
     "ion shows the product of the Diels-Alder reaction, with an electronic "
     "energy of -234.335601 Eh. Relative to butadiene plus ethylene this is "
     "-214.8 kJ/mol (electronic energies, no zero-point correction), a compu"
     "ted value only."},
    {"diels_alder_ts",
     "ORCA, B3LYP-D3BJ/def2-SVP. Transition-state optimisation (OptTS) and f"
     "requencies. This calculation shows that the Diels-Alder reaction goes "
     "through one transition state with one imaginary mode, -516.33 cm-1, an"
     "d two equal forming C...C distances of 2.271 Å, so the two bond"
     "s form together. Its electronic energy, -234.233996 Eh, is 51.9 kJ/mol"
     " above butadiene plus ethylene (computed, no zero-point correction)."},
    {"diels_alder_irc",
     "ORCA, B3LYP-D3BJ/def2-SVP. Intrinsic reaction coordinate (IRC) from th"
     "e transition state. This calculation shows the energy along the reacti"
     "on path on both sides of the transition state (45 points). The reactan"
     "t-side branch stopped at the iteration limit at -234.249081 Eh, above "
     "the optimised reactants (-234.253772 Eh); the product-side branch ende"
     "d at -234.326271 Eh, above cyclohexene (-234.335601 Eh)."},
    {"chloride",
     "ORCA, B3LYP-D3BJ/ma-def2-SVP. Single-point energy of Cl-. This calcula"
     "tion shows the reactant ion of the SN2 reaction, with an electronic en"
     "ergy of -460.082857 Eh, used for the energies relative to the reactant"
     " complex."},
    {"methyl_bromide",
     "ORCA, B3LYP-D3BJ/ma-def2-SVP. Optimisation and frequencies. This calcu"
     "lation shows the substrate of the SN2 reaction, with an electronic ene"
     "rgy of -2613.589298 Eh. Cl- plus CH3Br lies 49.7 kJ/mol above the reac"
     "tant complex (computed)."},
    {"reactant_complex",
     "ORCA, B3LYP-D3BJ/ma-def2-SVP. Optimisation and frequencies of Cl-...CH"
     "3Br. This calculation shows the ion-dipole complex on the reactant sid"
     "e of the SN2 profile; it is the zero of energy (-3073.691084 Eh) for t"
     "he energies in this set."},
    {"sn2_ts",
     "ORCA, B3LYP-D3BJ/ma-def2-SVP. Transition-state optimisation (OptTS) an"
     "d frequencies. This calculation shows the SN2 transition state, with o"
     "ne imaginary mode, -341.25 cm-1, a linear Cl-C-Br arrangement (180.0 d"
     "egrees), a planar CH3 group and C-Cl and C-Br distances of 2.420 and 2"
     ".473 Å. Its energy is 20.8 kJ/mol above the reactant complex an"
     "d 28.9 kJ/mol below Cl- plus CH3Br (computed electronic energies, no z"
     "ero-point correction)."},
    {"sn2_irc",
     "ORCA, B3LYP-D3BJ/ma-def2-SVP. Intrinsic reaction coordinate (IRC) from"
     " the transition state. This calculation shows the path from the transi"
     "tion state down to the ion-dipole complexes on both sides (both branch"
     "es converged); the end points, -3073.691304 Eh and -3073.699544 Eh, ar"
     "e close to the optimised complexes, -3073.691084 Eh and -3073.699428 E"
     "h."},
    {"product_complex",
     "ORCA, B3LYP-D3BJ/ma-def2-SVP. Optimisation and frequencies of ClCH3..."
     "Br-. This calculation shows the ion-dipole complex on the product side"
     " of the SN2 profile, -21.9 kJ/mol relative to the reactant complex (co"
     "mputed, no zero-point correction), so the profile has one well on each"
     " side of the transition state."},
    {"bromide",
     "ORCA, B3LYP-D3BJ/ma-def2-SVP. Single-point energy of Br-. This calcula"
     "tion shows the leaving ion of the SN2 reaction, with an electronic ene"
     "rgy of -2573.820390 Eh; Br- plus CH3Cl lies 20.1 kJ/mol above the reac"
     "tant complex (computed)."},
    {"methyl_chloride",
     "ORCA, B3LYP-D3BJ/ma-def2-SVP. Optimisation and frequencies. This calcu"
     "lation shows the product of the SN2 reaction, with an electronic energ"
     "y of -499.863026 Eh."},
    {"sulfate",
     "ORCA, PBE0-D3BJ/def2-SVP with CPCM(water), frequencies with IR and "
     "Raman. This calculation shows the S–O stretches of the free "
     "sulfate ion, the reference for the sulfate complexes in this set."},
    {"co_nh3_5_oso3",
     "ORCA, PBE0-D3BJ/def2-SVP with CPCM(water), frequencies with IR and "
     "Raman. This calculation shows the S–O stretches of a sulfate bound "
     "to cobalt through one oxygen in [Co(NH3)5(OSO3)]+."},
    {"co_en2_cl_oso3",
     "ORCA, PBE0-D3BJ/def2-SVP with CPCM(water), optimisation, frequencies "
     "with IR and Raman. This calculation shows the S–O stretches of a "
     "sulfate bound through one oxygen in cis-[Co(en)2Cl(OSO3)], with the "
     "same Co(en)2 framework as the chelating and bridging complexes, so "
     "the sulfate bands can be compared directly. Chloride is the sixth "
     "ligand because it has no internal vibrations and its Co–Cl stretch "
     "lies near 300 cm⁻¹, below the sulfate and Co–O–S region."},
    {"co_en2_o2so2",
     "ORCA, PBE0-D3BJ/def2-SVP with CPCM(water), frequencies with IR and "
     "Raman. This calculation shows the S–O stretches of a sulfate "
     "chelating cobalt through two oxygens in cis-[Co(en)2(O2SO2)]+."},
    {"co2_en4_oh_so4_bridged",
     "ORCA, PBE0-D3BJ/def2-SVP with CPCM(water), frequencies with IR and "
     "Raman. This calculation shows the S–O stretches of a sulfate "
     "bridging two cobalt centres in [(en)2Co(μ-OH)(μ-SO4)Co(en)2]3+."},
    {"formaldehyde",
     "ORCA, PBE0-D3BJ/def2-SVP, frequencies with IR and Raman. This "
     "calculation shows the C=O stretch of the simplest carbonyl "
     "compound, the start of the carbonyl series."},
    {"acetaldehyde",
     "ORCA, PBE0-D3BJ/def2-SVP, frequencies with IR and Raman. This "
     "calculation shows how one methyl group changes the C=O stretch of "
     "an aldehyde."},
    {"acetone",
     "ORCA, PBE0-D3BJ/def2-SVP, frequencies with IR and Raman. This "
     "calculation shows the C=O stretch of a ketone with two methyl "
     "groups."},
    {"acetic_acid",
     "ORCA, PBE0-D3BJ/def2-SVP, frequencies with IR and Raman. This "
     "calculation shows the C=O stretch of a carboxylic acid."},
    {"acetate_anion",
     "ORCA, PBE0-D3BJ/def2-SVP with CPCM(water), frequencies with IR and "
     "Raman. This calculation shows that the carboxylate anion has two "
     "coupled CO2 stretches instead of one C=O stretch."},
    {"methyl_acetate",
     "ORCA, PBE0-D3BJ/def2-SVP, frequencies with IR and Raman. This "
     "calculation shows the C=O stretch of an ester."},
    {"ethyl_acetate",
     "ORCA, PBE0-D3BJ/def2-SVP, frequencies with IR and Raman. This "
     "calculation shows the C=O stretch of an ester with a longer alkoxy "
     "group."},
    {"methyl_propionate",
     "ORCA, PBE0-D3BJ/def2-SVP, frequencies with IR and Raman. This "
     "calculation shows the C=O stretch of an ester with a longer acyl "
     "group."},
    {"n_methylacetamide",
     "ORCA, PBE0-D3BJ/def2-SVP, frequencies with IR and Raman. This "
     "calculation shows the amide I and amide II bands of the peptide "
     "linkage in trans-N-methylacetamide."},
    {"ac_ala_nhme",
     "ORCA, PBE0-D3BJ/def2-SVP, frequencies with IR and Raman. This "
     "calculation shows the amide bands of a small peptide, Ac-Ala-NHMe "
     "in the C7eq conformation, with a hydrogen bond between its two "
     "amide groups."},
    {"alpha_d_glucopyranose",
     "ORCA, PBE0-D3BJ/def2-SVP, frequencies with IR and Raman. This "
     "calculation shows the O–H, C–H and C–O vibrations of a sugar in "
     "the 4C1 chair."},
    {"alpha_d_galactopyranose",
     "ORCA, PBE0-D3BJ/def2-SVP, frequencies with IR and Raman. This "
     "calculation shows how the axial C4 hydroxyl group of galactose "
     "changes the vibrational spectrum compared with glucose."},
    {"H",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the single"
     " electron of the hydrogen atom in the 1s orbital."},
    {"He",
     "ECCE-QM, B3LYP/aug-cc-pVTZ, energy. This calculation shows the two "
     "electrons of helium paired in the 1s orbital."},
    {"Li",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the single"
     " 2s electron of lithium outside its filled 1s orbital."},
    {"Be",
     "ECCE-QM, B3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "occupied orbitals of the beryllium atom, filled in order of increasing"
     " energy."},
    {"B",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "occupied orbitals of the boron atom, filled in order of increasing "
     "energy."},
    {"C",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the two 2p"
     " electrons of carbon in different p orbitals with parallel spins."},
    {"N",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the three "
     "2p electrons of nitrogen in different p orbitals with parallel spins."},
    {"O",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the four "
     "2p electrons of oxygen, two of them paired and two unpaired."},
    {"F",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "occupied orbitals of the fluorine atom, filled in order of increasing "
     "energy."},
    {"Ne",
     "ECCE-QM, B3LYP/aug-cc-pVTZ, energy. This calculation shows the full "
     "second shell of neon, with all its orbitals doubly occupied."},
    {"Na",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the single"
     " 3s electron of sodium outside its filled neon-like core."},
    {"Mg",
     "ECCE-QM, B3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "occupied orbitals of the magnesium atom, filled in order of increasing"
     " energy."},
    {"Al",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "occupied orbitals of the aluminium atom, filled in order of increasing"
     " energy."},
    {"Si",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "occupied orbitals of the silicon atom, filled in order of increasing "
     "energy."},
    {"P",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "occupied orbitals of the phosphorus atom, filled in order of "
     "increasing energy."},
    {"S",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "occupied orbitals of the sulfur atom, filled in order of increasing "
     "energy."},
    {"Cl",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "occupied orbitals of the chlorine atom, filled in order of increasing "
     "energy."},
    {"Ar",
     "ECCE-QM, B3LYP/aug-cc-pVTZ, energy. This calculation shows the full "
     "third shell of argon, with all its orbitals doubly occupied."},
    {"Li_HF_neutral",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the "
     "lithium atom; with the cation, Tools > "
     "Compare Energies gives the ionisation energy as the difference of "
     "their energies."},
    {"Li_HF_cation",
     "ECCE-QM, HF/aug-cc-pVTZ, energy. This calculation shows the "
     "lithium cation, the atom after loss of its outermost electron; its "
     "energy minus that of the neutral atom is "
     "the ionisation energy."},
    {"Be_HF_neutral",
     "ECCE-QM, HF/aug-cc-pVTZ, energy. This calculation shows the "
     "beryllium atom; with the cation, Tools > "
     "Compare Energies gives the ionisation energy as the difference of "
     "their energies."},
    {"Be_HF_cation",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the "
     "beryllium cation, the atom after loss of its outermost electron; "
     "its energy minus that of the neutral atom "
     "is the ionisation energy."},
    {"C_HF_neutral",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the "
     "carbon atom; with the cation, Tools > "
     "Compare Energies gives the ionisation energy as the difference of "
     "their energies."},
    {"C_HF_cation",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the "
     "carbon cation, the atom after loss of its outermost electron; its "
     "energy minus that of the neutral atom is "
     "the ionisation energy."},
    {"N_HF_neutral",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the "
     "nitrogen atom; with the cation, Tools > "
     "Compare Energies gives the ionisation energy as the difference of "
     "their energies."},
    {"N_HF_cation",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the "
     "nitrogen cation, the atom after loss of its outermost electron; its "
     "energy minus that of the neutral atom is "
     "the ionisation energy."},
    {"O_HF_neutral",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the "
     "oxygen atom; with the cation, Tools > "
     "Compare Energies gives the ionisation energy as the difference of "
     "their energies."},
    {"O_HF_cation",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the "
     "oxygen cation, the atom after loss of its outermost electron; its "
     "energy minus that of the neutral atom is "
     "the ionisation energy."},
    {"F_HF_neutral",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the "
     "fluorine atom; with the cation, Tools > "
     "Compare Energies gives the ionisation energy as the difference of "
     "their energies."},
    {"F_HF_cation",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the "
     "fluorine cation, the atom after loss of its outermost electron; its "
     "energy minus that of the neutral atom is "
     "the ionisation energy."},
    {"Ne_HF_neutral",
     "ECCE-QM, HF/aug-cc-pVTZ, energy. This calculation shows the neon "
     "atom; with the cation, Tools > Compare "
     "Energies gives the ionisation energy as the difference of their "
     "energies."},
    {"Ne_HF_cation",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the neon "
     "cation, the atom after loss of its outermost electron; its "
     "energy minus that of the neutral atom is "
     "the ionisation energy."},
    {"Li_B3LYP_neutral",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "lithium atom; with the anion, Tools > Compare Energies gives the "
     "electron affinity as the difference of their energies."},
    {"Li_anion",
     "ECCE-QM, B3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "lithium anion, the atom with one extra electron; the energy of the "
     "neutral atom minus the energy of this anion is the electron "
     "affinity."},
    {"Be_B3LYP_neutral",
     "ECCE-QM, B3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "beryllium atom; with the anion, Tools > Compare Energies gives the "
     "electron affinity as the difference of their energies."},
    {"Be_anion",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "beryllium anion, the atom with one extra electron; the energy of "
     "the neutral atom minus the energy of this anion is the electron "
     "affinity."},
    {"C_B3LYP_neutral",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "carbon atom; with the anion, Tools > Compare Energies gives the "
     "electron affinity as the difference of their energies."},
    {"N_B3LYP_neutral",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "nitrogen atom; with the anion, Tools > Compare Energies gives the "
     "electron affinity as the difference of their energies."},
    {"O_B3LYP_neutral",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "oxygen atom; with the anion, Tools > Compare Energies gives the "
     "electron affinity as the difference of their energies."},
    {"F_B3LYP_neutral",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "fluorine atom; with the anion, Tools > Compare Energies gives the "
     "electron affinity as the difference of their energies."},
    {"C_anion",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "carbon anion, the atom with one extra electron; the energy of the "
     "neutral atom minus the energy of this anion is the electron "
     "affinity."},
    {"N_anion",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "nitrogen anion, the atom with one extra electron; the energy of "
     "the neutral atom minus the energy of this anion is the electron "
     "affinity."},
    {"O_anion",
     "ECCE-QM, UB3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "oxygen anion, the atom with one extra electron; the energy of the "
     "neutral atom minus the energy of this anion is the electron "
     "affinity."},
    {"F_anion",
     "ECCE-QM, B3LYP/aug-cc-pVTZ, energy. This calculation shows the "
     "fluorine anion, the atom with one extra electron; the energy of "
     "the neutral atom minus the energy of this anion is the electron "
     "affinity."},
    {"H_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the hydrogen atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"He_HF",
     "ECCE-QM, HF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the helium atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"Li_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the lithium atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"Be_HF",
     "ECCE-QM, HF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the beryllium atom in Hartree-Fock theory, where minus the"
     " energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"B_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the boron atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"C_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the carbon atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"N_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the nitrogen atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"O_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the oxygen atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"F_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the fluorine atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"Ne_HF",
     "ECCE-QM, HF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the neon atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"Na_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the sodium atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"Mg_HF",
     "ECCE-QM, HF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the magnesium atom in Hartree-Fock theory, where minus the"
     " energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"Al_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the aluminium atom in Hartree-Fock theory, where minus the"
     " energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"Si_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the silicon atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"P_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the phosphorus atom in Hartree-Fock theory, where minus "
     "the energy of the highest occupied orbital approximates the ionisation"
     " energy (Koopmans' theorem)."},
    {"S_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the sulfur atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"Cl_HF",
     "ECCE-QM, UHF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the chlorine atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"Ar_HF",
     "ECCE-QM, HF/aug-cc-pVTZ, energy. This calculation shows the orbital "
     "energies of the argon atom in Hartree-Fock theory, where minus the "
     "energy of the highest occupied orbital approximates the ionisation "
     "energy (Koopmans' theorem)."},
    {"Ne_HF_IVO",
     "ECCE-QM, HF/aug-cc-pVTZ, energy, improved virtual orbitals. This "
     "calculation shows the empty orbitals of neon computed as improved "
     "virtual orbitals, whose lowest members are the 3s and 3p orbitals."},
    {"CO",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the orbitals of CO, which has the "
     "same number of valence electrons as N2 and a triple bond between "
     "carbon and oxygen."},
    {"F2",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the single bond of F2, since the pi "
     "and pi* orbitals are both full."},
    {"H2",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the two electrons of H2 in one "
     "bonding sigma orbital, with the empty antibonding sigma* orbital above"
     " it."},
    {"H2_cation",
     "ECCE-QM, UB3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows H2+, held together by a single "
     "electron in the bonding sigma orbital."},
    {"HF",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals localised. "
     "This calculation shows the bonding sigma orbital of HF, built from the "
     "hydrogen 1s and a fluorine 2p orbital, and the unused fluorine 2p "
     "orbitals as lone pairs."},
    {"HF_dipole",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals localised. "
     "This calculation shows how the bonding electrons of HF are pulled "
     "towards the more electronegative fluorine atom, so the bond is polar."},
    {"He2",
     "ECCE-QM, B3LYP/6-31G*, energy, orbitals localised. This calculation "
     "shows He2 at a large distance, where the bonding and the antibonding "
     "orbital are both full, so there is no bond."},
    {"He2_cation",
     "ECCE-QM, UB3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows He2+, with two electrons in the "
     "bonding and one in the antibonding orbital, which makes a bond of half"
     " the strength of the one in H2."},
    {"Li2",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the single sigma bond of Li2, built "
     "from the 2s orbitals of the two lithium atoms."},
    {"LiF",
     "ECCE-QM, B3LYP/6-31G*, geometry optimisation, orbitals localised. "
     "This calculation shows the very polar bond of LiF, whose bonding "
     "orbital lies almost entirely on fluorine."},
    {"LiH",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the bonding orbital of LiH, built "
     "from the lithium 2s and hydrogen 1s orbitals, with the bonding "
     "electrons shifted towards hydrogen."},
    {"N2",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the triple bond of N2, with one "
     "sigma and two pi bonding orbitals filled."},
    {"O2",
     "ECCE-QM, UB3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the two unpaired electrons of O2 in "
     "the two pi* orbitals."},
    {"CH3F",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the polar C-F bond of fluoromethane,"
     " with the dipole moment pointing along it from the CH3 group towards "
     "fluorine."},
    {"CO2",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows that CO2 has polar C=O bonds but no "
     "dipole moment, because the two bond dipoles point in opposite "
     "directions and cancel."},
    {"HBr",
     "ECCE-QM, B3LYP/def2-SVP, optimisation and frequencies, orbitals "
     "localised. This calculation shows the polar H-Br bond, with electron "
     "density shifted towards the more electronegative bromine atom, which "
     "gives the molecule a dipole moment."},
    {"HCl",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the polar H-Cl bond, with electron "
     "density shifted towards the more electronegative chlorine atom, which "
     "gives the molecule a dipole moment."},
    {"SO2",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows that SO2 is bent, so its two polar "
     "bonds do not cancel and the molecule has a dipole moment."},
    {"BF3",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the trigonal planar shape of BF3, "
     "with three bonding pairs and no lone pairs around boron."},
    {"BeCl2",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the linear shape of BeCl2, with two "
     "bonding pairs and no lone pairs around beryllium."},
    {"BrF5",
     "ECCE-QM, B3LYP/def2-SVP, optimisation and frequencies, orbitals "
     "localised. This calculation shows the square pyramidal shape of BrF5, "
     "with five bonding pairs and one lone pair around bromine."},
    {"CH4",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the tetrahedral shape of methane, "
     "with four bonding pairs around carbon."},
    {"ClF3",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the T shape of ClF3, with three "
     "bonding pairs and two lone pairs in equatorial positions around "
     "chlorine."},
    {"H2O",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the bent shape of water, with two "
     "bonding pairs and two lone pairs around oxygen."},
    {"NH3",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the trigonal pyramidal shape of "
     "ammonia, with three bonding pairs and one lone pair around nitrogen."},
    {"PF5",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the trigonal bipyramidal shape of "
     "PF5, in which the axial and the equatorial P-F bonds are not "
     "equivalent."},
    {"SF4",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the seesaw shape of SF4, with four "
     "bonding pairs and one lone pair in an equatorial position around "
     "sulfur."},
    {"SF6",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the octahedral shape of SF6, with "
     "six equivalent S-F bonds."},
    {"BH3",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows an electron-deficient molecule, in "
     "which boron has only six electrons in three bonds and an empty p "
     "orbital."},
    {"CH2_triplet",
     "ECCE-QM, UB3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows that CH2 in the triplet state has "
     "two unpaired electrons, which a Lewis structure with only electron "
     "pairs cannot show."},
    {"CN_anion",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the cyanide ion, with a carbon-"
     "nitrogen triple bond and a lone pair on each atom, as its Lewis "
     "structure suggests."},
    {"N2O",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows the linear molecule N2O, which is "
     "described by resonance between two Lewis structures."},
    {"NO3_anion",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows that the three N-O bonds of the "
     "nitrate ion are equivalent, as the average of three resonance "
     "structures."},
    {"O3",
     "ECCE-QM, B3LYP/6-31G*, optimisation and frequencies, orbitals "
     "localised. This calculation shows that the two O-O bonds of ozone are "
     "equivalent, as the average of two resonance structures."},
    {"ar-gas",
     "GROMACS, Lennard-Jones argon, 1000 atoms, 300 K, 20 bar, 2 ns. This "
     "calculation shows the Maxwell-Boltzmann distribution of atomic speeds "
     "(mean 399 m/s) and the mean-square displacement of an almost ideal "
     "gas, which grows as t^2 while the atoms fly freely and as t once "
     "collisions have randomised their direction (D = 948 x 10^-5 cm^2/s)."},
    {"he-gas",
     "GROMACS, Lennard-Jones helium, 1000 atoms, 300 K, 20 bar, 2 ns. This "
     "calculation shows the Maxwell-Boltzmann distribution of the lighter "
     "helium atoms (mean 1259 m/s, faster than argon by the square root of "
     "the mass ratio) and their diffusion (D = 8656 x 10^-5 cm^2/s); use "
     "Compare to draw the argon gas over it."},
    {"ar-liquid",
     "GROMACS, Lennard-Jones argon, 1000 atoms, 85 K, 1.40 g/cm^3, 2 ns. "
     "This calculation shows the mean-square displacement of a liquid: "
     "after a free flight of about 0.1 ps each atom is held by its "
     "neighbours and the displacement grows linearly (D = 1.75 x 10^-5 "
     "cm^2/s, about 540 times smaller than in the argon gas); use Compare "
     "to draw the gas over it."},
    {"water-spce",
     "GROMACS, SPC/E water, 977 molecules, 298 K, 1 bar, PME, 5 ns. This "
     "calculation shows the diffusion of liquid water from the mean-square "
     "displacement of the molecular centres of mass (D = 2.47 x 10^-5 "
     "cm^2/s, Einstein fit over 628-2500 ps)."},
    {"decane-opls",
     "GROMACS, OPLS-AA n-decane, 200 molecules, 298 K, 1 bar, 5 ns. This "
     "calculation shows the diffusion of a liquid hydrocarbon from the "
     "mean-square displacement of the molecular centres of mass (D = 0.82 x "
     "10^-5 cm^2/s, about three times smaller than for water); use Compare "
     "to draw the water over it."},
    {"nacl-dissolving",
     "GROMACS, a NaCl crystal of 64 ions in 4005 SPC/E water molecules "
     "(Joung-Cheatham ions), 300 K, 1 bar, 20 ns; the trajectory holds the "
     "first 3 ns in 101 frames, centred on the crystal. This calculation shows the "
     "crystal dissolving: ions leave from the corners and edges first, each "
     "taking a shell of water molecules with it."},
    {"ice-to-steam",
     "GROMACS, a slab of 1600 TIP4P/Ice molecules (ice Ih) with vacuum "
     "above and below, heated from 200 K to 500 K over 30 ns and held at "
     "500 K for 10 ns. This calculation shows melting as a jump in the "
     "potential energy of about 4 kJ/mol per molecule near 280 K (the "
     "model melts at about 270 K; a heating ramp overshoots), followed by "
     "molecules evaporating from the liquid into the vacuum as the "
     "temperature rises."},
    {"ar-slab-40K",
     "GROMACS, Lennard-Jones argon, a crystal slab of 864 atoms in vacuum, "
     "40 K, 5 ns. This calculation shows a solid: the atoms vibrate about "
     "their lattice sites and the mean-square displacement stays flat; use "
     "Compare to draw the warmer slabs over it."},
    {"ar-slab-100K",
     "GROMACS, Lennard-Jones argon, the same slab at 100 K, 5 ns. This "
     "calculation shows the crystal melted into a liquid that holds "
     "together, with a few atoms evaporated into the space around it."},
    {"ar-slab-130K",
     "GROMACS, Lennard-Jones argon, the same slab at 130 K, 5 ns. This "
     "calculation shows a liquid close to its critical point: the slab "
     "spreads out and many atoms are in the vapour."},
    {"ar-slab-200K",
     "GROMACS, Lennard-Jones argon, the same slab at 200 K, 5 ns. This "
     "calculation shows a gas: no liquid is left and the atoms fill the box "
     "evenly."},
    {"trpcage", "GROMACS, AMBER99SB-ILDN, TIP3P. This study shows the stages "
                "of a molecular dynamics simulation: preparation, energy "
                "minimisation, equilibration and production. The temperature "
                "and energy plots show the system settling during "
                "equilibration; the run is too short to show folding."},
  };
  vector<string> thumbUrls;
  std::function<void(Resource *)> annotate = [&](Resource *projRes) {
    vector<Resource *> *calcs = projRes->getChildren(true);
    if (!calcs) return;
    for (Resource *calc : vector<Resource *>(*calcs)) {
      if (!calc) continue;
      if (calc->getDescriptor() &&
          calc->getDescriptor()->getName() == "project") {
        annotate(calc);   // a topic
        continue;
      }
      if (dynamic_cast<ICalculation*>(calc) && added.count(calc->getName()))
        thumbUrls.push_back(calc->getURL().toString());
      for (const auto &note : kNotes) {
        // a calculation that was already there keeps the user's annotation
        if (calc->getName() == note.name && added.count(calc->getName())) {
          vector<MetaDataResult> ann(1);
          ann[0].name = VDoc::getEcceNamespace() + ":annotation";
          ann[0].value = note.text;
          calc->addProps(ann);
          //  A study's tasks are what the user opens; each carries the note.
          if (vector<Resource*> *tasks = calc->getChildren(true))
            for (Resource *task : *tasks)
              if (task) task->addProps(ann);
        }
      }
    }
  };
  if (Resource *projRes = EDSIFactory::getResource(project)) annotate(projRes);
  // The imported calculations have no thumbnail until the Builder saves
  // one; ask VizThumbnail, as the Dirdy editor does. It renders in its own
  // process, so the import does not wait for it.
  if (!thumbUrls.empty()) {
    VizThumbnail::thumbnail(ORGANIZER, thumbUrls);
    cerr << "Organizer: thumbnails requested for " << thumbUrls.size()
         << " example calculations" << endl;
  }
  if (WxResourceTreeItemData *projNode = findNode(project, false, false))
    p_treeCtrl->refresh(projNode);
  findNode(project, true, true);
  setContextPanel();
  if (!p_batchError.empty()) {
    message = "An example could not be imported: " + p_batchError;
    p_batchError.clear();
    return "";
  }
  if (topUp) {
    message = "Added " + std::to_string(nExamples) + " example calculations";
    if (!addedTopics.empty()) {
      message += " (topics: ";
      for (size_t i = 0; i < addedTopics.size(); i++)
        message += (i ? ", " : "") + addedTopics[i];
      message += ")";
    }
    message += " to " + name + ".";
    return name;
  }
  message = "Reading the results of the example calculations in " +
            name + "...";
  return name;
}


/**
 * A user's first start puts the examples into "Examples" without the menu.
 * The preference records that it was done, so a user who deletes the
 * project does not get it back at every start; an Examples project that
 * is already there is left as it is and only recorded.
 * ECCE_NO_EXAMPLE_IMPORT=1 switches this off (the test suites set it).
 */
void CalcMgr::importExamplesOnce()
{
  const char *off = getenv("ECCE_NO_EXAMPLE_IMPORT");
  if (off && *off && string(off) != "0") return;
  Preferences prefs("Organizer");
  string done;
  prefs.getString("Organizer.ExampleSet", done);
  if (done == kExampleSetVersion) return;

  EDSIServerCentral central;
  EcceURL home = central.getDefaultUserHome();
  if (!EDSIFactory::getResource(home)) return;   // not logged in: next start
  cerr << "Organizer: examples import started" << endl;
  startDisabler();
  string message;
  bool created = false;
  if (!done.empty()) {
    // After an upgrade: the sets the user had come back in the new layout.
    std::set<string> sets;
    for (const string &id : removeOldExamples()) sets.insert(id);
    // Sets already opened in the new layout get what this version added.
    for (const ExampleSet &set : exampleSets())
      if (EDSIFactory::getResource(
              home.getChild(kExamplesTop).getChild(set.project)))
        sets.insert(set.id);
    //  An upgrade that finds no example set (an earlier start removed the
    //  old ones and stopped) still gets the first-start set.
    if (sets.empty()) sets.insert(kFirstStartSet);
    for (const string &id : sets)
      if (openExampleSet(id, message, created).empty())
        setMessage(message, WxFeedback::WARNING);
    setMessage("The example calculations are now in Examples, sorted by "
               "topic.", WxFeedback::INFO);
  } else if (!EDSIFactory::getResource(home.getChild("Examples"))) {
    string made = openExampleSet(kFirstStartSet, message, created);
    if (made.empty()) {
      stopDisabler();
      setMessage(message, WxFeedback::WARNING);
      return;
    }
    setMessage("The example calculations are in the project " + made +
               "; File > Open Example Calculations has more.",
               WxFeedback::INFO);
  }
  stopDisabler();
  prefs.setString("Organizer.ExampleSet", kExampleSetVersion);
  prefs.saveFile();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXPORT
 */
void CalcMgr::OnExportClick( wxCommandEvent& event )
{
}


// TreeCtrl event handling functions

/*!
 * wxEVT_COMMAND_TREE_SEL_CHANGED event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlSelChanged( wxTreeEvent& event )
{
  if (p_testTreeEvents)
    p_testTreeEvents->push_back(
        event.GetItem() == p_treeCtrl->GetRootItem() ? "sel:root" : "sel");

  // If the parent of current selection is collapsed, the tree will auto select
  // root and then select the collapsed parent node. Skip the select root case.
  if (event.GetItem() == p_treeCtrl->GetRootItem()) {
    return;
  }

  onSelectionChange(true);

#ifdef __WXMSW__
  // Rows of the old and new selection were seen left unpainted on
  // Windows after a click; repaint the tree once the click is done.
  CallAfter([this]() { p_treeCtrl->Refresh(); });
#endif

  //  cerr << "Leave SelChanged event\n";
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TREE_SEL_CHANGING event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlSelChanging( wxTreeEvent& event )
{
  //  cerr << "\nEnter SelChanging event\n";

  //  cerr << "Leave SelChanging event\n";
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TREE_BEGIN_DRAG event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlBeginDrag( wxTreeEvent& event )
{
  if (p_batch) { event.Veto(); return; }
  //  cerr << "\nEnter BeginDrag event\n";

  // Detect whether current selection can be moved and call veto if necessary
  WxResourceTreeItemData * itemData = dynamic_cast<WxResourceTreeItemData*>
    (p_treeCtrl->GetItemData(event.GetItem()));

  // @todo Put logic about movable here to allow or veto
  if (!itemData->getUrl().isSystemFolder()) {
    wxTextDataObject my_data(itemData->getUrl().toString());
    /*
    wxFileDataObject my_data;
    my_data.AddFile(itemData->getUrl().toString());
    wxArrayString filenames = my_data.GetFilenames();
    size_t nFiles = filenames.GetCount();
    wxString str;
    str.Printf( _T("%d files dropped"), (int)nFiles);
    cerr << str << endl;
    for ( size_t n = 0; n < nFiles; n++ ) {
      cerr << filenames[n] << endl;
    }
    */

    wxDropSource dragSource( my_data, this );
    (void)dragSource.DoDragDrop( wxDrag_DefaultMove );
    event.Skip();
  }
  else
    event.Veto();

  //  cerr << "Leave BeginDrag event\n";
}


/*!
 * wxEVT_COMMAND_TREE_END_DRAG event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlEndDrag( wxTreeEvent& event )
{
  /*
    No need for this function anymore since the event is being handled
    generically
  startDisabler();  //  cerr << "\nEnter EndDrag event\n";

  ReleaseMouse();

  WxResourceTreeItemData * target = dynamic_cast<WxResourceTreeItemData*>
    (p_treeCtrl->GetItemData(event.GetItem()));

  // When dropped outside the tree.
  if (target == 0) {
    event.Veto();
    stopDisabler();
    return;
  }
  
  // Check for authentication to access the target.
  bool authSucceed = true;
  if (p_treeCtrl->GetChildrenCount(target->GetId(), false) == 0) {
    authSucceed = p_treeCtrl->loadChildren(target, false);
  }

  // Veto drop to unauthorized area.
  if (!authSucceed) {
    event.Veto();
    stopDisabler();
    return;
  }

  EcceURL targetUrl = target->getUrl();
  EcceURL sourceUrl = p_dragSource->getUrl();

  // Veto drop to itself, its parent or its descendants.
  if (targetUrl == sourceUrl || targetUrl == sourceUrl.getParent() ||
      targetUrl.isChildOf(sourceUrl)) {
    event.Veto();
    stopDisabler();
    return;
  }

  // @todo Check target type to see if it can contain source
  // @todo Need to make sure destination is a collection type
  string sourceName = sourceUrl.getFilePathTail();
  EcceURL newUrl = targetUrl.getChild(sourceName);
  
  if (p_dragSource->getResource()->move(newUrl)) {
    p_treeCtrl->refresh(p_treeCtrl->getParent(p_dragSource));
    p_treeCtrl->refresh(findNode(targetUrl, false, true));
    findNode(newUrl, true, true);
    event.Allow();
    notifyRename(sourceUrl.toString(), newUrl.toString(), "");
  }
  else {
    ewxMessageDialog dlg(this, p_dragSource->getResource()->messages(),
                         "Paste Failed!", wxOK|wxICON_EXCLAMATION, 
                         wxDefaultPosition);
    dlg.ShowModal();
    event.Veto();
  }

  stopDisabler();
  //  cerr << "Leave EndDrag event\n";
  */
}


/*!
 * wxEVT_COMMAND_TREE_BEGIN_RDRAG event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlBeginRDrag( wxTreeEvent& event )
{
  cerr << "\nEnter BeginRDrag event\n";
  // @todo Will this happen at all since right click will pop up menu.

  event.Allow();

  cerr << "Leave BeginRDrag event\n";
}


/*!
 * wxEVT_COMMAND_TREE_DELETE_ITEM event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlDeleteItem( wxTreeEvent& event )
{
  //  cerr << "DeleteItem: " << p_treeCtrl->GetItemText(event.GetItem()) << endl;
  
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TREE_ITEM_ACTIVATED event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlItemActivated( wxTreeEvent& event )
{
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TREE_ITEM_COLLAPSED event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlItemCollapsed( wxTreeEvent& event )
{
  if (p_testTreeEvents) p_testTreeEvents->push_back("collapsed");
  //  cerr << "Leave Collapsed event\n";
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TREE_ITEM_COLLAPSING event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlItemCollapsing( wxTreeEvent& event )
{
  startDisabler();

  if (event.GetItem() != p_treeCtrl->GetSelection()) {
    // When current selection is not the one colapsing,
    // refresh direct children. (Grandchildren will be lost)
    p_treeCtrl->refresh(dynamic_cast<WxResourceTreeItemData *>
                        (p_treeCtrl->GetItemData(event.GetItem())));
  }

  event.Skip();
  stopDisabler();
}


/*!
 * wxEVT_COMMAND_TREE_ITEM_EXPANDED event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlItemExpanded( wxTreeEvent& event )
{
  //  cerr << "\nEnter Expanded event\n";

  //  Bring the new children into view, then the item itself.  Not
  //  ScrollTo(): on MSW it makes the item the top row, so every expand
  //  scrolled the tree.
  wxTreeItemId id = event.GetItem();
  wxTreeItemId last = p_treeCtrl->GetLastChild(id);
  wxTreeItemId next = p_treeCtrl->GetNextSibling(id);
  if (last.IsOk()) p_treeCtrl->EnsureVisible(last);
  if (next.IsOk()) p_treeCtrl->EnsureVisible(next);
  p_treeCtrl->EnsureVisible(id);
  
  //  cerr << "Leave Expanded event\n";
  event.Skip();
}


/*!
 * Be careful not to append node twice!!!!!!!!!!!!
 * wxEVT_COMMAND_TREE_ITEM_EXPANDING event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlItemExpanding( wxTreeEvent& event )
{
  //  cerr << "\nEnter Expanding event\n";
  startDisabler();

  WxResourceTreeItemData * itemData = (WxResourceTreeItemData *)
    p_treeCtrl->GetItemData(event.GetItem());

  //  cerr << "Expanding node: " << itemData->getName() << endl;
  //  cerr << "Children in widget tree: " << p_treeCtrl->GetChildrenCount(event.GetItem(), false) << endl;
  //  cerr << "Children in data tree: " << itemData->getnumChildren() << endl;

  bool authSucceed = true;
  if (p_treeCtrl->GetChildrenCount(itemData->GetId(), false) == 0) {
    authSucceed = p_treeCtrl->loadChildren(itemData, false);
  }

  //  cerr << "Leaving Expanding event\n";
  if (authSucceed)
    event.Skip();
  else
    event.Veto();

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_TREE_KEY_DOWN event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlKeyDown( wxTreeEvent& event )
{
  //  cerr << "\nEnter KeyDown event\n";
  startDisabler();

  int keycode = event.GetKeyEvent().GetKeyCode();

  //  cerr << (event.GetKeyEvent().ControlDown()?"Ctrl + ":"")
  //       << "Key Number: " << keycode << endl;

  // When ctrl is pressed down
  if (event.GetKeyEvent().ControlDown()) {
    switch (keycode) {
    case 'x':
    case 'X':
      copySelectionToClipboard(true);
      //      dumpClipboard();
      break;
    case 'c':
    case 'C':
      copySelectionToClipboard(false);
      //      dumpClipboard();
      break;
    case 'v':
    case 'V':
      if (!p_clipboard.empty()) {
        paste();
      }
      //      dumpClipboard();
      break;
    case 'q':
    case 'Q':
      Destroy();
      break;
    case WXK_TAB:
      break;
    default:
      // Control with something else will be treated using default handling
      event.Skip();
      break;
    }
  }
  else {
    // When ctrl is not pressed down
    wxTreeItemId itemId = p_treeCtrl->GetSelection();
    switch (keycode) {
    case WXK_SPACE:
    case WXK_RETURN:  // Key code for "Enter" key
      if (itemId.IsOk() && p_treeCtrl->ItemHasChildren(itemId)) {
        p_treeCtrl->Toggle(itemId);
      }
      break;
    case WXK_DELETE: // Key code for "Delete" key
    case WXK_BACK:   // Key code for "Backspace" key
      deleteSelection();
      break;
    case WXK_TAB:
      break;
    default:
      // Default key handling such as left right up down arrows and +- etc
      event.Skip();
      break;
    }
  }

  stopDisabler();
  //  cerr << "Leave KeyDown event\n";
}


/*!
 * wxEVT_COMMAND_TREE_ITEM_MENU event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlItemMenu( wxTreeEvent& event )
{
  //  The menu is built for the selection (getContextMenu).  The generic
  //  tree (GTK) selects the item under a right click; the native wxMSW one
  //  does not, so the menu was the previously selected item's.
  wxTreeItemId item = event.GetItem();
  if (item.IsOk() && !p_treeCtrl->IsSelected(item))
    p_treeCtrl->SelectItem(item);
  event.Skip();
}


void CalcMgr::OnPanelSelection( wxCommandEvent& event )
{
  vector<EcceURL> urls = EcceURL::toVector(event.GetString().ToStdString());
  if (urls.size() == 1 && p_contextPanel != 0) {
    p_contextPanel->selectResource(urls[0]);
  }

  p_focusOnTree = false;

  onSelectionChange(false);
}


/**
 * Get the currently selected node or nodes
 *
 * return true if single selection
 *
 * @todo Right now there is no list view panel so it is getting one selection
 * from the treectrl. Once the list view panel is done, the selection could
 * be obtained from either of the two places and could be multiple nodes too.
 */
bool CalcMgr::getSelection()
{
  p_currentSelection.clear();

  if (p_contextPanel && !p_popupOnTree && !p_focusOnTree) {
    vector<EcceURL> panelSelections = p_contextPanel->getSelections();
    if (panelSelections.size() != 0) {
      p_currentSelection = panelSelections;
    }
  }
  
  if (p_currentSelection.empty()) {
    WxResourceTreeItemData * itemData = p_treeCtrl->getSelection();
    p_currentSelection.push_back(itemData->getUrl());
  }

  return p_currentSelection.size()==1;
}


/**
 * Copy current selection(s) to clipboard
 *
 * Iterate through selections and copy them into clipboard.
 * Enable the paste button and menu.
 */
void CalcMgr::copySelectionToClipboard(bool deleteAfterPaste)
{
  p_deleteAfterPaste = deleteAfterPaste;
  getSelection();
  p_clipboard = p_currentSelection;
  p_toolBar->EnableTool(wxID_PASTE, !p_clipboard.empty());
  GetMenuBar()->Enable(wxID_PASTE, !p_clipboard.empty());
}


/**
 * Delete current selection
 */
// Menu state can lag behind the selection (keyboard Delete, accelerators),
// so the handlers refuse system folders themselves.
bool CalcMgr::selectionHasSystemFolder()
{
  if (!getSelection()) return false;
  for (size_t i = 0; i < p_currentSelection.size(); i++)
    if (p_currentSelection[i].isSystemFolder()) return true;
  return false;
}


void CalcMgr::deleteSelection()
{
  if (selectionHasSystemFolder()) return;
  getSelection();
  vector<EcceURL> selection = p_currentSelection;
  vector<EcceURL>::iterator itor = selection.begin();
  bool yesToAll = false;
  for (; itor != selection.end(); itor++) {

    Resource *currentRes = EDSIFactory::getResource(*itor);
    if (currentRes == 0)
      continue;

    bool allowedForDelete;
    string msg, name;
    currentRes->getDeleteMessage(allowedForDelete, msg);

    // special STTR reaction rate study logic to prevent the deletion
    // of the reactant/product/transition-state calculations for gas phase
    // and not to delete a non-leaf calculation for condensed phase
    // no need to check this if deletes are already prevented.
    // getResource() can legitimately return null here too -- guard
    // rather than crash the whole delete loop over inapplicable
    // reaction-study logic for one item.
    if (allowedForDelete) {
      Resource *parent = EDSIFactory::getResource(
                                         currentRes->getURL().getParent());
      if (parent != 0 && parent->getApplicationType() ==
                  ResourceDescriptor::AT_REACTION_STUDY) {
        name = currentRes->getName();
        if (name=="Transition-State" ||
            name=="Reactant1" || name=="Reactant2" ||
            name=="Product1" || name=="Product2") {
          allowedForDelete = false;
          msg = "Cannot delete reaction study calculation \"" + name + "\".";
        }
      } else if (parent != 0 && parent->getApplicationType() ==
                 ResourceDescriptor::AT_CONDENSED_REACTION_STUDY) {
        Session *session = dynamic_cast<Session*>(parent);
        vector<Resource *> targets =
          session->getTargets(currentRes, Session::INPUT_PROVIDER);
        if (targets.size() > 0) {
          name = currentRes->getName();
          allowedForDelete = false;
          msg = "Cannot delete condensed phase reaction study calculation \"" +
                name + "\" because it has dependent tasks.";
        }
      }
    }

    int okToDelete = wxID_NO;
    if (allowedForDelete) {
      if (GetMenuBar()->IsChecked(wxID_CONFIRM_DELETE) && !yesToAll
          && msg != "") {
        ewxMessageDialog dlg(this, msg, "Delete?",
                             wxNO_DEFAULT|wxYES_NO|wxICON_QUESTION,
                             wxDefaultPosition,
                             (selection.size()>1?ECCE_YES_NO_TO_ALL:0));
        okToDelete = dlg.ShowModal();
        if (okToDelete == wxID_YES_TO_ALL)
          yesToAll = true;
        else if (okToDelete == wxID_NO_TO_ALL)
          break;
      }
      else
        okToDelete = wxID_YES;
    }
    else {
      ewxMessageDialog dlg(this, msg, "Delete Not Allowed!",
                           wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
      dlg.ShowModal();
    }
    
    if (!yesToAll && okToDelete != wxID_YES)
      continue;

    string contentType = currentRes->getDescriptor()->getContentType();
    p_treeCtrl->removeFromTree(*itor, true);
    notifyRemove(itor->toString(), contentType);
  }
  
  WxResourceTreeItemData * itemData = p_treeCtrl->getSelection();
  
  if (itemData->getResource()->getDescriptor()->getIsSessionBased()) {
    p_treeCtrl->refresh(itemData);
  }
  
  setContextPanel();
  
  onSelectionChange(false);
}


/**
 * Paste nodes from clipboard to the current selected node
 */
void CalcMgr::paste()
{
  //  cerr << "\nEnter paste event\n";
  // Assert clipboard is not empty here.
  // Need to think about ret and msg
  string msg = "";
  vector<EcceURL> copiedFiles;
  
  WxResourceTreeItemData * destination = p_treeCtrl->getSelection();
  EcceURL destinationUrl = destination->getUrl();
  
  // First check if destination is a sub folder of the folder being copied
  if (destinationUrl == p_clipboard[0] ||
      destinationUrl.isChildOf(p_clipboard[0])) {
    ewxMessageDialog dlg(this, "Cannot paste parent folder into a child "
                         "folder", "Paste Failed!", 
                         wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg.ShowModal();
  }
  else if (!p_treeCtrl->setExpandable(destination)) {
    ewxMessageDialog dlg(this, "Must paste into a container type object",
                         "Paste Failed!", 
                         wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg.ShowModal();
  }
  // The logic to decide whether the copy/paste is on the same server is
  // moved to Resource::copy()
  else {
    vector<EcceURL>::iterator itor = p_clipboard.begin();
    for (; itor != p_clipboard.end(); itor++) {
      WxResourceTreeItemData * source = findNode(*itor, false, true);
      // Make sure the source still exists
      if (source) {
        string name = itor->getFilePathTail();
        EcceURL newUrl = destinationUrl.getChild(name);

        // If cut and paste into the same location, do nothing.
        if (p_deleteAfterPaste && newUrl == (*itor))
          continue;

        if (!(source->getResource()->copy(newUrl))) {
          msg += source->getResource()->messages();
          break;
        }
        else {
          copiedFiles.push_back(*itor);
          notifyCreate(newUrl.toString());
        }
      }
      else {
        // For now, quitly skip if not exist,
        // should show some error prompt and ask for continue or not.
        if (itor+1 != p_clipboard.end()) {
          ewxMessageDialog dlg(this, "Source file " + itor->toString() +
                               " for paste does not exist!\n"
                               "Do you want to continue pasting the other "
                               "files in the clipboard?",
                               "Source file does not exist!",
                               wxOK | wxCANCEL | wxICON_QUESTION,
                               wxDefaultPosition);
          if (dlg.ShowModal() != wxID_OK)
            break;
        }
        else {
          ewxMessageDialog dlg(this, "Source file " + itor->toString() +
                               " for paste does not exist!\n",
                               "Paste Failed!", wxOK|wxICON_EXCLAMATION,
                               wxDefaultPosition);
          dlg.ShowModal();
        }
      }
    }
    
    p_treeCtrl->refresh(destination);
    setContextPanel();

    if (p_deleteAfterPaste) {
      p_deleteAfterPaste = false;

      // delete the original files that were copied successfully.
      vector<EcceURL>::iterator itor = copiedFiles.begin();
      for (; itor != copiedFiles.end(); itor++) {
        Resource *res = EDSIFactory::getResource(*itor);
        if (res != 0) {
          if (p_treeCtrl->removeFromTree(*itor, true)) {
            notifyRemove(itor->toString(), "");
          }
        }
      }
      p_clipboard.clear();
      p_toolBar->EnableTool(wxID_PASTE, false);
      GetMenuBar()->Enable(wxID_PASTE, false);
      findNode(destinationUrl, true, true);
    }
  }

  onSelectionChange(false);

  if (msg != "") {
    ewxMessageDialog dlg(this, msg,
                         "Paste Failed!", wxOK|wxICON_EXCLAMATION, 
                         wxDefaultPosition);
    dlg.ShowModal();
  }
  //  cerr << "Leave paste event\n";
}


/**
 * Retrieves the ResourceType and RUNSTATE from the given tree item.
 */
bool CalcMgr::getTypeAndState(WxResourceTreeItemData * itemData,
                              ResourceType *& resType,
                              ResourceDescriptor::RUNSTATE & state)
{
  // try to play nicely, if the caller provided a bad pointer, get current
  // selection for them
  if (itemData == 0) {
    itemData = p_treeCtrl->getSelection();
  }

  if (itemData != 0) {
    Resource * res = itemData->getResource();
    // getResource() can legitimately return null (e.g. the resource
    // failed to load because the data server wasn't reachable/ready
    // yet) -- confirmed via a real core dump hit on every cold start,
    // in TreeEvtHandler::OnSetFocus() -> onSelectionChange() ->
    // getFileMenu() -> here, same bug shape as the WxResourceTreeCtrl
    // fix alongside this one.
    if (res == 0) {
      return false;
    }
    resType = res->getDescriptor();
    if (resType == 0) {
      return false;
    }

    if (resType->getStateBased()) {
      TaskJob *job = dynamic_cast<TaskJob*>(res);
      if (job) {
        state = job->getState();
      } else {
        return false;
      }
    }
    else {
      // We don't return false in this case so that this function can be used
      // generically with Projects, MDStudies, etc, which are stateless
      //return false;
    }
  } else {
    return false;
  }

  return true;
}


/**
 * Generate the right click menu based on the given tree item.
 * If a tree item is given, then it is used to determine the content of the
 * menu.  Otherwise, the content is based on the current tree selection.
 * The content of the given menu is cleared first.
 *
 * @param menu      the menu on which to append these generic items to, which 
 *                  allows the caller to customize their context menu
 * @param itemData  helps determing content of the menu
 */
void CalcMgr::getContextMenu(wxMenu & menu)
{
  ResourceType * resType = 0;
  ResourceDescriptor::RUNSTATE state = ResourceDescriptor::STATE_ILLEGAL;

  WxResourceTreeItemData * itemData = p_treeCtrl->getSelection();
  // error checking when gathering tree node and resource type
  if (!getTypeAndState(itemData, resType, state)) {
    return;
  }

  // Only a calculation can be duplicated (#47).
  bool isCalculationNode = itemData && itemData->getResource() &&
      itemData->getResource()->getContentType() ==
          ResourceDescriptor::CT_CALCULATION;

  // build the "new" part of the context menu first
  // the variables to iterate through what "can be contained"
  //  What New offers comes from the nearest container: a calculation holds
  //  nothing itself, so New beside it creates in its folder or project, as
  //  OnNewResourceClick does.
  WxResourceTreeItemData * container = nearestContainer(itemData, 0);
  Session * containerSession =
      container ? dynamic_cast<Session *>(container->getResource()) : 0;
  vector<ResourceType *> resVec;
  if (container && container->getResource() &&
      container->getResource()->getDescriptor())
    resVec = container->getResource()->getDescriptor()->getOfferedContains(
        containerSession && containerSession->numMembers() == 0);
  wxMenu *newMenu = new wxMenu();

  // for each "contained", add to menu
  for (size_t i = 0; i < resVec.size(); i++) {
    // need to remove redundant "New " from each sub menu item
    string name(resVec[i]->getMenuItem(), 4);
    wxMenuItem * menuItem = new wxMenuItem(&menu, resVec[i]->getId(),
            name.c_str(), _T(""), wxITEM_NORMAL);
    menuItem->SetBitmap(ewxBitmap(resVec[i]->getIcon()));
    newMenu->Append(menuItem);
  }
  //  An imported output is a new calculation too; users look for it here,
  //  not only under File.
  if (!resVec.empty()) {
    newMenu->AppendSeparator();
    newMenu->Append(wxID_IMPORT,
                    _("Import Calculation from Output File..."));
  }
  menu.Append(wxID_NEW, _T("New..."), newMenu);
  menu.Enable(wxID_NEW, !resVec.empty());
  menu.AppendSeparator();

  // build the standard part of the context menu
  menu.Append(wxID_CUT, wxT("C&ut"));
  menu.Enable(wxID_CUT, true);
  menu.Append(wxID_COPY, wxT("&Copy"));
  menu.Append(wxID_PASTE, wxT("&Paste"));
  menu.Enable(wxID_PASTE, !p_clipboard.empty());
  //  A study task is rerun by a copy that keeps its place in the study.
  if (!isCalculationNode && itemData && dynamic_cast<MdTask*>(
          itemData->getResource()))
    menu.Append(wxID_DUPLICATE, _("Duplicate for Rerun"));
  if (isCalculationNode) {
    menu.Append(wxID_DUPLICATE, _("Duplicate for Rerun"));
    menu.Append(wxID_DUPLICATE2, _("Duplicate Setup with Last Geometry"));
    menu.Append(wxID_LAUNCH_SELECTED, _("Launch Selected..."));
    //  Offered under the same rule as the Run Mgmt item: only for a job
    //  that has been submitted.
    wxMenuItem *reconnect = menu.Append(wxID_RECONNECT_SELECTED,
                                        _("Reconnect Selected..."));
    reconnect->Enable(state == ResourceDescriptor::STATE_SUBMITTED ||
                      state == ResourceDescriptor::STATE_RUNNING ||
                      state == ResourceDescriptor::STATE_WAITING);
    menu.Append(wxID_REPARSE_OUTPUTS, _("Re-Parse Outputs"), _("Run the "
                "parsers again over the outputs stored with the calculation"))
        ->Enable(state >= ResourceDescriptor::STATE_COMPLETED);
    string why;
    bool can = OrbitalGuess::available(
        dynamic_cast<TaskJob*>(itemData->getResource()), why);
    wxMenuItem *item = menu.Append(wxID_DUPLICATE3,
        _("Duplicate with Final Orbitals"),
        can ? _("Duplicate with the last geometry and start from this "
                "calculation's final orbitals") : wxString(why));
    item->Enable(can);
  }
  //  Several calculation rows selected in the project table: the tree's own
  //  selection is then the project, so the batch items come from the same
  //  rows Launch Selected itself would use.
  if (!isCalculationNode) {
    vector<EcceURL> rows = launchSelection();
    bool allCalcs = rows.size() > 1;
    for (size_t i = 0; allCalcs && i < rows.size(); i++) {
      WxResourceTreeItemData *n = findNode(rows[i], false, true);
      allCalcs = n && dynamic_cast<TaskJob*>(n->getResource());
    }
    if (allCalcs) {
      menu.Append(wxID_LAUNCH_SELECTED, _("Launch Selected..."));
      menu.Append(wxID_RECONNECT_SELECTED, _("Reconnect Selected..."));
      menu.Append(wxID_REPARSE_OUTPUTS, _("Re-Parse Outputs"));
    }
  }
  menu.Append(wxID_RENAME, wxT("&Rename"));
  menu.Enable(wxID_RENAME, true);
  menu.Append(wxID_DELETE, wxT("&Delete"));
  menu.Enable(wxID_DELETE, true);

  // get the tools available based on the state of the selection
  if (getSelection()) {
    itemData = findNode(p_currentSelection[0], false, true);
    getTypeAndState(itemData, resType, state);
    vector<ResourceTool *> toolVec;
    if (resType->getStateBased()) {
      toolVec = resType->getTools(state);
    }
    else {
      toolVec = resType->getTools();
    }

    // for each tool, add the tool to the menu
    if (!toolVec.empty()) {
      menu.AppendSeparator();
    }
    vector<ResourceTool *>::iterator toolItor;
    for (toolItor = toolVec.begin(); toolItor != toolVec.end(); toolItor++) {
      wxMenuItem *item = new wxMenuItem(&menu, (*toolItor)->getId(),
              (*toolItor)->getMenuItem().c_str(), _T(""), wxITEM_NORMAL);
      item->SetBitmap(wxBitmap(ewxBitmap((*toolItor)->getIcon())
                               .ConvertToImage().Scale(16,16)));
      menu.Append(item);
    }

    // The two Run Mgmt tasks used mid-run are also reachable here; the
    // state-based task list is what enables them, as in the menu bar.
    if (isCalculationNode) {
      bool sep = false;
      vector<ResourceRmTask *> rmVec = resType->getRmTasks(state);
      for (size_t i = 0; i < rmVec.size(); i++) {
        const string & rn = rmVec[i]->getName();
        if (rn != "terminateCalc" && rn != "shellInRunDir") continue;
        if (!sep) { menu.AppendSeparator(); sep = true; }
        menu.Append(rmVec[i]->getId(), rmVec[i]->getMenuItem(), "",
                    wxITEM_NORMAL);
      }
    }

    if (p_currentSelection[0].isSystemFolder()) {
      menu.Enable(wxID_CUT, false);
      menu.Enable(wxID_RENAME, false);
      menu.Enable(wxID_DELETE, false);
    }
  }
  else {
    menu.Enable(wxID_CUT, true);
    menu.Enable(wxID_RENAME, false);
    menu.Enable(wxID_DELETE, true);
  }
}


/**
 * Generate the File menu based on the given tree item.
 * If a tree item is given, then it is used to determine the content of the
 * menu.  Otherwise, the content is based on the current tree selection.
 * The content of the given menu is cleared first.
 *
 * @param menu      the menu on which to append these generic items to, which 
 *                  allows the caller to customize their context menu
 * @param itemData  helps determing content of the menu
 */
void CalcMgr::getFileMenu(wxMenu & menu, WxResourceTreeItemData * itemData)
{
  // clear given menu of any menu items
  clearMenu(menu);

  ResourceType * resType = 0;
  ResourceDescriptor::RUNSTATE state = ResourceDescriptor::STATE_ILLEGAL;

  // Without a selection the menu is still built, so New works before the
  // first click; the same menu is built at construction time.
  if (itemData == 0) itemData = p_treeCtrl->getSelection();

  // New: what OnNewResourceClick would offer, which is the nearest
  // container at or above the selection, else the user's home.
  {
    ResourceType * target = 0;
    WxResourceTreeItemData * where = nearestContainer(itemData, 0);
    if (where) target = where->getResource()->getDescriptor();
    else target = ResourceDescriptor::getResourceDescriptor().
        getResourceType("collection", "ecceProject", "");
    Session * session = where ?
        dynamic_cast<Session *>(where->getResource()) : 0;
    vector<ResourceType *> types;
    if (target) types = target->getOfferedContains(
        session && session->numMembers() == 0);
    wxMenu * newMenu = new wxMenu();
    for (size_t i = 0; i < types.size(); i++) {
      string label = types[i]->getMenuItem();
      if (label.size() > 4) label = label.substr(4);
      wxMenuItem * mi = new wxMenuItem(newMenu, types[i]->getId(),
              wxString::FromUTF8(label.c_str()), _T(""), wxITEM_NORMAL);
      mi->SetBitmap(ewxBitmap(types[i]->getIcon()));
      newMenu->Append(mi);
    }
    menu.Append(wxID_ANY, _("&New"), newMenu);
    menu.AppendSeparator();
  }

  if (itemData && getTypeAndState(itemData, resType, state)) {
    // (The import entry used to be added here, gated on the selected node
    // being a "project". It is now added unconditionally below -- see there.)

    // Upload only makes sense inside a calculation -- putting a file into a
    // job's own directory. It used to be offered for ANY expandable node,
    // so it also appeared on projects and on the server root, where it
    // reads like a general-purpose file manager action and nobody could say
    // what it was for. Reported from a live session 2026-09-22.
    //
    // Download is unchanged: it is offered for anything that is not a
    // collection, i.e. an actual file.
    bool isCollection = p_treeCtrl->setExpandable(itemData);
    bool isCalculation = false;
    Resource *menuRes = itemData->getResource();
    if (menuRes != (Resource*)0) {
      isCalculation = (menuRes->getContentType() ==
                       ResourceDescriptor::CT_CALCULATION);
    }

    wxMenuItem * tmpMenuItem = (wxMenuItem*)0;
    if (isCollection && isCalculation) {
      tmpMenuItem = new wxMenuItem(&menu, wxID_UPLOAD, _("&Upload File..."),
                                   _T(""), wxITEM_NORMAL);
    } else if (!isCollection) {
      tmpMenuItem = new wxMenuItem(&menu, wxID_DOWNLOAD,
                                   _("&Download File..."),
                                   _T(""), wxITEM_NORMAL);
    }
    if (tmpMenuItem != (wxMenuItem*)0) {
      menu.Append(tmpMenuItem);
    }
    
    menu.AppendSeparator();
  }

  // Importing a calculation from an output file does not depend on what is
  // selected, so it is always offered.
  //
  // It used to be added only when the selected node was literally a
  // "project" -- with a comment conceding the approach ("Hardwire the
  // import entry, not sure if there is any better way to do it"). The
  // effect was that File showed just "Upload File..." and "Quit" unless
  // the user happened to have clicked a project first, which made the
  // entry point for importing externally run output (issues #44 and #94)
  // very hard to find. Reported from a live session 2026-09-22: "Only see
  // it if I select users, or andy, and then click on File ... better to
  // always have access to it".
  // Issue #93: New Structure has to be added HERE, not in
  // CalcMgrGUI::CreateControls(), because getFileMenu() runs clearMenu()
  // and rebuilds this menu from scratch on every selection change -- so
  // anything appended statically at construction is wiped the first time
  // the user clicks a tree node, which is to say immediately. Adding it
  // in the GUI base looked right and did nothing; reported from a live
  // session 2026-09-22 ("Don't find any File > New Structure").
  //
  // Same trap as the Tools menu, which is why addGlobalTools() exists.
  menu.Append(wxID_NEWSTRUCTURE, _("&New Structure...\tCtrl+N"),
              _T(""), wxITEM_NORMAL);
  menu.AppendSeparator();

  menu.Append(wxID_IMPORT, _("&Import Calculation from Output File..."),
              _T(""), wxITEM_NORMAL);
  menu.Append(wxID_IMPORTGROMACS, _("Import &GROMACS Run..."),
              _T(""), wxITEM_NORMAL);
  menu.Append(wxID_OPENEXAMPLES, _("Open &Example Calculations"),
              _T(""), wxITEM_NORMAL);
  menu.AppendSeparator();

  menu.Append(wxID_EXIT, _("&Quit\tCtrl+Q"), _T(""), wxITEM_NORMAL);
}


/**
 * Generate the File menu based on the given tree item.
 * If a tree item is given, then it is used to determine the content of the
 * menu.  Otherwise, the content is based on the current tree selection.
 * The content of the given menu is cleared first.
 *
 * @param menu      the menu on which to append these generic items to, which 
 *                  allows the caller to customize their context menu
 * @param itemData  helps determing content of the menu
 */
void CalcMgr::getHistoryMenu(wxMenu & menu, WxResourceTreeItemData * itemData)
{
  EcceURL url = itemData->getUrl();
  p_hlist->add(url);

  clearMenu(menu);
  menu.Append(wxID_HOME, _("&Home\tAlt+HOME"), _T(""), wxITEM_NORMAL);
  menu.Append(wxID_UP, _("&Up\tAlt+UP"), _T(""), wxITEM_NORMAL);
  menu.Append(wxID_BACKWARD, _("&Back\tAlt+LEFT"), _T(""), wxITEM_NORMAL);
  menu.Append(wxID_FORWARD, _("&Forward\tAlt+RIGHT"), _T(""), wxITEM_NORMAL);

  menu.Enable(wxID_UP, p_treeCtrl->GetRootItem() !=
              p_treeCtrl->GetItemParent(p_treeCtrl->GetSelection()));
  menu.Enable(wxID_BACKWARD, p_hlist->canMoveBack());
  menu.Enable(wxID_FORWARD, p_hlist->canMoveForward());

  menu.AppendSeparator();

  int historyLength = p_hlist->size();
  int i;
  EDSIServerCentral servers;
  for (i=historyLength-1; i>=0; --i) {
    string label = servers.mapURLtoName(p_hlist->get(i));
    menu.AppendRadioItem(i+ID_HISTORY_BASE, label);
    if (p_hlist->get(i) == url)
      menu.Check(i+ID_HISTORY_BASE, true);
  }
}


/**
 * Generate the Run Management menu based on the given tree item.
 * If a tree item is given, then it is used to determine the content of the
 * menu.  Otherwise, the content is based on the current tree selection.
 * The content of the given menu is cleared first.
 *
 * @param menu      the menu on which to append these generic items to, which 
 *                  allows the caller to customize their context menu
 * @param itemData  helps determing content of the menu
 */
void CalcMgr::getRunMgmtMenu(wxMenu & menu, WxResourceTreeItemData * itemData)
{
  // clear given menu of any menu items
  clearMenu(menu);

  ResourceType * resType = 0;
  ResourceDescriptor::RUNSTATE state = ResourceDescriptor::STATE_ILLEGAL;

  if (!getTypeAndState(itemData, resType, state))
    return; // menu should be disabled for multiple selection

  string name, menuItem, subMenuItem;
  map<string, wxMenu*> subMenu;
  vector<ResourceRmTask *> rmTaskVec = resType->getRmTasks(state);
  vector<ResourceRmTask *>::iterator rmTaskItor = rmTaskVec.begin();
  for (;rmTaskItor != rmTaskVec.end(); rmTaskItor++) {
    menuItem = (*rmTaskItor)->getMenuItem();
    name = (*rmTaskItor)->getName();
    int idx;
    if (name == "separator") {
      menu.AppendSeparator();
    }
    else if ((idx = menuItem.find("::")) != string::npos) {
      subMenuItem = menuItem.substr(idx + 2, menuItem.length());
      menuItem = menuItem.substr(0, idx);
      if (subMenu.find(menuItem) == subMenu.end()) {
        subMenu[menuItem] = new wxMenu();
        menu.Append(wxID_STATIC, menuItem, subMenu[menuItem], "");
      }
      subMenu[menuItem]->Append((*rmTaskItor)->getId(), subMenuItem, "",
                                wxITEM_NORMAL);
    } else {
      menu.Append((*rmTaskItor)->getId(), menuItem, "", wxITEM_NORMAL);
    }
  }
  if ((state == ResourceDescriptor::STATE_READY ||
       state == ResourceDescriptor::STATE_CREATED) &&
      itemData && dynamic_cast<TaskJob*>(itemData->getResource())) {
    if (menu.GetMenuItemCount() > 0) menu.AppendSeparator();
    menu.Append(wxID_LAUNCH_SELECTED, _("Launch Selected..."), "",
                wxITEM_NORMAL);
  }
  if ((state == ResourceDescriptor::STATE_SUBMITTED ||
       state == ResourceDescriptor::STATE_RUNNING ||
       state == ResourceDescriptor::STATE_WAITING) &&
      itemData && dynamic_cast<TaskJob*>(itemData->getResource())) {
    if (menu.GetMenuItemCount() > 0) menu.AppendSeparator();
    menu.Append(wxID_RECONNECT_SELECTED, _("Reconnect Selected..."), "",
                wxITEM_NORMAL);
  }
  if (state >= ResourceDescriptor::STATE_COMPLETED &&
      itemData && dynamic_cast<TaskJob*>(itemData->getResource())) {
    if (menu.GetMenuItemCount() > 0) menu.AppendSeparator();
    menu.Append(wxID_REPARSE_OUTPUTS, _("Re-Parse Outputs"), "",
                wxITEM_NORMAL);
  }
}


/**
 * Generate the Tools menu based on the given tree item.
 * If a tree item is given, then it is used to determine the content of the
 * menu.  Otherwise, the content is based on the current tree selection.
 * The content of the given menu is cleared first.
 *
 * @param menu      the menu on which to append these generic items to, which 
 *                  allows the caller to customize their context menu
 * @param itemData  helps determing content of the menu
 */
/**
 * Append the tools that are useful regardless of what is selected.
 *
 * Issue #93: rather than porting the Gateway's row of launcher icons onto
 * the Organizer toolbar -- which was tried, looked wrong, and was reverted
 * -- the two that genuinely belong here are menu entries.
 *
 * Launching is a JMS publish on "ecce_get_app" and startApp() already sends
 * exactly that, so this is UI only. The ids come from the resource files
 * and are not known at compile time, hence the dynamic Connect.
 */
void CalcMgr::addGlobalTools(wxMenu & menu)
{
  // MachineRegister is how a machine gets *defined* in the first place;
  // it had Gateway=-1 in ResourceDescriptor.xml, i.e. no launcher icon,
  // so before this it was unreachable from anywhere in the GUI.
  static const char *globalTools[] = { "MachineRegister", "MachineBrowser",
                                       "PeriodicTable", 0 };

  ResourceDescriptor& descriptor =
      ResourceDescriptor::getResourceDescriptor();

  bool added = false;
  for (int index = 0; globalTools[index] != 0; index++) {
    ResourceTool *tool = descriptor.getTool(globalTools[index]);
    if (tool == (ResourceTool*)0) continue;

    string label = tool->getMenuItem();
    if (label.empty()) label = tool->getLabel();
    if (label.empty()) continue;

    wxMenuItem *item = new wxMenuItem(&menu, tool->getId(),
                                      wxString::FromAscii(label.c_str()),
                                      _T(""), wxITEM_NORMAL);
    menu.Append(item);
    // Connect ONCE per id. getToolsMenu() rebuilds this menu on every
    // selection change, and wxWidgets happily stacks duplicate dynamic
    // handlers for the same id -- which would launch the app once per
    // rebuild, so a click after browsing the tree a few times would open
    // several windows.
    if (p_globalToolIds.find(tool->getId()) == p_globalToolIds.end()) {
      Connect(tool->getId(), wxEVT_COMMAND_MENU_SELECTED,
              wxCommandEventHandler(CalcMgr::OnGlobalTool));
      p_globalToolIds.insert(tool->getId());
    }
    added = true;
  }

  menu.Append(ID_COMPARE_ENERGIES, _("Compare Energies..."));
  if (!p_compareEnergiesWired) {
    Connect(ID_COMPARE_ENERGIES, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(CalcMgr::OnCompareEnergies));
    p_compareEnergiesWired = true;
  }
  menu.Append(ID_ALLOCATIONS, _("Allocations..."));
  if (!p_allocationsWired) {
    Connect(ID_ALLOCATIONS, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(CalcMgr::OnAllocations));
    p_allocationsWired = true;
  }
  added = true;

  if (added) menu.AppendSeparator();
}


/**
 * A selection-independent tool was chosen; ask the gateway for it.
 */
// Two selected calculations become A and B; otherwise the dialog lets the
// user choose among the calculations of the selection's project.
CompareEnergiesDialog *CalcMgr::newCompareEnergiesDialog(
    const vector<EcceURL>& sel)
{
  string project;
  CompareEnergiesDialog::Side a, b;
  if (sel.size() == 2) {
    a.push_back(std::make_pair(sel[0].toString(), 1));
    b.push_back(std::make_pair(sel[1].toString(), 1));
  }
  if (!sel.empty()) {
    const bool isCalc =
        dynamic_cast<PropertyTask*>(EDSIFactory::getResource(sel[0])) != 0;
    project = (isCalc ? sel[0].getParent() : sel[0]).toString();
    if (isCalc && sel.size() == 1) a.push_back(std::make_pair(sel[0].toString(), 1));
  }
  return new CompareEnergiesDialog(this, calculationsIn(project), a, b);
}

void CalcMgr::OnCompareEnergies(wxCommandEvent&)
{
  getSelection();
  CompareEnergiesDialog *dialog = newCompareEnergiesDialog(p_currentSelection);
  dialog->ShowModal();
  dialog->Destroy();
}

void CalcMgr::OnAllocations(wxCommandEvent&)
{
  //  Not modal: the answers arrive in the background.
  (new AllocationsDialog(this))->Show();
}

void CalcMgr::OnGlobalTool(wxCommandEvent& event)
{
  ResourceTool *tool =
      ResourceDescriptor::getResourceDescriptor().getTool(event.GetId());
  if (tool == (ResourceTool*)0) {
    event.Skip();
    return;
  }
  startApp(tool->getName(), 0, "");
}


/**
 *  Issue #150: nothing in the running application told a first-time user
 *  that a compute machine has to be registered before any calculation can
 *  be launched -- both GETTING_STARTED.md-only facts, discoverable to
 *  nobody who installed from a .deb.  This builds the (initially hidden)
 *  one-line notice row and inserts it into the frame's own top-level
 *  sizer, between the toolbar and the tree splitter -- non-modal, so it
 *  never blocks getting on with anything else the Organizer can already
 *  do (browsing, importing, ...).
 *
 *  Built here, not in the generated CalcMgrGUI::CreateControls(), for the
 *  same reason WxBasisTool::buildFilterControls() is hand-written (#117):
 *  a wxFormBuilder-style regeneration of the .pjd would silently discard
 *  it.
 */
void CalcMgr::buildMachineNotice()
{
  p_machineNoticePanel = new ewxPanel(this, wxID_ANY);

  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);

  ewxStaticText *text = new ewxStaticText(p_machineNoticePanel, wxID_STATIC,
      _("No compute machine is registered yet -- a calculation cannot be "
        "launched until one is."));
  row->Add(text, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 6);

  ewxButton *button = new ewxButton(p_machineNoticePanel, wxID_ANY,
                                    _("Register Machines..."));
  button->Bind(wxEVT_BUTTON, &CalcMgr::machineNoticeRegisterClickCB, this);
  row->Add(button, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 6);

  p_machineNoticePanel->SetSizer(row);

  //  Right after the toolbar (index 0), before the tree splitter -- see
  //  CalcMgrGUI::CreateControls(), which adds exactly those two, in that
  //  order, and nothing else, to p_topSizer.
  p_topSizer->Insert(1, p_machineNoticePanel, 0, wxGROW|wxALL, 0);
  p_machineNoticePanel->Show(false);
  p_topSizer->Hide(p_machineNoticePanel);
}


/**
 *  Show or hide the notice depending on whether ANY compute machine is
 *  currently known -- site-configured (the central-server / classroom
 *  case, where students never register anything themselves) or the
 *  user's own MyMachines. RefMachine::referenceNames(allMachines) is
 *  exactly CalcMgr's own idea of "is a machine registered", already used
 *  (transitively, via RunMgmt::registerLocalMachine()) at the end of
 *  Create() -- so a fresh install that failed to auto-register the local
 *  host still gets the fallback of this notice pointing at Register
 *  Machines.
 *
 *  Called after Create()'s own initial check, again whenever the
 *  Register Machines tool reports a save or delete (JMS "ecce_machreg_
 *  changed", see CalcMgrApp::msgMachRegChangedMCB()), and on the frame
 *  being reactivated -- covering both "closed Register Machines and came
 *  back" and, belt-and-suspenders, any other path that changed
 *  MyMachines without publishing that message.
 */
void CalcMgr::refreshMachineNotice()
{
  if (p_machineNoticePanel == NULL)
    return;

  vector<string> *names =
      RefMachine::referenceNames(RefMachine::allMachines);
  bool none = (names == NULL) || names->empty();
  if (names != NULL)
    delete names;

  //  Opt-in diagnostic, same pattern as ECCE_DEBUG_GEOMTRACE (#99): silent
  //  by default, and the one thing a from-scratch build/test run needs to
  //  confirm this code path without a screen to look at.
  if (getenv("ECCE_DEBUG_MACHNOTICE") != NULL)
    cerr << "[MACHNOTICE] " << (none ? "shown" : "hidden") << endl;

  bool wasShown = p_topSizer->IsShown(p_machineNoticePanel);
  if (none == wasShown)
    return;

  p_machineNoticePanel->Show(none);
  p_topSizer->Show(p_machineNoticePanel, none);
  p_topSizer->Layout();
}


void CalcMgr::machineNoticeRegisterClickCB(wxCommandEvent& event)
{
  //  Exactly OnGlobalTool()'s own launch call -- see addGlobalTools().
  startApp("MachineRegister", 0, "");
}


void CalcMgr::OnActivate(wxActivateEvent& event)
{
  if (event.GetActive())
    refreshMachineNotice();
  event.Skip();
}


void CalcMgr::getToolsMenu(wxMenu & menu, WxResourceTreeItemData * itemData)
{
  // clear given menu of any menu items
  clearMenu(menu);

  // Tools that do not depend on the selection, so they are always here
  // (issue #93). These are the two of the Gateway's five launcher icons
  // that are actually useful from the Organizer: the Machine Browser, and
  // the Periodic Table as a reference. The other three were an Organizer
  // button inside the Organizer, a Builder button whose meaning from a tree
  // selection is unclear, and the Viewer, which the selection-dependent
  // tools below already offer for a calculation.
  addGlobalTools(menu);

  ResourceType * resType = 0;
  ResourceDescriptor::RUNSTATE state = ResourceDescriptor::STATE_ILLEGAL;

  vector<ResourceTool *> toolVec;
  if (getTypeAndState(itemData, resType, state)) {
    toolVec = resType->getStateBased() ? resType->getTools(state)
                                       : resType->getTools();
  }

  // With nothing applicable selected, offer an empty Builder: the same
  // entry as File > New Structure, so it needs no handler of its own.
  if (toolVec.empty()) {
    menu.Append(wxID_NEWSTRUCTURE, _("Builder"), _T(""), wxITEM_NORMAL);
    return;
  }

  // Say which item the tools below act on.
  if (itemData == 0) itemData = p_treeCtrl->getSelection();
  if (itemData) {
    wxMenuItem *head = menu.Append(wxID_ANY, wxString::Format(_("For %s:"),
        p_treeCtrl->GetItemText(itemData->GetId())));
    head->Enable(false);
  }

  vector<ResourceTool *>::iterator toolItor = toolVec.begin();
  for (;toolItor != toolVec.end(); toolItor++) {
    // The MD editors are registered once, as NWChem's, and also edit the
    // GROMACS tasks: name the code of the task (as the summary buttons do).
    wxString label = wxString::FromUTF8((*toolItor)->getMenuItem().c_str());
    if (label.StartsWith("NWChem MD ") && itemData && itemData->getResource()) {
      string app = itemData->getResource()->getProp(
                       VDoc::getEcceNamespace() + ":application");
      if (!app.empty() && app.compare(0, 6, "NWChem") != 0)
        label = wxString::FromUTF8(app.c_str()) + label.Mid(6);
    }
    wxMenuItem *item = new wxMenuItem(&menu, (*toolItor)->getId(),
            label, _T(""), wxITEM_NORMAL);
    item->SetBitmap(wxBitmap(ewxBitmap((*toolItor)->getIcon())
            .ConvertToImage().Scale(16,16)));
    menu.Append(item);
  }
}


/**
 * Clears the given menu of all of its menu items.
 */
void CalcMgr::clearMenu(wxMenu & menu)
{
  // By position, not wxMenuItemList::Node, which an STL-based wx lacks.
  while (menu.GetMenuItemCount() > 0) {
    wxMenuItem *item = menu.FindItemByPosition(0);
    if (item->GetSubMenu()) clearMenu(*item->GetSubMenu());
    menu.Delete(item);
  }
}


/**
 * Debug util: Dump clipboard content
 */
void CalcMgr::dumpClipboard()
{
  cerr << "Clipboard size: " << p_clipboard.size() << endl;
  
  vector<EcceURL>::iterator itor = p_clipboard.begin();
  for (; itor != p_clipboard.end(); itor++) {
    cerr << "\t" << itor->toString() << endl;
  }
}


/**
 * Check if the string is a valid name.
 * Rules here may be changed if there are better reasons.
 */
bool CalcMgr::isValidName(const string& name)
{
  for (int i=0; i<name.size(); i++) {
    char ch = name[i];
    if (!(ch == '.' || ch == '_' ||
          (ch >= 'a' && ch <= 'z') ||
          (ch >= 'A' && ch <= 'Z') ||
          (ch >= '0' && ch <= '9'))) {
      return false;
    }
  }
  return true;
}


/**
 * @todo will need this function to provide info about whether the
 * node can be moved away from current position.
 */
bool CalcMgr::checkMovable(WxResourceTreeItemData * node)
{
  return true;
}


/**
 * Invoke tool application by sending message.
 */
void CalcMgr::startApp(const string& app, int force, const string& url,
                       const string& codename, int debug)
{
  //  cerr << "\nInvoke " << app << " with url " << url << endl;
  // Send the start app request to gateway:
  Target gateway(GATEWAY, "");

  // Test hook: report the launch and, with ..._NOLAUNCH, do not send it.
  if (getenv("ECCE_TEST_ORGANIZER")) {
    cerr << "ECCE_TEST_ORGANIZER: startApp " << app << " force=" << force
         << " url=" << url << endl;
    if (getenv("ECCE_TEST_ORGANIZER_NOLAUNCH")) return;
  }

  JMSMessage *msg = newMessage();
  msg->addProperty("action","start");
  publish("ecce_activity",*msg);
  delete msg;

  msg = newMessage(gateway);
  msg->addProperty("appname",app);
  msg->addIntProperty("forcenew",force);
  msg->addProperty("calcurl",url);
  if (codename != "") {
    msg->addProperty("codename", codename);
    msg->addIntProperty("debug",debug);
  }
  publish("ecce_get_app",*msg);
  delete msg;
}


/**
 * Gets existing text property from selected url and sends to editing session.
 * It's used for annotations but could be used for any property the user might
 * edit as text.
 * The property name and url are combined to form a key that is then used when 
 * the callback that signals the editing is done is invoked.
 */
void CalcMgr::startPropertyEdit(const string& property)
{
  WxResourceTreeItemData * itemData = 0;
  if (getSelection()) {
    itemData = findNode(p_currentSelection[0], false, true);
  }
  else {
    // @todo Do we allow tools invoke in multi selection mode?
  }

  if (itemData != 0) {
    string text = itemData->getResource()->
      getProp(VDoc::getEcceNamespace() + ":" + property);
    EcceURL url = itemData->getUrl();
    WxEditSessionMgr sessionMgr;
    bool readonly = false;
    string id = property + "@" + url.toString();
    sessionMgr.edit(text, id, this, readonly, url.toString());
  }
}


/**
 * Tell the world a property has change?
 */
void CalcMgr::notifyProperty(const string& url, const string& name,
                             const string& value) const
{
  JMSMessage *msg = newMessage();
  msg->addProperty("url",url);
  msg->addProperty("name",name);
  msg->addProperty("value",value);
  publish("ecce_url_property",*msg);
  delete msg;
}


/**
 * Tell listeners that an object has been created.
 */
void CalcMgr::notifyCreate(const string& newurl) const
{
  JMSMessage *msg = newMessage();
  msg->addProperty("url",newurl);
  publish("ecce_url_created",*msg);
  delete msg;
}


/**
 * Tell listeners that an object has been removed.
 */
void CalcMgr::notifyRemove(const string& url,
                           const string& contentType) const
{
  JMSMessage *msg = newMessage();
  msg->addProperty("url",url);
  msg->addProperty("contenttype",contentType);
  publish("ecce_url_removed",*msg);
  delete msg;
}


/**
 * Tell listerners that an object has been renamed.
 */
void CalcMgr::notifyRename(const string& oldurl, const string& newurl,
                           const string& contentType) const
{
  JMSMessage *msg = newMessage();
  msg->addProperty("oldurl",oldurl);
  msg->addProperty("newurl",newurl);
  msg->addProperty("contenttype",contentType);
  publish("ecce_url_renamed",*msg);
  delete msg;
}


//##############################################################################
//#################### BEGIN RUN MANAGEMENT CALLBACKS ##########################
//##############################################################################

/**
 * Run management CCB.
 */
void CalcMgr::tailOutputFile()
{
  getSelection();
  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    tailOutputFile(findNode(*itor, false, true));
  }
}


/**
 * Run management CCB.
 */
void CalcMgr::shellInRunDir()
{
  getSelection();
  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    shellInRunDir(findNode(*itor, false, true));
  }
}


/**
 * Run management CCB.
 */
void CalcMgr::manageFiles()
{
  getSelection();
  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    manageFiles(findNode(*itor, false, true));
  }
}


/**
 * Run management CCB.
 */
void CalcMgr::terminateCalc()
{
  bool doit = true;

  // get the selected tree or panel item(s), remember how many selected
  bool single = getSelection();

  string msg = "Are you sure you want to terminate ";
  msg += single ? "this job?" : "these jobs?";

  ewxMessageDialog msgDlg(this, msg, "Terminate?", 
                          wxOK | wxCANCEL | wxICON_QUESTION, 
                          wxDefaultPosition);

  if(msgDlg.ShowModal() == wxID_CANCEL)
      {
        doit = false;
      }

  if (doit) {
    vector<EcceURL>::iterator itor = p_currentSelection.begin();
    for (; itor != p_currentSelection.end(); itor++) {
      terminateCalc(findNode(*itor, false, true));
      updateUrl(*itor);
    }
  }
}


/**
 * Run management CCB.
 * Only allows single selection.
 */
void CalcMgr::checkJobOnMachine()
{
  if (getSelection())
    checkJobOnMachine(findNode(p_currentSelection[0], false, true));
  else
    setMessage("Multiple selections not allowed for the machine check.");
}


/**
 * Run management CCB.
 * Only allows single selection.
 */
void CalcMgr::checkJob()
{
  // get the selected tree or panel item(s), remember how many selected
  bool single = getSelection();

  if (single) {
    checkJob(findNode(p_currentSelection[0], false, true));
  } else {
    setMessage("Multiple selections not allowed for job monitoring check.");
  }
}


/**
 * RunManagement CCB reconnect job.
 */
void CalcMgr::reconnectJob()
{
  getSelection();
  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    reconnectJob(findNode(*itor, false, true));
    updateUrl(*itor);
  }
}


/**
 * Run management CCB.
 */
void CalcMgr::resetForRerun()
{
  // Imported results are often the only copy and a reset deletes them, so
  // those calculations are skipped, with a message saying why.
  bool single = getSelection();
  vector<EcceURL> selections, skipped;
  for (vector<EcceURL>::iterator itor = p_currentSelection.begin();
       itor != p_currentSelection.end(); itor++) {
    WxResourceTreeItemData *item = findNode(*itor, false, true);
    TaskJob *task = item ? dynamic_cast<TaskJob*>(item->getResource()) : 0;
    if (task && task->getState() == ResourceDescriptor::STATE_LOADED)
      skipped.push_back(*itor);
    else
      selections.push_back(*itor);
  }

  if (!skipped.empty()) {
    char buf[64];
    string msg = "Imported results cannot be reset, because that would "
                 "delete them. Use Run Mgmt > Duplicate for Rerun to run "
                 "the calculation again.";
    if (!selections.empty()) {
      snprintf(buf, sizeof(buf), " %d imported calculation%s skipped.",
               (int)skipped.size(), skipped.size() == 1 ? " was" : "s were");
      msg += buf;
    }
    if (getenv("ECCE_TEST_RESETIMPORTED")) {
      fprintf(stderr, "ECCE_TEST_RESETIMPORTED: refused %d: %s\n",
              (int)skipped.size(), msg.c_str());
    } else {
      ewxMessageDialog dlg(this, msg, "Reset not allowed",
                           wxOK|wxICON_INFORMATION, wxDefaultPosition);
      dlg.ShowModal();
    }
  }
  if (selections.empty()) return;

  bool doit = true;
  if (GetMenuBar()->IsChecked(wxID_CONFIRM_RESET)) {
    single = selections.size() == 1;
  
    string msg = "Resetting to rerun ";
    msg += single ? "this calculation " : "these calculations ";
    msg += "will remove all existing output properties and change the ";
    msg += single ? "state " : "states ";
    msg += "to ready, so that the ";
    msg += single ? "job can " : "jobs can ";
    msg += "be rerun from the start.\n\n";
    msg += "Continue?";

    string title = "Reset ";
    title += single ? "Calculation?" : "Calculations?";

    ewxMessageDialog msgDlg(this, msg, title, wxOK | wxCANCEL | wxICON_QUESTION,
                            wxDefaultPosition);

    if (msgDlg.ShowModal() == wxID_CANCEL) {
      doit = false;
    }
  }

  if (doit) {
    vector<EcceURL>::iterator itor = selections.begin();
    for (; itor != selections.end(); itor++) {
      resetForRerun(findNode(*itor, false, true));
    }
  }
}


/**
 * Run management CCB.
 */
void CalcMgr::resetForRestart()
{
  bool doit = true;
  
  if (GetMenuBar()->IsChecked(wxID_CONFIRM_RESET)) {
    // get the selected tree or panel item(s), remember how many selected
    bool single = getSelection();

    string msg = "Resetting for restart will change the ";
    msg += single ? "state " : "states ";
    msg += "for ";
    msg += single ? "this calculation ": "these calculations ";
    msg += "back to ready allowing you to edit the input ";
    msg += single ? "file " : "files ";
    msg += "and specify any directives needed to restart the ";
    msg += single ? "job from where it " : "jobs from where they ";
    msg += "left off.  ";
    msg += "Existing properties will be retained and new output will be "
           "appended to the existing properties.  Continue?";

    ewxMessageDialog msgDlg(this, msg, "Reset?", wxOK|wxCANCEL|wxICON_QUESTION,
                            wxDefaultPosition);
    if (msgDlg.ShowModal() == wxID_CANCEL) {
      doit = false;
    }
  }

  if (doit) {
    vector<EcceURL>::iterator itor = p_currentSelection.begin();
    for (; itor != p_currentSelection.end(); itor++) {
      resetForRestart(findNode(*itor, false, true));
    }
  }
}


/**
 * Run managment CCB.
 */
void CalcMgr::resetCompleted()
{
  bool doit = true;

  if (GetMenuBar()->IsChecked(wxID_CONFIRM_RESET)) {
    // get the selected tree or panel item(s), remember how many selected
    bool single = getSelection();

    string msg = "Are you sure you want to change the ";
    msg += single ? "state " : "states ";
    msg += "for ";
    msg += single ? "this calculation " : "these calculations ";
    msg += "to completed?";

    ewxMessageDialog msgDlg(this, msg, "Reset?", wxOK|wxCANCEL|wxICON_QUESTION,
                            wxDefaultPosition);
    if (msgDlg.ShowModal() == wxID_CANCEL) {
      doit = false;
    }
  }

  if (doit) {
    vector<EcceURL>::iterator itor = p_currentSelection.begin();
    for (; itor != p_currentSelection.end(); itor++) {
      resetCompleted(findNode(*itor, false, true));
    }
  }
}


/**
 * Run managment CCB.
 */
void CalcMgr::forceFail()
{
  bool doit = true;

  if (GetMenuBar()->IsChecked(wxID_CONFIRM_RESET)) {
    // get the selected tree or panel item(s), remember how many selected
    bool single = getSelection();

    string msg = "Are you sure you want to force ";
    msg += single ? "this calculation " : "these calculations ";
    msg += "to a failed state?";

    ewxMessageDialog msgDlg(this, msg, "Reset?", wxOK|wxCANCEL|wxICON_QUESTION,
                            wxDefaultPosition);
    if (msgDlg.ShowModal() == wxID_CANCEL) {
      doit = false;
    }
  }

  if (doit) {
    getSelection();
    vector<EcceURL>::iterator itor = p_currentSelection.begin();
    for (; itor != p_currentSelection.end(); itor++) {
      forceFail(findNode(*itor, false, true));
    }
  }
}


/**
 * Run managment CCB.
 */
void CalcMgr::resetIncomplete()
{
  bool doit = true;

  if (GetMenuBar()->IsChecked(wxID_CONFIRM_RESET)) {
    // get the selected tree or panel item(s), remember how many selected
    bool single = getSelection();

    string msg = "Are you sure you want to change the ";
    msg += single ? "state " : "states ";
    msg += "for ";
    msg += single ? "this calculation " : "these calculations ";
    msg += "to incomplete?";

    ewxMessageDialog msgDlg(this, msg, "Reset?", wxOK|wxCANCEL|wxICON_QUESTION,
                            wxDefaultPosition);
    if (msgDlg.ShowModal() == wxID_CANCEL) {
      doit = false;
    }
  }

  if (doit) {
    getSelection();
    vector<EcceURL>::iterator itor = p_currentSelection.begin();
    for (; itor != p_currentSelection.end(); itor++) {
      resetIncomplete(findNode(*itor, false, true));
    }
  }
}


/**
 * Run managment CCB.
 */
void CalcMgr::resetImported()
{
  bool doit = true;

  if (GetMenuBar()->IsChecked(wxID_CONFIRM_RESET)) {
    // get the selected tree or panel item(s), remember how many selected
    bool single = getSelection();

    string msg = "Are you sure you want to change the ";
    msg += single ? "state " : "states ";
    msg += "for ";
    msg += single ? "this calculation " : "these calculations ";
    msg += "to imported?";

    ewxMessageDialog msgDlg(this, msg, "Reset?", wxOK|wxCANCEL|wxICON_QUESTION,
                            wxDefaultPosition);
    if (msgDlg.ShowModal() == wxID_CANCEL) {
      doit = false;
    }
  }

  if (doit) {
    getSelection();
    vector<EcceURL>::iterator itor = p_currentSelection.begin();
    for (; itor != p_currentSelection.end(); itor++) {
      resetImported(findNode(*itor, false, true));
    }
  }
}


/**
 * Run managment CCB.
 */
void CalcMgr::resetKilled()
{
  bool doit = true;

  if (GetMenuBar()->IsChecked(wxID_CONFIRM_RESET)) {
    // get the selected tree or panel item(s), remember how many selected
    bool single = getSelection();

    string msg = "Are you sure you want to change the ";
    msg += single ? "state " : "states ";
    msg += "for ";
    msg += single ? "this calculation " : "these calculations ";
    msg += "to killed?";

    ewxMessageDialog msgDlg(this, msg, "Reset?", wxOK|wxCANCEL|wxICON_QUESTION,
                            wxDefaultPosition);
    if (msgDlg.ShowModal() == wxID_CANCEL) {
      doit = false;
    }
  }

  if (doit) {
    getSelection();
    vector<EcceURL>::iterator itor = p_currentSelection.begin();
    for (; itor != p_currentSelection.end(); itor++) {
      resetKilled(findNode(*itor, false, true));
    }
  }
}


/**
 * Run managment CCB.
 */
void CalcMgr::markReviewed()
{
  getSelection();
  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    markReviewed(findNode(*itor, false, true));
    updateUrl(*itor);
  }
}


/**
 * Run managment CCB.
 */
void CalcMgr::markUnreviewed()
{
  getSelection();
  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    markUnreviewed(findNode(*itor, false, true));
    updateUrl(*itor);
  }
}


/**
 * Run managment CCB.
 */
void CalcMgr::cleanRunDir()
{
  getSelection();
  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    cleanRunDir(findNode(*itor, false, true));
    updateUrl(*itor);
  }
}


/**
 * Run managment CCB.
 */
void CalcMgr::viewRunLog()
{
  getSelection();
  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    viewRunLog(findNode(*itor, false, true));
  }
}


/**
 * Run managment CCB.
 */
void CalcMgr::viewOutputFile()
{
  getSelection();
  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    viewOutputFile(findNode(*itor, false, true));
  }
}


/**
 * Run management CCB.
 */
void CalcMgr::viewInputFile()
{
  getSelection();
  vector<EcceURL>::iterator itor = p_currentSelection.begin();
  for (; itor != p_currentSelection.end(); itor++) {
    viewInputFile(findNode(*itor, false, true));
  }
}

//##############################################################################
//################## END RUN MANAGEMENT CALLBACKS ##############################
//##############################################################################

//##############################################################################
//################## BEGIN RUN MANAGEMENT METHODS ##############################
//##############################################################################

/**
 * Follows the calculation's primary output file in a Tail window, over the
 * login ECCE already uses for the machine.  The TailInTerminal preference
 * (or ECCE_TAIL_TERMINAL=1) runs tail -f in the Terminal instead.
 *
 * @param itemData  the tree node containing the calculation to work on
 */
void CalcMgr::tailOutputFile(WxResourceTreeItemData *itemData)
{
  // Get TaskJob
  TaskJob *calc = dynamic_cast<TaskJob*>(itemData->getResource());
  NULLPOINTEREXCEPTION(calc,"Could not cast Resource to TaskJob.");

  const JCode *code = calc->application();
  if (!code || code->name() == "") {
    setMessage("No code information available.");
    return;
  }

  const Launchdata& launch = calc->launchdata();
  RefMachine* refMachine = RefMachine::refLookup(launch.machine);
  if (!refMachine) {
    // A job's persisted launch.machine can legitimately no longer match
    // any currently-registered machine (e.g. after a machine rename) --
    // refLookup() returns null by design in that case, not as a bug.
    // See RunMgmt::terminate() for the same pattern.
    setMessage("No job information available--machine \"" + launch.machine +
               "\" is not currently registered.");
    return;
  }

  Jobdata job = calc->jobdata();
  if (job.jobpath.empty()) {
    setMessage("No job information available.");
    return;
  }

  TypedFile tfile;
  calc->getDataFile(JCode::PRIMARY_OUTPUT, tfile);
  const string file = job.jobpath + "/" + tfile.name();
  string msg = openTail(calc->getName(), refMachine->fullname(),
                        launch.remoteShell, launch.user, file);
  if (msg != "") setMessage(msg, WxFeedback::ERROR);
}


/**
 * The Tail window for file on machine, or the Terminal when so configured.
 * Returns an error message, "" on success.
 */
string CalcMgr::openTail(const string& calcName, const string& machine,
                         const string& shell, const string& user,
                         const string& file)
{
  bool inTerminal = false;
  const char *env = getenv("ECCE_TAIL_TERMINAL");
  if (env && *env) {
    inTerminal = string(env) != "0";
  } else {
    Preferences pref(PrefLabels::GLOBALPREFFILE);
    pref.getBool(PrefLabels::TAILINTERMINAL, inTerminal);
  }

  if (inTerminal) {
    setMessage("Starting a terminal with tail -f.  Use the "
               "window manager menu to exit.", WxFeedback::INFO);
    EcceShell eshell;
    string cmd = "tail -f " + EcceShell::shellQuote(file);
    string msg = eshell.cmdshell(calcName, machine, shell, user, "", cmd, file);
    if (msg != "" && eshell.lastStatus() != -1) {
      setMessage(msg, WxFeedback::WARNING);
      return "";
    }
    return msg;
  }

  string error;
  wxBusyCursor busy;
  WxTailWindow *w = WxTailWindow::open(this, calcName, machine, shell, user,
                                       file, error);
  return w ? string("") : error;
}


/**
 * Opens a shell in the calculation run directory.
 *
 * @param itemData  the tree node containing the calculation to open the run
 *                  directory of
 */
void CalcMgr::shellInRunDir(WxResourceTreeItemData *itemData)
{
  string appName, calcName, user, shell, pathBase, pathTail;
  
  // Get TaskJob
  TaskJob *calc = dynamic_cast<TaskJob*>(itemData->getResource());
  NULLPOINTEREXCEPTION(calc, "Could not cast Resource to TaskJob.");

  calcName = calc->getName();
  const Launchdata& launch = calc->launchdata();
  shell = launch.remoteShell;
  user = launch.user;

  RefMachine* refMachine = RefMachine::refLookup(launch.machine);
  if (!refMachine) {
    // See tailOutputFile()/RunMgmt::terminate() for why this is a real,
    // reachable case (e.g. a machine renamed after this job was launched)
    // and not just defensive padding.
    setMessage("Unable to open shell--machine \"" + launch.machine +
               "\" is not currently registered.");
    return;
  }

  pathBase = launch.rundir;

  Jobdata job = calc->jobdata();
  pathTail = job.jobpath;
  if (!pathTail.empty()) {
    EcceShell eshell;
    string msg = eshell.dirshell(calcName, refMachine->fullname(),
                                 shell, user, "", pathBase, pathTail);
    if (msg != "") {
      setMessage(msg, (eshell.lastStatus()==-1) ?
                 WxFeedback::ERROR : WxFeedback::WARNING);
    }
  } else {
    setMessage("No job information available.");
  }

}


/**
 * Opens a dialog in which the user can relocate files from the calculation 
 * directory to the server and vice versa.  Not currently implemented.
 *
 * @param itemData  tree node containing the calculation to manage the files of
 */
void CalcMgr::manageFiles(WxResourceTreeItemData *itemData)
{
  /*
    if (p_filelist == 0) {
    p_filelist = new FileList("Manage Files");
    p_filelist->initialize(getExplorer());
    }

    TNode *node = 0;
    XrtNodeAbstraction *treenode = 0;
    if (getFirstSelection(&treenode, &node)) {

    EcceURL url = node->getUrl();

    ICalculation *calc = 
    dynamic_cast<ICalculation*>(EDSIFactory::getResource(url));

    p_filelist->pupup(XtGrabNone, calc);
    }
  */
  setMessage("Not implemented in this version.");
}


/**
 * Kills the calculation resource in the given tree node.
 *
 * @param itemData  the tree node containing the calculation to terminate
 */
void CalcMgr::terminateCalc(WxResourceTreeItemData *itemData)
{
  // Get URL
  EcceURL url = itemData->getUrl();

  // Get TaskJob
  TaskJob *calc = dynamic_cast<TaskJob*>(itemData->getResource());
  NULLPOINTEREXCEPTION(calc, "Could not cast from Resource to TaskJob.");

  Jobdata const job = calc->jobdata();
  if (job.jobid.empty()) {
    // this must be an import so send kill message to jobstore
    JMSMessage *msg = newMessage();
    msg->addProperty("url", url.toString());
    publish("ecce_ejs_kill", *msg);
    delete msg;
  } else {
    string msg;
    msg = RunMgmt::terminate(calc);
    setMessage(msg, WxFeedback::INFO);
  }
}


/**
 * Checks to see if the job monitoring is running.
 *
 * @param itemData  the tree node containing the job to check monitoring on
 */
void CalcMgr::checkJob(WxResourceTreeItemData *itemData)
{
  string message;
  if (checkSingleJob(itemData->getResource(), message)) {
    message = "Job monitoring running for " + itemData->getName();
    setMessage(message, WxFeedback::INFO);
  } else if (message != "") {
    setMessage(message, WxFeedback::WARNING);
  }
}


/**
 * Asks the compute machine, over ECCE's own login, whether the job's process
 * or queue job is still there and when its output was last written (#284).
 * Only reports: the job monitor (or Reconnect) still decides the state.
 */

void CalcMgr::checkJobOnMachine(WxResourceTreeItemData *itemData)
{
  TaskJob *calc = itemData ? dynamic_cast<TaskJob*>(itemData->getResource()) : 0;
  if (!calc) return;
  vector<string> lines;
  {
    wxBusyCursor busy;
    lines = RunMgmt::checkOnMachine(calc).describe();
  }
  time_t now = time(0);
  char at[32];
  strftime(at, sizeof at, "%H:%M", localtime(&now));
  lines.insert(lines.begin(), string("Checked at ") + at + ":");
  p_machineCheck[progressKey(itemData->getUrl())] = lines;
  for (size_t i = 1; i < lines.size(); i++)
    setMessage(itemData->getName() + ": " + lines[i], WxFeedback::INFO);
  contactChanged(itemData->getUrl());
}


/**
 * Session start (#208): catch up the calculations whose monitoring stopped
 * while ECCE was away, once per session.
 */
// The old job store holds the job's WaitingJobs lock for as long as it runs.
// A new store started before it is gone would meet its cleanup in the run
// directory, so wait for the lock to go (at most 15 s).
bool CalcMgr::stopOldMonitor(const EcceURL& url)
{
  const string key = url.toString();
  bool noted = false;
  for (int i = 0; i < 150 && WaitingJobs::watched(key); i++) {
    if (i == 10 && !noted) {
      setMessage("Waiting for the previous monitor to stop...",
                 WxFeedback::INFO);
      noted = true;
    }
    wxYield();
    wxMilliSleep(100);
  }
  return !WaitingJobs::watched(key);
}


void CalcMgr::catchUpWaitingJobs()
{
  if (WaitingJobs::list().empty() ||
      !WaitingJobs::firstInSession(Ecce::sessionKey()))
    return;

  wxBusyCursor busy;
  ewxProgressDialog* progress = 0;
  vector<JobCatchUp::Result> results =
    JobCatchUp::run([&](size_t done, size_t total) {
      if (!progress)
        progress = new ewxProgressDialog("ECCE",
            "Checking calculations that finished or ran on while ECCE was "
            "away...", (int)total, this, wxPD_AUTO_HIDE);
      progress->Update((int)done);
    });
  delete progress;

  for (size_t i = 0; i < results.size(); i++) {
    setMessage(results[i].message,
               results[i].ok ? WxFeedback::INFO : WxFeedback::WARNING);
    updateUrl(EcceURL(results[i].url));
  }
}


/**
 * Reconnect to the currently selected job.  I copied this from v3.x where it 
 * in turn had been copied from "Gary's original implementation."
 *
 * @param itemData  the tree node containing the resource job to reconnect to
 */
void CalcMgr::reconnectJob(WxResourceTreeItemData *itemData)
{
  // Get URL
  EcceURL url = itemData->getUrl();

  // Get PropertyTask from Resource
  PropertyTask *calc = dynamic_cast<PropertyTask*>(itemData->getResource());
  NULLPOINTEREXCEPTION(calc,"Could not cast from Resource to PropertyTask.")

  // Get state and job id
  Jobdata job = calc->jobdata();
  ResourceDescriptor::RUNSTATE state = calc->getState();

  if (job.jobid.empty()) {
    setMessage("No job id available to reconnect", WxFeedback::ERROR);
    return;
  }

  if (state >= ResourceDescriptor::STATE_COMPLETED) {
    ewxMessageDialog msgDlg(this, "Are you sure you want to re-parse output "
                            "from a calculation that is no longer running?",
                            "Re-parse Output?", 
                            wxCANCEL|wxOK|wxICON_QUESTION, wxDefaultPosition);
    if (msgDlg.ShowModal() == wxID_CANCEL)
      return;
  } else {
    JMSMessage *msg = newMessage();
    msg->addProperty("url",url.toString());
    publish("ecce_ejs_kill",*msg);
    delete msg;
    if (!stopOldMonitor(url)) {
      p_reconnectNote = "previous monitor still running";
      setMessage(p_reconnectNote, WxFeedback::ERROR);
      return;
    }
  }

  string message;
  if (JobCatchUp::reconnect(calc, message)) {
    p_reconnectNote = "reconnected: " + message;
    setMessage(message, WxFeedback::INFO);
    // The new monitor's own silence counts from now (#284).
    p_contact.erase(progressKey(url));
    p_machineCheck.erase(progressKey(url));

    // don't issue warning for submitted jobs because it would be silly
    if (state != ResourceDescriptor::STATE_SUBMITTED)
      setMessage("It may take several minutes for previous output to be "
                 "parsed.  You may want to close any Viewers for this "
                 "calculation and restart them later as they will be "
                 "extremely slow to respond to input.", WxFeedback::WARNING);
  } else {
    p_reconnectNote = "failed: " + message;
    setMessage(message, WxFeedback::ERROR);
  }
}


/**
 * Reset a calculation to rerun, copied from v3.x.  Output files are removed.
 * The state is reset to what was given or to something appropriate.
 *
 * @param itemData  the tree node containing the calculation to reset
 * @param tostate   defaults to STATE_ILLEGAL which causes the function to 
 *                  determine a correct state to be in
 */
bool CalcMgr::resetForRerun(WxResourceTreeItemData *itemData,
                            ResourceDescriptor::RUNSTATE tostate)
{
  TaskJob *task = dynamic_cast<TaskJob*>(itemData->getResource());
  string message;
  bool ret = false;
  if (task && task->canChangeState(message)) {
    ret = task->resetForRerun(tostate);
    if (!ret) {
      setMessage(task->messages());
    }
  }
  else {
    ewxMessageDialog dlg(this, message,
                         "Reset state not allowed!", wxOK|wxICON_EXCLAMATION,
                         wxDefaultPosition);
    dlg.ShowModal();
  }
  return ret;
}


/**
 * Reset the state to ready.  Also have to set the process flag to false.
 *
 * @param itemData  the tree node containing the calculation to reset
 */
bool CalcMgr::resetForRestart(WxResourceTreeItemData *itemData)
{
  TaskJob *job = dynamic_cast<TaskJob*>(itemData->getResource());
  string message;
  bool ret = false;
  if (job && job->canChangeState(message)) {
    ret = job->resetForRestart();
    if (!ret) {
      setMessage(job->messages());
    }
  }
  else {
    ewxMessageDialog dlg(this, message,
                         "Reset state not allowed!", wxOK|wxICON_EXCLAMATION,
                         wxDefaultPosition);
    dlg.ShowModal();
  }
  return ret;
}


/**
 * Change the calculation's state to completed.
 *
 * @param itemData  the tree node containing the calculation of which to change 
 *                  state to completed
 */
void CalcMgr::resetCompleted(WxResourceTreeItemData *itemData)
{
  setState(itemData, ResourceDescriptor::STATE_COMPLETED);
}


/**
 * Change the calculation's state to incomplete.
 *
 * @param itemData  the tree node containing the calculation of which to change 
 *                  state to incomplete
 */
void CalcMgr::resetIncomplete(WxResourceTreeItemData *itemData)
{
  setState(itemData, ResourceDescriptor::STATE_UNSUCCESSFUL);
}


/**
 * Change the calculation's state to imported.
 *
 * @param itemData  the tree node containing the calculation of which to change 
 *                  state to imported
 */
void CalcMgr::resetImported(WxResourceTreeItemData *itemData)
{
  setState(itemData, ResourceDescriptor::STATE_LOADED);
}


/**
 * Change the calculation's state to killed.
 *
 * @param itemData  the tree node containing the calculation of which to change 
 *                  state to killed
 */
void CalcMgr::resetKilled(WxResourceTreeItemData *itemData)
{
  setState(itemData, ResourceDescriptor::STATE_KILLED);
}


/**
 * Force the calculation's state to system error.
 *
 * @param itemData  the tree node containing the calculation of which to change 
 *                  state to incomplete
 */
void CalcMgr::forceFail(WxResourceTreeItemData *itemData)
{
  TaskJob *job = dynamic_cast<TaskJob*>(itemData->getResource());
  (void)job->setState(ResourceDescriptor::STATE_SYSTEM_FAILURE);
}


/**
 * Set the resource's reviewed status to true.  Sets reviewed date as well.
 *
 * @param itemData  the tree node containing the resource to mark as reviewed
 */
void CalcMgr::markReviewed(WxResourceTreeItemData *itemData)
{
  setReviewed(itemData, true);
}


/**
 * Set the resource's reviewed status to false.  Removes reviewed date as well.
 *
 * @param itemData  the tree node containing the resource to mark as unreviewed
 */
void CalcMgr::markUnreviewed(WxResourceTreeItemData *itemData)
{
  setReviewed(itemData, false);
}


/**
 * Removes files from the calculation's run directory.
 *
 * @param itemData  the tree node containing the calculation from which to 
 *                  remove its run directory files
 */
void CalcMgr::cleanRunDir(WxResourceTreeItemData *itemData)
{
  // Get TaskJob
  TaskJob *taskJob = dynamic_cast<TaskJob*>(itemData->getResource());
  NULLPOINTEREXCEPTION(taskJob, "Unable to cast from Resource to TaskJob.");

  string msg;
  msg = RunMgmt::cleanServerDirs(taskJob);
  if (msg != "") {
    setMessage(msg, WxFeedback::INFO);
  }
}


/**
 * Opens a shell and displays the log of the calculation's execution.
 *
 * @param itemData  the tree node containing the calculation of which to display
 *                  its execution log
 */
void CalcMgr::viewRunLog(WxResourceTreeItemData *itemData)
{
  try {
    // Get URL
    EcceURL url = itemData->getUrl();

    // Get vdoc
    VDoc *vdoc = EDSIFactory::getVdoc(url);

    if (vdoc != 0) {
      EcceURL runlogURL = vdoc->getOutput(vdoc->getRunLogName());
      EDSI *edsi = EDSIFactory::getEDSI(runlogURL);
      ostrstream os;
      if (edsi->getDataSet(os)) {
        //        os << ends;
        WxEditSessionMgr sessionMgr;
        sessionMgr.edit(os.str(), "testing", this, true, 
                        runlogURL.toString());
      } else {
        setMessage("No run log available for the selected calculation.");
      }
      delete edsi;
      delete vdoc;
    } else {
      setMessage("Could not retrieve calculation object.");
    }
  } catch (EcceException& ex) {
    setMessage(ex.what());
  }
}


/**
 * Opens a shell and displays the data of the calculation's execution.
 *
 * @param itemData  the tree node containing the calculation of which to display
 *                  its data file
 */
void CalcMgr::viewOutputFile(WxResourceTreeItemData *itemData)
{
  try {
    // Get TaskJob
    TaskJob *calc = dynamic_cast<TaskJob*>(itemData->getResource());
    NULLPOINTEREXCEPTION(calc,"Unable to cast from Resource to TaskJob.");

    istream* is = calc->getDataFile(JCode::PRIMARY_OUTPUT);
    if (is) {
      string text;
      StringConverter::streamToText(*is, text);
      delete is;
      
      WxEditSessionMgr sessionMgr;
      sessionMgr.edit(text, "testing", this, true, "Calculation Output");
    } else {
      setMessage("Output file for calculation not found.", WxFeedback::WARNING);
    }
  } catch (EcceException& ex) {
    setMessage(ex.what());
  }
}


/**
 * Opens a shell and displays the data of the calculation's input file.
 *
 * @param itemData  the tree node containing the calculation of which to display
 *                  its input file
 */
void CalcMgr::viewInputFile(WxResourceTreeItemData *itemData)
{
  try {
    // Get TaskJob
    TaskJob *calc = dynamic_cast<TaskJob*>(itemData->getResource());
    NULLPOINTEREXCEPTION(calc,"Unable to cast from Resource to TaskJob.");

    istream* is = calc->getDataFile(JCode::PRIMARY_INPUT);
    if (is) {
      string text;
      StringConverter::streamToText(*is, text);
      delete is;
      
      WxEditSessionMgr sessionMgr;
      sessionMgr.edit(text, "testing", this, true, "Calculation Input");
    } else {
      setMessage("Input file for calculation not found.", WxFeedback::WARNING);
    }
  } catch (EcceException& ex) {
    setMessage(ex.what());
  }
}

//##############################################################################
//################## END RUN MANAGEMENT METHODS ################################
//##############################################################################

//##############################################################################
//#################### BEGIN RUN MANAGEMENT HELPER METHODS #####################
//##############################################################################

/**
 * Sets the given task's state to that given.  Returns true if successful.
 *
 * @param node  the tree node of which to set the state of its calculation
 * @state       the state to set the calculation to
 * @return      if all properties save to DAV correctly, then true
 */
bool CalcMgr::setState(WxResourceTreeItemData *node, 
                       ResourceDescriptor::RUNSTATE state)
{
  TaskJob *job = dynamic_cast<TaskJob*>(node->getResource());
  string message;
  bool ret = false;
  if (job && job->canChangeState(message)) {
    ret = job->setState(state);
    if (!ret)
      setMessage(job->messages());
  }
  else {
    ewxMessageDialog dlg(this, message,
                         "Reset state not allowed!", wxOK|wxICON_EXCLAMATION,
                         wxDefaultPosition);
    dlg.ShowModal();
  }
  return ret;
}


/**
 * Set the reviewed flag of the current selection.
 *
 * @param itemData  the tree node containing the calculation to mark
 * @param flag      if true, sets as reviewed and sets reviewed date as well
 *                  if false, sets as unreviewed and removes reviewed date
 * @return          if all props set successfully, return true
 */
bool CalcMgr::setReviewed(WxResourceTreeItemData* itemData, bool flag)
{
  Resource *resource = itemData->getResource();

  return resource->setReviewed(flag);
}


/**
 * Check job monitoring status of given node's children.
 * Used in conjunction with the run management task of the same name, or 
 * when selecting a new node.
 *
 * @param node  the tree node of which to check if job monitoring is running
 * @return       0 indicates job monitoring is working for all running jobs
 *              >0 indicates job monitoring has failed for n jobs
 */
void CalcMgr::checkNodeJobs(WxResourceTreeItemData *node)
{
  static char *doJobChecks = getenv("ECCE_JOB_CHECK");

  // Here is some special failsafe code to check if jobstore is
  // running or if we have a hanging calc state.
  // NOTE: Might want to pull in jobStoreRunning code so we use
  // only one RCommand connection.
  if (node && doJobChecks) {
    vector<Resource*> * children = node->getResource()->getChildren();
    if (children == 0 || children->size() == 0) {
      p_treeCtrl->loadChildren(node);
      children = node->getResource()->getChildren();
    }
    // Null when the data server cannot list the collection.
    int numNodes = children ? (int)children->size() : 0;

    string message;
    for (int idx = 0; idx < numNodes; idx++) {
      (void)checkSingleJob((*children)[idx], message);
    }
  }
}


bool CalcMgr::checkSingleJob(Resource *resource, string& message)
{
  int ret = true;
  message = "";

  string statestr = resource->getProp(VDoc::getEcceNamespace() + ":state");

  ResourceDescriptor::RUNSTATE state = 
    ResourceUtils::stringToState(statestr);
  if (state == ResourceDescriptor::STATE_SUBMITTED ||
      state == ResourceDescriptor::STATE_RUNNING) {
    string hostname = Host().host_name();
    string jobid = resource->getProp(VDoc::getEcceNamespace() + ":job_jobid");
    string jobstoreclient = resource->getProp(VDoc::getEcceNamespace() +
                                              ":job_clienthost");

    if (!jobstoreclient.empty() && hostname == jobstoreclient) {
      string launchMachine = resource->getProp(VDoc::getEcceNamespace() +
                       ":launch_machine");
      RefMachine* refMachine = RefMachine::refLookup(launchMachine);

      if (refMachine != (RefMachine*)0) {
        bool singleFlag = refMachine->singleConnect() ||
                       (refMachine->frontendMachine()!="" &&
                        (refMachine->frontendBypass()=="" ||
                        !RCommand::isSameDomain(refMachine->frontendBypass())));

        if (!singleFlag) {
          if (!jobid.empty() && !RunMgmt::jobStoreRunning(jobid)) {
            ret = false;

            string msg = "Job monitoring process for ";
            msg += resource->getURL().toString();
            msg += " is no longer running; the job may still be running "
                   "or finished, and is caught up by Reconnect or at the "
                   "next session start.";
            setMessage(msg, WxFeedback::WARNING);

            // Losing the monitor says nothing about the job itself (#208).
            TaskJob *calc = dynamic_cast<TaskJob*>(resource);
            NULLPOINTEREXCEPTION(calc,
                                 "Unable to cast from Resource to TaskJob.");
            WaitingJobs::add(resource->getURL().toString());
            calc->setState(ResourceDescriptor::STATE_WAITING);
            updateUrl(resource->getURL());

          }
        } else {
          ret = false;
          message = "Can't check monitoring--job launched to " +
                   launchMachine + " which requires a single remote connection";
        }
      }
    } else if (!jobstoreclient.empty() && hostname != jobstoreclient) {
      ret = false;
      message = "Can't check monitoring--job launched from " +
                jobstoreclient + " rather than " + hostname;
    }
  }
  return ret;
}

//##############################################################################
//#################### END RUN MANAGEMENT HELPER METHODS #######################
//##############################################################################


/**
 * Uploads a file to url of itemData + file's filename (and contents).
 *
 * @param parent  the tree node which will contain the uploaded file
 * @param file    the file being written to the server
 * @return        the url of where the file was written
 */
EcceURL CalcMgr::upload(WxResourceTreeItemData *parent, SFile *file)
{
  // we need to double-check that the resource is a collection and not a file
  // if not, just use the parent of the file, which is guaranteed a collection
  // this is already being done by the tree ctrl, but if this function is
  // called outside that context, we need to be safe
  ResourceDescriptor::RESOURCETYPE rt;
  rt = parent->getResource()->getResourceType();
  if (rt == ResourceDescriptor::RT_DOCUMENT) {
    parent = p_treeCtrl->getParent(parent);
  }

  // construct a url based on the parent url + filename
  EcceURL url = parent->getUrl();
  url = url.getChild(file->filename());

  // get the edsi and set it to our soon-to-upload url
  EDSI *edsi = EDSIFactory::getEDSI(url);

  // check to see if this resource exists on the server already
  if (edsi->exists()) {
    // since a file with that name already exists, we'll change the name of
    // the file being uploaded for the user to something uniqe
    url = url.getParent();
    edsi->setURL(url);
    string newName = edsi->uniqueName(file->filename());
    url = url.getChild(newName);
    edsi->setURL(url);
  }
  
  ifstream is(file->path().c_str());
  if (! edsi->putDataSet(is)) {
    setMessage(edsi->m_msgStack.getMessage());
    url = EcceURL();
  }
  is.close();

  return url;
}


/**
 * Downloads a file to url of SFile (directory) plus name of target itemData.
 * Remembers where the last file was saved for ease of use.  This download is
 * currently not recursive, meaning it only works on single resources (not 
 * collections).
 *
 * @param itemData  the tree node containing the resource to download
 */
void CalcMgr::download(WxResourceTreeItemData *itemData)
{
  // get last download directory as preference
  Preferences prefs("Organizer");

  string dir;
  prefs.getString("Organizer.DownloadDir", dir);

  if (dir != "") {
    SFile dirfile(dir);
    if (!dirfile.exists()) dir = Ecce::realUserHome();
  } else {
    dir = Ecce::realUserHome();
  }

  ewxFileDialog *dlg = new ewxFileDialog(this, "Select download destination", 
                                         dir, 
                                         itemData->getName(), "*.*", 
                                         wxFD_SAVE|wxFD_OVERWRITE_PROMPT, 
                                         wxDefaultPosition);
  dlg->SetFilterIndex(1);
  if (dlg->ShowModal() == wxID_OK) {
    // make file from url of current directory plus filename (GetPath)
    string path = dlg->GetPath().ToStdString();
    SFile *file = new SFile(path);

    itemData->getResource()->getDocument(file);

    // clean up SFile pointer before leaving scope
    delete file;
  }

  // save last directory to calcmgr preferences
  string directory = dlg->GetDirectory().ToStdString();
  prefs.setString("Organizer.DownloadDir", directory);
  prefs.saveFile();

  dlg->Destroy();
}


/**
 * For each theory/runtype combination, a new calculation is created from a copy
 * of the given WxResourceTreeItemData so long as it is an ES calculation.
 * These copies are then placed into a folder at the same level and with the
 * same name as the given WxResourceTreeItemData.
 *
 * @param itemData  the tree node containing the calculation to generate on
 */
void CalcMgr::createTestSuite(WxResourceTreeItemData *itemData)
{
  // if any part of this process fails for any reason, this will help control
  // the flow of the method
  bool status = true;

  // the itemData will need to be refreshed periodically so we need to save its
  // URL for now
  EcceURL itemDataURL = itemData->getUrl();

  // only make the test suite if its a calculation
  if (itemData->getResource()->getContentType() == 
      ResourceDescriptor::CT_CALCULATION) {

    // get the parent of the current selection
    WxResourceTreeItemData *parent = p_treeCtrl->getParent(itemData);
    if (parent == (WxResourceTreeItemData*)0) {
      status = false;
      cerr << "Could not open the parent folder of this calculation." << endl;
    }

    // create the new project folder for the TestSuite
    EcceURL testSuiteURL;
    WxResourceTreeItemData *tsProject;
    if (status) {
      string name = itemData->getName();
      name += "_TestSuite";
      ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
      ResourceType *rt = rd.getResourceType( ResourceDescriptor::RT_COLLECTION, 
                                             ResourceDescriptor::CT_PROJECT,
                                             ResourceDescriptor::AT_UNDEFINED);
      Resource * tmp = parent->getResource()->createChild(name, rt);

      if (tmp == 0) {
        status = false;
        cerr << "Could not create TestSuite project folder." << endl;
      } else {
        testSuiteURL = tmp->getURL();
        p_treeCtrl->refresh(parent);
      }
    }

    // the parent of itemData was refreshed, which means the tree item that
    // itemData points to is no longer valid as well as tsProject
    if (status) {
      // refresh itemData
      itemData = findNode(itemDataURL, false, true);
      if (itemData == (WxResourceTreeItemData*)0) {
        status = false;
        cerr << "Could not relocate calculation selection after refresh." << endl;
      }
      // refresh testSuite
      tsProject = findNode(testSuiteURL, false, true);
      if (tsProject == (WxResourceTreeItemData*)0) {
        status = false;
        cerr << "Could not relocate test suite project after refresh." << endl;
      }
    }

    // duplicate the calculation and place it in the parent 
    // however, do not automatically select the item in the tree when created
    WxResourceTreeItemData *escalcItemData;
    if (status) {
      escalcItemData = duplicate(itemData, false);
      if (escalcItemData == (WxResourceTreeItemData*)0) {
        status = false;
        cerr << "Copy of selected calculation failed." << endl;
      }
      // duplicate(...) causes a parent refresh, so again we must update any 
      // pointers of tree items or resources
      // refresh itemData
      itemData = findNode(itemDataURL, false, true);
      if (itemData == (WxResourceTreeItemData*)0) {
        status = false;
        cerr << "Could not relocate calculation selection after refresh." << endl;
      }
      // refresh testSuite
      tsProject = findNode(testSuiteURL, false, true);
      if (tsProject == (WxResourceTreeItemData*)0) {
        status = false;
        cerr << "Could not relocate test suite project after refresh." << endl;
      }
    }

    // we're going to need the resource of the duplicated calculation as an
    // ICalculation rather than a resource
    ICalculation *escalc;
    if (status) {
      escalc = dynamic_cast<ICalculation*>(escalcItemData->getResource());
      if (escalc == (ICalculation*)0) {
        status = false;
        cerr << "Selection was not a calculation." << endl;
      }
    }

    // now clean up the duplicated calc to use as a template for the others
    if (status) {
      resetForRerun(escalcItemData);
      if (! escalc->removeInputFiles()) {
        status = false;
        cerr << "Failed to remove input files of original calculation." << endl;
      }
      // resetForRerun refreshes the entire tree... once again, refresh pointers
      // refresh itemData
      itemData = findNode(itemDataURL, false, true);
      if (itemData == (WxResourceTreeItemData*)0) {
        status = false;
        cerr << "Could not relocate calculation selection after refresh." << endl;
      }
      // refresh testSuite
      tsProject = findNode(testSuiteURL, false, true);
      if (tsProject == (WxResourceTreeItemData*)0) {
        status = false;
        cerr << "Could not relocate test suite project after refresh." << endl;
      }
    }

    // as part of the cleanup, set an empty set of GUI values
    if (status) {
      GUIValues *emptyguivalues = new GUIValues();
      if (emptyguivalues == (GUIValues*)0) {
        status = false;
        cerr << "Failed to reset GUIValues." << endl;
      } else {
        escalc->guiparams(emptyguivalues);
      }
      delete emptyguivalues;
    }

    // Get fragment so we can get the numElectrons
    int numElectrons = 0;
    if (status) {
      Fragment *frag = escalc->fragment();
      if (frag) {
        numElectrons = frag->numElectrons();
      } else {
        status = false;
        cerr << "Calculation did not contain a fragment." << endl;
      }
      delete frag;
    }

    // We'll need the spin multiplicty as well
    SpinMult::SpinMultEnum origspin;
    if (status) {
      origspin = escalc->spinMultiplicity();
    }

    // at this point we have a clean calc that we can use as the basis for our
    // test suite so long as the status is still okay
 
    // next get the code so we can figure out what we all have to create
    const JCode *jcode;
    if (status) {
      jcode = escalc->application();
      if (jcode == (JCode*)0) {
        status = false;
        cerr << "Calculation did not have an application code set." << endl;
      }
    }

    // make a subdir for each runtype
    if (status) {
      vector<TRunType> *allRunTypes = jcode->runTypes();
  
      ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
      ResourceType *rt = rd.getResourceType( ResourceDescriptor::RT_COLLECTION, 
                                             ResourceDescriptor::CT_PROJECT,
                                             ResourceDescriptor::AT_UNDEFINED );

      int rtit;
      for (rtit = allRunTypes->size()-1; rtit >= 0; rtit--) {
        string name = (*allRunTypes)[rtit].name();
        status &= (tsProject->getResource()->createChild(name, rt) != 0);
        if (!status) { break; }
      }
      delete allRunTypes;
    }
  
    // for each run type, create one of each theory and place them in the 
    // folders we just created
    if (status) {
      ICalculation *newcalc = 0;
      SpinMult::SpinMultEnum newspin = origspin;
      
      vector<TTheory> *theories = jcode->theories();
      int numTheories = theories->size();
      string tname;

      int idx;
      for (idx = 0; idx < numTheories; idx++) {
        vector<TRunType> *runtypes = jcode->runTypes(&(*theories)[idx]);
        int numRunTypes = runtypes->size();
        tname = (*theories)[idx].category();
        tname += "-";
        tname += (*theories)[idx].name();
        for (string::iterator it = tname.begin(); it != tname.end(); it++) {
          if ((*it) == '/' || (*it) == '(' || (*it) == ')') *it = '-';
        }

        EcceURL cpurl;

        string theoryName = (*theories)[idx].name();
        int jdx;
        for (jdx = 0; jdx < numRunTypes; jdx++) {
          cpurl = testSuiteURL.getChild((*runtypes)[jdx].name());
          cpurl = cpurl.getChild(tname);

          // at this point we have a url under the test suite project,
          // under the appropriate theory folder, with a theory/type name

          // hardwired logic for charge/spin semantics
          newspin = origspin;
          if ((numElectrons % 2) == 0) {
            newspin = SpinMult::singlet;
            if (theoryName.find("R0") == 0) {
              continue;
            }
          } else {
            if (theoryName.find("R") == 0 && theoryName.find("R0") != 0) {
              continue;
            }
          }
          // end hardwired logic
      
          // check p_stop at every stop so user can bail out of a process that 
          // might take too long
          if (escalc->copy(cpurl)) {
            //if (p_stop) break;
            newcalc = dynamic_cast<ICalculation*>
              (EDSIFactory::getResource(cpurl));
            newcalc->theory(&(*theories)[idx]);
            //if (p_stop) break;
            newcalc->runtype(TRunType((*runtypes)[jdx]));
            newcalc->setState(ResourceDescriptor::STATE_CREATED);
            //if (p_stop) break;
            if (origspin != newspin) {
              newcalc->spinMultiplicity(newspin);
              newcalc->openShells(0);
            }
          }
        }
        delete runtypes;
      }
      delete theories;
    }

    // delete our temporary calculation. 
    // reset to rerun however caused a refresh of the tree, update needed
    p_treeCtrl->removeFromTree(escalc->getURL(), true);

    // be nice and open the newly created test suite project folder
    findNode(testSuiteURL, true, true);

  } else {
    p_feedback->clearMessage();
    setMessage("Generating test suite can only be done from an electronic "
               "structure calculation.");
  }
}


/**
 * Creates a frame containing a properties panel for all metadata associated 
 * with the given tree item.
 */
void CalcMgr::viewProperties(WxResourceTreeItemData *itemData)
{
  EDSIServerCentral servers;
  string title = servers.mapURLtoName(itemData->getUrl());
  WxProperties *propsView = new WxProperties(this, wxID_ANY, title);

  propsView->setContext(itemData);
  propsView->Show(true);
  propsView->Raise();
}


/**
 * Returns to the last step of the given calculation.
 * This function checks to make sure this isn't a collection since only calc's
 * can have geometries.
 *
 * @param itemData  the tree node containing the calculation to reset
 */
void CalcMgr::resetWithLastGeom(WxResourceTreeItemData *itemData)
{
  // make sure this is a calculation and not a collection
  // get calculation
  PropertyTask *calc = dynamic_cast<PropertyTask*>(itemData->getResource());

  // get fragment
  Fragment *frag = (Fragment*)0;
  if (calc) {
    frag = calc->fragment();
  }

  // If we don't even have a frag then there is nothing more to do.  We
  // don't bother to call it an error.
  ResourceDescriptor::RUNSTATE tostate = ResourceDescriptor::STATE_CREATED;
  if (frag) {

    // Get GEOMTRACE property.
    // If we don't have one then again, there is nothing more to do.
    PropTSVecTable *geotrace = (PropTSVecTable*)calc->getProperty("GEOMTRACE");
    if (geotrace) {

      // Reset the Fragment coords with the last step
      int numSteps = geotrace->tables();
      int numRows = frag->numAtoms();
      const vector<double>& values = geotrace->values(numSteps-1);
      // A trace without the dummy atoms (MOPAC, Gaussian) maps onto the
      // other atoms in order.
      std::vector<int> target;
      for (int idx=0; idx<numRows; idx++)
        if (frag->atomRef(idx)->atomicSymbol() != "X") target.push_back(idx);
      if (target.size() != (size_t)numRows && 3*target.size() == values.size()) {
        for (size_t k=0; k<target.size(); k++)
          frag->atomRef(target[k])->coordinates(values[3*k], values[3*k+1],
                                                values[3*k+2]);
      } else if (3*(size_t)numRows != values.size()) {
        std::cerr << "GEOMTRACE last step holds " << values.size()
                  << " values for " << numRows
                  << " atoms; coordinates left unchanged" << std::endl;
      } else {
        TAtm *atom;
        int ptr=0;
        for (int idx=0; idx<numRows; idx++) {
          atom = frag->atomRef(idx);
          atom->coordinates(values[ptr],values[ptr+1], values[ptr+2]);
          ptr+=3;
        }
      }

      // Set the Fragment
      calc->fragment(frag);

    } else {
      // Since we didn't change geom, don't need to go to created
      // By setting tostate to ResourceDescriptor::STATE_ILLEGAL the 
      // resetForRerun() method will determine the correct runstate
      tostate = ResourceDescriptor::STATE_ILLEGAL;
      p_feedback->clearMessage();
      setMessage("This calculation has no optimised geometry; the copy keeps the starting structure.");
    }
    // So having done that we need to set the state to ready and remove
    // existing properties.
    // Do this regardless of whether it had a geomtrace
    calc->resetForRerun(tostate);
  } else if (calc) {
    p_feedback->clearMessage();
    setMessage("Unable to get chemical system for this calculation.");
  } else {
    p_feedback->clearMessage();
    setMessage("Must select a calculation for duplication operation.");
  }
}


/**
 * Makes a copy of the given tree item, resets the creation date, and places it
 * in the parent of the given tree item.
 *
 * @param itemData        the tree node to duplicate
 * @param openWhenFound   if true, it is selected in the tree upon creation
 * @return                returns a pointer to the newly created tree item or 
 *                        null if failed
 */
WxResourceTreeItemData* CalcMgr::duplicate(WxResourceTreeItemData* itemData,
                                           bool openWhenFound)
{
  // return variable
  WxResourceTreeItemData *ret = 0;

  // get the resource of the given tree item
  Resource *resource;
  if (itemData == 0) {
    setMessage("Duplicate was invoked with an invalid item.");
    return ret;
  } else {
    resource = itemData->getResource();
  }

  // get the url of the given tree item
  EcceURL copyUrl = itemData->getUrl();

  // the copy method will make an exact copy of the resource but give it a 
  // unique name and place it in the given item's parent
  // the creation date is automatically reset by the server
  if (! resource->copy(copyUrl)) {
    // copy failed for some reason, tell the user 
    setMessage(resource->messages());
  } else {
    //  Insert just the copy: reloading the parent collapsed every expanded
    //  sibling and dropped the selection to the root.
    if (!p_treeCtrl->addToTree(copyUrl))
      p_treeCtrl->refresh(p_treeCtrl->getParent(itemData));
    ret = findNode(copyUrl, openWhenFound, false);
    notifyCreate(copyUrl.toString());
  }

  // still null unless copy succeeded and was found
  return ret;
}


/**
 * EditListener virtual which will cause a memory leak most likely.
 * TODO double check memory leaks
 */
void CalcMgr::processEditCompletion(const EditEvent& ee)
{
  // parse out the property edited and the url of the WxResourceTreeItemData
  int atLoc = ee.id.find('@');
  if (atLoc != string::npos) {
    string property(ee.id.substr(0, atLoc));
    property = VDoc::getEcceNamespace() + ":" + property;
    EcceURL url(ee.id.substr(atLoc+1, ee.id.length()));

    // read the file into a string
    ifstream tmp(ee.filename.c_str());
    string buf;
    string value;
    while (tmp) {
      getline(tmp, buf);
      value += buf + '\n';
    }

    // add/change metadata and sync the metadata with the server
    WxResourceTreeItemData *itemData = findNode(url, false, false);
    if (! itemData->getResource()->addProp(property, value)) {
      setMessage(itemData->getResource()->messages());
    }

    // the tree and context panel must now be refreshed to reflect the changes
    updateUrl(url);

    // now tell the world of the property change, too
    notifyProperty(url.toString(), property, value);
  }
}


/**
 * Resource created in parent of type resType.  If option is selected, the
 * rename prompt is opened during creation.
 *
 * @param resType   the type of resource to create
 * @param parent    the node in which to place the newly created resource
 */
void CalcMgr::createResource(ResourceType * resType,
                             WxResourceTreeItemData * parent,
                             const bool& bypassNamePrompt)
{
  if (parent == NULL) {
    parent = p_treeCtrl->getSelection();
  }
  
  string name = resType->getLabel();
  if (!bypassNamePrompt && GetMenuBar()->IsChecked(wxID_ASKFORNAME)) {
    bool showInvalidMsg = false;
    string invalidMsg = "A name may contain only letters, digits, "
      "'.', '_' and '-'.\n\n";
    while (true) {
      name = resType->getLabel();
      string infoStr = "Please enter the name for the new " +
        name + ":";
      if (showInvalidMsg)
        infoStr = invalidMsg + infoStr;
      ewxTextEntryDialog dialog(this, infoStr, "New " + name + " Name",
                                name, wxOK | wxCANCEL);
      dialog.spacesToUnderscores();
      //  The menu action's window disabler and busy cursor are lifted
      //  while the user types: the prompt is not busy work.
      const bool held = p_disabler != 0;
      if (held) {
        delete p_disabler;  p_disabler = 0;
        delete p_cursor;    p_cursor = 0;
      }
      const int answer = dialog.ShowModal();
      if (held) {
        p_disabler = new wxWindowDisabler(this);
        p_cursor = new wxBusyCursor();
      }
      if (answer == wxID_OK) {
        name = dialog.GetValue();
        if (resType->isValidResourceName(name))
          break;
        else
          showInvalidMsg = true;
      }
      else
        // Cancel clicked, return without doing anything
        return;
    }
  }
  
  //  The tree is rebuilt and the new calculation selected: on MSW the
  //  collapsed tree, an empty pane and the half-filled summary were each
  //  painted on their way.
  FrameHold hold(this);
  try {
    Resource * parRes = parent->getResource();
    Resource * newRes = parRes->createChild(name, resType);

    if (newRes == 0)
      throw InvalidException("Null Pointer Resource", WHERE);

    EcceURL newurl = newRes->getURL();
    ResourceTool * tool = 0;
    if (GetMenuBar()->IsChecked(wxID_RUNDEFAULTTOOL))
      tool = newRes->getDefaultTool();

    // logic to handle MD and condensed phase reaction study branching
    // @todo should be moved to Session::createChild
    //  A GROMACS study has the application type of its tasks (an NWChem MD
    //  study has its own, MDStudy), so the study itself must not be linked
    //  into a session: only a task is.
    const bool isSession = dynamic_cast<Session*>(newRes) != 0;
    if (newRes->getApplicationType()==ResourceDescriptor::AT_NWCHEMMD ||
        (newRes->getApplicationType()==ResourceDescriptor::AT_GROMACS &&
         !isSession) ||
        parRes->getApplicationType()==ResourceDescriptor::AT_CONDENSED_REACTION_STUDY) {
      Resource *source = 0;
      vector<EcceURL> panelSelections = p_contextPanel->getSelections();
      // get the source to link from
      if (panelSelections.size() == 1) {
        source = dynamic_cast<ChemistryTask *>(EDSIFactory::getResource(panelSelections[0]));
      }

      if (newRes->getApplicationType() != ResourceDescriptor::AT_SOLVATE) {
        Session *session = dynamic_cast<Session*>(parRes);
        if (session == 0)
          throw InvalidException("Null Pointer Session", WHERE);
      
        try {
          session->addMemberAsTarget(newRes, source);
        }
        catch (InvalidException &ex) {
          // Since linking failed, delete the new resource
          newRes->remove();
          throw;
        }
      }
    }
    // the gas phase reaction study branching logic is contained within the
    // EDSIFactory and Session classes so nothing to do for that here
    
    p_treeCtrl->refresh(parent);
    setContextPanel();
    p_contextPanel->selectResource(newurl);

    // This commented out code and the comments are from Lisong.  I added
    // back the code that automatically selected the newly created object
    // because it is a useful feature.  I'm not seeing it crash at the moment,
    // but I thought it was safer to preserve Lisong's code and comments in
    // case problems return.  GDB 2/11/08

    // Comment out this part since it is causing the organizer crashing when
    // trying to open the new url.
    // Open the new resource if parent is not session based.
    //     if (parent->getResource()->getDescriptor()->getIsSessionBased()) {
    //       p_treeCtrl->refresh(parent);
    //       setContextPanel();
    //       p_contextPanel->selectResource(newurl);
    //     }
    //     else {
    //       p_treeCtrl->addToTree(newurl);
    //       findNode(newurl, true, true);
    //     }

    notifyCreate(newurl.toString());

    // automatically select the new object--this is Lisong's code that
    // he said was crashing on him
    // this logic is needed for the auto-open logic to work in
    // WxResourceTreeCtrl because that code works off the current selection
    // and this code makes the new object the current selection
    if (!parent->getResource()->getDescriptor()->getIsSessionBased()) {
      p_treeCtrl->addToTree(newurl);
      findNode(newurl, true, true);
    }

    if (tool) {
      startApp(tool->getName(), 0, newurl.toString());
    }

    // Immediately create a DirDyVTST task for a reaction study since that's
    // the only thing that makes sense to do initially
    if (newRes->getApplicationType() == ResourceDescriptor::AT_REACTION_STUDY) {
      ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
      ResourceType *rt = rd.getResourceType(
                               ResourceDescriptor::RT_VIRTUAL_DOCUMENT, 
                               ResourceDescriptor::CT_CALCULATION,
                               ResourceDescriptor::AT_DRDVTST);
      createResource(rt, NULL, true);
    }
    // Likewise, immediately create a Solvate task for a condensed reaction
    // study
    else if (newRes->getApplicationType() ==
                     ResourceDescriptor::AT_CONDENSED_REACTION_STUDY) {
      ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
      ResourceType *rt = rd.getResourceType(
                               ResourceDescriptor::RT_VIRTUAL_DOCUMENT, 
                               ResourceDescriptor::CT_CALCULATION,
                               ResourceDescriptor::AT_SOLVATE);
      createResource(rt, NULL, true);
    }
  }
  catch (InvalidException& ex) {
    if (p_testCreateError) {
      *p_testCreateError = ex.what();
      return;
    }
    ewxMessageDialog dlg(this, ex.what(),
                         "Unable to create " + name + " object!",
                         wxOK|wxICON_EXCLAMATION, wxDefaultPosition);
    dlg.ShowModal();
  }
}


/**
 * Sets the context of the access control dialog.
 */
void CalcMgr::updateAccessCtrlDlg()
{
  if (getSelection()) {
    if (p_accessControlDialog) {
      p_accessControlDialog->setContext(p_currentSelection[0].toString(),
                                        p_currentSelection[0].getFilePathTail());
    }
  }
}


/**
 * Sets the context of the find dialog.
 */
void CalcMgr::updateFindDlg()
{
  WxResourceTreeItemData *itemData = p_treeCtrl->getSelection();

  EcceURL url = itemData->getUrl();

  if (p_find) {
    p_find->setFrom(url.toString());
  }
}


/**
 * Returns the feedback display.
 */
WxFeedback * CalcMgr::getFeedback()
{
  return p_feedback;
}


/**
 * Refresh the tree ctrl and the context panel when a rename message is received
 */
void CalcMgr::updateName(const EcceURL & oldUrl, const EcceURL & newUrl)
{
  EcceURL selectedUrl = p_treeCtrl->getSelection()->getUrl();

  if (selectedUrl == oldUrl)
    selectedUrl = newUrl;
  else {
    string urlStr = selectedUrl.toString();
    if (urlStr.find(oldUrl.toDirString()) == 0) {
      selectedUrl = EcceURL(urlStr.replace(0, oldUrl.toDirString().length(),
                                           newUrl.toDirString()));
    }
  }
  updateAddNode(newUrl);
  findNode(selectedUrl, true, true);
  updateRemoveNode(oldUrl);
}


/**
 * Refresh the context panel and tree ctrl when the resource associated with
 * the URL got changed
 */
void CalcMgr::updateUrl(const EcceURL & url)
{
  WxResourceTreeItemData * target = findNode(url, false, false);
  
  if (target != 0) {

    EcceURL selectedUrl = p_treeCtrl->getSelection()->getUrl();
    EDSIFactory::changePoolResource(url);
    
    if (selectedUrl.isChildOf(url)) {
      findNode(url, true, false);
      selectedUrl = url;
    }

    if (target->getResource()->hasAccess()) {
      //      startDisabler();
      p_treeCtrl->refresh(target);
      //      stopDisabler();
    }

    if (selectedUrl == url || url.isChildOf(selectedUrl)) {
      //      startDisabler();
      //      findNode(selectedUrl, false, true);
      onSelectionChange(true);
      //      stopDisabler();
    }
  }
}


void CalcMgr::queueUpdate(UpdateKind kind, const EcceURL & url)
{
  p_testNotices++;
  std::pair<UpdateKind, string> item(kind, url.toString());
  // A repeated change notice adds nothing; additions and removals keep
  // their order, only an immediate repeat is dropped.
  bool dup = kind == UPDATE_CHANGED
    ? std::find(p_queuedUpdates.begin(), p_queuedUpdates.end(), item) !=
        p_queuedUpdates.end()
    : !p_queuedUpdates.empty() && p_queuedUpdates.back() == item;
  if (!dup) p_queuedUpdates.push_back(item);
  if (!p_updateTimer) {
    p_updateTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, [this](wxTimerEvent&) { applyQueuedUpdates(); },
         p_updateTimer->GetId());
  }
  // The notices of one Save arrive within a few ms of each other.
  if (!p_updateTimer->IsRunning()) p_updateTimer->StartOnce(150);
}


// One key per calculation whether or not its URL ends in a slash.
static string progressKey(const EcceURL & url)
{
  string s = url.toString();
  while (s.size() > 1 && s.back() == '/') s.pop_back();
  return s;
}


void CalcMgr::progressUpdate(const EcceURL & url, const string & text)
{
  const string key = progressKey(url);
  if (text.empty())
    p_liveProgress.erase(key);
  else
    p_liveProgress[key] = std::make_pair(text, (double)time(0));
  // Progress comes only from a monitor that is heard.
  auto lost = p_contact.find(key);
  if (lost != p_contact.end() && lost->second > 0) {
    lost->second = -1;
    p_machineCheck.erase(key);
    contactChanged(url);
    return;
  }
  CalculationContextPanel *panel =
      dynamic_cast<CalculationContextPanel*>(p_contextPanel);
  if (panel && progressKey(panel->getUrl()) == key)
    panel->updateProgress();
  // The project table: that one cell, no rebuild (#254).
  if (ProjectContextPanel *table = dynamic_cast<ProjectContextPanel*>(p_contextPanel))
    table->updateProgress(url);
}


void CalcMgr::forgetProgress(const EcceURL & url)
{
  p_liveProgress.erase(progressKey(url));
}


bool CalcMgr::liveProgress(const EcceURL & url, string & text,
                           double & received) const
{
  auto it = p_liveProgress.find(progressKey(url));
  if (it == p_liveProgress.end()) return false;
  text = it->second.first;
  received = it->second.second;
  return true;
}


//  The last jmPROG snapshot in a job monitor message file: messages are
//  \001, a sequence digit, a four-digit length and the payload, whose
//  newlines are \002 and whose fields are separated by \0.
static string lastProgressMessage(const string & raw)
{
  string last;
  size_t i = 0;
  while ((i = raw.find('\001', i)) != string::npos) {
    if (i + 6 > raw.size()) break;
    const size_t len = (size_t)atoi(raw.substr(i + 2, 4).c_str());
    const string payload = raw.substr(i + 6, len);
    i += 6 + len;
    if (payload.compare(0, 7, string("jmPROG", 6) + '\0') != 0) continue;
    last = payload.substr(7);
    for (char & c : last) if (c == '\002') c = '\n';
  }
  return last;
}


static bool copyStreamToFile(istream * is, const string & path)
{
  if (!is) return false;
  std::ofstream out(path.c_str(), std::ios::binary);
  out << is->rdbuf();
  delete is;
  return (bool)out;
}


void CalcMgr::backfillProgress(const EcceURL & url)
{
  const string key = progressKey(url);
  if (getenv("ECCE_NO_PROGRESS_BACKFILL") || !p_backfillTried.insert(key).second)
    return;
  string have;
  double at;
  if (liveProgress(url, have, at)) return;
  TaskJob * calc = dynamic_cast<TaskJob*>(EDSIFactory::getResource(url));
  if (!calc || !calc->application()) return;
  const string ecce = VDoc::getEcceNamespace();
  if (!calc->getProp(ecce + ":progress").empty()) return;
  if (calc->getState() < ResourceDescriptor::STATE_COMPLETED) return;

  string desc = calc->application()->parseDescriptor();
  if (desc.empty()) return;
  if (desc[0] != '/') desc = string(Ecce::ecceDataControllersPath()) + "/" + desc;
  const string monitor = string(Ecce::ecceHome()) + "/scripts/eccejobmonitor";

  //  What the job monitor of a live job reads: the parse file, and the
  //  program's own output as the second file.
  wxBusyCursor busy;
  const string base = wxFileName::CreateTempFileName("eccepg").ToStdString();
  const string work = base + ".d";
  if (base.empty() || !wxMkdir(work)) return;
  const string parse = base + ".parse", out2 = base + ".out";
  const bool haveParse = copyStreamToFile(
      calc->getDataFile(JCode::PARSE_OUTPUT), parse);
  const bool haveOut = copyStreamToFile(
      calc->getDataFile(JCode::PRIMARY_OUTPUT), out2);
  if (!haveParse && !haveOut) {
    wxRemoveFile(base);
    wxFileName::Rmdir(work, wxPATH_RMDIR_RECURSIVE);
    return;
  }

  wxString cmd;
  cmd << "perl \"" << monitor << "\" -commType stdio -monitoringMode post"
      << " -jobOutputFile \"" << (haveParse ? parse : out2) << "\"";
  if (haveParse && haveOut) cmd << " -jobOutputFile2 \"" << out2 << "\"";
  cmd << " -parseDescriptorFile \"" << desc << "\" -parseTypes NONE"
      << " -outputFile \"" << base << ".res\" -calcName backfill"
      << " -logFile \"" << work << "/monitor.log\" -filesToTransfer \"\""
      << " -importDir \"" << work << "/\" -progress 1";

  wxProcess * proc = new wxProcess(this);
  proc->Redirect();
  wxExecuteEnv env;
  env.cwd = work;
  const long pid = wxExecute(cmd, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, proc, &env);
  if (pid <= 0) {
    delete proc;
    wxRemoveFile(base);
    wxFileName::Rmdir(work, wxPATH_RMDIR_RECURSIVE);
    return;
  }
  //  The monitor reads standard input; end of input lets it run on.
  proc->CloseOutput();
  if (p_backfills.empty()) {
    Bind(wxEVT_END_PROCESS, [this](wxProcessEvent & ev) {
      finishProgressBackfill(ev.GetPid());
    });
  }
  p_backfills[pid] = std::make_pair(url, base);
}


void CalcMgr::finishProgressBackfill(long pid)
{
  auto it = p_backfills.find(pid);
  if (it == p_backfills.end()) return;
  const EcceURL url = it->second.first;
  const string base = it->second.second;
  p_backfills.erase(it);

  string raw;
  {
    std::ifstream in((base + ".res").c_str(), std::ios::binary);
    raw.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  }
  //  Only the files that were written exist (older calculations have no
  //  parse file); wxRemoveFile would pop an error dialog for the rest.
  for (const string & f : {base, base + ".parse", base + ".out", base + ".res"})
    if (wxFileExists(f)) wxRemoveFile(f);
  wxFileName::Rmdir(base + ".d", wxPATH_RMDIR_RECURSIVE);

  const string text = lastProgressMessage(raw);
  JobProgress check;
  if (text.empty() || !check.parse(text)) return;
  if (Resource * res = EDSIFactory::getResource(url)) {
    vector<MetaDataResult> results(1);
    results[0].name = VDoc::getEcceNamespace() + ":progress";
    results[0].value = text;
    res->addProps(results);
  }
  //  Age 0: a finished calculation's box shows no age.
  p_liveProgress[progressKey(url)] = std::make_pair(text, 0.0);
  CalculationContextPanel * panel =
      dynamic_cast<CalculationContextPanel*>(p_contextPanel);
  if (panel && progressKey(panel->getUrl()) == progressKey(url))
    panel->updateProgress();
}


string CalcMgr::progressCellText(Resource * resource) const
{
  if (!resource) return "";
  const string ecce = VDoc::getEcceNamespace();
  if (ResourceUtils::stringToState(resource->getProp(ecce + ":state")) !=
      ResourceDescriptor::STATE_RUNNING)
    return "";
  double lastHeard = 0;
  if (outOfContact(resource, lastHeard))
    return JobProgress::noContactText((double)time(0) - lastHeard);
  string text, live;
  double at = 0;
  if (liveProgress(resource->getURL(), live, at)) text = live;
  else text = resource->getProp(ecce + ":progress");
  JobProgress p;
  return p.parse(text) ? p.shortLine() : "";
}


void CalcMgr::contactUpdate(const EcceURL & url, const string & contact,
                            const string & progress)
{
  const string key = progressKey(url);
  if (contact.compare(0, 5, "lost ") == 0) {
    p_contact[key] = atof(contact.c_str() + 5);
  } else {
    p_contact[key] = -1;
    p_machineCheck.erase(key);
    if (contact.compare(0, 3, "ok ") == 0)
      p_checked[key] = atof(contact.c_str() + 3);
  }
  //  The snapshot it carries is the last one heard, not a new one: its
  //  age comes from the job store's stamp in it.
  if (!progress.empty()) {
    auto it = p_liveProgress.find(key);
    if (it == p_liveProgress.end() || it->second.first != progress)
      p_liveProgress[key] = std::make_pair(progress, 0.0);
  }
  contactChanged(url);
}


bool CalcMgr::outOfContact(Resource * resource, double & lastHeard,
                           bool queued) const
{
  if (!resource) return false;
  const string ecce = VDoc::getEcceNamespace();
  const ResourceDescriptor::RUNSTATE state =
      ResourceUtils::stringToState(resource->getProp(ecce + ":state"));
  if (state != ResourceDescriptor::STATE_RUNNING &&
      !(queued && state == ResourceDescriptor::STATE_SUBMITTED))
    return false;
  auto it = p_contact.find(progressKey(resource->getURL()));
  if (it != p_contact.end()) {
    lastHeard = it->second;
    return it->second > 0;
  }
  const string stored = resource->getProp(ecce + ":contact");
  if (stored.compare(0, 5, "lost ") != 0) return false;
  lastHeard = atof(stored.c_str() + 5);
  return lastHeard > 0;
}


double CalcMgr::lastChecked(Resource * resource) const
{
  if (!resource) return 0;
  auto it = p_checked.find(progressKey(resource->getURL()));
  if (it != p_checked.end()) return it->second;
  const string stored = resource->getProp(VDoc::getEcceNamespace() + ":contact");
  return stored.compare(0, 3, "ok ") == 0 ? atof(stored.c_str() + 3) : 0;
}


vector<string> CalcMgr::machineCheckLines(const EcceURL & url) const
{
  auto it = p_machineCheck.find(progressKey(url));
  return it == p_machineCheck.end() ? vector<string>() : it->second;
}


void CalcMgr::contactChanged(const EcceURL & url)
{
  const string key = progressKey(url);
  WxResourceTreeItemData *node = p_treeCtrl->findNode(url, false, false);
  if (node) p_treeCtrl->refreshState(node);
  CalculationContextPanel *panel =
      dynamic_cast<CalculationContextPanel*>(p_contextPanel);
  if (panel && progressKey(panel->getUrl()) == key)
    panel->updateProgress();
  if (ProjectContextPanel *table = dynamic_cast<ProjectContextPanel*>(p_contextPanel))
    table->updateProgress(url);
}


void CalcMgr::progressAction(int action, WxResourceTreeItemData * itemData)
{
  if (!itemData) return;
  if (action == 0) reconnectJob(itemData);
  else if (action == 1) checkJobOnMachine(itemData);
  else if (action == 2) tailOutputFile(itemData);
}


void CalcMgr::applyQueuedUpdates()
{
  std::vector<std::pair<UpdateKind, string> > todo;
  todo.swap(p_queuedUpdates);
  if (todo.empty()) return;
  p_testBatches++;
#ifdef __WXMSW__
  wxWindowUpdateLocker freeze(this);   // not on GTK3, see FrameHold
#endif
  for (size_t i = 0; i < todo.size(); i++) {
    p_testApplied++;
    EcceURL url(todo[i].second);
    if (todo[i].first == UPDATE_ADDED) updateAddNode(url);
    else if (todo[i].first == UPDATE_ADDED_SELECT) updateAddNode(url, true);
    else if (todo[i].first == UPDATE_REMOVED) updateRemoveNode(url);
    else updateUrl(url);
  }
}


/**
 * Refresh the context panel and tree ctrl when the resource associated with
 * the URL got added
 */
void CalcMgr::updateAddNode(const EcceURL & url, bool select)
{
  if (findNode(url.getParent(), false, false)) {
    p_treeCtrl->addToTree(url);
  
    if (url.getParent() == p_treeCtrl->getSelection()->getUrl()) {
      setContextPanel();
      if (select && p_contextPanel) p_contextPanel->selectResource(url);
    }
  }
}


/**
 * Refresh the context panel and tree ctrl when the resource associated with
 * the URL got removed
 */
void CalcMgr::updateRemoveNode(const EcceURL & url)
{
  if (findNode(url, false, false)) {
    //    startDisabler();
    p_treeCtrl->removeFromTree(url, false);
    if (url.getParent() == p_treeCtrl->getSelection()->getUrl())
      setContextPanel();
    //    stopDisabler();
  }
}


/**
 *
 */
void CalcMgr::updateBookmarkMenu()
{
  startDisabler();

  wxMenu* bookmarkMenu = new wxMenu;
  bookmarkMenu->Append(wxID_ADD_BOOKMARK, _("&Add to Bookmarks\tCtrl+D"),
                       _T(""), wxITEM_NORMAL);
  bookmarkMenu->Append(wxID_MANAGE_BOOKMARK, _("&Manage Bookmarks..."),
                       _T(""), wxITEM_NORMAL);
  bookmarkMenu->AppendSeparator();

  for (int i=0; i<p_bookmarkEditor->getSize(); i++) {
    bookmarkMenu->Append(ID_BOOKMARK_BASE+i,
                         p_bookmarkEditor->getBookmarkName(i),
                         p_bookmarkEditor->getBookmarkUrl(i),
                         wxITEM_NORMAL);
  }
  
  wxMenuBar * menuBar = GetMenuBar();
  delete menuBar->Replace(menuBar->FindMenu(_("Bookmarks")),
                          bookmarkMenu, _("&Bookmarks"));

  stopDisabler();
}


void CalcMgr::setMessage(const string & message,
                         WxFeedback::Severity severity,
                         bool clear)
{
  p_feedback->setMessage(message, severity, clear);
}


/**
 * A wrapper for the util function findNode in treectrl.
 * @param targetUrl target url to be found.
 * @param openWhenFound whether open the target node in the tree, default true.
 * @param loadFromServer if not in the tree, whether load children from server,
 *                       default true.
 */
WxResourceTreeItemData * CalcMgr::findNode(const EcceURL &targetUrl,
                                           bool openWhenFound,
                                           bool loadFromServer)
{
  return p_treeCtrl->findNode(targetUrl, openWhenFound, loadFromServer);
}


///////////////////// WxCalcImportClient virtuals ///////////////////////////

///////////////////////////////////////////////////////////////////////////
// Create an IPropCalculation object from the current url.  This is where
// the imported calc will be located.
///////////////////////////////////////////////////////////////////////////
TaskJob *CalcMgr::getContainer(const string& name)
{
  TaskJob *ret = 0;

  // get tree item data
  WxResourceTreeItemData *itemData = p_treeCtrl->getSelection();

  // getSelection() returns 0 when nothing is selected, and this used to
  // dereference it immediately. It was unreachable while the Import menu
  // entry was only offered for a selected "project"; now that the entry is
  // always offered (see getFileMenu), it is not.
  //
  // Returning 0 is the documented contract here: WxCalcImport checks for it
  // and reports "Can't access or write to the specified parent project for
  // the import", which is the right message for "you have not told me
  // where to put it".
  if (itemData == (WxResourceTreeItemData*)0) {
    return ret;
  }

  // Since f85eaae (#97) the import entry is offered regardless of what is
  // selected, but this used to createChild() straight under the current
  // selection -- so with a calculation selected, the import completed but
  // landed the new calc nested inside that calc. That is invisible: calc
  // nodes aren't expandable, so the imported calc simply never appeared
  // (#44). Walk up from the selection to the nearest enclosing "project"
  // node instead.
  WxResourceTreeItemData *projData = itemData;
  Resource *projRes = projData->getResource();

  while (projRes != (Resource*)0 &&
         projRes->getDescriptor() != (ResourceType*)0 &&
         projRes->getDescriptor()->getName() != "project") {
    WxResourceTreeItemData *parentData = p_treeCtrl->getParent(projData);

    // getParent() returns the node itself once it reaches the server
    // root -- nowhere higher to go, so give up rather than loop forever.
    if (parentData == projData || parentData == (WxResourceTreeItemData*)0) {
      projData = (WxResourceTreeItemData*)0;
      break;
    }

    projData = parentData;
    projRes = (projData != (WxResourceTreeItemData*)0) ?
              projData->getResource() : (Resource*)0;
  }

  if (projData == (WxResourceTreeItemData*)0 || projRes == (Resource*)0) {
    return ret;
  }

  // make resource
  Resource *itemDataRes = projRes;
  Resource *childRes = 0;

  if (itemDataRes != 0) {

    try {
      childRes = itemDataRes->createChild(name,
              ResourceDescriptor::RT_VIRTUAL_DOCUMENT,
              ResourceDescriptor::CT_CALCULATION,
              ResourceDescriptor::AT_UNDEFINED);
    }
    catch (InvalidException& ex) {
      // create resource failed, let it fall through
      //cerr << "EDSIFactory::createResource failed :: " << ex.what() << endl;
    }

    if (childRes != 0) {
      if (!p_batchImport) {
        p_treeCtrl->refresh(projData);
        setContextPanel();
      }

      ret = dynamic_cast<TaskJob*>(childRes);
    }
  }

  return ret;
}


void CalcMgr::importValidationComplete(TaskJob *ipc, bool status,
                                       string message)
{
  if (getenv("ECCE_TEST_CALCIMPORT")) {
    fprintf(stderr, "ECCE_TEST_CALCIMPORT: %s, code %s: %s\n",
            status ? "imported" : "refused",
            (ipc && ipc->application()) ? ipc->application()->name().c_str()
                                        : "-",
            message.c_str());
  }
  if (!status) {
    if (ipc != 0) {
      // Lisong, this is where the task is deleted if the import failed
      // Just need it to update/refresh the right hand panel
      EcceURL url = ipc->getURL();
      string ct = ResourceUtils::contentTypeToString(ipc->getContentType());
      if (p_treeCtrl->removeFromTree(url, true)) {
        notifyRemove(url.toString(), ct);
      }
      else {
        string msg = "Unable to delete ";
        msg += url.toString();
        msg += ".";
        setMessage(msg, WxFeedback::ERROR);
      }
    }
    if (p_batchImport && p_batchError.empty())
      p_batchError = message;
    setMessage(message, WxFeedback::ERROR);
  } else {
    if (message != "")
      setMessage(message, WxFeedback::WARNING);

    // The new calc's parent is wherever getContainer() actually put it
    // (the enclosing project, not necessarily what was selected -- #44),
    // so find and refresh that node from the calc's own URL rather than
    // assuming the tree selection didn't move in the meantime.
    WxResourceTreeItemData *parentData =
      p_treeCtrl->findNode(ipc->getURL().getParent(), false, false);

    string parentName = (parentData != (WxResourceTreeItemData*)0 &&
                         parentData->getResource() != (Resource*)0) ?
                         parentData->getResource()->getName() : "";

    message = "Calculation output currently being imported into ";
    if (parentName != "") {
      message += parentName + "/";
    }
    message += ipc->getName() + ".";
    setMessage(message, WxFeedback::INFO);

    // get the parent of the calculation
    if (p_batchImport) {
      // openExampleSet() rebuilds the project's subtree once, at the end.
    } else if (parentData != (WxResourceTreeItemData*)0) {
      p_treeCtrl->refresh(parentData);
    }
    else {
      p_treeCtrl->refresh(p_treeCtrl->getSelection());
    }
    if (!p_batchImport)
      setContextPanel();

    // let the world know this calculation was created/imported
    notifyCreate(ipc->getURL().toString());

    if (p_batchImport)
      return;

    // Select the new calculation, as a calculation made with New is
    // selected (OnNewResourceClick); otherwise the work area stays on
    // whatever folder was selected and the import looks lost.
    if (findNode(ipc->getURL(), true, true) != 0) {
      if (getenv("ECCE_TEST_CALCIMPORT")) {
        WxResourceTreeItemData *sel = p_treeCtrl->getSelection();
        fprintf(stderr, "ECCE_TEST_CALCIMPORT: selected %s\n",
                sel ? sel->getUrl().toString().c_str() : "-");
        // What the finished import stored, read back after the job store
        // has parsed the output (it sets the state to Loaded last).
        if (getenv("ECCE_TEST_IMPORTREPORT")) {
          for (int i = 0; i < 90 && ipc->getState() !=
                                     ResourceDescriptor::STATE_LOADED; i++) {
            wxMilliSleep(1000);
            wxYield();
          }
          ChemistryTask *chem = dynamic_cast<ChemistryTask*>(ipc);
          ICalculation *icalc = dynamic_cast<ICalculation*>(ipc);
          Fragment *frag = chem ? chem->fragment() : 0;
          TGBSConfig *cfg = icalc ? icalc->gbsConfig() : 0;
          fprintf(stderr, "ECCE_TEST_IMPORTREPORT: state %d pointgroup %s "
                  "multiplicity %d basis '%s' ecp '%s' spherical %s\n",
                  (int)ipc->getState(),
                  frag ? frag->pointGroup().c_str() : "-",
                  icalc ? (int)icalc->spinMultiplicity() : -1,
                  cfg ? cfg->name().c_str() : "-",
                  cfg ? cfg->ecpName().c_str() : "-",
                  cfg ? (cfg->coordsys() == TGaussianBasisSet::Spherical
                         ? "yes" : "no") : "-");
          // The stored stdout and progress record of an import with a
          // companion output (NWChem's .out beside the .eprint trace).
          {
            TaskJob *tj = dynamic_cast<TaskJob*>(ipc);
            TypedFile po;
            if (tj) tj->getDataFile(JCode::PRIMARY_OUTPUT, po);
            fprintf(stderr, "ECCE_TEST_IMPORTREPORT: output '%s' progress %d\n",
                    tj ? po.name().c_str() : "-",
                    (tj && !tj->getProp(VDoc::getEcceNamespace() +
                                        ":progress").empty()) ? 1 : 0);
          }
          // The numbers the viewer's Basis Set summary section shows.
          if (frag && cfg && !cfg->empty()) {
            TagCountMap *tc = frag->tagCountsSTL();
            if (tc) {
              fprintf(stderr, "ECCE_TEST_IMPORTREPORT: functions %d "
                      "primitives %d\n", (int)cfg->num_functions(*tc),
                      (int)cfg->num_primitives(*tc));
              delete tc;
            }
          }
          delete frag;
          delete cfg;
        }
        // Reset for Rerun on the imported calculation must refuse it.
        if (getenv("ECCE_TEST_RESETIMPORTED")) {
          // The job monitor sets Loaded after the import; this process
          // exits first, so set it as the monitor would.
          ipc->setState(ResourceDescriptor::STATE_LOADED);
          resetForRerun();
          TaskJob *t = dynamic_cast<TaskJob*>(ipc);
          fprintf(stderr, "ECCE_TEST_RESETIMPORTED: state after %s\n",
                  t && t->getState() == ResourceDescriptor::STATE_LOADED
                      ? "Loaded" : "changed");
        }
      }
    }
  }
  setContextPanel();
}


//  A top menu's state is set only when it changes: wxMSW redraws the whole
//  menu bar for every EnableTop().  `changed` says the bar needs a redraw.
static void enableTopMenu(wxMenuBar *bar, const wxString& title, bool enable,
                          bool& changed)
{
  const int pos = bar->FindMenu(title);
  if (pos != wxNOT_FOUND && bar->IsEnabledTop(pos) != enable) {
    bar->EnableTop(pos, enable);
    changed = true;
  }
}


//  Menu items and tools are only touched when their state changes, so a
//  selection that changes nothing redraws nothing (#296).
static void enableMenuItem(wxMenuBar *bar, int id, bool enable)
{
  if (bar->FindItem(id) && bar->IsEnabled(id) != enable)
    bar->Enable(id, enable);
}


static void enableTool(wxToolBar *bar, int id, bool enable)
{
  if (bar && bar->FindById(id) && bar->GetToolEnabled(id) != enable)
    bar->EnableTool(id, enable);
}


//  While frozen, wxMSW does not redraw the menu bar for each item rebuilt.
//  Its titles are the only part on screen, so the bar is redrawn after the
//  hold only when one of them changed state.
struct CalcMgr::MenuBarHold {
  wxMenuBar *bar;
  bool& titlesChanged;
  //  MSW only, as FrameHold: a frozen GTK3 menu bar is painted black.
#ifdef __WXMSW__
  MenuBarHold(wxMenuBar *b, bool& changed) : bar(b), titlesChanged(changed)
    { if (bar) bar->Freeze(); }
#else
  MenuBarHold(wxMenuBar *, bool& changed) : bar(0), titlesChanged(changed) {}
#endif
  ~MenuBarHold() {
    if (!bar) return;
    bar->Thaw();
    if (!bar->IsFrozen() && titlesChanged) {
      titlesChanged = false;
      bar->Refresh();
    }
  }
};


void CalcMgr::onSelectionChange(bool selectInTree)
{
  //  The menu bar is not one of the frame's children; wxMSW redraws it for
  //  every item added or removed (half a menu bar on screen) unless frozen.
  //  Thawing does not repaint it on MSW (it has no window of its own), so
  //  the hold redraws it when a title changed state (Run Mgmt drawn
  //  disabled for a completed calculation otherwise).
  MenuBarHold holdMenus(GetMenuBar(), p_menuTitlesChanged);
  //  The summary pane is replaced and the tree and toolbar change in one
  //  go; on MSW each step was painted (an empty pane, then half of it).
  //  Released before the menu bar, so its repaint is not swallowed.  On
  //  MSW the thaw only invalidates, and each child then painted on its own
  //  turn (the run-state legend a frame after the summary); paint them all
  //  at once.
  //  Not when the selection came from the project table (selectInTree
  //  false): nothing in the frame is rebuilt then, and the thaw's repaint
  //  of every child made the table and summary flash on each click.
  std::unique_ptr<FrameHold> hold;
  if (selectInTree)
    hold.reset(new FrameHold(this));
  wxMenuBar * menuBar = GetMenuBar();
  WxResourceTreeItemData *itemData = 0;

  if (selectInTree) {
    // Start disabler since loading children may take some time.
    startDisabler();

    itemData = p_treeCtrl->getSelection();
    
    bool isCollection = p_treeCtrl->setExpandable(itemData);
    if (isCollection)
      p_treeCtrl->loadChildren(itemData);
    
    setContextPanel();

    // Add the current node to the history list, HistoryList should be smart
    // enough to figure out whether this is revisiting using back-forth or a
    // new selection.
    wxMenu * historyMenu = menuBar->GetMenu(menuBar->FindMenu(_("Go")));
    getHistoryMenu(*historyMenu, itemData);
    
    // Enable/Disable the tool buttons based on the selection
    p_toolBar->EnableTool(wxID_UP, p_treeCtrl->GetRootItem() !=
                          p_treeCtrl->GetItemParent(p_treeCtrl->GetSelection()));
    p_toolBar->EnableTool(wxID_BACKWARD, p_hlist->canMoveBack());
    p_toolBar->EnableTool(wxID_FORWARD, p_hlist->canMoveForward());

    p_toolBar->EnableTool(wxID_UPLOAD, isCollection);
    
    EDSIServerCentral servers;
    EcceURL url = itemData->getUrl();
    p_feedback->setContextLabel(servers.mapURLtoName(url));
    
    // Move this out so that file menu will always get updated.
    // Doing this because of the Session require prepare as the first task.
    // wxMenu * fileMenu = menuBar->GetMenu(menuBar->FindMenu(_("File")));
    // getFileMenu(*fileMenu, itemData);

    stopDisabler();
  }

  //  The menus follow the selection once it has settled: a Shift range or
  //  a click that moves the selection reports every step, and rebuilding
  //  Run Mgmt, Tools and File for each made the top menus flicker between
  //  enabled and greyed on MSW.
#ifdef __WXMSW__
  if (!p_menusPending) {
    p_menusPending = true;
    CallAfter([this]() {
      p_menusPending = false;
      updateSelectionMenus();
    });
  }
#else
  updateSelectionMenus();
#endif
}


void CalcMgr::updateSelectionMenus()
{
  MenuBarHold holdMenus(GetMenuBar(), p_menuTitlesChanged);
  wxMenuBar * menuBar = GetMenuBar();
  WxResourceTreeItemData * itemData = p_treeCtrl->getSelection();
  wxMenu * fileMenu = menuBar->GetMenu(menuBar->FindMenu(_("File")));
  getFileMenu(*fileMenu, itemData);

  
  if (!getSelection()) {
    // Several calculations: Run Mgmt offers only the batch launch.
    bool allCalcs = p_currentSelection.size() > 1;
    for (size_t i = 0; allCalcs && i < p_currentSelection.size(); i++) {
      WxResourceTreeItemData *n = findNode(p_currentSelection[i], false, true);
      allCalcs = n && dynamic_cast<TaskJob*>(n->getResource());
    }
    wxMenu *rm = menuBar->GetMenu(menuBar->FindMenu(_("Run Mgmt")));
    clearMenu(*rm);
    if (allCalcs) {
      rm->Append(wxID_LAUNCH_SELECTED, _("Launch Selected..."));
      rm->Append(wxID_RECONNECT_SELECTED, _("Reconnect Selected..."));
      rm->Append(wxID_REPARSE_OUTPUTS, _("Re-Parse Outputs"));
    }
    enableTopMenu(menuBar, _("Run Mgmt"), allCalcs, p_menuTitlesChanged);
    enableTopMenu(menuBar, _("Tools"), false, p_menuTitlesChanged);
    enableMenuItem(menuBar, wxID_CHANGE_ACCESS, false);
    enableMenuItem(menuBar, wxID_RENAME, false);
    enableMenuItem(menuBar, wxID_VIEW_PROPERTY, false);
    enableMenuItem(menuBar, wxID_ANNOTATION, false);
    enableMenuItem(menuBar, wxID_DUPLICATE, false);
    enableMenuItem(menuBar, wxID_DUPLICATE2, false);
    enableMenuItem(menuBar, wxID_DUPLICATE3, false);
    enableTool(p_toolBar, wxID_RENAME, false);
    enableTool(p_toolBar, wxID_VIEW_PROPERTY, false);
    enableTool(p_toolBar, wxID_DOWNLOAD, false);

  }
  else {
    itemData = findNode(p_currentSelection[0], false, true);
    // getResource() can legitimately be null (e.g. resource failed to
    // load) -- confirmed via a real core dump hit on cold start, same
    // bug shape fixed twice already just above this call chain.
    if (itemData && itemData->getResource() && itemData->getResource()->hasAccess()) {
      bool isProject = (itemData->getResource()->getContentType() ==
                        ResourceDescriptor::CT_PROJECT);
      bool isCalculation = (itemData->getResource()->getContentType() ==
                            ResourceDescriptor::CT_CALCULATION);
      bool isDocument = (itemData->getResource()->getResourceType() ==
                         ResourceDescriptor::RT_DOCUMENT);
      bool isSystemFolder = itemData->getUrl().isSystemFolder();

      // Surface why an incomplete/failed calc ended up that way, right
      // where the user is already looking -- Andy (#--, 2026-09-27):
      // "the reason should be discoverable. The Launcher should tell
      // me." eccejobstore (JobStore.C's recordStatusReason()) writes the
      // most specific reason it has into this same namespaced metadata
      // key whenever a calc goes unsuccessful/failed, and clears it on
      // any other terminal state, so an empty value here means either
      // the state predates this feature or the calc is fine.
      //  Selection handling runs again on every state update, so the same
      //  reason was printed twice.  Show it once per calc and reason, and
      //  forget it as soon as the selection moves to anything else, so
      //  coming back to the calc shows it again.
      static string lastShown;
      bool shownNow = false;
      if (isCalculation) {
        ResourceDescriptor::RUNSTATE state = ResourceUtils::stringToState(
                    itemData->getResource()->getProp(
                        VDoc::getEcceNamespace() + ":state"));
        //  A Ready calculation carries a reason only after a failed submit.
        if (state == ResourceDescriptor::STATE_READY ||
            state == ResourceDescriptor::STATE_UNSUCCESSFUL ||
            state == ResourceDescriptor::STATE_FAILED ||
            state == ResourceDescriptor::STATE_SYSTEM_FAILURE ||
            state == ResourceDescriptor::STATE_WAITING) {
          string reason = itemData->getResource()->getProp(
                              VDoc::getEcceNamespace() + ":runStatusReason");
          const string key = itemData->getResource()->getURL().toString()
                             + "\n" + reason;
          if (!reason.empty()) {
            shownNow = true;
            if (key != lastShown) {
              lastShown = key;
              setMessage(reason, WxFeedback::WARNING);
            }
          }
        }
      }
      //  The reason belongs to that calculation: leaving it clears it.
      if (!shownNow && !lastShown.empty()) {
        p_feedback->clearMessage();
        lastShown.clear();
      }

      // Only work on project, not even virtual documents
      // @todo shouldn't use hardwired string
      // A local folder has one user, so there is nobody to grant access.
      enableMenuItem(menuBar, wxID_CHANGE_ACCESS,
                      isProject && !itemData->getUrl().isLocal());
      enableMenuItem(menuBar, wxID_CUT, !isSystemFolder);
      enableMenuItem(menuBar, wxID_RENAME, !isSystemFolder);
      enableMenuItem(menuBar, wxID_DELETE, !isSystemFolder);
      enableMenuItem(menuBar, wxID_VIEW_PROPERTY, true);
      enableMenuItem(menuBar, wxID_ANNOTATION, !isSystemFolder);
      enableMenuItem(menuBar, wxID_DUPLICATE, isCalculation ||
                     dynamic_cast<MdTask*>(itemData->getResource()) != 0);
      enableMenuItem(menuBar, wxID_DUPLICATE2, isCalculation);
      {
        //  Offered where it works; elsewhere the help line says why not.
        string why;
        bool can = isCalculation && OrbitalGuess::available(
            dynamic_cast<TaskJob*>(itemData->getResource()), why);
        enableMenuItem(menuBar, wxID_DUPLICATE3, can);
        wxMenuItem *item = menuBar->FindItem(wxID_DUPLICATE3);
        if (item)
          item->SetHelp(can ? _("Duplicate with the last geometry and start "
                                "from this calculation's final orbitals")
                            : wxString(why));
      }
      enableMenuItem(menuBar, wxID_TEST_SUITE, isCalculation);
      enableTool(p_toolBar, wxID_CUT, !isSystemFolder);
      enableTool(p_toolBar, wxID_RENAME, !isSystemFolder);
      enableTool(p_toolBar, wxID_DELETE, !isSystemFolder);
      enableTool(p_toolBar, wxID_VIEW_PROPERTY, true);
      enableTool(p_toolBar, wxID_DOWNLOAD, isDocument);

      // dynamically populate the Run Management menu
      // The run management menu resource always iterates from top to bottom.
      // The submenus are indicated by double colon's, ::, where the left side
      // is the name of the submenu and the right side is a field in that menu.
      // Which tasks get displayed depends on the state of the calculation.
      wxMenu * rmTaskMenu = menuBar->GetMenu(menuBar->FindMenu(_("Run Mgmt")));
      getRunMgmtMenu(*rmTaskMenu, itemData);
      enableTopMenu(menuBar, _("Run Mgmt"),
                         rmTaskMenu->GetMenuItemCount()>0, p_menuTitlesChanged);

      // dynamically populate the Tool menu
      wxMenu * toolMenu = menuBar->GetMenu(menuBar->FindMenu(_("Tools")));
      getToolsMenu(*toolMenu, itemData);
      enableTopMenu(menuBar, _("Tools"),
                         toolMenu->GetMenuItemCount()>0, p_menuTitlesChanged);
    }
    else {
      enableTopMenu(menuBar, _("Run Mgmt"), false, p_menuTitlesChanged);
      enableTopMenu(menuBar, _("Tools"), false, p_menuTitlesChanged);
      enableMenuItem(menuBar, wxID_CHANGE_ACCESS, false);
      enableMenuItem(menuBar, wxID_CUT, false);
      enableMenuItem(menuBar, wxID_RENAME, false);
      enableMenuItem(menuBar, wxID_DELETE, false);
      enableMenuItem(menuBar, wxID_VIEW_PROPERTY, false);
      enableMenuItem(menuBar, wxID_ANNOTATION, false);
      enableMenuItem(menuBar, wxID_DUPLICATE, false);
      enableMenuItem(menuBar, wxID_DUPLICATE2, false);
      enableMenuItem(menuBar, wxID_DUPLICATE3, false);
      enableMenuItem(menuBar, wxID_TEST_SUITE, false);
      enableTool(p_toolBar, wxID_CUT, false);
      enableTool(p_toolBar, wxID_RENAME, false);
      enableTool(p_toolBar, wxID_DELETE, false);
      enableTool(p_toolBar, wxID_VIEW_PROPERTY, false);
      enableTool(p_toolBar, wxID_DOWNLOAD, false);
    }
  }
}


void CalcMgr::redrawStateIcons()
{
  p_treeCtrl->recolorStatePixmaps();
  p_treeCtrl->Refresh();
}


string CalcMgr::getTreeSelectionName()
{
  string ret = "";
  WxResourceTreeItemData * sel = p_treeCtrl->getSelection();
  if (sel) {
    EDSIServerCentral servers;
    ret = servers.mapURLtoName(sel->getUrl());
  }
  return ret;
}


void CalcMgr::setTreeStyle()
{
  int treeStyle = p_treeCtrl->GetWindowStyleFlag();
  if (GetMenuBar()->IsChecked(wxID_SHOW_LINE))
    treeStyle = treeStyle & (~wxTR_NO_LINES);
  else
    treeStyle = treeStyle | wxTR_NO_LINES;
  p_treeCtrl->SetWindowStyle(treeStyle);
}


void CalcMgr::focusOnTree()
{
  p_focusOnTree = true;
  onSelectionChange(false);
  // p_contextPanel can still be null here -- confirmed via a real core
  // dump: the tree control can receive a GTK focus-in event (wired to
  // this via TreeEvtHandler::OnSetFocus()) during CalcMgr::Create()
  // itself, before p_contextPanel has been constructed yet. Same
  // premature-event-during-construction bug class as the Gateway/
  // GatewayPrefs layout crashes fixed earlier this session, just hit
  // via focus instead of size events.
  if (p_contextPanel != 0)
    p_contextPanel->loseFocus();
}


void CalcMgr::focusLeaveTree()
{
}

int CalcMgr::getProjectViewId() const
{
  return p_projectViewId;
}


void CalcMgr::refresh()
{
  WxResourceTreeItemData * itemData = p_treeCtrl->getSelection();
  p_treeCtrl->refresh(itemData);
  onSelectionChange(true);
}


/**
 * Start window disabler and busy cursr.
 *
 * The Organizer itself stays enabled: on MSW disabling the active window
 * hands the activation to the window behind it (another program), so a
 * prompt shown next opened behind that window, and the tree lost its focus
 * and repainted its selected rows.  A modal dialog disables the Organizer
 * itself while it runs.
 *
 * return true if it is already disabled.
 */
int CalcMgr::startDisabler()
{
  if (p_disablerCount == 0 && p_disabler == 0 && p_cursor == 0) {
    p_focusOnTreeBeforeDisabler = p_focusOnTree;
    p_disabler = new wxWindowDisabler(this);
    p_cursor = new wxBusyCursor();
  }

  return ++p_disablerCount;
}


/**
 * Stop window disabler and busy cursr.
 *
 * return true if it is already disabled.
 */
int CalcMgr::stopDisabler()
{
  if (p_disablerCount == 1) {
    delete p_disabler;
    p_disabler = 0;
    delete p_cursor;
    p_cursor = 0;
    if (p_focusOnTreeBeforeDisabler)
      p_treeCtrl->SetFocus();
  }

  return --p_disablerCount;
}


/**
 * Handle drop on tree node operation.
 * Figure out which node it is and do the move/copy operation based on def.
 */
wxDragResult CalcMgr::dropInTree( const vector<EcceURL> & urls,
                                  wxCoord x, wxCoord y, wxDragResult def )
{
  //  cerr << "In dropInTree\n";
  //  cerr << "num of url: " << urls.size() << endl;

  if (urls.size() == 0) {
    //    cerr << "No url passed in\n";
    return wxDragCancel;
  }

  wxPoint pt(x, y);
  int hitFlag = wxTREE_HITTEST_ONITEMLABEL| wxTREE_HITTEST_ONITEMICON;
  wxTreeItemId id = p_treeCtrl->HitTest(pt, hitFlag);

  WxResourceTreeItemData * target = dynamic_cast<WxResourceTreeItemData*>
    (p_treeCtrl->GetItemData(id));

  // Check if dropped target exists
  if (target == 0)
    return wxDragCancel;

  // Check for authentication to access the target.
  bool authSucceed = true;
  if (p_treeCtrl->GetChildrenCount(target->GetId(), false) == 0) {
    authSucceed = p_treeCtrl->loadChildren(target, false);
  }
  
  // Veto drop to unauthorized area.
  if (!authSucceed)
    return wxDragCancel;

  EcceURL targetUrl = target->getUrl();
  EcceURL currentSelection = p_treeCtrl->getSelection()->getUrl();

  startDisabler();
  for (int i=0; i<urls.size(); ++i) {
    EcceURL sourceUrl = urls[i];
    Resource * source = EDSIFactory::getResource(sourceUrl);

    if (!source) {
      //      cerr << "Can't generate Resource based on the url\n";
      def = wxDragCancel;
      break;
    }
    
    if (targetUrl == sourceUrl || targetUrl == sourceUrl.getParent() ||
        targetUrl.isChildOf(sourceUrl)) {
      //      cerr << "Invalid move/copy operation.\n";
      def = wxDragCancel;
      break;
    }

    string sourceName = sourceUrl.getFilePathTail();
    EcceURL newUrl = targetUrl.getChild(sourceName);

    if (def == wxDragMove && !sourceUrl.isLocal())
      def = wxDragCopy;

    if (def == wxDragMove) {
      if (source->move(newUrl)) {
        notifyRename(sourceUrl.toString(), newUrl.toString(), "");
      }
      else {
        ewxMessageDialog dlg(this, source->messages(),
                             "Move Failed!", wxOK|wxICON_EXCLAMATION, 
                             wxDefaultPosition);
        dlg.ShowModal();
        def = wxDragCancel;
        break;
      }
    }
    else if (def == wxDragCopy) {
      if (source->copy(newUrl)) {
        notifyCreate(newUrl.toString());
      }
      else {
        ewxMessageDialog dlg(this, source->messages(),
                             "Copy Failed!", wxOK|wxICON_EXCLAMATION, 
                             wxDefaultPosition);
        dlg.ShowModal();
        def = wxDragCancel;
        break;
      }
    }
  }
  
  p_treeCtrl->refresh(target);

  if (def == wxDragMove) {
    WxResourceTreeItemData * sourceDir =
      findNode(urls[0].getParent(), false, false);
    if (sourceDir)
      p_treeCtrl->refresh(sourceDir);
  }
  setContextPanel();

  stopDisabler();
  return def;
}


/**
 * Turn on/off drop highlight tree nodes
 */
void CalcMgr::enableTreeDropHighlight(bool flag, wxCoord x, wxCoord y)
{
  // If turn off flag is set
  if (!flag) {
    if (p_highlightId.IsOk()) {
      p_treeCtrl->SetItemDropHighlight(p_highlightId, false);
      p_highlightId = wxTreeItemId();
    }
  }
  else {
    wxPoint pt(x, y);
    int hitFlag = wxTREE_HITTEST_ONITEMLABEL| wxTREE_HITTEST_ONITEMICON;
    wxTreeItemId id = p_treeCtrl->HitTest(pt, hitFlag);
    
    // Turn off the highlight on the previously node
    if (id != p_highlightId) {
      if (p_highlightId.IsOk()) {
        p_treeCtrl->SetItemDropHighlight(p_highlightId, false);
        p_highlightId = wxTreeItemId();
      }
      // Turn on the highlight and save the current node
      if (id.IsOk()) {
        p_treeCtrl->SetItemDropHighlight(id, true);
        p_highlightId = id;
      }
    }
  }
}


bool CalcMgr::canDropOnTree(wxCoord x, wxCoord y)
{
  //  cerr << "Test if can drop on " << x << " and " << y << "\n";
  wxPoint pt(x, y);
  int hitFlag = wxTREE_HITTEST_ONITEMLABEL| wxTREE_HITTEST_ONITEMICON;
  wxTreeItemId id = p_treeCtrl->HitTest(pt, hitFlag);
  if (id.IsOk()) {
    WxResourceTreeItemData * itemData = dynamic_cast<WxResourceTreeItemData*>
      (p_treeCtrl->GetItemData(id));
    //    cerr << "On top of item " << itemData->getUrl().toString() << endl;
    // logic about whether it is ok to drop on it
    return (!itemData->getUrl().isSystemFolder() ||
            itemData->getUrl().getParent().isEcceUsers() ||
            itemData->getUrl().isLocalDataHome())
      && (itemData->getResource()->getResourceType() !=
          ResourceDescriptor::RT_DOCUMENT);
  }

  /*
    Expand button thing doesn't work. Maybe use a timer to auto expend?
  hitFlag = wxTREE_HITTEST_ONITEMBUTTON;
  id = p_treeCtrl->HitTest(pt, hitFlag);
  if (id.IsOk()) {
    WxResourceTreeItemData * itemData = dynamic_cast<WxResourceTreeItemData*>
      (p_treeCtrl->GetItemData(id));
    cerr << "On top of expand button of " << itemData->getUrl().toString() << endl;
    // Should expend the item here
    return true;
  }
  */

  return false;
}


TreeEvtHandler::TreeEvtHandler(CalcMgr * calcMgr)
{
  Connect(wxEVT_SET_FOCUS,
          wxFocusEventHandler(TreeEvtHandler::OnSetFocus));
  Connect(wxEVT_KILL_FOCUS,
          wxFocusEventHandler(TreeEvtHandler::OnKillFocus));
  p_calcMgr = calcMgr;
}


TreeEvtHandler::~TreeEvtHandler()
{
  Disconnect(wxEVT_SET_FOCUS,
             wxFocusEventHandler(TreeEvtHandler::OnSetFocus));
  Disconnect(wxEVT_KILL_FOCUS,
             wxFocusEventHandler(TreeEvtHandler::OnKillFocus));
  p_calcMgr = 0;
}


void TreeEvtHandler::OnSetFocus(wxFocusEvent & event)
{
  p_calcMgr->focusOnTree();
  event.Skip();
}


void TreeEvtHandler::OnKillFocus(wxFocusEvent & event)
{
  p_calcMgr->focusLeaveTree();
  event.Skip();
}
