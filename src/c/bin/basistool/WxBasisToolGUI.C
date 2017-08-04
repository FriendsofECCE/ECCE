/////////////////////////////////////////////////////////////////////////////
// Name:        WxBasisToolGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxBasisToolGUI.H"
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
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxGrid.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxFrame.H"
////@end includes

#include "WxBasisToolGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxBasisToolGUI::ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_04 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_SPLITTER_WXBASISTOOL_MAIN = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BOTTOM = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_0 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_NOTEBOOK_WXBASISTOOL_BASISSETS = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_08 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_MENUITEM_WXBASISTOOL_HELP_SUPPORT = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_1 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_10 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_03 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_11 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_BUTTON_WXBASISTOOL_SHOWEDITOR = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_07 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_2 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_MENUITEM_WXBASISTOOL_PROGRAM_COEFFICIENTS = wxNewId();
const wxWindowID WxBasisToolGUI::ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_1 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_06 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_CHECKBOX_WXBASISTOOL_OPTIMIZE = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_04 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_CONTEXT_OPERATIONS = wxNewId();
const wxWindowID WxBasisToolGUI::ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_2 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_MAIN = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_02 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_STATIC_WXBASISTOOL_FUNCTIONS = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_DIVIDER = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_1 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSETS = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL17 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_STATIC_WXBASISTOOL_CONTEXT_FORMULA = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_09 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_0 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_01 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_FRAME = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_05 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_0 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_11 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_STATIC_WXBASISTOOL_BASISSET_CATEGORY_1 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_03 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_2 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_MENUITEM_WXBASISTOOL_EDIT_CLEARSELECTION = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_10 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_08 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_STATIC_WXBASISTOOL_EFFPRIMATIVES = wxNewId();
const wxWindowID WxBasisToolGUI::ID_STATIC_WXBASISTOOL_PRIMATIVES = wxNewId();
const wxWindowID WxBasisToolGUI::ID_BUTTON_WXBASISTOOL_CONTEXT_ADD = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_PERTABLE = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_07 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_BUTTON_WXBASISTOOL_DETAILS = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_09 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_CONTEXTMOLECULE = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_00 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_FEEDBACK_WXBASISTOOL_MESSAGES = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_TOP = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_02 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_1 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_1 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_01 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_MENUITEM_WXBASISTOOL_EDIT_CLEARALL = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_3 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_06 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_PANEL_WXBASISTOOL_BASISSET_TYPE_05 = wxNewId();
const wxWindowID WxBasisToolGUI::ID_CHECKBOX_WXBASISTOOL_CONTEXT_ALLOWMULTIPLE = wxNewId();

/*!
 * WxBasisToolGUI type definition
 */

IMPLEMENT_CLASS( WxBasisToolGUI, ewxFrame )

/*!
 * WxBasisToolGUI event table definition
 */

BEGIN_EVENT_TABLE( WxBasisToolGUI, ewxFrame )

