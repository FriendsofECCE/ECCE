#include <algorithm>
   using std::fill;
   using std::reverse;
#include <limits>
#include <utility>
  using std::make_pair;

#include <wx/link.h>

#include "util/EventDispatcher.H"
#include "util/InternalException.H"
#include "util/PreferenceLabels.H"
#include "util/NullPointerException.H"

#include "tdat/PropTable.H"
#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include "tdat/SingleGrid.H"

#include "dsm/TGBSConfig.H"
#include "dsm/IPropCalculation.H"
#include "dsm/ICalculation.H"
#include "dsm/PropFactory.H"

#include "wxgui/ewxConfig.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxPlotCtrl.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/SliderCombo.H"
#include "wxgui/WindowEvent.H"

#include "viz/ComputeMoCmd.H"
#include "viz/IsoSurfaceCmd.H"
#include "viz/IsoValueCmd.H"
#include "viz/SurfDisplayTypeCmd.H"

#include "wxviz/WxVizToolFW.H"

#include "MoPanel.H"
#include "GridDlg.H"
#include "MoCoeffs.H"


static const char *INIFILE = "wxbuilder.ini";

const wxWindowID MoPanel::ID_SLIDER = wxNewId();


// TODO
// . The slice related code (find KLS) should be handled by the slicer tool (if
//    implemented) via our lightweight events.
// . fix transparency style
// . If you change transparency, it doesn't update until you spin the viewer
//   or perhaps change the scale.


// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE( MoPanel, MoGUI )

    EVT_MENU_RANGE(SOLID, PLOTSYM, MoPanel::OnMenuClick)
    EVT_GRID_SELECT_CELL(MoPanel::OnMoSelection)
    EVT_SCROLL(MoPanel::OnEndSliderMotion)
    EVT_TEXT_ENTER(ID_SLIDER, MoPanel::OnSliderTextEnter)
    EVT_PLOTCTRL_POINT_CLICKED(wxID_ANY, MoPanel::OnGraphSelection)

END_EVENT_TABLE()


wxFORCE_LINK_THIS_MODULE(MoPanel)


IMPLEMENT_DYNAMIC_CLASS(MoPanel, VizPropertyPanel)


MoPanel::MoPanel()
  : MoGUI(),
    TearableContentProvider(),
    p_mogrid(NULL),
    p_isValid(false),
    p_interrupt(false),
    p_griddlg(NULL),
    p_coeffsDlg(NULL),
    p_slider(NULL),
    p_selectedRow(0),
    p_plotReg(NULL),
    p_plotSym(NULL),
    p_pointToRowReg(),
    p_pointToRowSym()
{
}


MoPanel::MoPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : MoGUI(),
    TearableContentProvider(),
    p_mogrid(NULL),
    p_isValid(false),
    p_interrupt(false),
    p_griddlg(NULL),
    p_coeffsDlg(NULL),
    p_slider(NULL),
    p_selectedRow(0),
    p_plotReg(NULL),
    p_plotSym(NULL),
    p_pointToRowReg(),
    p_pointToRowSym()
{
   Create(calculation, parent, id, pos, size, style, name);
}


MoPanel::~MoPanel()
{
}



bool MoPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
   if (!MoGUI::Create(calculation, parent, id, pos, size, style )) {
      wxFAIL_MSG( wxT("MoPanel creation failed") );
      return false;
   }

   wxSize minSize(200,175);
   // Customizations of DB code
   p_mogrid = (wxGrid*)FindWindow(ID_GRID_MO);
   p_mogrid->SetRowLabelSize(0);
   p_mogrid->SetMinSize(minSize);
   wxSizer *sizer = p_mogrid->GetContainingSizer();

   // Create our two plots
   ewxPlotCtrl* plot[] = {p_plotReg, p_plotSym};
   for (int idx=0; idx<2; ++idx) {
      plot[idx] = new ewxPlotCtrl(this);
      plot[idx]->SetMinSize(minSize);
      plot[idx]->SetShowKey(true);
      plot[idx]->SetDrawSymbols(false);
      plot[idx]->SetDrawLines(false);
      plot[idx]->SetDrawSpline(false);
      plot[idx]->SetShowXAxis(false);
      plot[idx]->SetShowPlotTitle(true);
      plot[idx]->SetYAxisLabel("Energy (Hartree)");
      plot[idx]->SetShowYAxisLabel(true);
      plot[idx]->SetKeyPosition(wxPoint(100,0), true);
      sizer->Insert(1, plot[idx], 1, wxGROW);
   }
   p_plotReg = plot[0];
   p_plotSym = plot[1];

   // Hide/Show table or plots
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   int which;
   config->Read("MO/Which", &which, TABLE);
   p_mogrid->Show(which == TABLE);
   p_plotReg->Show(which == PLOT);
   p_plotSym->Show(which == PLOTSYM);
   sizer->Layout();

   wxTextCtrl *txt = (wxTextCtrl*)FindWindow(ID_TEXTCTRL_MO_CUTOFF);
   ewxNumericValidator validator(0.,100.,0.001);
   validator.setHardRange("(0.,100.)");
   validator.setValue(0.001);
   txt->SetValidator(validator);

   // Add the slider
   ewxPanel *p = new ewxPanel(this);
   p_sliderSizer->Add(p, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
   p_slider = new SliderCombo(p, ID_SLIDER);
   p_slider->SetRange((float)-5.0, (float)5.0);

   //p_slider->setRange(-5, 5);
   p_slider->SetValue(1.23);
   p_slider->SetToolTip("Log Iso value:");

   initialize();

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);

   // Have to do in here rather than in constructor to force a call
   // when the default constructor is used.  Can't be called before 
   // Create though.
   (void)getGridDlg();  // Force creation

   return true;
}

