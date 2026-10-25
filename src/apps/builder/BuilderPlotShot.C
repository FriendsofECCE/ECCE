//  Scene-script command that pictures one property panel (the plots, #214),
//  so the look of every plot can be compared before and after a change
//  without synthetic input.  Used through ECCE_VIEWER_SCENE by
//  tests/plots/capture.py.

#include <cmath>
#include <wx/aui/aui.h>
#include <wx/app.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/dcscreen.h>
#include <wx/image.h>
#include <wx/utils.h>

#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include "dsm/IPropCalculation.H"

#include "wxviz/SceneScript.H"

#include "Builder.H"
#include "PropertyPanel.H"
#include "MoPanel.H"
#include "DensityMapPanel.H"
#include "DensityProfilePanel.H"
#include "XYPlotCanvas.H"


namespace {

void settle(int ms)
{
  wxLongLong end = wxGetLocalTimeMillis() + ms;
  do {
    wxTheApp->Yield(true);
    wxMilliSleep(2);
  } while (wxGetLocalTimeMillis() < end);
}

void dump(wxWindow *w, int depth)
{
  fprintf(stderr, "PLOTSHOT:   %*s%s %dx%d at %d,%d%s\n", depth * 2, "",
          (const char *)wxString(w->GetClassInfo()->GetClassName()).mb_str(),
          w->GetSize().x, w->GetSize().y, w->GetPosition().x,
          w->GetPosition().y, w->IsShown() ? "" : " hidden");
  if (depth < 4)
    for (wxWindow *c : w->GetChildren()) dump(c, depth + 1);
}

//  One line per plot canvas in the window, for tests: the axes and, per
//  curve, its name, size, last point and mean of y.
void reportPlots(wxWindow *w, const string& name)
{
  if (XYPlotCanvas *p = dynamic_cast<XYPlotCanvas*>(w)) {
    fprintf(stderr, "PLOTSHOT-PLOT %s title='%s' x='%s' y='%s' curves=%d\n",
            name.c_str(), (const char*)p->title().utf8_str(),
            (const char*)p->xAxisLabel().utf8_str(),
            (const char*)p->yAxisLabel().utf8_str(), p->curveCount());
    for (int c = 0; c < p->curveCount(); c++) {
      const int n = p->pointCount(c);
      double mean = 0;
      for (int i = 0; i < n; i++) mean += p->yAt(c, i);
      if (n) mean /= n;
      fprintf(stderr, "PLOTSHOT-CURVE %s %d name='%s' n=%d lastx=%.9g "
              "lasty=%.9g meany=%.9g\n", name.c_str(), c,
              (const char*)p->curveName(c).utf8_str(), n,
              n ? p->xAt(c, n - 1) : 0.0, n ? p->yAt(c, n - 1) : 0.0, mean);
    }
  }
  for (wxWindow *c : w->GetChildren()) reportPlots(c, name);
}

//  ECCE_PLOTSHOT_ZOOM=<notches>: wheel notches of zoom-out (negative: in)
//  on every plot before the picture, then the view range it settled on.
void zoomPlots(wxWindow *w, double notches, const string& name)
{
  if (XYPlotCanvas *p = dynamic_cast<XYPlotCanvas*>(w)) {
    p->zoomBy(std::pow(0.8, -notches));
    p->Refresh();
    p->Update();
    double x0, x1, y0, y1;
    p->viewRange(&x0, &x1, &y0, &y1);
    fprintf(stderr, "PLOTSHOT-VIEW %s x=[%.6g, %.6g] y=[%.6g, %.6g]\n",
            name.c_str(), x0, x1, y0, y1);
  }
  for (wxWindow *c : w->GetChildren()) zoomPlots(c, notches, name);
}

}  // namespace


