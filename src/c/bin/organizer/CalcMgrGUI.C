/////////////////////////////////////////////////////////////////////////////
// Name:        CalcMgrGUI.C
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "CalcMgrGUI.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
#include "wxgui/WxResourceTreeCtrl.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxFrame.H"
////@end includes

#include "CalcMgrGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID CalcMgrGUI::ID_TOOLBAR1 = wxNewId();
const wxWindowID CalcMgrGUI::wxID_SHOW_LEGEND = wxNewId();
const wxWindowID CalcMgrGUI::wxID_UPLOAD = wxNewId();
const wxWindowID CalcMgrGUI::wxID_SHOW_INTERNAL = wxNewId();
const wxWindowID CalcMgrGUI::wxID_DUPLICATE2 = wxNewId();
const wxWindowID CalcMgrGUI::wxID_HOMEASROOT = wxNewId();
const wxWindowID CalcMgrGUI::ID_FRAME = wxNewId();
const wxWindowID CalcMgrGUI::wxID_SORT_TYPE = wxNewId();
const wxWindowID CalcMgrGUI::wxID_ANNOTATION = wxNewId();
const wxWindowID CalcMgrGUI::wxID_CONFIRM_DELETE = wxNewId();
const wxWindowID CalcMgrGUI::wxID_DOWNLOAD = wxNewId();
const wxWindowID CalcMgrGUI::wxID_TEST_SUITE = wxNewId();
const wxWindowID CalcMgrGUI::wxID_RUNDEFAULTTOOL = wxNewId();
const wxWindowID CalcMgrGUI::wxID_VIEW_PROPERTY = wxNewId();
const wxWindowID CalcMgrGUI::wxID_ADD_BOOKMARK = wxNewId();
const wxWindowID CalcMgrGUI::ID_MENU = wxNewId();
const wxWindowID CalcMgrGUI::wxID_SHOW_LINE = wxNewId();
const wxWindowID CalcMgrGUI::ID_SPLITTERWINDOW = wxNewId();
const wxWindowID CalcMgrGUI::wxID_MANAGE_BOOKMARK = wxNewId();
const wxWindowID CalcMgrGUI::wxID_SORT_DATE_REV = wxNewId();
const wxWindowID CalcMgrGUI::ID_MENU_SUPPORT = wxNewId();
const wxWindowID CalcMgrGUI::wxID_CONFIGURE_NWFS_ARCHIVE = wxNewId();
const wxWindowID CalcMgrGUI::wxID_SORT_NAME = wxNewId();
const wxWindowID CalcMgrGUI::ID_TREECTRL = wxNewId();
const wxWindowID CalcMgrGUI::wxID_ADD_SERVER = wxNewId();
const wxWindowID CalcMgrGUI::wxID_ASKFORNAME = wxNewId();
const wxWindowID CalcMgrGUI::wxID_SORT_NAME_REV = wxNewId();
const wxWindowID CalcMgrGUI::wxID_SORT_DATE = wxNewId();
const wxWindowID CalcMgrGUI::wxID_IMPORT = wxNewId();
const wxWindowID CalcMgrGUI::wxID_RENAME = wxNewId();
const wxWindowID CalcMgrGUI::wxID_SHOW_TOOLBAR = wxNewId();
const wxWindowID CalcMgrGUI::wxID_CHANGE_ACCESS = wxNewId();
const wxWindowID CalcMgrGUI::wxID_CONFIRM_RESET = wxNewId();
const wxWindowID CalcMgrGUI::wxID_EXPORT = wxNewId();
const wxWindowID CalcMgrGUI::wxID_SORT_TYPE_REV = wxNewId();
const wxWindowID CalcMgrGUI::wxID_SHOW_RUNSTATES = wxNewId();

/*!
 * CalcMgrGUI type definition
 */

IMPLEMENT_CLASS( CalcMgrGUI, ewxFrame )

/*!
 * CalcMgrGUI event table definition
 */