void MoPanel::initialize(bool cleargrids)
{
   SGContainer& sg = getFW().getSceneGraph();

   // Delete any existing 3D gridded field.
   // Only delete it if this is a new experiment.  Otherwise, we
   // probably got a property update and don't want to delete the
   // data.  If we were to delete it, there is currently no way to
   // tell the interface that there is no longer grid data.
   if (cleargrids) sg.clearGrids();

   updateUIOptions();

   fillUI();
}

int MoPanel::getSelectedMo() const
{
   /* Couldn't get these methods to work so keep a variable 
   wxArrayInt selections = p_mogrid->GetSelectedRows();
   int sel = 0;
   if (selections.size() > 0) 
      sel = selections[0];
   */

   wxString col1 = p_mogrid->GetColLabelValue(0);
   bool haveTypes = col1 == "Type";
   int offset = haveTypes ? 1: 0;
   long mo;
   p_mogrid->GetCellValue(p_selectedRow,offset).ToLong(&mo);
   return mo;
}


string MoPanel::getSelectedType() const
{
   wxString col1 = p_mogrid->GetColLabelValue(0);
   bool haveTypes = col1 == "Type";
   if (haveTypes) {
      return p_mogrid->GetCellValue(p_selectedRow,0).c_str();
   }
   return "alpha";
}


/**
 * Examine data to see what options can be performed (if any) and update
 * the ui accordingly.
 */
void MoPanel::updateUIOptions()
{
   IPropCalculation *expt = getCalculation();
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");

   // If we don't have the occupations, we can't do density
   bool densityOk=true;
   if (expt->getProperty("MO") && !expt->getProperty("ORBOCC")) {
      densityOk=false;
   } else if (expt->getProperty("MOBETA") && !expt->getProperty("ORBOCCBETA")){
      densityOk=false;
   }

   // If we don't have the occupations, we can't do spin density
   bool spinOk=false;
   if (expt->getProperty("MO") && expt->getProperty("MOBETA") &&
         expt->getProperty("ORBOCC") && expt->getProperty("ORBOCCBETA")) {
      spinOk=true;
   }

   ewxChoice *typewin = (ewxChoice*)FindWindow(ID_CHOICE_MO_TYPE);
   typewin->Clear();
   if (spinOk) typewin->Insert("Spin Density",0);
   if (densityOk) typewin->Insert("Density",0);
   typewin->Insert("MO",0);
   typewin->SetStringSelection(_("MO"));

   // Make sure we have a GBSConfig.  If not we'll crash when
   // we try to compute.
   TGBSConfig *config = escalc->gbsConfig();
   p_isValid = (config && !config->empty());

   ((ewxButton*)FindWindow(ID_BUTTON_MO_COMPUTE))->Enable(p_isValid);
   ((ewxButton*)FindWindow(ID_BUTTON_MO_VIEWCOEF))->Enable(p_isValid);

   if (!p_isValid) {
       getFW().showMessage("This calculation does not have a valid "
                             "basis set.  Orbitals cannot be computed.",
                              false);
   } else {
      const JCode* cap = escalc->application();
      TGBSAngFunc *angfunc = cap->getAngFunc(config->coordsys());
      int maxShell = angfunc->maxShells()-1;
      int maxl = config->max_l_value();
      if (maxl > maxShell) {
         if (maxShell == TGaussianBasisSet::p_shell) {
            getFW().showMessage(
                  "Only contributions from s and p functions are considered. "
                  "This calculation contains d or higher functions.",
                   false);
         }  else if (maxShell == TGaussianBasisSet::d_shell) {
            getFW().showMessage(
                  "Only contributions from s, p, and d functions are considered. "
                  "This calculation contains f or higher functions.",
                  false);
         }  else if (maxShell == TGaussianBasisSet::f_shell) {
            getFW().showMessage(
                  "Only contributions up to f functions are considered. "
                  "This calculation contains g or higher functions.",
                  false);
         }  else if (maxShell == TGaussianBasisSet::g_shell) {
            getFW().showMessage(
                  "Only contributions up to g functions are considered. "
                  "This calculation contains h or higher functions.",
                  false);
         }

      }

   }
}



/**
 * Fill the table and initialize the UI.
 * This code copied from motool_cdlg.
 */
