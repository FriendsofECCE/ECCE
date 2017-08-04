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

#include "tdat/PropVector.H"
#include "tdat/TProperty.H"
#include "tdat/TTCPair.H"

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
#include "wxgui/WindowEvent.H"
#include "wxgui/WxColorSchemeButton.H"
#include "wxgui/ewxGrid.H"
#include "wxgui/SliderCombo.H"

#include "wxviz/WxVizToolFW.H"
#include "wxviz/SGSelection.H"

#include "inv/ChemKit/ChemColor.H"

#include "viz/TensorCmd.H"

#include "VecAtomTensor.H"
#include "TensorTable.H"

  using std::find;


static const char *INIFILE = "wxbuilder.ini";


// ----------------------------------------------------------------------------


const wxWindowID VecAtomTensor::ID_DIALOG_VECATOMTENSOR = wxNewId();
const wxWindowID VecAtomTensor::ID_VECATOMTENSOR_SLIDER = wxNewId();
const wxWindowID VecAtomTensor::ID_BUTTON_VECATOMTENSOR_POSCOLOR = wxNewId();
const wxWindowID VecAtomTensor::ID_BUTTON_VECATOMTENSOR_NEGCOLOR = wxNewId();
const wxWindowID VecAtomTensor::ID_VECATOMTENSOR_GRID = wxNewId();
const wxWindowID VecAtomTensor::ID_VECATOMTENSOR_CHOICE = wxNewId();
const wxWindowID VecAtomTensor::ID_VECATOMTENSOR_SHOWDATA = wxNewId();


BEGIN_EVENT_TABLE( VecAtomTensor, VizPropertyPanel )
    EVT_MENU(ID_VECATOMTENSOR_SHOWDATA, VecAtomTensor::OnMenuClick)
    EVT_CHOICE(ID_VECATOMTENSOR_CHOICE, VecAtomTensor::OnChoiceSelect)
   // EVT_GRID_SELECT_CELL(VECATOMTENSOR::OnRowSelection)
    EVT_GRID_RANGE_SELECT(VecAtomTensor::OnGridRangeSelect)
    EVT_BUTTON( ID_BUTTON_VECATOMTENSOR_POSCOLOR, VecAtomTensor::OnPosColorClick )
    EVT_BUTTON( ID_BUTTON_VECATOMTENSOR_NEGCOLOR, VecAtomTensor::OnNegColorClick )
    EVT_SCROLL(VecAtomTensor::OnEndSliderMotion)
    EVT_TEXT_ENTER(ID_VECATOMTENSOR_SLIDER, VecAtomTensor::OnSliderTextEnter)
END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(VecAtomTensor, VizPropertyPanel)



VecAtomTensor::VecAtomTensor()
  : VizPropertyPanel(),
    p_grid(NULL),
    p_slider(NULL),
    p_guessScale(true)
{
   p_internalSelect = false;
   subscribe();
}



