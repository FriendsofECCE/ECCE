#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <algorithm>
//  using std::copy;
  using std::fill;
#include <limits>
#include <cmath>

#include <wx/link.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/tglbtn.h> // EVT_TOGGLEBUTTON
#include <wx/grid.h>
#include <wx/slider.h>

#include "util/InternalException.H"
#include "util/EventDispatcher.H"
#include "util/PreferenceLabels.H"

#include "tdat/PropTable.H"
#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include "tdat/TProperty.H"

#include "dsm/IPropCalculation.H"
#include "dsm/ICalculation.H"
#include "dsm/PropFactory.H"

#include "wxgui/ewxColorDialog.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxNumericValidator.H"

#include "wxgui/ewxColor.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/TreeListCtrl.H"
#include "wxgui/WindowEvent.H"
#include "wxgui/WxColorSchemeButton.H"
#include "wxgui/ewxGrid.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/SliderCombo.H"

#include "inv/ChemKit/ChemColor.H"

#include "viz/AddDipoleMomentCmd.H"
#include "viz/SGFragment.H"
#include "tdat/TAtm.H"
#include "viz/cvSGContainer.H"
#include "tdat/PropString.H"

#include "wxviz/WxVizToolFW.H"
#include "wxviz/SGSelection.H"

#include "MomentsPanel.H"

  using std::find;


static const char *INIFILE = "wxbuilder.ini";


// ----------------------------------------------------------------------------


const wxWindowID MomentsPanel::ID_DIALOG_MOMENTS = wxNewId();
const wxWindowID MomentsPanel::ID_MOMENTS_SLIDER = wxNewId();
const wxWindowID MomentsPanel::ID_BUTTON_MOMENTS_VECCOLOR = wxNewId();
const wxWindowID MomentsPanel::ID_MOMENTS_GRID = wxNewId();
static const wxWindowID ID_BOND_CHECK = wxNewId();
static const wxWindowID ID_BOND_CHOICE = wxNewId();


BEGIN_EVENT_TABLE( MomentsPanel, VizPropertyPanel )
    //EVT_GRID_SELECT_CELL(MomentsPanel::OnModeSelection)
    EVT_BUTTON( ID_BUTTON_MOMENTS_VECCOLOR, MomentsPanel::OnVecColorClick )
    EVT_CHECKBOX( ID_BOND_CHECK, MomentsPanel::OnBondToggle )
    EVT_CHOICE( ID_BOND_CHOICE, MomentsPanel::OnBondChoice )
    EVT_SCROLL(MomentsPanel::OnEndSliderMotion)
    EVT_TEXT_ENTER(ID_MOMENTS_SLIDER, MomentsPanel::OnSliderTextEnter)
END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(MomentsPanel, VizPropertyPanel)



MomentsPanel::MomentsPanel()
  : VizPropertyPanel(),
    p_grid(NULL),
    p_slider(NULL),
    p_guessScale(true)
{
}



// An ion's dipole depends on the origin; say which one the code used.
static wxString dipoleOriginText(IPropCalculation *expt)
{
   PropString *org = dynamic_cast<PropString*>(expt->getProperty("DIPOLEORIGIN"));
   if (org && org->value() == "centre_of_mass")
      return wxString::FromUTF8(" (about the centre of mass)");
   if (org && org->value() == "centre_of_charge")
      return wxString::FromUTF8(" (about the centre of nuclear charge)");
   if (org && org->value() == "coordinate_origin")
      return wxString::FromUTF8(" (about the coordinate origin)");
   return wxString();
}

MomentsPanel::MomentsPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : VizPropertyPanel(),
    p_grid(NULL),
    p_slider(NULL),
    p_guessScale(true)
{
   Create(calculation, parent, id, pos, size, style, name);
}



MomentsPanel::~MomentsPanel()
{
}



