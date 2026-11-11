/**
 * @file
 *
 * @author Lisong Sun
 *
 */
#ifndef _CALCMGR_H_
#define _CALCMGR_H_

#include <wx/weakref.h>
#include <wx/timer.h>
#include <wx/fswatcher.h>
#include <functional>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include "wxgui/WxTailWindow.H"
#include <wx/frame.h>
#include <wx/toolbar.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/dnd.h>

#include "util/ProgressEvent.H"
#include "util/TDateTime.H"
#include "util/JMSPublisher.H"
#include "util/EditListener.H"

#include "comm/LaunchBatch.H"
#include "LaunchSelected.H"
#include "dsm/TaskJob.H"
#include "dsm/IPropCalculation.H"
#include "dsm/ResourceType.H"

#include "wxgui/ewxTreeCtrl.H"
#include "wxgui/WxResourceTreeCtrl.H"
#include "wxgui/WxDavAuth.H"
#include "wxgui/WxAccessControl.H"
#include "wxgui/WxCalcImportClient.H"

#include "CalcMgrGUI.H"
#include "ContextPanelInterface.H"
#include "WorkflowSteps.H"
#include "WxProperties.H"

class BookmarkEditor;
class Session;
class WxFind;
class GlobalPrefs;
class ewxPanel;
class ReleaseNotice;

class WxConfigureNwfsArchive;

//  Return code for the "Quit and Stop Server" button added to the Quit
//  confirm dialog (issue #97). Mirrors ID_GATEWAY_QUIT_STOP_SERVER in
//  Gateway.H; kept a separate constant rather than shared because the two
//  apps have no common header and the value only has to be unique within
//  the dialog it is used in.
#define ID_ORGANIZER_QUIT_STOP_SERVER 10104

#include "dsm/SmilesBatch.H"