void MoPanel::fillUI()
{
   //static char *shells[] = {"s","p","d","f","g","h","i"};

   IPropCalculation *expt = getCalculation();

   // Clear table
   p_mogrid->ClearGrid();
   p_mogrid->DeleteCols(0,p_mogrid->GetNumberCols());
   p_mogrid->DeleteRows(0,p_mogrid->GetNumberRows());
   // Clear plots
   p_plotReg->DeleteCurve(-1);
   p_plotReg->ClearMarkers();
   p_plotSym->DeleteCurve(-1);
   p_plotSym->ClearMarkers();

   // Get the required properties 
   PropTable     *moCoefs = (PropTable*) expt->getProperty("MO");
   PropVector    *orbEnergy = (PropVector*) expt->getProperty("ORBENG");
   PropVector    *orbOcc = (PropVector*) expt->getProperty("ORBOCC");
   PropVecString *orbSym = (PropVecString*) expt->getProperty("ORBSYM");
   // betas if any
   PropVector    *orbEnergyBeta = (PropVector*) expt->getProperty("ORBENGBETA");
   PropVector    *orbOccBeta = (PropVector*) expt->getProperty("ORBOCCBETA");
   PropVecString *orbSymBeta = (PropVecString*) expt->getProperty("ORBSYMBETA");

   INTERNALEXCEPTION(orbEnergy,"Orbital energy vector is null.");
   if (!orbEnergy) {
      p_isValid = False;
      return;
   }

   int col = 0;
   bool showType = false;
   if (orbEnergyBeta) {
      p_mogrid->InsertCols(col, 1);
      p_mogrid->SetColLabelValue(col++, "Type");
      showType = true;
   }
   if (orbEnergy) {
      p_mogrid->InsertCols(col, 1);
      p_mogrid->SetColLabelValue(col++, "MO");
      p_mogrid->InsertCols(col, 1);
      p_mogrid->SetColLabelValue(col++, "Energy\nHartree");
   }
   if (orbOcc) {
      p_mogrid->InsertCols(col, 1);
      p_mogrid->SetColLabelValue(col++, "Occ #");
   }
   if (orbSym) {
      p_mogrid->InsertCols(col, 1);
      p_mogrid->SetColLabelValue(col++, "Sym");
   }

   int alphaRows = orbEnergy->rows();
   int idxHOMO = fillTable("alpha", orbEnergy, orbOcc, orbSym, 0, showType);
   (void)fillTable("beta", orbEnergyBeta, orbOccBeta, orbSymBeta, alphaRows, showType);
   fillGraph(showType);

   p_mogrid->AutoSize();
   // symbol not found - new in 2.8.8
   //p_mogrid->SetColLabelSize(wxGRID_AUTOSIZE);
   int cursize = p_mogrid->GetColLabelSize();
   p_mogrid->SetColLabelSize((int)(cursize + cursize * .20));

   // Select the alpha HOMO by default.
   selectMo(idxHOMO);

   // ------------------------------------------------------------

   if (moCoefs) {
      // If the Coeffs Table is displayed, undisplay it.
      // (Better yet, display the HOMO coefs by default)
      //            if (XtIsManaged(getcoefsForm())) {
      //               getcoefsTogglei()->setset(False);
      //               coefsToggleCB(getcoefsToggle(), (XtPointer)0, (XtPointer)0);
      //            }
      //            XtSetSensitive(getcomputeForm(),TRUE);
   } else {
      WxVizToolFW& fw = getFW();
      fw.showMessage("This calculation does not have any MO "
            "coefficients.", false/*warning*/);
      //            XtSetSensitive(getcomputeForm(),FALSE);
      //            if (XtIsManaged(getcoefsForm())) {
      //               getcoefsTogglei()->setset(False);
      //               coefsToggleCB(getcoefsToggle(), (XtPointer)0, (XtPointer)0);
      //            }
      p_isValid = False;
   }
}


int  MoPanel::fillTable(const string& type,
      PropVector *propOrbEnergy,
      PropVector *propOrbOcc,
      PropVecString *propOrbSym,
      int startRow,
      bool showType)
{
   // Declare all local variables here.
   int idxHOMO = -1;
   vector<double> energy;
   vector<double> occ;
   vector<string> sym;

   // We (may) have 3 parallel arrays and want to display them in the table in
   // reverse order i.e. values are stored as min(-) to max(+) energies,
   // however we want to display them with min at bottom of table.
   if (propOrbEnergy) {
      energy = propOrbEnergy->values();
      reverse(energy.begin(), energy.end());
   }
   if (propOrbOcc) {
      occ = propOrbOcc->values();
      reverse(occ.begin(), occ.end());
   }
   if (propOrbSym) {
      sym = propOrbSym->values();
      reverse(sym.begin(), sym.end());
   }

   for (int row=startRow, rows=energy.size()+startRow; row<rows; ++row) {
      int col = 0;
      unsigned long idx = row - startRow;
      p_mogrid->InsertRows(row,1);

      // Optional Column 0 - type 
      if (showType) {
         p_mogrid->SetCellValue(row, col++, type);
      }

      // Column 1 - MO # (start counting from 1 (=min energy))
      p_mogrid->SetCellValue(row, col++, wxString::Format(_T("%u"), rows-row));

      // Column 2 - Energy
      if (!energy.empty()) {
         p_mogrid->SetCellValue(row, col++,
               wxString::Format(_T(PrefLabels::DOUBLEFORMAT), energy[idx]));
      }

      // Col 3 - Occ # (might not have them)
      if (!occ.empty()) {
         p_mogrid->SetCellValue(row, col++,
               wxString::Format(_T("%.1f"), occ[idx]));
         if (idxHOMO < 0 && occ[idx] > 0.0) { // find Highest Occupied MO
            idxHOMO = idx;
         }
      }

      // Col 4 - Symmetry label
      if (!sym.empty()) {
         p_mogrid->SetCellValue(row, col++, sym[idx]);
      }
   }

   return idxHOMO;
}


