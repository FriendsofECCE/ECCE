#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <algorithm>
  using std::fill;
#include <limits>

#include <wx/link.h>

#include "util/EventDispatcher.H"
#include "util/InternalException.H"
#include "util/PreferenceLabels.H"

#include "tdat/PropVector.H"

#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxColorDialog.H"
#include "wxgui/ewxConfig.H"
#include "wxgui/ewxPlotCtrl.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/SliderCombo.H"
#include "wxgui/WindowEvent.H"

#include "viz/NModeStepCmd.H"
#include "viz/NModeTraceCmd.H"
#include "viz/NModeVectCmd.H"

#include "wxviz/SGSelection.H"
#include "wxviz/WxVizToolFW.H"

#include "NModePanel.H"

  using std::find;


static const char *INIFILE = "wxbuilder.ini";

const wxWindowID NModePanel::ID_SLIDER = wxNewId();

// TODO
// . The table doesn't resize properly in its panel if other tools get added
//   and so you can't find the compute button.
// . get wxgrid to scroll to the selected item.
// . connect with focus control


// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE( NModePanel, NModesGUI )

    EVT_MENU( wxID_ANY, NModePanel::OnMenuClick )
    EVT_SCROLL(NModePanel::OnEndSliderMotion)
    EVT_TEXT_ENTER(ID_SLIDER, NModePanel::OnSliderTextEnter)
    EVT_GRID_SELECT_CELL(NModePanel::OnModeSelection)

    EVT_RADIOBOX( ID_RADIOBOX_NMODE_VIZTYPE, NModePanel::OnRadioboxSelected )
    EVT_TIMER(wxID_ANY, NModePanel::OnTimer)
    EVT_PLOTCTRL_CLICKED(wxID_ANY, NModePanel::OnPlotClick)
    EVT_PLOTCTRL_POINT_CLICKED(wxID_ANY, NModePanel::OnPointClick)

END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(NModePanel, NModesGUI)


NModePanel::NModePanel()
  : NModesGUI(),
    TearableContentProvider(),
    p_grid(NULL),
    p_plotCtrl(NULL),
    p_timer(NULL),
    p_slider(NULL),
    p_selectedRow(0),
    p_loopSpeed(0),
    p_currentStep(0),
    p_mode(0),
    p_numAnimations(0),
    p_isValid(false)
{
   p_vecAmplitude = 1.0;
   p_aniAmplitude = 1.0;
}


NModePanel::NModePanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : NModesGUI(),
    TearableContentProvider(),
    p_grid(NULL),
    p_plotCtrl(NULL),
    p_timer(NULL),
    p_slider(NULL),
    p_selectedRow(0),
    p_loopSpeed(0),
    p_currentStep(0),
    p_mode(0),
    p_numAnimations(0),
    p_isValid(false)
{
   Create(calculation, parent, id, pos, size, style, name);
   p_vecAmplitude = 1.0;
   p_aniAmplitude = 1.0;
}

NModePanel::~NModePanel()
{
}



bool NModePanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
   if (!NModesGUI::Create(calculation, parent, id, pos, size, style )) {
      wxFAIL_MSG( wxT("NModePanel creation failed") );
      return false;
   }

   p_loopSpeed = 50;    // seems reasonable; overridden by prefs
   p_numAnimations = 15; // a reasonable guess
   p_currentStep = 0;
   p_isValid = false;
   p_mode = 0;

   p_timer = new wxTimer(this);
   p_plotCtrl = new ewxPlotCtrl(this, wxID_ANY);
   p_gridPlotSizer->Add(p_plotCtrl,1,wxGROW);

   ewxConfig *config = ewxConfig::getConfig(INIFILE);

   int vid = GRAPH;
   config->Read("NMode/View", &vid, GRAPH);
   if (vid == GRAPH)
      showGraph();
   else
      showTable();

   // Customizations of DB code
   p_grid = (wxGrid*)FindWindow(ID_GRID_NMODE);
   p_grid->SetRowLabelSize(0);

   wxString color = config->Read("NMode/Color", "yellow");
   ewxButton *btn = (ewxButton*)FindWindow(ID_BUTTON_NMODE_VECCOLOR);
   btn->SetBackgroundColour(wxColour(color));

   // Add the slider
   ewxPanel *p = new ewxPanel(this);
   p_sliderSizer->Add(p, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
   p_slider = new SliderCombo(p, ID_SLIDER);
   p_slider->SetRange((float)0.0, (float)30.0);
   p_slider->SetToolTip("Scale");


   // Nicer to open in vector display?
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   radbox->SetSelection(1);

   int delay;
   config->Read("NMode/Delay",&delay,20);
   ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NMODE_DELAY);
   text->setValueAsInt(delay);
   p_loopSpeed = delay;

   initialize();

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);


   return true;
}



