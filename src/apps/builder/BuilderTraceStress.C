//  Scene-script commands that drive the Geometry Trace panel the way a user
//  and a running job do together (#217): every step, playback started and
//  stopped, the trace changed under a running animation, the context
//  switched or closed, the panels unfocused as on quit.  Used only through
//  ECCE_VIEWER_SCENE (tests/apps/geomtrace_stress.py).

#include <cmath>
#include <cstdio>
#include <sstream>

#include <wx/aui/aui.h>
#include <wx/app.h>
#include <wx/utils.h>

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;

#include "tdat/PropTSVecTable.H"
#include "tdat/TAtm.H"
#include "dsm/IPropCalculation.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "wxgui/ewxPlotCtrl.H"
#include "wxgui/PlaybackControl.H"
#include "wxgui/ThingToggle.H"

#include "wxviz/SceneScript.H"

#include "Builder.H"
#include "GeomTracePropertyPanel.H"
#include "PropertyPanel.H"


namespace {

GeomTracePropertyPanel *tracePanel(IPropCalculation *calc)
{
  if (!calc) return 0;
  set<PropertyPanel*> panels =
      PropertyPanel::getPanels(calc->getURL().toString());
  for (PropertyPanel *p : panels) {
    GeomTracePropertyPanel *gt = dynamic_cast<GeomTracePropertyPanel*>(p);
    if (gt) return gt;
  }
  return 0;
}


PlaybackControl *playback(wxWindow *w)
{
  if (!w) return 0;
  PlaybackControl *pb = dynamic_cast<PlaybackControl*>(w);
  if (pb) return pb;
  for (wxWindow *c : w->GetChildren())
    if ((pb = playback(c)) != 0) return pb;
  return 0;
}


void spin(int ms)
{
  wxLongLong end = wxGetLocalTimeMillis() + ms;
  do {
    wxTheApp->Yield(true);
    wxMilliSleep(2);
  } while (wxGetLocalTimeMillis() < end);
}


//  One GEOMTRACE step as eccejobstore publishes it.
string stepMessage(int step, int atoms, int values)
{
  std::ostringstream os;
  os << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n"
     << "<tsvectable columnLabel=\"Atom\" columnLabels=\"X Y Z\" columns=\"3\""
     << " name=\"GEOMTRACE\" rowLabel=\"Geometry Step\" rows=\"" << atoms
     << "\" units=\"Angstrom\" vectorLabel=\"Coordinate\" vectors=\"1\">"
     << "<step number=\"" << step << "\">";
  for (int i = 0; i < values; i++) os << " " << 0.1 * (i % 7) + 0.01 * step;
  os << "</step></tsvectable>\n";
  return os.str();
}

}  // namespace