bool Builder::plotShotCommand(SceneScript& s, const vector<string>& w,
                              const string& outdir)
{
  if (w.size() < 5)
    return s.fail("plotshot <name> <width> <height> <panel name>");
  string panel;
  for (size_t i = 4; i < w.size(); i++) panel += (i > 4 ? " " : "") + w[i];
  //  "MOs:plot" / "MOs:plotsym": the MO panel's energy-level plots rather
  //  than its table.
  string mode;
  const size_t colon = panel.find(':');
  if (colon != string::npos) {
    mode = panel.substr(colon + 1);
    panel = panel.substr(0, colon);
  }
  //  "docked": leave the pane where the layout put it, at its default size.
  bool docked = false;
  if (mode.compare(0, 6, "docked") == 0) {
    docked = true;
    mode = mode.substr(6);
  }
  const int width = atoi(w[2].c_str()), height = atoi(w[3].c_str());

  static bool handlers = false;
  if (!handlers) { wxInitAllImageHandlers(); handlers = true; }

  wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(panel));
  if (!pane.IsOk()) return s.fail("plotshot: no panel named '" + panel + "'");
  pane.Show(true);
  for (int i = 0; i < p_propertyMenu->GetMenuItemCount(); i++) {
    wxMenuItem *item = p_propertyMenu->FindItemByPosition(i);
    if (item != 0 && item->GetItemLabelText() == panel) item->Check(true);
  }
  //  As a click on the list does: the panel becomes the detail pane.
  if (docked && p_panelMode == PANELS_DETAIL) selectDetail(panel);
  updatePanes(true);
  if (!mode.empty()) {
    MoPanel *mo = 0;
    set<PropertyPanel*> panels =
        PropertyPanel::getPanels(p_calculation->getURL().toString());
    for (set<PropertyPanel*>::iterator it = panels.begin();
         it != panels.end() && !mo; ++it)
      mo = dynamic_cast<MoPanel*>(*it);
    if (!mo) return s.fail("plotshot: no MO panel for '" + mode + "'");
    mo->ensureInitialized();
    if (mode == "loctable" && !mo->selectFieldType("Localised MO"))
      return s.fail("plotshot: no Localised MO for this calculation");
    wxCommandEvent ev(wxEVT_MENU, mode == "plotsym" ? MoPanel::PLOTSYM
                                 : mode == "plot" ? MoPanel::PLOT
                                                  : MoPanel::TABLE);
    mo->GetEventHandler()->ProcessEvent(ev);
  }
  //  ECCE_PLOTSHOT_POS="x,y" moves the floating panel clear of the
  //  Builder's own windows, which a screen blit would otherwise include.
  int px = 20, py = 20;
  if (const char *pos = getenv("ECCE_PLOTSHOT_POS")) sscanf(pos, "%d,%d", &px, &py);
  if (docked) {
    settle(1500);
  } else {
  pane.Float().FloatingPosition(wxPoint(px, py))
      .BestSize(wxSize(width, height)).MinSize(wxSize(100, 100))
      .FloatingSize(wxSize(width, height + 40));
  p_mgr.Update();
  settle(800);
  if (pane.frame) {
    pane.frame->SetClientSize(wxSize(width, height));
    pane.frame->Layout();
  }
  pane.window->SetSize(wxSize(width, height));
  pane.window->Layout();
  settle(800);
  }
  pane.window->Refresh();
  pane.window->Update();
  settle(1500);

  if (const char *z = getenv("ECCE_PLOTSHOT_ZOOM")) {
    zoomPlots(pane.window, atof(z), w[1]);
    settle(800);
  }
  if (getenv("ECCE_PLOTSHOT_DUMP")) dump(pane.window, 0);
  reportPlots(pane.window, w[1]);
  if (docked) {
    //  Where the layout put every shown pane, for the column-height checks.
    wxAuiPaneInfoArray &all = p_mgr.GetAllPanes();
    fprintf(stderr, "PLOTSHOT-PANES %s client %dx%d\n", w[1].c_str(),
            GetClientSize().x, GetClientSize().y);
    for (size_t i = 0; i < all.GetCount(); ++i) {
      const wxAuiPaneInfo &p = all.Item(i);
      if (!p.IsShown() || p.IsToolbar() || p.IsFloating() || !p.window) continue;
      fprintf(stderr, "PLOTSHOT-PANE %s x=%d y=%d w=%d h=%d min=%d prop=%d\n",
              (const char*)p.name.utf8_str(), p.rect.x, p.rect.y, p.rect.width,
              p.rect.height, p.min_size.y, p.dock_proportion);
    }
  }
  if (MoPanel *mo = dynamic_cast<MoPanel*>(pane.window)) {
    const string st = mo->tableState();
    fprintf(stderr, "PLOTSHOT-MOTABLE %s\n%s", w[1].c_str(), st.c_str());
  }
  const wxRect r = pane.window->GetScreenRect();
  fprintf(stderr, "PLOTSHOT: asked %dx%d, window %dx%d, frame %dx%d\n", width,
          height, pane.window->GetSize().x, pane.window->GetSize().y,
          pane.frame ? pane.frame->GetSize().x : -1,
          pane.frame ? pane.frame->GetSize().y : -1);
  wxBitmap bmp(r.width, r.height);
  {
    wxMemoryDC mdc(bmp);
    wxScreenDC screen;
    mdc.Blit(0, 0, r.width, r.height, &screen, r.x, r.y);
  }
  if (!bmp.SaveFile(wxString(outdir + "/" + w[1] + ".png"), wxBITMAP_TYPE_PNG))
    return s.fail("plotshot: cannot write " + w[1] + ".png");
  if (!docked) pane.Dock();
  if (!(docked && p_panelMode == PANELS_DETAIL)) pane.Hide();   // the next detail replaces it
  p_mgr.Update();
  settle(300);
  fprintf(stderr, "PLOTSHOT: %s %dx%d\n", w[1].c_str(), r.width, r.height);
  return true;
}