void NModePanel::initialize()
{
   bool haveGraphable = false;
   fillTable();
   haveGraphable = fillGraph();


   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NMODE_DELAY);
   ewxNumericValidator val;
   val.setHardRange("[10,50000)");
   val.setValue(50);
   txt->SetValidator(val);

   TransferDataFromWindow();
   txt->SetValue(wxString::Format (_T("%d"), p_loopSpeed));

   showMode(p_mode);

   if (!haveGraphable) {
      // Tricky situation.  Table looks better but don't want to override
      // preferences.  Therefore call showTable.
      // Easy to swap the two options below if others disagree.
      showTable();
      //wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, TABLE);
      //AddPendingEvent(event);

   }

   getFW().getViewer().getSel()->deselectAll();
   getFW().getViewer().viewAll();
}

void NModePanel::showTable()
{
   p_gridPlotSizer->Show((size_t)0,true);
   p_gridPlotSizer->Show((size_t)1,false);
   p_gridPlotSizer->Layout();
}

void NModePanel::showGraph()
{
   p_gridPlotSizer->Show((size_t)1,true);
   p_gridPlotSizer->Show((size_t)0,false);
   p_gridPlotSizer->Layout();
}

bool NModePanel::isGraphShown()
{
   return p_gridPlotSizer->GetItem(1)->IsShown();
}

void NModePanel::OnTimer(wxTimerEvent& evt)
{
   nextStep();
}
void NModePanel::OnPointClick(wxPlotCtrlEvent& event)
{
   p_mode = event.GetCurveDataIndex()/3;
   showMode(p_mode);
}

void NModePanel::OnPlotClick(wxPlotCtrlEvent& event)
{
}

void NModePanel::showAnimationMode()
{
   p_styleSizer->Show(p_animateSizer,1);
   p_styleSizer->Show(p_vectorSizer,0);
   p_styleSizer->Layout();

   SGContainer& sg = getFW().getSceneGraph();
   sg.getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
}

void NModePanel::showVectorMode()
{
   p_styleSizer->Show(p_animateSizer,0);
   p_styleSizer->Show(p_vectorSizer,1);
   p_styleSizer->Layout();

   stop();

   // Have to update because animations overwrite the cs.
   updateVectors();

   SGContainer& sg = getFW().getSceneGraph();
   sg.getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_ALL);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
}



int NModePanel::getSelectedMode() const
{
   /* Couldn't get these methods to work so keep a variable 
   wxArrayInt selections = p_grid->GetSelectedRows();
   cout << "selected rows " << selections.size() << endl;
   int sel = 0;
   if (selections.size() > 0) 
      sel = selections[0];
   */

   wxString str =  p_grid->GetCellValue(p_selectedRow,0);
   return atoi(str.c_str());
}



bool NModePanel::fillGraph()
{
  IPropCalculation *expt = getCalculation();
  if (expt == 0) {
    wxFAIL_MSG( wxT("No calculation pointer available.") );
    return false;
  }

  // Get the required properties 
  PropVector *vec =  (PropVector*) expt->getProperty("VIBFREQ");
  PropVector *ivec = (PropVector*) expt->getProperty("VIBIR");
  PropVector *rvec = (PropVector*) expt->getProperty("VIBRAM");

  if (vec == 0) {
    wxFAIL_MSG( wxT("No frequency data available.") );
    return false;
  }


  // clear any plot(s) that might exist
  p_plotCtrl->DeleteCurve(-1, false); // -1 deletes all, true emits event

  plotCurve(vec, ivec);
  p_plotCtrl->SetKeyColour(*wxGREEN);
  plotCurve(vec, rvec);
  p_plotCtrl->SetXAxisLabel("Frequency");

  return ivec != 0 || rvec != 0;
}