class CalcMgr: public CalcMgrGUI
             , public EditListener
             , public WxDavAuth
             , public ProgressEventListener
             , public WxCalcImportClient
             , public JMSPublisher
{    
DECLARE_CLASS( CalcMgr )
  
public:

  /*
   * Constructor.
   * Shouldn't be used, yet it is not easy to define it away.
   */
  CalcMgr();

  /*
   * Constructor.
   * All the default parameters are defined in CalcMgrGUI class
   */
  CalcMgr(wxWindow* parent, wxWindowID id = SYMBOL_CALCMGRGUI_IDNAME,
          const wxString& caption = SYMBOL_CALCMGRGUI_TITLE,
          const wxPoint& pos = SYMBOL_CALCMGRGUI_POSITION,
          const wxSize& size = SYMBOL_CALCMGRGUI_SIZE,
          long style = SYMBOL_CALCMGRGUI_STYLE );

  bool Create(wxWindow* parent, wxWindowID id = SYMBOL_CALCMGRGUI_IDNAME,
              const wxString& caption = SYMBOL_CALCMGRGUI_TITLE,
              const wxPoint& pos = SYMBOL_CALCMGRGUI_POSITION,
              const wxSize& size = SYMBOL_CALCMGRGUI_SIZE,
              long style = SYMBOL_CALCMGRGUI_STYLE );

  ~CalcMgr();

  // Issue #150: called by CalcMgrApp::msgMachRegChangedMCB() when the
  // Register Machines tool reports a save/delete, so the notice can
  // disappear (or, if the last machine was removed, reappear) without
  // waiting for the window to be reactivated.
  void refreshMachineNotice();

  /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME
  void OnCloseWindow( wxCloseEvent& event );

  /// wxEVT_CONTEXT_MENU event handler for ID_FRAME
  void OnContextMenu( wxContextMenuEvent& event );
  void menuSource(wxWindow *source);

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_IMPORT
  void OnNewStructureClick( wxCommandEvent& event );

  void OnImportClick( wxCommandEvent& event );

  void OnOpenExamplesClick( wxCommandEvent& event );
  /// File > Import GROMACS Run...
  void OnImportGromacsClick( wxCommandEvent& event );
  /// Imports runDir into the selection's project; "" with message on failure.
  string importGromacsFrom(const string& runDir, string& message);

  /// Imports the example set setId (general, computational) into its own
  /// project, or selects that project if it exists; returns the project's
  /// name, "" on failure.  created says whether it was imported now.
  string openExampleSet(const string& setId, string& message, bool& created);
  vector<string> removeOldExamples();
  /// At a user's first start, imports the general set into "Examples" once.
  void importExamplesOnce();

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXPORT
  void OnExportClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
  bool confirmAndQuit();

  /// One command of the ECCE_TEST_ORGANIZER hook (see CalcMgr::Create).
  void runTestCommand(const string& line);

  void OnExitClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE
  void OnDuplicateClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE2
  void OnDuplicate2Click( wxCommandEvent& event );

  /// "Duplicate with Final Orbitals": the last geometry and the orbitals
  void OnDuplicate3Click( wxCommandEvent& event );
  void OnLaunchSelectedClick( wxCommandEvent& event );
  void OnReconnectSelectedClick( wxCommandEvent& event );
  // Re-Parse Outputs (#651): parsers and progress record over the stored
  // outputs of the selected calculations; no login, state and dates kept.
  void OnReparseOutputsClick( wxCommandEvent& event );
  /// "New series from this...": one calculation per structure with the
  /// template's setup (#621); `sources` are .xyz paths or calculation URLs.
  void OnNewSeriesClick( wxCommandEvent& event );
  void newSeries(const EcceURL& templ, const vector<string>& sources,
                 string *outcome);
  /// Batch Add from SMILES (#961): one calculation per row in folder; the
  /// report, and the new calculations in made.
  void batchSmiles(const EcceURL& folder, const string& templ,
                   const vector<SmilesBatch::Row>& rows, bool replace,
                   bool interactive, string& report, vector<EcceURL>& made);
  /// MLP site screening (#728): the template's host and one calculation per
  /// site in a new folder, and the results of that folder as an ensemble.
  void screenSites(const EcceURL& templ, bool dialog, bool vacancy,
                   const string& element, double minHost, double minSep,
                   double maxDist, string *outcome);
  void siteResults(const EcceURL& at, bool interactive, string *outcome);
  void OnTerminateSelectedClick( wxCommandEvent& event );
  void reparseOutputs(const vector<EcceURL>& urls, bool interactive,
                      string *outcome = 0);
  // A job store ended a re-parse ("done" or "failed").
  void reparseFinished(const EcceURL& url, const string& result);
  // Reconnect Selected (#601), same selection; `listOnly`/`shot` for tests.
  void reconnectSelected(const vector<EcceURL>& urls, bool interactive,
                         string *outcome = 0, const string& shot = "",
                         bool listOnly = false, vector<string> *lines = 0,
                         const string& readyJobId = "");
  // Ready -> Submitted with a queue job id (ecce-reconnect jobid=N, the dialog).
  bool attachJobId(TaskJob *job, const string& jobId, string& why);
  // Launch steps run on a worker thread; a login it needs is asked here.
  bool getAuthorization(AuthEvent& event);
  // Launch Selected (#492); `outcome` and `shot` only for the test hook.
  vector<EcceURL> launchSelection();
  void launchSelected(const vector<EcceURL>& urls, bool interactive,
                      string *outcome = 0, const string& shot = "",
                      const LaunchBatch::Target& target = LaunchBatch::Target());
  // Test hook: the batch without dialogs, left running; false if one runs.
  bool launchSelectedAsync(const vector<EcceURL>& urls, string *outcome,
                           bool askForSettings = false);
  bool askForMissingSettings(vector<LaunchSelected::Row>& rows);
  string launchBatchStatus() const;
  // ecce-launch (#687): Launch Selected without dialogs for a request from
  // another process. One line per calculation, "name\tok\tjob id" or
  // "name\tno\treason"; false, with lines empty, while a batch submits.
  bool launchForCommand(const vector<EcceURL>& urls,
                        const LaunchBatch::Target& target,
                        vector<string>& lines);
  // lastGeom: end a running job first and start the copy from its last
  // stored geometry. cores > 0 and a non-empty wall ("D H:M" or H:M) are
  // stored as the copy's launch settings.
  bool rerunForCommand(const vector<EcceURL>& urls, bool launch,
                       const LaunchBatch::Target& target, vector<string>& lines,
                       bool lastGeom = false, unsigned long cores = 0,
                       const string& wall = "");
  // ecce-launch --move (#830): cancel a still-queued Slurm job and submit
  // the calculation on target.machine; same line format as launchForCommand.
  bool moveLaunchForCommand(const vector<EcceURL>& urls,
                            const LaunchBatch::Target& target,
                            vector<string>& lines);
  // ecce-terminate (#695): Terminate Selected without the dialog; same line
  // format as launchForCommand. False, with lines empty, while a batch submits.
  bool terminateForCommand(const vector<EcceURL>& urls, const string& reason,
                           vector<string>& lines);
  // ecce-annotate (#742): the Annotation field set without the editor; same
  // line format ("name\tok\t" or "name\tno\treason"). Never refused.
  bool annotateForCommand(const vector<EcceURL>& urls, const string& text,
                          vector<string>& lines, const string& key = "");
  // ecce-move / ecce-rename (#824) and ecce-copy (#890): verb move, rename or
  // copy; the Organizer's own operation, same line format.
  bool moveForCommand(const string& verb, const vector<EcceURL>& urls,
                      const string& second, vector<string>& lines);
  // ecce-mkdir (#870): make each URL as a project or folder; with parents
  // also the missing ones above it. Lines "name\tok\tURL" (made),
  // "name\tok\texists" or "name\tno\treason".
  bool mkdirForCommand(const vector<EcceURL>& urls, bool parents,
                       vector<string>& lines);
  // ecce-workflow (#807): "ok <workflow URL>" or "error: reason".
  string workflowForCommand(const string& verb, const EcceURL& calc,
                            const string& second);
  // ecce-workflow create (#874): TARGET is a workflow URL or a new name;
  // links "A:B[=role]" by calculation name.  "ok <workflow URL> ..." or
  // "error: reason".
  string workflowCreateForCommand(const string& target,
                                  const vector<EcceURL>& calcs,
                                  const vector<string>& links, bool autoLinks);
  // ecce-reconnect: Reconnect Selected without the dialogs; same line format.
  bool reconnectForCommand(const vector<EcceURL>& urls, vector<string>& lines,
                           const string& jobId = "");
  bool anyReconnectable(const vector<EcceURL>& urls);
  // ecce-fetch: run-directory files into the Outputs (or a listing).
  bool fetchForCommand(const EcceURL& url, const vector<string>& patterns,
                       bool listOnly, vector<string>& lines);
  // A Launch Selected batch is submitting (#640): the calculations in it
  // must not be deleted, moved, launched or stopped meanwhile.
  bool batchRunning() const { return p_batch != 0; }

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CUT
  void OnCutClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_COPY
  void OnCopyClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PASTE
  void OnPasteClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_RENAME
  void OnRenameClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DELETE
  void OnDeleteClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ANNOTATION
  void OnAnnotationClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_SERVER
  void OnAddServerClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CHANGE_ACCESS
  void OnChangeAccessClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CONFIGURE_NWFS_ARCHIVE
  void OnConfigureNwfsArchiveClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOMEASROOT
  void OnHomeasrootClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_TOOLBAR
  void OnShowToolbarClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LEGEND
  void OnShowLegendClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_INTERNAL
  void OnShowInternalClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_RUNSTATES
  void OnShowRunstatesClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LINE
  void OnShowLineClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SORT_TYPE
  void OnSortTreeClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_BOOKMARK
  void OnAddBookmarkClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_MANAGE_BOOKMARK
  void OnManageBookmarkClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_TEST_SUITE
  void OnTestSuiteClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_VIEW_PROPERTY
  void OnViewPropertyClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOME
  void OnHomeClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UP
  void OnUpClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_BACKWARD
  void OnBackwardClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FORWARD
  void OnForwardClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FIND
  void OnFindClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_REFRESH
  void OnRefreshClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UPLOAD
  void OnUploadClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DOWNLOAD
  void OnDownloadClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_MENU_HELP
  void OnHelpClick( wxCommandEvent& event );
  void OnPreferencesClick( wxCommandEvent& event );

  /// Help > Running Codes on This Computer
  void OnRunningCodesClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_MENU_SUPPORT
  void OnMenuSupportClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_TREE_SEL_CHANGED event handler for ID_TREECTRL
  void OnTreectrlSelChanged( wxTreeEvent& event );

  /// wxEVT_COMMAND_TREE_SEL_CHANGING event handler for ID_TREECTRL
  void OnTreectrlSelChanging( wxTreeEvent& event );

  /// wxEVT_COMMAND_TREE_BEGIN_DRAG event handler for ID_TREECTRL
  void OnTreectrlBeginDrag( wxTreeEvent& event );

  /// wxEVT_COMMAND_TREE_END_DRAG event handler for ID_TREECTRL
  void OnTreectrlEndDrag( wxTreeEvent& event );

  /// wxEVT_COMMAND_TREE_BEGIN_RDRAG event handler for ID_TREECTRL
  void OnTreectrlBeginRDrag( wxTreeEvent& event );

  /// wxEVT_COMMAND_TREE_DELETE_ITEM event handler for ID_TREECTRL
  void OnTreectrlDeleteItem( wxTreeEvent& event );
  
  /// wxEVT_COMMAND_TREE_ITEM_ACTIVATED event handler for ID_TREECTRL
  void OnTreectrlItemActivated( wxTreeEvent& event );

  /// wxEVT_COMMAND_TREE_ITEM_COLLAPSED event handler for ID_TREECTRL
  void OnTreectrlItemCollapsed( wxTreeEvent& event );

  /// wxEVT_COMMAND_TREE_ITEM_COLLAPSING event handler for ID_TREECTRL
  void OnTreectrlItemCollapsing( wxTreeEvent& event );

  /// wxEVT_COMMAND_TREE_ITEM_EXPANDED event handler for ID_TREECTRL
  void OnTreectrlItemExpanded( wxTreeEvent& event );

  /// wxEVT_COMMAND_TREE_ITEM_EXPANDING event handler for ID_TREECTRL
  void OnTreectrlItemExpanding( wxTreeEvent& event );

  /// wxEVT_COMMAND_TREE_KEY_DOWN event handler for ID_TREECTRL
  void OnTreectrlKeyDown( wxTreeEvent& event );

  /// wxEVT_COMMAND_TREE_ITEM_MENU event handler for ID_TREECTRL
  void OnTreectrlItemMenu( wxTreeEvent& event );

  void saveSettings();
  void restoreSettings();
  void setContextPanel();
  /// Workflow steps (WorkflowSteps.H): the code buttons and the hooks.
  /// ask: show the New-step dialog; false takes its defaults.
  Resource *newWorkflowStep(Session *workflow, ResourceType *type,
                            Resource *source, bool ask, string *outcome,
                            const string& name = "",
                            const WorkflowSteps::Carry& carry = WorkflowSteps::Carry());
  /// A step taking several sources (G16 QST2/QST3, ORCA NEB-TS); ask shows
  /// the join dialog with the given roles as its defaults.
  Resource *newWorkflowJoin(Session *workflow, ResourceType *type,
                            const vector<WorkflowSteps::JoinSource>& sources,
                            bool ask, string *outcome, const string& name = "");
  /// Apply Recipe: the variant's steps in the workflow; the tree and view
  /// follow.  Empty on failure, with *outcome saying why.
  /// A calculation's structure was stored (JMS ecce_url_subject): a recipe
  /// step's workflow gets the rules and levels again.  What was stored, or "".
  string structureStored(const EcceURL& url);
  vector<Resource*> newRecipeSteps(Session *workflow, const Recipe& recipe,
                                   const RecipeVariant& variant,
                                   const WorkflowSteps::Levels& levels,
                                   Resource *after, string *outcome);
  void launchReadySteps(Session *workflow, bool interactive, string *outcome,
                        const string& shot);
  /// Launch one step if it is ready (the play button, #736); otherwise say
  /// why not.
  void launchStep(Session *workflow, Resource *step, bool interactive,
                  string *outcome);
  /// Auto-run (#736): launch what is ready in a workflow that has it on
  /// and whose run the user started with Launch Ready Steps (#926).
  void autoRunWorkflow(Session *workflow);
  /// Close a workflow's view: select the folder it is in, which shows that
  /// folder's calculation table.
  void closeWorkflowView(const string& workflowUrl);
  void createResource(ResourceType * resource,
                      WxResourceTreeItemData * parent = NULL,
                      const bool& bypassNamePrompt = false);

  void addRoot(const string& name, const string& url);
  WxFeedback * getFeedback();
  void redrawStateIcons();
  void updateName(const EcceURL & oldUrl, const EcceURL & newUrl);
  void updateUrl(const EcceURL & url);
  void updateAddNode(const EcceURL & url, bool select = false);
  void updateRemoveNode(const EcceURL & url);
  // The broker's ecce_url_* notices.  One Save or Launch sends several in a
  // row, and each rebuilt the tree and the summary (a visible flicker on
  // MSW); they are collected and applied together under one Freeze.
  enum UpdateKind { UPDATE_CHANGED, UPDATE_ADDED, UPDATE_REMOVED,
                    UPDATE_ADDED_SELECT };
  void queueUpdate(UpdateKind kind, const EcceURL & url);
  // ecce_url_progress (#284): the newest snapshot per calculation, kept so
  // that selecting a running calculation shows it at once.  Empty text
  // forgets it (a new run).  Only the shown summary's Progress box is
  // repainted; nothing else is reloaded.
  void progressUpdate(const EcceURL & url, const string & text);
  // Drops the snapshot held for a calculation that is not running (a rerun).
  void forgetProgress(const EcceURL & url);
  bool liveProgress(const EcceURL & url, string & text,
                    double & received) const;
  // A finished calculation with an output but no stored snapshot (it ran
  // before progress was kept, or was imported): fold the output through the
  // job monitor's [PROGRESS] entries in the background, once per session
  // per calculation, store the result as the monitor would, and show it.
  void backfillProgress(const EcceURL & url);
  // The project table's Progress cell: JobProgress::shortLine() of the live
  // snapshot, else the stored one; empty unless the calculation is running.
  string progressCellText(Resource * resource) const;
  string totalEnergyCellText(Resource * resource) const;
  // Out of contact (#284): the job store's "contact" ("lost <epoch last
  // heard>" or "ok") on ecce_url_progress, with the snapshot it holds.
  void contactUpdate(const EcceURL & url, const string & contact,
                     const string & progress);
  // A running calculation whose monitor is out of contact: the live
  // notice, else the stored ecce:contact.  lastHeard: seconds since 1970.
  // queued: a submitted job counts too (the Progress box only).
  bool outOfContact(Resource * resource, double & lastHeard,
                    bool queued = false) const;
  // When the monitor last confirmed a queued job, seconds since 1970 (0
  // unknown): the live notice, else the stored ecce:contact "ok <epoch>".
  double lastChecked(Resource * resource) const;
  // What the last "Check Job on Machine" said, one sentence a line.
  vector<string> machineCheckLines(const EcceURL & url) const;
  // The Progress box's buttons: 0 Reconnect, 1 Check Job on Machine,
  // 2 Show Output So Far (the Tail window).
  void progressAction(int action, WxResourceTreeItemData * itemData);
  void setMessage(const string & message,
                  WxFeedback::Severity severity=WxFeedback::WARNING,
                  bool clear = false);
  // Session start (#208): catch up calculations waiting for login.
  void catchUpWaitingJobs();
  bool stopOldMonitor(const EcceURL& url);
  string p_reconnectNote;   // the single Reconnect's last message (test hook)
  // Re-parses started and not yet ended (URL -> name), and those that failed.
  std::map<string, string> p_reparsing;
  vector<string> p_reparseFailed;
  int p_reparseDone = 0;

  // findNode, re-reading from the nearest known ancestor when the node is missing.
  WxResourceTreeItemData * findNodeFresh(const EcceURL &targetUrl,
                                         bool openWhenFound);
  WxResourceTreeItemData * findNode(const EcceURL &targetUrl,
                                    bool openWhenFound = true,
                                    bool loadFromServer = true);
  void onSelectionChange(bool selectInTree = false);
  void updateSelectionMenus();
  struct MenuBarHold;
  void focusOnTree();
  void focusLeaveTree();
  void copySelectionToClipboard(bool deleteAfterPaste);
  void paste();
  // Move to... / Copy to... (#929): the same paste, into a chosen folder.
  string moveToRefusal(const vector<EcceURL>& sources, const EcceURL& dest);
  string transferTo(bool cut, const EcceURL& dest);
  void chooseAndTransfer(bool cut);
  void dragScrollTick();
  void deleteSelection();
  bool selectionHasSystemFolder();
  string getTreeSelectionName();
  bool getTypeAndState(WxResourceTreeItemData * itemData, 
                       ResourceType *& resType,
                       ResourceDescriptor::RUNSTATE & state);
  void getContextMenu(wxMenu & menu);
  /// Runs f; an exception it throws comes back as text (its type and
  /// message) instead of leaving a wx event handler and ending the app.
  static string guarded(const std::function<void()>& f);

  void getFileMenu(   wxMenu & menu, WxResourceTreeItemData * itemData );
  void getHistoryMenu(wxMenu & menu, WxResourceTreeItemData * itemData );
  void getRunMgmtMenu(wxMenu & menu, WxResourceTreeItemData * itemData );
  void getToolsMenu(  wxMenu & menu, WxResourceTreeItemData * itemData );
  void clearMenu(wxMenu & menu);
  int getProjectViewId() const;
  void refresh();
  int startDisabler();
  int stopDisabler();

  // TreeCtrl drag and drop related functions
  wxDragResult dropInTree( const vector<EcceURL> & urls,
                           wxCoord x, wxCoord y, wxDragResult def );
  void enableTreeDropHighlight(bool flag, wxCoord x = 0, wxCoord y = 0);
  bool canDropOnTree(wxCoord x, wxCoord y);

protected:

  /// wxEVT_COMMAND_MENU_SELECTED event handler for all ID_RESOURCE
  void OnNewResourceClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for all ID_RMTASK
  void OnRmTaskClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for all ID_TOOL
  void OnToolClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for all ID_HISTORY
  void OnHistoryClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for all ID_BOOKMARK
  void OnBookmarkClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for all ID_PROJECTVIEW
  void OnProjectViewClick( wxCommandEvent& event );

  void OnPanelSelection( wxCommandEvent& event );

  void initializeGUI();
  void setTreeStyle();
  void initializeTreeData();
  // Roots for the ECCE servers in ~/.ECCE/AddedServers (#767).
  void syncServerRoots();
  bool connectServer(const string& ref);
  bool connectSsh(const string& address);
  bool rememberedServer(const string& ref) const;
  void disconnectServer(const string& ref);
  void serverRootMenu(wxMenu& menu, const string& ref);
  void serversMenu(wxMenu& menu);
  void showServersMenu();
  int p_serversToolId = wxID_ANY;
  string crossTreeRefusal(const vector<EcceURL>& sources, const EcceURL& dest);
  bool isCrossTree(const EcceURL& a, const EcceURL& b);
  string copyAcrossTrees(const EcceURL& src, const EcceURL& destFolder,
                         bool move, bool unique, EcceURL& made);
  bool refuseAddedLaunch(const vector<EcceURL>& urls);
  void tellRefusal(const string& msg);

  void progressNotice(ProgressEvent& event);
  bool getSelection();
  bool isValidName(const string& name);
  bool checkMovable(WxResourceTreeItemData * node);
  // Issue #93: selection-independent entries on the Tools menu.
  void addGlobalTools(wxMenu & menu);
  WxResourceTreeItemData * nearestContainer(WxResourceTreeItemData * node,
                                            ResourceType * resType);
  void OnGlobalTool(wxCommandEvent& event);
  // Tools > Energy Calculator (#573).
  class CompareEnergiesDialog *newCompareEnergiesDialog(const vector<EcceURL>& sel);
  void OnCompareEnergies(wxCommandEvent& event);
  bool p_compareEnergiesWired = false;
  // File > Export > Supporting Information (#625).
  bool p_siExportWired = false;
  // Set by the "exportmenu" test hook: OnExportClick records what it would
  // have asked or said here instead of opening a modal dialog.
  std::string *p_exportTrace = nullptr;
  std::string p_exportOut, p_exportTable;
  void OnRank(wxCommandEvent& event);
  // Tools > Allocations (#613).
  void OnAllocations(wxCommandEvent& event);
  bool p_allocationsWired = false;
  // Tools > Batch Add from SMILES (#961).
  void OnBatchSmiles(wxCommandEvent& event);
  bool p_batchSmilesWired = false;
  // Ids already wired to OnGlobalTool, so rebuilding the menu does not
  // stack duplicate handlers.
  std::set<int> p_globalToolIds;

  // Issue #150: nothing told a first-time user that a machine has to be
  // registered before any calculation can be launched.  A non-modal
  // notice sits between the toolbar and the tree, shown only while
  // RefMachine::referenceNames() is empty, with a button that reuses the
  // exact same launch path as the Tools menu's own Register Machines
  // entry (see addGlobalTools()/OnGlobalTool()).  refreshMachineNotice()
  // itself is public -- see above.
  void buildMachineNotice();
  void machineNoticeRegisterClickCB(wxCommandEvent& event);
  void OnActivate(wxActivateEvent& event);

  // Auto-refresh (#889): the folder being shown follows changes made outside
  // ECCE.  Local folders are watched; server and SSH folders are polled.
  void autoRefreshArm();
  void autoRefreshCheck();
  void autoRefreshMark();
  void autoRefreshTick();
  bool autoRefreshIdle() const;
  string autoRefreshStamp(const EcceURL& url);
  WxResourceTreeItemData *autoRefreshFolder();
  wxFileSystemWatcher *p_autoWatcher = 0;
  wxTimer *p_autoDebounce = 0;   // settles a burst of events, then retries while busy
  wxTimer *p_autoPoll = 0;       // 30 s, remote folders only
  string p_autoUrl;              // the folder armed
  string p_autoStamp;            // its last known modification time (remote)
  bool p_autoDirty = false;
  bool p_autoLocal = false;
  int p_autoRetry = 0;
  string p_autoTried;
  int p_uiBusy = 0;              // popup menu or drag in progress
  int p_autoReloads = 0;         // test hook
  ewxPanel   *p_machineNoticePanel;
  ReleaseNotice *p_releaseNotice;   // #273
  LaunchSelected::Batch *p_batch = 0;
  string p_batchReport;
  void startBatch(const vector<LaunchSelected::Row>& rows, bool showReport);
  bool refuseDuringBatch(const string& what);
  bool refuseMoveOfActiveJob(Resource *res, const string& what);
  string activeJobRefusal(Resource *res, const string& what);
  // Make Workflow... / Move into Workflow > (#807).
  string workflowMoveRefusal(Resource *calc);
  string moveIntoWorkflow(Resource *calc, Session *wf, const string& name,
                          EcceURL *wfUrl);
  void showInWorkflow(const EcceURL& wfUrl, const EcceURL& step);
  void makeWorkflowFromSelection(Session *into);
  // Make Workflow from Selected... / Move Selected into Workflow > (#874).
  struct WorkflowLinkSpec { EcceURL from, to; string name; };
  string severalWorkflowRefusal(const vector<EcceURL>& calcs, Session *into,
                                vector<Resource*> *found = 0);
  vector<WorkflowSteps::ProposedLink> proposeWorkflowLinks(
      const vector<Resource*>& calcs, Session *into);
  string moveSeveralIntoWorkflow(const vector<EcceURL>& calcs, Session *into,
                                 const string& name,
                                 const vector<WorkflowLinkSpec>& links,
                                 EcceURL *wfUrl);
  void makeWorkflowFromRows(Session *into);
  bool p_quitAsking = false;        // the Quit dialog is open (#291)
  bool p_quitConfirmed = false;     // the user said yes; teardown pending

  void startApp(const string& app, int force, const string& url,
                const string& codename = "", int amulet = 0);
  void startPropertyEdit(const string& property);

  // Messaging stuff
  void notifyProperty(const string& url, const string& name, 
                      const string& value) const;
  void notifyCreate(const string& newurl) const;
  void notifyRemove(const string& url,
                    const string& contentType) const;
  void notifyRename(const string& oldurl, const string& newurl,
                    const string& contentType) const;

  // Run management menu callbacks
  void tailOutputFile();
  void shellInRunDir();
  void manageFiles();
  void terminateCalc();
  void checkJob();
  void checkJobOnMachine();
  void reconnectJob();
  void forceFail();
  void resetForRerun();
  void resetForRestart();
  void resetCompleted();
  void resetIncomplete();
  void resetImported();
  void resetKilled();
  void markReviewed();
  void markUnreviewed();
  void cleanRunDir();
  void viewRunLog();
  void viewOutputFile();
  void viewInputFile();

  // Run management methods, to support multiple selections
  // I felt a good design descision was to restrict the access to the tree
  // to the callbacks, and to use the callbacks as a way to filter out
  // multiple selections from the tree.
  // This also allows the functions to be called as regular functions rather 
  // than always being in the context of the tree.
  void tailOutputFile(  WxResourceTreeItemData*);
  string openTail(const string& calcName, const string& machine,
                  const string& shell, const string& user, const string& file);
  void shellInRunDir(   WxResourceTreeItemData*);
  void manageFiles(     WxResourceTreeItemData*);
  string terminateCalc( WxResourceTreeItemData*);
  void checkJob(        WxResourceTreeItemData*);
  void checkJobOnMachine(WxResourceTreeItemData*);
  void reconnectJob(    WxResourceTreeItemData*);
  bool resetForRerun(   WxResourceTreeItemData*,
                        ResourceDescriptor::RUNSTATE state = 
                        ResourceDescriptor::STATE_ILLEGAL);
  bool resetForRestart( WxResourceTreeItemData*);
  void resetCompleted(  WxResourceTreeItemData*);
  void resetIncomplete( WxResourceTreeItemData*);
  void resetImported(   WxResourceTreeItemData*);
  void resetKilled(     WxResourceTreeItemData*);
  void forceFail(       WxResourceTreeItemData*);
  void markReviewed(    WxResourceTreeItemData*);
  void markUnreviewed(  WxResourceTreeItemData*);
  void cleanRunDir(     WxResourceTreeItemData*);
  void viewRunLog(      WxResourceTreeItemData*);
  void viewOutputFile(  WxResourceTreeItemData*);
  void viewInputFile(   WxResourceTreeItemData*);

  // Run management helper methods also supporting multiple selections
  // Some of the run management functions have similar functionality.  these
  // functions here help to reuse code
  bool setState(WxResourceTreeItemData *itemData, 
                ResourceDescriptor::RUNSTATE state);
  bool setReviewed(WxResourceTreeItemData *itemData, bool flag);
  void checkNodeJobs(WxResourceTreeItemData *node);
  bool checkSingleJob(Resource *resource, string& message);

  // Other callback helper functions
  EcceURL upload(WxResourceTreeItemData *parent,  SFile *file);
  void download(WxResourceTreeItemData *itemData);
  void createTestSuite(WxResourceTreeItemData *itemData);
  void viewProperties(WxResourceTreeItemData *itemData);
  void resetWithLastGeom(WxResourceTreeItemData *itemData);
  WxResourceTreeItemData* duplicate(WxResourceTreeItemData* itemData,
                                    bool openWhenFound = true,
                                    const string& name = "");
  WxResourceTreeItemData* duplicateForRerun(WxResourceTreeItemData* itemData,
                                           bool select, const string& name = "",
                                           string *why = 0);

  // EditListener virtuals
  void processEditCompletion(const EditEvent& ee);

  // WxCalcImportClient
  TaskJob *getContainer(const string& name);
  void importValidationComplete(TaskJob *ipc, bool status, string message);

  void updateAccessCtrlDlg();
  void updateFindDlg();
  void updateBookmarkMenu();

  void dumpClipboard();

  ewxPanel              * p_legend;
  WxFeedback            * p_feedback;
  ContextPanelInterface * p_contextPanel;

  WxAccessControl       * p_accessControlDialog;
  BookmarkEditor        * p_bookmarkEditor;
  BookmarkEditor        * p_serverListEditor;
  wxTimer               * p_serverTimer = 0;
  long                    p_serverListStamp = -1;
  WxFind                * p_find;
  GlobalPrefs           * p_prefs;
  
  WxConfigureNwfsArchive * p_nwfs;

  HistoryList           * p_hlist;

  // Not vector of WxResourceTreeItemData * because EcceURL will still be valid
  // after refresh, WxResourceTreeItemData * may get out of sync.
  // Every time use it, must call getSelection() first
  vector<EcceURL> p_currentSelection;

  // Not vector of WxResourceTreeItemData * because EcceURL will still be valid
  // after refresh, WxResourceTreeItemData * may get out of sync.
  vector<EcceURL> p_clipboard;

  // True means cut clicked, false means copy clicked.
  bool p_deleteAfterPaste;

  // The last failed paste's text, for the ECCE_TEST_ORGANIZER hook.
  string p_pasteMessage;

  bool p_focusOnTree;
  bool p_menusPending = false;   // MSW: menus rebuilt once per selection
  bool p_menuTitlesChanged = false;  // a top menu's state changed under a hold
  bool p_focusOnTreeBeforeDisabler;
  bool p_popupOnTree;

  int p_treeSort;
  int p_projectViewId;

  // No need for this, should be deleted soon.
  // This is single drag source in the treectrl, need a better solution.
  //  WxResourceTreeItemData * p_dragSource;

  WxResourceTreeItemData * p_panelNode;

  TDateTime::DateFormat   p_dateFormat;
  TDateTime::TimeFormat   p_timeFormat;

  wxWindowDisabler * p_disabler;
  int p_disablerCount;
  wxBusyCursor * p_cursor;
  wxTreeItemId p_highlightId;
  wxTimer *p_dragScrollTimer = 0;   // scrolls the tree while a drag is at its edge
  wxCoord p_dragX = 0, p_dragY = 0;

  static const char     * vMsgNoMoving;

  // The Tail window the ECCE_TEST_ORGANIZER hook opened.
  wxWeakRef<WxTailWindow> p_testTail;
  //  Set by the "new" test command: createResource reports its error
  //  here instead of in a modal dialog.
  string *p_testCreateError = 0;
  //  Set by the workflownew test command: a New-step dialog is not shown
  //  but logged here ("carry;" with a source, "nocarry;" without).
  string *p_testStepDialogs = 0;
  //  Set by the "duplicate" test command: the tree's selection and
  //  collapse events, in order.
  std::vector<string> *p_testTreeEvents = 0;

  void applyQueuedUpdates();
  std::vector<std::pair<UpdateKind, string> > p_queuedUpdates;
  // Steps auto-run launched whose state has not yet left Created, so a
  // second notice cannot launch them again.
  std::set<string> p_autoLaunched;
  // Workflows to look at again when the running launch batch ends.
  std::set<string> p_autoPending;
  bool p_autoBatch = false;
  wxTimer *p_updateTimer = 0;
  // For the "counts" test command.
  // True while openExampleSet() imports several outputs: the tree is rebuilt
  // once at the end, not after each calculation (#263).
  bool p_batchImport = false;
  // The workflow example being imported: target step -> source step (names).
  // A step joins its session, and the links are made, when its import is done.
  std::map<std::string, std::vector<std::pair<std::string, std::string> > >
    p_flowSources;   // target calc -> (source calc, link name)
  std::map<std::string, std::string> p_flowHessian;   // step -> .hess file
  std::map<std::string, std::pair<std::string, bool> > p_flowKeys;   // step -> key role, thermo
  void joinExampleWorkflow(Resource *step);
  std::string p_batchError;     // the first import that failed in the batch
  int p_testNotices = 0, p_testApplied = 0, p_testBatches = 0;
  // url -> (snapshot, when it arrived, seconds since 1970)
  std::map<string, std::pair<string, double> > p_liveProgress;
  // Progress backfills (backfillProgress): tried urls, and the running
  // monitors by pid -> (url, temp file prefix).
  std::set<string> p_backfillTried;
  std::map<long, std::pair<EcceURL, string> > p_backfills;
  void finishProgressBackfill(long pid);
  // url -> epoch last heard while out of contact, -1 in contact again
  std::map<string, double> p_contact;
  // url -> epoch of the last "ok <epoch>" for a queued job
  std::map<string, double> p_checked;
  std::map<string, vector<string> > p_machineCheck;
  // The tree icon, the shown summary and the table's cells of one
  // calculation, after its contact changed.
  void contactChanged(const EcceURL & url);
};


class TreeEvtHandler : public wxEvtHandler
{
public:
  TreeEvtHandler(CalcMgr * calcMgr);
  ~TreeEvtHandler();

protected:
  virtual void OnSetFocus(wxFocusEvent & event);
  virtual void OnKillFocus(wxFocusEvent & event);
  CalcMgr * p_calcMgr;
};

#endif // _CALCMGR_H_
