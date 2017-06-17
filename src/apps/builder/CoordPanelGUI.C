/////////////////////////////////////////////////////////////////////////////
// Name:        CoordPanelGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "CoordPanelGUI.H"
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
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "CoordPanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM1_Z = wxNewId();
const wxWindowID CoordPanelGUI::ID_RADIO_CENTER_Z = wxNewId();
const wxWindowID CoordPanelGUI::ID_RADIO_CENTER_X = wxNewId();
const wxWindowID CoordPanelGUI::ID_BUTTON_ALIGN_Z_XY = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROTATE_END_X = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_FROM_X = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_APPLY_TO = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM2_COUNT = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_FROM_PICK = wxNewId();
const wxWindowID CoordPanelGUI::ID_BUTTON_ALIGN_Y_ZX = wxNewId();
const wxWindowID CoordPanelGUI::ID_CENTER_PANEL = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_BY_Y = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROTATE_START_Z = wxNewId();
const wxWindowID CoordPanelGUI::ID_ALIGN_PANEL = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM2_Y = wxNewId();
const wxWindowID CoordPanelGUI::ID_ALIGN_TO_AXIS = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROTATE = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANSLATE_PANEL = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM2_X = wxNewId();
const wxWindowID CoordPanelGUI::ID_RANDOM_ORIENT = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_TO_Z = wxNewId();
const wxWindowID CoordPanelGUI::ID_ALIGN_NZ = wxNewId();
const wxWindowID CoordPanelGUI::ID_STATIC_PLAX = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROTATE_END_Y = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_TO_PICK = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_EXPAND = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROTATE_DEGREE = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROT_APPLY_TO = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM3_Y = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_TO_X = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_PANEL = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM3_Z = wxNewId();
const wxWindowID CoordPanelGUI::ID_COORDINATE_PANEL = wxNewId();
const wxWindowID CoordPanelGUI::ID_ALIGN = wxNewId();
const wxWindowID CoordPanelGUI::ID_ALIGN_NX = wxNewId();
const wxWindowID CoordPanelGUI::ID_ALIGN_APPLY_TO = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROTATE_PANEL = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM1_Y = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM1_COUNT = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_FROM_Z = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANSLATE = wxNewId();
const wxWindowID CoordPanelGUI::ID_NOTEBOOK = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROTATE_START_X = wxNewId();
const wxWindowID CoordPanelGUI::ID_RADIO_CENTER_Y = wxNewId();
const wxWindowID CoordPanelGUI::ID_ALIGN_NY = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROTATE_START_Y = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM2_Z = wxNewId();
const wxWindowID CoordPanelGUI::ID_CENTER = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_FROM_Y = wxNewId();
const wxWindowID CoordPanelGUI::ID_ALIGN_TO_PLANE = wxNewId();
const wxWindowID CoordPanelGUI::ID_RADIO_CENTER_ORIGIN = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM3_COUNT = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROTATE_END_PICK = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROTATE_START_PICK = wxNewId();
const wxWindowID CoordPanelGUI::ID_CENTER_APPLY_TO = wxNewId();
const wxWindowID CoordPanelGUI::ID_ROTATE_END_Z = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_BY_Z = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_BY_X = wxNewId();
const wxWindowID CoordPanelGUI::ID_BUTTON_ALIGN_X_YZ = wxNewId();
const wxWindowID CoordPanelGUI::ID_TRANS_TO_Y = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM3_X = wxNewId();
const wxWindowID CoordPanelGUI::ID_DUPLICATE_DIM1_X = wxNewId();

/*!
 * CoordPanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( CoordPanelGUI, ewxPanel )

/*!
 * CoordPanelGUI event table definition
 */

BEGIN_EVENT_TABLE( CoordPanelGUI, ewxPanel )

