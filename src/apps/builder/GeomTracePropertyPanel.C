#include <wx/checkbox.h>
#include <wx/collpane.h>
#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/filedlg.h>
#include <wx/choicdlg.h>
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "util/Ecce.H"
#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"
#include "inv/ChemKit/ChemColor.H"
#include "wxviz/WxVizToolFW.H"
#include <wx/listctrl.h>
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
#include "dsm/PropertyTask.H"
#include "dsm/ScanPoint.H"

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

#include "PlaybackLoop.H"
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
    p_dispPane(NULL), p_dispList(NULL), p_dispColour(NULL),
    p_dispSelect(NULL), p_dispInfo(NULL), p_dispLegend(NULL), p_dispBar(NULL),
    p_dispSavedBinding(0), p_dispPainted(false),
    p_patterns(NULL),
    p_plotCtrl(NULL),
    p_playback(NULL),
    p_propChoice(NULL),
    p_delay(500),
    p_loop(NULL),
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
    p_dispPane(NULL), p_dispList(NULL), p_dispColour(NULL),
    p_dispSelect(NULL), p_dispInfo(NULL), p_dispLegend(NULL), p_dispBar(NULL),
    p_dispSavedBinding(0), p_dispPainted(false),
    p_patterns(NULL),
    p_plotCtrl(NULL),
    p_playback(NULL),
    p_propChoice(NULL),
    p_delay(500),
    p_loop(NULL),
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
    wxString label = StepPlotAxes::plotName(pr ? pr->long_name : p_importantProps[i]);
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

  // A CREST ensemble around an ion: its coordination patterns (ecce-contacts)
  // with the lowest-energy conformer of each.
  {
    vector<ScanPoint::Pattern> pats;
    string why;
    if (ScanPoint::ensemblePatterns(dynamic_cast<PropertyTask*>(getCalculation()),
                                    pats, why)) {
      p_patterns = new wxListCtrl(this, wxID_ANY, wxDefaultPosition,
                                  wxSize(-1, 24 * ((int)pats.size() + 1)),
                                  wxLC_REPORT | wxLC_SINGLE_SEL);
      p_patterns->AppendColumn("Coordination pattern", wxLIST_FORMAT_LEFT, 260);
      p_patterns->AppendColumn("Conformers", wxLIST_FORMAT_RIGHT, 90);
      p_patterns->AppendColumn("Lowest (kcal/mol)", wxLIST_FORMAT_RIGHT, 120);
      p_patterns->AppendColumn("Conformer", wxLIST_FORMAT_RIGHT, 80);
      for (size_t i = 0; i < pats.size(); i++) {
        long row = p_patterns->InsertItem((long)i, wxString::FromUTF8(pats[i].label.c_str()));
        p_patterns->SetItem(row, 1, wxString::Format("%d", pats[i].count));
        p_patterns->SetItem(row, 2, wxString::Format("%.3f", pats[i].relative));
        p_patterns->SetItem(row, 3, wxString::Format("%d", pats[i].rank));
      }
      p_patterns->SetToolTip("Conformers grouped by first-shell donor atoms; "
          "relative energies are within the one CREST method only");
      GetSizer()->Add(p_patterns, 0, wxEXPAND|wxALL, 2);
    }
  }

  buildDisplacementPane();

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
  p_loop = new wxCheckBox(this, wxID_ANY, "Loop");
  p_loop->SetToolTip("Play forward to the last step, then backward to the "
                     "first, and repeat until stopped");
  sizer->Add(p_loop, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
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
  p_plotCtrl->setXAxisLabel(StepPlotAxes::xAxisName(prop->rowLabel(), getCalculation()));
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


vector<string> GeomTracePropertyPanel::patternRows() const
{
  vector<string> rows;
  if (!p_patterns) return rows;
  for (long i = 0; i < p_patterns->GetItemCount(); i++) {
    string r;
    for (int c = 0; c < 4; c++) {
      wxListItem it;
      it.SetId(i);
      it.SetColumn(c);
      it.SetMask(wxLIST_MASK_TEXT);
      p_patterns->GetItem(it);
      r += (c ? " | " : "") + string(it.GetText().utf8_str());
    }
    rows.push_back(r);
  }
  return rows;
}


bool GeomTracePropertyPanel::showSeries(const string& key)
{
  if (std::find(p_importantProps.begin(), p_importantProps.end(), key) ==
      p_importantProps.end())
    return false;
  selectProp(key);
  fillPlot();
  return true;
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


// With Loop on, the control's wrap from one end to the other becomes a
// reversal, as in the IRC Path and Relaxed Scan panels.
void GeomTracePropertyPanel::OnPlaybackLoop(wxCommandEvent& event)
{
  const long n = numSteps();
  if (p_loop && p_loop->GetValue() && n > 0) {
    const bool wasForward = event.GetInt() != 0;
    const playbackloop::Pos p = playbackloop::next(
        wasForward ? playbackloop::Pos{n - 1, 1} : playbackloop::Pos{0, -1}, n);
    p_playback->SetStepSize(p.dir);
    p_playback->SetCurrentStep(p.cur);
    processStep(p.cur);
    return;
  }
  OnPlaybackTick(event);
}


// An MD run stores at most 100 frames spread evenly over the whole run, but
// one plot point per logged energy step, so the click is mapped by position
// along the run, not by index.  Other trace plots have one point per frame.
int GeomTracePropertyPanel::frameOfPlotPoint(int index, int points) const
{
  const int frames = numSteps();
  if (frames <= 0) return 0;
  if (points == frames || points < 2 || frames < 2 ||
      !getCalculation()->getProperty("TIMEVEC"))
    return std::max(0, std::min(index, frames - 1));
  const double f = (double)index * (frames - 1) / (points - 1);
  return std::max(0, std::min((int)(f + 0.5), frames - 1));
}


int GeomTracePropertyPanel::plotPointOfFrame(int frame, int points) const
{
  const int frames = numSteps();
  if (points == frames || points < 2 || frames < 2 ||
      !getCalculation()->getProperty("TIMEVEC"))
    return frame;
  return (int)((double)frame * (points - 1) / (frames - 1) + 0.5);
}


void GeomTracePropertyPanel::OnPointClick(int, int index)
{
  showPlotPoint(index, p_plotCtrl->pointCount(0));
}


void GeomTracePropertyPanel::showPlotPoint(int index, int points)
{
  const int frame = frameOfPlotPoint(index, points);
  // Play continues from the clicked step, not from the first.
  if (p_playback) p_playback->SetCurrentStep((long)frame);
  processStep(frame);
  const int shownPoint = plotPointOfFrame(frame, points);
  if (shownPoint == index) return;

  // No stored structure at this point: say which one is shown.
  std::vector<double> t;
  wxString unit;
  wxString where;
  if (StepPlotAxes::mdTime(getCalculation(), points, t, unit) &&
      index < (int)t.size() && shownPoint < (int)t.size())
    where = wxString::Format(" (%.4g %s) for the one at %.4g %s", t[shownPoint],
                             unit, t[index], unit);
  getFW().showMessage(std::string(wxString::Format(
      "No structure is stored at that point; showing the nearest in time, "
      "structure %d of %d%s.", frame + 1, numSteps(), where).ToUTF8()), false);
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
  const int plotPoint = plotPointOfFrame(step, p_plotCtrl->pointCount(0));
  if (plotPoint < p_plotCtrl->pointCount(0))
    p_plotCtrl->setSelectedPoint(0, plotPoint);

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
  if (p_dispColour && p_dispColour->GetValue()) paintDisplacement(true);
  processStep(p_playback->GetCurrentStep());
}


void GeomTracePropertyPanel::loseFocus()
{
  paintDisplacement(false);
  p_playback->Pause();
  processStep(numSteps()-1);
}



// ---- Displacement (#701): the numbers are ecce-displacement's ------------

namespace {

//  A scratch calculation folder holding Props/GEOMTRACE of `calc`, which is
//  what ecce-displacement reads; rowLabels carry the elements.
bool writeTraceFolder(IPropCalculation *calc, const std::filesystem::path& dir)
{
  PropTSVecTable *trace =
      dynamic_cast<PropTSVecTable*>(calc->getProperty("GEOMTRACE"));
  PropertyTask *task = dynamic_cast<PropertyTask*>(calc);
  if (!trace || !task || trace->tables() < 1) return false;
  Fragment *frag = task->fragment();
  if (!frag) return false;
  const int n = trace->rows();
  if ((int)frag->numAtoms() != n) { delete frag; return false; }
  std::error_code ec;
  std::filesystem::create_directories(dir / "Props", ec);
  std::ofstream f(dir / "Props" / "GEOMTRACE");
  if (!f) { delete frag; return false; }
  f << "<?xml version=\"1.0\"?>\n<tsvectable columns=\"3\" name=\"GEOMTRACE\" "
       "rowLabels=\"";
  for (int i = 0; i < n; i++)
    f << (i ? " " : "") << i + 1 << "-" << frag->atomRef(i)->atomicSymbol();
  f << "\" rows=\"" << n << "\" units=\"Angstrom\">\n";
  char buf[64];
  for (int t = 0; t < trace->tables(); t++) {
    f << "<step number=\"" << t + 1 << "\">";
    const vector<double>& v = trace->values(t);
    for (size_t k = 0; k < v.size(); k++) {
      snprintf(buf, sizeof buf, "%s%.10e", k ? " " : "", v[k]);
      f << buf;
    }
    f << "</step>\n";
  }
  f << "</tsvectable>\n";
  delete frag;
  return (bool)f;
}

string shellQuote(const string& s)
{
  string q;
  for (char c : s) if (c != '\'' && c != '"') q += c;
  return "'" + q + "'";
}

}  // namespace


bool GeomTracePropertyPanel::runDisplacement()
{
  p_dispCols.clear();
  p_dispTable.clear();
  p_dispSummary.clear();
  IPropCalculation *calc = getCalculation();
  static int serial = 0;
  const std::filesystem::path tmp = std::filesystem::temp_directory_path() /
      ("ecce-disp-" + std::to_string((long)std::filesystem::file_time_type::clock::now().time_since_epoch().count()) + "-" +
       std::to_string(serial++));
  std::error_code ec;
  string cmdArgs;
  bool ok = writeTraceFolder(calc, tmp / "a");
  if (ok) cmdArgs = shellQuote((tmp / "a").string());
  if (ok && !p_dispCompare.empty()) {
    ok = false;
    vector<string> labels;
    vector<IPropCalculation*> found = comparable(&labels);
    for (size_t i = 0; i < found.size() && !ok; i++)
      if (labels[i] == p_dispCompare)
        ok = writeTraceFolder(found[i], tmp / "b");
    if (ok) cmdArgs += " " + shellQuote((tmp / "b").string());
  }
  if (!ok) {
    p_dispSummary = p_dispCompare.empty() ? "No geometry trace to analyse."
        : "The two calculations do not have the same atoms.";
    std::filesystem::remove_all(tmp, ec);
    return false;
  }
  if (!p_dispSelection.empty())
    cmdArgs += " --select " + shellQuote(p_dispSelection);

  const char *home = getenv("ECCE_HOME");
  string script = home && *home ? string(home) + "/bin/ecce-displacement"
                                : string("ecce-displacement");
#ifdef _WIN32
  std::filesystem::path py = std::filesystem::path(Ecce::ecceHome()) / "python" / "python.exe";
  string pyExe = std::filesystem::exists(py) ? py.string() : string("python.exe");
  string cmd = "\"" + pyExe + "\" \"" + script + "\" " + cmdArgs + " --csv";
#else
  string cmd = "'" + script + "' " + cmdArgs + " --csv 2>/dev/null";
#endif
  string text;
  int status = Ecce::readCommand(cmd, text);
  std::filesystem::remove_all(tmp, ec);
  if (status != 0) {
    p_dispSummary = "ecce-displacement could not be run (check the selection).";
    return false;
  }
  //  First line: the summary; then a CSV header and one row per atom.
  std::istringstream in(text);
  string line;
  std::getline(in, p_dispSummary);
  bool header = true;
  while (std::getline(in, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty()) continue;
    vector<string> fields;
    std::istringstream ls(line);
    string fld;
    while (std::getline(ls, fld, ',')) fields.push_back(fld);
    if (header) { p_dispCols = fields; header = false; }
    else p_dispTable.push_back(fields);
  }
  return !p_dispCols.empty();
}


vector<IPropCalculation*> GeomTracePropertyPanel::comparable(
    vector<string> *labels)
{
  vector<IPropCalculation*> out;
  vector<string> names;
  IPropCalculation *calc = getCalculation();
  PropTSVecTable *own =
      dynamic_cast<PropTSVecTable*>(calc ? calc->getProperty("GEOMTRACE") : 0);
  Resource *folder = calc ? EDSIFactory::getResource(calc->getURL().getParent()) : 0;
  vector<Resource*> *kids = folder ? folder->getChildren() : 0;
  for (size_t i = 0; kids && i < kids->size(); i++) {
    IPropCalculation *other = dynamic_cast<IPropCalculation*>((*kids)[i]);
    if (!other || other->getURL().toString() == calc->getURL().toString())
      continue;
    PropTSVecTable *t = dynamic_cast<PropTSVecTable*>(other->getProperty("GEOMTRACE"));
    if (!t || !own || t->rows() != own->rows() || t->tables() < 1) continue;
    out.push_back(other);
    names.push_back(other->getURL().getFilePathTail());
  }
  if (labels) *labels = names;
  return out;
}


vector<string> GeomTracePropertyPanel::comparableNames()
{
  vector<string> names;
  comparable(&names);
  return names;
}


vector<string> GeomTracePropertyPanel::displacementRows()
{
  if (p_dispTable.empty()) runDisplacement();
  vector<string> rows;
  for (size_t i = 0; i < p_dispTable.size(); i++) {
    string r;
    for (size_t c = 0; c < p_dispTable[i].size(); c++)
      r += (c ? " | " : "") + p_dispTable[i][c];
    rows.push_back(r);
  }
  return rows;
}


bool GeomTracePropertyPanel::setDisplacementSelection(const string& spec)
{
  p_dispSelection = spec;
  bool ok = runDisplacement();
  showDisplacement();
  return ok;
}


bool GeomTracePropertyPanel::compareWith(const string& name)
{
  const string before = p_dispCompare;
  p_dispCompare = name;
  bool ok = runDisplacement();
  if (!ok && !name.empty()) {
    p_dispCompare = before;
    string why = p_dispSummary;
    runDisplacement();
    p_dispSummary = why;
  }
  showDisplacement();
  if (p_dispColour && p_dispColour->GetValue()) paintDisplacement(true);
  return ok;
}


bool GeomTracePropertyPanel::exportDisplacement(const string& path) const
{
  std::ofstream f(path);
  if (!f || p_dispCols.empty()) return false;
  for (size_t c = 0; c < p_dispCols.size(); c++)
    f << (c ? "," : "") << p_dispCols[c];
  f << "\n";
  for (size_t i = 0; i < p_dispTable.size(); i++) {
    for (size_t c = 0; c < p_dispTable[i].size(); c++)
      f << (c ? "," : "") << p_dispTable[i][c];
    f << "\n";
  }
  return (bool)f;
}


//  The scale and its legend: pale yellow (none) through orange to dark red.
static void displacementColour(double t, float rgb[3])
{
  static const float stop[3][3] = {{1.00f, 0.95f, 0.70f},
                                   {0.99f, 0.55f, 0.20f},
                                   {0.55f, 0.05f, 0.10f}};
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  const int k = t < 0.5 ? 0 : 1;
  const float u = (float)(t < 0.5 ? t * 2 : t * 2 - 1);
  for (int i = 0; i < 3; i++)
    rgb[i] = stop[k][i] + u * (stop[k + 1][i] - stop[k][i]);
}


void GeomTracePropertyPanel::paintDisplacement(bool on)
{
  ChemColor *colors = getFW().getSceneGraph().getColorNode();
  if (!colors) return;
  if (!on) {
    if (!p_dispPainted) return;
    for (size_t i = 0; i * 3 < p_dispSaved.size(); i++)
      colors->atomColor.set1Value((int)i, p_dispSaved[3*i], p_dispSaved[3*i+1],
                                  p_dispSaved[3*i+2]);
    colors->atomColorBinding.setValue(p_dispSavedBinding);
    p_dispPainted = false;
    getFW().getViewer().refreshRenderArea();
    return;
  }
  if (p_dispTable.empty()) runDisplacement();
  //  The value column: "max" for one calculation, "displacement" after a fit.
  int vc = -1;
  for (size_t c = 0; c < p_dispCols.size(); c++)
    if (p_dispCols[c] == "max" || p_dispCols[c] == "displacement") vc = (int)c;
  if (vc < 0) return;
  const int n = getFW().getSceneGraph().getFragment()->numAtoms();
  if (!p_dispPainted) {
    p_dispSaved.clear();
    for (int i = 0; i < colors->atomColor.getNum(); i++)
      for (int k = 0; k < 3; k++) p_dispSaved.push_back(colors->atomColor[i][k]);
    p_dispSavedBinding = colors->atomColorBinding.getValue();
  }
  double top = 0;
  vector<double> value(n, -1);
  for (size_t r = 0; r < p_dispTable.size(); r++) {
    int idx = atoi(p_dispTable[r][0].c_str()) - 1;
    if (idx < 0 || idx >= n) continue;
    value[idx] = atof(p_dispTable[r][vc].c_str());
    top = std::max(top, value[idx]);
  }
  for (int i = 0; i < n; i++) {
    float rgb[3] = {0.8f, 0.8f, 0.8f};      // not in the selection
    if (value[i] >= 0) displacementColour(top > 0 ? value[i] / top : 0, rgb);
    colors->atomColor.set1Value(i, rgb[0], rgb[1], rgb[2]);
  }
  colors->atomColorBinding.setValue(ChemColor::ATOM_PER_ATOM);
  p_dispPainted = true;
  if (p_dispLegend)
    p_dispLegend->SetLabel(wxString::Format("0 to %.3f Å", top));
  getFW().getViewer().refreshRenderArea();
}


void GeomTracePropertyPanel::colourByDisplacement(bool on)
{
  if (p_dispColour) p_dispColour->SetValue(on);
  paintDisplacement(on);
}


vector<string> GeomTracePropertyPanel::atomColourRows() const
{
  vector<string> rows;
  ChemColor *colors = const_cast<GeomTracePropertyPanel*>(this)->getFW()
                          .getSceneGraph().getColorNode();
  if (!colors) return rows;
  const int n = const_cast<GeomTracePropertyPanel*>(this)->getFW()
                    .getSceneGraph().getFragment()->numAtoms();
  for (int i = 0; i < n && i < colors->atomColor.getNum(); i++)
    rows.push_back(string(wxString::Format("%d %.3f %.3f %.3f",
        (int)colors->atomColorBinding.getValue(), colors->atomColor[i][0],
        colors->atomColor[i][1], colors->atomColor[i][2]).utf8_str()));
  return rows;
}


void GeomTracePropertyPanel::buildDisplacementPane()
{
  p_dispPane = new wxCollapsiblePane(this, wxID_ANY, "Displacement");
  p_dispPane->Collapse(true);
  wxWindow *pane = p_dispPane->GetPane();
  wxBoxSizer *col = new wxBoxSizer(wxVERTICAL);
  p_dispList = new wxListCtrl(pane, wxID_ANY, wxDefaultPosition,
                              wxSize(-1, 120), wxLC_REPORT);
  col->Add(p_dispList, 1, wxEXPAND | wxALL, 2);
  p_dispInfo = new wxStaticText(pane, wxID_ANY, "");
  col->Add(p_dispInfo, 0, wxEXPAND | wxALL, 2);

  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  p_dispColour = new wxCheckBox(pane, wxID_ANY, "Colour by displacement");
  p_dispColour->SetToolTip("Colour each atom by how far it moved from the "
      "first frame (or, when comparing, after alignment); off restores the "
      "normal colours");
  row->Add(p_dispColour, 0, wxALIGN_CENTER_VERTICAL);
  p_dispBar = new wxPanel(pane, wxID_ANY, wxDefaultPosition, wxSize(90, 14));
  p_dispBar->Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
    wxPaintDC dc(p_dispBar);
    const wxSize sz = p_dispBar->GetClientSize();
    for (int x = 0; x < sz.x; x++) {
      float rgb[3];
      displacementColour(sz.x > 1 ? (double)x / (sz.x - 1) : 0, rgb);
      dc.SetPen(wxPen(wxColour((int)(rgb[0]*255), (int)(rgb[1]*255),
                               (int)(rgb[2]*255))));
      dc.DrawLine(x, 0, x, sz.y);
    }
  });
  row->Add(p_dispBar, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
  p_dispLegend = new wxStaticText(pane, wxID_ANY, "0 to max Å");
  row->Add(p_dispLegend, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
  col->Add(row, 0, wxALL, 2);

  row = new wxBoxSizer(wxHORIZONTAL);
  wxButton *cmp = new wxButton(pane, wxID_ANY, "Compare with...");
  row->Add(cmp, 0, wxALIGN_CENTER_VERTICAL);
  row->Add(new wxStaticText(pane, wxID_ANY, "Atoms:"), 0,
           wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
  p_dispSelect = new wxTextCtrl(pane, wxID_ANY, "", wxDefaultPosition,
                                wxDefaultSize, wxTE_PROCESS_ENTER);
  p_dispSelect->SetHint("all, or 1 3 5, or within 3.0 of 1");
  p_dispSelect->SetToolTip("Restrict the table, the fit and the colours: "
      "1-based atom numbers, or 'within R of N' (Enter applies)");
  row->Add(p_dispSelect, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
  wxButton *exp = new wxButton(pane, wxID_ANY, "Export CSV...");
  row->Add(exp, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
  col->Add(row, 0, wxEXPAND | wxALL, 2);
  pane->SetSizer(col);

  p_dispPane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED,
                   &GeomTracePropertyPanel::OnDispExpand, this);
  p_dispColour->Bind(wxEVT_CHECKBOX, &GeomTracePropertyPanel::OnDispColour, this);
  cmp->Bind(wxEVT_BUTTON, &GeomTracePropertyPanel::OnDispCompare, this);
  p_dispSelect->Bind(wxEVT_TEXT_ENTER, &GeomTracePropertyPanel::OnDispSelect, this);
  exp->Bind(wxEVT_BUTTON, &GeomTracePropertyPanel::OnDispExport, this);
  GetSizer()->Add(p_dispPane, 0, wxEXPAND | wxALL, 2);
}


void GeomTracePropertyPanel::showDisplacement()
{
  if (!p_dispList) return;
  p_dispList->ClearAll();
  for (size_t c = 0; c < p_dispCols.size(); c++) {
    wxString h = wxString::FromUTF8(p_dispCols[c].c_str());
    if (p_dispCols[c] == "max" || p_dispCols[c] == "rms" ||
        p_dispCols[c] == "displacement")
      h += " (Å)";
    p_dispList->AppendColumn(h, wxLIST_FORMAT_RIGHT, 90);
  }
  for (size_t r = 0; r < p_dispTable.size(); r++) {
    long row = p_dispList->InsertItem((long)r,
        wxString::FromUTF8(p_dispTable[r][0].c_str()));
    for (size_t c = 1; c < p_dispTable[r].size(); c++)
      p_dispList->SetItem(row, (int)c, wxString::FromUTF8(p_dispTable[r][c].c_str()));
  }
  p_dispInfo->SetLabel(wxString::FromUTF8(p_dispSummary.c_str()));
  p_dispPane->GetPane()->Layout();
}


void GeomTracePropertyPanel::OnDispExpand(wxCollapsiblePaneEvent&)
{
  if (!p_dispPane->IsCollapsed() && p_dispTable.empty()) {
    runDisplacement();
    showDisplacement();
  }
  Layout();
}


void GeomTracePropertyPanel::OnDispColour(wxCommandEvent& event)
{
  paintDisplacement(event.IsChecked());
}


void GeomTracePropertyPanel::OnDispSelect(wxCommandEvent&)
{
  setDisplacementSelection(string(p_dispSelect->GetValue().utf8_str()));
  if (p_dispColour->GetValue()) paintDisplacement(true);
}


void GeomTracePropertyPanel::OnDispCompare(wxCommandEvent&)
{
  vector<string> names = comparableNames();
  if (names.empty()) {
    wxMessageBox("No other calculation in this folder has a geometry trace "
                 "with the same atoms.", "Compare", wxOK | wxICON_INFORMATION, this);
    return;
  }
  wxArrayString choices;
  choices.Add("(this calculation alone)");
  for (size_t i = 0; i < names.size(); i++)
    choices.Add(wxString::FromUTF8(names[i].c_str()));
  const int at = wxGetSingleChoiceIndex(
      "Align this calculation's final structure with the last structure of:",
      "Compare with", choices, this);
  if (at < 0) return;
  compareWith(at == 0 ? string() : names[at - 1]);
}


void GeomTracePropertyPanel::OnDispExport(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Export displacements", "", "displacement.csv",
                   "CSV files (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() == wxID_OK) exportDisplacement(string(dlg.GetPath().utf8_str()));
}