void MoPanel::fillGraph(bool haveTypes)
{
   // We have two different graphs to fill, one grouped by symmetries.
   // Each graph is split into three different data plots
   // 1) Unoccupied,
   // 2) Singly Occupied,
   // 3) Doubly Occupied
   // The Y-axis values (Energy) are shared between the two graphs, but the
   // X-axis values depend on the occ#.

   // bail if we don't have enough columns
   if (p_mogrid->GetNumberCols() < 4) {
      return;
   }

   int col = 0;
   int typeColumn = -1;
   if (haveTypes) typeColumn = col++;
   col++; // for "moColumn", but avoid compiler warning on unused variable
   int energyColumn = col++;
   int occColumn = col++;
   int symColumn = col++;

   map<wxString,double> regXValueMap; // "[alpha/beta]" to X-axis values
   map<wxString,double> symXValueMap; // "<sym>[ - alpha/beta]" to X-axis value
   for (int row=0, rows=p_mogrid->GetNumberRows(); row<rows; ++row) {
      wxString regValue;
      wxString symValue = p_mogrid->GetCellValue(row, symColumn);
      if (haveTypes) {
         wxString type = p_mogrid->GetCellValue(row, typeColumn);
         regValue << type;
         symValue << " - " << type;
      }
      regXValueMap[regValue] = 1;
      symXValueMap[symValue] = 1;
   }
   // renumber the above mappings
   map<wxString,double>::iterator it;
   col = 1;
   for (it = regXValueMap.begin(); it != regXValueMap.end(); ++it) {
      it->second = col++;
   }
   col = 1;
   for (it = symXValueMap.begin(); it != symXValueMap.end(); ++it) {
      it->second = col++;
   }

   // The logic is easier if we iterate over the table 3 times, one for each
   // of the occupation numbers.
   double occs[] = {0, 1, 2};
   string names[] = {"Unoccupied", "Singly Occupied", "Doubly Occupied"};
   wxBrush brushes[] = {*wxBLACK_BRUSH, *wxRED_BRUSH, *wxCYAN_BRUSH};
   wxPen pens[] = {*wxBLACK_PEN, *wxRED_PEN, *wxCYAN_PEN};
   //wxColor colors[] = {*wxBLACK, *wxRED, *wxCYAN}; // unused
   // create bitmaps to use for markers
   int width=13, height=1; // odd numbers recommended so that they center
   wxBitmap unoccupied(width, height);
   wxBitmap singlyoccupied(width, height);
   wxBitmap doublyoccupied(width, height);
   wxBitmap markers[] = {unoccupied, singlyoccupied, doublyoccupied};
   for (int idx=0; idx<3; ++idx) {
   wxMemoryDC dc;
      dc.SelectObject(markers[idx]);
      dc.SetBackground(brushes[idx]);
      dc.Clear();
      dc.SelectObject(wxNullBitmap);
   }
   wxPlotMarker marker;
   AxisMap regAxisMap, symAxisMap;
   for (int occIdx=0; occIdx<3; ++occIdx) {
      vector<double> values;
      vector<double> regXValues;
      vector<double> symXValues;
      for (int row=0, rows=p_mogrid->GetNumberRows(); row<rows; ++row) {
         double occ;
         p_mogrid->GetCellValue(row, occColumn).ToDouble(&occ);
         if (occ == occs[occIdx]) {
            double energy;
            p_mogrid->GetCellValue(row, energyColumn).ToDouble(&energy);
            wxString sym = p_mogrid->GetCellValue(row, symColumn);
            wxString regXValueString = sym;
            wxString symXValueString = sym;
            wxString type;
            if (haveTypes) {
               type = p_mogrid->GetCellValue(row, typeColumn);
               regXValueString << " - " << type;
               symXValueString << " - " << type;
            }
            double regXValue = regXValueMap[type];
            double symXValue = symXValueMap[symXValueString];
            regAxisMap[wxPoint2DDouble(regXValue,energy)] = regXValueString;
            symAxisMap[wxPoint2DDouble(symXValue,energy)] = symXValueString;
            regXValues.push_back(regXValue);
            symXValues.push_back(symXValue);
            values.push_back(energy);
            p_pointToRowReg[wxPoint2DDouble(regXValue,energy)] = row;
            p_pointToRowSym[wxPoint2DDouble(symXValue,energy)] = row;
         }
      }
      if (!values.empty()) {
         ewxPlotCtrl *plot[] = {p_plotReg, p_plotSym};
         for (int plotIdx=0; plotIdx<2; ++plotIdx) {
            int size = values.size();
            double *x = new double[size];
            double *y = new double[size];
            if (plot[plotIdx] == p_plotReg) {
               copy(regXValues.begin(), regXValues.end(), x);
            } else {
               copy(symXValues.begin(), symXValues.end(), x);
            }
            copy(values.begin(), values.end(), y);
            wxPlotData data(x, y, size);
            data.SetFilename(names[occIdx]);
            data.SetPen(wxPLOTPEN_NORMAL, pens[occIdx]);
            data.SetPen(wxPLOTPEN_ACTIVE, pens[occIdx]);
            data.SetPen(wxPLOTPEN_SELECTED, pens[occIdx]);
            plot[plotIdx]->AddCurve(data, false, true);
            // add markers
            for (int mdx=0; mdx<size; ++mdx) {
               marker.CreateBitmapMarker(wxPoint2DDouble(x[mdx],y[mdx]),
                                         markers[occIdx]);
               plot[plotIdx]->AddMarker(marker);
            }
         }
      }
   }
   // add x-value mappings
   p_plotReg->SetXValueMap(regAxisMap);
   p_plotSym->SetXValueMap(symAxisMap);
   // fix the plot titles
   wxString regTitle;
   wxString symTitle;
   for (it = regXValueMap.begin(); it != regXValueMap.end(); ++it) {
      regTitle << it->first << " : ";
   }
   regTitle = regTitle.Left(regTitle.Len()-3);
   for (it = symXValueMap.begin(); it != symXValueMap.end(); ++it) {
      symTitle << it->first << " : ";
   }
   symTitle = symTitle.Left(symTitle.Len()-3);
   p_plotReg->SetPlotTitle(regTitle);
   p_plotSym->SetPlotTitle(symTitle);
   // special zoom to fit
   // pad the bounding rect so our data isn't off screen
   ewxPlotCtrl *plot[] = {p_plotReg, p_plotSym};
   for (int plotIdx=0; plotIdx<2; ++plotIdx) {
      wxRect2DDouble rect;
      plot[plotIdx]->CalcBoundingPlotRect();
      rect = plot[plotIdx]->GetCurveBoundingRect();
      rect.m_x -= 1; // so our data labels don't go off screen
      rect.m_width += 1+1; // to make up for the m_x change, plus additional
      rect.m_y -= 20; // make room for our key
      rect.m_height += 20+2; // make up for m_y change, plus additional
      plot[plotIdx]->SetCurveBoundingRect(rect);
      plot[plotIdx]->MakeCurveVisible(-1);
   }
}


