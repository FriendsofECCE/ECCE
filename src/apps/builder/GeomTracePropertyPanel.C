#include "util/UnitDisplay.H"
#include <algorithm>
  using std::copy;
  using std::fill;

#include "wx/link.h"
#include "wx/log.h"
#include "wx/sizer.h"
#include "wx/choice.h"

#include "util/EventDispatcher.H"
#include "util/Event.H"
#include "util/Transformations.H"
  using namespace ecce;

#include "tdat/PropTSVecTable.H"
#include "tdat/PropVector.H"
#include "tdat/TProperty.H"
#include "tdat/LatticeDef.H"

#include "dsm/IPropCalculation.H"
#include "dsm/PropFactory.H"

#include "viz/GTStepCmd.H"
#include "viz/PBCFoldCmd.H"
#include "viz/PBCReplicateCmd.H"

#include "wxgui/ewxConfig.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "XYPlotCanvas.H"
#include "StepPlotAxes.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/GenericValidator.H"
#include "wxgui/NumericTextCtrl.H"
#include "wxgui/NumericValidator.H"
#include "wxgui/PlaybackControl.H"

#include "GeomTracePropertyPanel.H"


static const char *INIFILE = "wxbuilder.ini";


wxFORCE_LINK_THIS_MODULE(GeomTracePropertyPanel)


IMPLEMENT_DYNAMIC_CLASS(GeomTracePropertyPanel, VizPropertyPanel)


const wxWindowID GeomTracePropertyPanel::ID_CHECKBOX_BONDS = wxNewId();
const wxWindowID GeomTracePropertyPanel::ID_CHECKBOX_FOLD = wxNewId();
const wxWindowID GeomTracePropertyPanel::ID_DELAY = wxNewId();
const wxWindowID GeomTracePropertyPanel::ID_PLAYBACK = wxNewId();


BEGIN_EVENT_TABLE(GeomTracePropertyPanel, VizPropertyPanel)
  EVT_COMMAND(ID_PLAYBACK, wxEVT_PLAYBACK_TICK_EVENT,
              GeomTracePropertyPanel::OnPlaybackTick)
  EVT_COMMAND(ID_PLAYBACK, wxEVT_PLAYBACK_LOOP_EVENT,
              GeomTracePropertyPanel::OnPlaybackLoop)
  EVT_TOOL(ID_CHECKBOX_BONDS, GeomTracePropertyPanel::OnToolClick)
  EVT_TOOL(ID_CHECKBOX_FOLD, GeomTracePropertyPanel::OnToolClick)
  EVT_COMMAND(wxID_ANY, wxEVT_VALIDATION_EVENT,
              GeomTracePropertyPanel::OnValidation)
END_EVENT_TABLE()


GeomTracePropertyPanel::GeomTracePropertyPanel()
  : VizPropertyPanel(),
    p_plotCtrl(NULL),
    p_playback(NULL),
    p_propChoice(NULL),
    p_delay(500),
    p_recompute(false),
    p_fold(true),
    p_currentProp(""),
    p_propChosen(false),
    p_importantProps(),
    p_keyToId(),
    p_idToKey()
{
}


GeomTracePropertyPanel::GeomTracePropertyPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : VizPropertyPanel(),
    p_plotCtrl(NULL),
    p_playback(NULL),
    p_propChoice(NULL),
    p_delay(500),
    p_recompute(false),
    p_fold(false),
    p_currentProp(""),
    p_propChosen(false),
    p_importantProps(),
    p_keyToId(),
    p_idToKey()
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool GeomTracePropertyPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!VizPropertyPanel::Create(calculation, parent, id, pos, size, style,
                                name)) {
    wxFAIL_MSG( wxT("GeomTracePropertyPanel creation failed") );
    return false;
  }

  return true;
}


GeomTracePropertyPanel::~GeomTracePropertyPanel()
{
  map<string,wxWindowID>::iterator string_to_id_it;
  for (string_to_id_it = p_keyToId.begin();
          string_to_id_it != p_keyToId.end();
          ++string_to_id_it) {
    GetEventHandler()->Disconnect(string_to_id_it->second,
            wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(GeomTracePropertyPanel::OnToolClick));
  }
}