////@begin WxBasisToolGUI event table entries
    EVT_CLOSE( WxBasisToolGUI::windowCloseCB )
    EVT_SIZE( WxBasisToolGUI::windowSizeCB )

    EVT_SPLITTER_SASH_POS_CHANGED( ID_SPLITTER_WXBASISTOOL_MAIN, WxBasisToolGUI::windowSplitterSashPosChangedCB )
    EVT_SPLITTER_SASH_POS_CHANGING( ID_SPLITTER_WXBASISTOOL_MAIN, WxBasisToolGUI::windowSplitterSashPosChangingCB )
    EVT_SIZE( WxBasisToolGUI::windowSplitterSizeCB )

    EVT_NOTEBOOK_PAGE_CHANGED( ID_NOTEBOOK_WXBASISTOOL_BASISSETS, WxBasisToolGUI::basisSetsNotebookPageChangedCB )
    EVT_NOTEBOOK_PAGE_CHANGING( ID_NOTEBOOK_WXBASISTOOL_BASISSETS, WxBasisToolGUI::basisSetsNotebookPageChangingCB )

    EVT_NOTEBOOK_PAGE_CHANGED( ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0, WxBasisToolGUI::basisSetCategoryNotebookPageChangedCB )
    EVT_NOTEBOOK_PAGE_CHANGING( ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0, WxBasisToolGUI::basisSetCategoryNotebookPageChanging )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_01, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_01, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_02, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_02, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_03, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_03, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_04, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_04, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_05, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_05, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_NOTEBOOK_PAGE_CHANGED( ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_1, WxBasisToolGUI::basisSetCategoryNotebookPageChangedCB )
    EVT_NOTEBOOK_PAGE_CHANGING( ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_1, WxBasisToolGUI::basisSetCategoryNotebookPageChanging )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_06, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_06, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_07, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_07, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_08, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_08, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_09, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_09, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_10, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_10, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_LISTBOX( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_11, WxBasisToolGUI::basisSetTypeListboxSelectedCB )
    EVT_LISTBOX_DCLICK( ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_11, WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB )

    EVT_BUTTON( ID_BUTTON_WXBASISTOOL_DETAILS, WxBasisToolGUI::detailsButtonClickCB )

    EVT_BUTTON( ID_BUTTON_WXBASISTOOL_CONTEXT_ADD, WxBasisToolGUI::contextAddButtonClickCB )

    EVT_NOTEBOOK_PAGE_CHANGED( ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS, WxBasisToolGUI::contextBasisSetsNotebookPageChangedCB )
    EVT_NOTEBOOK_PAGE_CHANGING( ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS, WxBasisToolGUI::contextBasisSetsNotebookPageChangingCB )

    EVT_GRID_CMD_SELECT_CELL( ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0, WxBasisToolGUI::contextBasisSetsGridCellSelectCB )
    EVT_GRID_CMD_RANGE_SELECT( ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0, WxBasisToolGUI::contextBasisSetsGridRangeSelectCB )

    EVT_GRID_CMD_SELECT_CELL( ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_1, WxBasisToolGUI::contextBasisSetsGridCellSelectCB )
    EVT_GRID_CMD_RANGE_SELECT( ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_1, WxBasisToolGUI::contextBasisSetsGridRangeSelectCB )

    EVT_GRID_CMD_SELECT_CELL( ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_2, WxBasisToolGUI::contextBasisSetsGridCellSelectCB )
    EVT_GRID_CMD_RANGE_SELECT( ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_2, WxBasisToolGUI::contextBasisSetsGridRangeSelectCB )

    EVT_CHECKBOX( ID_CHECKBOX_WXBASISTOOL_CONTEXT_ALLOWMULTIPLE, WxBasisToolGUI::contextMultipleAllowedCheckboxClickCB )

    EVT_BUTTON( ID_BUTTON_WXBASISTOOL_SHOWEDITOR, WxBasisToolGUI::showEditorButtonClickCB )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_1, WxBasisToolGUI::contextPolarizationRadioButtonSelectedCB )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_0, WxBasisToolGUI::contextPolarizationRadioButtonSelectedCB )

    EVT_CHECKBOX( ID_CHECKBOX_WXBASISTOOL_OPTIMIZE, WxBasisToolGUI::contextOptimizeCheckboxClickCB )

    EVT_MENU( wxID_SAVE, WxBasisToolGUI::programSaveMenuItemClickCB )

    EVT_MENU( ID_MENUITEM_WXBASISTOOL_PROGRAM_COEFFICIENTS, WxBasisToolGUI::programCoefficientsMenuItemClickCB )

    EVT_MENU( wxID_EXIT, WxBasisToolGUI::programQuitMenuItemClickCB )

    EVT_MENU( ID_MENUITEM_WXBASISTOOL_EDIT_CLEARALL, WxBasisToolGUI::editClearAllMenuItemClickCB )

    EVT_MENU( ID_MENUITEM_WXBASISTOOL_EDIT_CLEARSELECTION, WxBasisToolGUI::editClearSelectionMenuItemClickCB )

    EVT_MENU( wxID_HELP, WxBasisToolGUI::helpProgramMenuItemClickCB )

    EVT_MENU( ID_MENUITEM_WXBASISTOOL_HELP_SUPPORT, WxBasisToolGUI::helpSupportMenuItemClickCB )

////@end WxBasisToolGUI event table entries

END_EVENT_TABLE()

/*!
 * WxBasisToolGUI constructors
 */

WxBasisToolGUI::WxBasisToolGUI( )
{
}

WxBasisToolGUI::WxBasisToolGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * WxBasisToolGUI creator
 */

bool WxBasisToolGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxBasisToolGUI member initialisation
////@end WxBasisToolGUI member initialisation

////@begin WxBasisToolGUI creation
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
////@end WxBasisToolGUI creation
    return true;
}

/*!
 * Control creation for WxBasisToolGUI
 */

