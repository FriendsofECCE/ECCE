/////////////////////////////////////////////////////////////////////////////
// Name:        BuilderGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "BuilderGUI.H"
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
#include "wxgui/ewxTool.H"
#include "wxgui/ewxMenuBar.H"
////@end includes

#include "BuilderGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID BuilderGUI::ID_SELECT_SIDECHAINS = wxNewId();
const wxWindowID BuilderGUI::ID_IMPORTCALC = wxNewId();
const wxWindowID BuilderGUI::ID_MODE_NEXT = wxNewId();
const wxWindowID BuilderGUI::ID_SELECT_BACKBONE = wxNewId();
const wxWindowID BuilderGUI::ID_FRAME_Builder = wxNewId();
const wxWindowID BuilderGUI::ID_UNSELECT = wxNewId();
const wxWindowID BuilderGUI::ID_IMPORT = wxNewId();
const wxWindowID BuilderGUI::ID_SELECT_REVERSE = wxNewId();
const wxWindowID BuilderGUI::ID_SELECT_MOLECULE = wxNewId();
const wxWindowID BuilderGUI::ID_DUMPSG = wxNewId();
const wxWindowID BuilderGUI::ID_SELECT_SOLVENT = wxNewId();
const wxWindowID BuilderGUI::ID_SAVE_THUMB = wxNewId();
const wxWindowID BuilderGUI::ID_SUPPORT = wxNewId();
const wxWindowID BuilderGUI::ID_MODE_PREV = wxNewId();

/*!
 * BuilderGUI type definition
 */

IMPLEMENT_CLASS( BuilderGUI, ewxTool )

/*!
 * BuilderGUI event table definition
 */

BEGIN_EVENT_TABLE( BuilderGUI, ewxTool )

////@begin BuilderGUI event table entries
    EVT_CLOSE( BuilderGUI::mainWindowCloseCB )
    EVT_SIZE( BuilderGUI::OnSize )
    EVT_LEFT_UP( BuilderGUI::OnLeftUp )
    EVT_KEY_DOWN( BuilderGUI::OnKeyDown )

    EVT_MENU( ID_IMPORT, BuilderGUI::OnImportChemsysClick )

    EVT_MENU( ID_IMPORTCALC, BuilderGUI::OnImportCalcClick )

    EVT_MENU( wxID_NEW, BuilderGUI::OnNewClick )

    EVT_MENU( wxID_OPEN, BuilderGUI::OnOpenClick )

    EVT_MENU( wxID_CLOSE, BuilderGUI::OnCloseClick )

    EVT_MENU( wxID_SAVE, BuilderGUI::OnSaveClick )

    EVT_MENU( wxID_SAVEAS, BuilderGUI::OnSaveasClick )

    EVT_MENU( ID_SAVE_THUMB, BuilderGUI::OnSaveThumbClick )

    EVT_MENU( ID_DUMPSG, BuilderGUI::OnDumpsgClick )

    EVT_MENU( wxID_EXIT, BuilderGUI::quitMenuItemClickCB )

    EVT_MENU( wxID_UNDO, BuilderGUI::OnUndoClick )

    EVT_MENU( wxID_REDO, BuilderGUI::OnRedoClick )

    EVT_MENU( wxID_CUT, BuilderGUI::OnCutClick )

    EVT_MENU( wxID_COPY, BuilderGUI::OnCopyClick )

    EVT_MENU( wxID_PASTE, BuilderGUI::OnPasteClick )

    EVT_MENU( wxID_CLEAR, BuilderGUI::OnClearClick )

    EVT_MENU( wxID_DELETE, BuilderGUI::OnDeleteClick )

    EVT_MENU( wxID_SELECTALL, BuilderGUI::OnSelectallClick )

    EVT_MENU( ID_SELECT_MOLECULE, BuilderGUI::OnSelectMoleculeClick )

    EVT_MENU( ID_SELECT_SOLVENT, BuilderGUI::OnSelectSolventClick )

    EVT_MENU( ID_SELECT_BACKBONE, BuilderGUI::OnSelectBackboneClick )

    EVT_MENU( ID_SELECT_SIDECHAINS, BuilderGUI::OnSelectSidechainsClick )

    EVT_MENU( ID_SELECT_REVERSE, BuilderGUI::OnSelectReverseClick )

    EVT_MENU( ID_UNSELECT, BuilderGUI::OnUnselectClick )

    EVT_MENU( ID_MODE_PREV, BuilderGUI::OnModePrevClick )

    EVT_MENU( ID_MODE_NEXT, BuilderGUI::OnModeNextClick )

    EVT_MENU( wxID_HELP, BuilderGUI::helpBuilderMenuitemClick )

    EVT_MENU( ID_SUPPORT, BuilderGUI::helpSupportMenuitemClick )

////@end BuilderGUI event table entries

END_EVENT_TABLE()

/*!
 * BuilderGUI constructors
 */

BuilderGUI::BuilderGUI( )
{
}

BuilderGUI::BuilderGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * BuilderGUI creator
 */

