#include <iostream>
using namespace std;

#include <wx/dialog.h>

#include "wxgui/ewxConfig.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxColorDialog.H"

#include "Plane.H"

static const char *INIFILE = "wxbuilder.ini";

IMPLEMENT_DYNAMIC_CLASS( Plane, PlaneGUI )

Plane::Plane( )
{
}

Plane::Plane( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

bool Plane::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{

    ewxDialog::Create( parent, id, caption, pos, size, style );
    CreateControls();
    GetSizer()->Add(CreateStdDialogButtonSizer(wxOK|wxCANCEL),1,wxALIGN_RIGHT);

    wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_PLANE_COLOR);
    if (btn) {
       ewxConfig *config = ewxConfig::getConfig(INIFILE);
       wxString color = config->Read("Plane/Color", "yellow");
       btn->SetBackgroundColour(wxColour(color));
    }

    generateMPName();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
    return true;
}

bool   Plane::isMillerPlane() const
{
   // Code relies on miller plane tab being the first one
   wxNotebook *nb = (wxNotebook*)FindWindow(ID_NOTEBOOK_PLANE);
   return nb->GetSelection() == 0;
}

void Plane::generateMPName()
{
   int h,k,l;
   ewxSpinCtrl *spin;
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PLANE_H);
   h = spin->GetValue();
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PLANE_K);
   k = spin->GetValue();
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PLANE_L);
   l = spin->GetValue();

   char buf[32];
   sprintf(buf,"(%d,%d,%d)",h,k,l);
   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLANE_NAME);
   txt->SetValue(buf);
   
}

void   Plane::getMillerIndices(int& h, int& k, int& l)
{
   ewxSpinCtrl *spin;
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PLANE_H);
   h = spin->GetValue();
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PLANE_K);
   k = spin->GetValue();
   spin = (ewxSpinCtrl*)FindWindow(ID_SPINCTRL_PLANE_L);
   l = spin->GetValue();
}

bool   Plane::isTransparent() const
{
   ewxCheckBox *tgl = (ewxCheckBox*)FindWindow(ID_CHECKBOX_PLANE_TRANSPARENT);
   return tgl->IsChecked();
}

string Plane::getName() const
{
   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_PLANE_NAME);
   return txt->GetValue().c_str();
}

wxColour Plane::getColor() const
{
   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_PLANE_COLOR);
   return btn->GetBackgroundColour();
}


void Plane::OnTextctrlEnter( wxCommandEvent& event )
{
    event.Skip();
}

void Plane::OnButtonClick( wxCommandEvent& event )
{
   // TODO fix me. Can't do modal within modal!!!
   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_PLANE_COLOR);
   wxColour color = btn->GetBackgroundColour();

   wxPoint pos = wxGetMousePosition();
   ewxColorDialog dlg(color, this, wxID_ANY, "ECCE Plane Color", pos);
   if (dlg.ShowModal() == wxID_OK) {
      wxString bgcolor = dlg.GetColor().GetAsString(wxC2S_HTML_SYNTAX);


      btn->SetBackgroundColour(bgcolor);

      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write("Plane/Color", bgcolor);

   }
   event.Skip();
}

void Plane::OnCheckboxPlaneTransparentClick( wxCommandEvent& event )
{
    event.Skip();
}

void Plane::OnSpinctrlPlaneHUpdated( wxSpinEvent& event )
{
    generateMPName();
    event.Skip();
}

void Plane::OnSpinctrlPlaneKUpdated( wxSpinEvent& event )
{
    generateMPName();
    event.Skip();
}

void Plane::OnSpinctrlPlaneLUpdated( wxSpinEvent& event )
{
    generateMPName();
    event.Skip();
}

