#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <algorithm>
  using std::fill;
#include <limits>

#include <wx/link.h>
#include <wx/listctrl.h>

#include "util/EventDispatcher.H"
#include "util/InternalException.H"
#include "util/PreferenceLabels.H"

#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"

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
    EVT_UPDATE_UI( ID_RADIOBOX_NMODE_VIZTYPE, NModePanel::OnRadioboxUpdateUI )
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
    p_isValid(false),
    p_lastRadioSel(-1)
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
    p_isValid(false),
    p_lastRadioSel(-1)
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
   p_irColumn = -1;
   p_ramanColumn = -1;

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

   // Issue #81: the Animation/Vector radio box's selection never reached
   // OnRadioboxSelected() under wx3.2/GTK3 -- verified at syscall level by
   // an earlier session (the native GTK bullet moves, the handler is never
   // entered), which left the whole display-mode switch dead: no row swap,
   // no vector/animate switch, and the Play button never appears, so the
   // animation can never be started at all.
   //
   // Two things are wired up here rather than relying on the static event
   // table, which is what was failing:
   //
   // 1. A dynamic Bind() directly on the radio box instance. The static
   //    EVT_RADIOBOX entry relies on the command event propagating from
   //    the control up to this panel, and ewxRadioBox::Create() pushes an
   //    ewxHelpHandler onto the control's own handler chain
   //    (PushEventHandler), so that propagation path is not the plain one
   //    wx documents. Binding on the widget itself delivers the event to
   //    this handler directly, without depending on it.
   //
   // 2. wxWS_EX_PROCESS_UI_UPDATES, which the existing
   //    OnRadioboxUpdateUI() idle-poll fallback needs in order to receive
   //    anything at all. wxGTK does not send wxUpdateUIEvent to ordinary
   //    child controls during idle unless they ask for it, so that
   //    fallback (added 2026-09-17 and reported as making "no visible
   //    difference") was inert for a concrete reason, not a mysterious
   //    one. With the style set it becomes a real second line of defence.
   //
   // Both are kept deliberately, the same layered approach used for the
   // #78 construction-order fix: if the Bind() works the poll is a no-op,
   // and if the Bind() somehow doesn't, the poll now actually runs.
   radbox->Bind(wxEVT_RADIOBOX, &NModePanel::OnRadioboxSelected, this);
   radbox->SetExtraStyle(radbox->GetExtraStyle() | wxWS_EX_PROCESS_UI_UPDATES);

   int delay;
   config->Read("NMode/Delay",&delay,20);
   ewxTextCtrl *text = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_NMODE_DELAY);
   text->setValueAsInt(delay);
   p_loopSpeed = delay;

   // Construction is fully finished as of here -- every control
   // OnModeSelection()/showMode() touch via FindWindow() actually
   // exists, so it's safe to stop ignoring premature grid selection
   // events (see the guards in OnModeSelection() and showMode()).
   p_isValid = true;

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

      //  Only show a column for a property the code actually produced.
      //  MOPAC computes no Raman activities at all, so a permanently
      //  empty "Raman" column there is just a question the user cannot
      //  answer -- "is this blank because the job failed, or because
      //  the code cannot do it?". Reported live 2026-09-22.
      //
      //  Kept general rather than special-casing MOPAC: the panel
      //  already knows, because getProperty() returns null for a
      //  property that was never extracted. Any code missing either one
      //  now simply has no column for it.
      //
      //  The indices are tracked rather than hardcoded, because
      //  skipping Infrared while keeping Raman would otherwise leave
      //  the Raman values written to a column that does not exist.
      p_irColumn = -1;
      p_ramanColumn = -1;
      int nextCol = 2;
      if (ivec != (PropVector*)0) {
         p_grid->InsertCols(nextCol,1);
         p_grid->SetColLabelValue(nextCol,"Infrared\n"+ivec->units());
         p_irColumn = nextCol++;
      }
      if (rvec != (PropVector*)0) {
         p_grid->InsertCols(nextCol,1);
         p_grid->SetColLabelValue(nextCol,"Raman\n"+rvec->units());
         p_ramanColumn = nextCol++;
      }


      int nRows = vec->rows();

      //  The "Sym" column used to show VIBFREQ's ROW LABELS, which most
      //  parsers emit as plain mode numbers -- so it displayed 1, 2,
      //  3... instead of Mulliken symbols. Reported for MOPAC, but ORCA
      //  emits numeric row labels too, so it was never code-specific.
      //
      //  Meanwhile VIBSYM, which mopac.desc and orca.desc both extract,
      //  was not read by this panel at all: a correctly parsed property
      //  with no consumer. Read it here, and keep the row labels as the
      //  fallback for a code that carries its symmetries there instead.
      PropVecString *symvec = (PropVecString*) expt->getProperty("VIBSYM");
      const vector<string> *syms = vec->rowLabels();
      INTERNALEXCEPTION(syms,"No symmetry labels - gotta have 'em");

      p_grid->InsertRows(0,nRows);

      for (int idx=0; idx<nRows; idx++) {
         p_grid->SetCellValue(idx,0,
               wxString::Format (PrefLabels::DOUBLEFORMAT, vec->value(idx)));
         if (symvec != (PropVecString*)0 && idx < symvec->rows()) {
            p_grid->SetCellValue(idx,1,symvec->value(idx).c_str());
         } else if (syms && syms->size()>0) {
            p_grid->SetCellValue(idx,1,(*syms)[idx].c_str());
         }

         if (ivec != (PropVector*)0 && p_irColumn >= 0) {
            p_grid->SetCellValue(idx,p_irColumn,
                  wxString::Format (PrefLabels::DOUBLEFORMAT, ivec->value(idx)));
         }

         if (rvec != (PropVector*)0 && p_ramanColumn >= 0) {
            //  Its OWN column. This used to write into column 2 -- the
            //  Infrared column -- so for any code emitting both VIBIR
            //  and VIBRAM the Raman value overwrote the infrared one and
            //  the Raman column was NEVER populated, for every code.
            p_grid->SetCellValue(idx,p_ramanColumn,
                  wxString::Format (PrefLabels::DOUBLEFORMAT, rvec->value(idx)));
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
   // OnModeSelection() already guards against the premature
   // wxEVT_GRID_SELECT_CELL that wxGrid::SetTable() fires synchronously
   // during CreateControls() (see the comment there). This second check
   // is defense-in-depth for any other path into showMode() -- e.g.
   // selectMode() re-selecting a grid row -- that could reach here before
   // Create() has finished building sibling controls this function
   // depends on (p_slider, ...).
   if (!p_isValid) {
      return;
   }

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
   cmd->getParameter("Color")->setString(color.ToStdString());
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

   // Issue #99, same fault as the geometry trace: Open Inventor's redraw
   // sensor is a one-shot that re-arms on render, and with a render
   // callback installed nothing re-arms it -- so after the first step the
   // scene manager's render callback is never entered again and the view
   // freezes while the data keeps moving. Proved live on the geometry
   // trace by instrumenting SoWxRenderArea::renderCB; see
   // GeomTracePropertyPanel::processStep() for the captured log.
   //
   // This path has the identical shape (step command mutates atom
   // coordinates, then relies on notification), and vibration animation
   // was never verified end to end because the display-mode switch that
   // reveals the Play button was itself broken until recently. Fixing it
   // here too rather than waiting to reproduce the same thing twice.
   fw.getViewer().refreshRenderArea();
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
   p_lastRadioSel = radbox->GetSelection();
   if (p_lastRadioSel == 0) {
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

      updateVectors(p_mode, p_slider->GetFloatValue(), sign, bgcolor.ToStdString());

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
   p_lastRadioSel = radbox->GetSelection();
   setSlider();
   if (p_lastRadioSel == 0) {
      showAnimationMode();
   } else {
      showVectorMode();
   }
   event.Skip();
}

/**
 * Confirmed live (strace on an instrumented build, synthetic click
 * synced with the trace window): clicking the Animation/Vector radio
 * box toggles the native GTK widget's own selected bullet, but
 * OnRadioboxSelected() below is never entered -- zero writes from any
 * of its instrumented call sites, for either the real click or a
 * synced synthetic one. wxEVT_COMMAND_RADIOBOX_SELECTED is not
 * reaching this object under wx3.2/GTK3 for this control.
 *
 * This handler was added as a suspected fix: EVT_UPDATE_UI is a
 * wx-level idle-time poll, independent of whatever GTK signal wiring
 * is failing for the click event, so in theory it should still detect
 * radbox->GetSelection() changing even when the native event doesn't
 * arrive. Built, packaged, and live-tested (2026-09-17) -- no visible
 * difference; the Animation/Vector display still doesn't switch. That
 * means either this EVT_UPDATE_UI handler also isn't firing for this
 * control, or the display-switch code path itself (showAnimationMode/
 * showVectorMode) isn't the actual mechanism behind what's visible in
 * the panel, or the installed package still wasn't picking up this
 * binary for some reason not yet ruled out. Not root-caused. Next
 * session: don't assume this fix works, and don't re-spend time
 * re-deriving the "native event never arrives" fact above -- it's
 * solid (syscall-level trace, synced click) -- but treat the
 * EVT_UPDATE_UI theory and everything below it as unverified.
 */
void NModePanel::OnRadioboxUpdateUI( wxUpdateUIEvent& event )
{
   wxRadioBox *radbox = (wxRadioBox*)FindWindow(ID_RADIOBOX_NMODE_VIZTYPE);
   int sel = radbox->GetSelection();
   if (sel != p_lastRadioSel) {
      p_lastRadioSel = sel;
      setSlider();
      if (sel == 0) {
         showAnimationMode();
      } else {
         showVectorMode();
      }
   }
}

void NModePanel::OnModeSelection( wxGridEvent& event )
{
   // wxGrid::CreateGrid() (NModesGUI::CreateControls(), building the
   // placeholder 5x5 grid) fires a real EVT_GRID_SELECT_CELL synchronously
   // as part of construction, well before Create() has gotten back from
   // NModesGUI::Create() to build this panel's own sibling controls (the
   // color button, sign checkbox, ...). showMode() unconditionally
   // dereferences those via FindWindow(), so handling this premature event
   // segfaults on a null FindWindow() result -- confirmed live (crash in
   // wxWindowBase::GetBackgroundColour() called on a null button pointer,
   // via OnModeSelection -> showMode, entered from inside
   // NModesGUI::CreateControls() -> wxGrid::CreateGrid() ->
   // UpdateCurrentCellOnRedim() -> SetCurrentCell() -> SendEvent()).
   // p_isValid (already a member, previously set but never checked) is
   // the guard: Create() flips it true only once every control this
   // handler touches actually exists. The real, wanted mode-0 selection
   // happens moments later via Create()'s own initialize() -> showMode()
   // call, once p_isValid is true, so simply ignoring this early one
   // loses nothing.
   if (!p_isValid) {
      event.Skip();
      return;
   }
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

   updateVectors(p_mode, value, sign, color.ToStdString());
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