void MoPanel::selectMo(int index)
{
   p_selectedRow = index;
   p_mogrid->SelectRow(index);

   /* didn't work :(
   // select corresponding points in plots
   int offset = (p_mogrid->GetNumberCols() == 5) ? 1: 0;
   double val;
   p_mogrid->GetCellValue(p_selectedRow,offset+1).ToDouble(&val);
   p_plotReg->SelectYRange(-1, wxRangeDouble(val,val), true);
   p_plotSym->SelectYRange(-1, wxRangeDouble(val,val), true);
   */

   // This doesn't seem to work
   // 8/5/08 - Only works if the panel is showing so may not work
   // due to the fact that the panel is created collapsed.
   // Add code to receiveFocus to work around this.
   p_mogrid->MakeCellVisible(index,0);
}


/**
 * This is the callback for user selection from the MO menu.
 */
void MoPanel::OnMenuClick( wxCommandEvent& event )
{
   if (event.GetId() == DEFINEGRID) {
      showGridDlg();
   } else if (event.GetId() == TABLE ||
              event.GetId() == PLOT ||
              event.GetId() == PLOTSYM) {
      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write("MO/Which", event.GetId());
      p_mogrid->Show(event.GetId() == TABLE);
      p_plotReg->Show(event.GetId() == PLOT);
      p_plotSym->Show(event.GetId() == PLOTSYM);
      p_mogrid->GetContainingSizer()->Layout();
   } else if (event.GetId() == SOLID  ||
              event.GetId() == MESH ||
              event.GetId() == CONTOUR) {
      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write("MO/Surface", event.GetId());
      updateSurface();
   } else if (event.GetId() == REDGREEN  ||
              event.GetId() == REDBLUE ||
              event.GetId() == BLUEYELLOW ||
              event.GetId() == LIGHTDARK) {
      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write("MO/Colors", event.GetId());
      updateIsoValue();
   } else if (event.GetId() == OPAQUE  ||
              event.GetId() == SEMITRANSPARENT) {
      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write("MO/Transparency", event.GetId());
      updateIsoValue();
   }
}

void MoPanel::resetColorMap()
{
//KLS TODO 
/*
   double fieldMin = cvsg()->getCurrentGrid()->fieldMin();
   double fieldMax = cvsg()->getCurrentGrid()->fieldMax();

   //  - update the Slicer gui (TODO: need access to the gui)

   //  - update necessary cmds' params
   UCommand *cmd = cmdMgr()->getCommand("CmdXYSlice");
   cmd->setParam("CMapMin",fieldMin);
   cmd->setParam("CMapMax",fieldMax);

   cmd = cmdMgr()->getCommand("CmdXZSlice");
   cmd->setParam("CMapMin",fieldMin);
   cmd->setParam("CMapMax",fieldMax);

   cmd = cmdMgr()->getCommand("CmdYZSlice");
   cmd->setParam("CMapMin",fieldMin);
   cmd->setParam("CMapMax",fieldMax);
*/

}

wxWindow* MoPanel::GetTearableContent()
{
  wxToolBar *tb = new wxToolBar(this, wxID_ANY,
          wxDefaultPosition, wxDefaultSize,
          wxTB_VERTICAL|wxTB_TEXT|wxTB_NOICONS,
          "MoPanel Menu");
 
  tb->AddRadioTool(TABLE,"Table", wxNullBitmap, wxNullBitmap, "");
  tb->AddRadioTool(PLOT,"Graph",wxNullBitmap, wxNullBitmap, "");
  tb->AddRadioTool(PLOTSYM,"Graph by Symmetries",wxNullBitmap,wxNullBitmap,"");

  tb->AddSeparator();

  tb->AddRadioTool(SOLID,"Solid", wxNullBitmap, wxNullBitmap, "");
  tb->AddRadioTool(MESH,"Mesh",wxNullBitmap, wxNullBitmap, "");
  tb->AddRadioTool(CONTOUR,"Contour",wxNullBitmap, wxNullBitmap, "");

  tb->AddSeparator();

  tb->AddRadioTool(REDGREEN,"Red/Green", wxNullBitmap, wxNullBitmap, "");
  tb->AddRadioTool(REDBLUE,"Red/Blue",wxNullBitmap, wxNullBitmap, "");
  tb->AddRadioTool(BLUEYELLOW,"Blue/Yellow",wxNullBitmap, wxNullBitmap, "");
  tb->AddRadioTool(LIGHTDARK,"Light/Dark",wxNullBitmap, wxNullBitmap, "");

  tb->AddSeparator();
  tb->AddTool(DEFINEGRID,"Define Grid...",wxNullBitmap, "");
//   tb->AddCheckTool(HIDE,"Hide Surface...",wxNullBitmap, "");
// Decided this was kinda lame anyway so remove it
  tb->AddSeparator();
  tb->AddRadioTool(SEMITRANSPARENT,"Semi-Transparent",wxNullBitmap,wxNullBitmap,  "");
  tb->AddRadioTool(OPAQUE,"Opaque",wxNullBitmap, wxNullBitmap, "");

  tb->Realize();

  ewxConfig *config = ewxConfig::getConfig(INIFILE);
  int id;
  config->Read("MO/Which", &id, TABLE);
  tb->ToggleTool(id,true);
  config->Read("MO/Colors", &id, REDGREEN);
  tb->ToggleTool(id,true);
  config->Read("MO/Surface", &id, SOLID);
  tb->ToggleTool(id,true);
  config->Read("MO/Transparency", &id, SEMITRANSPARENT);
  tb->ToggleTool(id,true);

  return tb;
}

