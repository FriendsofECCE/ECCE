/////////////////////////////////////////////////////////////////////////////
// Name:        VizStyleChooserGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "VizStyleChooserGUI.H"
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
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxOpenCloseToggle.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxFrame.H"
////@end includes

#include "wxviz/VizStyleChooserGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID VizStyleChooserGUI::ID_RADIOBUTTON_DSPLY_NONE = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_CHOICE_DSPLY_CSOPT = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_CCYLRQ = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_HLXW = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_PANEL_DSPLY_BLOCKSCHEME = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_BUTTON_DSPLY_SAVE = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_HLXRAD = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_CYLRAD = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_SPHSCLTXT = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_SPHRQ = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_BUTTON_DSPLY_HELP = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_HLXW = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_HLXRAD = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_STATICTEXT1 = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_CYLRQ = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_CYLRQMIN = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_RADIOBUTTON_DSPLY_ROUND = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_SPHSCL = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_RBTHK = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_RBTHK = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_CYLRADTXT = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_PANEL_DSP_CHOICES = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_SPHSCLMAX = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_PANEL_DSPLY_BLOCKCOLOR = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_COILW = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_BUTTON_DSPLY_UPDATE = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_BUTTON_DSPLY_REMOVE = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_CCYLRQ = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_LNWDTH = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_BUTTON_DSPLY_CLOSE = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_TUBER = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_CHOICE_DSPLY_BONDCYLINDER = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_PANEL_DSPLY_RIBBON = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_CHOICE_DSPLY_ATOMSPHERE = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_CYLRQ = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_RESRAD = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_VIZSTYLECHOOSER = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_CYLRAD = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_RESRADMAX = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_RESRADTXT = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_PANEL_DSPLY_LNWDTH = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_TOGGLE_DSPLY_SHOW = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_SHTW = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_CYLR = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_RESRAD = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_CHECKBOX_DSPLY_BLNDCL = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_CHECKBOX_DSPLY_RNDRB = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_PANEL_DSPLY_CYLNDR = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_CHOICE_DSPLY_DSOPT = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_COILW = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_BLOCKCOLOR = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_SPHSCL = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_SPHSCLMIN = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_CYLRQMAX = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_TUBER = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_PANEL_DSPLY_SPECTRUM = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_RADIOBUTTON_DSPLY_FLAT = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_LNWDTH = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_RBTHKMAX = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_BUTTON_DSPLY_BLOCKEDIT = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_CYLRADMIN = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_BUTTON_DSPLY_ADD = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_RBTHKTXT = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_RBTHKMIN = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_CYLRQTXT = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_SLIDER_DSPLY_SPHRQ = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_BUTTON_DSPLY_RESET = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_PANEL_DSPLY_CNTRLS = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_SHTW = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_PANEL_DSPLY_CRTN = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_CYLR = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_PANEL_DSPLY_SWOTH = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_RESRADMIN = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_PANEL_DSPLY_SPHERE = wxNewId();
const wxWindowID VizStyleChooserGUI::ID_LISTBOX_DSPLY_GROUPS = wxNewId();
const wxWindowID VizStyleChooserGUI::wxID_STATIC_DSPLY_CYLRADMAX = wxNewId();

/*!
 * VizStyleChooserGUI type definition
 */

IMPLEMENT_CLASS( VizStyleChooserGUI, ewxFrame )

/*!
 * VizStyleChooserGUI event table definition
 */

BEGIN_EVENT_TABLE( VizStyleChooserGUI, ewxFrame )

