/**
 * @file
 *
 * @author Lisong Sun
 *
 */
#ifndef _CALCMGR_H_
#define _CALCMGR_H_

#include <wx/frame.h>
#include <wx/toolbar.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/dnd.h>

#include "util/ProgressEvent.H"
#include "util/TDateTime.H"
#include "util/JMSPublisher.H"
#include "util/EditListener.H"

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
#include "WxProperties.H"

class BookmarkEditor;
class WxFind;

class WxConfigureNwfsArchive;

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

  /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME
  void OnCloseWindow( wxCloseEvent& event );

  /// wxEVT_CONTEXT_MENU event handler for ID_FRAME
  void OnContextMenu( wxContextMenuEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_IMPORT
  void OnImportClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXPORT
  void OnExportClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
  void OnExitClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE
  void OnDuplicateClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE2
  void OnDuplicate2Click( wxCommandEvent& event );

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
  void createResource(ResourceType * resource,
                      WxResourceTreeItemData * parent = NULL,
                      const bool& bypassNamePrompt = false);

  void addRoot(const string& name, const string& url);
  WxFeedback * getFeedback();
  void updateName(const EcceURL & oldUrl, const EcceURL & newUrl);
  void updateUrl(const EcceURL & url);
  void updateAddNode(const EcceURL & url);
  void updateRemoveNode(const EcceURL & url);
  void setMessage(const string & message,
                  WxFeedback::Severity severity=WxFeedback::WARNING,
                  bool clear = false);
  WxResourceTreeItemData * findNode(const EcceURL &targetUrl,
                                    bool openWhenFound = true,
                                    bool loadFromServer = true);
  void recolorStatePixmaps();
  void onSelectionChange(bool selectInTree = false);
  void focusOnTree();
  void focusLeaveTree();
  void copySelectionToClipboard(bool deleteAfterPaste);
  void paste();
  void deleteSelection();
  string getTreeSelectionName();
  bool getTypeAndState(WxResourceTreeItemData * itemData, 
                       ResourceType *& resType,
                       ResourceDescriptor::RUNSTATE & state);
  void getContextMenu(wxMenu & menu);

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

  void progressNotice(ProgressEvent& event);
  bool getSelection();
  bool isValidName(const string& name);
  bool checkMovable(WxResourceTreeItemData * node);
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
  void shellInRunDir(   WxResourceTreeItemData*);
  void manageFiles(     WxResourceTreeItemData*);
  void terminateCalc(   WxResourceTreeItemData*);
  void checkJob(        WxResourceTreeItemData*);
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
                                    bool openWhenFound = true);

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
  WxFind                * p_find;
  
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

  bool p_focusOnTree;
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

  static const char     * vMsgNoMoving;

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
