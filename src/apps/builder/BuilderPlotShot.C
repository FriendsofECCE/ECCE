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
    wxCommandEvent ev(wxEVT_MENU, mode == "plotsym" ? MoPanel::PLOTSYM
                                 : mode == "plot" ? MoPanel::PLOT
                                                  : MoPanel::TABLE);
    mo->GetEventHandler()->ProcessEvent(ev);
  }
  pane.Float().FloatingPosition(wxPoint(20, 20))
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