void GeomTracePropertyPanel::initialize()
{
  set<string> names;
  set<string>::iterator name;
  map<string,wxWindowID>::iterator string_to_id_it;
  TProperty * prop;
  wxBoxSizer * sizer;
  ewxStaticText * stext;
  NumericTextCtrl * text;

  if (!GetSizer()) {
    SetSizer(new wxBoxSizer(wxVERTICAL));
  }

  // there should be only one PropTSVecTable, but many PropTSVectors
  names = getPropertyNames();
  if (names.empty()) {
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY,
            "No properties registered for GeomTracePropertyPanel"));
    return;
  }

  // build the "important props" vector, which is all of the PropTSVectors
  // so really our "names" minus the PropTSVecTable
  for (name = names.begin(); name != names.end(); ++name) {
    string key = *name;
    prop = getCalculation()->getProperty(key);
    if (dynamic_cast<PropTSVecTable*>(prop)) {
      continue;
    }
    //  The time series is an axis, not a quantity to plot.
    if (key == "TIMEVEC" && prop && prop->units() == "ps") continue;
    //  The panel is registered for every Geometry-Step series ECCE knows;
    //  offer only those this calculation holds.
    PropVector *vec = dynamic_cast<PropVector*>(prop);
    if (!vec || vec->rows() == 0) continue;
    p_importantProps.push_back(key);
    wxWindowID id = wxNewId();
    p_keyToId[key] = id;
    p_idToKey[id] = key;
  }

  // event handling for dynamic menu
  for (string_to_id_it = p_keyToId.begin();
          string_to_id_it != p_keyToId.end();
          ++string_to_id_it) {
    GetEventHandler()->Connect(string_to_id_it->second,
            wxEVT_COMMAND_TOOL_CLICKED,
            wxCommandEventHandler(GeomTracePropertyPanel::OnToolClick));
  }

  // Every logged series is one choice away; the tear-off menu holds them
  // too but is easy to miss.
  sizer = new wxBoxSizer(wxHORIZONTAL);
  sizer->Add(new ewxStaticText(this, wxID_ANY, "Plot:"), 0,
             wxALIGN_CENTER_VERTICAL|wxRIGHT, 6);
  p_propChoice = new wxChoice(this, wxID_ANY);
  for (size_t i = 0; i < p_importantProps.size(); i++) {
    const Property_Ref *pr = PropFactory::getPropRef(p_importantProps[i]);
    string label = StepPlotAxes::plotName(pr ? pr->long_name : p_importantProps[i]);
    //  Several series share a long name (Kinetic Energy from different codes).
    for (size_t j = 0; j < p_importantProps.size(); j++) {
      const Property_Ref *pj = PropFactory::getPropRef(p_importantProps[j]);
      if (j != i && pr && pj && pj->long_name == pr->long_name) {
        label += " (" + p_importantProps[i] + ")";
        break;
      }
    }
    p_propChoice->Append(label);
  }
  p_propChoice->Bind(wxEVT_CHOICE, &GeomTracePropertyPanel::OnPropChoice, this);
  sizer->Add(p_propChoice, 1, wxALIGN_CENTER_VERTICAL);
  GetSizer()->Add(sizer, 0, wxEXPAND|wxALL, 2);

  // create our wxPlotCtrl, add to sizer
  p_plotCtrl = new XYPlotCanvas(this);
  p_plotCtrl->setPointHandler([this](int, int index) { OnPointClick(0, index); });
  GetSizer()->Add(p_plotCtrl, 1, wxEXPAND);

  sizer = new wxBoxSizer(wxHORIZONTAL);
  // delay
  stext = new ewxStaticText(this, wxID_ANY, "Delay:");
  sizer->Add(stext, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
  text = new NumericTextCtrl(this, ID_DELAY);
  text->SetValidator(NumericValidator<int>(&p_delay, 1, 10000).Units("ms"));
  sizer->Add(text, 1, wxALIGN_CENTER_VERTICAL);
  // playback area
  p_playback = new PlaybackControl(this, ID_PLAYBACK);
  p_playback->SetDelay(&p_delay);
  p_playback->SetStopIndex(numSteps()-1);
  FindWindow(PlaybackControl::ID_RECORD)->Hide();
  sizer->Add(p_playback, 0, wxALIGN_CENTER_VERTICAL);
  GetSizer()->Add(sizer, 0);

  restoreSettings();
  selectProp(p_currentProp);

  TransferDataToWindow();

  fillPlot();

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);
}


