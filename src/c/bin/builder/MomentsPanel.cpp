#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <algorithm>
//  using std::copy;
  using std::fill;
#include <limits>

#include <wx/link.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/tglbtn.h> // EVT_TOGGLEBUTTON
#include <wx/grid.h>
#include <wx/slider.h>

#include "util/InternalException.hpp"
#include "util/EventDispatcher.hpp"
#include "util/PreferenceLabels.hpp"

#include "tdat/PropTable.hpp"
#include "tdat/PropVector.hpp"
#include "tdat/PropVecString.hpp"
#include "tdat/TProperty.hpp"

#include "dsm/IPropCalculation.hpp"
#include "dsm/ICalculation.hpp"
#include "dsm/PropFactory.hpp"

#include "wxgui/ewxColorDialog.hpp"
#include "wxgui/ewxBitmap.hpp"
#include "wxgui/ewxConfig.hpp"
#include "wxgui/ewxButton.hpp"
#include "wxgui/ewxCheckBox.hpp"
#include "wxgui/ewxChoice.hpp"
#include "wxgui/ewxTextCtrl.hpp"
#include "wxgui/ewxNumericValidator.hpp"

#include "wxgui/ewxColor.hpp"
#include "wxgui/ewxNonBoldLabel.hpp"
#include "wxgui/ewxStaticText.hpp"
#include "wxgui/ewxTextCtrl.hpp"
#include "wxgui/ewxWindowUtils.hpp"
#include "wxgui/TreeListCtrl.hpp"
#include "wxgui/WindowEvent.hpp"
#include "wxgui/WxColorSchemeButton.hpp"
#include "wxgui/ewxGrid.hpp"
#include "wxgui/ewxProgressDialog.hpp"
#include "wxgui/ewxPlotCtrl.hpp"
#include "wxgui/SliderCombo.hpp"

#include "inv/ChemKit/ChemColor.hpp"

#include "viz/AddDipoleMomentCmd.hpp"

#include "wxviz/WxVizToolFW.hpp"
#include "wxviz/SGSelection.hpp"

#include "MomentsPanel.hpp"

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

   wxBoxSizer *optionsSizer = new wxBoxSizer(wxHORIZONTAL);
   panel->GetSizer()->Add(optionsSizer, 0, wxALIGN_LEFT|wxALL, 0);

    // A button to control color
    optionsSizer->AddSpacer(3);
    ewxStaticText* label = new ewxStaticText( panel, wxID_STATIC, _("Tensor Color:"), wxDefaultPosition, wxDefaultSize, 0 );
    optionsSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);
    ewxButton* colorbtn = new ewxButton( panel, ID_BUTTON_MOMENTS_VECCOLOR, _T(""), wxDefaultPosition, wxSize(24, 24), wxBU_EXACTFIT );
    optionsSizer->Add(colorbtn, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    optionsSizer->AddSpacer(3);
    label = new ewxStaticText( panel, wxID_STATIC, _("Scale:"), wxDefaultPosition, wxDefaultSize, 0 );
    optionsSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

   // Initilialize button color
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   wxString color = config->Read(getConfigKey().c_str(), "yellow");
   colorbtn->SetBackgroundColour(wxColour(color));

   // Add the slider
   ewxPanel *p = new ewxPanel(this);
   optionsSizer->Add(p, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
   p_slider = new SliderCombo(p, ID_MOMENTS_SLIDER);
   p_slider->SetRange(0.1,10000.0);
   p_slider->SetToolTip("Scale");

   fillTable();

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);

   return true;
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


        int nRows = vec->rows();
        p_grid->InsertRows(0,nRows/3);
        wxString cellv;
        for (int idx=0; idx<nRows; idx++) {
           cellv = wxString::Format (_T(PrefLabels::DOUBLEFORMAT), vec->value(idx));
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
   cmd->getParameter("neutral")->setString(color.GetAsString(wxC2S_HTML_SYNTAX).c_str());
   cmd->getParameter("Amplitude")->setDouble(p_slider->GetFloatValue());
   if (guessScale) {
      cmd->getParameter("newData")->setBoolean(guessScale);
   }

   fw.execute(cmd);

   if (guessScale) {
      p_slider->SetValue(cmd->getParameter("Amplitude")->getDouble());
   }
}
