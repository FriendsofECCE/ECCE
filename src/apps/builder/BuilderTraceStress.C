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
#include <wx/radiobox.h>
#include <wx/utils.h>

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;

#include "tdat/PropTSVecTable.H"
#include "tdat/TAtm.H"
#include "dsm/IPropCalculation.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "viz/VRVector.H"

#include "XYPlotCanvas.H"
#include "wxgui/PlaybackControl.H"
#include "wxgui/ThingToggle.H"

#include "wxviz/SceneScript.H"

#include "Builder.H"
#include "GeomTracePropertyPanel.H"
#include "wxgui/SliderCombo.H"
#include "NModePanel.H"
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


XYPlotCanvas *plotOf(wxWindow *w)
{
  if (!w) return 0;
  XYPlotCanvas *p = dynamic_cast<XYPlotCanvas*>(w);
  if (p) return p;
  for (wxWindow *c : w->GetChildren())
    if ((p = plotOf(c)) != 0) return p;
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

  //  "gttitle <text>": the plot's title contains <text> (the series shown).
  if (c == "gttitle" && w.size() >= 2) {
    XYPlotCanvas *plot = plotOf(gt);
    string want = w[1];
    for (size_t i = 2; i < w.size(); i++) want += " " + w[i];
    if (!plot) return s.fail("gttitle: no plot");
    string title = string(plot->title().ToUTF8());
    if (title.find(want) == string::npos)
      return s.fail("gttitle: plot shows '" + title + "', want '" + want + "'");
    return true;
  }

  //  "gtpick <step|mid|last>": click the plot where that step's point is
  //  drawn (zero-based; mid and last of the plotted points), as a user does; the molecule on screen must then be the
  //  one the trace holds for that step.
  if (c == "gtpick" && w.size() == 2) {
    XYPlotCanvas *plot = plotOf(gt);
    PropTSVecTable *trace =
        dynamic_cast<PropTSVecTable*>(p_calculation->getProperty("GEOMTRACE"));
    if (!plot || !trace) return s.fail("gtpick: no plot or no trace");
    const int count = plot->pointCount(0);
    int step = atoi(w[1].c_str());
    if (w[1] == "mid") step = count / 2;
    if (w[1] == "last") step = count - 1;
    if (step < 0 || step >= count || step >= trace->tables())
      return s.fail("gtpick: step out of range");
    wxPoint at;
    if (!plot->pointPosition(0, step, &at))
      return s.fail("gtpick: the point is not drawn");
    wxMouseEvent down(wxEVT_LEFT_DOWN), up(wxEVT_LEFT_UP);
    down.SetEventObject(plot);
    up.SetEventObject(plot);
    down.SetPosition(at);
    up.SetPosition(at);
    down.SetLeftDown(true);
    plot->GetEventHandler()->ProcessEvent(down);
    plot->GetEventHandler()->ProcessEvent(up);
    spin(50);
    SGFragment *frag = getSG()->getFragment();
    for (size_t i = 0; i < frag->numAtoms() && (int)i < trace->rows(); i++) {
      const double *xyz = frag->atomRef(i)->coordinates();
      for (int k = 0; k < 3; k++) {
        const double want = trace->value(step, i, k);
        if (std::fabs(xyz[k] - want) > 1e-3 * (1 + std::fabs(want))) {
          char buf[160];
          snprintf(buf, sizeof buf, "gtpick %d: atom %lu coordinate %d is "
                   "%g, the trace has %g", step, (unsigned long)i, k, xyz[k],
                   want);
          return s.fail(buf);
        }
      }
    }
    fprintf(stderr, "GTSTRESS: picked step %d\n", step);
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


//  Scene-script commands that drive the Normal Modes panel as a user does,
//  after the structure may have been edited away from the calculation's.
//  Used only through ECCE_VIEWER_SCENE (tests/apps/nmode_edit_test.py).
bool Builder::nmodeTestCommand(SceneScript& s, const vector<string>& w)
{
  const string& c = w[0];
  NModePanel *nm = 0;
  if (p_calculation)
    for (PropertyPanel *p :
         PropertyPanel::getPanels(p_calculation->getURL().toString()))
      if ((nm = dynamic_cast<NModePanel*>(p)) != 0) break;
  if (!nm) return s.fail(c + ": no Normal Modes panel");
  fprintf(stderr, "VIBTEST: %s\n", c.c_str());

  //  "vibfocus" / "vibunfocus": the panel takes or gives up the viewer.
  if (c == "vibfocus" || c == "vibunfocus") {
    nm->setFocus(c == "vibfocus");
    spin(50);
    return true;
  }
  //  "vibmode <i>": a click on row i of the mode table.
  if (c == "vibmode" && w.size() == 2) {
    nm->selectMode(atoi(w[1].c_str()));
    spin(50);
    return true;
  }
  //  "vibanim" / "vibvector": the Animation/Vector radio box.
  if (c == "vibanim" || c == "vibvector") {
    wxRadioBox *box = dynamic_cast<wxRadioBox*>(
        nm->FindWindow(NModesGUI::ID_RADIOBOX_NMODE_VIZTYPE));
    if (!box) return s.fail(c + ": no Animation/Vector control");
    box->SetSelection(c == "vibanim" ? 0 : 1);
    wxCommandEvent ev(wxEVT_RADIOBOX, box->GetId());
    ev.SetEventObject(box);
    ev.SetInt(box->GetSelection());
    box->GetEventHandler()->ProcessEvent(ev);
    spin(50);
    return true;
  }
  //  "vibplay <ms>": Play for that long, then Stop.
  if (c == "vibplay" && w.size() == 2) {
    nm->start();
    spin(atoi(w[1].c_str()));
    nm->stop();
    return true;
  }
  //  "vibscale <v>": v typed into the Arrow length / Amplitude field.
  if (c == "vibscale" && w.size() == 2) {
    fprintf(stderr, "VIBSCALE: %.2f\n", nm->typeScale(atof(w[1].c_str())));
    return true;
  }
  //  "vibspace": the Space key in the viewer.
  if (c == "vibspace") {
    fprintf(stderr, "VIBSPACE: %d\n", pauseVibrationKey() ? 1 : 0);
    spin(50);
    return true;
  }
  //  "vibstep <n>": n animation steps, as the timer makes them.
  if (c == "vibstep" && w.size() == 2) {
    for (int i = 0; i < atoi(w[1].c_str()); i++) {
      nm->nextStep();
      spin(5);
    }
    return true;
  }
  //  "vibstate": what the panel and the viewer show, for the test to read.
  if (c == "vibstate") {
    SGContainer *sg = getSG();
    //  Sum of the coordinates: equal before and after only if restored.
    double sum = 0;
    SGFragment *f = sg->getFragment();
    for (size_t i = 0; i < f->numAtoms(); i++) {
      const double *x = f->atomCoords(i);
      sum += x[0] + 2*x[1] + 3*x[2];
    }
    fprintf(stderr, "VIBSTATE: atoms=%lu arrows=%d enabled=%d animating=%d "
            "paused=%d coords=%.5f\n", (unsigned long)f->numAtoms(),
            sg->getNMVecRoot()->getNumChildren(), nm->modesApply() ? 1 : 0,
            nm->isAnimating() ? 1 : 0, nm->isPaused() ? 1 : 0, sum);
    return true;
  }
  //  "vibstarts": per arrow, the distance from its start to its atom's
  //  centre and the sphere radius the atom is drawn with (#312).
  if (c == "vibstarts") {
    const string tag = w.size() > 1 ? w[1] : "";
    SGContainer *sg = getSG();
    SGFragment *frag = sg->getFragment();
    SoSwitch *root = sg->getNMVecRoot();
    for (int k = 0; k < root->getNumChildren() && k < (int)frag->numAtoms(); k++) {
      VRVector *v = dynamic_cast<VRVector*>(root->getChild(k));
      if (!v) continue;
      const double *p = v->getPosition();
      const double *x = frag->atomRef(k)->coordinates();
      double r = sqrt((p[0]-x[0])*(p[0]-x[0]) + (p[1]-x[1])*(p[1]-x[1]) +
                      (p[2]-x[2])*(p[2]-x[2]));
      fprintf(stderr, "VIBSTART: %s atom=%d %s start=%.4f sphere=%.4f which=%d len=%.6f head=%.6f\n", tag.c_str(), k,
              frag->atomRef(k)->atomicSymbol().c_str(), r,
              sg->displayedSphereRadius(k), (int)root->whichChild.getValue(), sqrt(v->getDirection()[0]*v->getDirection()[0]+v->getDirection()[1]*v->getDirection()[1]+v->getDirection()[2]*v->getDirection()[2]), v->headRadius());
    }
    return true;
  }
  //  "vibdirs": each drawn atom and the arrow on it, for tests that check
  //  the arrows numerically (tests/apps/mopac_import_modes_test.py).
  if (c == "vibdirs") {
    SGFragment *frag = getSG()->getFragment();
    SoSwitch *root = getSG()->getNMVecRoot();
    for (int k = 0; k < root->getNumChildren() && k < (int)frag->numAtoms(); k++) {
      VRVector *v = dynamic_cast<VRVector*>(root->getChild(k));
      if (!v) continue;
      const double *x = frag->atomRef(k)->coordinates();
      const double *d = v->getDirection();
      fprintf(stderr, "VIBDIR: %d %s %.6f %.6f %.6f %.6f %.6f %.6f\n", k,
              frag->atomRef(k)->atomicSymbol().c_str(), x[0], x[1], x[2],
              d[0], d[1], d[2]);
    }
    return true;
  }
  //  "vibframe": whether the arrows sit in the frame of the drawn atoms.
  //  The hydrogen with the longest arrow in a stretch moves along its own
  //  X-H bond, so cos(arrow, bond) is 1 when atoms and modes share a frame
  //  and is arbitrary when they do not.
  if (c == "vibframe") {
    SGFragment *frag = getSG()->getFragment();
    SoSwitch *root = getSG()->getNMVecRoot();
    int n = root->getNumChildren();
    int best = -1;
    double longest = 0.0;
    for (int k = 0; k < n && k < (int)frag->numAtoms(); k++) {
      VRVector *v = dynamic_cast<VRVector*>(root->getChild(k));
      if (!v || frag->atomRef(k)->atomicSymbol() != "H") continue;
      const double *d = v->getDirection();
      double len = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
      if (len > longest) { longest = len; best = k; }
    }
    double cosine = -1.0;
    if (best >= 0) {
      const double *h = frag->atomRef(best)->coordinates();
      const double *d = dynamic_cast<VRVector*>(root->getChild(best))
                            ->getDirection();
      int heavy = -1;
      double near = 1.0e30;
      for (int k = 0; k < (int)frag->numAtoms(); k++) {
        if (frag->atomRef(k)->atomicSymbol() == "H") continue;
        const double *x = frag->atomRef(k)->coordinates();
        double r = (x[0]-h[0])*(x[0]-h[0]) + (x[1]-h[1])*(x[1]-h[1]) +
                   (x[2]-h[2])*(x[2]-h[2]);
        if (r < near) { near = r; heavy = k; }
      }
      if (heavy >= 0) {
        const double *x = frag->atomRef(heavy)->coordinates();
        double b[3] = {h[0]-x[0], h[1]-x[1], h[2]-x[2]};
        double dot = d[0]*b[0] + d[1]*b[1] + d[2]*b[2];
        cosine = fabs(dot) / (longest * sqrt(near));
      }
    }
    fprintf(stderr, "VIBFRAME: atom=%d cos=%.4f\n", best, cosine);
    return true;
  }
  return s.fail("unknown command: " + c);
}