void NModePanel::plotCurve(PropVector *xprop, PropVector *yprop)
{
  // Not sure if we should draw line at 1 like old calcviewer or draw
  // nothing.  Graph looks funny with points on it
  // Comment or uncomment to control this...
  //if (yprop == 0) return;

  // The times 3 is because each curve is continuous line with each spectrum
  // represented as three points in 
  //    x (val-1,val,val+1) and 
  //    y (0,val,0)
  int size = xprop->rows() * 3;


  // create and initialize the data arrays
  double *x = new double[size];
  double *y = new double[size];
  fill(x, x+size, 0);
  fill(y, y+size, 1.);

  int sidx; // y=0, x=x-1
  int vidx;  // y=y, x=x (the actual point)
  int eidx;  // y=0, x=x+1

  float val;
  int cnt = xprop->rows();
  for (int idx=0; idx<cnt; idx++) {
     sidx = idx + idx*2;
     vidx = idx + idx*2+1;
     eidx = idx + idx*2+2;
     val = xprop->value(idx);
     
     x[sidx] = val-1;
     x[vidx] = val;
     x[eidx] = val+1;

     if (y) {
        y[sidx] = 0.;
        // Can be null pointer in which case, just plot 1s
        if (yprop) 
           y[vidx] = yprop->value(idx);
        else
           y[vidx] = 1.0;
        y[eidx] = 0.;
     }
  }


  wxPlotData *plotData = new wxPlotData(x, y, size);
  p_plotCtrl->AddCurve(plotData, true, true);

}


/**
 * Fill the table and initialize the UI.
 * This code copied from motool_cdlg but the graph code is currently excluded.
 */
void NModePanel::fillTable()
{

   IPropCalculation *expt = getCalculation();

   // Clear table
   p_grid->ClearGrid();
   p_grid->DeleteCols(0,p_grid->GetNumberCols());
   p_grid->DeleteRows(0,p_grid->GetNumberRows());

   // Get the required properties 
   PropVector *vec =  (PropVector*) expt->getProperty("VIBFREQ");
   PropVector *ivec = (PropVector*) expt->getProperty("VIBIR");
   PropVector *rvec = (PropVector*) expt->getProperty("VIBRAM");

   if (vec != (PropVector*)0) {
      wxListItem itemCol;
      itemCol.SetText(_T("Frequency"));

      //label = "Frequency\n" + vec->units();
      p_grid->InsertCols(0,1);
      p_grid->SetColLabelValue(0,"Frequency\n" + vec->units());

      p_grid->InsertCols(1,1);
      p_grid->SetColLabelValue(1,"Sym");

      p_grid->InsertCols(2,1);
      p_grid->SetColLabelValue(2,"Infrared\n"+(ivec ? ivec->units(): ""));

      p_grid->InsertCols(3,1);
      p_grid->SetColLabelValue(3,"Raman\n"+(rvec ? rvec->units(): ""));


      int nRows = vec->rows();
      const vector<string> *syms = vec->rowLabels();
      INTERNALEXCEPTION(syms,"No symmetry labels - gotta have 'em");

      p_grid->InsertRows(0,nRows);

      for (int idx=0; idx<nRows; idx++) {
         p_grid->SetCellValue(idx,0,
               wxString::Format (_T(PrefLabels::DOUBLEFORMAT), vec->value(idx)));
         if (syms && syms->size()>0) {
            p_grid->SetCellValue(idx,1,(*syms)[idx].c_str());
         }

         if (ivec != (PropVector*)0) {
            p_grid->SetCellValue(idx,2,
                  wxString::Format (_T(PrefLabels::DOUBLEFORMAT), ivec->value(idx)));
         }

         if (rvec != (PropVector*)0) {
            p_grid->SetCellValue(idx,2,
                  wxString::Format (_T(PrefLabels::DOUBLEFORMAT), rvec->value(idx)));
         }
      }
   }
   p_grid->AutoSize();
}



/**
 * Show the visualization for the specified mode.
 */
