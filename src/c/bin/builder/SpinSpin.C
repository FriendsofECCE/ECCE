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
#include "wxgui/ewxPlotCtrl.H"
#include "wxgui/SliderCombo.H"

#include "inv/ChemKit/ChemColor.H"

#include "viz/SpinSpinTensorCmd.H"

#include "wxviz/WxVizToolFW.H"
#include "wxviz/SGSelection.H"

#include "SpinSpin.H"

  using std::find;


static const char *INIFILE = "wxbuilder.ini";


// ----------------------------------------------------------------------------


const wxWindowID SpinSpin::ID_DIALOG_SPINSPIN = wxNewId();
const wxWindowID SpinSpin::ID_SPINSPIN_SLIDER = wxNewId();
const wxWindowID SpinSpin::ID_BUTTON_SPINSPIN_POSCOLOR = wxNewId();
const wxWindowID SpinSpin::ID_BUTTON_SPINSPIN_NEGCOLOR = wxNewId();
const wxWindowID SpinSpin::ID_SPINSPIN_GRID = wxNewId();


BEGIN_EVENT_TABLE( SpinSpin, VizPropertyPanel )
   // EVT_GRID_SELECT_CELL(SpinSpin::OnRowSelection)
    EVT_GRID_RANGE_SELECT(SpinSpin::OnGridRangeSelect)
    EVT_BUTTON( ID_BUTTON_SPINSPIN_POSCOLOR, SpinSpin::OnPosColorClick )
    EVT_BUTTON( ID_BUTTON_SPINSPIN_NEGCOLOR, SpinSpin::OnNegColorClick )
    EVT_SCROLL(SpinSpin::OnEndSliderMotion)
    EVT_TEXT_ENTER(ID_SPINSPIN_SLIDER, SpinSpin::OnSliderTextEnter)
END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(SpinSpin, VizPropertyPanel)



SpinSpin::SpinSpin()
  : VizPropertyPanel(),
    p_grid(NULL),
    p_slider(NULL),
    p_guessScale(true)
{
}