bool Builder::traceStressCommand(SceneScript& s, const vector<string>& w)
{
  const string& c = w[0];
  GeomTracePropertyPanel *gt = tracePanel(p_calculation);
  PlaybackControl *pb = playback(gt);

  //  "gtfocus": give the panel the viewer, as a click into it does.
  if (c == "gtfocus") {
    if (!gt) {
      string have;
      for (PropertyPanel *p :
           PropertyPanel::getPanels(p_calculation->getURL().toString()))
        have += " [" + p->getName() + "]";
      return s.fail("gtfocus: no Geometry Trace panel in " +
                    p_calculation->getURL().toString() + "; panels:" + have);
    }
    gt->setFocus(true);
    spin(50);
    return true;
  }

  //  "gtsweep": a playback tick for every step and a few past either end.
  if (c == "gtsweep") {
    if (!gt || !pb) return s.fail("gtsweep: no Geometry Trace panel");
    PropTSVecTable *trace =
        dynamic_cast<PropTSVecTable*>(p_calculation->getProperty("GEOMTRACE"));
    int n = trace ? trace->tables() : 0;
    for (int i = -2; i < n + 3; i++) {
      wxCommandEvent ev(wxEVT_PLAYBACK_TICK_EVENT, pb->GetId());
      ev.SetEventObject(pb);
      ev.SetExtraLong(i);
      gt->GetEventHandler()->ProcessEvent(ev);
      spin(5);
    }
    return true;
  }

  //  "gtplay <ms> [delay ms]": press Play and let it run.
  if (c == "gtplay" && w.size() >= 2) {
    if (!pb) return s.fail("gtplay: no playback control");
    if (w.size() >= 3) pb->SetDelay(atoi(w[2].c_str()));
    wxCustomButton *b =
        dynamic_cast<wxCustomButton*>(pb->FindWindow(PlaybackControl::ID_PLAY));
    if (!b) return s.fail("gtplay: no Play button");
    b->SetValue(true);
    wxCommandEvent ev(wxEVT_TOGGLEBUTTON, PlaybackControl::ID_PLAY);
    ev.SetEventObject(b);
    ev.SetInt(1);
    pb->GetEventHandler()->ProcessEvent(ev);
    spin(atoi(w[1].c_str()));
    return true;
  }

  //  "gtstop": press Stop.
  if (c == "gtstop") {
    if (!pb) return s.fail("gtstop: no playback control");
    wxCommandEvent ev(wxEVT_BUTTON, PlaybackControl::ID_STOP);
    ev.SetEventObject(pb->FindWindow(PlaybackControl::ID_STOP));
    pb->GetEventHandler()->ProcessEvent(ev);
    spin(20);
    return true;
  }

  //  "gtupdate grow|gap|badsize|junk": a GEOMTRACE message from a running
  //  job, delivered as propertyChangeMCB does: the next step, a step that
  //  skips ahead (forces a reload), a step whose size disagrees with its
  //  header, and a link to a step the data server cannot deliver.
  if (c == "gtupdate" && w.size() == 2) {
    PropTSVecTable *trace =
        dynamic_cast<PropTSVecTable*>(p_calculation->getProperty("GEOMTRACE"));
    if (!trace) return s.fail("gtupdate: no GEOMTRACE");
    int n = trace->tables();
    int atoms = trace->rows();
    string value;
    if (w[1] == "grow") value = stepMessage(n + 1, atoms, 3 * atoms);
    else if (w[1] == "gap") value = stepMessage(n + 3, atoms, 3 * atoms);
    else if (w[1] == "badsize") value = stepMessage(n + 1, atoms, 3 * atoms + 3);
    else if (w[1] == "junk")
      value = p_calculation->getURL().toString() + "/Props/NOSUCHSTEP";
    else return s.fail("gtupdate: grow, gap, badsize or junk");
    try {
      p_calculation->updateProperty("GEOMTRACE", value);
    } catch (...) {
      return s.fail("gtupdate " + w[1] + ": updateProperty threw");
    }
    set<PropertyPanel*> panels = PropertyPanel::getPanelsForPropertyName(
        p_calculation->getURL().toString(), "GEOMTRACE");
    for (PropertyPanel *p : panels) p->propertyUpdate("GEOMTRACE");
    spin(20);
    return true;
  }

  //  "gtexpect <steps>": the cached trace still holds <steps> steps and the
  //  molecule on screen has finite coordinates.
  if (c == "gtexpect" && w.size() == 2) {
    TProperty *prop = p_calculation->getProperty("GEOMTRACE");
    PropTSVecTable *trace = dynamic_cast<PropTSVecTable*>(prop);
    int want = atoi(w[1].c_str());
    if (!trace || trace->tables() != want) {
      char buf[96];
      snprintf(buf, sizeof buf, "gtexpect: GEOMTRACE has %d steps, want %d",
               trace ? trace->tables() : -1, want);
      return s.fail(buf);
    }
    SGFragment *frag = getSG()->getFragment();
    for (size_t i = 0; i < frag->numAtoms(); i++) {
      const double *xyz = frag->atomRef(i)->coordinates();
      if (!std::isfinite(xyz[0]) || !std::isfinite(xyz[1]) ||
          !std::isfinite(xyz[2]))
        return s.fail("gtexpect: atom coordinates are not finite");
    }
    fprintf(stderr, "GTSTRESS: %d steps, %lu atoms\n", want,
            (unsigned long)frag->numAtoms());
    return true;
  }

  //  "gtcontext <url>": switch calculation, playing or not.
  if (c == "gtcontext" && w.size() == 2) {
    setContext(w[1]);
    spin(200);
    return true;
  }

  //  "gtfloat": float the trace pane, then Tools > Dock Floating Panels.
  if (c == "gtfloat") {
    if (!gt) return s.fail("gtfloat: no Geometry Trace panel");
    wxAuiPaneInfo &pane = p_mgr.GetPane(gt);
    if (!pane.IsOk()) return s.fail("gtfloat: the panel has no pane");
    pane.Show().Float();
    p_mgr.Update();
    spin(200);
    wxCommandEvent none;
    OnDockFloatingPanels(none);
    spin(200);
    return true;
  }

  //  "gtunfocusall": what quit() does to the panels before it exits.
  if (c == "gtunfocusall") {
    set<VizPropertyPanel*> panels = VizPropertyPanel::getPanels();
    for (VizPropertyPanel *p : panels) p->setFocus(false);
    spin(50);
    return true;
  }

  //  "gtremovepanels": what a state change back to "created" does to the
  //  panels (urlStateMCB), with the animation still running.
  if (c == "gtremovepanels") {
    removePropertyPanels(p_calculation->getURL().toString());
    spin(500);
    return true;
  }

  //  "gtclose": File > Close on the current calculation.
  if (c == "gtclose") {
    doClose("", true);
    spin(300);
    return true;
  }

  return s.fail("unknown command: " + c);
}