bool MomentsPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
   if (!VizPropertyPanel::Create(calculation, parent, id, pos, size, style )) {
      wxFAIL_MSG( wxT("VizPropertyPanel creation failed") );
      return false;
   }

   wxPanel *panel = this;

   this->SetSizer(new wxBoxSizer(wxVERTICAL));

   wxBoxSizer* tableSizer = new wxBoxSizer(wxHORIZONTAL);
   panel->GetSizer()->Add(tableSizer, isDipole() ? 0 : 1, wxGROW|wxALL, 0);
   p_grid = new ewxGrid( panel, ID_MOMENTS_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
   p_grid->SetRowLabelSize(0);
   p_grid->SetMinSize(wxSize(200, isDipole() ? 60 : 175));
   p_grid->CreateGrid(3, 3, wxGrid::wxGridSelectRows);
   tableSizer->Add(p_grid, 1, wxGROW|wxALL, 3);

   // Quadrupole, hyperpolarizability and stress panels show just the table:
   // an arrow, its colour and its scale mean nothing there.
   if (drawsOverlay()) {
      wxBoxSizer *optionsSizer = new wxBoxSizer(wxHORIZONTAL);
      panel->GetSizer()->Add(optionsSizer, 0, wxALIGN_LEFT|wxALL, 0);

       // A button to control color (the multipole surfaces use a fixed
       // pair, blue for positive and vermillion for negative)
       ewxButton* colorbtn = NULL;
       ewxStaticText* label = NULL;
       if (!isMultipole()) {
          label = new ewxStaticText( panel, wxID_STATIC, _("Tensor Color:"), wxDefaultPosition, wxDefaultSize, 0 );
          optionsSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
          colorbtn = new ewxButton( panel, ID_BUTTON_MOMENTS_VECCOLOR, _T(""), wxDefaultPosition, wxSize(24, 24), wxBU_EXACTFIT );
          optionsSizer->Add(colorbtn, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
       }

       optionsSizer->AddSpacer(3);
       // The dipole arrow runs between the charge poles: no scale.
       if (!isDipole()) {
       label = new ewxStaticText( panel, wxID_STATIC,
             isPolarizability() ?
                wxString::FromUTF8("Size, \u00c5 per 10 a.u.:") :
             isMultipole() ? wxString::FromUTF8("Largest lobe, \u00c5:") :
             _("Scale:"),
             wxDefaultPosition, wxDefaultSize, 0 );
       optionsSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
       }

      // Initilialize button color
      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      wxString color = config->Read(getConfigKey().c_str(),
            isPolarizability() ? "#56b4e9" : "yellow");
      if (colorbtn) colorbtn->SetBackgroundColour(wxColour(color));

      // Add the slider
      if (!isDipole()) {
      ewxPanel *p = new ewxPanel(this);
      optionsSizer->Add(p, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
      p_slider = new SliderCombo(p, ID_MOMENTS_SLIDER);
      if (isPolarizability()) {
         p_slider->SetRange(0.01, 20.0);
         p_slider->SetToolTip(wxString::FromUTF8(
               "Semi-axis length in \u00c5 per 10 a.u. of polarizability"));
      } else if (isMultipole()) {
         p_slider->SetRange(0.1, 30.0);
         p_slider->SetToolTip(wxString::FromUTF8(
               "Distance from the origin to the tip of the largest lobe, "
               "\u00c5"));
      } else {
         p_slider->SetRange(0.1,10000.0);
         p_slider->SetToolTip("Scale");
      }
      }
   }

   fillTable();

   if (isDipole()) {
      PropVector *vec = (PropVector*)
         calculation->getProperty(*getPropertyNames().begin());
      double mu = 0.0;
      for (int i = 0; vec && i < 3 && i < (int)vec->rows(); i++)
         mu += vec->value(i)*vec->value(i);
      string key = AddDipoleMomentCmd::chargeKey(calculation);
      wxString from = key.empty() ?
         wxString::FromUTF8("The calculation has no atomic charges: the "
            "arrow is 1 \u00c5 long, through the centre of nuclear charge.") :
         wxString::Format(wxString::FromUTF8("The arrow runs from the "
            "negative to the positive pole along \u03bc. Each pole carries "
            "the total positive %s charge, and they are as far apart as that "
            "charge must be to give \u03bc."),
            AddDipoleMomentCmd::chargeName(key));
      wxString about = dipoleOriginText(calculation);
      wxString note = wxString::Format(wxString::FromUTF8(
         "|\u03bc| = %.3f D%s. %s"), sqrt(mu), about, from);
      p_dipoleNote = new ewxStaticText(panel, wxID_STATIC, note);
      p_dipoleNote->Wrap(260);
      GetSizer()->Add(p_dipoleNote, 0, wxALIGN_LEFT|wxALL, 3);

      p_bondKeys = AddDipoleMomentCmd::bondChargeKeys(calculation);
      if (!p_bondKeys.empty() && drawsOverlay()) {
         wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
         p_bondCheck = new ewxCheckBox(panel, ID_BOND_CHECK, _("Bond dipoles"));
         p_bondCheck->SetValue(false);
         p_bondCheck->SetToolTip(_("Split the point-charge dipole into one "
            "dipole per bond, from the chosen atomic charges"));
         row->Add(p_bondCheck, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
         p_bondChoice = new ewxChoice(panel, ID_BOND_CHOICE);
         int sel = 0;
         for (size_t i = 0; i < p_bondKeys.size(); i++) {
            p_bondChoice->Append(wxString::FromUTF8(
               AddDipoleMomentCmd::bondChargeName(p_bondKeys[i])));
            if (p_bondKeys[i] == key) sel = (int)i;
         }
         p_bondChoice->SetSelection(sel);
         p_bondChoice->Enable(false);
         row->Add(p_bondChoice, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
         GetSizer()->Add(row, 0, wxALIGN_LEFT|wxALL, 0);
         p_bondNote = new ewxStaticText(panel, wxID_STATIC, wxString());
         p_bondGrid = new ewxGrid(panel, wxID_ANY, wxDefaultPosition,
               wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL);
         p_bondGrid->SetRowLabelSize(0);
         p_bondGrid->SetMinSize(wxSize(200, 120));
         p_bondGrid->CreateGrid(0, 3, wxGrid::wxGridSelectRows);
         p_bondNote->Hide();
         p_bondGrid->Hide();
         GetSizer()->Add(p_bondNote, 0, wxALIGN_LEFT|wxALL, 3);
         GetSizer()->Add(p_bondGrid, 1, wxGROW|wxALL, 3);
      }
   }

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);

   return true;
}


void MomentsPanel::OnBondToggle(wxCommandEvent& event)
{
   setBondDipoles(p_bondCheck->GetValue());
}


void MomentsPanel::OnBondChoice(wxCommandEvent& event)
{
   setBondDipoles(p_bondCheck->GetValue());
}


void MomentsPanel::setBondDipoles(bool on, const string& key)
{
   if (!p_bondCheck) return;
   p_bondCheck->SetValue(on);
   p_bondChoice->Enable(on);
   for (size_t i = 0; i < p_bondKeys.size(); i++)
      if (p_bondKeys[i] == key) p_bondChoice->SetSelection((int)i);
   p_bondNote->Show(on);
   p_bondGrid->Show(on);
   // The pane is short: the table takes the note's room while it is shown.
   if (p_dipoleNote) p_dipoleNote->Show(!on);
   showVectors(false);
   fillBondTable();
   GetSizer()->SetSizeHints(this);
   Layout();
}


// Direction as a unit vector; the sign is the arrow's, negative to positive.
static wxString directionText(const double v[3])
{
   double n = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
   if (n < 1e-9) return wxString("-");
   return wxString::Format("%+.2f %+.2f %+.2f", v[0]/n, v[1]/n, v[2]/n);
}


void MomentsPanel::fillBondTable()
{
   p_bondRows.clear();
   if (!p_bondGrid) return;
   if (p_bondGrid->GetNumberRows() > 0)
      p_bondGrid->DeleteRows(0, p_bondGrid->GetNumberRows());
   p_bondGrid->SetColLabelValue(0, "Bond");
   p_bondGrid->SetColLabelValue(1, wxString::FromUTF8("|\u03bc| / D"));
   p_bondGrid->SetColLabelValue(2, "Direction (- to +)");
   if (!p_bondCheck->GetValue()) return;

   IPropCalculation *expt = getCalculation();
   SGFragment *frag = getFW().getSceneGraph().getFragment();
   const string key = p_bondKeys[p_bondChoice->GetSelection()];
   vector<std::pair<int,int> > bonds;
   BondDipoles::Result r;
   if (!AddDipoleMomentCmd::bondDipoles(expt, frag, key, bonds, r)) {
      p_bondNote->SetLabel(_("No bonds or charges to split."));
      return;
   }
   p_bondNote->SetLabel(wxString::Format(wxString::FromUTF8(
      "Bond dipoles from %s charges. The point-charge sum is taken about "
      "the centre of mass."), wxString::FromUTF8(
      AddDipoleMomentCmd::bondChargeName(key))));
   p_bondNote->Wrap(300);

   auto add = [&](const string& name, const double v[3], bool direction) {
      double n = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]) *
         BondDipoles::DEBYE_PER_E_ANGSTROM;
      vector<string> row;
      row.push_back(name);
      row.push_back(wxString::Format("%.3f", n).ToStdString());
      row.push_back(direction ? directionText(v).ToStdString() : "");
      p_bondRows.push_back(row);
   };
   for (size_t b = 0; b < bonds.size(); b++) {
      double v[3] = { r.mu[0][b], r.mu[1][b], r.mu[2][b] };
      TAtm *a = frag->atomRef(bonds[b].first), *z = frag->atomRef(bonds[b].second);
      add(a->atomicSymbol() + std::to_string(a->index() + 1) + " -> " +
          z->atomicSymbol() + std::to_string(z->index() + 1), v, true);
   }
   double net = 0.0, mu[3] = {0, 0, 0}, rem[3];
   for (int k = 0; k < 3; k++) net += fabs(r.netChargeTerm[k]);
   add("Sum of bonds", r.bondSum, true);
   if (net > 1e-6) add("Net-charge term", r.netChargeTerm, true);
   add("Point charges", r.pointCharge, true);
   PropVector *vec = (PropVector*) expt->getProperty(
      *getPropertyNames().begin());
   for (int k = 0; vec && k < 3 && k < (int)vec->rows(); k++)
      mu[k] = vec->value(k) / BondDipoles::DEBYE_PER_E_ANGSTROM;
   add("Computed", mu, true);
   for (int k = 0; k < 3; k++) rem[k] = mu[k] - r.pointCharge[k];
   add("Remainder (computed - point charges)", rem, true);

   p_bondGrid->AppendRows((int)p_bondRows.size());
   for (size_t i = 0; i < p_bondRows.size(); i++)
      for (int c = 0; c < 3; c++)
         p_bondGrid->SetCellValue((int)i, c,
            wxString::FromUTF8(p_bondRows[i][c]));
   p_bondGrid->AutoSize();
}


bool MomentsPanel::isDipole()
{
  if (GetClassInfo() != CLASSINFO(MomentsPanel)) return false;
  set<string> keys = getPropertyNames();
  return keys.size() > 0 &&
     (*keys.begin() == "DIPOLE" || *keys.begin() == "MP2DIPOLE");
}


string MomentsPanel::getConfigKey()
{
  string ret;
  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
     ret = "Moments/" + *keys.begin();
  }
  return ret;
}