wxWindow * GeomTracePropertyPanel::GetTearableContent()
{
  wxToolBar *tb = new wxToolBar(this, wxID_ANY,
          wxDefaultPosition, wxDefaultSize,
          wxTB_VERTICAL|wxNO_BORDER|wxTB_TEXT|wxTB_NOICONS,
          getName() + " Menu");

  // any prop that is indexed by geometry step is viable to show
  vector<string>::iterator key;
  for (key = p_importantProps.begin(); key != p_importantProps.end(); ++key) {
    TProperty *prop = getCalculation()->getProperty(*key);
    if (prop) {
       const Property_Ref *propref = PropFactory::getPropRef(*key);
       wxString long_name = propref ? propref->long_name : *key;
       const wxString shown = StepPlotAxes::plotName(long_name.ToStdString());
       tb->AddCheckTool(p_keyToId[*key], shown, wxNullBitmap);
       tb->ToggleTool(p_keyToId[*key], *key == p_currentProp);
    }
  }
  tb->AddSeparator();
  tb->AddCheckTool(ID_CHECKBOX_BONDS, "Recompute bonds at each step",
                   wxNullBitmap, wxNullBitmap);
  tb->ToggleTool(ID_CHECKBOX_BONDS, p_recompute);
  tb->AddCheckTool(ID_CHECKBOX_FOLD, "Fold into lattice at each step",
                   wxNullBitmap, wxNullBitmap);
  tb->ToggleTool(ID_CHECKBOX_FOLD, p_fold);

  tb->Realize();

  return tb;
}


void GeomTracePropertyPanel::refresh()
{
  if (!p_playback) return;   // not initialized yet
  p_playback->SetStopIndex(numSteps()-1);
  fillPlot();
}


void GeomTracePropertyPanel::restoreSettings()
{
  ewxConfig *config = ewxConfig::getConfig(INIFILE);
  config->Read("GeomTrace/Recompute",&p_recompute,false);
  config->Read("GeomTrace/Fold",&p_fold,false);
  config->Read("GeomTrace/Delay",&p_delay,500);
  p_currentProp = config->Read("GeomTrace/Prop","");
  if (p_currentProp.empty() || !getCalculation()->getProperty(p_currentProp)) {
    // Open on the total energy when there is one; else the first series.
    if (getCalculation()->getProperty("TEVEC") && p_keyToId.count("TEVEC")) {
      p_currentProp = "TEVEC";
      return;
    }
    map<string,wxWindowID>::iterator string_to_id_it;
    for (string_to_id_it = p_keyToId.begin();
            string_to_id_it != p_keyToId.end();
            ++string_to_id_it) {
      if (getCalculation()->getProperty(string_to_id_it->first)) {
        p_currentProp = string_to_id_it->first;
        break;
      }
    }
  }
}


void GeomTracePropertyPanel::saveSettings()
{
  ewxConfig *config = ewxConfig::getConfig(INIFILE);
  config->Write("GeomTrace/Recompute", p_recompute);
  config->Write("GeomTrace/Fold", p_fold);
  config->Write("GeomTrace/Delay", p_delay);
  if (p_propChosen)
    config->Write("GeomTrace/Prop", wxString(p_currentProp));
}


bool GeomTracePropertyPanel::isRelevant(IPropCalculation *propCalc) const
{
  bool ret = false;

  if (propCalc) {
    PropTSVecTable *trace = (PropTSVecTable*)propCalc->getProperty(getTracePropKey());
    if (trace && trace->tables() > 1) {
      ret = true;
    }
  }

  return ret;
}


