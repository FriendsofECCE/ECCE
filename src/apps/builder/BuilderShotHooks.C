//  Scene-script commands that put the Builder in a state the help and
//  teaching text describe, so it can be photographed headlessly (#860):
//    assigncharges <panel>      the charge panel's Assign Charges button,
//                               then charge labels on every atom
//    panelfloat <panel> <w> <h> float the pane at that client size
//    panelsize <panel> <w> <h>  size the docked pane
//    shot <name>                writes <outdir>/<name>.ready and keeps the
//                               event loop running until <name>.go exists:
//                               the driver photographs the screen meanwhile
//                               (the window is painted for real, floating
//                               panels included)
//  Each reports on stderr as "SHOTHOOK: ...".  Used through
//  ECCE_VIEWER_SCENE (tests/apps/shot_hooks_test.py, tools/screenshots).

#include <cstdio>
#include <fstream>
#include <sys/stat.h>

#include <wx/app.h>
#include <wx/aui/aui.h>
#include <wx/utils.h>

#include <wx/grid.h>

#include "dsm/IPropCalculation.H"
#include "tdat/PropVector.H"
#include "wxgui/TreeListCtrl.H"
#include "tdat/TAtm.H"
#include "viz/AtomLabelsCmd.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "wxviz/SceneScript.H"

#include "Builder.H"
#include "MullikenPanel.H"
#include "PartialCharges.H"
#include "PropertyPanel.H"


namespace {

void settle(int ms)
{
  wxLongLong end = wxGetLocalTimeMillis() + ms;
  do {
    wxTheApp->Yield(true);
    wxMilliSleep(2);
  } while (wxGetLocalTimeMillis() < end);
}

string join(const vector<string>& w, size_t from)
{
  string s;
  for (size_t i = from; i < w.size(); i++) s += (i > from ? " " : "") + w[i];
  return s;
}

}  // namespace


bool Builder::shotHookCommand(SceneScript& s, const vector<string>& w,
                              const string& outdir)
{
  const string& c = w[0];

  if (c == "assigncharges") {
    const string name = join(w, 1);
    wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(name));
    if (!pane.IsOk()) return s.fail("assigncharges: no panel named '" + name + "'");
    pane.Show(true);
    updatePanes(true);
    settle(500);
    MullikenPanel *mul = 0;
    PartialCharges *part = 0;
    for (PropertyPanel *p :
         PropertyPanel::getPanels(p_calculation->getURL().toString())) {
      if (p->getName() != name && p != pane.window) continue;
      if (!mul) mul = dynamic_cast<MullikenPanel*>(p);
      if (!part) part = dynamic_cast<PartialCharges*>(p);
    }
    if (!mul && !part) return s.fail("assigncharges: '" + name + "' is no charge panel");
    (mul ? static_cast<PropertyPanel*>(mul) : part)->ensureInitialized();
    if (mul) mul->assignCharges();
    else part->assignCharges();
    SGFragment *frag = getSG()->getFragment();
    AtomLabelsCmd labels("AtomLabel", getSG());
    labels.getParameter("type")->setInteger(AtomLabelsCmd::CHARGE2);
    labels.execute();
    getSG()->touchChemDisplay();
    settle(300);
    double sum = 0;
    for (size_t i = 0; i < frag->numAtoms(); i++) sum += frag->atomRef(i)->partialCharge();
    fprintf(stderr, "SHOTHOOK: assigncharges %s atoms=%d first=%.3f sum=%.3f label1=[%s]\n",
            name.c_str(), (int)frag->numAtoms(), frag->atomRef(0)->partialCharge(),
            sum, frag->atomRef(0)->displayLabel().c_str());
    return true;
  }

  if (c == "shot" && w.size() == 2) {
    const string base = outdir + "/" + w[1];
    std::remove((base + ".go").c_str());
    settle(1500);
    std::ofstream(base + ".ready") << "ready\n";
    struct stat st;
    wxLongLong end = wxGetLocalTimeMillis() + 120000;
    while (stat((base + ".go").c_str(), &st) != 0 && wxGetLocalTimeMillis() < end)
      settle(100);
    return true;
  }

  if ((c == "panelfloat" || c == "panelsize") && w.size() >= 4) {
    const int width = atoi(w[w.size() - 2].c_str());
    const int height = atoi(w.back().c_str());
    vector<string> nameWords(w.begin() + 1, w.end() - 2);
    const string name = join(nameWords, 0);
    wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(name));
    if (!pane.IsOk()) return s.fail(c + ": no panel named '" + name + "'");
    pane.Show(true);
    if (c == "panelfloat") {
      pane.Float().FloatingSize(wxSize(width, height + 40))
          .BestSize(wxSize(width, height)).MinSize(wxSize(100, 100));
    } else {
      pane.BestSize(wxSize(width, height));
    }
    updatePanes(true);
    settle(800);
    if (c == "panelfloat" && pane.frame) {
      pane.frame->SetClientSize(wxSize(width, height));
      pane.frame->Layout();
      settle(300);
    }
    wxWindow *win = pane.window;
    fprintf(stderr, "SHOTHOOK: %s %s floating=%d size=%dx%d\n", c.c_str(),
            name.c_str(), pane.IsFloating() ? 1 : 0, win ? win->GetSize().x : -1,
            win ? win->GetSize().y : -1);
    return true;
  }

  return s.fail("unknown command: " + c);
}