////@begin CoordPanelGUI event table entries
    EVT_RADIOBUTTON( ID_RADIO_CENTER_ORIGIN, CoordPanelGUI::OnCenterSelected )

    EVT_RADIOBUTTON( ID_RADIO_CENTER_X, CoordPanelGUI::OnCenterSelected )

    EVT_RADIOBUTTON( ID_RADIO_CENTER_Y, CoordPanelGUI::OnCenterSelected )

    EVT_RADIOBUTTON( ID_RADIO_CENTER_Z, CoordPanelGUI::OnCenterSelected )

    EVT_CHOICE( ID_CENTER_APPLY_TO, CoordPanelGUI::OnCenterApplyToSelected )

    EVT_BUTTON( ID_CENTER, CoordPanelGUI::OnCenterClick )

    EVT_TEXT_ENTER( ID_TRANS_BY_X, CoordPanelGUI::OnTransByXEnter )

    EVT_TEXT_ENTER( ID_TRANS_BY_Y, CoordPanelGUI::OnTransByYEnter )

    EVT_TEXT_ENTER( ID_TRANS_BY_Z, CoordPanelGUI::OnTransByZEnter )

    EVT_CHECKBOX( ID_TRANS_EXPAND, CoordPanelGUI::OnTransExpandClick )

    EVT_TEXT_ENTER( ID_TRANS_FROM_X, CoordPanelGUI::OnTransFromXEnter )

    EVT_TEXT_ENTER( ID_TRANS_FROM_Y, CoordPanelGUI::OnTransFromYEnter )

    EVT_TEXT_ENTER( ID_TRANS_FROM_Z, CoordPanelGUI::OnTransFromZEnter )

    EVT_BUTTON( ID_TRANS_FROM_PICK, CoordPanelGUI::OnTransFromPickClick )

    EVT_TEXT_ENTER( ID_TRANS_TO_X, CoordPanelGUI::OnTransToXEnter )

    EVT_TEXT_ENTER( ID_TRANS_TO_Y, CoordPanelGUI::OnTransToYEnter )

    EVT_TEXT_ENTER( ID_TRANS_TO_Z, CoordPanelGUI::OnTransToZEnter )

    EVT_BUTTON( ID_TRANS_TO_PICK, CoordPanelGUI::OnTransToPickClick )

    EVT_BUTTON( ID_TRANSLATE, CoordPanelGUI::OnTranslateClick )

    EVT_TEXT_ENTER( ID_DUPLICATE_DIM1_X, CoordPanelGUI::OnDuplicateDim1XEnter )

    EVT_TEXT_ENTER( ID_DUPLICATE_DIM1_Y, CoordPanelGUI::OnDuplicateDim1YEnter )

    EVT_TEXT_ENTER( ID_DUPLICATE_DIM1_Z, CoordPanelGUI::OnDuplicateDim1ZEnter )

    EVT_TEXT_ENTER( ID_DUPLICATE_DIM2_X, CoordPanelGUI::OnDuplicateDim2XEnter )

    EVT_TEXT_ENTER( ID_DUPLICATE_DIM2_Y, CoordPanelGUI::OnDuplicateDim2YEnter )

    EVT_TEXT_ENTER( ID_DUPLICATE_DIM2_Z, CoordPanelGUI::OnDuplicateDim2ZEnter )

    EVT_TEXT_ENTER( ID_DUPLICATE_DIM3_X, CoordPanelGUI::OnDuplicateDim3XEnter )

    EVT_TEXT_ENTER( ID_DUPLICATE_DIM3_Y, CoordPanelGUI::OnDuplicateDim3YEnter )

    EVT_TEXT_ENTER( ID_DUPLICATE_DIM3_Z, CoordPanelGUI::OnDuplicateDim3ZEnter )

    EVT_BUTTON( ID_DUPLICATE, CoordPanelGUI::OnDuplicateClick )

    EVT_TEXT( ID_ROTATE_START_X, CoordPanelGUI::OnRotateStartUpdated )

    EVT_TEXT( ID_ROTATE_START_Y, CoordPanelGUI::OnRotateStartUpdated )

    EVT_TEXT( ID_ROTATE_START_Z, CoordPanelGUI::OnRotateStartUpdated )

    EVT_BUTTON( ID_ROTATE_START_PICK, CoordPanelGUI::OnRotateStartPickClick )

    EVT_TEXT( ID_ROTATE_END_X, CoordPanelGUI::OnRotateEndUpdated )

    EVT_TEXT( ID_ROTATE_END_Y, CoordPanelGUI::OnRotateEndUpdated )

    EVT_TEXT( ID_ROTATE_END_Z, CoordPanelGUI::OnRotateEndUpdated )

    EVT_BUTTON( ID_ROTATE_END_PICK, CoordPanelGUI::OnRotateEndPickClick )

    EVT_BUTTON( ID_ROTATE, CoordPanelGUI::OnRotateClick )

    EVT_RADIOBUTTON( ID_ALIGN_TO_AXIS, CoordPanelGUI::OnAlignToAxisSelected )

    EVT_RADIOBUTTON( ID_ALIGN_TO_PLANE, CoordPanelGUI::OnAlignToPlaneSelected )

    EVT_BUTTON( ID_BUTTON_ALIGN_X_YZ, CoordPanelGUI::OnButtonAlignX_YZClick )

    EVT_BUTTON( ID_BUTTON_ALIGN_Y_ZX, CoordPanelGUI::OnButtonAlignY_ZXClick )

    EVT_BUTTON( ID_BUTTON_ALIGN_Z_XY, CoordPanelGUI::OnButtonAlignZ_XYClick )

    EVT_BUTTON( ID_ALIGN, CoordPanelGUI::OnAlignClick )

////@end CoordPanelGUI event table entries