void MoPanel::OnButtonMoViewcoefClick( wxCommandEvent& event )
{
   showCoeffs(true);

}

void MoPanel::showCoeffs(bool force)
{
   if (p_coeffsDlg == 0) {
      p_coeffsDlg = new MoCoeffs(this);
   }
   if (force) p_coeffsDlg->Show();
   if (p_coeffsDlg->IsShownOnScreen())
      p_coeffsDlg->showCoeffs(
            dynamic_cast<ICalculation*>(getCalculation()), getSelectedMo());
}

void MoPanel::OnButtonMoComputeClick( wxCommandEvent& event )
{
   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
   SGContainer& sg = fw.getSceneGraph();

   // Take the focus
   setFocus(true);

   ewxProgressDialog *dlg = 0;
   dlg = new ewxProgressDialog ("ECCE Compute MOs","Initializing...",100,0,
   wxPD_AUTO_HIDE|wxPD_CAN_ABORT|wxPD_ELAPSED_TIME|wxPD_SMOOTH);
   wxPoint pos = wxGetMousePosition();
   dlg->SetSize(pos.x, pos.y, -1, -1);
   // NOte might also want to just call CenterOnParent()
   // Wasn't sure it was 'nice' to obscure the viewer.
   dlg->Show();



   Command *cmd = new ComputeMoCmd("Compute Mo", &sg, expt,dlg);

   string fieldtype = "MO";
   ewxChoice *typewin = (ewxChoice*)FindWindow(ID_CHOICE_MO_TYPE);
   fieldtype = typewin->GetString(typewin->GetSelection());
   cmd->getParameter("FieldType")->setString(fieldtype);
   ewxTextCtrl *cutoff = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_MO_CUTOFF);
   cmd->getParameter("CoefCutoff")->setDouble(cutoff->getValueAsDouble());
   cmd->getParameter("SelectedMO")->setInteger(getSelectedMo()); // starts at 1
   cmd->getParameter("Type")->setString(getSelectedType());
   const JCode* cap = escalc->application();
   cmd->getParameter("Code")->setString(cap->name());

   float fromx,tox, fromy, toy, fromz, toz;
   int resx, resy, resz;
   bool uniform;

   // Could not do this at create time becaus fragment is not yet set
   updateGridDlg();
   p_griddlg->getResolution(resx,resy,resz,uniform);
   p_griddlg->getExtent(fromx,tox,fromy,toy,fromz,toz);
   cmd->getParameter("resX")->setInteger(resx);
   cmd->getParameter("resY")->setInteger(resy);
   cmd->getParameter("resZ")->setInteger(resz);
   cmd->getParameter("fromX")->setDouble(fromx);
   cmd->getParameter("toX")->setDouble(tox);
   cmd->getParameter("fromY")->setDouble(fromy);
   cmd->getParameter("toY")->setDouble(toy);
   cmd->getParameter("fromZ")->setDouble(fromz);
   cmd->getParameter("toZ")->setDouble(toz);
   fw.execute(cmd);
   dlg->Show(false);  // make sure it goes away

   // Get min,max of computed field
   double fieldMin = 0.;
   double fieldMax = 0.;

   SingleGrid *grid = sg.getCurrentGrid();
   if (grid) {
      fieldMin = grid->fieldMin();
      fieldMax = grid->fieldMax();
   }

   bool validField = true;
   if (fieldMax > fieldMin) {

      // Once the MO field is computed, we need to decide what, if anything,
      // to display.  It would seem strange to not have anything displayed,
      // however if Surface/Hide is toggled on and the Slicer planes are
      // toggled off then nothing should be displayed.
      // (I don't currently have access to the Hide toggle, I can't change it.)

      // (Need a way to get params of any cmd)
      //      char *displayType = cmdMgr()->getParam(s_cmdLibName,
      //                                "SurfDisplayType","IsosurfStyle",

      // --- Isosurface  (TODO: what if Surface/Hide is toggled on?)
      double posR, posG, posB, negR, negG, negB;
      getSurfaceColors(posR, posG, posB, negR, negG, negB);

      cmd = new IsoSurfaceCmd("Iso Surface", &sg, expt);
      cmd->getParameter("transparency")->setDouble(0.5);
      cmd->getParameter("positiveRed")->setDouble(posR);
      cmd->getParameter("positiveGreen")->setDouble(posG);
      cmd->getParameter("positiveBlue")->setDouble(posB);
      cmd->getParameter("negativeRed")->setDouble(negR);
      cmd->getParameter("negativeGreen")->setDouble(negG);
      cmd->getParameter("negativeBlue")->setDouble(negB);
      fw.execute(cmd);

      updateSurface();


      // Determine max abs value of field for the (abs value) slider
      // Bruce says that for spin density, its better to hardwire the max
      // scale to no omre than 1.0 but probably even less.
      // Edo suggests we use a log scale instead.
      // Now we decided to hardwire the density max to 1.0.
      float absIsovalMax = 0.2; // default for spin density
      if (fieldtype == "Density") {
         absIsovalMax = 0.2; // default for density
      } else if (fieldtype == "MO") {
         absIsovalMax = fabs(fieldMin) > fabs(fieldMax) ?
            fabs(fieldMin) : fabs(fieldMax);

      }

      // Pct of computed field's (absolute) range for initial isovalue
      // Was class static before
      const double s_isovalPct = 0.25;

      // set the initial slider value and the corresponding Cmd parameter:
      double isovalue = log10(s_isovalPct * absIsovalMax);
      p_slider->SetValue(isovalue);
      updateIsoValue(isovalue);

      // --- Deal with orthogonal slices (Slicer gui)
      // We need to update the max values of all 3 ortho slice sliders and
      // reset the current values if, for example, we have reduced the
      // resolution along one axis and the previous ortho slice on that axis
      // is invalid.

      // Update any displayed orthogonal slicing planes:
      /* KLS
         if (slicer() != (SlicerGUI*)0 ) {
      // Update the max values of the slices
      slicer()->updateSlicesMax();
      // Update the min & max values of the colormap
      slicer()->updateCMapMinMax();
      slicer()->displaySlices(cvsg());
      }
       */
   } else {
      validField = False;
      ewxMessageDialog dlg(this, 
            "The grid value for this MO is 0.  This may be \n"
            "because this MO was NOT provided by the code or it \n"
            "may be that there was no contribution from any of \n"
            "shells currently supported by the compute algorithm.\n",
            "ECCE Error", wxOK|wxICON_EXCLAMATION);
      (void)dlg.ShowModal();
      dlg.Destroy();
      fw.showMessage("Display is unchanged.");
   }

   // --- Make this available only if we really have a valid field
   p_slider->Enable(validField);
   //fw.getSceneGraph().getMORoot()->whichChild.setValue(SO_SWITCH_ALL);


   resetColorMap();

   event.Skip();
}



