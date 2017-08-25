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
#endif

#include <sstream>
#include <iostream>
using std::cerr;
using std::endl;

#include <wx/dataobj.h>
#include <wx/dnd.h>
#include <wx/treectrl.h>

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/Host.H"
#include "util/TypedFile.H"
#include "util/InvalidException.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/Preferences.H"
#include "util/ResourceUtils.H"
#include "util/StringConverter.H"
#include "util/TDateTime.H"
#include "util/SFile.H"
#include "util/BrowserHelp.H"
#include "util/EditEvent.H"
#include "util/NullPointerException.H"

#include "tdat/PropTSVecTable.H"
#include "tdat/RefMachine.H"

#include "dsm/NWChemMDModel.H"
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
#include "dsm/DirDyVTSTTask.H"

#include "comm/EcceShell.H"
#include "comm/Launch.H"
#include "comm/RCommand.H"
#include "comm/RunMgmt.H"

#include "wxgui/EcceTool.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxFileDialog.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxTextEntryDialog.H"
#include "wxgui/WxCalcImport.H"
#include "wxgui/WxCalcImportClient.H"
#include "wxgui/WxDavAuth.H"
#include "wxgui/WxEditSessionMgr.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/WxResourceImageList.H"
#include "wxgui/WxResourceTreeCtrl.H"
#include "wxgui/WxResourceTreeItemData.H"
#include "wxgui/WxResourceStateImageList.H"
#include "wxgui/WxState.H"
#include "wxgui/WxStateImageList.H"

#include "BookmarkEditor.H"
#include "ContextPanelFactory.H"
#include "ContextPanelInterface.H"
#include "TreeDropTarget.H"
#include "WxConfigureNwfsArchive.H"
#include "WxFind.H"

#include "CalcMgr.H"

IMPLEMENT_CLASS( CalcMgr, wxFrame )


#define ID_BOOKMARK_BASE 80000
#define ID_BOOKMARK_TOP 80200

#define ID_HISTORY_BASE 80201
#define ID_HISTORY_TOP 80400

/**
 * Constructor.
 * Should never be used.
 */
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

  initializeGUI();

  setAuthDialogParent(this);
  EDSIFactory::addAuthEventListener(this);
  EDSIFactory::addProgressEventListener(this);

  // Get Registry
  ResourceDescriptor rs = ResourceDescriptor::getResourceDescriptor();


  // Set desktop icon
  SetIcon(wxIcon(ewxBitmap::pixmapFile(rs.getTool(ORGANIZER)->getIcon()), 
                                       wxBITMAP_TYPE_XPM));


  p_panelNode = 0;
  p_accessControlDialog = 0;
  p_bookmarkEditor = new BookmarkEditor("Bookmarks", true, this,
                                        wxID_ANY, "Bookmark Editor");


  p_serverListEditor = new BookmarkEditor("ServerList", false, this,
                                          wxID_ANY, "Server List Editor");
  p_find = 0;
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
  if (msg != "") {
    setMessage(msg, WxFeedback::INFO);
  }

  stopDisabler();

  return true;
}


/**
 * Destructor.
 * Save preference, disconnect event handling functions, clean memory.
 */
