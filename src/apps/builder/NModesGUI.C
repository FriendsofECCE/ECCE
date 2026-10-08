/////////////////////////////////////////////////////////////////////////////
// Name:        NModesGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "NModesGUI.H"
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
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxRadioBox.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include <wx/wrapsizer.h>
#include <cmath>
#include <wx/bmpbndl.h>
#include "NModesGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID NModesGUI::ID_BITMAPBUTTON_START = wxNewId();
const wxWindowID NModesGUI::ID_BITMAPBUTTON_STOP = wxNewId();
const wxWindowID NModesGUI::ID_RADIOBOX_NMODE_VIZTYPE = wxNewId();
const wxWindowID NModesGUI::ID_RADIOBOX_NMODE_DATAVIEW = wxNewId();
const wxWindowID NModesGUI::ID_GRID_NMODE = wxNewId();
const wxWindowID NModesGUI::ID_PANEL_NMODE = wxNewId();
const wxWindowID NModesGUI::ID_CHECKBOX_NMODE_VECSIGN = wxNewId();
const wxWindowID NModesGUI::ID_TEXTCTRL_NMODE_DELAY = wxNewId();
const wxWindowID NModesGUI::ID_BUTTON_NMODE_VECCOLOR = wxNewId();

/*!
 * NModesGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( NModesGUI, VizPropertyPanel )

/*!
 * NModesGUI event table definition
 */

BEGIN_EVENT_TABLE( NModesGUI, VizPropertyPanel )

////@begin NModesGUI event table entries
    EVT_GRID_SELECT_CELL( NModesGUI::OnSelectCell )

    EVT_RADIOBOX( ID_RADIOBOX_NMODE_VIZTYPE, NModesGUI::OnRadioboxNmodeViztypeSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NMODE_DELAY, NModesGUI::OnTextctrlNmodeDelayEnter )

    EVT_BUTTON( ID_BITMAPBUTTON_START, NModesGUI::OnBitmapbuttonStartClick )

    EVT_BUTTON( ID_BITMAPBUTTON_STOP, NModesGUI::OnBitmapbuttonStopClick )

    EVT_BUTTON( ID_BUTTON_NMODE_VECCOLOR, NModesGUI::OnButtonNmodeVeccolorClick )

    EVT_CHECKBOX( ID_CHECKBOX_NMODE_VECSIGN, NModesGUI::OnCheckboxNmodeVecsignClick )

////@end NModesGUI event table entries

END_EVENT_TABLE()

/*!
 * NModesGUI constructors
 */

NModesGUI::NModesGUI( )
{
}

NModesGUI::NModesGUI( IPropCalculation* calculation, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(calculation, parent, id, pos, size, style);
}

/*!
 * NModesGUI creator
 */

bool NModesGUI::Create( IPropCalculation* calculation, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NModesGUI member initialisation
    p_gridPlotSizer = NULL;
    p_ = NULL;
    p_sliderSizer = NULL;
    p_styleSizer = NULL;
    p_animateSizer = NULL;
    p_vectorSizer = NULL;
////@end NModesGUI member initialisation

////@begin NModesGUI creation
    VizPropertyPanel::Create( calculation, parent, id, pos, size, style );

    CreateControls();
    // Deliberately NOT calling GetSizer()->Fit()/SetSizeHints() here (as
    // the wxFormBuilder-generated code originally did): at this point
    // CreateControls() has only built the placeholder controls (e.g. an
    // empty 5x5 grid), not real content -- NModePanel::Create() (the only
    // subclass that ever instantiates this) already does the equivalent
    // Fit()/SetSizeHints() call itself, later, after initialize() has
    // populated real data. Computing a best-size from placeholder content
    // this early was a suspected contributor to a wx3.2/GTK3 AUI-dock
    // layout crash on GeoVib jobs (a near-zero best size reported before
    // the panel is ever painted/populated, while multiple panels compete
    // for the same dock stack) -- see the CLAUDE.md wx3.2/GTK3 reentrancy
    // pitfall note.
////@end NModesGUI creation
    return true;
}

/*!
 * A play triangle or a stop square, drawn in the button's text colour at one
 * and two times 12 px: it needs no icon theme and no image file.
 */
static wxBitmapBundle playGlyph(bool stop)
{
    const wxColour ink = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);
    wxVector<wxBitmap> sizes;
    for (int scale = 1; scale <= 2; ++scale) {
        const int n = 12 * scale;
        wxImage img(n, n);
        img.InitAlpha();
        unsigned char *rgb = img.GetData();
        unsigned char *alpha = img.GetAlpha();
        for (int y = 0; y < n; ++y) {
            for (int x = 0; x < n; ++x) {
                int covered = 0;
                for (int sy = 0; sy < 4; ++sy) {
                    for (int sx = 0; sx < 4; ++sx) {
                        const double px = (x + (sx + 0.5)/4)/n;
                        const double py = (y + (sy + 0.5)/4)/n;
                        const bool in = stop
                            ? (px >= 0.1 && px <= 0.9 && py >= 0.1 && py <= 0.9)
                            : (px >= 0.2 && px <= 0.92 &&
                               fabs(py - 0.5) <= 0.45*(0.92 - px)/0.72);
                        if (in) ++covered;
                    }
                }
                const int i = y*n + x;
                rgb[3*i] = ink.Red();
                rgb[3*i+1] = ink.Green();
                rgb[3*i+2] = ink.Blue();
                alpha[i] = (unsigned char)(covered*255/16);
            }
        }
        sizes.push_back(wxBitmap(img));
    }
    return wxBitmapBundle::FromBitmaps(sizes);
}