GridDlg *MoPanel::getGridDlg()
{
   if (p_griddlg == 0) {
      p_griddlg = new GridDlg(this);
      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      int x,y,z;
      bool uniform;
      config->Read("Grid/x",&x,30);
      config->Read("Grid/y",&y,30);
      config->Read("Grid/z",&z,30);
      config->Read("Grid/uniform",&uniform,true);
      p_griddlg->setResolution(x,y,z,uniform);


      p_griddlg->Connect(wxID_OK, wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) (wxEventFunction)
            &MoPanel::OnGridOkButton, 0, this);
      p_griddlg->Connect(wxID_APPLY, wxEVT_COMMAND_BUTTON_CLICKED,
            (wxObjectEventFunction) (wxEventFunction)
            &MoPanel::OnGridOkButton, 0, this);

   }
   wxPoint pos = wxGetMousePosition();
   p_griddlg->SetSize(pos.x, pos.y, -1, -1);
   return p_griddlg;
}

void MoPanel::updateGridDlg()
{
   GridDlg *grid = getGridDlg();

   SGFragment *frag = getFW().getSceneGraph().getFragment() ;
   if (frag) {
      const float s_boundingBoxExtra = 2.0;
      float xmin, ymin, zmin, xmax, ymax, zmax;
      frag->getBoundBox(xmin, ymin, zmin, xmax, ymax, zmax);

      // Extend the range of the grid beyond the
      // bounding box of the chemical system.
      xmin -= s_boundingBoxExtra;
      ymin -= s_boundingBoxExtra;
      zmin -= s_boundingBoxExtra;
      xmax += s_boundingBoxExtra;
      ymax += s_boundingBoxExtra;
      zmax += s_boundingBoxExtra;
      grid->setExtent(xmin,xmax,ymin,ymax,zmin,zmax);

   }
}


/**
 * Popup the Grid Dialog and make sure its initialized.
 */
void MoPanel::showGridDlg()
{
   updateGridDlg();
   getGridDlg()->Show();
}




void MoPanel::OnGridOkButton(wxCommandEvent& evt)
{
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   int x,y,z;
   bool uniform;
   p_griddlg->getResolution(x,y,z,uniform);

   config->Write("Grid/x",x);
   config->Write("Grid/y",y);
   config->Write("Grid/z",z);
   config->Write("Grid/uniform",uniform);
   evt.Skip();
}

void MoPanel::OnMoSelection(wxGridEvent& event)
{
   p_selectedRow = event.GetRow();
   showCoeffs(false);
   event.Skip();
}

void MoPanel::OnGraphSelection(wxPlotCtrlEvent& event)
{
   wxPlotCtrl * plot = event.GetPlotCtrl();
   wxPlotData * curve = (wxPlotData*)event.GetCurve();
   int index = event.GetCurveDataIndex();
   wxPoint2DDouble point = curve->GetPoint(index);
   if (plot == p_plotReg) {
      if (p_pointToRowReg.find(point) != p_pointToRowReg.end()) {
         selectMo(p_pointToRowReg[point]);
      }
   } else if (plot == p_plotSym) {
       if (p_pointToRowSym.find(point) != p_pointToRowSym.end()) {
          selectMo(p_pointToRowSym[point]);
       }
   }
   event.Skip();
}

void MoPanel::updateSurface()
{
   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   IPropCalculation *expt = getCalculation();

   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   int id;

   config->Read("MO/Surface", &id, SOLID);
   string stype = "Solid";
   if (id == MESH)
      stype = "Mesh";
   else if (id == CONTOUR)
      stype = "Contour";
   Command *cmd = new SurfDisplayTypeCmd("Surface Type", &sg, expt);
   cmd->getParameter("IsosurfStyle")->setString(stype);
   fw.execute(cmd);
}

void MoPanel::OnEndSliderMotion(wxScrollEvent& event)
{
   updateIsoValue();
   event.Skip();
}

void MoPanel::OnSliderTextEnter(wxCommandEvent& event)
{
   updateIsoValue();
   event.Skip();
}

/**
 * Re-issue isovalue command based on slider value
 */
