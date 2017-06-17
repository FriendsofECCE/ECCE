#include <algorithm>
  using std::copy;
  using std::fill;

#include "wx/link.h"
#include "wx/log.h"
#include "wx/sizer.h"

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
#include "wxgui/ewxPlotCtrl.H"
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
  EVT_PLOTCTRL_POINT_CLICKED(wxID_ANY, GeomTracePropertyPanel::OnPointClick)
  EVT_COMMAND(wxID_ANY, wxEVT_VALIDATION_EVENT,
              GeomTracePropertyPanel::OnValidation)
END_EVENT_TABLE()


GeomTracePropertyPanel::GeomTracePropertyPanel()
  : VizPropertyPanel(),
    p_plotCtrl(NULL),
    p_playback(NULL),
    p_delay(500),
    p_recompute(false),
    p_fold(true),
    p_currentProp(""),
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
    p_delay(500),
    p_recompute(false),
    p_fold(false),
    p_currentProp(""),
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

  // create our wxPlotCtrl, add to sizer
  p_plotCtrl = new ewxPlotCtrl(this);
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
       tb->AddCheckTool(p_keyToId[*key], long_name, wxNullBitmap);
       tb->ToggleTool(p_keyToId[*key], long_name == p_plotCtrl->GetPlotTitle());
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
    // find first important prop that actually exists to fill plot
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
  config->Write("GeomTrace/Prop", p_currentProp);
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
  PropVector *prop = dynamic_cast<PropVector*>(
          getCalculation()->getProperty(p_currentProp));

  if (!prop) return; // bail if bad prop

  // clear any plots that might exist, don't send event
  p_plotCtrl->DeleteCurve(-1, false);

  // wxPlotData expects a pointer to an array
  size_t size = prop->rows();
  double *arrayData = new double[size];
  fill(arrayData, arrayData+size, 0);
  vector<double> &vectorData = prop->values();
  copy(vectorData.begin(), vectorData.end(), arrayData);
  
  // We also need our "step" data
  double *stepData = new double[size];
  for (size_t i = 0; i < size; i++) {
    stepData[i] = i+1;
  }

  // create our curve and add to plot control
  wxPlotData plotData(stepData, arrayData, size);
  p_plotCtrl->AddCurve(plotData, true, true);

  // add and show the axes labels
  p_plotCtrl->SetXAxisLabel(prop->rowLabel());
  p_plotCtrl->SetYAxisLabel(prop->units());
  p_plotCtrl->SetShowXAxisLabel(true);
  p_plotCtrl->SetShowYAxisLabel(true);

  // add and show the plot title
  const Property_Ref *propref = PropFactory::getPropRef(p_currentProp);
  if (propref) {
    p_plotCtrl->SetPlotTitle(propref->long_name);
    p_plotCtrl->SetShowPlotTitle(true);
  }
}


void GeomTracePropertyPanel::OnToolClick(wxCommandEvent& event)
{
  if (event.GetId() == ID_CHECKBOX_BONDS) {
    p_recompute = !p_recompute;
  } else if (event.GetId() == ID_CHECKBOX_FOLD) {
    p_fold = event.IsChecked();
  } else {
    p_currentProp = p_idToKey[event.GetId()];
    fillPlot();
  }
  saveSettings();
}


void GeomTracePropertyPanel::OnPlaybackTick(wxCommandEvent& event)
{
  processStep(event.GetExtraLong());
}


void GeomTracePropertyPanel::OnPlaybackLoop(wxCommandEvent& event)
{
  OnPlaybackTick(event);
}


void GeomTracePropertyPanel::OnPointClick(wxPlotCtrlEvent& event)
{
  processStep(event.GetCurveDataIndex());
}


void GeomTracePropertyPanel::OnValidation(wxCommandEvent& event)
{
  saveSettings();
}


// Commands executed directly instead of via fw.execute() because
// we don't want/need undo
void GeomTracePropertyPanel::processStep(int step)
{
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

  p_plotCtrl->SetCursorDataIndex(0, step);

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
    if (propref->classType == TProperty::PROPTSVECTABLE) {
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