VecAtomTensor::VecAtomTensor(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : VizPropertyPanel(),
    p_grid(NULL),
    p_slider(NULL),
    p_guessScale(true)
{
   Create(calculation, parent, id, pos, size, style, name);
   p_internalSelect = false;
   subscribe();
}



VecAtomTensor::~VecAtomTensor()
{
   unsubscribe();
}



bool VecAtomTensor::Create(IPropCalculation *calculation,
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
   p_grid = new ewxGrid( panel, ID_VECATOMTENSOR_GRID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
   p_grid->SetRowLabelSize(0);
   p_grid->SetMinSize(wxSize(200,175));
   p_grid->CreateGrid(2, 2, wxGrid::wxGridSelectRows);
   tableSizer->Add(p_grid, 0, wxGROW|wxALL, 3);

    tableSizer->Add(new ewxStaticText( panel, wxID_STATIC, _("Select:"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
   p_selectOptions = new ewxChoice(this, ID_VECATOMTENSOR_CHOICE);
   tableSizer->Add(p_selectOptions, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);
   wxBoxSizer* spaceSizer = new wxBoxSizer(wxHORIZONTAL);
   tableSizer->Add(spaceSizer,1,wxGROW|wxALL,0);
   spaceSizer->AddStretchSpacer(3);

   wxBoxSizer *optionsSizer = new wxBoxSizer(wxHORIZONTAL);
   panel->GetSizer()->Add(optionsSizer, 0, wxALIGN_LEFT|wxALL, 0);

    // A button to control color
    optionsSizer->AddSpacer(3);
    ewxStaticText* label = new ewxStaticText( panel, wxID_STATIC, _("Tensor Color:"), wxDefaultPosition, wxDefaultSize, 0 );
    optionsSizer->Add(label, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);
    ewxButton* pcolorbtn = new ewxButton( panel, ID_BUTTON_VECATOMTENSOR_POSCOLOR, _T("+"), wxDefaultPosition, wxSize(24, 24), wxBU_EXACTFIT );
    optionsSizer->Add(pcolorbtn, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    ewxButton *ncolorbtn = new ewxButton( panel, ID_BUTTON_VECATOMTENSOR_NEGCOLOR, _T("-"), wxDefaultPosition, wxSize(24, 24), wxBU_EXACTFIT );
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
   p_slider = new SliderCombo(p, ID_VECATOMTENSOR_SLIDER);
   p_slider->SetToolTip("Scale");

   fillTable();

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);

   return true;
}

wxWindow* VecAtomTensor::GetTearableContent()
{
  wxToolBar *tb = new wxToolBar(this, wxID_ANY,
          wxDefaultPosition, wxDefaultSize,
          wxTB_VERTICAL|wxTB_TEXT|wxTB_NOICONS,
          "Tensor Menu");
 
  tb->AddTool(ID_VECATOMTENSOR_SHOWDATA,"Show Vector Data...",wxNullBitmap, "");
  tb->Realize();

  return tb;
}

string VecAtomTensor::getConfigKey()
{
  string ret;
  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
     ret = "VecAtomTensor/" + *keys.begin();
  }
  return ret;
}




/**
 * Fill the table and initialize the UI.
 */
void VecAtomTensor::fillTable()
{

   IPropCalculation *expt = getCalculation();

   // Clear table
   p_grid->ClearGrid();
   p_grid->DeleteCols(0,p_grid->GetNumberCols());
   p_grid->DeleteRows(0,p_grid->GetNumberRows());


   p_selectOptions->Clear();
   p_selectOptions->Append("By Atom");
   p_selectOptions->SetSelection(0);


   set<string> keys = getPropertyNames();
   size_t idx;
   if (keys.size() > 0) {
      string key = *keys.begin();
      PropVector *vec = (PropVector*) expt->getProperty(key);
      if (vec) {
         Fragment *frag = expt->fragment();
         size_t numAtoms = frag->numAtoms();

         // Pointers to TensorTables - none now
         p_tables.resize(numAtoms,0);

         // Figure out which atoms we have in the system
         // TODO create choice menu and populate
         vector<string> options;
         options.push_back("By Atom");
         vector<TTagCountPair>* tags = frag->tagCounts();
         for (idx=0; idx<tags->size(); idx++) {
            options.push_back((*tags)[idx].tag);
            p_selectOptions->Append((*tags)[idx].tag);
         }
         delete tags;

         //const vector<string> *rowLabels = vec->rowLabels();
         p_grid->InsertCols(0,2);
         p_grid->InsertRows(0,numAtoms);

         p_grid->SetColLabelValue(0,"#");
         p_grid->SetColLabelValue(1,"Symbol");

         for (idx=0; idx<numAtoms; idx++) {
            p_grid->SetCellValue(idx, 0, wxString::Format (_T("%zu"),idx+1));
            p_grid->SetCellValue(idx, 1, frag->atomRef(idx)->atomicSymbol().c_str());
         }

         if (numAtoms > 0) p_grid->SelectRow(0);
      }

   }

   p_grid->AutoSize();
}



void VecAtomTensor::OnEndSliderMotion(wxScrollEvent& event)
{
#ifndef INSTALL
   IPropCalculation *expt = getCalculation();
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif

   showVectors(false);
   event.Skip();
}



void VecAtomTensor::OnSliderTextEnter(wxCommandEvent& event)
{
   showVectors(false);
   event.Skip();
}



void VecAtomTensor::receiveFocus()
{
   //selectFragStep(-1);
   showVectors(true /*p_guessScale*/);
   p_guessScale = false;

   SGContainer& sg = getFW().getSceneGraph();
   SoSwitch *pswitch = sg.getPropertySwitch(*(getPropertyNames().begin()));
   pswitch->whichChild.setValue(SO_SWITCH_ALL);
}

void VecAtomTensor::loseFocus()
{
   SGContainer& sg = getFW().getSceneGraph();
   SoSwitch *pswitch = sg.getPropertySwitch(*(getPropertyNames().begin()));
   pswitch->whichChild.setValue(SO_SWITCH_NONE);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
}


void VecAtomTensor::OnPosColorClick( wxCommandEvent& event )
{
   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_VECATOMTENSOR_POSCOLOR);
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

void VecAtomTensor::OnNegColorClick( wxCommandEvent& event )
{
   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_VECATOMTENSOR_NEGCOLOR);
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

void VecAtomTensor::OnChoiceSelect( wxCommandEvent& event )
{
   ewxChoice *choice = (ewxChoice*)FindWindow(ID_VECATOMTENSOR_CHOICE);
   int index = choice->GetSelection();
   if (index > 0) {
      p_internalSelect = true;
      wxString value = choice->GetString(index);
      p_grid->ClearSelection();
      p_selectedRows.clear();
      int nRows = p_grid->GetNumberRows();
      for (int row=0; row<nRows; row++) {
        wxString el = p_grid->GetCellValue(row, 1 /*symbol*/);
        if (el == value) {
           p_grid->SelectRow(row,true);
           p_selectedRows.insert(row);
        }
      }
      p_internalSelect = false;
      showVectors(true);

   }
   event.Skip();

}



void VecAtomTensor::showVectors(bool guessScale)
{
   vector<int> atoms;
   set<int>::const_iterator sit = p_selectedRows.begin();
   for ( ; sit != p_selectedRows.end(); sit++) {
      //cout << *sit << " ";
      atoms.push_back(*sit);
   }
   //cout << endl;
   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
#ifndef INSTALL
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif
   SGContainer& sg = fw.getSceneGraph();

   ewxButton* btn = (ewxButton*)FindWindow(ID_BUTTON_VECATOMTENSOR_POSCOLOR);
   wxColour pos = btn->GetBackgroundColour();
   btn = (ewxButton*)FindWindow(ID_BUTTON_VECATOMTENSOR_NEGCOLOR);
   wxColour neg = btn->GetBackgroundColour();

   // Only the neutral color is used for this vector...
   Command *cmd = new TensorCmd("Tensor",&sg, expt);
   set<string> props = getPropertyNames();
   string pname = *(props.begin());
   cmd->getParameter("property")->setString(pname);
   cmd->getParameter("positive")->setString(pos.GetAsString(wxC2S_HTML_SYNTAX).c_str());
   cmd->getParameter("negative")->setString(neg.GetAsString(wxC2S_HTML_SYNTAX).c_str());
   cmd->getParameter("Amplitude")->setDouble(p_slider->GetFloatValue());
   cmd->getParameter("Atoms")->setIntegerList(atoms);
   cmd->getParameter("newData")->setBoolean(guessScale);

   fw.execute(cmd);

   // Set up log scale based on computed vector sizes
   if (guessScale) {
      double value = cmd->getParameter("Amplitude")->getDouble();
      int nearestInt = static_cast<int>(value);
      int sliderMin = (nearestInt-2);
      int sliderMax = (nearestInt+2);
      p_slider->SetRange((float)sliderMin,(float)sliderMax);

      p_slider->SetValue(value);
   }
}


void VecAtomTensor::OnRowSelection(wxGridEvent& event)
{
   //showVectors(false);
   event.Skip();
}


void VecAtomTensor::OnMenuClick( wxCommandEvent& event )
{
   if (event.GetId() == ID_VECATOMTENSOR_SHOWDATA) {
      IPropCalculation *expt = getCalculation();
#ifndef INSTALL
      ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
      INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif
      //cout << "show data table(s)" << endl;
      set<int>::const_iterator sit = p_selectedRows.begin();
      for ( ; sit != p_selectedRows.end(); sit++) {
         // NOTE: no connection to OK button.
         // The default OK appears to NOT destroy the dialog which
         // means we can simply re-show it next time the user asks.
         TensorTable *tbl = p_tables[*sit];
         if (tbl == 0) {
            string key = *(getPropertyNames().begin());
            PropVector *vec = (PropVector*) expt->getProperty(key);
            tbl = new TensorTable(this);
            tbl->fillTable(vec, *sit);
            p_tables[*sit] = tbl;
         }
         tbl->Show(true);
         tbl->Raise();
      }
   }
   event.Skip();
}


/**
 * Process a range selection.
 * CTRL selects come in one per row making this a bit tricky.
 * Bizarre selection patterns typically seems to result in a unselect of the
 * full range followed by a select of the new range making it easier.
 */
void VecAtomTensor::OnGridRangeSelect(wxGridRangeSelectEvent& event)
{

   if (p_internalSelect) return;

   // Reset to by atom selection once user clicks in table
   ewxChoice *choice = (ewxChoice*)FindWindow(ID_VECATOMTENSOR_CHOICE);
   choice->SetSelection(0);


   //cout << "VecAtomTensor selection" << event.GetTopRow() << "," << event.GetBottomRow() << " selecting" << event.Selecting() << endl;
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

   // Reset to select by atom in this case
   p_selectOptions->SetSelection(0);

   event.Skip();
}


void VecAtomTensor::eventMCB(const Event& event)
{
   string name = event.getName();
   string value = event.getValue();
   if (name == "SelectionChanged") {
      if (!p_internalSelect) {
         SGFragment *frag = getFW().getSceneGraph().getFragment();
         int numSel = frag->m_atomHighLight.size();
         p_internalSelect = true;
         p_selectedRows.clear();
         p_grid->ClearSelection();
         for (int idx=0; idx<numSel; idx++) {
            p_grid->SelectRow(frag->m_atomHighLight[idx],true);
            p_selectedRows.insert(frag->m_atomHighLight[idx]);
         }
         showVectors(true);
         p_internalSelect = false;
      }
   }
}
