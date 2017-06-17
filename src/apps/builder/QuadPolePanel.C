#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <algorithm>
//  using std::copy;
  using std::fill;
#include <limits>

#include "wx/grid.h" 
#include <wx/link.h>

#include "util/InternalException.H"
#include "util/PreferenceLabels.H"

#include "tdat/PropVector.H"

#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"

#include "wxgui/ewxButton.H"
#include "wxgui/SliderCombo.H"

#include "viz/AddQuadMomentCmd.H"

#include "wxviz/WxVizToolFW.H"

#include "QuadPolePanel.H"


// TODO
// . Find a better way to set the scale range
// . Handle disparate geometry indices (see old CVDerivites code (searchfor Gaussian)


// ----------------------------------------------------------------------------




IMPLEMENT_DYNAMIC_CLASS(QuadPolePanel, MomentsPanel)


QuadPolePanel::QuadPolePanel()
  : MomentsPanel()
{
   p_useOriginalGeomHack = false;
}


QuadPolePanel::QuadPolePanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : MomentsPanel()
{
  p_useOriginalGeomHack = false;
  Create(calculation, parent, id, pos, size, style, name);
}



bool QuadPolePanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!MomentsPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("QuadPolePanel creation failed") );
    return false;
  }
  p_grid->SetColLabelSize(0);  // No labels for us

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);

  // Designed HACK to decide which geometry we need
  // See cvderivs.C for the original implemenation which was not based on
  // property names but on panel name.  This should be equivalent.
  set<string> keys = getPropertyNames();
  string pname = *keys.begin();
  PropertyTask *task = dynamic_cast<PropertyTask*>(calculation);
  if (calculation && task) {
     if (pname == "POLARIZ" || pname == "HYPERPOL1") {
        const JCode *code = task->application();
        p_useOriginalGeomHack =  code->name().find("Gaussian") != string::npos;
     }
  }


  return true;
}


QuadPolePanel::~QuadPolePanel()
{
}

bool QuadPolePanel::isRelevant(IPropCalculation *propCalc) const
{
  set<string> names;
  names.insert("POLARIZ");
  names.insert("HYPERPOL1");
  return isRelevent(propCalc, "Gaussian", "Geo", names);
}


/**
 * Fill the table and initialize the UI.
 */
void QuadPolePanel::fillTable()
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
        const vector<string> *rowLabels = vec->rowLabels();

        // TODO the label and scalarize value

        int nRows = vec->rows();
        p_grid->InsertRows(0,nRows/3);
        p_grid->InsertCols(0,3);
        wxString cellv;
        for (int idx=0; idx<nRows; idx++) {
           if (rowLabels) {
              cellv = wxString::Format (wxString("%s=")+_T(PrefLabels::DOUBLEFORMAT), (*rowLabels)[idx].c_str(),vec->value(idx));
           } else {
              // No labels
              cellv = wxString::Format (_T(PrefLabels::DOUBLEFORMAT), vec->value(idx));
           }
           // Remove too much white space but make sure we have a space 
           // after the = or its too hard to read.
           cellv.Replace(" ","",true);
           cellv.Replace("=","= ",true);
           p_grid->SetCellValue((idx/3),idx%3,cellv);
        }
     }

  }


   p_grid->AutoSize();
}


void QuadPolePanel::showVectors(bool guessScale)
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
   Command *cmd = new AddQuadMomentCmd("Dipole",&sg, expt);
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


void QuadPolePanel::receiveFocus()
{
   //cout << "Using geometry " << (p_useOriginalGeomHack ? -2 : -1) << endl;
   selectFragStep(p_useOriginalGeomHack ? -2 : -1);
   showVectors(p_guessScale);
   p_guessScale = false;

   SGContainer& sg = getFW().getSceneGraph();
   SoSwitch *pswitch = sg.getPropertySwitch(*(getPropertyNames().begin()));
   pswitch->whichChild.setValue(SO_SWITCH_ALL);
}

void QuadPolePanel::loseFocus()
{
   selectFragStep(-1);  // restore to default
   SGContainer& sg = getFW().getSceneGraph();
   SoSwitch *pswitch = sg.getPropertySwitch(*(getPropertyNames().begin()));
   pswitch->whichChild.setValue(SO_SWITCH_NONE);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
}