CalcMgr::~CalcMgr( )
{
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


  p_legend = new ewxPanel(this, -1, wxDefaultPosition,
                          wxDefaultSize, wxNO_BORDER);
  wxBoxSizer * legendVSizer = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer * legendSizer = new wxBoxSizer(wxHORIZONTAL);
  p_legend->SetSizer(legendVSizer);
  
  ewxStaticText* stateLabel =
    new ewxStaticText(p_legend, -1, "Run States: ");
  legendSizer->Add(stateLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);
  
  WxState* stateIcon;
  for (int state = ResourceDescriptor::STATE_CREATED;
       state<ResourceDescriptor::NUMBER_OF_STATES; state++) {
    stateIcon = new WxState(p_legend);
    stateIcon->setRunState((ResourceDescriptor::RUNSTATE)state);
    legendSizer->Add(stateIcon, 0, 
                     wxFIXED_MINSIZE|wxALIGN_CENTER_VERTICAL|wxLEFT, 6);

    stateLabel = new ewxStaticText(p_legend, -1, stateIcon->getName());
    stateLabel->SetFont(ewxStyledWindow::getSmallLabelFont());
    legendSizer->Add(stateLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);
  }

  legendVSizer->Add(legendSizer, 0, wxGROW|wxALL, 2);
  legendVSizer->Add(new ewxStaticLine(p_legend, -1), 0, wxGROW, 0);

  p_topSizer->Add(p_legend, 0, wxGROW|wxALL, 0);

  // Feedback area at the botton of the page, no need to delete it since
  // wxFrame destructor will delete it.
  p_feedback = new WxFeedback(this);
  p_feedback->hideEditState();
  p_feedback->hideDropSite();
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

  restoreWindowSettings("Organizer", prefs, true);

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
  PopupMenu(&menu);
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
  BrowserHelp help;
  help.showPage(help.URL("Organizer"));
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
  Destroy();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */
void CalcMgr::OnExitClick( wxCommandEvent& event )
{
  Destroy();
  event.Skip();
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


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for all ID_RESOURCE
 */
void CalcMgr::OnNewResourceClick( wxCommandEvent& event )
{
  startDisabler();

  ResourceType * resType = ResourceDescriptor::getResourceDescriptor().
    getResourceType(event.GetId());
  createResource(resType);

  stopDisabler();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for all ID_RMTASK
 */
void CalcMgr::OnRmTaskClick( wxCommandEvent& event )
{
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


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE
 */
void CalcMgr::OnDuplicateClick( wxCommandEvent& event )
{
  startDisabler();

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
    // the reaction study
    Resource *parent = EDSIFactory::getResource(res->getURL().getParent());
    if (parent->getApplicationType() == ResourceDescriptor::AT_REACTION_STUDY) {
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
    string invalidMsg =  "Invalid name entered.  Please make sure the\n"
      "new name only contains the following characters:\n" +
      resType->getNameRule() + "\n\n";
    while (true) {
      newName = oldUrl.getFilePathTail();
      string infoStr = "Please enter the new name for " +
        resType->getLabel() + ":";
      if (showInvalidMsg)
        infoStr = invalidMsg + infoStr;
      ewxTextEntryDialog dialog(this, infoStr, "Rename " + resType->getLabel(),
                                newName, wxOK | wxCANCEL);
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
                                         wxOPEN, wxDefaultPosition);
  dlg->SetFilterIndex(1);
  if (dlg->ShowModal() == wxID_OK) {
    // access the file to upload through an SFile
    string path = dlg->GetPath().c_str();
    SFile *file = new SFile(path);

    // Upload to selected folder in the tree
    EcceURL url = upload(p_treeCtrl->getSelection(), file);

    if (url != EcceURL())
      updateAddNode(url);

    // clean up the SFile pointer before going out of scope
    delete file;
  }

  // save the last directory and filename to calcmgr preferences
  string directory = dlg->GetDirectory().c_str();
  prefs.setString("Organizer.UploadDir", directory);
  string filename = dlg->GetFilename().c_str();
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
  //  cerr << "\nEnter SelChanged event\n";
  
  // If the parent of current selection is collapsed, the tree will auto select
  // root and then select the collapsed parent node. Skip the select root case.
  if (event.GetItem() == p_treeCtrl->GetRootItem()) {
    return;
  }

  onSelectionChange(true);

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
  //  cerr << "\nEnter ItemActivated event\n";
  //  cerr << "Leave ItemActivated event\n";
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TREE_ITEM_COLLAPSED event handler for ID_TREECTRL
 */
void CalcMgr::OnTreectrlItemCollapsed( wxTreeEvent& event )
{
  //  cerr << "\nEnter Collapsed event\n";
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

  wxTreeItemId id = event.GetItem();
  // A small hack to ensure that all the children are visible
  p_treeCtrl->ScrollTo(p_treeCtrl->GetLastChild(id));
  // @todo May not have a sibling if last child!!
  p_treeCtrl->ScrollTo(p_treeCtrl->GetNextSibling(id));
  p_treeCtrl->ScrollTo(id);
  
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
  //  cerr << "\nEnter ItemMenu event\n";

  //  cerr << "Leave ItemMenu event\n";
  event.Skip();
}


void CalcMgr::OnPanelSelection( wxCommandEvent& event )
{
  vector<EcceURL> urls = EcceURL::toVector(event.GetString().c_str());
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
void CalcMgr::deleteSelection()
{
  getSelection();
  vector<EcceURL> selection = p_currentSelection;
  vector<EcceURL>::iterator itor = selection.begin();
  bool yesToAll = false;
  for (; itor != selection.end(); itor++) {

    Resource *currentRes = EDSIFactory::getResource(*itor);

    bool allowedForDelete;
    string msg, name;
    currentRes->getDeleteMessage(allowedForDelete, msg);

    // special STTR reaction rate study logic to prevent the deletion
    // of the reactant/product/transition-state calculations for gas phase
    // and not to delete a non-leaf calculation for condensed phase
    // no need to check this if deletes are already prevented
    if (allowedForDelete) {
      Resource *parent = EDSIFactory::getResource(
                                         currentRes->getURL().getParent());
      if (parent->getApplicationType() ==
                  ResourceDescriptor::AT_REACTION_STUDY) {
        name = currentRes->getName();
        if (name=="Transition-State" ||
            name=="Reactant1" || name=="Reactant2" ||
            name=="Product1" || name=="Product2") {
          allowedForDelete = false;
          msg = "Cannot delete reaction study calculation \"" + name + "\".";
        }
      } else if (parent->getApplicationType() ==
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
                             (selection.size()>1?wxYES_NO_TO_ALL:0));
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

  // build the "new" part of the context menu first
  // the variables to iterate through what "can be contained"
  vector<ResourceType *> resVec;
  resVec = resType->getContains();
  vector<ResourceType *>::iterator resItor;
  wxMenu *newMenu = new wxMenu();

  // for each "contained", add to menu
  for (resItor = resVec.begin(); resItor != resVec.end(); resItor++) {
    // need to remove redundant "New " from each sub menu item
    string name((*resItor)->getMenuItem(), 4);
    wxMenuItem * menuItem = new wxMenuItem(&menu, (*resItor)->getId(),
            name.c_str(), _T(""), wxITEM_NORMAL);
    menuItem->SetBitmap(ewxBitmap((*resItor)->getIcon()));
    newMenu->Append(menuItem);

    if ((*resItor)->getMenuItem() == "New NWChem MD Prepare..." &&
        dynamic_cast<Session *>(itemData->getResource())->numMembers() == 0)
      resItor += 3;
    else if ((*resItor)->getMenuItem() ==
        "New Direct Dynamics VTST Calculation..." &&
        dynamic_cast<Session *>(itemData->getResource())->numMembers() == 0)
      resItor += 5;
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

  if (getTypeAndState(itemData, resType, state)) {
    vector<ResourceType *> resVec = resType->getContains();
    vector<ResourceType *>::iterator resItor = resVec.begin();
    for (;resItor != resVec.end(); resItor++) {
      wxMenuItem * menuItem = new wxMenuItem(&menu,(*resItor)->getId(),
              (*resItor)->getMenuItem(), _T(""), wxITEM_NORMAL);
      menuItem->SetBitmap(ewxBitmap((*resItor)->getIcon()));
      menu.Append(menuItem);

      if ((*resItor)->getMenuItem() == "New NWChem MD Prepare..." &&
          dynamic_cast<Session *>(itemData->getResource())->numMembers() == 0)
        resItor += 3;
      else if ((*resItor)->getMenuItem() ==
          "New Direct Dynamics VTST Calculation..." &&
          dynamic_cast<Session *>(itemData->getResource())->numMembers() == 0)
        resItor += 5;
    }
    
    if (resVec.size() > 0) {
      menu.AppendSeparator();
    }
    
    // Hardwire the import entry, not sure if there is any better way to do it
    if (resType->getName() == "project") {
      menu.Append(wxID_IMPORT, _("&Import Calculation from Output File..."),
                  _T(""), wxITEM_NORMAL);
      // menu.Append(wxID_EXPORT, _("Export"), _T(""), wxITEM_NORMAL);
    }
    
    bool isCollection = p_treeCtrl->setExpandable(itemData);
    wxMenuItem * tmpMenuItem = new wxMenuItem(&menu,
            isCollection ? wxID_UPLOAD : wxID_DOWNLOAD,
            _(isCollection ? "&Upload File..." : "&Download File..."),
            _T(""), wxITEM_NORMAL);
    //  tmpMenuItem->SetBitmap(ewxBitmap(isCollection?"upload.xpm":"download.xpm"));
    menu.Append(tmpMenuItem);
    
    menu.AppendSeparator();
  }

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
void CalcMgr::getToolsMenu(wxMenu & menu, WxResourceTreeItemData * itemData)
{
  // clear given menu of any menu items
  clearMenu(menu);

  ResourceType * resType = 0;
  ResourceDescriptor::RUNSTATE state = ResourceDescriptor::STATE_ILLEGAL;

  if(!getTypeAndState(itemData, resType, state))
    return; // menu should be disabled for multiple selection

  vector<ResourceTool *> toolVec;
  if (resType->getStateBased()) {
    toolVec = resType->getTools(state);
  }
  else {
    toolVec = resType->getTools();
  }

  vector<ResourceTool *>::iterator toolItor = toolVec.begin();
  for (;toolItor != toolVec.end(); toolItor++) {
    wxMenuItem *item = new wxMenuItem(&menu, (*toolItor)->getId(),
            (*toolItor)->getMenuItem(), _T(""), wxITEM_NORMAL);
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
  // the algorithm for iterating through a list and deleting each item was
  // modified from the wxWidgets documentation for wxList
  //
  // also, the documentation for wxMenu reveals that the items stored in the
  // wxMenuItemList are actually pointers to wxMenuItems
  wxMenuItemList items = menu.GetMenuItems();
  wxMenuItemList::Node *node = items.GetFirst();
  while (node) {
    wxMenuItem * item = 0;
    item = node->GetData();
    if (item->GetSubMenu()) {
      wxMenu *subMenu = item->GetSubMenu();
      clearMenu(*subMenu);
    }
    menu.Delete(node->GetData());
    delete node;
    node = items.GetFirst();
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
  bool doit = true;

  if (GetMenuBar()->IsChecked(wxID_CONFIRM_RESET)) {
    // get the selected tree or panel item(s), remember how many selected
    bool single = getSelection();
  
    string msg = "Resetting to rerun ";
    msg += single ? "this calculation " : "these calculations ";
    msg += "will remove all existing output properties and change the ";
    msg += single ? "state " : "states ";
    msg += "to ready allowing the ";
    msg += single ? "job " : "jobs ";
    msg += "to be rerun from the start.";
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
    vector<EcceURL> selections = p_currentSelection;
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
 * Opens a shell in the calculation run directory and uses the tail -f command
 * to display part of the output file.
 *
 * @param itemData  the tree node containing the calculation to work on
 */
void CalcMgr::tailOutputFile(WxResourceTreeItemData *itemData)
{
  string appName, calcName, user, shell, pathFull;

  // Get URL
  EcceURL url = itemData->getUrl();

  // Get TaskJob
  TaskJob *calc = dynamic_cast<TaskJob*>(itemData->getResource());
  NULLPOINTEREXCEPTION(calc,"Could not cast Resource to TaskJob.");

  const JCode *code = calc->application();
  if (code) {
    appName = code->name();

    calcName = calc->getName();
    const Launchdata& launch = calc->launchdata();

    shell = launch.remoteShell;
    user = launch.user;

    RefMachine* refMachine = RefMachine::refLookup(launch.machine);

    Jobdata job = calc->jobdata();
    pathFull = job.jobpath;
    if (!pathFull.empty()) {

      if (appName != "") {
        setMessage("Starting xterm with tail -f.  Use the "
                   "window manager menu to exit.", WxFeedback::INFO);
        const JCode* code = CodeFactory::lookup(appName.c_str());
        if (code) {
          TypedFile tfile;
          calc->getDataFile(JCode::PRIMARY_OUTPUT, tfile);
          appName = tfile.name();
          EcceShell eshell;
          string file = pathFull + "/" + appName;
          string cmd = "tail -f " + file;
          string msg = eshell.cmdshell(calcName, refMachine->fullname(), shell,
                                       user, "", cmd, file);
          if (msg != "") {
            setMessage(msg,
                       (eshell.lastStatus()==-1) ?
                       WxFeedback::ERROR : WxFeedback::WARNING);
          }
        } else {
          string msg = "No information about " + appName;
          setMessage(msg);

        }
      }
    } else {
      setMessage("No job information available.");
    }
  } else {
    setMessage("No code information available.");
  }
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
  }

  // remove properties
  if (!calc->deleteProperties()) {
    setMessage("Unable to delete existing output properties");
    return;
  }

  // For debugging purposes we keep run logs for failed (monitor error) jobs
  if (state != ResourceDescriptor::STATE_FAILED)
    calc->removeJobLog();

  calc->removeOutputFiles();

  // set state to submitted
  // we used to set it to ready but then if reconnect fails you can't
  // just try it again because the option is disabled!
  calc->setState(ResourceDescriptor::STATE_SUBMITTED);
  //  notifyState(url.toString(),
  //        ResourceUtils::stateToString(ResourceDescriptor::STATE_SUBMITTED));

  Launchdata launchdata = calc->launchdata();

  // Separate this out or there may be message timing problems
  EcceMap kvargs;

  // Probably should be error handling here on getting this info from the
  // codecap file.
  string tmp;
  const JCode* jcode = calc->application();
  if (jcode) {

    TypedFile file;
    calc->getDataFile(JCode::PRIMARY_OUTPUT, file);
    kvargs["##output##"] = file.name();
    calc->getDataFile(JCode::PARSE_OUTPUT, file);
    kvargs["##parse##"] = file.name();
    calc->getDataFile(JCode::PROPERTY_OUTPUT, file);
    kvargs["##property##"] = file.name();
    calc->getDataFile(JCode::AUXILIARY_OUTPUT, file);
    kvargs["##auxiliary##"] = file.name();

    // Add MD args, if necessary
    MdTask *mdTask = dynamic_cast<MdTask*>(calc);
    if (mdTask != 0) {
      NWChemMDModel taskModel;
      try {
        mdTask->getTaskModel(taskModel);

        // Fragment
        kvargs["##output_frag##"] = mdTask->getOutputFragmentName();

        // Restart
        kvargs["##restart##"] = mdTask->getRestartName();

        // MD Output
        kvargs["##md_output##"] = mdTask->getMdOutputName();

        // Topology
        kvargs["##topology##"] = mdTask->getTopologyName();
      }
      catch (...) {
        //cerr << "Failed to set MD job output file parameters" << endl;
      }
    }

    Launch *launch = new Launch(calc, kvargs);
    if (launch->validateLocalDir() &&
        launch->validateRemoteLogin() &&
        launch->generateJobMonitoringFiles() &&
        launch->moveJobMonitoringFiles() &&
        launch->startJobStore("")) {
      string msg = "Monitoring reconnected to job " + job.jobid +
        " on " + launchdata.machine;
      setMessage(msg, WxFeedback::INFO);

      // don't issue warning for submitted jobs because it would be silly
      if (state != ResourceDescriptor::STATE_SUBMITTED)
        setMessage("It may take several minutes for previous output to be "
                   "parsed.  You may want to close any Viewers for this "
                   "calculation and restart them later as they will be "
                   "extremely slow to respond to input.", WxFeedback::WARNING);
    } else {
      setMessage(launch->message(), WxFeedback::ERROR);
    }
    delete launch;
  } else {
    setMessage("No code registration data found for the selected calculation");
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
    int numNodes = children->size();

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
            msg += " is no longer running.";
            setMessage(msg, WxFeedback::WARNING);

            // set the state to failed (which sends out JMS notification)
            TaskJob *calc = dynamic_cast<TaskJob*>(resource);
            NULLPOINTEREXCEPTION(calc,
                                 "Unable to cast from Resource to TaskJob.");
            calc->setState(ResourceDescriptor::STATE_FAILED);
            updateUrl(resource->getURL());

            // Email failures to ecce-test if ECCE_JOB_FAILMAIL tells us to
            /**
             * @todo This will not work under windows, not sure about mac.
             *       May need some ifdef to handle it differently.
             */
            string url = resource->getURL().toString();
            string failmail = getenv("ECCE_JOB_FAILMAIL") ?
              getenv("ECCE_JOB_FAILMAIL"): "";
            if (failmail=="true" ||
                (failmail!="" && url.find(failmail)!=string::npos)) {
              string mail =
              "Mail -s 'Job Monitoring Failure' ecce-test@emsl.pnl.gov << EOM\n"
              "Job found in the submitted/running state without an "
              "eccejobstore ";
              mail += Ecce::ecceVersion();
              mail += " process.\nCalculation name: " + url + "\nEOM";
              (void)system(mail.c_str());
            }
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
                                         wxSAVE|wxOVERWRITE_PROMPT, 
                                         wxDefaultPosition);
  dlg->SetFilterIndex(1);
  if (dlg->ShowModal() == wxID_OK) {
    // make file from url of current directory plus filename (GetPath)
    string path = dlg->GetPath().c_str();
    SFile *file = new SFile(path);

    itemData->getResource()->getDocument(file);

    // clean up SFile pointer before leaving scope
    delete file;
  }

  // save last directory to calcmgr preferences
  string directory = dlg->GetDirectory().c_str();
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
      EE_ASSERT((3*numRows == values.size()),EE_WARNING,"Bad Geomtrace");
      TAtm *atom;
      int ptr=0;
      for (int idx=0; idx<numRows; idx++) {
        atom = frag->atomRef(idx);
        atom->coordinates(values[ptr],values[ptr+1], values[ptr+2]);
        ptr+=3;
      }

      // Set the Fragment
      calc->fragment(frag);

    } else {
      // Since we didn't change geom, don't need to go to created
      // By setting tostate to ResourceDescriptor::STATE_ILLEGAL the 
      // resetForRerun() method will determine the correct runstate
      tostate = ResourceDescriptor::STATE_ILLEGAL;
      p_feedback->clearMessage();
      setMessage("There is no geometry trace property.  Coordinates unchanged.");
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
    string invalidMsg =  "Invalid name entered.  Please make sure the\n"
      "name only contains the following characters:\n" +
      resType->getNameRule() + "\n\n";
    while (true) {
      name = resType->getLabel();
      string infoStr = "Please enter the name for the new " +
        name + ":";
      if (showInvalidMsg)
        infoStr = invalidMsg + infoStr;
      ewxTextEntryDialog dialog(this, infoStr, "New " + name + " Name",
                                name, wxOK | wxCANCEL);
      if (dialog.ShowModal() == wxID_OK) {
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
    if (newRes->getApplicationType()==ResourceDescriptor::AT_NWCHEMMD ||
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


/**
 * Refresh the context panel and tree ctrl when the resource associated with
 * the URL got added
 */
void CalcMgr::updateAddNode(const EcceURL & url)
{
  if (findNode(url.getParent(), false, false)) {
    p_treeCtrl->addToTree(url);
  
    if (url.getParent() == p_treeCtrl->getSelection()->getUrl()) {
      setContextPanel();
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


void CalcMgr::recolorStatePixmaps()
{
  p_treeCtrl->recolorStatePixmaps();
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

  // make resource
  Resource *itemDataRes = itemData->getResource();
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
      p_treeCtrl->refresh(itemData);
      setContextPanel();

      ret = dynamic_cast<TaskJob*>(childRes);
    }
  }

  return ret;
}


void CalcMgr::importValidationComplete(TaskJob *ipc, bool status,
                                       string message)
{
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
    setMessage(message, WxFeedback::ERROR);
  } else {
    if (message != "")
      setMessage(message, WxFeedback::WARNING);

    message = "Calculation output currently being imported into " +
      ipc->getName() + ".";
    setMessage(message, WxFeedback::INFO);

    // get the parent of the calculation
    WxResourceTreeItemData *itemData = p_treeCtrl->getSelection();
    p_treeCtrl->refresh(itemData);
    setContextPanel();

    // let the world know this calculation was created/imported
    notifyCreate(ipc->getURL().toString());
  }
  setContextPanel();
}


void CalcMgr::onSelectionChange(bool selectInTree)
{
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

  itemData = p_treeCtrl->getSelection();
  wxMenu * fileMenu = menuBar->GetMenu(menuBar->FindMenu(_("File")));
  getFileMenu(*fileMenu, itemData);

  
  if (!getSelection()) {
    menuBar->EnableTop(menuBar->FindMenu(_("Run Mgmt")), false);
    menuBar->EnableTop(menuBar->FindMenu(_("Tools")), false);
    menuBar->Enable(wxID_CHANGE_ACCESS, false);
    menuBar->Enable(wxID_RENAME, false);
    menuBar->Enable(wxID_VIEW_PROPERTY, false);
    menuBar->Enable(wxID_ANNOTATION, false);
    menuBar->Enable(wxID_DUPLICATE, false);
    menuBar->Enable(wxID_DUPLICATE2, false);
    p_toolBar->EnableTool(wxID_RENAME, false);
    p_toolBar->EnableTool(wxID_VIEW_PROPERTY, false);
    p_toolBar->EnableTool(wxID_DOWNLOAD, false);

  }
  else {
    itemData = findNode(p_currentSelection[0], false, true);
    if (itemData && itemData->getResource()->hasAccess()) {
      bool isProject = (itemData->getResource()->getContentType() ==
                        ResourceDescriptor::CT_PROJECT);
      bool isCalculation = (itemData->getResource()->getContentType() ==
                            ResourceDescriptor::CT_CALCULATION);
      bool isDocument = (itemData->getResource()->getResourceType() ==
                         ResourceDescriptor::RT_DOCUMENT);
      bool isSystemFolder = itemData->getUrl().isSystemFolder();

      // Only work on project, not even virtual documents
      // @todo shouldn't use hardwired string
      menuBar->Enable(wxID_CHANGE_ACCESS, isProject);
      menuBar->Enable(wxID_CUT, !isSystemFolder);
      menuBar->Enable(wxID_RENAME, !isSystemFolder);
      menuBar->Enable(wxID_DELETE, !isSystemFolder);
      menuBar->Enable(wxID_VIEW_PROPERTY, true);
      menuBar->Enable(wxID_ANNOTATION, !isSystemFolder);
      menuBar->Enable(wxID_DUPLICATE, isCalculation);
      menuBar->Enable(wxID_DUPLICATE2, isCalculation);
      menuBar->Enable(wxID_TEST_SUITE, isCalculation);
      p_toolBar->EnableTool(wxID_CUT, !isSystemFolder);
      p_toolBar->EnableTool(wxID_RENAME, !isSystemFolder);
      p_toolBar->EnableTool(wxID_DELETE, !isSystemFolder);
      p_toolBar->EnableTool(wxID_VIEW_PROPERTY, true);
      p_toolBar->EnableTool(wxID_DOWNLOAD, isDocument);

      // dynamically populate the Run Management menu
      // The run management menu resource always iterates from top to bottom.
      // The submenus are indicated by double colon's, ::, where the left side
      // is the name of the submenu and the right side is a field in that menu.
      // Which tasks get displayed depends on the state of the calculation.
      wxMenu * rmTaskMenu = menuBar->GetMenu(menuBar->FindMenu(_("Run Mgmt")));
      getRunMgmtMenu(*rmTaskMenu, itemData);
      menuBar->EnableTop(menuBar->FindMenu(_("Run Mgmt")),
                         rmTaskMenu->GetMenuItemCount()>0);

      // dynamically populate the Tool menu
      wxMenu * toolMenu = menuBar->GetMenu(menuBar->FindMenu(_("Tools")));
      getToolsMenu(*toolMenu, itemData);
      menuBar->EnableTop(menuBar->FindMenu(_("Tools")),
                         toolMenu->GetMenuItemCount()>0);
    }
    else {
      menuBar->EnableTop(menuBar->FindMenu(_("Run Mgmt")), false);
      menuBar->EnableTop(menuBar->FindMenu(_("Tools")), false);
      menuBar->Enable(wxID_CHANGE_ACCESS, false);
      menuBar->Enable(wxID_CUT, false);
      menuBar->Enable(wxID_RENAME, false);
      menuBar->Enable(wxID_DELETE, false);
      menuBar->Enable(wxID_VIEW_PROPERTY, false);
      menuBar->Enable(wxID_ANNOTATION, false);
      menuBar->Enable(wxID_DUPLICATE, false);
      menuBar->Enable(wxID_DUPLICATE2, false);
      menuBar->Enable(wxID_TEST_SUITE, false);
      p_toolBar->EnableTool(wxID_CUT, false);
      p_toolBar->EnableTool(wxID_RENAME, false);
      p_toolBar->EnableTool(wxID_DELETE, false);
      p_toolBar->EnableTool(wxID_VIEW_PROPERTY, false);
      p_toolBar->EnableTool(wxID_DOWNLOAD, false);
    }
  }
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
 * return true if it is already disabled.
 */
int CalcMgr::startDisabler()
{
  if (p_disablerCount == 0 && p_disabler == 0 && p_cursor == 0) {
    p_focusOnTreeBeforeDisabler = p_focusOnTree;
    p_disabler = new wxWindowDisabler;
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
            itemData->getUrl().getParent().isEcceUsers())
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