////@begin VizStyleChooserGUI event table entries
    EVT_CLOSE( VizStyleChooserGUI::OnCloseWindow )

    EVT_CHOICE( ID_CHOICE_DSPLY_DSOPT, VizStyleChooserGUI::OnChoiceDsplyDsoptSelected )

    EVT_CHOICE( ID_CHOICE_DSPLY_CSOPT, VizStyleChooserGUI::OnChoiceDsplyCsoptSelected )

    EVT_BUTTON( ID_BUTTON_DSPLY_BLOCKEDIT, VizStyleChooserGUI::OnButtonDsplyBlockeditClick )

    EVT_BUTTON( ID_TOGGLE_DSPLY_SHOW, VizStyleChooserGUI::OnToggleDsplyShowClick )

    EVT_SLIDER( ID_SLIDER_DSPLY_LNWDTH, VizStyleChooserGUI::OnSliderDsplyLnwdthUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_CYLRQ, VizStyleChooserGUI::OnSliderDsplyCylrqUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_CYLRAD, VizStyleChooserGUI::OnSliderDsplyCylradUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_RESRAD, VizStyleChooserGUI::OnSliderDsplyResradUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_SPHRQ, VizStyleChooserGUI::OnSliderDsplySphrqUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_SPHSCL, VizStyleChooserGUI::OnSliderDsplySphsclUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_COILW, VizStyleChooserGUI::OnSliderDsplyCoilwUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_HLXRAD, VizStyleChooserGUI::OnSliderDsplyHlxradUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_HLXW, VizStyleChooserGUI::OnSliderDsplyHlxwUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_SHTW, VizStyleChooserGUI::OnSliderDsplyShtwUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_RBTHK, VizStyleChooserGUI::OnSliderDsplyRbthkUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_CCYLRQ, VizStyleChooserGUI::OnSliderDsplyCcylrqUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_CYLR, VizStyleChooserGUI::OnSliderDsplyCylrUpdated )

    EVT_SLIDER( ID_SLIDER_DSPLY_TUBER, VizStyleChooserGUI::OnSliderDsplyTuberUpdated )

    EVT_LISTBOX( ID_LISTBOX_DSPLY_GROUPS, VizStyleChooserGUI::OnListboxDsplyGroupsSelected )

    EVT_BUTTON( ID_BUTTON_DSPLY_UPDATE, VizStyleChooserGUI::OnButtonDsplyUpdateClick )

    EVT_BUTTON( ID_BUTTON_DSPLY_ADD, VizStyleChooserGUI::OnButtonDsplyAddClick )

    EVT_BUTTON( ID_BUTTON_DSPLY_REMOVE, VizStyleChooserGUI::OnButtonDsplyRemoveClick )

    EVT_BUTTON( ID_BUTTON_DSPLY_CLOSE, VizStyleChooserGUI::OnButtonDsplyCloseClick )

    EVT_BUTTON( ID_BUTTON_DSPLY_SAVE, VizStyleChooserGUI::OnButtonDsplySaveClick )

    EVT_BUTTON( ID_BUTTON_DSPLY_RESET, VizStyleChooserGUI::OnButtonDsplyResetClick )

    EVT_BUTTON( ID_BUTTON_DSPLY_HELP, VizStyleChooserGUI::OnButtonDsplyHelpClick )

////@end VizStyleChooserGUI event table entries

END_EVENT_TABLE()

/*!
 * VizStyleChooserGUI constructors
 */

VizStyleChooserGUI::VizStyleChooserGUI( )
{
}

VizStyleChooserGUI::VizStyleChooserGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * VizStyleChooserGUI creator
 */

bool VizStyleChooserGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin VizStyleChooserGUI member initialisation
    p_mainSizer = NULL;
    p_optionSizer = NULL;
////@end VizStyleChooserGUI member initialisation

////@begin VizStyleChooserGUI creation
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
////@end VizStyleChooserGUI creation
    return true;
}

/*!
 * Control creation for VizStyleChooserGUI
 */