END_EVENT_TABLE()

/*!
 * CoordPanelGUI constructors
 */

CoordPanelGUI::CoordPanelGUI( )
{
}

CoordPanelGUI::CoordPanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * CoordPanelGUI creator
 */

bool CoordPanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin CoordPanelGUI member initialisation
    p_translateSizer = NULL;
////@end CoordPanelGUI member initialisation

////@begin CoordPanelGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end CoordPanelGUI creation
    return true;
}

/*!
 * Control creation for CoordPanelGUI
 */

void CoordPanelGUI::CreateControls()
{    
////@begin CoordPanelGUI content construction
    CoordPanelGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    ewxNotebook* itemNotebook3 = new ewxNotebook( itemPanel1, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    ewxPanel* itemPanel4 = new ewxPanel( itemNotebook3, ID_CENTER_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemPanel4->SetSizer(itemBoxSizer5);

    wxFlexGridSizer* itemFlexGridSizer6 = new wxFlexGridSizer(3, 4, 0, 0);
    itemFlexGridSizer6->AddGrowableCol(0);
    itemFlexGridSizer6->AddGrowableCol(1);
    itemFlexGridSizer6->AddGrowableCol(2);
    itemFlexGridSizer6->AddGrowableCol(3);
    itemBoxSizer5->Add(itemFlexGridSizer6, 0, wxGROW|wxALL, 5);
    itemFlexGridSizer6->Add(0, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxRadioButton* itemRadioButton8 = new ewxRadioButton( itemPanel4, ID_RADIO_CENTER_ORIGIN, _("Origin"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton8->SetValue(true);
    if (ShowToolTips())
        itemRadioButton8->SetToolTip(_("Move center of mass to origin"));
    itemFlexGridSizer6->Add(itemRadioButton8, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5);

    ewxRadioButton* itemRadioButton9 = new ewxRadioButton( itemPanel4, ID_RADIO_CENTER_X, _("X Axis"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton9->SetValue(false);
    if (ShowToolTips())
        itemRadioButton9->SetToolTip(_("Move x component of center of mass to zero"));
    itemFlexGridSizer6->Add(itemRadioButton9, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5);

    itemFlexGridSizer6->Add(0, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemFlexGridSizer6->Add(0, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemFlexGridSizer6->Add(0, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxRadioButton* itemRadioButton13 = new ewxRadioButton( itemPanel4, ID_RADIO_CENTER_Y, _("Y Axis"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton13->SetValue(false);
    if (ShowToolTips())
        itemRadioButton13->SetToolTip(_("Move y component of center of mass to zero"));
    itemFlexGridSizer6->Add(itemRadioButton13, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5);

    itemFlexGridSizer6->Add(0, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemFlexGridSizer6->Add(0, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemFlexGridSizer6->Add(0, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxRadioButton* itemRadioButton17 = new ewxRadioButton( itemPanel4, ID_RADIO_CENTER_Z, _("Z Axis"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton17->SetValue(false);
    if (ShowToolTips())
        itemRadioButton17->SetToolTip(_("Move z component of center of mass to zero"));
    itemFlexGridSizer6->Add(itemRadioButton17, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP, 5);

    itemBoxSizer5->Add(0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 0);

    ewxStaticLine* itemStaticLine19 = new ewxStaticLine( itemPanel4, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer5->Add(itemStaticLine19, 0, wxGROW|wxLEFT|wxRIGHT, 3);

    wxBoxSizer* itemBoxSizer20 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer5->Add(itemBoxSizer20, 0, wxGROW, 0);
    ewxStaticText* itemStaticText21 = new ewxStaticText( itemPanel4, wxID_STATIC, _("Apply to:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(itemStaticText21, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice22Strings[] = {
        _("All"),
        _("Selection")
    };
    ewxChoice* itemChoice22 = new ewxChoice( itemPanel4, ID_CENTER_APPLY_TO, wxDefaultPosition, wxDefaultSize, 2, itemChoice22Strings, 0 );
    itemChoice22->SetStringSelection(_("All"));
    itemBoxSizer20->Add(itemChoice22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer20->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton24 = new ewxButton( itemPanel4, ID_CENTER, _("Center"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemButton24->SetDefault();
    itemBoxSizer20->Add(itemButton24, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook3->AddPage(itemPanel4, _("Center"));

    ewxPanel* itemPanel25 = new ewxPanel( itemNotebook3, ID_TRANSLATE_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxVERTICAL);
    itemPanel25->SetSizer(itemBoxSizer26);

    p_translateSizer = new wxFlexGridSizer(4, 5, 0, 0);
    p_translateSizer->AddGrowableCol(1);
    p_translateSizer->AddGrowableCol(2);
    p_translateSizer->AddGrowableCol(3);
    itemBoxSizer26->Add(p_translateSizer, 0, wxGROW, 0);
    p_translateSizer->Add(1, 1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText29 = new ewxStaticText( itemPanel25, wxID_STATIC, _("X"), wxDefaultPosition, wxDefaultSize, 0 );
    p_translateSizer->Add(itemStaticText29, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText30 = new ewxStaticText( itemPanel25, wxID_STATIC, _("Y"), wxDefaultPosition, wxDefaultSize, 0 );
    p_translateSizer->Add(itemStaticText30, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText31 = new ewxStaticText( itemPanel25, wxID_STATIC, _("Z"), wxDefaultPosition, wxDefaultSize, 0 );
    p_translateSizer->Add(itemStaticText31, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    p_translateSizer->Add(1, 1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText33 = new ewxStaticText( itemPanel25, wxID_STATIC, _("By"), wxDefaultPosition, wxDefaultSize, 0 );
    p_translateSizer->Add(itemStaticText33, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl34 = new ewxTextCtrl( itemPanel25, ID_TRANS_BY_X, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    p_translateSizer->Add(itemTextCtrl34, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl35 = new ewxTextCtrl( itemPanel25, ID_TRANS_BY_Y, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    p_translateSizer->Add(itemTextCtrl35, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl36 = new ewxTextCtrl( itemPanel25, ID_TRANS_BY_Z, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    p_translateSizer->Add(itemTextCtrl36, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxCheckBox* itemCheckBox37 = new ewxCheckBox( itemPanel25, ID_TRANS_EXPAND, _("Use Sel."), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox37->SetValue(true);
    if (ShowToolTips())
        itemCheckBox37->SetToolTip(_("Use builder selection to construct translation vector"));
    p_translateSizer->Add(itemCheckBox37, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_BOTTOM, 3);

    ewxStaticText* itemStaticText38 = new ewxStaticText( itemPanel25, wxID_STATIC, _("From"), wxDefaultPosition, wxDefaultSize, 0 );
    p_translateSizer->Add(itemStaticText38, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl39 = new ewxTextCtrl( itemPanel25, ID_TRANS_FROM_X, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    p_translateSizer->Add(itemTextCtrl39, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl40 = new ewxTextCtrl( itemPanel25, ID_TRANS_FROM_Y, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    p_translateSizer->Add(itemTextCtrl40, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl41 = new ewxTextCtrl( itemPanel25, ID_TRANS_FROM_Z, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    p_translateSizer->Add(itemTextCtrl41, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxButton* itemButton42 = new ewxButton( itemPanel25, ID_TRANS_FROM_PICK, _("Get Sel."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton42->SetToolTip(_("Get coordinates from builder"));
    p_translateSizer->Add(itemButton42, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 3);

    ewxStaticText* itemStaticText43 = new ewxStaticText( itemPanel25, wxID_STATIC, _("To"), wxDefaultPosition, wxDefaultSize, 0 );
    p_translateSizer->Add(itemStaticText43, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl44 = new ewxTextCtrl( itemPanel25, ID_TRANS_TO_X, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    p_translateSizer->Add(itemTextCtrl44, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    ewxTextCtrl* itemTextCtrl45 = new ewxTextCtrl( itemPanel25, ID_TRANS_TO_Y, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    p_translateSizer->Add(itemTextCtrl45, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    ewxTextCtrl* itemTextCtrl46 = new ewxTextCtrl( itemPanel25, ID_TRANS_TO_Z, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    p_translateSizer->Add(itemTextCtrl46, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    ewxButton* itemButton47 = new ewxButton( itemPanel25, ID_TRANS_TO_PICK, _("Get Sel."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton47->SetToolTip(_("Get coordinates from builder"));
    p_translateSizer->Add(itemButton47, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3);

    itemBoxSizer26->Add(0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 0);

    ewxStaticLine* itemStaticLine49 = new ewxStaticLine( itemPanel25, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer26->Add(itemStaticLine49, 0, wxGROW|wxLEFT|wxRIGHT, 3);

    wxBoxSizer* itemBoxSizer50 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer26->Add(itemBoxSizer50, 0, wxGROW, 0);
    ewxStaticText* itemStaticText51 = new ewxStaticText( itemPanel25, wxID_STATIC, _("Apply to:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer50->Add(itemStaticText51, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice52Strings[] = {
        _("All"),
        _("Selection")
    };
    ewxChoice* itemChoice52 = new ewxChoice( itemPanel25, ID_TRANS_APPLY_TO, wxDefaultPosition, wxDefaultSize, 2, itemChoice52Strings, 0 );
    itemChoice52->SetStringSelection(_("All"));
    itemBoxSizer50->Add(itemChoice52, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer50->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton54 = new ewxButton( itemPanel25, ID_TRANSLATE, _("Translate"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemButton54->SetDefault();
    itemBoxSizer50->Add(itemButton54, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook3->AddPage(itemPanel25, _("Translate"));

    ewxPanel* itemPanel55 = new ewxPanel( itemNotebook3, ID_DUPLICATE_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer56 = new wxBoxSizer(wxVERTICAL);
    itemPanel55->SetSizer(itemBoxSizer56);

    wxFlexGridSizer* itemFlexGridSizer57 = new wxFlexGridSizer(4, 5, 0, 0);
    itemFlexGridSizer57->AddGrowableCol(1);
    itemFlexGridSizer57->AddGrowableCol(2);
    itemFlexGridSizer57->AddGrowableCol(3);
    itemBoxSizer56->Add(itemFlexGridSizer57, 0, wxGROW, 0);
    itemFlexGridSizer57->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText59 = new ewxStaticText( itemPanel55, wxID_STATIC, _("X"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText59, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText60 = new ewxStaticText( itemPanel55, wxID_STATIC, _("Y"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText60, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText61 = new ewxStaticText( itemPanel55, wxID_STATIC, _("Z"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText61, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText62 = new ewxStaticText( itemPanel55, wxID_STATIC, _("Repeat"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText62, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText63 = new ewxStaticText( itemPanel55, wxID_STATIC, _("Dim.1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText63, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl64 = new ewxTextCtrl( itemPanel55, ID_DUPLICATE_DIM1_X, _("1.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer57->Add(itemTextCtrl64, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl65 = new ewxTextCtrl( itemPanel55, ID_DUPLICATE_DIM1_Y, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer57->Add(itemTextCtrl65, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl66 = new ewxTextCtrl( itemPanel55, ID_DUPLICATE_DIM1_Z, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer57->Add(itemTextCtrl66, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxSpinCtrl* itemSpinCtrl67 = new ewxSpinCtrl( itemPanel55, ID_DUPLICATE_DIM1_COUNT, _("1"), wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 1, 1000, 1 );
    if (ShowToolTips())
        itemSpinCtrl67->SetToolTip(_("Number of repeat units for first lattice vector"));
    itemFlexGridSizer57->Add(itemSpinCtrl67, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxStaticText* itemStaticText68 = new ewxStaticText( itemPanel55, wxID_STATIC, _("Dim.2"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText68, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl69 = new ewxTextCtrl( itemPanel55, ID_DUPLICATE_DIM2_X, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer57->Add(itemTextCtrl69, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl70 = new ewxTextCtrl( itemPanel55, ID_DUPLICATE_DIM2_Y, _("1.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer57->Add(itemTextCtrl70, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl71 = new ewxTextCtrl( itemPanel55, ID_DUPLICATE_DIM2_Z, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer57->Add(itemTextCtrl71, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxSpinCtrl* itemSpinCtrl72 = new ewxSpinCtrl( itemPanel55, ID_DUPLICATE_DIM2_COUNT, _("1"), wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 1, 1000, 1 );
    if (ShowToolTips())
        itemSpinCtrl72->SetToolTip(_("Number of repeat units for second lattice vector"));
    itemFlexGridSizer57->Add(itemSpinCtrl72, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxStaticText* itemStaticText73 = new ewxStaticText( itemPanel55, wxID_STATIC, _("Dim.3"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer57->Add(itemStaticText73, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl74 = new ewxTextCtrl( itemPanel55, ID_DUPLICATE_DIM3_X, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer57->Add(itemTextCtrl74, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    ewxTextCtrl* itemTextCtrl75 = new ewxTextCtrl( itemPanel55, ID_DUPLICATE_DIM3_Y, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer57->Add(itemTextCtrl75, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    ewxTextCtrl* itemTextCtrl76 = new ewxTextCtrl( itemPanel55, ID_DUPLICATE_DIM3_Z, _("1.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer57->Add(itemTextCtrl76, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    ewxSpinCtrl* itemSpinCtrl77 = new ewxSpinCtrl( itemPanel55, ID_DUPLICATE_DIM3_COUNT, _("1"), wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 1, 1000, 1 );
    if (ShowToolTips())
        itemSpinCtrl77->SetToolTip(_("Number of repeat units for third lattice vector"));
    itemFlexGridSizer57->Add(itemSpinCtrl77, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    itemBoxSizer56->Add(0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 0);

    ewxStaticLine* itemStaticLine79 = new ewxStaticLine( itemPanel55, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer56->Add(itemStaticLine79, 0, wxGROW|wxLEFT|wxRIGHT, 3);

    wxBoxSizer* itemBoxSizer80 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer56->Add(itemBoxSizer80, 0, wxGROW, 0);
    ewxCheckBox* itemCheckBox81 = new ewxCheckBox( itemPanel55, ID_RANDOM_ORIENT, _("Randomize Orientation"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox81->SetValue(false);
    if (ShowToolTips())
        itemCheckBox81->SetToolTip(_("Randomize orientation of individual lattice elements"));
    itemBoxSizer80->Add(itemCheckBox81, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5);

    itemBoxSizer80->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton83 = new ewxButton( itemPanel55, ID_DUPLICATE, _("Duplicate"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemButton83->SetDefault();
    itemBoxSizer80->Add(itemButton83, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook3->AddPage(itemPanel55, _("Lattice"));

    ewxPanel* itemPanel84 = new ewxPanel( itemNotebook3, ID_ROTATE_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer85 = new wxBoxSizer(wxVERTICAL);
    itemPanel84->SetSizer(itemBoxSizer85);

    wxFlexGridSizer* itemFlexGridSizer86 = new wxFlexGridSizer(4, 5, 0, 0);
    itemFlexGridSizer86->AddGrowableCol(1);
    itemFlexGridSizer86->AddGrowableCol(2);
    itemFlexGridSizer86->AddGrowableCol(3);
    itemBoxSizer85->Add(itemFlexGridSizer86, 0, wxGROW, 0);
    itemFlexGridSizer86->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText88 = new ewxStaticText( itemPanel84, wxID_STATIC, _("X"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer86->Add(itemStaticText88, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText89 = new ewxStaticText( itemPanel84, wxID_STATIC, _("Y"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer86->Add(itemStaticText89, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText90 = new ewxStaticText( itemPanel84, wxID_STATIC, _("Z"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer86->Add(itemStaticText90, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    itemFlexGridSizer86->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText92 = new ewxStaticText( itemPanel84, wxID_STATIC, _("Axis Start"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer86->Add(itemStaticText92, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl93 = new ewxTextCtrl( itemPanel84, ID_ROTATE_START_X, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer86->Add(itemTextCtrl93, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl94 = new ewxTextCtrl( itemPanel84, ID_ROTATE_START_Y, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer86->Add(itemTextCtrl94, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl95 = new ewxTextCtrl( itemPanel84, ID_ROTATE_START_Z, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer86->Add(itemTextCtrl95, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxButton* itemButton96 = new ewxButton( itemPanel84, ID_ROTATE_START_PICK, _("Get Sel."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton96->SetToolTip(_("Get coordinates from builder"));
    itemFlexGridSizer86->Add(itemButton96, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 3);

    ewxStaticText* itemStaticText97 = new ewxStaticText( itemPanel84, wxID_STATIC, _("Axis End"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer86->Add(itemStaticText97, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl98 = new ewxTextCtrl( itemPanel84, ID_ROTATE_END_X, _("1.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer86->Add(itemTextCtrl98, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl99 = new ewxTextCtrl( itemPanel84, ID_ROTATE_END_Y, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer86->Add(itemTextCtrl99, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl100 = new ewxTextCtrl( itemPanel84, ID_ROTATE_END_Z, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer86->Add(itemTextCtrl100, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxButton* itemButton101 = new ewxButton( itemPanel84, ID_ROTATE_END_PICK, _("Get Sel."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    if (ShowToolTips())
        itemButton101->SetToolTip(_("Get coordinates from builder"));
    itemFlexGridSizer86->Add(itemButton101, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 3);

    itemFlexGridSizer86->Add(0, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);

    itemFlexGridSizer86->Add(0, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);

    ewxStaticText* itemStaticText104 = new ewxStaticText( itemPanel84, wxID_STATIC, _("By"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer86->Add(itemStaticText104, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl105 = new ewxTextCtrl( itemPanel84, ID_ROTATE_DEGREE, _("90.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    if (ShowToolTips())
        itemTextCtrl105->SetToolTip(_("Rotation angle about axis vector"));
    itemFlexGridSizer86->Add(itemTextCtrl105, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText106 = new ewxStaticText( itemPanel84, wxID_STATIC, _("degree"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer86->Add(itemStaticText106, 0, wxALIGN_LEFT|wxALIGN_BOTTOM|wxADJUST_MINSIZE, 3);

    itemBoxSizer85->Add(0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 0);

    ewxStaticLine* itemStaticLine108 = new ewxStaticLine( itemPanel84, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer85->Add(itemStaticLine108, 0, wxGROW|wxLEFT|wxRIGHT, 3);

    wxBoxSizer* itemBoxSizer109 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer85->Add(itemBoxSizer109, 0, wxGROW, 0);
    ewxStaticText* itemStaticText110 = new ewxStaticText( itemPanel84, wxID_STATIC, _("Apply to:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer109->Add(itemStaticText110, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice111Strings[] = {
        _("All"),
        _("Selection")
    };
    ewxChoice* itemChoice111 = new ewxChoice( itemPanel84, ID_ROT_APPLY_TO, wxDefaultPosition, wxDefaultSize, 2, itemChoice111Strings, 0 );
    itemChoice111->SetStringSelection(_("All"));
    itemBoxSizer109->Add(itemChoice111, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer109->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton113 = new ewxButton( itemPanel84, ID_ROTATE, _("Rotate"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemButton113->SetDefault();
    itemBoxSizer109->Add(itemButton113, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook3->AddPage(itemPanel84, _("Rotate"));

    ewxPanel* itemPanel114 = new ewxPanel( itemNotebook3, ID_ALIGN_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer115 = new wxBoxSizer(wxVERTICAL);
    itemPanel114->SetSizer(itemBoxSizer115);

    wxBoxSizer* itemBoxSizer116 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer115->Add(itemBoxSizer116, 0, wxGROW, 0);
    ewxStaticText* itemStaticText117 = new ewxStaticText( itemPanel114, wxID_STATIC, _("Align to"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer116->Add(itemStaticText117, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxRadioButton* itemRadioButton118 = new ewxRadioButton( itemPanel114, ID_ALIGN_TO_AXIS, _("Axis"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton118->SetValue(false);
    if (ShowToolTips())
        itemRadioButton118->SetToolTip(_("Align point to axis"));
    itemBoxSizer116->Add(itemRadioButton118, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton119 = new ewxRadioButton( itemPanel114, ID_ALIGN_TO_PLANE, _("Plane"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton119->SetValue(false);
    if (ShowToolTips())
        itemRadioButton119->SetToolTip(_("Align point to plane"));
    itemBoxSizer116->Add(itemRadioButton119, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer120 = new wxFlexGridSizer(3, 4, 0, 0);
    itemFlexGridSizer120->AddGrowableCol(1);
    itemFlexGridSizer120->AddGrowableCol(2);
    itemFlexGridSizer120->AddGrowableCol(3);
    itemBoxSizer115->Add(itemFlexGridSizer120, 0, wxGROW, 0);
    itemFlexGridSizer120->Add(0, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText122 = new ewxStaticText( itemPanel114, wxID_STATIC, _("n_x"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer120->Add(itemStaticText122, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText123 = new ewxStaticText( itemPanel114, wxID_STATIC, _("n_y"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer120->Add(itemStaticText123, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText124 = new ewxStaticText( itemPanel114, wxID_STATIC, _("n_z"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer120->Add(itemStaticText124, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText125 = new ewxStaticText( itemPanel114, ID_STATIC_PLAX, _("Axis    "), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer120->Add(itemStaticText125, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl126 = new ewxTextCtrl( itemPanel114, ID_ALIGN_NX, _("1.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer120->Add(itemTextCtrl126, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl127 = new ewxTextCtrl( itemPanel114, ID_ALIGN_NY, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer120->Add(itemTextCtrl127, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    ewxTextCtrl* itemTextCtrl128 = new ewxTextCtrl( itemPanel114, ID_ALIGN_NZ, _("0.0"), wxDefaultPosition, wxSize(60, -1), 0 );
    itemFlexGridSizer120->Add(itemTextCtrl128, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxBOTTOM, 3);

    itemFlexGridSizer120->Add(54, 0, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton130 = new ewxButton( itemPanel114, ID_BUTTON_ALIGN_X_YZ, _("  X Axis   "), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemFlexGridSizer120->Add(itemButton130, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton131 = new ewxButton( itemPanel114, ID_BUTTON_ALIGN_Y_ZX, _("  Y Axis   "), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemFlexGridSizer120->Add(itemButton131, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton132 = new ewxButton( itemPanel114, ID_BUTTON_ALIGN_Z_XY, _("  Z Axis   "), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemFlexGridSizer120->Add(itemButton132, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer115->Add(0, 0, 1, wxALIGN_CENTER_HORIZONTAL, 0);

    ewxStaticLine* itemStaticLine134 = new ewxStaticLine( itemPanel114, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer115->Add(itemStaticLine134, 0, wxGROW|wxLEFT|wxRIGHT, 5);

    wxBoxSizer* itemBoxSizer135 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer115->Add(itemBoxSizer135, 0, wxGROW, 0);
    ewxStaticText* itemStaticText136 = new ewxStaticText( itemPanel114, wxID_STATIC, _("Apply to:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer135->Add(itemStaticText136, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice137Strings[] = {
        _("All"),
        _("Selection")
    };
    ewxChoice* itemChoice137 = new ewxChoice( itemPanel114, ID_ALIGN_APPLY_TO, wxDefaultPosition, wxDefaultSize, 2, itemChoice137Strings, 0 );
    itemChoice137->SetStringSelection(_("All"));
    itemBoxSizer135->Add(itemChoice137, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer135->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton139 = new ewxButton( itemPanel114, ID_ALIGN, _("Align"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
    itemButton139->SetDefault();
    itemBoxSizer135->Add(itemButton139, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook3->AddPage(itemPanel114, _("Align"));

    itemBoxSizer2->Add(itemNotebook3, 1, wxGROW|wxALL, 0);

////@end CoordPanelGUI content construction
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIO_CENTER_ORIGIN
 */

void CoordPanelGUI::OnCenterSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIO_CENTER_ORIGIN in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIO_CENTER_ORIGIN in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CENTER_APPLY_TO
 */

void CoordPanelGUI::OnCenterApplyToSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CENTER_APPLY_TO in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CENTER_APPLY_TO in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CENTER
 */

void CoordPanelGUI::OnCenterClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CENTER in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CENTER in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_X
 */

void CoordPanelGUI::OnTransByXEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_X in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_X in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_Y
 */

void CoordPanelGUI::OnTransByYEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_Y in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_Y in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_Z
 */

void CoordPanelGUI::OnTransByZEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_Z in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_BY_Z in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_TRANS_EXPAND
 */

void CoordPanelGUI::OnTransExpandClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_TRANS_EXPAND in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_TRANS_EXPAND in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_X
 */

void CoordPanelGUI::OnTransFromXEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_X in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_X in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_Y
 */

void CoordPanelGUI::OnTransFromYEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_Y in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_Y in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_Z
 */

void CoordPanelGUI::OnTransFromZEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_Z in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_FROM_Z in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_FROM_PICK
 */

void CoordPanelGUI::OnTransFromPickClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_FROM_PICK in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_FROM_PICK in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_X
 */

void CoordPanelGUI::OnTransToXEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_X in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_X in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_Y
 */

void CoordPanelGUI::OnTransToYEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_Y in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_Y in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_Z
 */

void CoordPanelGUI::OnTransToZEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_Z in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TRANS_TO_Z in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_TO_PICK
 */

void CoordPanelGUI::OnTransToPickClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_TO_PICK in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_TO_PICK in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANSLATE
 */

void CoordPanelGUI::OnTranslateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANSLATE in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANSLATE in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_X
 */

void CoordPanelGUI::OnDuplicateDim1XEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_X in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_X in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_Y
 */

void CoordPanelGUI::OnDuplicateDim1YEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_Y in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_Y in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_Z
 */

void CoordPanelGUI::OnDuplicateDim1ZEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_Z in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_Z in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_X
 */

void CoordPanelGUI::OnDuplicateDim2XEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_X in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_X in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_Y
 */

void CoordPanelGUI::OnDuplicateDim2YEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_Y in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_Y in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_Z
 */

void CoordPanelGUI::OnDuplicateDim2ZEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_Z in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_Z in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_X
 */

void CoordPanelGUI::OnDuplicateDim3XEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_X in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_X in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_Y
 */

void CoordPanelGUI::OnDuplicateDim3YEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_Y in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_Y in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_Z
 */

void CoordPanelGUI::OnDuplicateDim3ZEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_Z in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_Z in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DUPLICATE
 */

void CoordPanelGUI::OnDuplicateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DUPLICATE in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DUPLICATE in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_START_X
 */

void CoordPanelGUI::OnRotateStartUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_START_X in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_START_X in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_START_PICK
 */

void CoordPanelGUI::OnRotateStartPickClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_START_PICK in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_START_PICK in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_END_X
 */

void CoordPanelGUI::OnRotateEndUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_END_X in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_END_X in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_END_PICK
 */

void CoordPanelGUI::OnRotateEndPickClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_END_PICK in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_END_PICK in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE
 */

void CoordPanelGUI::OnRotateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_ALIGN_TO_AXIS
 */

void CoordPanelGUI::OnAlignToAxisSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_ALIGN_TO_AXIS in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_ALIGN_TO_AXIS in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_ALIGN_TO_PLANE
 */

void CoordPanelGUI::OnAlignToPlaneSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_ALIGN_TO_PLANE in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_ALIGN_TO_PLANE in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_X_YZ
 */

void CoordPanelGUI::OnButtonAlignX_YZClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_X_YZ in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_X_YZ in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Y_ZX
 */

void CoordPanelGUI::OnButtonAlignY_ZXClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Y_ZX in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Y_ZX in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Z_XY
 */

void CoordPanelGUI::OnButtonAlignZ_XYClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Z_XY in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Z_XY in CoordPanelGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ALIGN
 */

void CoordPanelGUI::OnAlignClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ALIGN in CoordPanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ALIGN in CoordPanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool CoordPanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap CoordPanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin CoordPanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end CoordPanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon CoordPanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin CoordPanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end CoordPanelGUI icon retrieval
}