void GeomTracePropertyPanel::fillPlot()
{
  //  A code that stores a trace but no per-step series (Quantum ESPRESSO)
  //  leaves nothing to plot.  Asking for a property named "" fetches the
  //  whole Props folder, and the answer crashed the parser.
  if (p_currentProp.empty()) return;

  PropVector *prop = dynamic_cast<PropVector*>(
          getCalculation()->getProperty(p_currentProp));

  if (!prop) return; // bail if bad prop

  // clear any plots that might exist, don't send event
  p_plotCtrl->clearCurves();

  size_t size = prop->rows();
  vector<double> arrayData(size, 0.0), stepData(size);
  vector<double> &vectorData = prop->values();
  copy(vectorData.begin(), vectorData.begin() + std::min(size, vectorData.size()),
       arrayData.begin());
  for (size_t i = 0; i < size; i++) stepData[i] = i+1;

  //  An MD run is read against its simulation time; any other against the
  //  step.
  wxString timeUnit;
  const bool timed = StepPlotAxes::mdTime(getCalculation(), size, stepData,
                                          timeUnit);
  const Property_Ref *propref = PropFactory::getPropRef(p_currentProp);
  const wxString name = propref ? StepPlotAxes::plotName(propref->long_name)
                                : wxString();
  p_plotCtrl->setShowKey(false);
  p_plotCtrl->setIntegerX(false);
  const StepPlotAxes::Shaped shaped =
      timed ? StepPlotAxes::Shaped()
            : StepPlotAxes::shape(p_currentProp, prop->units(), arrayData);
  if (!timed && !shaped.y.empty()) arrayData = shaped.y;
  const int curve = p_plotCtrl->addCurve(stepData, arrayData,
                                         timed ? name : wxString());
  for (size_t i = 0; i < shaped.hover.size(); i++)
    p_plotCtrl->setPointLabel(curve, (int)i, shaped.hover[i]);
  if (timed) StepPlotAxes::addMovingAverage(p_plotCtrl, stepData, arrayData, timeUnit);

  const wxString yUnit = StepPlotAxes::unitText(prop->units());
  if (timed) {
    p_plotCtrl->setXAxisLabel("Time (" + timeUnit + ")");
    p_plotCtrl->setReadout("Frame", "Time", timeUnit, "", yUnit);
    p_plotCtrl->setYAxisLabel(yUnit);
    if (propref) p_plotCtrl->setTitle(name);
    return;
  }

  p_plotCtrl->setIntegerX(true);
  p_plotCtrl->setXAxisLabel(StepPlotAxes::xAxisName(prop->rowLabel()));
  p_plotCtrl->setReadout("Step", "", "", shaped.yName, shaped.yUnit);
  p_plotCtrl->setYAxisLabel(shaped.yLabel.empty() ? yUnit : shaped.yLabel);
  if (propref)
    p_plotCtrl->setTitle(shaped.titleNote.empty()
                         ? name : name + " (" + shaped.titleNote + ")");
}


void GeomTracePropertyPanel::OnToolClick(wxCommandEvent& event)
{
  if (event.GetId() == ID_CHECKBOX_BONDS) {
    p_recompute = !p_recompute;
  } else if (event.GetId() == ID_CHECKBOX_FOLD) {
    p_fold = event.IsChecked();
  } else {
    selectProp(p_idToKey[event.GetId()]);
    p_propChosen = true;
    fillPlot();
  }
  saveSettings();
}


void GeomTracePropertyPanel::OnPropChoice(wxCommandEvent& event)
{
  const int i = event.GetSelection();
  if (i < 0 || i >= (int)p_importantProps.size()) return;
  selectProp(p_importantProps[i]);
  p_propChosen = true;
  fillPlot();
  saveSettings();
}


void GeomTracePropertyPanel::selectProp(const string& key)
{
  p_currentProp = key;
  if (!p_propChoice) return;
  const vector<string>::iterator it =
      std::find(p_importantProps.begin(), p_importantProps.end(), key);
  if (it != p_importantProps.end())
    p_propChoice->SetSelection((int)(it - p_importantProps.begin()));
}


void GeomTracePropertyPanel::OnPlaybackTick(wxCommandEvent& event)
{
  processStep(event.GetExtraLong());
}


void GeomTracePropertyPanel::OnPlaybackLoop(wxCommandEvent& event)
{
  OnPlaybackTick(event);
}


void GeomTracePropertyPanel::OnPointClick(int, int index)
{
  // Play continues from the clicked step, not from the first.
  if (p_playback) p_playback->SetCurrentStep((long)index);
  processStep(index);
}


void GeomTracePropertyPanel::OnValidation(wxCommandEvent& event)
{
  saveSettings();
}


