//  Scene-script command that pictures one property panel (the plots, #214),
//  so the look of every plot can be compared before and after a change
//  without synthetic input.  Used through ECCE_VIEWER_SCENE by
//  tests/plots/capture.py.

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
    wxCommandEvent ev(wxEVT_MENU, mode == "plotsym" ? MoPanel::PLOTSYM
                                 : mode == "plot" ? MoPanel::PLOT
                                                  : MoPanel::TABLE);
    mo->GetEventHandler()->ProcessEvent(ev);
  }
  //  ECCE_PLOTSHOT_POS="x,y" moves the floating panel clear of the
  //  Builder's own windows, which a screen blit would otherwise include.
  int px = 20, py = 20;
  if (const char *pos = getenv("ECCE_PLOTSHOT_POS")) sscanf(pos, "%d,%d", &px, &py);
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
  pane.window->Refresh();
  pane.window->Update();
  settle(1500);

  if (getenv("ECCE_PLOTSHOT_DUMP")) dump(pane.window, 0);
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
  pane.Dock().Hide();
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
//    dmcsv <name> | dmpng <name> <w> <h> | dmstate | dmhide
bool Builder::densityMapCommand(SceneScript& s, const vector<string>& w,
                                const string& outdir)
{
  wxAuiPaneInfo &pane = p_mgr.GetPane("Density Map");
  if (!pane.IsOk()) return s.fail(w[0] + ": no Density Map panel");
  if (w[0] == "dmhide") {
    pane.Hide();
    updatePanes(true);
    return true;
  }
  if (!pane.IsShown()) {
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
  } else if (cmd != "dmstate") {
    return s.fail("unknown density map command: " + cmd);
  }
  if (!ok) return s.fail(cmd + ": refused");
  const std::string st = dm->state();
  fprintf(stderr, "DENSITYMAP: %s\n%s", cmd.c_str(), st.c_str());
  return true;
}