BEGIN_EVENT_TABLE( CalcMgrGUI, ewxFrame )

////@begin CalcMgrGUI event table entries
    EVT_CLOSE( CalcMgrGUI::OnCloseWindow )
    EVT_CONTEXT_MENU( CalcMgrGUI::OnContextMenu )

    EVT_MENU( wxID_IMPORT, CalcMgrGUI::OnImportClick )

    EVT_MENU( wxID_EXPORT, CalcMgrGUI::OnExportClick )

    EVT_MENU( wxID_EXIT, CalcMgrGUI::OnExitClick )

    EVT_MENU( wxID_DUPLICATE, CalcMgrGUI::OnDuplicateClick )

    EVT_MENU( wxID_DUPLICATE2, CalcMgrGUI::OnDuplicate2Click )

    EVT_MENU( wxID_CUT, CalcMgrGUI::OnCutClick )

    EVT_MENU( wxID_COPY, CalcMgrGUI::OnCopyClick )

    EVT_MENU( wxID_PASTE, CalcMgrGUI::OnPasteClick )

    EVT_MENU( wxID_RENAME, CalcMgrGUI::OnRenameClick )

    EVT_MENU( wxID_DELETE, CalcMgrGUI::OnDeleteClick )

    EVT_MENU( wxID_ANNOTATION, CalcMgrGUI::OnAnnotationClick )

    EVT_MENU( wxID_CHANGE_ACCESS, CalcMgrGUI::OnChangeAccessClick )

    EVT_MENU( wxID_CONFIGURE_NWFS_ARCHIVE, CalcMgrGUI::OnConfigureNwfsArchiveClick )

    EVT_MENU( wxID_SHOW_TOOLBAR, CalcMgrGUI::OnShowToolbarClick )

    EVT_MENU( wxID_SHOW_LEGEND, CalcMgrGUI::OnShowLegendClick )

    EVT_MENU( wxID_SHOW_LINE, CalcMgrGUI::OnShowLineClick )

    EVT_MENU( wxID_SHOW_RUNSTATES, CalcMgrGUI::OnShowRunstatesClick )

    EVT_MENU( wxID_SHOW_INTERNAL, CalcMgrGUI::OnShowInternalClick )

    EVT_MENU( wxID_HOMEASROOT, CalcMgrGUI::OnHomeasrootClick )

    EVT_MENU( wxID_SORT_TYPE, CalcMgrGUI::OnSortTreeClick )

    EVT_MENU( wxID_SORT_TYPE_REV, CalcMgrGUI::OnSortTreeClick )

    EVT_MENU( wxID_SORT_NAME, CalcMgrGUI::OnSortTreeClick )

    EVT_MENU( wxID_SORT_NAME_REV, CalcMgrGUI::OnSortTreeClick )

    EVT_MENU( wxID_SORT_DATE, CalcMgrGUI::OnSortTreeClick )

    EVT_MENU( wxID_SORT_DATE_REV, CalcMgrGUI::OnSortTreeClick )

    EVT_MENU( wxID_ADD_BOOKMARK, CalcMgrGUI::OnAddBookmarkClick )

    EVT_MENU( wxID_MANAGE_BOOKMARK, CalcMgrGUI::OnManageBookmarkClick )

    EVT_MENU( wxID_HELP, CalcMgrGUI::OnHelpClick )

    EVT_MENU( ID_MENU_SUPPORT, CalcMgrGUI::OnMenuSupportClick )

    EVT_MENU( wxID_TEST_SUITE, CalcMgrGUI::OnTestSuiteClick )

    EVT_MENU( wxID_VIEW_PROPERTY, CalcMgrGUI::OnViewPropertyClick )

    EVT_MENU( wxID_ADD_SERVER, CalcMgrGUI::OnAddServerClick )

    EVT_MENU( wxID_HOME, CalcMgrGUI::OnHomeClick )

    EVT_MENU( wxID_UP, CalcMgrGUI::OnUpClick )

    EVT_MENU( wxID_BACKWARD, CalcMgrGUI::OnBackwardClick )

    EVT_MENU( wxID_FORWARD, CalcMgrGUI::OnForwardClick )

    EVT_MENU( wxID_FIND, CalcMgrGUI::OnFindClick )

    EVT_MENU( wxID_REFRESH, CalcMgrGUI::OnRefreshClick )

    EVT_MENU( wxID_UPLOAD, CalcMgrGUI::OnUploadClick )

    EVT_MENU( wxID_DOWNLOAD, CalcMgrGUI::OnDownloadClick )

    EVT_TREE_SEL_CHANGED( ID_TREECTRL, CalcMgrGUI::OnTreectrlSelChanged )
    EVT_TREE_SEL_CHANGING( ID_TREECTRL, CalcMgrGUI::OnTreectrlSelChanging )
    EVT_TREE_BEGIN_DRAG( ID_TREECTRL, CalcMgrGUI::OnTreectrlBeginDrag )
    EVT_TREE_END_DRAG( ID_TREECTRL, CalcMgrGUI::OnTreectrlEndDrag )
    EVT_TREE_BEGIN_RDRAG( ID_TREECTRL, CalcMgrGUI::OnTreectrlBeginRDrag )
    EVT_TREE_DELETE_ITEM( ID_TREECTRL, CalcMgrGUI::OnTreectrlDeleteItem )
    EVT_TREE_ITEM_ACTIVATED( ID_TREECTRL, CalcMgrGUI::OnTreectrlItemActivated )
    EVT_TREE_ITEM_COLLAPSED( ID_TREECTRL, CalcMgrGUI::OnTreectrlItemCollapsed )
    EVT_TREE_ITEM_COLLAPSING( ID_TREECTRL, CalcMgrGUI::OnTreectrlItemCollapsing )
    EVT_TREE_ITEM_EXPANDED( ID_TREECTRL, CalcMgrGUI::OnTreectrlItemExpanded )
    EVT_TREE_ITEM_EXPANDING( ID_TREECTRL, CalcMgrGUI::OnTreectrlItemExpanding )
    EVT_TREE_KEY_DOWN( ID_TREECTRL, CalcMgrGUI::OnTreectrlKeyDown )
    EVT_TREE_ITEM_MENU( ID_TREECTRL, CalcMgrGUI::OnTreectrlItemMenu )