// Commands executed directly instead of via fw.execute() because
// we don't want/need undo
void GeomTracePropertyPanel::processStep(int step)
{
  // The playback position can outlive the trace it was set for (a reload
  // after a message from a running job), and a trace can be empty.
  int steps = numSteps();
  if (steps == 0) return;
  if (step < 0) step = 0;
  if (step >= steps) step = steps - 1;

  WxVizToolFW& fw = getFW();
  IPropCalculation *expt = getCalculation();
  SGContainer& sg = fw.getSceneGraph();
  SGFragment *frag = sg.getFragment();
  LatticeDef *lattice = frag->getLattice();
  int numAtoms = frag->numAtoms();

  int x,y,z = 1; // lattice replication factors

  if (lattice) {
     lattice->getReplicationFactors(x,y,z);
     if (x*y*z > 1) {
        frag->resize(numAtoms / (x * y * z));
     }
  }

  Command *cmd = new GTStepCmd("Trace Step", &sg, expt);
  cmd->getParameter("Index")->setInteger(step);
  cmd->getParameter("PropKey")->setString(getTracePropKey());
  cmd->execute();
  delete cmd;

  if (p_recompute) {
     frag->deleteAllBonds();
     frag->addCovalentBonds();
     frag->touchNumbers();
     sg.touchChemDisplay();
     // This may be inefficient because the GTStepCmd sends the StepChange
     // message.  Here we send another.  Could remove from GTStepCmd
     // to optimize.
     EventDispatcher::getDispatcher().publish(Event("StepChange"));
  }
  if (p_fold && lattice) {
     // We don't want to add this to the undo so just execute directly
     Command *cmd = new PBCFoldCmd("Trace Fold", &sg, frag);
     cmd->execute();
     delete cmd;
  }

  // Now possibly replicate
  if (lattice) {
     if (x*y*z > 1) {
        vector<MPoint> *vecs = lattice->toVectors();
        MPoint a1 = (*vecs)[0];
        MPoint a2 = (*vecs)[1];
        MPoint a3 = (*vecs)[2];
        delete vecs;
        cmd = new PBCReplicateCmd("Replicate", &sg, frag);
        cmd->getParameter("axis1")->setInteger(x);
        cmd->getParameter("axis2")->setInteger(y);
        cmd->getParameter("axis3")->setInteger(z);
        cmd->getParameter("x1")->setDouble(a1.x());
        cmd->getParameter("y1")->setDouble(a1.y());
        cmd->getParameter("z1")->setDouble(a1.z());
        cmd->getParameter("x2")->setDouble(a2.x());
        cmd->getParameter("y2")->setDouble(a2.y());
        cmd->getParameter("z2")->setDouble(a2.z());
        cmd->getParameter("x3")->setDouble(a3.x());
        cmd->getParameter("y3")->setDouble(a3.y());
        cmd->getParameter("z3")->setDouble(a3.z());
        cmd->execute();
        delete cmd;
        frag = sg.getFragment();
     }
  }

  // The plotted series (TEVEC, ...) may have fewer points than the trace.
  if (step < p_plotCtrl->pointCount(0)) p_plotCtrl->setSelectedPoint(0, step);

  // Issue #99: drive the repaint directly instead of relying on Open
  // Inventor's change notification, which stops firing after the first
  // step.
  //
  // Proved by instrumenting SoWxRenderArea::renderCB (the scene manager's
  // render callback) alongside GTStepCmd and stepping a five-frame trace
  // live. GTStepCmd ran every time with correctly changing coordinates,
  // and SGFragment::getAtomCoordinates() reads TAtm live rather than from
  // a cached copy, so the scene always had fresh data to draw -- yet
  // renderCB was entered for step 0 and then NOT ONCE for the thirteen
  // steps that followed:
  //
  //   [GEOMTRACE] step=0 ...   [RENDERCB] inPaint=0 -> refresh
  //   [GEOMTRACE] step=1 ...   (no RENDERCB)
  //   [GEOMTRACE] step=2 ...   (no RENDERCB)
  //   ... eleven more, none
  //
  // So the fault was never the render cache (disabling caching
  // process-wide changed nothing), never the data path, and never
  // OnPaint's p_redrawPending retry -- the redraw was simply never
  // requested, because the scene manager's redraw sensor is a one-shot
  // that re-arms on render, and with a render callback installed nothing
  // re-armed it.
  //
  // refreshRenderArea() forces a wx paint, which reaches redraw() and
  // renders the scene, so it does not depend on that sensor at all.
  // Deliberately unconditional rather than inside the p_recompute branch
  // where sg.touchChemDisplay() sits: the geometry moves on every step,
  // whether or not bonds are being recomputed.
  fw.getViewer().refreshRenderArea();

  ::wxYieldIfNeeded();
}


int GeomTracePropertyPanel::numSteps() const
{
  int ret = 0;

  PropTSVecTable * trace = dynamic_cast<PropTSVecTable*>(
          getCalculation()->getProperty(getTracePropKey()));
  if (trace) {
    ret = trace->tables();
  }

  return ret;
}


string GeomTracePropertyPanel::getTracePropKey() const
{
  set<string> names = getPropertyNames();
  set<string>::iterator name;
  for (name = names.begin(); name != names.end(); ++name) {
    string key = *name;
    const Property_Ref *propref = PropFactory::getPropRef(key);
    if (propref && propref->classType == TProperty::PROPTSVECTABLE) {
      return key;
    }
  }
  return "";
}


void GeomTracePropertyPanel::receiveFocus()
{
  processStep(p_playback->GetCurrentStep());
}


void GeomTracePropertyPanel::loseFocus()
{
  p_playback->Pause();
  processStep(numSteps()-1);
}

