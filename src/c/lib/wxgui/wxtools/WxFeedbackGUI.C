/////////////////////////////////////////////////////////////////////////////
// Name:        WxFeedbackGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxFeedbackGUI.H"
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
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/WxFeedbackGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxFeedbackGUI::ID_PANEL_STATE = wxNewId();
const wxWindowID WxFeedbackGUI::ID_PANEL_FEEDBACK = wxNewId();
const wxWindowID WxFeedbackGUI::ID_TEXTCTRL_FEEDBACK_MSG = wxNewId();
const wxWindowID WxFeedbackGUI::ID_BITMAPBUTTON_DROPICON = wxNewId();
const wxWindowID WxFeedbackGUI::ID_RESETSTATE = wxNewId();
const wxWindowID WxFeedbackGUI::ID_SAVE = wxNewId();
const wxWindowID WxFeedbackGUI::wxID_STATIC_FEEDBACK_CONTEXT = wxNewId();

/*!
 * WxFeedbackGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxFeedbackGUI, ewxPanel )

/*!
 * WxFeedbackGUI event table definition
 */

BEGIN_EVENT_TABLE( WxFeedbackGUI, ewxPanel )

////@begin WxFeedbackGUI event table entries
    EVT_CONTEXT_MENU( WxFeedbackGUI::OnContextMenu )

    EVT_BUTTON(ID_RESETSTATE, WxFeedbackGUI::onResetstateClicked)

    EVT_BUTTON(ID_SAVE, WxFeedbackGUI::onSaveClicked)

////@end WxFeedbackGUI event table entries

END_EVENT_TABLE()

/*!
 * WxFeedbackGUI constructors
 */

WxFeedbackGUI::WxFeedbackGUI( )
{
}

WxFeedbackGUI::WxFeedbackGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * WxFeedbackGUI creator
 */

bool WxFeedbackGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxFeedbackGUI member initialisation
    p_topLineSizer = NULL;
    p_statePanel = NULL;
////@end WxFeedbackGUI member initialisation

////@begin WxFeedbackGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxFeedbackGUI creation
    return true;
}

/*!
 * Control creation for WxFeedbackGUI
 */

void WxFeedbackGUI::CreateControls()
{    
////@begin WxFeedbackGUI content construction
    WxFeedbackGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    p_topLineSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(p_topLineSizer, 0, wxGROW|wxLEFT, 2);

    p_statePanel = new ewxPanel( itemPanel1, ID_PANEL_STATE, wxDefaultPosition, wxSize(16, 16), wxNO_BORDER );
    p_topLineSizer->Add(p_statePanel, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxFIXED_MINSIZE, 2);

    p_topLineSizer->Add(2, 0, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxFIXED_MINSIZE, 0);

    FlatBitmapButton* itemFlatBitmapButton6 = new FlatBitmapButton( itemPanel1, ID_RESETSTATE, wxDefaultPosition, wxSize(16, 16), wxNO_BORDER );
    if (ShowToolTips())
        itemFlatBitmapButton6->SetToolTip(_("Reset state to allow changes and rerun"));
    itemFlatBitmapButton6->Show(false);
    p_topLineSizer->Add(itemFlatBitmapButton6, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5);

    p_topLineSizer->Add(1, 1, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxFIXED_MINSIZE, 0);

    ewxStaticText* itemStaticText8 = new ewxStaticText( itemPanel1, wxID_STATIC_FEEDBACK_CONTEXT, _("Tool Context URL"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    p_topLineSizer->Add(itemStaticText8, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    p_topLineSizer->Add(1, 1, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxFIXED_MINSIZE, 0);

    FlatBitmapButton* itemFlatBitmapButton10 = new FlatBitmapButton( itemPanel1, ID_SAVE, wxDefaultPosition, wxSize(16, 16), wxNO_BORDER );
    p_topLineSizer->Add(itemFlatBitmapButton10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer11 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer11, 1, wxGROW|wxALL, 0);

    ewxTextCtrl* itemTextCtrl12 = new ewxTextCtrl( itemPanel1, ID_TEXTCTRL_FEEDBACK_MSG, _T(""), wxDefaultPosition, wxSize(-1, 60), wxTE_MULTILINE|wxTE_READONLY|wxTE_LEFT|wxTE_WORDWRAP );
    itemBoxSizer11->Add(itemTextCtrl12, 1, wxGROW|wxALL, 0);

    wxBitmap itemBitmapButton13Bitmap(itemPanel1->GetBitmapResource(wxT("drop_w.xpm")));
    ewxBitmapButton* itemBitmapButton13 = new ewxBitmapButton( itemPanel1, ID_BITMAPBUTTON_DROPICON, itemBitmapButton13Bitmap, wxDefaultPosition, wxSize(50, 60), wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer11->Add(itemBitmapButton13, 0, wxALIGN_BOTTOM|wxLEFT, 0);

////@end WxFeedbackGUI content construction
}

/*!
 * wxEVT_CONTEXT_MENU event handler for ID_PANEL_FEEDBACK
 */

void WxFeedbackGUI::OnContextMenu( wxContextMenuEvent& event )
{
////@begin wxEVT_CONTEXT_MENU event handler for ID_PANEL_FEEDBACK in WxFeedbackGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CONTEXT_MENU event handler for ID_PANEL_FEEDBACK in WxFeedbackGUI. 
}

/*!
 * ButtonClick event handler for ID_RESETSTATE
 */

void WxFeedbackGUI::onResetstateClicked( wxCommandEvent& event )
{
////@begin ButtonClick event handler for ID_RESETSTATE in WxFeedbackGUI.
////@end ButtonClick event handler for ID_RESETSTATE in WxFeedbackGUI. 
}

/*!
 * ButtonClick event handler for ID_SAVE
 */

void WxFeedbackGUI::onSaveClicked( wxCommandEvent& event )
{
////@begin ButtonClick event handler for ID_SAVE in WxFeedbackGUI.
////@end ButtonClick event handler for ID_SAVE in WxFeedbackGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxFeedbackGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxFeedbackGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxFeedbackGUI bitmap retrieval
    wxUnusedVar(name);
    if (name == wxT("drop_w.xpm"))
    {
        ewxBitmap bitmap(_T("drop_w.xpm"), wxBITMAP_TYPE_XPM);
        return bitmap;
    }
    return wxNullBitmap;
////@end WxFeedbackGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxFeedbackGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxFeedbackGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxFeedbackGUI icon retrieval
}
