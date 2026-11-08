//  The Display panel (#881): construction, Render > Display Settings..., and
//  the scene-script hook "display <form>" that calls every control:
//    display open | info <name> | reset
//    display style <name> | scheme <name> | slider <sphscl|cylrad|sphrq|cylrq> <n>
//    display auto <sphrq|cylrq> <on|off> | background r g b | foreground r g b
//    display lobes r g b r g b | preset 0-3 | surface solid|mesh|contour
//    display opacity <percent> | cell r g b | celllabels on|off
//    display atomlabels <0-6> | bondlabels|residuelabels|summary|axes on|off

#include <fstream>

#include <wx/aui/aui.h>

#include "dsm/IPropCalculation.H"
#include "viz/ForegroundCmd.H"
#include "viz/SGContainer.H"
#include "wxgui/ewxConfig.H"
#include "wxviz/SceneScript.H"
#include "wxviz/VizStyleChooser.H"
#include "wxviz/ViewerEvtHandler.H"

#include "Builder.H"
#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include "Cube.H"
#include "DisplayPanel.H"
#include "MoPanel.H"
#include "PropertyPanel.H"
#include "SurfaceStyle.H"

static const wxWindowID ID_DISPLAY_SETTINGS = wxNewId();
static const char *DISPLAY_PANE = "Display";

void Builder::createDisplayPanel()
{
  SurfaceStyle::applyCell();
  DisplayHost h;
  h.chooser = [this]() { return p_viewerEvtHandler->styleChooser(); };
  h.openCustomStyle = [this]() { p_viewerEvtHandler->openStyleChooser(); };
  h.menuAction = [this](int id, bool check, bool on) {
    wxCommandEvent ev(wxEVT_MENU, id);
    if (check) ev.SetInt(on ? 1 : 0);
    ev.SetEventObject(this);
    GetEventHandler()->ProcessEvent(ev);
  };
  h.checked = [this](int id) { return getToolState(id); };
  h.getBackground = [this](double rgb[3]) {
    SbColor c = getViewer().getBackgroundColor();
    for (int i = 0; i < 3; i++) rgb[i] = c[i];
  };
  h.getForeground = [this](double rgb[3]) {
    ForegroundCmd cmd("Foreground", getSG());
    rgb[0] = cmd.getParameter("red")->getDouble();
    rgb[1] = cmd.getParameter("green")->getDouble();
    rgb[2] = cmd.getParameter("blue")->getDouble();
  };
  h.setBackground = [this](double r, double g, double b) {
    p_viewerEvtHandler->setBackground(r, g, b);
  };
  h.setForeground = [this](double r, double g, double b) {
    ForegroundCmd cmd("Foreground", getSG());
    cmd.getParameter("red")->setDouble(r);
    cmd.getParameter("green")->setDouble(g);
    cmd.getParameter("blue")->setDouble(b);
    cmd.execute();
  };
  h.surfacesChanged = [this]() {
    if (getSG()) getSG()->touchLattice();
    if (!p_calculation) return;
    for (PropertyPanel *p :
         PropertyPanel::getPanels(p_calculation->getURL().toString())) {
      if (!p->IsShownOnScreen()) continue;
      if (MoPanel *mo = dynamic_cast<MoPanel*>(p)) mo->surfaceStyleChanged();
      else if (Cube *cube = dynamic_cast<Cube*>(p)) cube->surfaceStyleChanged();
    }
  };
  h.resetAll = [this]() {
    ewxConfig::getConfig("vizstyles.ini")->DeleteEntry("Ball And Stick");
    p_viewerEvtHandler->doStyleChange("Ball And Stick");
    p_viewerEvtHandler->setBackground(0, 0, 0);
  };
  p_displayPanel = new DisplayPanel(this, h);
  wxAuiPaneInfo info;
  info.Name(DISPLAY_PANE).Caption("Display").CaptionVisible(true).Right()
      .Layer(2).Resizable(true).CloseButton(true).Floatable(true)
      .MinSize(wxSize(260, 200)).BestSize(wxSize(320, 640)).Show(false);
  p_mgr.AddPane(p_displayPanel, info);

  p_renderMenu->Append(ID_DISPLAY_SETTINGS, _("Display Settings..."), _T(""),
                       wxITEM_NORMAL);
  Bind(wxEVT_MENU, [this](wxCommandEvent&) { showDisplayPanel(); },
       ID_DISPLAY_SETTINGS);
}

void Builder::showDisplayPanel()
{
  if (!p_displayPanel) return;
  p_displayPanel->refresh();
  p_mgr.GetPane(DISPLAY_PANE).Show(true);
  p_mgr.Update();
}

bool Builder::displayCommand(SceneScript& s, const vector<string>& w,
                             const string& outdir)
{
  if (!p_displayPanel) return s.fail("display: no panel");
  if (w.size() < 2) return s.fail("display: form missing");
  if (w[1] == "open") {
    showDisplayPanel();
  } else if (w[1] == "info" && w.size() == 3) {
    showDisplayPanel();
    std::ofstream(outdir + "/" + w[2] + ".txt") << p_displayPanel->info();
  } else {
    string err;
    vector<string> rest(w.begin() + 1, w.end());
    if (!p_displayPanel->script(rest, err)) return s.fail(err);
  }
  return true;
}