void MoPanel::updateIsoValue()
{
   double isovalue = static_cast<double>(p_slider->GetFloatValue());
   updateIsoValue(isovalue);
}

/**
 * Recompute the iso value with appropriate color and other settings.
 * NOTE: the transparency style doesn't seem to update until some
 * secondary tweak to the UI - not sure why.
 */
void MoPanel::updateIsoValue(double isovalue)
{
   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   IPropCalculation *expt = getCalculation();

   double posR, posG, posB, negR, negG, negB;
   getSurfaceColors(posR, posG, posB, negR, negG, negB);

   Command *cmd = new IsoValueCmd("Iso Value", &sg, expt);
   cmd->getParameter("Value")->setDouble(isovalue);
   cmd->getParameter("transparency")->setDouble(getTransparency());
   cmd->getParameter("positiveRed")->setDouble(posR);
   cmd->getParameter("positiveGreen")->setDouble(posG);
   cmd->getParameter("positiveBlue")->setDouble(posB);
   cmd->getParameter("negativeRed")->setDouble(negR);
   cmd->getParameter("negativeGreen")->setDouble(negG);
   cmd->getParameter("negativeBlue")->setDouble(negB);
   fw.execute(cmd);
}


/**
 * Apparently users required the capability to override our limited
 * set of color choices via an env variable.
 * The Light/Dark is overridden
 */
void MoPanel::getSurfaceColors(double& posR, double& posG, double& posB, 
                               double& negR, double& negG, double& negB)
{
   int id;
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Read("MO/Colors", &id, REDGREEN);
   if (id == REDGREEN) {
      posR = 1.0; posG = 0.0; posB = 0.0;
      negR = 0.0; negG = 1.0; negB = 0.0;

   } else if (id == REDBLUE) {
      posR = 1.0; posG = 0.0; posB = 0.0;
      negR = 0.0; negG = 0.2; negB = 1.0;

   } else if (id == BLUEYELLOW) {
      posR = 0.0; posG = 0.2; posB = 1.0;
      negR = 1.0; negG = 1.0; negB = 0.0;

   } else if (id == LIGHTDARK) {
      posR = 0.95; posG = 0.95; posB = 0.95;
      negR = 0.2; negG = 0.2; negB = 0.2;


      char* clrptr;
      // first check for 0.0 to 1.0 normalized values
      char* clrstr = getenv("ECCE_SURFACE_COLOR1");
      if (clrstr) {
         clrptr = strtok(clrstr, " ,:/");
         posR = strtod(clrptr, NULL);
         clrptr = strtok(NULL, " ,:/");
         posG = strtod(clrptr, NULL);
         clrptr = strtok(NULL, " ,:/");
         posB = strtod(clrptr, NULL);
         clrptr = strtok(NULL, " ,:/");
         negR = strtod(clrptr, NULL);
         clrptr = strtok(NULL, " ,:/");
         negG = strtod(clrptr, NULL);
         clrptr = strtok(NULL, " ,:/");
         negB = strtod(clrptr, NULL);
      }

      // then check for 0 to 255 8-bit/1-byte values
      clrstr = getenv("ECCE_SURFACE_COLOR255");
      if (clrstr) {
         clrptr = strtok(clrstr, " ,:/");
         int clrval = (int)strtol(clrptr, NULL, 10);
         posR = ((double)clrval)/255.0;
         clrptr = strtok(NULL, " ,:/");
         clrval = (int)strtol(clrptr, NULL, 10);
         posG = ((double)clrval)/255.0;
         clrptr = strtok(NULL, " ,:/");
         clrval = (int)strtol(clrptr, NULL, 10);
         posB = ((double)clrval)/255.0;
         clrptr = strtok(NULL, " ,:/");
         clrval = (int)strtol(clrptr, NULL, 10);
         negR = ((double)clrval)/255.0;
         clrptr = strtok(NULL, " ,:/");
         clrval = (int)strtol(clrptr, NULL, 10);
         negG = ((double)clrval)/255.0;
         clrptr = strtok(NULL, " ,:/");
         clrval = (int)strtol(clrptr, NULL, 10);
         negB = ((double)clrval)/255.0;
      }



   }


}

double MoPanel::getTransparency()
{
   double ret = 0.0;
   int id;
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Read("MO/Transparency", &id, SEMITRANSPARENT);
   if (id == SEMITRANSPARENT)  ret = 0.5;
   return ret;
}

void MoPanel::receiveFocus()
{
   //
   //  *************   Hack *************
   // Here is where we hardwire the geometry step number to use
   // for MO calculation and display.  For NWChem, its the initial geometry.
   // Unfortunately the MO data does not tell us this.
   // losefocus restores to the base fragment
   //
   int step = -1;
   IPropCalculation *expt = getCalculation();
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   NULLPOINTEREXCEPTION(escalc,"Unable to downcast to ICalcualtion");
   if (expt) {
      const JCode *code = escalc->application();
      if (code && code->name() == "NWChem") {
            step = 0;
      }
   }
   selectFragStep(step);

   getFW().getViewer().setTransparencyType(SoGLRenderAction::SCREEN_DOOR);
   //getFW().getViewer().setTransparencyType(SoGLRenderAction::DELAYED_ADD);
   getFW().getSceneGraph().getMORoot()->whichChild.setValue(SO_SWITCH_ALL);

   p_mogrid->MakeCellVisible(p_selectedRow,0);
}

void MoPanel::loseFocus()
{
   selectFragStep(-1);
   getFW().getViewer().setTransparencyType(SoGLRenderAction::DELAYED_ADD);
   getFW().getSceneGraph().getMORoot()->whichChild.setValue(SO_SWITCH_NONE);
}
