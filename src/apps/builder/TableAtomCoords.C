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

#include "viz/AtomCoordVecCmd.H"

#include "wxviz/WxVizToolFW.H"
#include "wxviz/SGSelection.H"

#include "TableAtomCoords.H"

  using std::find;


//TODO 
// . show vector associated with just the selected row?

//static const char *INIFILE = "wxbuilder.ini";


// ----------------------------------------------------------------------------


IMPLEMENT_DYNAMIC_CLASS(TableAtomCoords, MomentsPanel)



TableAtomCoords::TableAtomCoords()
  : MomentsPanel()
{
}



TableAtomCoords::TableAtomCoords(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : MomentsPanel()
{
   Create(calculation, parent, id, pos, size, style, name);
   // Copied hardwired numbers from CVMatrixCoordinates
   // Doesn't work to set it at create time for some reason
   //p_slider->SetRange((float)-3.,(float)7.);
}



TableAtomCoords::~TableAtomCoords()
{
}



string TableAtomCoords::getConfigKey()
{
  string ret = "TableAtomCoords";
  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
     ret =  *keys.begin();
  }
  return ret;
}




/**
 * Fill the table and initialize the UI.
 */
void TableAtomCoords::fillTable()
{

   IPropCalculation *expt = getCalculation();

   // Clear table
   p_grid->ClearGrid();
   p_grid->DeleteCols(0,p_grid->GetNumberCols());
   p_grid->DeleteRows(0,p_grid->GetNumberRows());

  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
     string key = *keys.begin();
     PropTable *tbl = (PropTable*) expt->getProperty(key);
     if (tbl) {
        int cols = tbl->columns();
        int rows = tbl->rows();

        SGFragment *frag = getFW().getSceneGraph().getFragment();
        //NOTE #rows should be #atoms
        int nAtoms = frag->numAtoms();

        if (nAtoms != rows) {
           cerr << "# of atoms != # rows - proceeding anyway!" << endl;
        }

        const vector<string> *colLabels = tbl->colLabels();
        if (colLabels && colLabels->size() > 0) {
           p_grid->InsertCols(0,colLabels->size());
           for (size_t ic=0; ic<colLabels->size(); ic++) {
              p_grid->SetColLabelValue(ic,(*colLabels)[ic]);
           }
        } else {
           // Fallback in case not provided - don't know if its useful
           p_grid->InsertCols(0,3);
           p_grid->SetColLabelValue(0,"X");
           p_grid->SetColLabelValue(1,"Y");
           p_grid->SetColLabelValue(2,"Z");
        }

        p_grid->InsertRows(0,rows);
        for (int ir=0; ir<rows; ir++) {
          p_grid->SetRowLabelValue(ir,frag->atomRef(ir)->atomicSymbol().c_str());
        }

        wxString strval;
        for (int ic=0; ic<cols; ic++) {
           for (int ir=0; ir<rows; ir++) {
              //strval = wxString::Format (_T(PrefLabels::DOUBLEFORMAT), tbl->value(ir,ic));
              // Old calcviewer used ""%#0.7g" - not sure which we want 
              strval = wxString::Format (_T("%#0.7g"), tbl->value(ir,ic));
              p_grid->SetCellValue(ir, ic, strval);
           }
        }

     }

  }

   // Requires wxwidgets 2.8.8
   //p_grid->SetRowLabelSize(wxGRID_AUTOSIZE);
   p_grid->SetRowLabelSize(25);
   p_grid->AutoSize();
}



void TableAtomCoords::showVectors(bool guessScale)
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
   set<string> props = getPropertyNames();
   string pname = *(props.begin());
   Command *cmd = new AtomCoordVecCmd(pname,&sg, expt);
   cmd->getParameter("property")->setString(pname);
   cmd->getParameter("Center")->setString("atom");
   cmd->getParameter("neutral")->setString(color.GetAsString(wxC2S_HTML_SYNTAX).c_str());
   cmd->getParameter("Amplitude")->setDouble(p_slider->GetFloatValue());
   if (guessScale) {
      cmd->getParameter("newData")->setBoolean(guessScale);
   }

   fw.execute(cmd);

   // For whatever reason, I could not set the range at create time but
   // it works if I set it here.
   // Copied hardwired numbers from CVMatrixCoordinates
   p_slider->SetRange((float)-3.,(float)7.);
   if (guessScale) {
      p_slider->SetValue(cmd->getParameter("Amplitude")->getDouble());
   }
}
