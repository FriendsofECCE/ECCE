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
#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/PropertyTask.H"
#include "dsm/ResourceType.H"
#include "tdat/Fragment.H"
#include "tdat/GUIValues.H"
#include "dsm/VDoc.H"
#include "viz/VRVector.H"

#include "XYPlotCanvas.H"
#include "wxgui/PlaybackControl.H"
#include <wx/checkbox.h>
#include "wxgui/ThingToggle.H"

#include "wxviz/SceneScript.H"

#include "Builder.H"
#include "GeomTracePropertyPanel.H"
#include "EnsemblePropertyPanel.H"
#include "wxgui/ewxChoice.H"
#include "PlotAnyPropertyPanel.H"
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


bool Builder::traceFrame(int index)
{
  GeomTracePropertyPanel *gt = tracePanel(p_calculation);
  if (!gt || !plotOf(gt)) return false;
  gt->showFrame(index);
  spin(50);
  PlaybackControl *pb = playback(gt);
  fprintf(stderr, "GTSTRESS: step %d -> playback %ld, plot marker %d of %d\n", index,
          pb ? pb->GetCurrentStep() : -1, plotOf(gt)->selectedIndex(),
          plotOf(gt)->pointCount(0));
  return true;
}


bool Builder::traceStressCommand(SceneScript& s, const vector<string>& w)
{
  const string& c = w[0];
  GeomTracePropertyPanel *gt = tracePanel(p_calculation);
  PlaybackControl *pb = playback(gt);

  //  "gtframe <n>" (0-based; "last"): show that frame as gtstep does, and
  //  report the playback position and the plot's marker (stderr).
  if (c == "gtframe" && w.size() == 2) {
    XYPlotCanvas *plot = plotOf(gt);
    if (!gt || !plot) return s.fail("gtframe: no Geometry Trace panel");
    const int frame = w[1] == "last" ? gt->frameCount() - 1 : atoi(w[1].c_str());
    return traceFrame(frame);
  }

  //  "gtinview": whether an atom is outside the 3-D view, the atoms and
  //  whether there is a cell (stderr).
  if (c == "gtinview") {
    SGFragment *frag = getSG() ? getSG()->getFragment() : 0;
    fprintf(stderr, "GTSTRESS: inview outside=%d atoms=%d lattice=%s\n",
            atomsOutsideView() ? 1 : 0, frag ? (int)frag->numAtoms() : -1,
            frag && frag->getLattice() ? "yes" : "no");
    return true;
  }

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

  //  "gtclick <index>": a click on plot point <index> of the Geometry Trace
  //  plot; "gtstepclick <index>": the same in Geometry Step Plots.  Both
  //  report the structure now shown and the selected point.
  if ((c == "gtclick" || c == "gtstepclick") && w.size() == 2) {
    XYPlotCanvas *plot = plotOf(gt);
    if (!gt || !plot) return s.fail(c + ": no Geometry Trace panel");
    int index = atoi(w[1].c_str());
    if (w[1] == "last") index = plot->pointCount(0) - 1;
    if (c == "gtclick") {
      gt->showPlotPoint(index, plot->pointCount(0));
    } else {
      PlotAnyPropertyPanel *any = 0;
      for (PropertyPanel *p :
           PropertyPanel::getPanels(p_calculation->getURL().toString()))
        if (p->getName() == "Geometry Step Plots" &&
            (any = dynamic_cast<PlotAnyPropertyPanel*>(p)) != 0) break;
      if (!any) return s.fail("gtstepclick: no Geometry Step Plots panel");
      any->OnPointClick(index);
    }
    spin(50);
    fprintf(stderr, "GTSTRESS: click %d -> structure %ld of %d, plot point %d\n",
            index, pb ? pb->GetCurrentStep() + 1 : -1,
            gt->frameCount(), plot->selectedIndex());
    return true;
  }

  //  Ensemble panel: "gtenrows" prints the table and the panel's kind;
  //  "gtencalc <type> <rank>..." makes new calculations (#708);
  //  "gtenselect <rank>" shows that member and prints atom 1; "gtencsv <path>".
  //  "gtenkind <runtype, _ for space> <0|1 scored>": the entry name for a run type.
  if (c == "gtenkind" && w.size() == 3) {
    string rt = w[1];
    for (size_t i = 0; i < rt.size(); i++) if (rt[i] == '_') rt[i] = ' ';
    fprintf(stderr, "GTSTRESS: enkind %s = %s\n", w[1].c_str(),
            EnsemblePropertyPanel::kindFor(rt, w[2] == "1").c_str());
    return true;
  }
  if (c == "gtenrows" || c == "gtencsv" || c == "gtenselect" ||
      c == "gtencalc" || c == "gtenfit" || c == "gtenpng" ||
      c == "gtenthumbs" || c == "gtensort" || c == "gtenviewer") {
    EnsemblePropertyPanel *en = dynamic_cast<EnsemblePropertyPanel*>(gt);
    if (!en) return s.fail(c + ": no ensemble panel");
    if (c == "gtenrows") {
      vector<string> rows = en->tableRows();
      fprintf(stderr, "GTSTRESS: ensemble kind=%s rows=%d\n", en->kind().c_str(),
              (int)rows.size());
      for (size_t i = 0; i < rows.size(); i++)
        fprintf(stderr, "GTSTRESS: enrow %s\n", rows[i].c_str());
    } else if (c == "gtencsv" && w.size() == 2) {
      fprintf(stderr, "GTSTRESS: encsv %s\n", en->exportCsv(w[1]) ? "ok" : "failed");
    } else if (c == "gtenpng" && w.size() == 2) {
      if (!en->exportPng(w[1])) return s.fail("gtenpng: render or write failed");
      fprintf(stderr, "GTSTRESS: enpng ok\n");
    } else if (c == "gtenthumbs") {
      for (int rank : en->thumbnailRanks())
        fprintf(stderr, "GTSTRESS: enthumb %d\n", rank);
    } else if (c == "gtenviewer" && w.size() == 2) {
      wxCheckBox *box = en->viewerToggle();
      if (!box) return s.fail("gtenviewer: no viewer toggle");
      const bool on = w[1] == "on";
      box->SetValue(on);
      wxCommandEvent event(wxEVT_CHECKBOX, box->GetId());
      event.SetEventObject(box);
      event.SetInt(on);
      box->GetEventHandler()->ProcessEvent(event);
    } else if (c == "gtensort" && w.size() == 3) {
      en->sortTable(atoi(w[1].c_str()), w[2] == "up");
    } else if (c == "gtenfit") {
      fprintf(stderr, "GTSTRESS: enfit members=%d visible=%d\n",
              (int)en->superpositionRows().size(), (int)en->superpositionVisible());
      for (const string& row : en->superpositionRows())
        fprintf(stderr, "GTSTRESS: enfit %s\n", row.c_str());
    } else if (c == "gtenselect" && w.size() >= 2) {
      vector<int> ranks;
      for (size_t i = 1; i < w.size(); i++) ranks.push_back(atoi(w[i].c_str()));
      const bool ok = en->selectRanks(ranks);
      spin(50);
      const double *p = getSG()->getFragment()->atomRef(0)->coordinates();
      fprintf(stderr, "GTSTRESS: enselect %s ok=%d atom1 %.6f %.6f %.6f plot=%d\n",
              w[1].c_str(), (int)ok, p[0], p[1], p[2],
              plotOf(en) ? plotOf(en)->selectedIndex() + 1 : -1);
    } else if (c == "gtencalc" && w.size() >= 2) {
      vector<int> ranks;
      for (size_t i = 2; i < w.size(); i++) ranks.push_back(atoi(w[i].c_str()));
      string why;
      vector<string> urls = en->newCalcsFromRanks(ranks, w[1], why);
      for (size_t i = 0; i < urls.size(); i++) {
        // Read the new calculation back: its name and first/last atom.
        PropertyTask *nt = dynamic_cast<PropertyTask*>(
            EDSIFactory::getResource(EcceURL(urls[i])));
        Fragment *nf = nt ? nt->fragment() : 0;
        const int na = nf ? nf->numAtoms() : 0;
        const double *a = na ? nf->atomRef(0)->coordinates() : 0;
        const double *z = na ? nf->atomRef(na - 1)->coordinates() : 0;
        ICalculation *ic = dynamic_cast<ICalculation*>(nt);
        fprintf(stderr, "GTSTRESS: encalc %d type=%s run=%s mult=%d name=%s atoms=%d first %.6f %.6f %.6f "
                "last %.6f %.6f %.6f\n", ranks[i],
                nt && nt->getDescriptor() ? nt->getDescriptor()->getName().c_str() : "",
                ic ? ic->runtype().name().c_str() : "", ic ? (int)ic->spinMultiplicity() : 0,
                EcceURL(urls[i]).getFilePathTail().c_str(), na,
                a ? a[0] : 0, a ? a[1] : 0, a ? a[2] : 0,
                z ? z[0] : 0, z ? z[1] : 0, z ? z[2] : 0);
      }
      fprintf(stderr, "GTSTRESS: encalc made=%d why=%s\n", (int)urls.size(), why.c_str());
    } else {
      return s.fail("bad ensemble command");
    }
    return true;
  }

  //  "gtpatterns": the coordination-pattern table, one row per line.
  if (c == "gtpatterns") {
    if (!gt) return s.fail("gtpatterns: no Geometry Trace panel");
    vector<string> rows = gt->patternRows();
    fprintf(stderr, "GTSTRESS: patterns=%d\n", (int)rows.size());
    for (size_t i = 0; i < rows.size(); i++)
      fprintf(stderr, "GTSTRESS: pattern %s\n", rows[i].c_str());
    return true;
  }

  //  Displacement panel (#701).  "gtdisp": its rows and summary;
  //  "gtdispcolour on|off": colour by displacement, then the atom colours;
  //  "gtdispcompare <name>" ("-" ends it); "gtdispselect <spec>" ("-" clears);
  //  "gtdispexport <file>".
  if (c.compare(0, 6, "gtdisp") == 0) {
    if (!gt) return s.fail(c + ": no Geometry Trace panel");
    string rest;
    for (size_t i = 1; i < w.size(); i++) rest += (i > 1 ? " " : "") + w[i];
    if (rest == "-") rest.clear();
    bool ok = true;
    if (c == "gtdispcolour") gt->colourByDisplacement(rest == "on");
    else if (c == "gtdispcompare") ok = gt->compareWith(rest);
    else if (c == "gtdispselect") ok = gt->setDisplacementSelection(rest);
    else if (c == "gtdispexport") ok = gt->exportDisplacement(rest);
    else if (c != "gtdisp") return s.fail("unknown command " + c);
    spin(50);
    fprintf(stderr, "GTSTRESS: %s ok=%d\n", c.c_str(), (int)ok);
    if (c == "gtdispcolour") {
      for (const string& r : gt->atomColourRows())
        fprintf(stderr, "GTSTRESS: colour %s\n", r.c_str());
    } else {
      fprintf(stderr, "GTSTRESS: summary %s\n", gt->displacementSummary().c_str());
      for (const string& r : gt->displacementRows())
        fprintf(stderr, "GTSTRESS: disp %s\n", r.c_str());
    }
    return true;
  }

  //  "gtseries <key>": plot that series; reports the axis labels.
  if (c == "gtseries" && w.size() == 2) {
    XYPlotCanvas *plot = plotOf(gt);
    if (!gt || !plot || !gt->showSeries(w[1]))
      return s.fail("gtseries: no such series " + w[1]);
    fprintf(stderr, "GTSTRESS: series %s x='%s' y='%s'\n", w[1].c_str(),
            (const char*)plot->xAxisLabel().utf8_str(),
            (const char*)plot->yAxisLabel().utf8_str());
    return true;
  }

  //  "gttake <label> <set|selected> [align]": Take Coordinates From... with
  //  the source picked by its dialog label, e.g. "calc (final geometry)";
  //  then every atom's coordinates (#620).
  if (c == "gttake" && w.size() >= 3) {
    vector<string> labels, el, xyz;
    string label = w[1];
    for (size_t i = 0; i < label.size(); i++) if (label[i] == '_') label[i] = ' ';
    size_t at = 0;
    //  The folder listing can still be arriving from the server.
    for (int tries = 0; tries < 40; tries++) {
      labels.clear(); el.clear(); xyz.clear();
      takeSources(labels, el, xyz);
      for (at = 0; at < labels.size() && labels[at] != label; at++) {}
      if (at < labels.size()) break;
      spin(250);
    }
    if (at == labels.size()) {
      string have;
      for (const string& l : labels) have += " [" + l + "]";
      fprintf(stderr, "GTSTRESS: gttake sources:%s\n", have.c_str());
      return s.fail("gttake: no source " + label);
    }
    double rmsd = 0;
    bool ok = takeCoordinates(w[2] == "selected" ? "" : w[2], el[at], xyz[at],
                              w.size() > 3 && w[3] == "align", &rmsd);
    fprintf(stderr, "GTSTRESS: gttake ok=%d rmsd=%.6g\n", (int)ok, rmsd);
    SGFragment *frag = getSG()->getFragment();
    for (size_t i = 0; i < frag->numAtoms(); i++) {
      const double *p = frag->atomRef(i)->coordinates();
      fprintf(stderr, "GTSTRESS: take %zu %s %.10f %.10f %.10f\n", i + 1,
              frag->atomRef(i)->atomicSymbol().c_str(), p[0], p[1], p[2]);
    }
    return true;
  }

  //  "gtselect <n>" selects atoms 1..n; "gtselected": reports how many are.
  if (c == "gtselect" && w.size() == 2) {
    SGFragment *frag = getSG()->getFragment();
    frag->m_atomHighLight.clear();
    const int n = atoi(w[1].c_str());
    for (int i = 0; i < n && i < (int)frag->numAtoms(); i++)
      frag->m_atomHighLight.push_back(i);
    getSG()->touchChemDisplay();
    return true;
  }
  if (c == "gtselected") {
    fprintf(stderr, "GTSTRESS: selected=%lu\n",
            (unsigned long)getSG()->getFragment()->m_atomHighLight.size());
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
  //  "vibbandshift <calculation> <atoms|->" and "vibbandclick <row>" (#707):
  //  the Match bands action; prints the table as shown, and the mode a row
  //  click selects.
  if (c == "vibbandshift" && w.size() == 3) {
    wxString err;
    bool ok = nm->matchBands(w[1], w[2] == "-" ? string() : w[2], &err);
    fprintf(stderr, "VIBBANDSHIFT: ok=%d error=%s\n", ok ? 1 : 0,
            (const char*)err.utf8_str());
    std::istringstream rows((const char*)nm->bandTableText().utf8_str());
    string row;
    while (std::getline(rows, row)) fprintf(stderr, "VIBBAND: %s\n", row.c_str());
    return true;
  }
  //  "vibspectrum <csv> <calculation|-> <diff 0|1> <experimental file|-> <zoom lo,hi|->"
  //  (#707): the compared calculation, the difference curve, an
  //  experimental file and a zoom, then the CSV an export would write.
  if (c == "vibspectrum" && w.size() == 6) {
    bool ok = w[2] == "-" || nm->compareWith(w[2]);
    nm->setShowDifference(w[3] == "1");
    wxString err;
    if (w[4] != "-") ok = nm->importExperimental(w[4], "auto", &err) && ok;
    double a, b2;
    if (w[5] != "-" && sscanf(w[5].c_str(), "%lf,%lf", &a, &b2) == 2)
      nm->zoomSpectrum(a, b2);
    ok = nm->exportSpectrumCsv(wxString::FromUTF8(w[1].c_str()), VIB_IR) && ok;
    fprintf(stderr, "VIBSPECTRUM: ok=%d error=%s\n", ok ? 1 : 0,
            (const char*)err.utf8_str());
    return true;
  }
  if (c == "vibbandclick" && w.size() == 2) {
    fprintf(stderr, "VIBBANDCLICK: selected=%d\n", nm->scriptBandClick(atoi(w[1].c_str())));
    return true;
  }
  //  "vibdisplace <row|all> <amplitude> <+|-|both>" (#618): the Displace
  //  dialog's action; prints each new calculation's run type, annotation and
  //  structure for the test to compare with the source.
  if (c == "vibdisplace" && w.size() == 4) {
    vector<string> urls;
    {
      Fragment sf;
      string sxyz, els;
      char b[80];
      IPropCalculation *sc = p_calculation;
      if (sc && sc->getFragment(sf))
        for (size_t a = 0; a < sf.numAtoms(); a++) {
          const double *x = sf.atomRef(a)->coordinates();
          snprintf(b, sizeof b, " %.6f %.6f %.6f", x[0], x[1], x[2]);
          sxyz += b;
          els += (a ? " " : "") + sf.atomRef(a)->atomicSymbol();
        }
      fprintf(stderr, "VIBSOURCE: url=%s els=%s xyz=%s\n",
              sc ? sc->getURL().toString().c_str() : "", els.c_str(), sxyz.c_str());
    }
    string err = nm->displaceCalculation(w[1] == "all" ? -1 : atoi(w[1].c_str()),
                                         atof(w[2].c_str()), w[3], &urls);
    fprintf(stderr, "VIBDISPLACE: error=%s n=%d\n", err.c_str(), (int)urls.size());
    for (size_t i = 0; i < urls.size(); i++) {
      Resource *r = EDSIFactory::getResource(EcceURL(urls[i]));
      ICalculation *ic = dynamic_cast<ICalculation*>(r);
      PropertyTask *pt = dynamic_cast<PropertyTask*>(r);
      Fragment *f = pt ? pt->fragment() : 0;
      string xyz;
      char b[80];
      for (size_t a = 0; f && a < f->numAtoms(); a++) {
        const double *x = f->atomRef(a)->coordinates();
        snprintf(b, sizeof b, " %.6f %.6f %.6f", x[0], x[1], x[2]);
        xyz += b;
      }
      GUIValues *gv = ic ? ic->guiparams() : 0;
      const int vibkeys = gv && gv->get("ES.Runtype.Vibration.Method") ? 1 : 0;
      delete gv;
      fprintf(stderr, "VIBDISPLACED: url=%s vibkeys=%d runtype=%s note=%s xyz=%s\n",
              urls[i].c_str(), vibkeys,
              ic ? ic->runtype().name().c_str() : "?",
              r ? r->getProp(VDoc::getEcceNamespace() + ":annotation").c_str() : "?",
              xyz.c_str());
    }
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
