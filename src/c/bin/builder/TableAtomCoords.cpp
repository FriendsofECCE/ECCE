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

#include "viz/AtomCoordVecCmd.hpp"

#include "wxviz/WxVizToolFW.hpp"
#include "wxviz/SGSelection.hpp"

#include "TableAtomCoords.hpp"

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