void NModePanel::showMode(int index)
{
   p_currentStep = 0;
   p_mode = index;


   // Restore to proper fragment so commands use correct coordinates
   selectFragStep(-1);

   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
#ifndef INSTALL
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif
   SGContainer& sg = fw.getSceneGraph();

   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_NMODE_VECCOLOR);
   wxString color = btn->GetBackgroundColour().GetAsString(wxC2S_HTML_SYNTAX);

   wxCheckBox *tgl = (wxCheckBox*)FindWindow(ID_CHECKBOX_NMODE_VECSIGN);
   bool sign = tgl->IsChecked();

   // Note: we want to get the suggested slider value back from the command
   // and set it in the ui.
   Command *cmd = new NModeVectCmd("Normal Mode Vectors", &sg, expt);
   //cmd->getParameter("Amplitude")->setDouble(-1.); 
   cmd->getParameter("Mode")->setInteger(p_mode);
   cmd->getParameter("Color")->setString(color.c_str());
   cmd->getParameter("Sign")->setBoolean(sign);
   fw.execute(cmd);
   p_vecAmplitude = cmd->getParameter("Amplitude")->getDouble();


   Command *cmd2 = new NModeTraceCmd("Normal Mode Animation", &sg, expt);
   // No UI to support this right now...
   //cmd->getParameter("NumAnimations")->setInteger, num);
   //cmd->getParameter("Amplitude")->setInteger, num);
   cmd2->getParameter("Mode")->setInteger(p_mode);
   fw.execute(cmd2);
   p_aniAmplitude = cmd2->getParameter("Amplitude")->getDouble();

   setSlider();

   processStep(p_currentStep);

   /*
   cmdMgr()->setParam("CmdAddNormalMode","Mode",num);
   num = p_numAnimations;
   cmdMgr()->setParam("CmdAddNormalMode", "NumAnimations", num);
   viewer()->getSel()->deselectAll();
   cmdMgr()->execute("CmdAddNormalMode",cvsg());

   */
   // TODO....
   //sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_ALL);


}

void NModePanel::setSlider()
{
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   if (radbox->GetSelection() == 0) {
      p_slider->SetValue(p_aniAmplitude);
   } else {
      p_slider->SetValue(p_vecAmplitude);
   }
}

void NModePanel::processStep(int step)
{
   p_currentStep = step;

   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
#ifndef INSTALL
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif
   SGContainer& sg = fw.getSceneGraph();

   Command *cmd = new NModeStepCmd("Normal Mode Step", &sg, expt);
   cmd->getParameter("Index")->setInteger(step);

   fw.execute(cmd);
}

void NModePanel::nextStep()
{
   int step = p_currentStep + 1;
   if (step >= p_numAnimations) {
      step = 0;
   }
   processStep(step);

}

void NModePanel::previousStep()
{
   int step = p_currentStep - 1;
   if (step < 0) {
      step = p_numAnimations-1;
   }
   processStep(step);

}

void NModePanel::start()
{
   if (!p_timer->IsRunning()) {
      ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NMODE_DELAY);
      p_timer->Start(text->getValueAsInt());
   }
}

void NModePanel::stop()
{
   p_timer->Stop();
}


/**
 * Selects the specified mode.
 * The visualization is also updated (showMode)
 */
void NModePanel::selectMode(int index)
{
   p_selectedRow = index;
   p_grid->SelectRow(index);

   // This doesn't seem to work
   p_grid->MakeCellVisible(index,0);

   showMode(index);
}


/**
 * This is the callback for user selection from the MO menu.
 */
void NModePanel::OnMenuClick( wxCommandEvent& event )
{
   if (event.GetId() == GRAPH) {
       showGraph();
   } else if (event.GetId() == TABLE) {
       showTable();
   }
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Write("NMode/View", event.GetId());

   event.Skip();
}


wxWindow* NModePanel::GetTearableContent()
{
  wxToolBar *tb = new wxToolBar(this, wxID_ANY,
          wxDefaultPosition, wxDefaultSize,
          wxTB_VERTICAL|wxTB_TEXT|wxTB_NOICONS,
          "NModePanel Menu");
     
  if (isGraphShown()) {
     tb->AddTool(TABLE,"Show Table",wxNullBitmap,"Switch to Table View");
  } else {
     tb->AddTool(GRAPH,"Show Graph",wxNullBitmap,"Switch to Graph View");
  }

  return tb;
}


void NModePanel::OnTextctrlNmodeDelayEnter( wxCommandEvent& event )
{
   ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NMODE_DELAY);
   ewxConfig *config = ewxConfig::getConfig(INIFILE);
   config->Write("NMode/Delay",text->getValueAsInt());

   if (p_timer->IsRunning()) {
      stop();
      start();
   }
}




void NModePanel::OnEndSliderMotion(wxScrollEvent& event)
{
   double value = static_cast<double>(p_slider->GetFloatValue());

   WxVizToolFW& fw = getFW();
   IPropCalculation *expt = getCalculation();
#ifndef INSTALL
   ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
   INTERNALEXCEPTION(escalc, "Cannot down cast to ICalucation");
#endif
   SGContainer& sg = fw.getSceneGraph();

//TODO toggle the scene graph switches
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   if (radbox->GetSelection() == 0) {
      ; // animation
      bool restart = p_timer->IsRunning();
      stop();
      Command *cmd = new NModeTraceCmd("Normal Mode Animation", &sg, expt);
      // No UI to support this right now...
      //cmd->getParameter("NumAnimations")->setInteger, num);
      cmd->getParameter("Mode")->setInteger(p_mode);
      cmd->getParameter("Amplitude")->setDouble(value);
      fw.execute(cmd);
      if (restart) {
         start();
      }
   } else {


      updateVectors();
   }
   event.Skip();
}