void WxBasisToolGUI::CreateControls()
{    
////@begin WxBasisToolGUI content construction
    WxBasisToolGUI* itemFrame1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu110 = new wxMenu;
    itemMenu110->Append(wxID_SAVE, _("&Save\tCTRL+S"), _T(""), wxITEM_NORMAL);
    itemMenu110->Append(ID_MENUITEM_WXBASISTOOL_PROGRAM_COEFFICIENTS, _("View Exponents and Coefficients..."), _T(""), wxITEM_NORMAL);
    itemMenu110->Enable(ID_MENUITEM_WXBASISTOOL_PROGRAM_COEFFICIENTS, false);
    itemMenu110->AppendSeparator();
    itemMenu110->Append(wxID_EXIT, _("&Quit\tCTRL+Q"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu110, _("Molecular Basis"));
    wxMenu* itemMenu115 = new wxMenu;
    itemMenu115->Append(ID_MENUITEM_WXBASISTOOL_EDIT_CLEARALL, _("Clear All"), _T(""), wxITEM_NORMAL);
    itemMenu115->Append(ID_MENUITEM_WXBASISTOOL_EDIT_CLEARSELECTION, _("Clear Selection"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu115, _("Edit"));
    wxMenu* itemMenu118 = new wxMenu;
    itemMenu118->Append(wxID_HELP, _("On Basis Set Tool...\tCTRL+H"), _T(""), wxITEM_NORMAL);
    itemMenu118->Append(ID_MENUITEM_WXBASISTOOL_HELP_SUPPORT, _("Support..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu118, _("Help"));
    itemFrame1->SetMenuBar(menuBar);

    ewxPanel* itemPanel2 = new ewxPanel( itemFrame1, ID_PANEL_WXBASISTOOL_MAIN, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemPanel2->SetSizer(itemBoxSizer3);

    wxSplitterWindow* itemSplitterWindow4 = new wxSplitterWindow( itemPanel2, ID_SPLITTER_WXBASISTOOL_MAIN, wxDefaultPosition, wxSize(100, 100), wxSP_3DSASH|wxNO_BORDER );

    ewxPanel* itemPanel5 = new ewxPanel( itemSplitterWindow4, ID_PANEL_WXBASISTOOL_TOP, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel5->SetSizer(itemBoxSizer6);

    ewxPanel* itemPanel7 = new ewxPanel( itemPanel5, ID_PANEL_WXBASISTOOL_BASISSETS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer6->Add(itemPanel7, 0, wxGROW|wxALL, 3);
    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxVERTICAL);
    itemPanel7->SetSizer(itemBoxSizer8);

    ewxNotebook* itemNotebook9 = new ewxNotebook( itemPanel7, ID_NOTEBOOK_WXBASISTOOL_BASISSETS, wxDefaultPosition, wxDefaultSize, wxNB_TOP|wxNO_BORDER );

    ewxPanel* itemPanel10 = new ewxPanel( itemNotebook9, ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_0, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxVERTICAL);
    itemPanel10->SetSizer(itemBoxSizer11);

    ewxNotebook* itemNotebook12 = new ewxNotebook( itemPanel10, ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0, wxDefaultPosition, wxDefaultSize, wxNB_LEFT );

    ewxPanel* itemPanel13 = new ewxPanel( itemNotebook12, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_00, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxVERTICAL);
    itemPanel13->SetSizer(itemBoxSizer14);

    wxString* itemListBox15Strings = NULL;
    ewxListBox* itemListBox15 = new ewxListBox( itemPanel13, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00, wxDefaultPosition, wxDefaultSize, 0, itemListBox15Strings, wxLB_SINGLE );
    itemBoxSizer14->Add(itemListBox15, 1, wxGROW|wxALL, 3);

    itemNotebook12->AddPage(itemPanel13, _("Pople Shared"));

    ewxPanel* itemPanel16 = new ewxPanel( itemNotebook12, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_01, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxVERTICAL);
    itemPanel16->SetSizer(itemBoxSizer17);

    wxString* itemListBox18Strings = NULL;
    ewxListBox* itemListBox18 = new ewxListBox( itemPanel16, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_01, wxDefaultPosition, wxDefaultSize, 0, itemListBox18Strings, wxLB_SINGLE );
    itemBoxSizer17->Add(itemListBox18, 1, wxGROW|wxALL, 3);

    itemNotebook12->AddPage(itemPanel16, _("Other Segmented"));

    ewxPanel* itemPanel19 = new ewxPanel( itemNotebook12, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_02, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer20 = new wxBoxSizer(wxVERTICAL);
    itemPanel19->SetSizer(itemBoxSizer20);

    wxString* itemListBox21Strings = NULL;
    ewxListBox* itemListBox21 = new ewxListBox( itemPanel19, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_02, wxDefaultPosition, wxDefaultSize, 0, itemListBox21Strings, wxLB_SINGLE );
    itemBoxSizer20->Add(itemListBox21, 1, wxGROW|wxALL, 3);

    itemNotebook12->AddPage(itemPanel19, _("Corr. Consistent"));

    ewxPanel* itemPanel22 = new ewxPanel( itemNotebook12, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_03, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer23 = new wxBoxSizer(wxVERTICAL);
    itemPanel22->SetSizer(itemBoxSizer23);

    wxString* itemListBox24Strings = NULL;
    ewxListBox* itemListBox24 = new ewxListBox( itemPanel22, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_03, wxDefaultPosition, wxDefaultSize, 0, itemListBox24Strings, wxLB_SINGLE );
    itemBoxSizer23->Add(itemListBox24, 1, wxGROW|wxALL, 3);

    itemNotebook12->AddPage(itemPanel22, _("Other Gen. Contr."));

    ewxPanel* itemPanel25 = new ewxPanel( itemNotebook12, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_04, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxVERTICAL);
    itemPanel25->SetSizer(itemBoxSizer26);

    wxString* itemListBox27Strings = NULL;
    ewxListBox* itemListBox27 = new ewxListBox( itemPanel25, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_04, wxDefaultPosition, wxDefaultSize, 0, itemListBox27Strings, wxLB_SINGLE );
    itemBoxSizer26->Add(itemListBox27, 1, wxGROW|wxALL, 3);

    itemNotebook12->AddPage(itemPanel25, _("ECP Orbital"));

    ewxPanel* itemPanel28 = new ewxPanel( itemNotebook12, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_05, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer29 = new wxBoxSizer(wxVERTICAL);
    itemPanel28->SetSizer(itemBoxSizer29);

    wxString* itemListBox30Strings = NULL;
    ewxListBox* itemListBox30 = new ewxListBox( itemPanel28, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_05, wxDefaultPosition, wxDefaultSize, 0, itemListBox30Strings, wxLB_SINGLE );
    itemBoxSizer29->Add(itemListBox30, 1, wxGROW|wxALL, 3);

    itemNotebook12->AddPage(itemPanel28, _("DFT Orbital"));

    itemBoxSizer11->Add(itemNotebook12, 1, wxGROW|wxALL, 3);

    itemNotebook9->AddPage(itemPanel10, _("Orbital"));

    ewxPanel* itemPanel31 = new ewxPanel( itemNotebook9, ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer32 = new wxBoxSizer(wxVERTICAL);
    itemPanel31->SetSizer(itemBoxSizer32);

    ewxNotebook* itemNotebook33 = new ewxNotebook( itemPanel31, ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_1, wxDefaultPosition, wxDefaultSize, wxNB_LEFT );

    ewxPanel* itemPanel34 = new ewxPanel( itemNotebook33, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_06, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer35 = new wxBoxSizer(wxVERTICAL);
    itemPanel34->SetSizer(itemBoxSizer35);

    wxString* itemListBox36Strings = NULL;
    ewxListBox* itemListBox36 = new ewxListBox( itemPanel34, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_06, wxDefaultPosition, wxDefaultSize, 0, itemListBox36Strings, wxLB_SINGLE );
    itemBoxSizer35->Add(itemListBox36, 1, wxGROW|wxALL, 3);

    itemNotebook33->AddPage(itemPanel34, _("Polarization"));

    ewxPanel* itemPanel37 = new ewxPanel( itemNotebook33, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_07, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer38 = new wxBoxSizer(wxVERTICAL);
    itemPanel37->SetSizer(itemBoxSizer38);

    wxString* itemListBox39Strings = NULL;
    ewxListBox* itemListBox39 = new ewxListBox( itemPanel37, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_07, wxDefaultPosition, wxDefaultSize, 0, itemListBox39Strings, wxLB_SINGLE );
    itemBoxSizer38->Add(itemListBox39, 1, wxGROW|wxALL, 3);

    itemNotebook33->AddPage(itemPanel37, _("Diffuse"));

    ewxPanel* itemPanel40 = new ewxPanel( itemNotebook33, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_08, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer41 = new wxBoxSizer(wxVERTICAL);
    itemPanel40->SetSizer(itemBoxSizer41);

    wxString* itemListBox42Strings = NULL;
    ewxListBox* itemListBox42 = new ewxListBox( itemPanel40, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_08, wxDefaultPosition, wxDefaultSize, 0, itemListBox42Strings, wxLB_SINGLE );
    itemBoxSizer41->Add(itemListBox42, 1, wxGROW|wxALL, 3);

    itemNotebook33->AddPage(itemPanel40, _("Rydberg"));

    itemBoxSizer32->Add(itemNotebook33, 1, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText43 = new ewxStaticText( itemPanel31, ID_STATIC_WXBASISTOOL_BASISSET_CATEGORY_1, _("You must first select\nan Orbital Basis Set"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemStaticText43->Show(false);
    itemBoxSizer32->Add(itemStaticText43, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 3);

    itemNotebook9->AddPage(itemPanel31, _("Auxiliary"));

    ewxPanel* itemPanel44 = new ewxPanel( itemNotebook9, ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_2, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer45 = new wxBoxSizer(wxVERTICAL);
    itemPanel44->SetSizer(itemBoxSizer45);

    ewxPanel* itemPanel46 = new ewxPanel( itemPanel44, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_09, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer45->Add(itemPanel46, 1, wxGROW|wxALL, 3);
    wxBoxSizer* itemBoxSizer47 = new wxBoxSizer(wxVERTICAL);
    itemPanel46->SetSizer(itemBoxSizer47);

    wxString* itemListBox48Strings = NULL;
    ewxListBox* itemListBox48 = new ewxListBox( itemPanel46, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_09, wxDefaultPosition, wxDefaultSize, 0, itemListBox48Strings, wxLB_SINGLE );
    itemBoxSizer47->Add(itemListBox48, 1, wxGROW|wxALL, 3);

    itemNotebook9->AddPage(itemPanel44, _("ECP"));

    ewxPanel* itemPanel49 = new ewxPanel( itemNotebook9, ID_PANEL_WXBASISTOOL_BASISSET_CATEGORY_3, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer50 = new wxBoxSizer(wxVERTICAL);
    itemPanel49->SetSizer(itemBoxSizer50);

    ewxPanel* itemPanel51 = new ewxPanel( itemPanel49, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_10, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer50->Add(itemPanel51, 1, wxGROW|wxALL, 3);
    wxBoxSizer* itemBoxSizer52 = new wxBoxSizer(wxVERTICAL);
    itemPanel51->SetSizer(itemBoxSizer52);

    ewxStaticText* itemStaticText53 = new ewxStaticText( itemPanel51, wxID_STATIC, _("Charge Fitting"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer52->Add(itemStaticText53, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemListBox54Strings = NULL;
    ewxListBox* itemListBox54 = new ewxListBox( itemPanel51, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_10, wxDefaultPosition, wxDefaultSize, 0, itemListBox54Strings, wxLB_SINGLE );
    itemBoxSizer52->Add(itemListBox54, 1, wxGROW|wxALL, 3);

    itemBoxSizer50->Add(5, 5, 0, wxGROW|wxALL, 3);

    ewxPanel* itemPanel56 = new ewxPanel( itemPanel49, ID_PANEL_WXBASISTOOL_BASISSET_TYPE_11, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer50->Add(itemPanel56, 1, wxGROW|wxALL, 3);
    wxBoxSizer* itemBoxSizer57 = new wxBoxSizer(wxVERTICAL);
    itemPanel56->SetSizer(itemBoxSizer57);

    ewxStaticText* itemStaticText58 = new ewxStaticText( itemPanel56, wxID_STATIC, _("Exchange Fitting"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer57->Add(itemStaticText58, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemListBox59Strings = NULL;
    ewxListBox* itemListBox59 = new ewxListBox( itemPanel56, ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_11, wxDefaultPosition, wxDefaultSize, 0, itemListBox59Strings, wxLB_SINGLE );
    itemBoxSizer57->Add(itemListBox59, 1, wxGROW|wxALL, 3);

    itemNotebook9->AddPage(itemPanel49, _("DFT"));

    itemBoxSizer8->Add(itemNotebook9, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer60 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer8->Add(itemBoxSizer60, 0, wxALIGN_LEFT, 3);
    ewxButton* itemButton61 = new ewxButton( itemPanel7, ID_BUTTON_WXBASISTOOL_DETAILS, _("Details..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer60->Add(itemButton61, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton62 = new ewxButton( itemPanel7, ID_BUTTON_WXBASISTOOL_CONTEXT_ADD, _("Add to Table"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer60->Add(itemButton62, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel63 = new ewxPanel( itemPanel5, ID_PANEL_WXBASISTOOL_CONTEXTMOLECULE, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer6->Add(itemPanel63, 1, wxGROW, 3);
    wxBoxSizer* itemBoxSizer64 = new wxBoxSizer(wxVERTICAL);
    itemPanel63->SetSizer(itemBoxSizer64);

    wxBoxSizer* itemBoxSizer65 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer64->Add(itemBoxSizer65, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT, 15);
    ewxStaticText* itemStaticText66 = new ewxStaticText( itemPanel63, wxID_STATIC, _("Chemical Formula:"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemStaticText66->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemBoxSizer65->Add(itemStaticText66, 0, wxALIGN_BOTTOM|wxRIGHT|wxTOP, 3);

    ewxStaticText* itemStaticText67 = new ewxStaticText( itemPanel63, ID_STATIC_WXBASISTOOL_CONTEXT_FORMULA, _T(""), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT|wxSUNKEN_BORDER );
    itemStaticText67->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemBoxSizer65->Add(itemStaticText67, 1, wxALIGN_BOTTOM|wxLEFT|wxRIGHT|wxTOP, 3);

    ewxPanel* itemPanel68 = new ewxPanel( itemPanel63, ID_PANEL_WXBASISTOOL_PERTABLE, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer64->Add(itemPanel68, 1, wxGROW|wxFIXED_MINSIZE, 0);
    wxBoxSizer* itemBoxSizer69 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel68->SetSizer(itemBoxSizer69);

    ewxStaticText* itemStaticText70 = new ewxStaticText( itemPanel63, wxID_STATIC, _("To view only those basis sets that support specific elements,\nselect those elements from the periodic table above."), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemStaticText70->SetFont(wxFont(8, wxSWISS, wxITALIC, wxNORMAL, false, _T("Sans")));
    itemBoxSizer64->Add(itemStaticText70, 0, wxGROW|wxALL|wxFIXED_MINSIZE, 3);

    ewxPanel* itemPanel71 = new ewxPanel( itemSplitterWindow4, ID_PANEL_WXBASISTOOL_BOTTOM, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer72 = new wxBoxSizer(wxVERTICAL);
    itemPanel71->SetSizer(itemBoxSizer72);

    ewxPanel* itemPanel73 = new ewxPanel( itemPanel71, ID_PANEL_WXBASISTOOL_DIVIDER, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer72->Add(itemPanel73, 0, wxGROW, 3);
    wxBoxSizer* itemBoxSizer74 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel73->SetSizer(itemBoxSizer74);

    ewxStaticLine* itemStaticLine75 = new ewxStaticLine( itemPanel73, wxID_STATIC, wxDefaultPosition, wxSize(15, -1), wxLI_HORIZONTAL );
    itemBoxSizer74->Add(itemStaticLine75, 0, wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText76 = new ewxStaticText( itemPanel73, wxID_STATIC, _("Basis Set Selections"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer74->Add(itemStaticText76, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticLine* itemStaticLine77 = new ewxStaticLine( itemPanel73, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer74->Add(itemStaticLine77, 1, wxALIGN_CENTER_VERTICAL, 3);

    wxBoxSizer* itemBoxSizer78 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer72->Add(itemBoxSizer78, 1, wxGROW, 3);
    ewxNotebook* itemNotebook79 = new ewxNotebook( itemPanel71, ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS, wxDefaultPosition, wxDefaultSize, wxNB_TOP|wxNO_BORDER );

    ewxPanel* itemPanel80 = new ewxPanel( itemNotebook79, ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_0, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer81 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel80->SetSizer(itemBoxSizer81);

    ewxGrid* itemGrid82 = new ewxGrid( itemPanel80, ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0, wxDefaultPosition, wxSize(200, 150), wxHSCROLL|wxVSCROLL );
    itemGrid82->SetDefaultColSize(50);
    itemGrid82->SetDefaultRowSize(25);
    itemGrid82->SetColLabelSize(25);
    itemGrid82->SetRowLabelSize(50);
    itemGrid82->CreateGrid(1, 7, wxGrid::wxGridSelectCells);
    itemBoxSizer81->Add(itemGrid82, 1, wxGROW|wxALL, 3);

    itemNotebook79->AddPage(itemPanel80, _("Simple"));

    ewxPanel* itemPanel83 = new ewxPanel( itemNotebook79, ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer84 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel83->SetSizer(itemBoxSizer84);

    ewxGrid* itemGrid85 = new ewxGrid( itemPanel83, ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_1, wxDefaultPosition, wxSize(200, 150), wxHSCROLL|wxVSCROLL );
    itemGrid85->SetDefaultColSize(50);
    itemGrid85->SetDefaultRowSize(25);
    itemGrid85->SetColLabelSize(25);
    itemGrid85->SetRowLabelSize(50);
    itemGrid85->CreateGrid(1, 7, wxGrid::wxGridSelectCells);
    itemBoxSizer84->Add(itemGrid85, 1, wxGROW|wxALL, 3);

    itemNotebook79->AddPage(itemPanel83, _("Element"));

    ewxPanel* itemPanel86 = new ewxPanel( itemNotebook79, ID_PANEL_WXBASISTOOL_CONTEXT_BASISSETS_2, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel86->Show(false);
    wxBoxSizer* itemBoxSizer87 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel86->SetSizer(itemBoxSizer87);

    ewxGrid* itemGrid88 = new ewxGrid( itemPanel86, ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_2, wxDefaultPosition, wxSize(200, 150), wxHSCROLL|wxVSCROLL );
    itemGrid88->SetDefaultColSize(50);
    itemGrid88->SetDefaultRowSize(25);
    itemGrid88->SetColLabelSize(25);
    itemGrid88->SetRowLabelSize(50);
    itemGrid88->CreateGrid(1, 7, wxGrid::wxGridSelectCells);
    itemBoxSizer87->Add(itemGrid88, 1, wxGROW|wxALL, 3);

    itemNotebook79->AddPage(itemPanel86, _("Atom"));

    itemBoxSizer78->Add(itemNotebook79, 1, wxGROW|wxALL, 3);

    ewxPanel* itemPanel89 = new ewxPanel( itemPanel71, ID_PANEL_WXBASISTOOL_CONTEXT_OPERATIONS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer78->Add(itemPanel89, 0, wxALIGN_BOTTOM|wxALL, 3);
    wxBoxSizer* itemBoxSizer90 = new wxBoxSizer(wxVERTICAL);
    itemPanel89->SetSizer(itemBoxSizer90);

    ewxCheckBox* itemCheckBox91 = new ewxCheckBox( itemPanel89, ID_CHECKBOX_WXBASISTOOL_CONTEXT_ALLOWMULTIPLE, _("Allow Multiple Basis Sets"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox91->SetValue(true);
    itemCheckBox91->Show(false);
    itemBoxSizer90->Add(itemCheckBox91, 0, wxALIGN_LEFT|wxALL, 3);

    ewxButton* itemButton92 = new ewxButton( itemPanel89, ID_BUTTON_WXBASISTOOL_SHOWEDITOR, _("Contraction Editor..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer90->Add(itemButton92, 0, wxALIGN_LEFT|wxALL, 3);

    ewxPanel* itemPanel93 = new ewxPanel( itemPanel89, ID_PANEL17, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer90->Add(itemPanel93, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxTOP, 3);
    wxBoxSizer* itemBoxSizer94 = new wxBoxSizer(wxVERTICAL);
    itemPanel93->SetSizer(itemBoxSizer94);

    ewxStaticText* itemStaticText95 = new ewxStaticText( itemPanel93, wxID_STATIC, _("Polarization Components:"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemBoxSizer94->Add(itemStaticText95, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer96 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer94->Add(itemBoxSizer96, 0, wxGROW, 3);
    ewxRadioButton* itemRadioButton97 = new ewxRadioButton( itemPanel93, ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_1, _("Spherical"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton97->SetValue(false);
    itemBoxSizer96->Add(itemRadioButton97, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton98 = new ewxRadioButton( itemPanel93, ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_0, _("Cartesian"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton98->SetValue(false);
    itemBoxSizer96->Add(itemRadioButton98, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox99 = new ewxCheckBox( itemPanel93, ID_CHECKBOX_WXBASISTOOL_OPTIMIZE, _("Optimize General Contractions"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox99->SetValue(false);
    itemBoxSizer94->Add(itemCheckBox99, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    ewxStaticLine* itemStaticLine100 = new ewxStaticLine( itemPanel93, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer94->Add(itemStaticLine100, 1, wxGROW, 3);

    wxGridSizer* itemGridSizer101 = new wxGridSizer(3, 2, 0, 0);
    itemBoxSizer94->Add(itemGridSizer101, 0, wxGROW, 3);
    ewxStaticText* itemStaticText102 = new ewxStaticText( itemPanel93, wxID_STATIC, _("Functions:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemGridSizer101->Add(itemStaticText102, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText103 = new ewxStaticText( itemPanel93, ID_STATIC_WXBASISTOOL_FUNCTIONS, _("40"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer101->Add(itemStaticText103, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText104 = new ewxStaticText( itemPanel93, wxID_STATIC, _("Primitives:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer101->Add(itemStaticText104, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText105 = new ewxStaticText( itemPanel93, ID_STATIC_WXBASISTOOL_PRIMATIVES, _("80"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer101->Add(itemStaticText105, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText106 = new ewxStaticText( itemPanel93, wxID_STATIC, _("Eff. Primitives:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer101->Add(itemStaticText106, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText107 = new ewxStaticText( itemPanel93, ID_STATIC_WXBASISTOOL_EFFPRIMATIVES, _("80"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer101->Add(itemStaticText107, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemSplitterWindow4->SplitHorizontally(itemPanel5, itemPanel71, 300);
    itemBoxSizer3->Add(itemSplitterWindow4, 1, wxGROW, 3);

    WxFeedback* itemWxFeedback108 = new WxFeedback( itemPanel2, ID_FEEDBACK_WXBASISTOOL_MESSAGES, wxDefaultPosition, wxSize(100, 100), wxNO_BORDER );
    itemBoxSizer3->Add(itemWxFeedback108, 0, wxGROW|wxALL, 3);

////@end WxBasisToolGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME
 */

void WxBasisToolGUI::windowCloseCB( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_FRAME in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_FRAME in WxBasisToolGUI. 
}

/*!
 * wxEVT_SIZE event handler for ID_FRAME
 */

void WxBasisToolGUI::windowSizeCB( wxSizeEvent& event )
{
////@begin wxEVT_SIZE event handler for ID_FRAME in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_SIZE event handler for ID_FRAME in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED event handler for ID_SPLITTER_WXBASISTOOL_MAIN
 */

void WxBasisToolGUI::windowSplitterSashPosChangedCB( wxSplitterEvent& event )
{
////@begin wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED event handler for ID_SPLITTER_WXBASISTOOL_MAIN in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGED event handler for ID_SPLITTER_WXBASISTOOL_MAIN in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING event handler for ID_SPLITTER_WXBASISTOOL_MAIN
 */

void WxBasisToolGUI::windowSplitterSashPosChangingCB( wxSplitterEvent& event )
{
////@begin wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING event handler for ID_SPLITTER_WXBASISTOOL_MAIN in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPLITTER_SASH_POS_CHANGING event handler for ID_SPLITTER_WXBASISTOOL_MAIN in WxBasisToolGUI. 
}

/*!
 * wxEVT_SIZE event handler for ID_SPLITTER_WXBASISTOOL_MAIN
 */

void WxBasisToolGUI::windowSplitterSizeCB( wxSizeEvent& event )
{
////@begin wxEVT_SIZE event handler for ID_SPLITTER_WXBASISTOOL_MAIN in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_SIZE event handler for ID_SPLITTER_WXBASISTOOL_MAIN in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSETS
 */

void WxBasisToolGUI::basisSetsNotebookPageChangedCB( wxNotebookEvent& event )
{
////@begin wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSETS in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSETS in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSETS
 */

void WxBasisToolGUI::basisSetsNotebookPageChangingCB( wxNotebookEvent& event )
{
////@begin wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSETS in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSETS in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0
 */

void WxBasisToolGUI::basisSetCategoryNotebookPageChangedCB( wxNotebookEvent& event )
{
////@begin wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0 in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0 in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0
 */

void WxBasisToolGUI::basisSetCategoryNotebookPageChanging( wxNotebookEvent& event )
{
////@begin wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0 in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_BASISSET_CATEGORY_0 in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00
 */

void WxBasisToolGUI::basisSetTypeListboxSelectedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00 in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00 in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00
 */

void WxBasisToolGUI::basisSetTypeListboxDoubleClickedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00 in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX_WXBASISTOOL_BASISSET_TYPE_00 in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_DETAILS
 */

void WxBasisToolGUI::detailsButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_DETAILS in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_DETAILS in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_CONTEXT_ADD
 */

void WxBasisToolGUI::contextAddButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_CONTEXT_ADD in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_CONTEXT_ADD in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS
 */

void WxBasisToolGUI::contextBasisSetsNotebookPageChangedCB( wxNotebookEvent& event )
{
////@begin wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS
 */

void WxBasisToolGUI::contextBasisSetsNotebookPageChangingCB( wxNotebookEvent& event )
{
////@begin wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGING event handler for ID_NOTEBOOK_WXBASISTOOL_CONTEXT_BASISSETS in WxBasisToolGUI. 
}

/*!
 * wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0
 */

void WxBasisToolGUI::contextBasisSetsGridCellSelectCB( wxGridEvent& event )
{
////@begin wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0 in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_CMD_SELECT_CELL event handler for ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0 in WxBasisToolGUI. 
}

/*!
 * wxEVT_GRID_CMD_RANGE_SELECT event handler for ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0
 */

void WxBasisToolGUI::contextBasisSetsGridRangeSelectCB( wxGridRangeSelectEvent& event )
{
////@begin wxEVT_GRID_CMD_RANGE_SELECT event handler for ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0 in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_CMD_RANGE_SELECT event handler for ID_GRID_WXBASISTOOL_CONTEXT_BASISSETS_0 in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_WXBASISTOOL_CONTEXT_ALLOWMULTIPLE
 */

void WxBasisToolGUI::contextMultipleAllowedCheckboxClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_WXBASISTOOL_CONTEXT_ALLOWMULTIPLE in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_WXBASISTOOL_CONTEXT_ALLOWMULTIPLE in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_SHOWEDITOR
 */

void WxBasisToolGUI::showEditorButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_SHOWEDITOR in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISTOOL_SHOWEDITOR in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_1
 */

void WxBasisToolGUI::contextPolarizationRadioButtonSelectedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_1 in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_WXBASISTOOL_POLARIZATION_1 in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_WXBASISTOOL_OPTIMIZE
 */

void WxBasisToolGUI::contextOptimizeCheckboxClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_WXBASISTOOL_OPTIMIZE in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_WXBASISTOOL_OPTIMIZE in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
 */

void WxBasisToolGUI::programSaveMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_PROGRAM_COEFFICIENTS
 */

void WxBasisToolGUI::programCoefficientsMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_PROGRAM_COEFFICIENTS in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_PROGRAM_COEFFICIENTS in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void WxBasisToolGUI::programQuitMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_EDIT_CLEARALL
 */

void WxBasisToolGUI::editClearAllMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_EDIT_CLEARALL in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_EDIT_CLEARALL in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_EDIT_CLEARSELECTION
 */

void WxBasisToolGUI::editClearSelectionMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_EDIT_CLEARSELECTION in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_EDIT_CLEARSELECTION in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
 */

void WxBasisToolGUI::helpProgramMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in WxBasisToolGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_HELP_SUPPORT
 */

void WxBasisToolGUI::helpSupportMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_HELP_SUPPORT in WxBasisToolGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXBASISTOOL_HELP_SUPPORT in WxBasisToolGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxBasisToolGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxBasisToolGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxBasisToolGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxBasisToolGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxBasisToolGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxBasisToolGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxBasisToolGUI icon retrieval
}
