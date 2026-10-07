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