//  Scene commands for the Density Map panel (#215); each GUI action has one:
//    densitymap <name> <w> <h> <orbital|0|sumN> <alpha|beta> <plane 0-3> [a b c]
//    dmpick A B [C]       atoms picked in the 3-D view (1-based)
//    dmoffset X [drag]    the offset slider, A
//    dmview rx ry rz ux uy uz | dmviewcurrent     "Use this view"
//    dmfollow N           select MO N in the MOs panel
//    dmcsv <name> | dmpng <name> <w> <h> | dmzoom F U V|@N | dmzoomreset | dmstate | dmhide | dmclose | dmopen | dmlayout W | dmplane N
bool Builder::densityMapCommand(SceneScript& s, const vector<string>& w,
                                const string& outdir)
{
  wxAuiPaneInfo &pane = p_mgr.GetPane("Density Map");
  if (!pane.IsOk()) return s.fail(w[0] + ": no Density Map panel");
  if (w[0] == "dmhide") {
    //  As the menu's untick does; a ticked entry would bring the pane back.
    for (int i = 0; i < p_propertyMenu->GetMenuItemCount(); i++) {
      wxMenuItem *item = p_propertyMenu->FindItemByPosition(i);
      if (item != 0 && item->GetItemLabelText() == "Density Map") item->Check(false);
    }
    pane.Hide();
    updatePanes(true);
    return true;
  }
  if (!pane.IsShown() && w[0] != "dmstate") {
    pane.Show(true);
    for (int i = 0; i < p_propertyMenu->GetMenuItemCount(); i++) {
      wxMenuItem *item = p_propertyMenu->FindItemByPosition(i);
      if (item != 0 && item->GetItemLabelText() == "Density Map") item->Check(true);
    }
    updatePanes(true);
  }
  DensityMapPanel *dm = 0;
  set<PropertyPanel*> panels =
      PropertyPanel::getPanels(p_calculation->getURL().toString());
  for (set<PropertyPanel*>::iterator it = panels.begin();
       it != panels.end() && !dm; ++it)
    dm = dynamic_cast<DensityMapPanel*>(*it);
  if (!dm) return s.fail(w[0] + ": no Density Map panel for this calculation");
  dm->ensureInitialized();
  const std::string& cmd = w[0];
  bool ok = true;
  if (cmd == "densitymap") {
    if (w.size() < 7)
      return s.fail("densitymap <name> <w> <h> <orbital|0|sumN> <alpha|beta> "
                    "<plane 0-3> [a b c]");
    const bool sum = w[4].compare(0, 3, "sum") == 0;
    const int orbital = atoi(w[4].c_str() + (sum ? 3 : 0));
    const int a = w.size() > 7 ? atoi(w[7].c_str()) : 0;
    const int b = w.size() > 8 ? atoi(w[8].c_str()) : 0;
    const int c = w.size() > 9 ? atoi(w[9].c_str()) : 0;
    if (!dm->setView(orbital, w[5] == "beta", atoi(w[6].c_str()), a, b, c, sum))
      return s.fail("densitymap: the plane or orbital was refused");
    if (!dm->savePng(wxString(outdir + "/" + w[1] + ".png"), atoi(w[2].c_str()),
                     atoi(w[3].c_str())))
      return s.fail("densitymap: cannot write " + w[1] + ".png");
  } else if (cmd == "dmpick" && w.size() >= 3) {
    vector<int> atoms;
    for (size_t i = 1; i < w.size(); i++) atoms.push_back(atoi(w[i].c_str()));
    ok = dm->pickAtoms(atoms);
  } else if (cmd == "dmoffset" && w.size() >= 2) {
    ok = dm->setOffset(atof(w[1].c_str()), w.size() > 2 && w[2] == "drag");
  } else if (cmd == "dmview" && w.size() == 7) {
    const double r[3] = { atof(w[1].c_str()), atof(w[2].c_str()), atof(w[3].c_str()) };
    const double u[3] = { atof(w[4].c_str()), atof(w[5].c_str()), atof(w[6].c_str()) };
    ok = dm->useView(r, u);
  } else if (cmd == "dmviewcurrent") {
    ok = dm->useCurrentView();
  } else if (cmd == "dmfollow" && w.size() >= 2) {
    set<PropertyPanel*> all =
        PropertyPanel::getPanels(p_calculation->getURL().toString());
    ok = false;
    for (set<PropertyPanel*>::iterator it = all.begin(); it != all.end(); ++it)
      if (MoPanel *mo = dynamic_cast<MoPanel*>(*it))
        ok = mo->selectRowForMo(atoi(w[1].c_str()));
  } else if (cmd == "dmcsv" && w.size() == 2) {
    ok = dm->exportCsv(wxString(outdir + "/" + w[1] + ".csv"));
  } else if (cmd == "dmpng" && w.size() == 4) {
    ok = dm->savePng(wxString(outdir + "/" + w[1] + ".png"), atoi(w[2].c_str()),
                     atoi(w[3].c_str()));
  } else if (cmd == "dmlayout" && w.size() == 2) {
    //  Each control's rectangle at that panel width; clipped ones fail.
    const std::string report = dm->layoutReport(atoi(w[1].c_str()));
    fprintf(stderr, "DENSITYMAP layout\n%s", report.c_str());
    ok = report.find("layout ok") != std::string::npos;
  } else if (cmd == "dmopen") {
    dm->Show(true);
  } else if (cmd == "dmplane" && w.size() == 2) {
    //  The plane radio box, as a click on entry N does.
    ok = dm->choosePlane(atoi(w[1].c_str()));
  } else if (cmd == "dmcollapse" && w.size() == 2) {
    //  The "Hide/Show plane controls" button: 1 = collapsed.
    dm->setPlaneControlsOpen(w[1] == "0");
  } else if (cmd == "dmzoom" && w.size() >= 3) {
    //  dmzoom F U V (plane coordinates, A) | dmzoom F @N (atom N, 1-based)
    double u = 0, v = 0;
    if (w[2][0] == '@') ok = dm->atomPoint(atoi(w[2].c_str() + 1), u, v);
    else if (w.size() >= 4) { u = atof(w[2].c_str()); v = atof(w[3].c_str()); }
    else ok = false;
    ok = ok && dm->zoomAt(u, v, atof(w[1].c_str()));
  } else if (cmd == "dmzoomreset") {
    dm->resetZoom();
  } else if (cmd == "dmclose") {
    //  What setDetail does to a panel another one replaces.
    dm->Show(false);
  } else if (cmd != "dmstate") {
    return s.fail("unknown density map command: " + cmd);
  }
  if (!ok) return s.fail(cmd + ": refused");
  const std::string st = dm->state();
  fprintf(stderr, "DENSITYMAP: %s\n%s", cmd.c_str(), st.c_str());
  return true;
}


