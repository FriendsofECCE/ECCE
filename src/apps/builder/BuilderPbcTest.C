//  Scene-script commands that drive the Periodic Builder and the Builder's
//  editing commands as the buttons do, and write out the resulting
//  structure.  Used only through ECCE_VIEWER_SCENE
//  (tests/apps/pbc_edit_test.py).

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <wx/app.h>
#include <wx/scrolwin.h>
#include <wx/spinctrl.h>
#include <wx/utils.h>
#include <wx/aui/aui.h>

#include "tdat/LatticeDef.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "wxviz/SceneScript.H"

#include "Builder.H"
#include "PBC.H"
#include "LayerBuilder.H"


namespace {

void settle(int ms)
{
  wxLongLong end = wxGetLocalTimeMillis() + ms;
  do {
    wxTheApp->Yield(true);
    wxMilliSleep(2);
  } while (wxGetLocalTimeMillis() < end);
}

}  // namespace


bool Builder::pbcTestCommand(SceneScript& s, const vector<string>& w,
                             const string& outdir)
{
  const string& c = w[0];
  string line;
  for (const string& t : w) line += " " + t;
  fprintf(stderr, "PBCTEST:%s\n", line.c_str());
  PBC *pbc = dynamic_cast<PBC*>(p_mgr.GetPane(NAME_TOOL_PBC).window);
  wxWindow *layer = p_mgr.GetPane(NAME_TOOL_LAYER).window;

  //  "pbcopen": Tools > Periodic Builder.
  if (c == "pbcopen") {
    if (!pbc) return s.fail("pbcopen: no Periodic Builder panel");
    p_mgr.GetPane(NAME_TOOL_PBC).Show(true);
    pbc->refresh();
    updatePanes(true);
    settle(200);
    return true;
  }

  //  "pbcpress <create|generate|replicate|restore|super|fold|delete> [n]":
  //  press that Periodic Builder button; replicate first sets n x n x n.
  if (c == "pbcpress" && w.size() >= 2) {
    if (!pbc) return s.fail("pbcpress: no Periodic Builder panel");
    wxWindowID id = wxID_NONE;
    const string& b = w[1];
    if (b == "create") id = PBCGUI::ID_BUTTON_PBC_CREATE;
    else if (b == "generate") id = PBCGUI::ID_PBC_GENERATE;
    else if (b == "replicate") id = PBCGUI::ID_BUTTON_PBC_REPLICATE;
    else if (b == "restore") id = PBCGUI::ID_BUTTON_PBC_RESTORE;
    else if (b == "super") id = PBCGUI::ID_BUTTON_PBC_SUPER;
    else if (b == "fold") id = PBCGUI::ID_BUTTON_PBC_FOLD;
    else if (b == "delete") id = PBCGUI::ID_BUTTON_PBC_DELETE;
    else if (b == "layers") id = LayerBuilder::ID_BUTTON_LAYER_BUILD;
    else return s.fail("pbcpress: unknown button " + b);
    if (b == "replicate" && w.size() >= 3) {
      int n = atoi(w[2].c_str());
      wxWindowID spins[3] = {PBCGUI::ID_SPINCTRL_PBC_XREP,
                             PBCGUI::ID_SPINCTRL_PBC_YREP,
                             PBCGUI::ID_SPINCTRL_PBC_ZREP};
      for (wxWindowID sid : spins) {
        wxSpinCtrl *sp = dynamic_cast<wxSpinCtrl*>(pbc->FindWindow(sid));
        if (!sp) return s.fail("pbcpress: no replication spin control");
        sp->SetValue(n);
      }
    }
    //  Build Layers lives in its own pane (Tools > Layer Builder).
    wxWindow *btn = (b == "layers" ? layer : pbc)->FindWindow(id);
    if (!btn) return s.fail("pbcpress: no button " + b);
    wxCommandEvent ev(wxEVT_BUTTON, id);
    ev.SetEventObject(btn);
    btn->ProcessWindowEvent(ev);
    settle(200);
    return true;
  }

  //  "importfile <path>": Add Structure from File, the type from the
  //  file's extension.
  if (c == "importfile" && w.size() == 2) {
    const size_t dot = w[1].rfind('.');
    if (dot == string::npos) return s.fail("importfile: no extension");
    importChemicalSystem(w[1], "", wxString(w[1].substr(dot + 1)));
    settle(500);
    return true;
  }

  //  "tooldock <tool> <left|right>": the pane dragged to that side.
  if (c == "tooldock" && w.size() == 3) {
    string name = w[1];
    for (char& ch : name) if (ch == '_') ch = ' ';
    wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(name));
    if (!pane.IsOk()) return s.fail("tooldock: no pane " + name);
    if (w[2] == "left") pane.Left().Layer(0).Row(0);
    else pane.Right();
    pane.Show(true);
    updatePanes(true);
    settle(400);
    return true;
  }

  //  "collapse <0|1>": View > Hide Side Panels off or on.
  if (c == "collapse" && w.size() == 2) {
    setColumnCollapsed(w[1] == "1");
    settle(400);
    return true;
  }

  //  "undo" / "redo": Edit > Undo / Redo.
  if ((c == "undo" || c == "redo") && w.size() == 1) {
    wxCommandEvent ev(wxEVT_MENU, c == "undo" ? wxID_UNDO : wxID_REDO);
    if (c == "undo") OnUndoClick(ev); else OnRedoClick(ev);
    settle(300);
    return true;
  }

  //  "pbclayer <field> <value>": set a Layer Builder field (layers spacing
  //  shifta shiftb ion1 ion1n ion1q ion1min ion2 ... water watermin seed).
  if (c == "pbclayer" && w.size() == 3) {
    if (!layer) return s.fail("pbclayer: no Layer Builder panel");
    static const struct { const char* n; wxWindowID id; bool spin; } f[] = {
      {"layers", LayerBuilder::ID_SPIN_LAYER_N, true},
      {"spacing", LayerBuilder::ID_TEXT_LAYER_SPACING, false},
      {"shifta", LayerBuilder::ID_TEXT_LAYER_SHIFTA, false},
      {"shiftb", LayerBuilder::ID_TEXT_LAYER_SHIFTB, false},
      {"ion1", LayerBuilder::ID_TEXT_LAYER_ION1SYM, false},
      {"ion1n", LayerBuilder::ID_SPIN_LAYER_ION1N, true},
      {"ion1q", LayerBuilder::ID_TEXT_LAYER_ION1Q, false},
      {"ion1min", LayerBuilder::ID_TEXT_LAYER_ION1MIN, false},
      {"ion2", LayerBuilder::ID_TEXT_LAYER_ION2SYM, false},
      {"ion2n", LayerBuilder::ID_SPIN_LAYER_ION2N, true},
      {"ion2q", LayerBuilder::ID_TEXT_LAYER_ION2Q, false},
      {"ion2min", LayerBuilder::ID_TEXT_LAYER_ION2MIN, false},
      {"water", LayerBuilder::ID_SPIN_LAYER_WATER, true},
      {"watermin", LayerBuilder::ID_TEXT_LAYER_WATERMIN, false},
      {"seed", LayerBuilder::ID_SPIN_LAYER_SEED, true}};
    for (const auto& e : f) {
      if (w[1] != e.n) continue;
      wxWindow *win = layer->FindWindow(e.id);
      if (!win) return s.fail("pbclayer: no field " + w[1]);
      if (e.spin) dynamic_cast<wxSpinCtrl*>(win)->SetValue(atoi(w[2].c_str()));
      else dynamic_cast<wxTextCtrl*>(win)->SetValue(wxString(w[2]));
      return true;
    }
    return s.fail("pbclayer: unknown field " + w[1]);
  }

  //  "setcontext <url>": switch this Builder to another calculation.
  if (c == "setcontext" && w.size() == 2) {
    setContext(w[1]);
    settle(1500);
    return true;
  }

  //  "columntab <0|1>": click the Structure (0) or Properties (1) tab.
  if (c == "columntab" && w.size() == 2) {
    setColumnTab(atoi(w[1].c_str()) ? 1 : 0, true);
    settle(300);
    return true;
  }

  //  "narrowcolumn <px>": as a layout saved narrow comes back: the column
  //  is px wide, its panes ask for no more, then the panes are laid out.
  if (c == "narrowcolumn" && w.size() == 2) {
    const int px = atoi(w[1].c_str());
    p_columnWidth = px;
    wxAuiPaneInfoArray &all = p_mgr.GetAllPanes();
    for (size_t i = 0; i < all.GetCount(); i++) {
      wxAuiPaneInfo &pane = all.Item(i);
      if (pane.IsShown() && !pane.IsFloating() && !pane.IsToolbar() &&
          pane.dock_direction == wxAUI_DOCK_RIGHT && pane.dock_layer >= 1) {
        pane.BestSize(wxSize(px, pane.best_size.y));
        pane.rect.width = px;
      }
    }
    updatePanes(true);
    settle(400);
    return true;
  }

  //  "toolfloat <tool>": the pane torn off into its own frame.
  if (c == "toolfloat" && w.size() == 2) {
    string name = w[1];
    for (char& ch : name) if (ch == '_') ch = ' ';
    wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(name));
    if (!pane.IsOk()) return s.fail("toolfloat: no pane " + name);
    pane.Float().Show(true);
    p_mgr.Update();
    updatePanes(true);
    settle(400);
    return true;
  }

  //  "toolclip <name> <tool>" -> <outdir>/<name>.txt: "pane <w>", then one
  //  "clipped <class> <right edge> <width>" line per control of the tool's
  //  scroll window that ends beyond its client width, and "hscroll 1" if
  //  the window scrolls sideways.
  if (c == "toolclip" && w.size() == 3) {
    string name = w[2];
    for (char& ch : name) if (ch == '_') ch = ' ';
    std::ofstream out(outdir + "/" + w[1] + ".txt");
    wxAuiPaneInfo &p = p_mgr.GetPane(wxString(name));
    if (!p.IsOk() || !p.window) return s.fail("toolclip: no pane " + name);
    out << "pane " << (p.IsFloating() && p.frame ? p.frame->GetClientSize().x
                                                       : p.rect.width) << "\n";
    wxScrolledWindow *scroll = 0;
    for (wxWindow *k : p.window->GetChildren())
      if (k->GetName() == "ecceToolScroll")
        scroll = dynamic_cast<wxScrolledWindow*>(k);
    if (!scroll) return s.fail("toolclip: " + name + " has no scroll window");
    const int width = scroll->GetClientSize().x;
    std::vector<wxWindow*> todo(scroll->GetChildren().begin(),
                                scroll->GetChildren().end());
    while (!todo.empty()) {
      wxWindow *k = todo.back();
      todo.pop_back();
      if (!k->IsShown()) continue;
      const wxPoint pos = k->GetPosition();
      const wxPoint origin = k->GetParent() == scroll ? wxPoint(0, 0)
          : k->GetParent()->ClientToScreen(wxPoint(0, 0)) -
            scroll->ClientToScreen(wxPoint(0, 0));
      const int right = origin.x + pos.x + k->GetSize().x;
      if (right > width)
        out << "clipped " << k->GetClassInfo()->GetClassName() << " "
            << right << " " << width << "\n";
      for (wxWindow *g : k->GetChildren()) todo.push_back(g);
    }
    out << "need " << scroll->GetSizer()->GetMinSize().x << "\n";
    int vx = 0, vy = 0;
    scroll->GetVirtualSize(&vx, &vy);
    out << "hscroll " << (vx > width ? 1 : 0) << "\n";
    return true;
  }

  //  "xshot <file>": the whole screen -> <file> (ImageMagick import).
  if (c == "xshot" && w.size() == 2) {
    settle(500);
    wxString cmd = wxString("import -window root ") + wxString(w[1]);
    wxExecute(cmd, wxEXEC_SYNC);
    return true;
  }

  //  "panelmode <classic|stacked|accordion|detail>": View > Panel layout.
  if (c == "panelmode" && w.size() == 2) {
    static const char *names[] = {"classic", "stacked", "accordion", "detail"};
    for (int m = 0; m < 4; m++) {
      if (w[1] == names[m]) {
        setPanelMode((PanelMode)m);
        settle(300);
        return true;
      }
    }
    return s.fail("panelmode: unknown layout " + w[1]);
  }

  //  "toolmenu <tool> <on|off>" and "paneclose <tool>": the Tools menu
  //  item and the pane's close button.  "_" in <tool> stands for a space.
  if ((c == "toolmenu" && w.size() == 3) || (c == "paneclose" && w.size() == 2)) {
    string name = w[1];
    for (char& ch : name) if (ch == '_') ch = ' ';
    wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(name));
    if (!pane.IsOk()) return s.fail(c + ": no pane " + name);
    if (c == "toolmenu") {
      const int id = p_toolMenu->FindItem(wxString(name));
      if (id == wxNOT_FOUND) return s.fail("toolmenu: no menu item " + name);
      const bool on = w[2] == "on";
      p_toolMenu->Check(id, on);
      wxCommandEvent ev(wxEVT_MENU, id);
      ev.SetInt(on ? 1 : 0);
      OnToolMenuClick(ev);
    } else {
      wxAuiManagerEvent ev(wxEVT_AUI_PANE_CLOSE);
      ev.SetManager(&p_mgr);
      ev.SetPane(&pane);
      OnPaneClose(ev);
      p_mgr.ClosePane(pane);
      updatePanes(true);
    }
    settle(400);
    return true;
  }

  //  "panefold <tool>": the fold button on that pane's caption.
  if (c == "panefold" && w.size() == 2) {
    string name = w[1];
    for (char& ch : name) if (ch == '_') ch = ' ';
    wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(name));
    if (!pane.IsOk() || !pane.HasPinButton())
      return s.fail("panefold: no fold button on " + name);
    toggleFold(pane.window);
    settle(400);
    return true;
  }

  //  "foldclick <pane>": a click on the fold button, dispatched as
  //  wxAuiManager dispatches it (the frame first, then the manager), so
  //  the toolkit's own pin handler competes as it does for the mouse.
  if (c == "foldclick" && w.size() == 2) {
    string name = w[1];
    for (char& ch : name) if (ch == '_') ch = ' ';
    wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(name));
    if (!pane.IsOk() || !pane.HasPinButton())
      return s.fail("foldclick: no fold button on " + name);
    wxAuiManagerEvent e(wxEVT_AUI_PANE_BUTTON);
    e.SetManager(&p_mgr);
    e.SetPane(&pane);
    e.SetButton(wxAUI_BUTTON_PIN);
    if (!GetEventHandler()->ProcessEvent(e)) p_mgr.ProcessEvent(e);
    settle(400);
    return true;
  }

  //  "sashdrag <tool> <dy> [lost|top]": press on the sash under that pane
  //  ("top": on the pane's own top edge, the sash of the pane above), move
  //  dy px in ten steps and release, as the mouse does it through the
  //  frame's event chain (AUI first); "lost" drops the release and moves
  //  once more without the button.  Appends "<tool> <dy> capture <0|1>"
  //  to <outdir>/sash.txt: whether the frame still holds the mouse.
  if (c == "sashdrag" && (w.size() == 3 || w.size() == 4)) {
    string name = w[1];
    for (char& ch : name) if (ch == '_') ch = ' ';
    wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(name));
    if (!pane.IsOk() || !pane.IsShown())
      return s.fail("sashdrag: no shown pane " + name);
    const int dy = atoi(w[2].c_str());
    const int sash = p_mgr.GetArtProvider()->GetMetric(wxAUI_DOCKART_SASH_SIZE);
    const int x = pane.rect.x + pane.rect.width / 2;
    const bool top = w.size() == 4 && w[3] == "top";
    //  rect is the client area; the caption and border lie above it.
    wxAuiDockArt *art = p_mgr.GetArtProvider();
    const int y0 = top
        ? pane.rect.y - art->GetMetric(wxAUI_DOCKART_CAPTION_SIZE) -
              art->GetMetric(wxAUI_DOCKART_PANE_BORDER_SIZE) - 1
        : pane.rect.GetBottom() + 1 + sash / 2;
    auto send = [this, x](wxEventType type, int y, bool down) {
      wxMouseEvent e(type);
      e.SetPosition(wxPoint(x, y));
      e.m_leftDown = down;
      e.SetEventObject(this);
      GetEventHandler()->ProcessEvent(e);
      wxTheApp->Yield(true);
    };
    send(wxEVT_LEFT_DOWN, y0, true);
    for (int i = 1; i <= 10; i++) send(wxEVT_MOTION, y0 + dy * i / 10, true);
    if (w.size() == 4 && !top) {
      send(wxEVT_MOTION, y0 + dy + 3, false);
    } else {
      send(wxEVT_LEFT_UP, y0 + dy, false);
    }
    settle(400);
    std::ofstream out(outdir + "/sash.txt", std::ios::app);
    out << w[1] << " " << dy << " capture " << (HasCapture() ? 1 : 0) << "\n";
    return true;
  }

  //  "panestate <name> <tool>..." -> <outdir>/<name>.txt: one line per tool:
  //  shown, floating, close button, window child of the frame, menu ticked,
  //  rectangle; then the client size and every shown pane's name.
  if (c == "panestate" && w.size() >= 2) {
    std::ofstream out(outdir + "/" + w[1] + ".txt");
    for (size_t i = 2; i < w.size(); i++) {
      string name = w[i];
      for (char& ch : name) if (ch == '_') ch = ' ';
      wxAuiPaneInfo &p = p_mgr.GetPane(wxString(name));
      const int id = p_toolMenu->FindItem(wxString(name));
      out << "tool \"" << name << "\" "
          << (p.IsOk() ? 1 : 0) << " shown " << (p.IsOk() && p.IsShown())
          << " floating " << (p.IsOk() && p.IsFloating())
          << " close " << (p.IsOk() && p.HasCloseButton())
          << " child " << (p.IsOk() && p.window && p.window->GetParent() == this)
          << " ticked " << (id != wxNOT_FOUND && p_toolMenu->IsChecked(id))
          << " rect " << p.rect.x << " " << p.rect.y << " " << p.rect.width
          << " " << p.rect.height;
      wxSizer *content = p.IsOk() && p.window ? p.window->GetSizer() : 0;
      out << " need " << (content ? content->GetMinSize().x : 0) << " have "
          << (p.IsOk() && p.window ? p.window->GetClientSize().x : 0)
          << " pin " << (p.IsOk() && p.HasPinButton()) << " folded "
          << (p.IsOk() && p_folded.count(p.window)) << "\n";
    }
    out << "client " << GetClientSize().x << " " << GetClientSize().y << "\n";
    wxAuiPaneInfoArray &all = p_mgr.GetAllPanes();
    for (size_t i = 0; i < all.GetCount(); i++) {
      if (all.Item(i).IsShown() && !all.Item(i).IsToolbar())
        out << "shown \"" << all.Item(i).name.ToStdString() << "\" rect "
            << all.Item(i).rect.x << " " << all.Item(i).rect.y << " "
            << all.Item(i).rect.width << " " << all.Item(i).rect.height
            << " prop " << all.Item(i).dock_proportion << " pos "
            << all.Item(i).dock_pos << " layer " << all.Item(i).dock_layer
            << " min " << all.Item(i).min_size.y << " best "
            << all.Item(i).best_size.y << "\n";
    }
    out << "readonly " << (p_calculation ? isReadOnly() : -1) << "\n";
    return true;
  }

  //  "pbcset <field> <value>": type <value> into a Periodic Builder text
  //  field (a b c alpha beta gamma) and press Enter in it.
  if (c == "pbcset" && w.size() == 3) {
    if (!pbc) return s.fail("pbcset: no Periodic Builder panel");
    wxWindowID id = wxID_NONE;
    if (w[1] == "a") id = PBCGUI::ID_TEXTCTRL_PBC_A;
    else if (w[1] == "b") id = PBCGUI::ID_TEXTCTRL_PBC_B;
    else if (w[1] == "c") id = PBCGUI::ID_TEXTCTRL_PBC_C;
    else if (w[1] == "alpha") id = PBCGUI::ID_TEXTCTRL_PBC_ALPHA;
    else if (w[1] == "beta") id = PBCGUI::ID_TEXTCTRL_PBC_BETA;
    else if (w[1] == "gamma") id = PBCGUI::ID_TEXTCTRL_PBC_GAMMA;
    else return s.fail("pbcset: unknown field " + w[1]);
    wxTextCtrl *t = dynamic_cast<wxTextCtrl*>(pbc->FindWindow(id));
    if (!t) return s.fail("pbcset: no field " + w[1]);
    t->SetValue(wxString(w[2]));
    wxCommandEvent ev(wxEVT_TEXT_ENTER, id);
    ev.SetEventObject(t);
    ev.SetString(wxString(w[2]));
    t->ProcessWindowEvent(ev);
    settle(300);
    return true;
  }

  //  "cmd <text>": a Builder command-line command (add, addh, removeh,
  //  clear, select ...), executed as the toolbar buttons execute theirs.
  if (c == "cmd" && w.size() >= 2) {
    string text;
    for (size_t i = 1; i < w.size(); i++) text += (i > 1 ? " " : "") + w[i];
    Command *cmd = createCommand(text);
    execute(cmd);
    settle(200);
    return true;
  }

  //  "fragdump <name>": the structure -> <outdir>/<name>.txt: counts, nubs
  //  without a parent, and per atom its symbol, Cartesian and fractional
  //  coordinates and bond count.
  if (c == "fragdump" && w.size() == 2) {
    SGFragment *frag = getSG()->getFragment();
    std::ofstream out(outdir + "/" + w[1] + ".txt");
    int nH = 0, nNub = 0, orphans = 0, n = frag->numAtoms();
    for (int i = 0; i < n; i++) {
      TAtm *a = frag->atomRef(i);
      if (a->atomicSymbol() == "H") nH++;
      if (a->atomicSymbol() == "Nub") {
        nNub++;
        if (frag->nubParent(a) == 0) orphans++;
      }
    }
    out << "atoms " << n << "\nH " << nH << "\nnubs " << nNub
        << "\norphanNubs " << orphans << "\n";
    LatticeDef *lattice = frag->getLattice();
    out << "lattice " << (lattice ? 1 : 0) << "\n";
    {
      PBC *pbcPanel = dynamic_cast<PBC*>(p_mgr.GetPane(NAME_TOOL_PBC).window);
      out << "latticeshown " << (pbcPanel && pbcPanel->latticeShown() ? 1 : 0)
          << "\n";
    }
    MPoint d, e, f, origin;
    double volume = 0.0;
    if (lattice) {
      vector<MPoint> *basis = lattice->toVectors();
      MPoint a = (*basis)[0], b = (*basis)[1], cc = (*basis)[2];
      delete basis;
      volume = (a.crossProduct1(b)).dotProduct(cc);
      d = b.crossProduct1(cc);
      e = cc.crossProduct1(a);
      f = a.crossProduct1(b);
      origin = lattice->getLatticeCorner();
      out << "vector " << a.x() << " " << a.y() << " " << a.z() << "\n"
          << "vector " << b.x() << " " << b.y() << " " << b.z() << "\n"
          << "vector " << cc.x() << " " << cc.y() << " " << cc.z() << "\n";
      int r1, r2, r3;
      lattice->getReplicationFactors(r1, r2, r3);
      out << "replication " << r1 << " " << r2 << " " << r3 << "\n";
    }
    for (int i = 0; i < n; i++) {
      TAtm *a = frag->atomRef(i);
      const double *x = a->coordinates();
      vector<TBond*> bonds = a->bondList();
      out << "atom " << a->atomicSymbol() << " " << x[0] << " " << x[1]
          << " " << x[2] << " bonds " << bonds.size();
      if (lattice && volume != 0.0) {
        double p[3] = {x[0] - origin.x(), x[1] - origin.y(), x[2] - origin.z()};
        out << " frac "
            << (p[0] * d.x() + p[1] * d.y() + p[2] * d.z()) / volume << " "
            << (p[0] * e.x() + p[1] * e.y() + p[2] * e.z()) / volume << " "
            << (p[0] * f.x() + p[1] * f.y() + p[2] * f.z()) / volume;
      }
      out << "\n";
    }
    return true;
  }

  return s.fail("unknown command: " + c);
}