/**
 * Fill the table and initialize the UI.
 */
void MomentsPanel::fillTable()
{

   IPropCalculation *expt = getCalculation();

   // Clear table
   p_grid->ClearGrid();
   p_grid->DeleteCols(0,p_grid->GetNumberCols());
   p_grid->DeleteRows(0,p_grid->GetNumberRows());

  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
     string key = *keys.begin();
     PropVector *vec = (PropVector*) expt->getProperty(key);
     if (vec) {

        // TODO the label and scalarize value

        // Note:  Could be taken from the rowLabels array but this
        // seems easier...  If modified, be sure to handle the case of
        // missing row labels.
        //const vector<string> *rowLabels = vec->rowLabels();
        p_grid->InsertCols(0,3);
        p_grid->SetColLabelValue(0,"X");
        p_grid->SetColLabelValue(1,"Y");
        p_grid->SetColLabelValue(2,"Z");


        if (isDipole()) {
           p_grid->SetColLabelValue(0,"X / D");
           p_grid->SetColLabelValue(1,"Y / D");
           p_grid->SetColLabelValue(2,"Z / D");
        }

        int nRows = vec->rows();
        p_grid->InsertRows(0,nRows/3);
        wxString cellv;
        for (int idx=0; idx<nRows; idx++) {
           cellv = wxString::Format (PrefLabels::DOUBLEFORMAT, vec->value(idx));
           cellv.Replace(" ","",true);
           p_grid->SetCellValue((idx/3),idx%3,cellv);
        }
     }

  }

   p_grid->AutoSize();
}