////@end CalcMgrGUI event table entries

END_EVENT_TABLE()

/*!
 * CalcMgrGUI constructors
 */

CalcMgrGUI::CalcMgrGUI( )
{
}

CalcMgrGUI::CalcMgrGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * CalcMgrGUI creator
 */

bool CalcMgrGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CalcMgrGUI member initialisation
    p_topSizer = NULL;
    p_toolBar = NULL;
    p_splitterWindow = NULL;
    p_treeCtrl = NULL;
////@end CalcMgrGUI member initialisation

////@begin CalcMgrGUI creation
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    Centre();
////@end CalcMgrGUI creation
    return true;
}

/*!
 * Control creation for CalcMgrGUI
 */

void CalcMgrGUI::CreateControls()
{    
////@begin CalcMgrGUI content construction
    CalcMgrGUI* itemFrame1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu3 = new wxMenu;
    itemMenu3->Append(wxID_IMPORT, _("&Import Calculation from Output File..."), _T(""), wxITEM_NORMAL);
    itemMenu3->Append(wxID_EXPORT, _("&Export"), _T(""), wxITEM_NORMAL);
    itemMenu3->AppendSeparator();
    itemMenu3->Append(wxID_EXIT, _("&Quit\tCtrl+Q"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu3, _("&File"));
    wxMenu* itemMenu8 = new wxMenu;
    itemMenu8->Append(wxID_DUPLICATE, _("Duplicate for Rerun"), _T(""), wxITEM_NORMAL);
    itemMenu8->Append(wxID_DUPLICATE2, _("Duplicate Setup with Last Geometry"), _T(""), wxITEM_NORMAL);
    itemMenu8->AppendSeparator();
    itemMenu8->Append(wxID_CUT, _("&Cut\tCtrl+X"), _T(""), wxITEM_NORMAL);
    itemMenu8->Append(wxID_COPY, _("C&opy\tCtrl+C"), _T(""), wxITEM_NORMAL);
    itemMenu8->Append(wxID_PASTE, _("&Paste\tCtrl+V"), _T(""), wxITEM_NORMAL);
    itemMenu8->AppendSeparator();
    itemMenu8->Append(wxID_RENAME, _("Rename"), _T(""), wxITEM_NORMAL);
    itemMenu8->Append(wxID_DELETE, _("&Delete\tDel"), _T(""), wxITEM_NORMAL);
    itemMenu8->Append(wxID_ANNOTATION, _("Annotation..."), _T(""), wxITEM_NORMAL);
    itemMenu8->Append(wxID_FIND, _("&Find...\tCtrl+F"), _T(""), wxITEM_NORMAL);
    itemMenu8->AppendSeparator();
    itemMenu8->Append(wxID_CHANGE_ACCESS, _("Change Access Control..."), _T(""), wxITEM_NORMAL);
    itemMenu8->Append(wxID_CONFIGURE_NWFS_ARCHIVE, _("Configure NWFS Archive Access..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu8, _("&Edit"));
    wxMenu* itemMenu23 = new wxMenu;
    itemMenu23->Append(wxID_SHOW_TOOLBAR, _("Show Toolbar"), _T(""), wxITEM_CHECK);
    itemMenu23->Check(wxID_SHOW_TOOLBAR, true);
    itemMenu23->Append(wxID_SHOW_LEGEND, _("Show Run State Legend"), _T(""), wxITEM_CHECK);
    itemMenu23->Check(wxID_SHOW_LEGEND, true);
    itemMenu23->Append(wxID_SHOW_LINE, _("Show Lines in Tree View"), _T(""), wxITEM_CHECK);
    itemMenu23->Check(wxID_SHOW_LINE, true);
    itemMenu23->Append(wxID_SHOW_RUNSTATES, _("Show Run States in Tree"), _T(""), wxITEM_CHECK);
    itemMenu23->Check(wxID_SHOW_RUNSTATES, true);
    itemMenu23->Append(wxID_SHOW_INTERNAL, _("Show ECCE Internal Files"), _T(""), wxITEM_CHECK);
    itemMenu23->AppendSeparator();
    itemMenu23->Append(wxID_HOMEASROOT, _("Set Home as Root"), _T(""), wxITEM_CHECK);
    itemMenu23->Append(wxID_ASKFORNAME, _("Ask for File Name at Creation"), _T(""), wxITEM_CHECK);
    itemMenu23->Check(wxID_ASKFORNAME, true);
    itemMenu23->Append(wxID_RUNDEFAULTTOOL, _("Invoke Default Tool at Creation"), _T(""), wxITEM_CHECK);
    itemMenu23->Append(wxID_CONFIRM_DELETE, _("Confirm Deletes"), _T(""), wxITEM_CHECK);
    itemMenu23->Check(wxID_CONFIRM_DELETE, true);
    itemMenu23->Append(wxID_CONFIRM_RESET, _("Confirm Resets"), _T(""), wxITEM_CHECK);
    itemMenu23->Check(wxID_CONFIRM_RESET, true);
    wxMenu* itemMenu35 = new wxMenu;
    itemMenu35->Append(wxID_SORT_TYPE, _("Type"), _T(""), wxITEM_RADIO);
    itemMenu35->Check(wxID_SORT_TYPE, true);
    itemMenu35->Append(wxID_SORT_TYPE_REV, _("Type (Reverse)"), _T(""), wxITEM_RADIO);
    itemMenu35->Append(wxID_SORT_NAME, _("Name"), _T(""), wxITEM_RADIO);
    itemMenu35->Append(wxID_SORT_NAME_REV, _("Name (Reverse)"), _T(""), wxITEM_RADIO);
    itemMenu35->Append(wxID_SORT_DATE, _("Date"), _T(""), wxITEM_RADIO);
    itemMenu35->Append(wxID_SORT_DATE_REV, _("Date (Reverse)"), _T(""), wxITEM_RADIO);
    itemMenu23->Append(ID_MENU, _("Sort Tree by"), itemMenu35);
    menuBar->Append(itemMenu23, _("O&ptions"));
    wxMenu* itemMenu42 = new wxMenu;
    itemMenu42->Append(wxID_HOME, _("&Home\tAlt+HOME"), _T(""), wxITEM_NORMAL);
    itemMenu42->Append(wxID_UP, _("&Up\tAlt+UP"), _T(""), wxITEM_NORMAL);
    itemMenu42->Append(wxID_BACKWARD, _("&Back\tAlt+LEFT"), _T(""), wxITEM_NORMAL);
    itemMenu42->Append(wxID_FORWARD, _("&Forward\tAlt+RIGHT"), _T(""), wxITEM_NORMAL);
    itemMenu42->AppendSeparator();
    menuBar->Append(itemMenu42, _("&Go"));
    wxMenu* itemMenu48 = new wxMenu;
    itemMenu48->Append(wxID_ADD_BOOKMARK, _("&Add to Bookmarks\tCtrl+D"), _T(""), wxITEM_NORMAL);
    itemMenu48->Append(wxID_MANAGE_BOOKMARK, _("&Manage Bookmarks..."), _T(""), wxITEM_NORMAL);
    itemMenu48->AppendSeparator();
    menuBar->Append(itemMenu48, _("&Bookmarks"));
    wxMenu* itemMenu52 = new wxMenu;
    menuBar->Append(itemMenu52, _("Run &Mgmt"));
    wxMenu* itemMenu53 = new wxMenu;
    menuBar->Append(itemMenu53, _("&Tools"));
    wxMenu* itemMenu54 = new wxMenu;
    itemMenu54->Append(wxID_HELP, _("&on Organizer...\tCtrl+H"), _T(""), wxITEM_NORMAL);
    itemMenu54->Append(ID_MENU_SUPPORT, _("Support..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu54, _("&Help"));
    wxMenu* itemMenu57 = new wxMenu;
    itemMenu57->Append(wxID_TEST_SUITE, _("Generate Theory/RunType Test Suite"), _T(""), wxITEM_NORMAL);
    itemMenu57->Append(wxID_VIEW_PROPERTY, _("View Properties..."), _T(""), wxITEM_NORMAL);
    itemMenu57->Append(wxID_ADD_SERVER, _("Manage Data Servers..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu57, _("&Developer"));
    itemFrame1->SetMenuBar(menuBar);

    p_topSizer = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(p_topSizer);

    p_toolBar = new wxToolBar( itemFrame1, ID_TOOLBAR1, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL );
    p_toolBar->SetToolPacking(2);
    p_toolBar->SetToolBitmapSize(wxSize(24, 24));
    wxBitmap itemtool63Bitmap(itemFrame1->GetBitmapResource(wxT("gohome.xpm")));
    p_toolBar->AddTool(wxID_HOME, _T(""), itemtool63Bitmap, _("Go to user home on current server"), wxITEM_NORMAL);
    wxBitmap itemtool64Bitmap(itemFrame1->GetBitmapResource(wxT("goup.xpm")));
    p_toolBar->AddTool(wxID_UP, _T(""), itemtool64Bitmap, _("Go up"), wxITEM_NORMAL);
    wxBitmap itemtool65Bitmap(itemFrame1->GetBitmapResource(wxT("back.xpm")));
    p_toolBar->AddTool(wxID_BACKWARD, _T(""), itemtool65Bitmap, _("Go back"), wxITEM_NORMAL);
    wxBitmap itemtool66Bitmap(itemFrame1->GetBitmapResource(wxT("forward.xpm")));
    p_toolBar->AddTool(wxID_FORWARD, _T(""), itemtool66Bitmap, _("Go forward"), wxITEM_NORMAL);
    p_toolBar->AddSeparator();
    wxBitmap itemtool68Bitmap(itemFrame1->GetBitmapResource(wxT("editcut.xpm")));
    p_toolBar->AddTool(wxID_CUT, _T(""), itemtool68Bitmap, _("Cut"), wxITEM_NORMAL);
    wxBitmap itemtool69Bitmap(itemFrame1->GetBitmapResource(wxT("editcopy.xpm")));
    p_toolBar->AddTool(wxID_COPY, _T(""), itemtool69Bitmap, _("Copy"), wxITEM_NORMAL);
    wxBitmap itemtool70Bitmap(itemFrame1->GetBitmapResource(wxT("editpaste.xpm")));
    p_toolBar->AddTool(wxID_PASTE, _T(""), itemtool70Bitmap, _("Paste"), wxITEM_NORMAL);
    wxBitmap itemtool71Bitmap(itemFrame1->GetBitmapResource(wxT("rename.xpm")));
    p_toolBar->AddTool(wxID_RENAME, _T(""), itemtool71Bitmap, _("Rename"), wxITEM_NORMAL);
    wxBitmap itemtool72Bitmap(itemFrame1->GetBitmapResource(wxT("editdelete.xpm")));
    p_toolBar->AddTool(wxID_DELETE, _T(""), itemtool72Bitmap, _("Delete"), wxITEM_NORMAL);
    p_toolBar->AddSeparator();
    wxBitmap itemtool74Bitmap(itemFrame1->GetBitmapResource(wxT("filefind.xpm")));
    p_toolBar->AddTool(wxID_FIND, _T(""), itemtool74Bitmap, _("Search File..."), wxITEM_NORMAL);
    wxBitmap itemtool75Bitmap(itemFrame1->GetBitmapResource(wxT("reload.xpm")));
    p_toolBar->AddTool(wxID_REFRESH, _T(""), itemtool75Bitmap, _("Refresh"), wxITEM_NORMAL);
    p_toolBar->AddSeparator();
    wxBitmap itemtool77Bitmap(itemFrame1->GetBitmapResource(wxT("upload.xpm")));
    p_toolBar->AddTool(wxID_UPLOAD, _T(""), itemtool77Bitmap, _("Upload File..."), wxITEM_NORMAL);
    wxBitmap itemtool78Bitmap(itemFrame1->GetBitmapResource(wxT("download.xpm")));
    p_toolBar->AddTool(wxID_DOWNLOAD, _T(""), itemtool78Bitmap, _("Download File..."), wxITEM_NORMAL);
    wxBitmap itemtool79Bitmap(itemFrame1->GetBitmapResource(wxT("info.xpm")));
    p_toolBar->AddTool(wxID_VIEW_PROPERTY, _T(""), itemtool79Bitmap, _("View Properties..."), wxITEM_NORMAL);
    p_toolBar->Realize();
    p_topSizer->Add(p_toolBar, 0, wxGROW|wxALL, 0);

    p_splitterWindow = new wxSplitterWindow( itemFrame1, ID_SPLITTERWINDOW, wxDefaultPosition, wxSize(100, 100), wxSP_3DBORDER|wxSP_3DSASH|wxNO_BORDER );

    p_treeCtrl = new WxResourceTreeCtrl( p_splitterWindow, ID_TREECTRL, wxDefaultPosition, wxSize(100, 100), wxTR_HAS_BUTTONS |wxTR_NO_LINES|wxTR_HIDE_ROOT|wxTR_SINGLE|wxSUNKEN_BORDER|wxCLIP_CHILDREN  );

    ewxPanel* itemPanel82 = new ewxPanel( p_splitterWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );

    p_splitterWindow->SplitVertically(p_treeCtrl, itemPanel82, 250);
    p_topSizer->Add(p_splitterWindow, 1, wxGROW, 0);

////@end CalcMgrGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME
 */

void CalcMgrGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_FRAME in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_FRAME in CalcMgrGUI. 
}

/*!
 * wxEVT_CONTEXT_MENU event handler for ID_FRAME
 */

void CalcMgrGUI::OnContextMenu( wxContextMenuEvent& event )
{
////@begin wxEVT_CONTEXT_MENU event handler for ID_FRAME in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CONTEXT_MENU event handler for ID_FRAME in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_IMPORT
 */

void CalcMgrGUI::OnImportClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_IMPORT in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_IMPORT in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXPORT
 */

void CalcMgrGUI::OnExportClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXPORT in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXPORT in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void CalcMgrGUI::OnExitClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE
 */

void CalcMgrGUI::OnDuplicateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE2
 */

void CalcMgrGUI::OnDuplicate2Click( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE2 in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DUPLICATE2 in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CUT
 */

void CalcMgrGUI::OnCutClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CUT in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CUT in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_COPY
 */

void CalcMgrGUI::OnCopyClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_COPY in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_COPY in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PASTE
 */

void CalcMgrGUI::OnPasteClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PASTE in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PASTE in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_RENAME
 */

void CalcMgrGUI::OnRenameClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_RENAME in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_RENAME in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DELETE
 */

void CalcMgrGUI::OnDeleteClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DELETE in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DELETE in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ANNOTATION
 */

void CalcMgrGUI::OnAnnotationClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ANNOTATION in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ANNOTATION in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CHANGE_ACCESS
 */

void CalcMgrGUI::OnChangeAccessClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CHANGE_ACCESS in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CHANGE_ACCESS in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CONFIGURE_NWFS_ARCHIVE
 */

void CalcMgrGUI::OnConfigureNwfsArchiveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CONFIGURE_NWFS_ARCHIVE in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CONFIGURE_NWFS_ARCHIVE in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_TOOLBAR
 */

void CalcMgrGUI::OnShowToolbarClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_TOOLBAR in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_TOOLBAR in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LEGEND
 */

void CalcMgrGUI::OnShowLegendClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LEGEND in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LEGEND in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LINE
 */

void CalcMgrGUI::OnShowLineClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LINE in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_LINE in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_RUNSTATES
 */

void CalcMgrGUI::OnShowRunstatesClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_RUNSTATES in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_RUNSTATES in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_INTERNAL
 */

void CalcMgrGUI::OnShowInternalClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_INTERNAL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SHOW_INTERNAL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOMEASROOT
 */

void CalcMgrGUI::OnHomeasrootClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOMEASROOT in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOMEASROOT in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SORT_TYPE
 */

void CalcMgrGUI::OnSortTreeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SORT_TYPE in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SORT_TYPE in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_BOOKMARK
 */

void CalcMgrGUI::OnAddBookmarkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_BOOKMARK in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_BOOKMARK in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_MANAGE_BOOKMARK
 */

void CalcMgrGUI::OnManageBookmarkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_MANAGE_BOOKMARK in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_MANAGE_BOOKMARK in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
 */

void CalcMgrGUI::OnHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_SUPPORT
 */

void CalcMgrGUI::OnMenuSupportClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_SUPPORT in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_SUPPORT in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_TEST_SUITE
 */

void CalcMgrGUI::OnTestSuiteClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_TEST_SUITE in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_TEST_SUITE in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_VIEW_PROPERTY
 */

void CalcMgrGUI::OnViewPropertyClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_VIEW_PROPERTY in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_VIEW_PROPERTY in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_SERVER
 */

void CalcMgrGUI::OnAddServerClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_SERVER in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ADD_SERVER in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOME
 */

void CalcMgrGUI::OnHomeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOME in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HOME in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UP
 */

void CalcMgrGUI::OnUpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UP in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UP in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_BACKWARD
 */

void CalcMgrGUI::OnBackwardClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_BACKWARD in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_BACKWARD in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FORWARD
 */

void CalcMgrGUI::OnForwardClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FORWARD in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FORWARD in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FIND
 */

void CalcMgrGUI::OnFindClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FIND in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_FIND in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_REFRESH
 */

void CalcMgrGUI::OnRefreshClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_REFRESH in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_REFRESH in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UPLOAD
 */

void CalcMgrGUI::OnUploadClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UPLOAD in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UPLOAD in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DOWNLOAD
 */

void CalcMgrGUI::OnDownloadClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DOWNLOAD in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DOWNLOAD in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_SEL_CHANGED event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlSelChanged( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_SEL_CHANGED event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_SEL_CHANGED event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_SEL_CHANGING event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlSelChanging( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_SEL_CHANGING event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_SEL_CHANGING event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_BEGIN_DRAG event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlBeginDrag( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_BEGIN_DRAG event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_BEGIN_DRAG event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_END_DRAG event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlEndDrag( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_END_DRAG event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_END_DRAG event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_BEGIN_RDRAG event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlBeginRDrag( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_BEGIN_RDRAG event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_BEGIN_RDRAG event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_DELETE_ITEM event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlDeleteItem( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_DELETE_ITEM event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_DELETE_ITEM event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_ITEM_ACTIVATED event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlItemActivated( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_ITEM_ACTIVATED event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_ITEM_ACTIVATED event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_ITEM_COLLAPSED event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlItemCollapsed( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_ITEM_COLLAPSED event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_ITEM_COLLAPSED event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_ITEM_COLLAPSING event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlItemCollapsing( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_ITEM_COLLAPSING event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_ITEM_COLLAPSING event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_ITEM_EXPANDED event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlItemExpanded( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_ITEM_EXPANDED event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_ITEM_EXPANDED event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_ITEM_EXPANDING event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlItemExpanding( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_ITEM_EXPANDING event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_ITEM_EXPANDING event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_KEY_DOWN event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlKeyDown( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_KEY_DOWN event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_KEY_DOWN event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * wxEVT_COMMAND_TREE_ITEM_MENU event handler for ID_TREECTRL
 */

void CalcMgrGUI::OnTreectrlItemMenu( wxTreeEvent& event )
{
////@begin wxEVT_COMMAND_TREE_ITEM_MENU event handler for ID_TREECTRL in CalcMgrGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TREE_ITEM_MENU event handler for ID_TREECTRL in CalcMgrGUI. 
}

/*!
 * Should we show tooltips?
 */

bool CalcMgrGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap CalcMgrGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CalcMgrGUI bitmap retrieval
    wxUnusedVar(name);
    if (name == wxT("gohome.xpm"))
    {
        ewxBitmap bitmap(_T("gohome.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("goup.xpm"))
    {
        ewxBitmap bitmap(_T("goup.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("back.xpm"))
    {
        ewxBitmap bitmap(_T("back.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("forward.xpm"))
    {
        ewxBitmap bitmap(_T("forward.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("editcut.xpm"))
    {
        ewxBitmap bitmap(_T("editcut.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("editcopy.xpm"))
    {
        ewxBitmap bitmap(_T("editcopy.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("editpaste.xpm"))
    {
        ewxBitmap bitmap(_T("editpaste.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("rename.xpm"))
    {
        ewxBitmap bitmap(_T("rename.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("editdelete.xpm"))
    {
        ewxBitmap bitmap(_T("editdelete.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("filefind.xpm"))
    {
        ewxBitmap bitmap(_T("filefind.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("reload.xpm"))
    {
        ewxBitmap bitmap(_T("reload.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("upload.xpm"))
    {
        ewxBitmap bitmap(_T("upload.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("download.xpm"))
    {
        ewxBitmap bitmap(_T("download.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    else if (name == wxT("info.xpm"))
    {
        ewxBitmap bitmap(_T("info.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    return wxNullBitmap;
////@end CalcMgrGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CalcMgrGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CalcMgrGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end CalcMgrGUI icon retrieval
}