bool BuilderGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin BuilderGUI member initialisation
////@end BuilderGUI member initialisation

////@begin BuilderGUI creation
    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    ewxTool::Create( parent, id, caption, pos, size, style );

    CreateControls();
    Centre();
////@end BuilderGUI creation
    return true;
}

/*!
 * Control creation for BuilderGUI
 */

void BuilderGUI::CreateControls()
{    
////@begin BuilderGUI content construction
    BuilderGUI* itemewxTool1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu3 = new wxMenu;
    itemMenu3->Append(ID_IMPORT, _("Import Chemical System..."), _T(""), wxITEM_NORMAL);
    itemMenu3->Append(ID_IMPORTCALC, _("Import Calculation from Output File..."), _T(""), wxITEM_NORMAL);
    itemMenu3->Append(wxID_NEW, _("New Context\tCtrl+n"), _T(""), wxITEM_NORMAL);
    itemMenu3->Append(wxID_OPEN, _("Open in New Context...\tCtrl+o"), _T(""), wxITEM_NORMAL);
    itemMenu3->Append(wxID_CLOSE, _("Close Context\tCtrl+w"), _T(""), wxITEM_NORMAL);
    itemMenu3->AppendSeparator();
    itemMenu3->Append(wxID_SAVE, _("Save\tCtrl+s"), _T(""), wxITEM_NORMAL);
    itemMenu3->Append(wxID_SAVEAS, _("Save As...\tCtrl+e"), _T(""), wxITEM_NORMAL);
    itemMenu3->Append(ID_SAVE_THUMB, _("Create Thumbnail"), _T(""), wxITEM_NORMAL);
    itemMenu3->Append(ID_DUMPSG, _("Dump Scene Graph"), _T(""), wxITEM_NORMAL);
    itemMenu3->AppendSeparator();
    itemMenu3->Append(wxID_EXIT, _("Quit\tCtrl+q"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu3, _("File"));
    wxMenu* itemMenu16 = new wxMenu;
    itemMenu16->Append(wxID_UNDO, _("Undo..\tCtrl+z"), _T(""), wxITEM_NORMAL);
    itemMenu16->Append(wxID_REDO, _("Redo\tCtrl+y"), _T(""), wxITEM_NORMAL);
    itemMenu16->AppendSeparator();
    itemMenu16->Append(wxID_CUT, _("Cut\tCtrl+x"), _T(""), wxITEM_NORMAL);
    itemMenu16->Append(wxID_COPY, _("Copy\tCtrl+c"), _T(""), wxITEM_NORMAL);
    itemMenu16->Append(wxID_PASTE, _("Paste\tCtrl+v"), _T(""), wxITEM_NORMAL);
    itemMenu16->AppendSeparator();
    itemMenu16->Append(wxID_CLEAR, _T(""), _T(""), wxITEM_NORMAL);
    itemMenu16->Append(wxID_DELETE, _("Delete Selection\tCtrl+d"), _T(""), wxITEM_NORMAL);
    itemMenu16->AppendSeparator();
    itemMenu16->Append(wxID_SELECTALL, _("Select All\tCtrl+a"), _T(""), wxITEM_NORMAL);
    itemMenu16->Append(ID_SELECT_MOLECULE, _("Select Molecule"), _T(""), wxITEM_NORMAL);
    itemMenu16->Append(ID_SELECT_SOLVENT, _("Select Solvent"), _T(""), wxITEM_NORMAL);
    itemMenu16->Append(ID_SELECT_BACKBONE, _("Select Backbone"), _T(""), wxITEM_NORMAL);
    itemMenu16->Append(ID_SELECT_SIDECHAINS, _("Select Side Chains"), _T(""), wxITEM_NORMAL);
    itemMenu16->Append(ID_SELECT_REVERSE, _("Reverse Selection\tCtrl+r"), _T(""), wxITEM_NORMAL);
    itemMenu16->Append(ID_UNSELECT, _("Clear Selection\tCtrl+u"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu16, _("Edit"));
    wxMenu* itemMenu34 = new wxMenu;
    menuBar->Append(itemMenu34, _("Options"));
    wxMenu* itemMenu35 = new wxMenu;
    menuBar->Append(itemMenu35, _("Render"));
    wxMenu* itemMenu36 = new wxMenu;
    menuBar->Append(itemMenu36, _("Build"));
    wxMenu* itemMenu37 = new wxMenu;
    itemMenu37->Append(ID_MODE_PREV, _("Previous Mode\tCTRL+,"), _T(""), wxITEM_NORMAL);
    itemMenu37->Append(ID_MODE_NEXT, _("Next Mode\tCTRL+."), _T(""), wxITEM_NORMAL);
    itemMenu37->AppendSeparator();
    menuBar->Append(itemMenu37, _("Mode"));
    wxMenu* itemMenu41 = new wxMenu;
    menuBar->Append(itemMenu41, _("Measure"));
    wxMenu* itemMenu42 = new wxMenu;
    menuBar->Append(itemMenu42, _("Tools"));
    wxMenu* itemMenu43 = new wxMenu;
    menuBar->Append(itemMenu43, _("Properties"));
    wxMenu* itemMenu44 = new wxMenu;
    menuBar->Append(itemMenu44, _("Context"));
    wxMenu* itemMenu45 = new wxMenu;
    itemMenu45->Append(wxID_HELP, _("on Builder...\tCtrl+h"), _T(""), wxITEM_NORMAL);
    itemMenu45->Append(ID_SUPPORT, _("Support..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu45, _("Help"));
    itemewxTool1->SetMenuBar(menuBar);

////@end BuilderGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME_Builder
 */

void BuilderGUI::mainWindowCloseCB( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_FRAME_Builder in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_FRAME_Builder in BuilderGUI. 
}

/*!
 * wxEVT_SIZE event handler for ID_FRAME_Builder
 */

void BuilderGUI::OnSize( wxSizeEvent& event )
{
////@begin wxEVT_SIZE event handler for ID_FRAME_Builder in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_SIZE event handler for ID_FRAME_Builder in BuilderGUI. 
}

/*!
 * wxEVT_LEFT_UP event handler for ID_FRAME_Builder
 */

void BuilderGUI::OnLeftUp( wxMouseEvent& event )
{
////@begin wxEVT_LEFT_UP event handler for ID_FRAME_Builder in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_LEFT_UP event handler for ID_FRAME_Builder in BuilderGUI. 
}

/*!
 * wxEVT_KEY_DOWN event handler for ID_FRAME_Builder
 */

void BuilderGUI::OnKeyDown( wxKeyEvent& event )
{
////@begin wxEVT_KEY_DOWN event handler for ID_FRAME_Builder in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_KEY_DOWN event handler for ID_FRAME_Builder in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_IMPORT
 */

void BuilderGUI::OnImportChemsysClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_IMPORT in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_IMPORT in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_IMPORTCALC
 */

void BuilderGUI::OnImportCalcClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_IMPORTCALC in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_IMPORTCALC in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_NEW
 */

void BuilderGUI::OnNewClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_NEW in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_NEW in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_OPEN
 */

void BuilderGUI::OnOpenClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_OPEN in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_OPEN in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CLOSE
 */

void BuilderGUI::OnCloseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CLOSE in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CLOSE in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
 */

void BuilderGUI::OnSaveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVEAS
 */

void BuilderGUI::OnSaveasClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVEAS in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVEAS in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_SAVE_THUMB
 */

void BuilderGUI::OnSaveThumbClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_SAVE_THUMB in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_SAVE_THUMB in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_DUMPSG
 */

void BuilderGUI::OnDumpsgClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_DUMPSG in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_DUMPSG in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void BuilderGUI::quitMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UNDO
 */

void BuilderGUI::OnUndoClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UNDO in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_UNDO in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_REDO
 */

void BuilderGUI::OnRedoClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_REDO in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_REDO in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CUT
 */

void BuilderGUI::OnCutClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CUT in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CUT in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_COPY
 */

void BuilderGUI::OnCopyClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_COPY in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_COPY in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PASTE
 */

void BuilderGUI::OnPasteClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PASTE in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_PASTE in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CLEAR
 */

void BuilderGUI::OnClearClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CLEAR in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_CLEAR in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DELETE
 */

void BuilderGUI::OnDeleteClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DELETE in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_DELETE in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SELECTALL
 */

void BuilderGUI::OnSelectallClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SELECTALL in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SELECTALL in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_MOLECULE
 */

void BuilderGUI::OnSelectMoleculeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_MOLECULE in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_MOLECULE in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_SOLVENT
 */

void BuilderGUI::OnSelectSolventClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_SOLVENT in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_SOLVENT in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_BACKBONE
 */

void BuilderGUI::OnSelectBackboneClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_BACKBONE in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_BACKBONE in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_SIDECHAINS
 */

void BuilderGUI::OnSelectSidechainsClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_SIDECHAINS in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_SIDECHAINS in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_REVERSE
 */

void BuilderGUI::OnSelectReverseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_REVERSE in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_SELECT_REVERSE in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_UNSELECT
 */

void BuilderGUI::OnUnselectClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_UNSELECT in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_UNSELECT in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MODE_PREV
 */

void BuilderGUI::OnModePrevClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MODE_PREV in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MODE_PREV in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MODE_NEXT
 */

void BuilderGUI::OnModeNextClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MODE_NEXT in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MODE_NEXT in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
 */

void BuilderGUI::helpBuilderMenuitemClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in BuilderGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_SUPPORT
 */

void BuilderGUI::helpSupportMenuitemClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_SUPPORT in BuilderGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_SUPPORT in BuilderGUI. 
}

/*!
 * Should we show tooltips?
 */

bool BuilderGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap BuilderGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin BuilderGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end BuilderGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon BuilderGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin BuilderGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end BuilderGUI icon retrieval
}
