
// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_TEXTDLG


#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/dialog.h"
    #include "wx/button.h"
    #include "wx/stattext.h"
    #include "wx/textctrl.h"
    #include "wx/intl.h"
    #include "wx/sizer.h"
#endif

#if wxUSE_STATLINE
    #include "wx/statline.h"
#endif

#include "util/StringConverter.H"

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/WxMeasurePrompt.H"

static const int wxID_TEXT = 106800;
static const int wxID_TGL =  106801;


BEGIN_EVENT_TABLE(WxMeasurePrompt, wxDialog)
    EVT_BUTTON(wxID_OK, WxMeasurePrompt::OnOK)
END_EVENT_TABLE()

IMPLEMENT_CLASS(WxMeasurePrompt, wxDialog)

WxMeasurePrompt::WxMeasurePrompt(wxWindow *parent,
                                     MeasureType type,
                                     double value,
                                     long style,
                                     const wxPoint& pos)
                 : wxDialog(parent, wxID_ANY, "", pos, wxDefaultSize,
                            wxDEFAULT_DIALOG_STYLE),
                   ewxStyledWindow(),
                   m_value(value)
{
    m_dialogStyle = style;


//    wxBeginBusyCursor();

    // Set some things based on the type
    wxString message,tglmsg, tip;
    double min, max;
    if (type == DISTANCE) {
       SetTitle("Edit Distance");
       message = "Enter new distance.  Second atom selected will move.";
       tglmsg = "Move all atoms connected to the second atom.";
       tip = "if set, all atoms connected to the third atom will rotate.\n"
             "Otherwise, only the third atom selected will be rotated.";
       min = 0.0001;
       max = 100000.;

    } else if (type == ANGLE) {
       SetTitle("Edit Angle");
       message = "Enter new angle.  Third atom selected will move.";
       tglmsg = "Rotate all atoms connected to the third atom.";
       tip = "if set, all atoms connected to the second atom will move.\n"
             "Otherwise, only the second atom selected will be moved.";
       min = 0.0;
       max = 360.0;

    } else if (type == TORSION) {
       SetTitle("Edit Torsion");
       message = "Enter new torsion.  Fourth atom selected will move.";
       tglmsg = "Rotate group about bond.";
       tip = "if set, all atoms connected to the central bond will rotate.\n"
             "Otherwise, only atoms connected the the last atom selected will be rotated.";
       // Bruce says to be loose with the validation
       // though typical values will be -180 to 180
       min = -360.0;
       max = 360.0;
    }


    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );

    wxSizerFlags flagsBorder2;
    flagsBorder2.DoubleBorder();

#if wxUSE_STATTEXT
    // 1) text message
    topsizer->Add(CreateTextSizer(message), flagsBorder2);
#endif

    // 2) text ctrl
    m_textctrl = new ewxTextCtrl(this, wxID_TEXT, "",
                                wxDefaultPosition, wxSize(300, wxDefaultCoord),
                                style & ~WxMeasurePromptStyle);

    SetValue(value);
    SetToolTip(tip);
    topsizer->Add(m_textctrl,
                  wxSizerFlags(style & wxTE_MULTILINE ? 1 : 0).
                    Expand().
                    TripleBorder(wxLEFT | wxRIGHT));

    // @todo customizer for type
    ewxNumericValidator v(min, max, value);
    v.setValue(value);
    SetValidator(v);

    // Toggle
    wxBoxSizer* tglsizer = new wxBoxSizer(wxHORIZONTAL);
    p_tgl  = new ewxCheckBox( this, -1, tglmsg, wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    p_tgl->SetValue(true);
    tglsizer->Add(15, 6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0); // spacer
    tglsizer->Add(p_tgl, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    topsizer->Add(tglsizer, 0, wxGROW|wxALL, 0);



    // 3) buttons if any
    wxSizer *buttonSizer = CreateSeparatedButtonSizer(style & ButtonSizerFlags);
    if ( buttonSizer )
    {
        topsizer->Add(buttonSizer, wxSizerFlags(flagsBorder2).Expand());
    }


    SetAutoLayout( true );
    SetSizer( topsizer );

    topsizer->SetSizeHints( this );
    topsizer->Fit( this );

    if ( style & wxCENTRE )
        Centre( wxBOTH );

    m_textctrl->SetSelection(-1, -1);
    m_textctrl->SetFocus();

    setStyles(this, true);
//    wxEndBusyCursor();
}


double WxMeasurePrompt::GetValue() const
{
   return m_value;
}

void WxMeasurePrompt::OnOK(wxCommandEvent& WXUNUSED(event) )
{
#if wxUSE_VALIDATORS
    if( Validate() && TransferDataFromWindow() )
    {
        // KLS need to set m_value all the time because GetValue returns it
        // regardless of whether or not wxUSE_VALIDATORS is defined
        StringConverter::toDouble(m_textctrl->GetValue().c_str(),m_value);
        EndModal( wxID_OK );
    }
#else
    StringConverter::toDouble(m_textctrl->GetValue().c_str(),m_value);

    EndModal(wxID_OK);
#endif
  // wxUSE_VALIDATORS
}

void WxMeasurePrompt::SetValue(double val)
{
    m_value = val;

    m_textctrl->SetValue(StringConverter::toString(val));
}

wxValidator* WxMeasurePrompt::GetValidator()
{
   return (wxValidator*)m_textctrl->GetValidator();
}

#if wxUSE_VALIDATORS

void WxMeasurePrompt::SetValidator( const wxValidator& validator )
{
    m_textctrl->SetValidator( validator );
}

#endif
  // wxUSE_VALIDATORS


void WxMeasurePrompt::setToggle(bool value)
{
    p_tgl->SetValue(true);
}

bool WxMeasurePrompt::isToggleSet() const
{
   return p_tgl->GetValue();
}


#endif 
