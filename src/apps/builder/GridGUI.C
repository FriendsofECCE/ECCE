/////////////////////////////////////////////////////////////////////////////
// Name:        GridGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "GridGUI.H"
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
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "GridGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID GridGUI::ID_TEXTCTRL_GRID_TOY = wxNewId();
const wxWindowID GridGUI::ID_TEXTCTRL_GRID_ZRES = wxNewId();
const wxWindowID GridGUI::ID_STATICTEXT_GRIDZ = wxNewId();
const wxWindowID GridGUI::ID_TEXTCTRLGRID_FROMX = wxNewId();
const wxWindowID GridGUI::ID_TEXTCTRL_GRID_TOZ = wxNewId();
const wxWindowID GridGUI::ID_CHECKBOX_GRID_UNIFORM = wxNewId();
const wxWindowID GridGUI::ID_TEXTCTRL_GRID_FROMZ = wxNewId();
const wxWindowID GridGUI::ID_DIALOG_GRID = wxNewId();
const wxWindowID GridGUI::ID_STATICTEXT_GRIDX = wxNewId();
const wxWindowID GridGUI::ID_TEXTCTRL_GRID_UNIFORM = wxNewId();
const wxWindowID GridGUI::ID_TEXTCTRL_GRID_TOX = wxNewId();
const wxWindowID GridGUI::ID_TEXTCTRL_GRID_XRES = wxNewId();
const wxWindowID GridGUI::ID_TEXTCTRL_GRID_YRES = wxNewId();
const wxWindowID GridGUI::ID_TEXTCTRL_GRID_FROMY = wxNewId();
const wxWindowID GridGUI::ID_STATICTEXT_GRIDY = wxNewId();

/*!
 * GridGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( GridGUI, ewxDialog )

/*!
 * GridGUI event table definition
 */

BEGIN_EVENT_TABLE( GridGUI, ewxDialog )

////@begin GridGUI event table entries
    EVT_CLOSE( GridGUI::OnCloseWindow )

    EVT_CHECKBOX( ID_CHECKBOX_GRID_UNIFORM, GridGUI::OnCheckboxGridUniformClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_GRID_UNIFORM, GridGUI::OnTextctrlGridUniformEnter )

    EVT_BUTTON( wxID_OK, GridGUI::OnOkClick )

    EVT_BUTTON( wxID_APPLY, GridGUI::OnApplyClick )

////@end GridGUI event table entries

END_EVENT_TABLE()

/*!
 * GridGUI constructors
 */

GridGUI::GridGUI( )
{
}

GridGUI::GridGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * GridGUI creator
 */

bool GridGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin GridGUI member initialisation
////@end GridGUI member initialisation

////@begin GridGUI creation
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
////@end GridGUI creation
    return true;
}

/*!
 * Control creation for GridGUI
 */

void GridGUI::CreateControls()
{    
////@begin GridGUI content construction
    GridGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(3, 4, 0, 0);
    itemBoxSizer3->Add(itemFlexGridSizer4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemFlexGridSizer4->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText6 = new ewxStaticText( itemDialog1, ID_STATICTEXT_GRIDX, _("X"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText6, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemDialog1, ID_STATICTEXT_GRIDY, _("Y"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText7, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText8 = new ewxStaticText( itemDialog1, ID_STATICTEXT_GRIDZ, _("Z"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Res:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText9, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 1);

    ewxTextCtrl* itemTextCtrl10 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_GRID_XRES, _("30"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl10, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxTextCtrl* itemTextCtrl11 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_GRID_YRES, _("30"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl11, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxTextCtrl* itemTextCtrl12 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_GRID_ZRES, _("30"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl12, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxStaticText* itemStaticText13 = new ewxStaticText( itemDialog1, wxID_STATIC, _("From:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText13, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 1);

    ewxTextCtrl* itemTextCtrl14 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRLGRID_FROMX, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl14, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxTextCtrl* itemTextCtrl15 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_GRID_FROMY, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl15, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxTextCtrl* itemTextCtrl16 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_GRID_FROMZ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl16, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxStaticText* itemStaticText17 = new ewxStaticText( itemDialog1, wxID_STATIC, _("To:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText17, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 1);

    ewxTextCtrl* itemTextCtrl18 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_GRID_TOX, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl18, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxTextCtrl* itemTextCtrl19 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_GRID_TOY, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl19, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

    ewxTextCtrl* itemTextCtrl20 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_GRID_TOZ, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl20, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer3->Add(itemBoxSizer21, 0, wxALIGN_TOP|wxALL, 5);

    wxBoxSizer* itemBoxSizer22 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer21->Add(itemBoxSizer22, 0, wxALIGN_BOTTOM|wxALL, 5);

    ewxCheckBox* itemCheckBox23 = new ewxCheckBox( itemDialog1, ID_CHECKBOX_GRID_UNIFORM, _("Uniform "), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox23->SetValue(false);
    itemBoxSizer22->Add(itemCheckBox23, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer21->Add(itemBoxSizer24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText25 = new ewxStaticText( itemDialog1, wxID_STATIC, _("X=Y=Z"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText25, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl26 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_GRID_UNIFORM, _("30"), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    itemBoxSizer24->Add(itemTextCtrl26, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 1);

    wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer27, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine28 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer27->Add(itemStaticLine28, 1, wxGROW|wxALL, 0);

    wxStdDialogButtonSizer* itemStdDialogButtonSizer29 = new wxStdDialogButtonSizer;

    itemBoxSizer2->Add(itemStdDialogButtonSizer29, 0, wxGROW|wxALL, 5);
    ewxButton* itemButton30 = new ewxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStdDialogButtonSizer29->AddButton(itemButton30);

    ewxButton* itemButton31 = new ewxButton( itemDialog1, wxID_APPLY, _("&Apply"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStdDialogButtonSizer29->AddButton(itemButton31);

    itemStdDialogButtonSizer29->Realize();

////@end GridGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_DIALOG_GRID
 */

void GridGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_DIALOG_GRID in GridGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_DIALOG_GRID in GridGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_GRID_UNIFORM
 */

void GridGUI::OnCheckboxGridUniformClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_GRID_UNIFORM in GridGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_GRID_UNIFORM in GridGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_GRID_UNIFORM
 */

void GridGUI::OnTextctrlGridUniformEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_GRID_UNIFORM in GridGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_GRID_UNIFORM in GridGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void GridGUI::OnOkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in GridGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in GridGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_APPLY
 */

void GridGUI::OnApplyClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_APPLY in GridGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_APPLY in GridGUI. 
}

/*!
 * Should we show tooltips?
 */

bool GridGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap GridGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin GridGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end GridGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon GridGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin GridGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end GridGUI icon retrieval
}