/*!
 * Control creation for NModesGUI
 */

void NModesGUI::CreateControls()
{    
////@begin NModesGUI content construction
    NModesGUI* itemVizPropertyPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemVizPropertyPanel1->SetSizer(itemBoxSizer2);

    p_gridPlotSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(p_gridPlotSizer, 1, wxGROW|wxALL, 0);

    ewxGrid* itemGrid4 = new ewxGrid( itemVizPropertyPanel1, ID_GRID_NMODE, wxDefaultPosition, wxSize(200, 225), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    itemGrid4->SetDefaultColSize(50);
    itemGrid4->SetDefaultRowSize(25);
    itemGrid4->SetColLabelSize(40);
    itemGrid4->SetRowLabelSize(50);
    // CreateGrid() itself (below, not here) is deferred to the end of this
    // function, after every other control is constructed -- it internally
    // calls wxGrid::SetTable(), which on wx3.2/GTK3 synchronously fires a
    // wxEVT_GRID_SELECT_CELL event. NModePanel::OnModeSelection (bound to
    // that event) calls showMode(), which does
    // FindWindow(ID_BUTTON_NMODE_VECCOLOR) and dereferences the result
    // unchecked -- that button (itemButton20 below) and the "Use Negative
    // Displacement" checkbox didn't exist yet when CreateGrid() ran here,
    // so FindWindow() returned NULL and showMode() segfaulted on
    // btn->GetBackgroundColour(), reliably crashing builder on opening any
    // job with vibrational (VIB) data. Sizer membership/position is set
    // here as before; only the CreateGrid() call itself moves.
    p_gridPlotSizer->Add(itemGrid4, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer5, 0, wxEXPAND|wxALL, 0);

    //  Wraps: the two radio boxes and the scale do not fit a docked
    //  panel's width side by side.
    p_ = new wxWrapSizer(wxHORIZONTAL);
    itemBoxSizer5->Add(p_, 0, wxEXPAND|wxALL, 0);

    wxString itemRadioBox7Strings[] = {
        _("&Animation"),
        _("&Vector")
    };
    //  Labelled, where this used to pass _T("") -- see the note on the
    //  Graph/Table box below for why.
    ewxRadioBox* itemRadioBox7 = new ewxRadioBox( itemVizPropertyPanel1, ID_RADIOBOX_NMODE_VIZTYPE, _("Viewer"), wxDefaultPosition, wxDefaultSize, 2, itemRadioBox7Strings, 1, wxRA_SPECIFY_ROWS );
    p_->Add(itemRadioBox7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    //  Graph or table for the frequency list.  A SEPARATE control from the
    //  Animation/Vector box above, deliberately: that one chooses how the
    //  mode is drawn in the 3-D viewer, this one chooses how the
    //  frequencies are listed, and the two are independent.  Folding them
    //  into one three-way choice would leave "Table" saying nothing about
    //  what the viewer should do.
    //
    //  BOTH boxes are labelled, which they were not when this one was
    //  added: two unlabelled two-item radio boxes sat side by side with
    //  nothing saying which axis either controlled, and the report on
    //  this control was "found it -- hard to discover".  A caption on
    //  each is what distinguishes them; the suggestion it prompted, one
    //  "Animation / Vector / Table" box, is the folding ruled out above.
    //
    //  It exists at all because the only way to reach this was the panel's
    //  tear-off options menu, which the wx3.2 AUI port left unreachable --
    //  and which, once restored, is on a right-click that the plot's own
    //  context menu intercepts.
    wxString itemRadioBox7bStrings[] = {
        _("&Graph"),
        _("&Table")
    };
    ewxRadioBox* itemRadioBox7b = new ewxRadioBox( itemVizPropertyPanel1, ID_RADIOBOX_NMODE_DATAVIEW, _("Frequencies"), wxDefaultPosition, wxDefaultSize, 2, itemRadioBox7bStrings, 1, wxRA_SPECIFY_ROWS );
    p_->Add(itemRadioBox7b, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_sliderSizer = new wxBoxSizer(wxHORIZONTAL);
    p_->Add(p_sliderSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemVizPropertyPanel1, wxID_STATIC, _("Scale:"), wxDefaultPosition, wxDefaultSize, 0 );
    p_sliderSizer->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_styleSizer = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer5->Add(p_styleSizer, 0, wxEXPAND|wxALL, 0);

    p_animateSizer = new wxWrapSizer(wxHORIZONTAL);
    p_styleSizer->Add(p_animateSizer, 0, wxEXPAND|wxALL, 0);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemVizPropertyPanel1, wxID_STATIC, _("Delay: "), wxDefaultPosition, wxDefaultSize, 0 );
    p_animateSizer->Add(itemStaticText12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxTextCtrl* itemTextCtrl13 = new ewxTextCtrl( itemVizPropertyPanel1, ID_TEXTCTRL_NMODE_DELAY, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    //  Room for the largest delay, 5000, from the font in use.
    itemTextCtrl13->SetMinSize(wxSize(
        itemTextCtrl13->GetSizeFromTextSize(
            itemTextCtrl13->GetTextExtent(_T("50000")).x).x, -1));
    p_animateSizer->Add(itemTextCtrl13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemVizPropertyPanel1, wxID_STATIC, _("50-5000 ms "), wxDefaultPosition, wxDefaultSize, 0 );
    p_animateSizer->Add(itemStaticText14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    //  Start and Stop carry a drawn glyph and a label: a theme without the
    //  media icons (or one that draws them in an unreadable colour) leaves
    //  a blank button, and a button must never be blank.
    ewxButton* itemBitmapButton16 = new ewxButton( itemVizPropertyPanel1, ID_BITMAPBUTTON_START, _("Start"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBitmapButton16->SetBitmap(playGlyph(false));
    if (ShowToolTips())
        itemBitmapButton16->SetToolTip(_("Start the animation of this normal mode"));
    p_animateSizer->Add(itemBitmapButton16, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 6);

    ewxButton* itemBitmapButton18 = new ewxButton( itemVizPropertyPanel1, ID_BITMAPBUTTON_STOP, _("Stop"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBitmapButton18->SetBitmap(playGlyph(true));
    if (ShowToolTips())
        itemBitmapButton18->SetToolTip(_("Stop the animation"));
    p_animateSizer->Add(itemBitmapButton18, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 3);

    p_vectorSizer = new wxWrapSizer(wxHORIZONTAL);
    p_styleSizer->Add(p_vectorSizer, 0, wxEXPAND|wxALL, 0);

    ewxButton* itemButton20 = new ewxButton( itemVizPropertyPanel1, ID_BUTTON_NMODE_VECCOLOR, _T(""), wxDefaultPosition, wxSize(24, 24), wxBU_EXACTFIT );
    p_vectorSizer->Add(itemButton20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox21 = new ewxCheckBox( itemVizPropertyPanel1, ID_CHECKBOX_NMODE_VECSIGN, _("Use Negative Displacement"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox21->SetValue(false);
    p_vectorSizer->Add(itemCheckBox21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

////@end NModesGUI content construction

    // See the comment at itemGrid4's construction above -- this must run
    // after every sibling control above it exists.
    itemGrid4->CreateGrid(5, 5, wxGrid::wxGridSelectRows);
}

/*!
 * wxEVT_GRID_SELECT_CELL event handler for ID_GRID_NMODE
 */

void NModesGUI::OnSelectCell( wxGridEvent& event )
{
////@begin wxEVT_GRID_SELECT_CELL event handler for ID_GRID_NMODE in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_SELECT_CELL event handler for ID_GRID_NMODE in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX_NMODE_VIZTYPE
 */

void NModesGUI::OnRadioboxNmodeViztypeSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX_NMODE_VIZTYPE in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX_NMODE_VIZTYPE in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NMODE_DELAY
 */

void NModesGUI::OnTextctrlNmodeDelayEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NMODE_DELAY in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NMODE_DELAY in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_START
 */

void NModesGUI::OnBitmapbuttonStartClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_START in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_START in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_STOP
 */

void NModesGUI::OnBitmapbuttonStopClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_STOP in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_STOP in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NMODE_VECCOLOR
 */

void NModesGUI::OnButtonNmodeVeccolorClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NMODE_VECCOLOR in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NMODE_VECCOLOR in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NMODE_VECSIGN
 */

void NModesGUI::OnCheckboxNmodeVecsignClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NMODE_VECSIGN in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NMODE_VECSIGN in NModesGUI. 
}

/*!
 * Should we show tooltips?
 */

bool NModesGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap NModesGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NModesGUI bitmap retrieval
    wxUnusedVar(name);
    if (name == wxT("player_play.png"))
    {
        ewxBitmap bitmap(_T("player_play.png"), wxBITMAP_TYPE_PNG);
        return bitmap;
    }
    else if (name == wxT("player_stop.png"))
    {
        ewxBitmap bitmap(_T("player_stop.png"), wxBITMAP_TYPE_PNG);
        return bitmap;
    }
    return wxNullBitmap;
////@end NModesGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NModesGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NModesGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end NModesGUI icon retrieval
}