void NModePanel::OnSliderTextEnter(wxCommandEvent& event)
{
  wxScrollEvent dummy;
  OnEndSliderMotion(dummy);
  event.Skip();
}




void NModePanel::receiveFocus()
{
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   if (radbox->GetSelection() == 0) {
      showAnimationMode();
   } else {
      showVectorMode();
   }
   setSlider();
}

void NModePanel::loseFocus()
{
   SGContainer& sg = getFW().getSceneGraph();
   sg.getNMVecRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getNMRoot()->whichChild.setValue(SO_SWITCH_NONE);
   sg.getcsSwitch()->whichChild.setValue(SO_SWITCH_ALL);
   p_timer->Stop();
   selectFragStep(-1); // restore in case we messed it up with animation
}

void NModePanel::OnBitmapbuttonStartClick( wxCommandEvent& event )
{
   start();
   event.Skip();
}


void NModePanel::OnBitmapbuttonStopClick( wxCommandEvent& event )
{
   stop();
   event.Skip();
}


void NModePanel::OnButtonNmodeVeccolorClick( wxCommandEvent& event )
{
   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_NMODE_VECCOLOR);
   wxColour color = btn->GetBackgroundColour();

   wxPoint pos = wxGetMousePosition();
   ewxColorDialog dlg(color, this, wxID_ANY, "ECCE Vector Color", pos);
   if (dlg.ShowModal() == wxID_OK) {
      wxString bgcolor = dlg.GetColor().GetAsString(wxC2S_HTML_SYNTAX);

      wxCheckBox *tgl = (wxCheckBox*)FindWindow(ID_CHECKBOX_NMODE_VECSIGN);
      bool sign = tgl->IsChecked();

      updateVectors(p_mode, p_slider->GetFloatValue(), sign, bgcolor.c_str());

      btn->SetBackgroundColour(bgcolor);

      ewxConfig *config = ewxConfig::getConfig(INIFILE);
      config->Write("NMode/Color", bgcolor);

   }


   event.Skip();
}


void NModePanel::OnCheckboxNmodeVecsignClick( wxCommandEvent& event )
{
   /*
   wxCheckBox *tgl = (wxCheckBox*)FindWindow(ID_CHECKBOX_NMODE_VECSIGN);
   bool sign = tgl->IsChecked();

   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_NMODE_VECCOLOR);
   wxString color = btn->GetBackgroundColour().GetAsString(wxC2S_HTML_SYNTAX);

   updateVectors(p_mode, p_slider->GetFloatValue(), sign, color.c_str());
   */
   updateVectors();

   event.Skip();
}

void NModePanel::OnRadioboxSelected( wxCommandEvent& event )
{
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   setSlider();
   if (radbox->GetSelection() == 0) {
      showAnimationMode();
   } else {
      showVectorMode();
   }
   event.Skip();
}

void NModePanel::OnModeSelection( wxGridEvent& event )
{
   p_mode = p_selectedRow = event.GetRow();
   showMode(p_mode);
   event.Skip();
}


void NModePanel::updateVectors()
{
   wxCheckBox *tgl = (wxCheckBox*)FindWindow(ID_CHECKBOX_NMODE_VECSIGN);
   bool sign = tgl->IsChecked();

   wxButton *btn = (wxButton*)FindWindow(ID_BUTTON_NMODE_VECCOLOR);
   wxString color = btn->GetBackgroundColour().GetAsString(wxC2S_HTML_SYNTAX);

   double value = static_cast<double>(p_slider->GetFloatValue());

   updateVectors(p_mode, value, sign, color.c_str());
}


void NModePanel::updateVectors(int mode, 
                               float amplitude,
                               bool sign,
                               string color)
{
   WxVizToolFW& fw = getFW();
   SGContainer& sg = fw.getSceneGraph();
   IPropCalculation *expt = getCalculation();

   NModeVectCmd *cmd = new NModeVectCmd("Normal Mode Vectors", &sg, expt);
   cmd->getParameter("Mode")->setInteger(mode);
   cmd->getParameter("Amplitude")->setDouble(amplitude);
   cmd->getParameter("Color")->setString(color.c_str());
   cmd->getParameter("Sign")->setBoolean(sign);
   fw.execute(cmd);
}