void MomentsPanel::OnEndSliderMotion(wxScrollEvent& event)
{
#ifndef INSTALL
   IPropCalculation *expt = getCalculation();
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif

   showVectors(false);
   event.Skip();
}



void MomentsPanel::OnSliderTextEnter(wxCommandEvent& event)
{
   showVectors(false);
   event.Skip();
}



void MomentsPanel::receiveFocus()
{
   //selectFragStep(-1);
   showVectors(p_guessScale);
   p_guessScale = false;

   SGContainer& sg = getFW().getSceneGraph();
   SoSwitch *pswitch = sg.getPropertySwitch(*(getPropertyNames().begin()));
   pswitch->whichChild.setValue(SO_SWITCH_ALL);
}

void MomentsPanel::loseFocus()
{
   SGContainer& sg = getFW().getSceneGraph();
   SoSwitch *pswitch = sg.getPropertySwitch(*(getPropertyNames().begin()));
   pswitch->whichChild.setValue(SO_SWITCH_NONE);
   if (isDipole()) getFW().getViewer().setURightText("");
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
}


void MomentsPanel::OnVecColorClick( wxCommandEvent& event )
{
   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_MOMENTS_VECCOLOR);
   wxColour color = btn->GetBackgroundColour();

   wxPoint pos = wxGetMousePosition();
   ewxColorDialog dlg(color, this, wxID_ANY, "ECCE Vector Color", pos);
   if (dlg.ShowModal() == wxID_OK) {
      wxString bgcolor = dlg.GetColor().GetAsString(wxC2S_HTML_SYNTAX);


      btn->SetBackgroundColour(bgcolor);

      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write(getConfigKey().c_str(), bgcolor);

      showVectors(false);

   }

   event.Skip();
}