SpinSpin::SpinSpin(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : VizPropertyPanel(),
    p_grid(NULL),
    p_slider(NULL),
    p_guessScale(true)
{
   Create(calculation, parent, id, pos, size, style, name);
}



SpinSpin::~SpinSpin()
{
}



bool SpinSpin::Create(IPropCalculation *calculation,
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
   p_grid = new ewxGrid( panel, ID_SPINSPIN_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
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
    ewxButton* pcolorbtn = new ewxButton( panel, ID_BUTTON_SPINSPIN_POSCOLOR, _T("+"), wxDefaultPosition, wxSize(24, 24), wxBU_EXACTFIT );
    optionsSizer->Add(pcolorbtn, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    ewxButton *ncolorbtn = new ewxButton( panel, ID_BUTTON_SPINSPIN_NEGCOLOR, _T("-"), wxDefaultPosition, wxSize(24, 24), wxBU_EXACTFIT );
    optionsSizer->Add(ncolorbtn, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    optionsSizer->AddSpacer(3);
    label = new ewxStaticText( panel, wxID_STATIC, _("Scale:"), wxDefaultPosition, wxDefaultSize, 0 );
    optionsSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

   // Initilialize button color
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   wxString color = config->Read((getConfigKey()+"Pos").c_str(), "red");
   pcolorbtn->SetBackgroundColour(wxColour(color));
   pcolorbtn->Refresh();
   color = config->Read((getConfigKey()+"Neg").c_str(), "blue");
   ncolorbtn->SetBackgroundColour(wxColour(color));
   ncolorbtn->Refresh();


   // Add the slider
   ewxPanel *p = new ewxPanel(this);
   optionsSizer->Add(p, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
   p_slider = new SliderCombo(p, ID_SPINSPIN_SLIDER);
   p_slider->SetToolTip("Scale");

   fillTable();

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);

   return true;
}


string SpinSpin::getConfigKey()
{
  string ret;
  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
     ret = "SpinSpin/" + *keys.begin();
  }
  return ret;
}




/**
 * Fill the table and initialize the UI.
 */
void SpinSpin::fillTable()
{

   IPropCalculation *expt = getCalculation();

   // Clear table
   p_grid->ClearGrid();
   p_grid->DeleteCols(0,p_grid->GetNumberCols());
   p_grid->DeleteRows(0,p_grid->GetNumberRows());


   set<string> keys = getPropertyNames();
   if (keys.size() > 0) {
      string key = *keys.begin();
      PropVector *vec = (PropVector*) expt->getProperty("SSISOTROPY");
      if (vec) {

         const vector<string> *rowLabels = vec->rowLabels();
         int nRows = vec->rows();
         p_grid->InsertCols(0,3);
         p_grid->InsertRows(0,nRows);

         p_grid->SetColLabelValue(0,"Atom 1");
         p_grid->SetColLabelValue(1,"Atom 2");
         string unit = vec->units();
         string valueLabel = vec->valueLabel();
         if (valueLabel == "") {
            valueLabel = "Isotropy " + unit;
         } else {
            valueLabel += unit;
         }
         p_grid->SetColLabelValue(2,valueLabel.c_str());

         Fragment *frag = expt->fragment();
         int numAtoms = frag->numAtoms();
         char buf[32];
         int a1, a2;
         p_atom1Indices.clear();
         p_atom2Indices.clear();
         string atSym;
         for (int idx=0; idx<nRows; idx++) {
            sscanf((*rowLabels)[idx].c_str(), "%d-%d",&a1, &a2);
            if (a1-1 < numAtoms) {
               p_atom1Indices.push_back(a1-1);
               atSym = frag->atomRef(a1-1)->atomicSymbol();
               sprintf(buf,"%d",a1);
               atSym.append(buf);
               p_grid->SetCellValue(idx,0,atSym.c_str());
            } else {
               p_grid->SetCellValue(idx,0,"Out of Range");
            }
            if (a2-1 < numAtoms) {
               p_atom2Indices.push_back(a2-1);
               atSym = frag->atomRef(a2-1)->atomicSymbol();
               sprintf(buf,"%d",a2);
               atSym.append(buf);
               p_grid->SetCellValue(idx,1,atSym.c_str());
            } else {
               p_grid->SetCellValue(idx,1,"Out of Range");
            }
            p_grid->SetCellValue(idx,2,wxString::Format (_T(PrefLabels::DOUBLEFORMAT), vec->value(idx)));
         }
      }

   }
   p_grid->SelectRow(0);

   p_grid->AutoSize();
}



void SpinSpin::OnEndSliderMotion(wxScrollEvent& event)
{
#ifndef INSTALL
   IPropCalculation *expt = getCalculation();
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif

   showVectors(false);
   event.Skip();
}



void SpinSpin::OnSliderTextEnter(wxCommandEvent& event)
{
   showVectors(false);
   event.Skip();
}



void SpinSpin::receiveFocus()
{
   //selectFragStep(-1);
   showVectors(p_guessScale);
   p_guessScale = false;

   SGContainer& sg = getFW().getSceneGraph();
   SoSwitch *pswitch = sg.getPropertySwitch(*(getPropertyNames().begin()));
   pswitch->whichChild.setValue(SO_SWITCH_ALL);
}

void SpinSpin::loseFocus()
{
   SGContainer& sg = getFW().getSceneGraph();
   SoSwitch *pswitch = sg.getPropertySwitch(*(getPropertyNames().begin()));
   pswitch->whichChild.setValue(SO_SWITCH_NONE);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
}


void SpinSpin::OnPosColorClick( wxCommandEvent& event )
{
   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_SPINSPIN_POSCOLOR);
   wxColour color = btn->GetBackgroundColour();

   wxPoint pos = wxGetMousePosition();
   ewxColorDialog dlg(color, this, wxID_ANY, "ECCE Vector Color", pos);
   if (dlg.ShowModal() == wxID_OK) {
      wxString bgcolor = dlg.GetColor().GetAsString(wxC2S_HTML_SYNTAX);


      btn->SetBackgroundColour(bgcolor);

      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write((getConfigKey()+"Pos").c_str(), bgcolor);

      showVectors(false);

   }

   event.Skip();
}

void SpinSpin::OnNegColorClick( wxCommandEvent& event )
{
   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_SPINSPIN_NEGCOLOR);
   wxColour color = btn->GetBackgroundColour();

   wxPoint pos = wxGetMousePosition();
   ewxColorDialog dlg(color, this, wxID_ANY, "ECCE Vector Color", pos);
   if (dlg.ShowModal() == wxID_OK) {
      wxString bgcolor = dlg.GetColor().GetAsString(wxC2S_HTML_SYNTAX);


      btn->SetBackgroundColour(bgcolor);

      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write((getConfigKey()+"Neg").c_str(), bgcolor);

      showVectors(false);

   }

   event.Skip();
}



void SpinSpin::showVectors(bool guessScale)
{
   //cout << "SpinSpin showVectors: " << endl;
   vector<int> rows;
   set<int>::const_iterator sit = p_selectedRows.begin();
   for ( ; sit != p_selectedRows.end(); sit++) {
      //cout << *sit << " ";
      rows.push_back(*sit);
   }
   //cout << endl;
   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
#ifndef INSTALL
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif
   SGContainer& sg = fw.getSceneGraph();

   //ewxButton* btn = (ewxButton*)FindWindow(ID_BUTTON_SPINSPIN_VECCOLOR);
   //wxColour color = btn->GetBackgroundColour();
   ewxButton* btn = (ewxButton*)FindWindow(ID_BUTTON_SPINSPIN_POSCOLOR);
   wxColour pos = btn->GetBackgroundColour();
   btn = (ewxButton*)FindWindow(ID_BUTTON_SPINSPIN_NEGCOLOR);
   wxColour neg = btn->GetBackgroundColour();

   // Only the neutral color is used for this vector...
   Command *cmd = new SpinSpinTensorCmd("Spin Spin Tensor",&sg, expt);
   set<string> props = getPropertyNames();
   string pname = *(props.begin());
   cmd->getParameter("property")->setString(pname);
   cmd->getParameter("positive")->setString(pos.GetAsString(wxC2S_HTML_SYNTAX).c_str());
   cmd->getParameter("negative")->setString(neg.GetAsString(wxC2S_HTML_SYNTAX).c_str());
   cmd->getParameter("Amplitude")->setDouble(p_slider->GetFloatValue());
   cmd->getParameter("rows")->setIntegerList(rows);
   if (guessScale) {
      cmd->getParameter("newData")->setBoolean(guessScale);
   }

   fw.execute(cmd);

   if (guessScale) {
      double value = cmd->getParameter("Amplitude")->getDouble();
      int nearestInt = static_cast<int>(value);
      int sliderMin = (nearestInt-2);
      int sliderMax = (nearestInt+2);
      p_slider->SetRange((float)sliderMin,(float)sliderMax);
      p_slider->SetValue(cmd->getParameter("Amplitude")->getDouble());
   }
}


void SpinSpin::OnRowSelection(wxGridEvent& event)
{
   //showVectors(false);
   event.Skip();
}



/**
 * Process a range selection.
 * CTRL selects come in one per row making this a bit tricky.
 * Bizarre selection patterns typically seems to result in a unselect of the
 * full range followed by a select of the new range making it easier.
 */
void SpinSpin::OnGridRangeSelect(wxGridRangeSelectEvent& event)
{

   //cout << "SpinSpin selection" << event.GetTopRow() << "," << event.GetBottomRow() << " selecting" << event.Selecting() << endl;
   if (event.Selecting()) {
      if (event.ControlDown()) {
         // add to selection - top and bottom should be the same
         //frag->m_atomHighLight.push_back(event.GetTopRow());
         p_selectedRows.insert(event.GetTopRow());
      } else {
         // a row or possible a block of rows
         p_selectedRows.clear();
         for (int idx=event.GetTopRow(); idx<=event.GetBottomRow(); idx++) {
            p_selectedRows.insert(idx);
         }
      }
   } else {
      // Unselect - rebuild the set minus the deselection 
      set<int> tmp = p_selectedRows;
      p_selectedRows.clear();
      set<int>::const_iterator it = tmp.begin();
      for ( ; it != tmp.end(); it++) {
         if (*it < event.GetTopRow() || *it > event.GetBottomRow()) {
            p_selectedRows.insert(*it);
         }
      }
   }
   /*
   cout << "FInal: " << endl;
   set<int>::const_iterator sit = p_selectedRows.begin();
   for ( ; sit != p_selectedRows.end(); sit++) {
      cout << *sit << " ";
   }
   cout << endl;
   */
   showVectors(true);
   event.Skip();
}
