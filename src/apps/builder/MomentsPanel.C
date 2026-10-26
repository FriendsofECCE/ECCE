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


BEGIN_EVENT_TABLE( MomentsPanel, VizPropertyPanel )
    //EVT_GRID_SELECT_CELL(MomentsPanel::OnModeSelection)
    EVT_BUTTON( ID_BUTTON_MOMENTS_VECCOLOR, MomentsPanel::OnVecColorClick )
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
   panel->GetSizer()->Add(tableSizer, 1, wxGROW|wxALL, 0);
   p_grid = new ewxGrid( panel, ID_MOMENTS_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
   p_grid->SetRowLabelSize(0);
   p_grid->SetMinSize(wxSize(200,175));
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
         wxString::Format(wxString::FromUTF8("The poles are the centres of "
            "negative and positive %s charge."),
            AddDipoleMomentCmd::chargeName(key));
      wxString note = wxString::Format(wxString::FromUTF8(
         "|\u03bc| = %.3f D. The arrow runs from the negative to the "
         "positive pole, along \u03bc; its length follows the geometry, "
         "not |\u03bc|. %s"), sqrt(mu), from);
      ewxStaticText *text = new ewxStaticText(panel, wxID_STATIC, note);
      text->Wrap(260);
      GetSizer()->Add(text, 0, wxALIGN_LEFT|wxALL, 3);
   }

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);

   return true;
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
   if (p_slider) cmd->getParameter("Amplitude")->setDouble(p_slider->GetFloatValue());
   if (guessScale && p_slider) {
      cmd->getParameter("newData")->setBoolean(guessScale);
   }

   fw.execute(cmd);

   if (guessScale && p_slider) {
      p_slider->SetValue(cmd->getParameter("Amplitude")->getDouble());
   }
}
