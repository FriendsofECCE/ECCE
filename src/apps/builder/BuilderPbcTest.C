//  Scene-script commands that drive the Periodic Builder and the Builder's
//  editing commands as the buttons do, and write out the resulting
//  structure.  Used only through ECCE_VIEWER_SCENE
//  (tests/apps/pbc_edit_test.py).

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>

#include <wx/app.h>
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
    wxWindow *btn = pbc->FindWindow(id);
    if (!btn) return s.fail("pbcpress: no button " + b);
    wxCommandEvent ev(wxEVT_BUTTON, id);
    ev.SetEventObject(btn);
    btn->ProcessWindowEvent(ev);
    settle(200);
    return true;
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
          << (p.IsOk() && p.window ? p.window->GetClientSize().x : 0) << "\n";
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