//  Scene commands for the Density Profile panel; each GUI action has one:
//    dpopen | dpclose
//    dpmode line|atom
//    dpatoms A B | dpmargin ANGSTROM | dpcentre N     (atoms 1-based as labelled)
//    dpquantity density|psi2|psi (line) or radial|avg (atom)
//    dpshow all|totalselected|selected|total
//    dpmo N,N,...|none    exactly these orbitals ticked
//    dpspin alpha|beta|both | dpunit A|bohr | dpcombine 0|1 | dpclip 0|1
//    dpxaxis linear|sqrt|log (atom) | dpyaxis linear|log
//    dpshift ANGSTROM [in|out] (line) | dpzoom whole|fit|in|out | dpmore 0|1
//    dpfollow N [beta]    what a click on an MO Diagram level does
//    dpcsv <name> | dppng <name> <w> <h> | dpstate
bool Builder::densityProfileCommand(SceneScript& s, const vector<string>& w,
                                    const string& outdir)
{
  wxAuiPaneInfo &pane = p_mgr.GetPane("Density Profile");
  if (!pane.IsOk()) return s.fail(w[0] + ": no Density Profile panel");
  if (w[0] == "dpclose") {
    pane.Hide();
    updatePanes(true);
    return true;
  }
  if (!pane.IsShown()) {
    pane.Show(true);
    for (int i = 0; i < p_propertyMenu->GetMenuItemCount(); i++) {
      wxMenuItem *item = p_propertyMenu->FindItemByPosition(i);
      if (item != 0 && item->GetItemLabelText() == "Density Profile") item->Check(true);
    }
    updatePanes(true);
  }
  DensityProfilePanel *dp = 0;
  set<PropertyPanel*> panels =
      PropertyPanel::getPanels(p_calculation->getURL().toString());
  for (set<PropertyPanel*>::iterator it = panels.begin();
       it != panels.end() && !dp; ++it)
    dp = dynamic_cast<DensityProfilePanel*>(*it);
  if (!dp) return s.fail(w[0] + ": no Density Profile panel for this calculation");
  dp->ensureInitialized();
  const std::string& cmd = w[0];
  bool ok = true;
  if (cmd == "dpmode" && w.size() == 2) {
    ok = dp->setMode(w[1] == "atom");
  } else if (cmd == "dpatoms" && w.size() == 3) {
    ok = dp->setAtoms(atoi(w[1].c_str()), atoi(w[2].c_str()));
  } else if (cmd == "dpmargin" && w.size() == 2) {
    ok = dp->setMargin(atof(w[1].c_str()));
  } else if (cmd == "dpcentre" && w.size() == 2) {
    ok = dp->setCentre(atoi(w[1].c_str()));
  } else if (cmd == "dpquantity" && w.size() == 2) {
    ok = dp->setQuantity(w[1]);
  } else if (cmd == "dpshow" && w.size() == 2) {
    ok = dp->setShow(w[1]);
  } else if (cmd == "dpmo" && w.size() == 2) {
    vector<int> mos;
    if (w[1] != "none") {
      size_t at = 0;
      while (at < w[1].size()) {
        mos.push_back(atoi(w[1].c_str() + at));
        const size_t comma = w[1].find(',', at);
        if (comma == string::npos) break;
        at = comma + 1;
      }
    }
    ok = dp->setOrbitals(mos);
  } else if (cmd == "dpspin" && w.size() == 2) {
    ok = dp->setSpin(w[1]);
  } else if (cmd == "dpunit" && w.size() == 2) {
    ok = dp->setBohr(w[1] == "bohr");
  } else if (cmd == "dpcombine" && w.size() == 2) {
    ok = dp->setCombine(w[1] != "0");
  } else if (cmd == "dpclip" && w.size() == 2) {
    ok = dp->setClip(w[1] != "0");
  } else if (cmd == "dpxaxis" && w.size() == 2) {
    ok = dp->setXAxis(w[1]);
  } else if (cmd == "dpyaxis" && w.size() == 2) {
    ok = dp->setYAxis(w[1]);
  } else if (cmd == "dpshift" && w.size() >= 2) {
    ok = dp->setShift(atof(w[1].c_str()), w.size() < 3 || w[2] != "in");
  } else if (cmd == "dpzoom" && w.size() == 2) {
    ok = dp->zoom(w[1]);
  } else if (cmd == "dpmore" && w.size() == 2) {
    ok = dp->setMoreOpen(w[1] != "0");
  } else if (cmd == "dpfollow" && w.size() >= 2) {
    dp->followMo(atoi(w[1].c_str()), w.size() > 2 && w[2] == "beta");
  } else if (cmd == "dpcsv" && w.size() == 2) {
    ok = dp->exportCsv(wxString(outdir + "/" + w[1] + ".csv"));
  } else if (cmd == "dppng" && w.size() == 4) {
    ok = dp->savePng(wxString(outdir + "/" + w[1] + ".png"), atoi(w[2].c_str()),
                     atoi(w[3].c_str()));
  } else if (cmd != "dpopen" && cmd != "dpstate") {
    return s.fail("unknown density profile command: " + cmd);
  }
  if (!ok) return s.fail(cmd + ": refused");
  fprintf(stderr, "DENSITYPROFILE: %s\n%s", cmd.c_str(), dp->state().c_str());
  return true;
}
