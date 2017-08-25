/////////////////////////////////////////////////////////////////////////////
// Name:        TableGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "TableGUI.H"
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
#include "wxgui/ewxGrid.H"
////@end includes

#include "TableGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID TableGUI::ID_GRID_TABLE = wxNewId();
const wxWindowID TableGUI::ID_PANEL_TABLE = wxNewId();

/*!
 * TableGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( TableGUI, PropertyPanel )

/*!
 * TableGUI event table definition
 */

BEGIN_EVENT_TABLE( TableGUI, PropertyPanel )

////@begin TableGUI event table entries
////@end TableGUI event table entries

END_EVENT_TABLE()

/*!
 * TableGUI constructors
 */

TableGUI::TableGUI( )
{
}

TableGUI::TableGUI( IPropCalculation* calculation, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(calculation, parent, id, pos, size, style);
}

/*!
 * TableGUI creator
 */

bool TableGUI::Create( IPropCalculation* calculation, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin TableGUI member initialisation
////@end TableGUI member initialisation

////@begin TableGUI creation
    PropertyPanel::Create( calculation, parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
////@end TableGUI creation
    return true;
}

/*!
 * Control creation for TableGUI
 */

void TableGUI::CreateControls()
{    
////@begin TableGUI content construction
    TableGUI* itemPropertyPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPropertyPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 1, wxGROW|wxALL, 0);

    ewxGrid* itemGrid4 = new ewxGrid( itemPropertyPanel1, ID_GRID_TABLE, wxDefaultPosition, wxSize(200, 150), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    itemGrid4->SetDefaultColSize(50);
    itemGrid4->SetDefaultRowSize(25);
    itemGrid4->SetColLabelSize(25);
    itemGrid4->SetRowLabelSize(50);
    itemGrid4->CreateGrid(5, 5, wxGrid::wxGridSelectRows);
    itemBoxSizer3->Add(itemGrid4, 1, wxGROW|wxALL, 5);

////@end TableGUI content construction
}

/*!
 * Should we show tooltips?
 */

bool TableGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap TableGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin TableGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end TableGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon TableGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin TableGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end TableGUI icon retrieval
}