void VizStyleChooserGUI::CreateControls()
{    
////@begin VizStyleChooserGUI content construction
    VizStyleChooserGUI* itemFrame1 = this;

    p_mainSizer = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(p_mainSizer);

    ewxPanel* itemPanel3 = new ewxPanel( itemFrame1, ID_PANEL_DSP_CHOICES, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_mainSizer->Add(itemPanel3, 0, wxGROW|wxALL, 0);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(2, 2, 0, 0);
    itemPanel3->SetSizer(itemFlexGridSizer4);

    ewxStaticText* itemStaticText5 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Display Style:"), wxDefaultPosition, wxSize(140, -1), 0 );
    itemFlexGridSizer4->Add(itemStaticText5, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice6Strings = NULL;
    ewxChoice* itemChoice6 = new ewxChoice( itemPanel3, ID_CHOICE_DSPLY_DSOPT, wxDefaultPosition, wxDefaultSize, 0, itemChoice6Strings, 0 );
    itemFlexGridSizer4->Add(itemChoice6, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Color Scheme:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText7, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice8Strings = NULL;
    ewxChoice* itemChoice8 = new ewxChoice( itemPanel3, ID_CHOICE_DSPLY_CSOPT, wxDefaultPosition, wxDefaultSize, 0, itemChoice8Strings, 0 );
    itemFlexGridSizer4->Add(itemChoice8, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel9 = new ewxPanel( itemFrame1, ID_PANEL_DSPLY_SPECTRUM, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel9->Show(false);
    p_mainSizer->Add(itemPanel9, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel9->SetSizer(itemBoxSizer10);

    ewxStaticText* itemStaticText11 = new ewxStaticText( itemPanel9, wxID_STATIC, _("Color Spectrum:"), wxDefaultPosition, wxSize(140, -1), 0 );
    itemBoxSizer10->Add(itemStaticText11, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxPanel* itemPanel12 = new ewxPanel( itemFrame1, ID_PANEL_DSPLY_SWOTH, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel12->Show(false);
    p_mainSizer->Add(itemPanel12, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel12->SetSizer(itemBoxSizer13);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemPanel12, wxID_STATIC, _("Colors:"), wxDefaultPosition, wxSize(140, -1), 0 );
    itemBoxSizer13->Add(itemStaticText14, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxPanel* itemPanel15 = new ewxPanel( itemFrame1, ID_PANEL_DSPLY_BLOCKSCHEME, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel15->Show(false);
    p_mainSizer->Add(itemPanel15, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel15->SetSizer(itemBoxSizer16);

    ewxStaticText* itemStaticText17 = new ewxStaticText( itemPanel15, wxID_STATIC_DSPLY_BLOCKCOLOR, _("Color"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemStaticText17, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxPanel* itemPanel18 = new ewxPanel( itemPanel15, ID_PANEL_DSPLY_BLOCKCOLOR, wxDefaultPosition, wxSize(30, 20), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer16->Add(itemPanel18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer16->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton20 = new ewxButton( itemPanel15, ID_BUTTON_DSPLY_BLOCKEDIT, _("Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemButton20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxHORIZONTAL);
    p_mainSizer->Add(itemBoxSizer21, 0, wxALIGN_LEFT|wxALL, 0);

    wxBitmap itemBitmapButton22Bitmap(wxNullBitmap);
    ewxOpenCloseToggle* itemBitmapButton22 = new ewxOpenCloseToggle( itemFrame1, ID_TOGGLE_DSPLY_SHOW, itemBitmapButton22Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    itemBoxSizer21->Add(itemBitmapButton22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText23 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Advanced Options"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer21->Add(itemStaticText23, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxPanel* itemPanel24 = new ewxPanel( itemFrame1, ID_PANEL_DSPLY_CNTRLS, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
    p_mainSizer->Add(itemPanel24, 0, wxGROW|wxLEFT, 15);

    p_optionSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel24->SetSizer(p_optionSizer);

    ewxPanel* itemPanel26 = new ewxPanel( itemPanel24, ID_PANEL_DSPLY_LNWDTH, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel26->Show(false);
    p_optionSizer->Add(itemPanel26, 0, wxALIGN_LEFT|wxALL, 0);

    wxFlexGridSizer* itemFlexGridSizer27 = new wxFlexGridSizer(2, 3, 0, 0);
    itemPanel26->SetSizer(itemFlexGridSizer27);

    ewxStaticText* itemStaticText28 = new ewxStaticText( itemPanel26, wxID_STATIC, _("Line Width:"), wxDefaultPosition, wxSize(140, -1), 0 );
    itemFlexGridSizer27->Add(itemStaticText28, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider29 = new wxSlider( itemPanel26, ID_SLIDER_DSPLY_LNWDTH, 1, 1, 10, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer27->Add(itemSlider29, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText30 = new ewxStaticText( itemPanel26, wxID_STATIC_DSPLY_LNWDTH, _("1"), wxDefaultPosition, wxSize(30, -1), 0 );
    itemFlexGridSizer27->Add(itemStaticText30, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    itemFlexGridSizer27->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer32 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer27->Add(itemBoxSizer32, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText33 = new ewxStaticText( itemPanel26, wxID_STATIC, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemStaticText33, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer32->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText35 = new ewxStaticText( itemPanel26, wxID_STATIC, _("10"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemStaticText35, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer27->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxPanel* itemPanel37 = new ewxPanel( itemPanel24, ID_PANEL_DSPLY_CYLNDR, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel37->Show(false);
    p_optionSizer->Add(itemPanel37, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer38 = new wxBoxSizer(wxVERTICAL);
    itemPanel37->SetSizer(itemBoxSizer38);

    wxBoxSizer* itemBoxSizer39 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer38->Add(itemBoxSizer39, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText40 = new ewxStaticText( itemPanel37, wxID_STATIC, _("Cap Style:"), wxDefaultPosition, wxSize(140, -1), 0 );
    itemBoxSizer39->Add(itemStaticText40, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxRadioButton* itemRadioButton41 = new ewxRadioButton( itemPanel37, ID_RADIOBUTTON_DSPLY_NONE, _("None"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
    itemRadioButton41->SetValue(false);
    itemBoxSizer39->Add(itemRadioButton41, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton42 = new ewxRadioButton( itemPanel37, ID_RADIOBUTTON_DSPLY_FLAT, _("Flat"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton42->SetValue(false);
    itemBoxSizer39->Add(itemRadioButton42, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxRadioButton* itemRadioButton43 = new ewxRadioButton( itemPanel37, ID_RADIOBUTTON_DSPLY_ROUND, _("Round"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton43->SetValue(false);
    itemBoxSizer39->Add(itemRadioButton43, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer44 = new wxFlexGridSizer(6, 3, 0, 0);
    itemBoxSizer38->Add(itemFlexGridSizer44, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText45 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_CYLRQTXT, _("Cylinder Quality:"), wxDefaultPosition, wxSize(140, -1), 0 );
    itemFlexGridSizer44->Add(itemStaticText45, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider46 = new wxSlider( itemPanel37, ID_SLIDER_DSPLY_CYLRQ, 56, 1, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer44->Add(itemSlider46, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText47 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_CYLRQ, _("56"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer44->Add(itemStaticText47, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer44->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer49 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer44->Add(itemBoxSizer49, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText50 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_CYLRQMIN, _("1%"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer49->Add(itemStaticText50, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer49->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText52 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_CYLRQMAX, _("100%"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer49->Add(itemStaticText52, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer44->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText54 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_CYLRADTXT, _("Cylinder Radius:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer44->Add(itemStaticText54, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider55 = new wxSlider( itemPanel37, ID_SLIDER_DSPLY_CYLRAD, 14, 1, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer44->Add(itemSlider55, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText56 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_CYLRAD, _("14"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer44->Add(itemStaticText56, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer44->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer58 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer44->Add(itemBoxSizer58, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText59 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_CYLRADMIN, _("1%"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer58->Add(itemStaticText59, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer58->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText61 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_CYLRADMAX, _("100%"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer58->Add(itemStaticText61, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer44->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText63 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_RESRADTXT, _("Cylinder Radius:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer44->Add(itemStaticText63, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider64 = new wxSlider( itemPanel37, ID_SLIDER_DSPLY_RESRAD, 10, 0, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer44->Add(itemSlider64, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText65 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_RESRAD, _("1.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer44->Add(itemStaticText65, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer44->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer67 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer44->Add(itemBoxSizer67, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText68 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_RESRADMIN, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer67->Add(itemStaticText68, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer67->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText70 = new ewxStaticText( itemPanel37, wxID_STATIC_DSPLY_RESRADMAX, _("10.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer67->Add(itemStaticText70, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer44->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer72 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer38->Add(itemBoxSizer72, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText73 = new ewxStaticText( itemPanel37, wxID_STATIC, _("Cylinder Rendering:"), wxDefaultPosition, wxSize(140, -1), 0 );
    itemBoxSizer72->Add(itemStaticText73, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice74Strings = NULL;
    ewxChoice* itemChoice74 = new ewxChoice( itemPanel37, ID_CHOICE_DSPLY_BONDCYLINDER, wxDefaultPosition, wxDefaultSize, 0, itemChoice74Strings, 0 );
    itemBoxSizer72->Add(itemChoice74, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel75 = new ewxPanel( itemPanel24, ID_PANEL_DSPLY_SPHERE, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel75->Show(false);
    p_optionSizer->Add(itemPanel75, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer76 = new wxBoxSizer(wxVERTICAL);
    itemPanel75->SetSizer(itemBoxSizer76);

    wxFlexGridSizer* itemFlexGridSizer77 = new wxFlexGridSizer(4, 3, 0, 0);
    itemBoxSizer76->Add(itemFlexGridSizer77, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText78 = new ewxStaticText( itemPanel75, wxID_STATIC, _("Sphere Quality:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer77->Add(itemStaticText78, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider79 = new wxSlider( itemPanel75, ID_SLIDER_DSPLY_SPHRQ, 50, 1, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer77->Add(itemSlider79, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText80 = new ewxStaticText( itemPanel75, wxID_STATIC_DSPLY_SPHRQ, _("50"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer77->Add(itemStaticText80, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer77->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer82 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer77->Add(itemBoxSizer82, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText83 = new ewxStaticText( itemPanel75, wxID_STATIC, _("1%"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer82->Add(itemStaticText83, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer82->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText85 = new ewxStaticText( itemPanel75, wxID_STATIC, _("100%"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer82->Add(itemStaticText85, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer77->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText87 = new ewxStaticText( itemPanel75, wxID_STATIC_DSPLY_SPHSCLTXT, _("Ball and Stick Scale:"), wxDefaultPosition, wxSize(140, -1), 0 );
    itemFlexGridSizer77->Add(itemStaticText87, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider88 = new wxSlider( itemPanel75, ID_SLIDER_DSPLY_SPHSCL, 50, 1, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer77->Add(itemSlider88, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText89 = new ewxStaticText( itemPanel75, wxID_STATIC_DSPLY_SPHSCL, _("50"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer77->Add(itemStaticText89, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer77->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer91 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer77->Add(itemBoxSizer91, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText92 = new ewxStaticText( itemPanel75, wxID_STATIC_DSPLY_SPHSCLMIN, _("1%"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer91->Add(itemStaticText92, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer91->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText94 = new ewxStaticText( itemPanel75, wxID_STATIC_DSPLY_SPHSCLMAX, _("100%"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer91->Add(itemStaticText94, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer77->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer96 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer76->Add(itemBoxSizer96, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText97 = new ewxStaticText( itemPanel75, ID_STATICTEXT1, _("Sphere Rendering:"), wxDefaultPosition, wxSize(140, -1), 0 );
    itemBoxSizer96->Add(itemStaticText97, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice98Strings = NULL;
    ewxChoice* itemChoice98 = new ewxChoice( itemPanel75, ID_CHOICE_DSPLY_ATOMSPHERE, wxDefaultPosition, wxDefaultSize, 0, itemChoice98Strings, 0 );
    itemBoxSizer96->Add(itemChoice98, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel99 = new ewxPanel( itemPanel24, ID_PANEL_DSPLY_RIBBON, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel99->Show(false);
    p_optionSizer->Add(itemPanel99, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer100 = new wxBoxSizer(wxVERTICAL);
    itemPanel99->SetSizer(itemBoxSizer100);

    wxBoxSizer* itemBoxSizer101 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer100->Add(itemBoxSizer101, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox102 = new ewxCheckBox( itemPanel99, ID_CHECKBOX_DSPLY_BLNDCL, _("Blend Ribbon Colors"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox102->SetValue(false);
    itemBoxSizer101->Add(itemCheckBox102, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer103 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer100->Add(itemBoxSizer103, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox104 = new ewxCheckBox( itemPanel99, ID_CHECKBOX_DSPLY_RNDRB, _("Rounded Ribbon Edges"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox104->SetValue(false);
    itemBoxSizer103->Add(itemCheckBox104, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer105 = new wxFlexGridSizer(10, 3, 0, 0);
    itemBoxSizer100->Add(itemFlexGridSizer105, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxStaticText* itemStaticText106 = new ewxStaticText( itemPanel99, wxID_STATIC, _("Coil Width:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer105->Add(itemStaticText106, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider107 = new wxSlider( itemPanel99, ID_SLIDER_DSPLY_COILW, 10, 0, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer105->Add(itemSlider107, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText108 = new ewxStaticText( itemPanel99, wxID_STATIC_DSPLY_COILW, _("1.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer105->Add(itemStaticText108, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer105->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer110 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer105->Add(itemBoxSizer110, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText111 = new ewxStaticText( itemPanel99, wxID_STATIC, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer110->Add(itemStaticText111, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer110->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText113 = new ewxStaticText( itemPanel99, wxID_STATIC, _("10.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer110->Add(itemStaticText113, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer105->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText115 = new ewxStaticText( itemPanel99, wxID_STATIC, _("Helix Radius:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer105->Add(itemStaticText115, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider116 = new wxSlider( itemPanel99, ID_SLIDER_DSPLY_HLXRAD, 30, 0, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer105->Add(itemSlider116, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText117 = new ewxStaticText( itemPanel99, wxID_STATIC_DSPLY_HLXRAD, _("3.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer105->Add(itemStaticText117, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer105->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer119 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer105->Add(itemBoxSizer119, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText120 = new ewxStaticText( itemPanel99, wxID_STATIC, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer119->Add(itemStaticText120, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer119->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText122 = new ewxStaticText( itemPanel99, wxID_STATIC, _("10.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer119->Add(itemStaticText122, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer105->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText124 = new ewxStaticText( itemPanel99, wxID_STATIC, _("Helix Width:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer105->Add(itemStaticText124, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider125 = new wxSlider( itemPanel99, ID_SLIDER_DSPLY_HLXW, 30, 0, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer105->Add(itemSlider125, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText126 = new ewxStaticText( itemPanel99, wxID_STATIC_DSPLY_HLXW, _("3.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer105->Add(itemStaticText126, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer105->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer128 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer105->Add(itemBoxSizer128, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText129 = new ewxStaticText( itemPanel99, wxID_STATIC, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer128->Add(itemStaticText129, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer128->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText131 = new ewxStaticText( itemPanel99, wxID_STATIC, _("10.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer128->Add(itemStaticText131, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer105->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText133 = new ewxStaticText( itemPanel99, wxID_STATIC, _("Sheet Width:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer105->Add(itemStaticText133, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider134 = new wxSlider( itemPanel99, ID_SLIDER_DSPLY_SHTW, 30, 0, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer105->Add(itemSlider134, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText135 = new ewxStaticText( itemPanel99, wxID_STATIC_DSPLY_SHTW, _("3.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer105->Add(itemStaticText135, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer105->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer137 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer105->Add(itemBoxSizer137, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText138 = new ewxStaticText( itemPanel99, wxID_STATIC, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer137->Add(itemStaticText138, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer137->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText140 = new ewxStaticText( itemPanel99, wxID_STATIC, _("10.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer137->Add(itemStaticText140, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer105->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText142 = new ewxStaticText( itemPanel99, wxID_STATIC_DSPLY_RBTHKTXT, _("Ribbon Thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer105->Add(itemStaticText142, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider143 = new wxSlider( itemPanel99, ID_SLIDER_DSPLY_RBTHK, 10, 0, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer105->Add(itemSlider143, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText144 = new ewxStaticText( itemPanel99, wxID_STATIC_DSPLY_RBTHK, _("0.25"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer105->Add(itemStaticText144, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer105->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer146 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer105->Add(itemBoxSizer146, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText147 = new ewxStaticText( itemPanel99, wxID_STATIC_DSPLY_RBTHKMIN, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer146->Add(itemStaticText147, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer146->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText149 = new ewxStaticText( itemPanel99, wxID_STATIC_DSPLY_RBTHKMAX, _("5.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer146->Add(itemStaticText149, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer105->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxPanel* itemPanel151 = new ewxPanel( itemPanel24, ID_PANEL_DSPLY_CRTN, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel151->Show(false);
    p_optionSizer->Add(itemPanel151, 0, wxALIGN_LEFT|wxALL, 0);

    wxFlexGridSizer* itemFlexGridSizer152 = new wxFlexGridSizer(6, 3, 0, 0);
    itemPanel151->SetSizer(itemFlexGridSizer152);

    ewxStaticText* itemStaticText153 = new ewxStaticText( itemPanel151, wxID_STATIC, _("Cylinder Quality:"), wxDefaultPosition, wxSize(140, -1), 0 );
    itemFlexGridSizer152->Add(itemStaticText153, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider154 = new wxSlider( itemPanel151, ID_SLIDER_DSPLY_CCYLRQ, 70, 0, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer152->Add(itemSlider154, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText155 = new ewxStaticText( itemPanel151, wxID_STATIC_DSPLY_CCYLRQ, _("70"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer152->Add(itemStaticText155, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer152->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer157 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer152->Add(itemBoxSizer157, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText158 = new ewxStaticText( itemPanel151, wxID_STATIC, _("1%"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer157->Add(itemStaticText158, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer157->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText160 = new ewxStaticText( itemPanel151, wxID_STATIC, _("100%"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer157->Add(itemStaticText160, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer152->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText162 = new ewxStaticText( itemPanel151, wxID_STATIC, _("Cylinder Radius:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer152->Add(itemStaticText162, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider163 = new wxSlider( itemPanel151, ID_SLIDER_DSPLY_CYLR, 23, 0, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer152->Add(itemSlider163, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText164 = new ewxStaticText( itemPanel151, wxID_STATIC_DSPLY_CYLR, _("2.3"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer152->Add(itemStaticText164, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer152->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer166 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer152->Add(itemBoxSizer166, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText167 = new ewxStaticText( itemPanel151, wxID_STATIC, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer166->Add(itemStaticText167, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer166->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText169 = new ewxStaticText( itemPanel151, wxID_STATIC, _("10.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer166->Add(itemStaticText169, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer152->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText171 = new ewxStaticText( itemPanel151, wxID_STATIC, _("Tube Radius:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer152->Add(itemStaticText171, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxSlider* itemSlider172 = new wxSlider( itemPanel151, ID_SLIDER_DSPLY_TUBER, 7, 1, 100, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL );
    itemFlexGridSizer152->Add(itemSlider172, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText173 = new ewxStaticText( itemPanel151, wxID_STATIC_DSPLY_TUBER, _("1.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer152->Add(itemStaticText173, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer152->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer175 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer152->Add(itemBoxSizer175, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText176 = new ewxStaticText( itemPanel151, wxID_STATIC, _("0.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer175->Add(itemStaticText176, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemBoxSizer175->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText178 = new ewxStaticText( itemPanel151, wxID_STATIC, _("10.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer175->Add(itemStaticText178, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer152->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer180 = new wxBoxSizer(wxHORIZONTAL);
    p_mainSizer->Add(itemBoxSizer180, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine181 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer180->Add(itemStaticLine181, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer182 = new wxBoxSizer(wxHORIZONTAL);
    p_mainSizer->Add(itemBoxSizer182, 1, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText183 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Current Styles:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer182->Add(itemStaticText183, 0, wxALIGN_TOP|wxALL|wxADJUST_MINSIZE, 5);

    wxString* itemListBox184Strings = NULL;
    ewxListBox* itemListBox184 = new ewxListBox( itemFrame1, ID_LISTBOX_DSPLY_GROUPS, wxDefaultPosition, wxDefaultSize, 0, itemListBox184Strings, wxLB_SINGLE );
    itemBoxSizer182->Add(itemListBox184, 1, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 3);

    wxFlexGridSizer* itemFlexGridSizer185 = new wxFlexGridSizer(3, 1, 0, 0);
    itemBoxSizer182->Add(itemFlexGridSizer185, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton186 = new ewxButton( itemFrame1, ID_BUTTON_DSPLY_UPDATE, _("Update"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer185->Add(itemButton186, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton187 = new ewxButton( itemFrame1, ID_BUTTON_DSPLY_ADD, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton187->SetDefault();
    itemFlexGridSizer185->Add(itemButton187, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton188 = new ewxButton( itemFrame1, ID_BUTTON_DSPLY_REMOVE, _("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer185->Add(itemButton188, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer189 = new wxBoxSizer(wxHORIZONTAL);
    p_mainSizer->Add(itemBoxSizer189, 0, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine190 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer189->Add(itemStaticLine190, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer191 = new wxBoxSizer(wxHORIZONTAL);
    p_mainSizer->Add(itemBoxSizer191, 0, wxGROW|wxALL, 0);

    ewxButton* itemButton192 = new ewxButton( itemFrame1, ID_BUTTON_DSPLY_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton192->SetDefault();
    itemBoxSizer191->Add(itemButton192, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer191->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton194 = new ewxButton( itemFrame1, ID_BUTTON_DSPLY_SAVE, _("Save"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton194->SetDefault();
    if (ShowToolTips())
        itemButton194->SetToolTip(_("Saves the detailed settings for this display style as a preference."));
    itemBoxSizer191->Add(itemButton194, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer191->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton196 = new ewxButton( itemFrame1, ID_BUTTON_DSPLY_RESET, _("Reset"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer191->Add(itemButton196, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer191->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton198 = new ewxButton( itemFrame1, ID_BUTTON_DSPLY_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer191->Add(itemButton198, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 3);

////@end VizStyleChooserGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_VIZSTYLECHOOSER
 */

void VizStyleChooserGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_VIZSTYLECHOOSER in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_VIZSTYLECHOOSER in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DSPLY_DSOPT
 */

void VizStyleChooserGUI::OnChoiceDsplyDsoptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DSPLY_DSOPT in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DSPLY_DSOPT in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DSPLY_CSOPT
 */

void VizStyleChooserGUI::OnChoiceDsplyCsoptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DSPLY_CSOPT in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_DSPLY_CSOPT in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_BLOCKEDIT
 */

void VizStyleChooserGUI::OnButtonDsplyBlockeditClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_BLOCKEDIT in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_BLOCKEDIT in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_DSPLY_SHOW
 */

void VizStyleChooserGUI::OnToggleDsplyShowClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_DSPLY_SHOW in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TOGGLE_DSPLY_SHOW in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_LNWDTH
 */

void VizStyleChooserGUI::OnSliderDsplyLnwdthUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_LNWDTH in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_LNWDTH in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLRQ
 */

void VizStyleChooserGUI::OnSliderDsplyCylrqUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLRQ in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLRQ in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLRAD
 */

void VizStyleChooserGUI::OnSliderDsplyCylradUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLRAD in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLRAD in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_RESRAD
 */

void VizStyleChooserGUI::OnSliderDsplyResradUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_RESRAD in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_RESRAD in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SPHRQ
 */

void VizStyleChooserGUI::OnSliderDsplySphrqUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SPHRQ in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SPHRQ in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SPHSCL
 */

void VizStyleChooserGUI::OnSliderDsplySphsclUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SPHSCL in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SPHSCL in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_COILW
 */

void VizStyleChooserGUI::OnSliderDsplyCoilwUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_COILW in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_COILW in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_HLXRAD
 */

void VizStyleChooserGUI::OnSliderDsplyHlxradUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_HLXRAD in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_HLXRAD in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_HLXW
 */

void VizStyleChooserGUI::OnSliderDsplyHlxwUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_HLXW in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_HLXW in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SHTW
 */

void VizStyleChooserGUI::OnSliderDsplyShtwUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SHTW in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_SHTW in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_RBTHK
 */

void VizStyleChooserGUI::OnSliderDsplyRbthkUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_RBTHK in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_RBTHK in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CCYLRQ
 */

void VizStyleChooserGUI::OnSliderDsplyCcylrqUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CCYLRQ in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CCYLRQ in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLR
 */

void VizStyleChooserGUI::OnSliderDsplyCylrUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLR in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_CYLR in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_TUBER
 */

void VizStyleChooserGUI::OnSliderDsplyTuberUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_TUBER in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SLIDER_UPDATED event handler for ID_SLIDER_DSPLY_TUBER in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_DSPLY_GROUPS
 */

void VizStyleChooserGUI::OnListboxDsplyGroupsSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_DSPLY_GROUPS in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_DSPLY_GROUPS in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_UPDATE
 */

void VizStyleChooserGUI::OnButtonDsplyUpdateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_UPDATE in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_UPDATE in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_ADD
 */

void VizStyleChooserGUI::OnButtonDsplyAddClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_ADD in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_ADD in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_REMOVE
 */

void VizStyleChooserGUI::OnButtonDsplyRemoveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_REMOVE in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_REMOVE in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_CLOSE
 */

void VizStyleChooserGUI::OnButtonDsplyCloseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_CLOSE in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_CLOSE in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_SAVE
 */

void VizStyleChooserGUI::OnButtonDsplySaveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_SAVE in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_SAVE in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_RESET
 */

void VizStyleChooserGUI::OnButtonDsplyResetClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_RESET in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_RESET in VizStyleChooserGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_HELP
 */

void VizStyleChooserGUI::OnButtonDsplyHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_HELP in VizStyleChooserGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DSPLY_HELP in VizStyleChooserGUI. 
}

/*!
 * Should we show tooltips?
 */

bool VizStyleChooserGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap VizStyleChooserGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin VizStyleChooserGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end VizStyleChooserGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon VizStyleChooserGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin VizStyleChooserGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end VizStyleChooserGUI icon retrieval
}