void MomentsPanel::showVectors(bool guessScale)
{
   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
#ifndef INSTALL
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif
   SGContainer& sg = fw.getSceneGraph();

   ewxButton* btn = (ewxButton*)FindWindow(ID_BUTTON_MOMENTS_VECCOLOR);
   wxColour color = btn->GetBackgroundColour();

   // Only the neutral color is used for this vector...
   Command *cmd = new AddDipoleMomentCmd("Dipole",&sg, expt);
   set<string> props = getPropertyNames();
   string pname = *(props.begin());
   cmd->getParameter("property")->setString(pname);
   cmd->getParameter("neutral")->setString(color.GetAsString(wxC2S_HTML_SYNTAX).ToStdString());
   if (p_bondCheck && p_bondCheck->GetValue())
      cmd->getParameter("BondCharges")->setString(
         p_bondKeys[p_bondChoice->GetSelection()]);
   if (p_slider) cmd->getParameter("Amplitude")->setDouble(p_slider->GetFloatValue());
   if (guessScale && p_slider) {
      cmd->getParameter("newData")->setBoolean(guessScale);
   }

   fw.execute(cmd);

   if (isDipole()) {
      PropVector *mu = (PropVector*) expt->getProperty(pname);
      double m2 = 0.0;
      for (int i = 0; mu && i < 3 && i < (int)mu->rows(); i++)
         m2 += mu->value(i)*mu->value(i);
      fw.getViewer().setURightText(AddDipoleMomentCmd::label(sqrt(m2),
            AddDipoleMomentCmd::chargeKey(expt),
            cmd->getParameter("ChargesOppose")->getBoolean()));
      const double q = cmd->getParameter("PoleCharge")->getDouble();
      if (p_dipoleNote && q > 0.0) {
         p_dipoleNote->SetLabel(wxString::Format(wxString::FromUTF8(
            "|\u03bc| = %.3f D%s. The arrow runs from the negative to the "
            "positive pole along \u03bc. The poles carry \u00b1%.2f e (the "
            "positive %s charge) and are %.2f \u00c5 apart, as far as that "
            "charge must be to give \u03bc."), sqrt(m2), dipoleOriginText(expt), q,
            AddDipoleMomentCmd::chargeName(AddDipoleMomentCmd::chargeKey(expt)),
            cmd->getParameter("PoleSeparation")->getDouble()));
         p_dipoleNote->Wrap(260);
         Layout();
      }
   }

   if (guessScale && p_slider) {
      p_slider->SetValue(cmd->getParameter("Amplitude")->getDouble());
   }
}
