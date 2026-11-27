//  The AutoDock Vina search box in the Builder (#455): set from the selected
//  atoms, drawn as a wireframe in the viewer, redrawn when the editor's
//  numbers change.  The box values live in the calculation's run type
//  details (ES.Runtype.Vina.*), which ai.vina turns into the config.
#include "dock/PocketFinder.H"
#include "tdat/LigandSelect.H"
#include "tdat/Residue.H"
#include "dock/DockSplit.H"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <wx/menu.h>
#include <wx/utils.h>

#include "inv/nodes/SoSwitch.H"
#include "inv/nodes/SoSeparator.H"
#include "inv/nodes/SoMaterial.H"
#include "inv/nodes/SoDrawStyle.H"
#include "inv/nodes/SoLightModel.H"
#include "inv/nodes/SoCoordinate3.H"
#include "inv/nodes/SoIndexedLineSet.H"
#include "dsm/ICalculation.H"
#include "dsm/TaskJob.H"
#include "dsm/JCode.H"
#include "dsm/EDSIFactory.H"
#include "tdat/GUIValues.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "wxviz/SGViewer.H"
#include "PropertyPanel.H"
#include "EnsemblePropertyPanel.H"
#include "Builder.H"

using std::string;
using std::vector;

namespace {

const char *const AXES[3] = {"X", "Y", "Z"};

string value(GUIValues *g, const string& key, const string& dflt)
{
  GUIValue *v = g ? g->get(key) : 0;
  return v && !v->getValueAsString().empty() ? v->getValueAsString() : dflt;
}

// type is the dialog's own (toggle_input, float_input, text_input): the dialog
// restores a value only for the types it knows, else keeps its default.
void store(GUIValues *g, const string& key, const string& val,
           const string& type, const string& units)
{
  GUIValue *v = g->get(key);
  if (!v) {
    // A value the details dialog has not written yet: exported like its own.
    GUIValue nv;
    nv.m_key = key;
    nv.m_type = type;
    nv.m_units = units;
    nv.m_sensitive = true;
    nv.m_write = true;
    nv.setValue(val);
    g->set(key, new GUIValue(nv));
  } else {
    v->setValue(val);
  }
}

// As ai.vina's round1 (sprintf "%.1f"), so the drawn and written boxes agree.
double round1(double v)
{
  char b[32];
  snprintf(b, sizeof b, "%.1f", v);
  return atof(b);
}

}  // namespace


TaskJob *Builder::vinaJob() const
{
  TaskJob *job = dynamic_cast<TaskJob*>(p_calculation);
  return job && job->application() && job->application()->name() == "Vina"
             ? job : 0;
}


Builder::VinaBox Builder::vinaBoxFromDetails(const std::map<string, string>& d)
{
  auto get = [&](const string& k, const string& dflt) {
    auto it = d.find(k);
    return it == d.end() || it->second.empty() ? dflt : it->second;
  };
  VinaBox box;
  const string a = get("ES.Runtype.Vina.BoxAuto", "1");
  box.automatic = a == "1" || a == "true" || a == "True" || a == "yes";
  box.margin = atof(get("ES.Runtype.Vina.BoxMargin", "5").c_str());
  if (!(box.margin >= 0)) box.margin = 5;
  for (int k = 0; k < 3; k++) {
    box.centre[k] = atof(get(string("ES.Runtype.Vina.BoxCenter") + AXES[k], "0").c_str());
    box.size[k] = atof(get(string("ES.Runtype.Vina.BoxSize") + AXES[k], "20").c_str());
  }
  box.valid = true;
  if (!box.automatic) return box;

  // As ai.vina's SearchBox: the ligand's bounding box plus the margin on
  // each side, at least 16 A.  tests/apps/vina_viewer_test.py compares the
  // two.
  SGFragment *frag = getSG() ? getSG()->getFragment() : 0;
  const int n = frag ? frag->numAtoms() : 0;
  // The split is DockSplit's, as ai.vina's (via ecce-docksplit), so the box
  // follows the same ligand the job script will dock.
  vector<int> lig;
  if (n) {
    vector<DockSplitAtom> atoms(n);
    for (int i = 0; i < n; i++) {
      TAtm *at = frag->atomRef(i);
      DockSplitAtom& a = atoms[i];
      a.element = at->atomicSymbol();
      a.x = at->coordinates()[0]; a.y = at->coordinates()[1]; a.z = at->coordinates()[2];
      if (Residue *r = at->getResidue()) {
        a.resName = r->name();
        a.resNum = std::to_string(at->residueNumber());
        a.chain = string(1, r->chain());
      }
    }
    DockSplitResult r = DockSplit::split(atoms, get("ES.Runtype.Docking.LigandAtoms", ""));
    if (r.ok) lig = r.ligand;
  }
  double lo[3] = {1e30, 1e30, 1e30}, hi[3] = {-1e30, -1e30, -1e30};
  int used = 0;
  for (int i : lig) {
    if (i < 0 || i >= n) continue;
    const double *p = frag->atomRef(i)->coordinates();
    for (int k = 0; k < 3; k++) { lo[k] = std::min(lo[k], p[k]); hi[k] = std::max(hi[k], p[k]); }
    used++;
  }
  if (!used) { box.valid = false; return box; }
  for (int k = 0; k < 3; k++) {
    box.centre[k] = round1((lo[k] + hi[k]) / 2);
    box.size[k] = std::max(16.0, round1(hi[k] - lo[k] + 2 * box.margin));
  }
  return box;
}


void Builder::drawVinaBox(const VinaBox& box)
{
  SGContainer *sg = getSG();
  if (!sg) return;
  SoSwitch *sw = sg->getPropertySwitch("VinaSearchBox");
  sw->removeAllChildren();
  p_vinaBox = box;
  if (!box.valid) {
    sw->whichChild = SO_SWITCH_NONE;
    getViewer().refreshRenderArea();
    return;
  }
  SoSeparator *sep = new SoSeparator;
  SoLightModel *light = new SoLightModel;
  light->model = SoLightModel::BASE_COLOR;
  sep->addChild(light);
  SoMaterial *mat = new SoMaterial;
  mat->diffuseColor.setValue(0.0f, 0.62f, 0.25f);   // green on white and black
  sep->addChild(mat);
  SoDrawStyle *style = new SoDrawStyle;
  style->lineWidth = 2;
  sep->addChild(style);
  SoCoordinate3 *c = new SoCoordinate3;
  for (int i = 0; i < 8; i++)
    c->point.set1Value(i,
        box.centre[0] + ((i & 1) ? 0.5 : -0.5) * box.size[0],
        box.centre[1] + ((i & 2) ? 0.5 : -0.5) * box.size[1],
        box.centre[2] + ((i & 4) ? 0.5 : -0.5) * box.size[2]);
  sep->addChild(c);
  // The twelve edges: corners differing in one bit.
  SoIndexedLineSet *lines = new SoIndexedLineSet;
  int n = 0;
  for (int i = 0; i < 8; i++)
    for (int bit = 1; bit < 8; bit <<= 1)
      if (!(i & bit)) {
        lines->coordIndex.set1Value(n++, i);
        lines->coordIndex.set1Value(n++, i | bit);
        lines->coordIndex.set1Value(n++, -1);
      }
  sep->addChild(lines);
  sw->addChild(sep);
  sw->whichChild = SO_SWITCH_ALL;
  getViewer().refreshRenderArea();
}


//  The box a launched run used, from its stored deck.  The settings can hold
//  unused defaults (automatic box, centre 0 0 0) and the automatic rule is
//  the generator's to apply, so after Launch the deck is the only truth.
//  ligand gets the deck's LIGAND_ATOMS (0-based).
bool Builder::vinaBoxFromDeck(ICalculation *ic, VinaBox& box, vector<int> *ligand)
{
  std::unique_ptr<std::istream> in(ic->getDataFile(JCode::PRIMARY_INPUT));
  if (!in) return false;
  static const char *const keys[6] = {"center_x", "center_y", "center_z",
                                      "size_x", "size_y", "size_z"};
  double v[6];
  int found = 0;
  for (string line; std::getline(*in, line); ) {
    for (int k = 0; k < 6; k++) {
      const size_t n = strlen(keys[k]);
      if (line.compare(0, n, keys[k]) == 0 && line.find('=') != string::npos) {
        v[k] = atof(line.c_str() + line.find('=') + 1);
        found |= 1 << k;
      }
    }
    if (ligand && line.compare(0, 16, "#| LIGAND_ATOMS=") == 0) {
      string spec = line.substr(16);
      for (char& c : spec) if (c == ',') c = ' ';
      std::istringstream ss(spec);
      for (string part; ss >> part; ) {
        int x = 0, y = 0;
        if (sscanf(part.c_str(), "%d-%d", &x, &y) == 2)
          for (int i = x; i <= y; i++) ligand->push_back(i - 1);
        else if (sscanf(part.c_str(), "%d", &x) == 1) ligand->push_back(x - 1);
      }
    }
  }
  if (found != 63) return false;
  box.valid = true;
  for (int k = 0; k < 3; k++) { box.centre[k] = v[k]; box.size[k] = v[k + 3]; }
  return true;
}


void Builder::updateVinaBox()
{
  TaskJob *job = vinaJob();
  ICalculation *ic = dynamic_cast<ICalculation*>(job);
  if (!ic) {
    if (p_vinaBox.valid) drawVinaBox(VinaBox());
    return;
  }
  std::map<string, string> d;
  if (GUIValues *g = ic->guiparams()) {
    for (const auto& kv : *g) d[kv.first] = kv.second->getValueAsString();
    delete g;
  }
  VinaBox box = vinaBoxFromDetails(d);
  VinaBox used;
  if (job->getState() > ResourceDescriptor::STATE_READY &&
      vinaBoxFromDeck(ic, used)) {
    used.automatic = box.automatic;
    used.margin = box.margin;
    box = used;
  }
  drawVinaBox(box);
}


void Builder::vinaDetailsChanged(const string& url, const std::map<string, string> *live)
{
  if (!vinaJob() || p_calculation->getURL().toString() != url) return;
  if (live) {
    drawVinaBox(vinaBoxFromDetails(*live));
  } else {
    EDSIFactory::changePoolResource(url);
    updateVinaBox();
  }
}


// The tail shared by every way of setting the box: the six keys (stored by the
// caller into g), the job back to CREATED, the drawing and the editor message.
// Takes ownership of g.
string Builder::commitVinaBox(TaskJob *job, ICalculation *ic, GUIValues *g,
                              const VinaBox& box)
{
  char buf[32];
  for (int k = 0; k < 3; k++) {
    snprintf(buf, sizeof buf, "%.1f", box.centre[k]);
    store(g, string("ES.Runtype.Vina.BoxCenter") + AXES[k], buf, "float_input", "A");
    snprintf(buf, sizeof buf, "%.1f", box.size[k]);
    store(g, string("ES.Runtype.Vina.BoxSize") + AXES[k], buf, "float_input", "A");
  }
  snprintf(buf, sizeof buf, "%.1f", box.margin);
  store(g, "ES.Runtype.Vina.BoxMargin", buf, "float_input", "A");
  store(g, "ES.Runtype.Vina.BoxAuto", "0", "toggle_input", "");
  const bool ok = ic->guiparams(g);
  delete g;
  if (!ok) return "the box could not be stored with the calculation";
  // The deck no longer matches: Launch must not reuse it, and an open
  // Calculation Editor reloads the details (CalcEd::detailsMCB).
  if (job->getState() > ResourceDescriptor::STATE_CREATED)
    job->setState(ResourceDescriptor::STATE_CREATED);
  drawVinaBox(box);
  JMSMessage *msg = newMessage();
  msg->addProperty("url", getContext());
  publish("ecce_url_details", *msg);
  delete msg;
  return "";
}


// Binding pockets of the structure shown (#1320), best first.  The receptor is
// what is scored: waters and bound ligands are dropped by the detector.
string Builder::findPocketsReport(PocketFinder::Result& res)
{
  SGFragment *frag = getSG()->getFragment();
  if (!frag || frag->numAtoms() == 0) return "there is no structure";
  wxBusyCursor busy;  // ~0.2 s on a 4700-atom receptor, too short for a bar
  res = PocketFinder::findPockets(*frag);
  if (res.pockets.empty())
    return "no binding pocket was found (is a receptor with a cleft loaded?)";
  return "";
}


string Builder::vinaBoxFromPocket(int n, double margin, string *what)
{
  TaskJob *job = vinaJob();
  ICalculation *ic = dynamic_cast<ICalculation*>(job);
  if (!ic) return "the structure open is not a Vina calculation";
  if (isReadOnly()) return "the calculation can no longer be edited";
  PocketFinder::Result res;
  string why = findPocketsReport(res);
  if (!why.empty()) return why;
  if (n < 1 || n > (int)res.pockets.size()) {
    char b[80];
    snprintf(b, sizeof b, "there is no pocket %d, %d found", n, (int)res.pockets.size());
    return b;
  }
  const PocketFinder::Pocket& pk = res.pockets[n - 1];
  GUIValues *g = ic->guiparams();
  if (!g) g = new GUIValues;
  if (margin < 0) margin = atof(value(g, "ES.Runtype.Vina.BoxMargin", "5").c_str());
  if (!(margin >= 0)) margin = 5;
  VinaBox box;
  box.valid = true;
  box.automatic = false;
  box.margin = margin;
  for (int k = 0; k < 3; k++) {
    box.centre[k] = round1((pk.lo[k] + pk.hi[k]) / 2);
    box.size[k] = std::max(1.0, round1(pk.hi[k] - pk.lo[k] + 2 * margin));
  }
  if (what) {
    char b[160];
    snprintf(b, sizeof b, "pocket %d of %d (volume %.0f A^3, %d lining residues)",
             n, (int)res.pockets.size(), pk.volume, (int)pk.lining.size());
    *what = b;
  }
  return commitVinaBox(job, ic, g, box);
}


string Builder::vinaBoxFromSelection(double margin)
{
  TaskJob *job = vinaJob();
  ICalculation *ic = dynamic_cast<ICalculation*>(job);
  if (!ic) return "the structure open is not a Vina calculation";
  if (isReadOnly()) return "the calculation can no longer be edited";
  SGFragment *frag = getSG()->getFragment();
  if (!frag || frag->m_atomHighLight.empty())
    return "select the atoms (the ligand or the binding-site residues) first";
  GUIValues *g = ic->guiparams();
  if (!g) g = new GUIValues;
  if (margin < 0) margin = atof(value(g, "ES.Runtype.Vina.BoxMargin", "5").c_str());
  if (!(margin >= 0)) margin = 5;
  double lo[3] = {1e30, 1e30, 1e30}, hi[3] = {-1e30, -1e30, -1e30};
  for (int a : frag->m_atomHighLight) {
    if (a < 0 || a >= frag->numAtoms()) continue;
    const double *p = frag->atomRef(a)->coordinates();
    for (int k = 0; k < 3; k++) { lo[k] = std::min(lo[k], p[k]); hi[k] = std::max(hi[k], p[k]); }
  }
  VinaBox box;
  box.valid = true;
  box.automatic = false;
  box.margin = margin;
  for (int k = 0; k < 3; k++) {
    box.centre[k] = round1((lo[k] + hi[k]) / 2);
    box.size[k] = std::max(1.0, round1(hi[k] - lo[k] + 2 * margin));
  }
  // A selection that is exactly one ligand residue (not polymer, water or an
  // ion) is also the ligand to dock: fill Ligand atoms with it.  A box around
  // binding-site residues leaves the field as it was.
  {
    const Residue *only = 0;
    bool one = true;
    for (int a : frag->m_atomHighLight) {
      const Residue *r = (a >= 0 && a < frag->numAtoms()) ? frag->atomRef(a)->getResidue() : 0;
      if (!r || (only && r != only)) { one = false; break; }
      only = r;
    }
    if (one && only && SGFragment::residueClass(only->name()) == SGFragment::RES_OTHER) {
      vector<int> idx(frag->m_atomHighLight.begin(), frag->m_atomHighLight.end());
      string ranges = LigandSelect::atomRangeString(idx);
      store(g, "ES.Runtype.Docking.LigandAtoms", ranges, "text_input", "");
    }
  }
  return commitVinaBox(job, ic, g, box);
}


void Builder::OnVinaBoxFromSelection(wxCommandEvent&)
{
  const string why = vinaBoxFromSelection(-1);
  if (!why.empty()) {
    showMessage("Docking box: " + why + ".", true);
  } else {
    char buf[200];
    snprintf(buf, sizeof buf, "Docking box set: centre %.1f %.1f %.1f, size "
             "%.1f x %.1f x %.1f A. Save the calculation in the editor to "
             "write it into the input.", p_vinaBox.centre[0],
             p_vinaBox.centre[1], p_vinaBox.centre[2], p_vinaBox.size[0],
             p_vinaBox.size[1], p_vinaBox.size[2]);
    showMessage(buf);
  }
}


void Builder::OnFindPockets(wxCommandEvent&)
{
  string what;
  const string why = vinaBoxFromPocket(1, -1, &what);
  if (!why.empty()) {
    showMessage("Binding pockets: " + why + ".", true);
    return;
  }
  char buf[300];
  snprintf(buf, sizeof buf, "Docking box set around %s: centre %.1f %.1f %.1f, "
           "size %.1f x %.1f x %.1f A. Save the calculation in the editor to "
           "write it into the input.", what.c_str(), p_vinaBox.centre[0],
           p_vinaBox.centre[1], p_vinaBox.centre[2], p_vinaBox.size[0],
           p_vinaBox.size[1], p_vinaBox.size[2]);
  showMessage(buf);
}


//  ECCE_BUILDER_SCRIPT commands for the box and the Poses panel:
//    selectatoms <1-5,9>   select these atoms (numbered from 1)
//    vinabox [margin]      Tools > Docking Box Around Selection
//    vinabox pocket=N [margin]  the box around pocket N (1 = best) of findpockets
//    findpockets           one line per binding pocket, best first
//    vinaboxinfo           the box drawn: "drawn=0|1 centre x y z size x y z"
//    poses select <rank>   show a pose, as a click on its row
//    poses info            rows, the energy label, population, pocket, thumbnails
//    poses png <file>      the panel's Export .png
//    poses pocket on|off   the Pocket view box
string Builder::vinaScriptCommand(const vector<string>& w)
{
  SGFragment *frag = getSG()->getFragment();
  char buf[256];
  if (w[0] == "selectatoms" && w.size() == 2) {
    frag->m_atomHighLight.clear();
    string spec = w[1];
    for (char& c : spec) if (c == ',') c = ' ';
    std::istringstream in(spec);
    for (string part; in >> part; ) {
      int a = 0, b = 0;
      if (sscanf(part.c_str(), "%d-%d", &a, &b) != 2) {
        if (sscanf(part.c_str(), "%d", &a) != 1) return "FAIL: bad atom list";
        b = a;
      }
      for (int i = a; i <= b; i++)
        if (i >= 1 && i <= frag->numAtoms()) frag->m_atomHighLight.push_back(i - 1);
    }
    getViewer().refreshRenderArea();
    snprintf(buf, sizeof buf, "selected %d", (int)frag->m_atomHighLight.size());
    return buf;
  }
  if (w[0] == "vinabox") {
    // Words after the command: pocket=N and/or a margin; "pocket=" is taken
    // out first so the positional margin keeps its meaning.
    int pocket = 0;
    double margin = -1;
    for (size_t i = 1; i < w.size(); i++) {
      if (w[i].compare(0, 7, "pocket=") == 0) {
        pocket = atoi(w[i].c_str() + 7);
        if (pocket < 1) return "FAIL: pocket=N needs N from 1";
      } else {
        margin = atof(w[i].c_str());
      }
    }
    const string why = pocket ? vinaBoxFromPocket(pocket, margin) : vinaBoxFromSelection(margin);
    if (!why.empty()) return "FAIL: " + why;
  }
  if (w[0] == "findpockets") {
    PocketFinder::Result res;
    const string why = findPocketsReport(res);
    if (!why.empty()) return "FAIL: " + why;
    string out = "pockets=" + std::to_string(res.pockets.size());
    int rank = 0;
    for (const PocketFinder::Pocket& pk : res.pockets) {
      snprintf(buf, sizeof buf, " ; #%d score %.1f volume %.0f centre %.1f %.1f %.1f "
               "extent %.1f x %.1f x %.1f lining %d", ++rank, pk.score, pk.volume,
               pk.centre[0], pk.centre[1], pk.centre[2], pk.hi[0] - pk.lo[0],
               pk.hi[1] - pk.lo[1], pk.hi[2] - pk.lo[2], (int)pk.lining.size());
      out += buf;
    }
    return out;
  }
  if (w[0] == "vinabox" || w[0] == "vinaboxinfo") {
    snprintf(buf, sizeof buf, "drawn=%d auto=%d centre %.1f %.1f %.1f size %.1f %.1f %.1f",
             (int)p_vinaBox.valid, (int)p_vinaBox.automatic, p_vinaBox.centre[0],
             p_vinaBox.centre[1], p_vinaBox.centre[2], p_vinaBox.size[0],
             p_vinaBox.size[1], p_vinaBox.size[2]);
    string out = buf;
    // The ligand atoms (the deck's list) of the structure shown against the
    // drawn box: all of them inside means the box is in the shown frame.
    VinaBox deck;
    vector<int> lig;
    if (ICalculation *ic = dynamic_cast<ICalculation*>(vinaJob()))
      if (vinaBoxFromDeck(ic, deck, &lig) && p_vinaBox.valid && !lig.empty()) {
        int inside = 0, total = 0;
        for (int i : lig) {
          if (i < 0 || i >= frag->numAtoms()) continue;
          const double *p = frag->atomRef(i)->coordinates();
          bool in = true;
          for (int k = 0; k < 3; k++)
            if (fabs(p[k] - p_vinaBox.centre[k]) > 0.5 * p_vinaBox.size[k]) in = false;
          total++;
          inside += in;
        }
        snprintf(buf, sizeof buf, " ligand_inside=%d/%d", inside, total);
        out += buf;
      }
    return out;
  }
  if (w[0] == "poses" && w.size() >= 2) {
    EnsemblePropertyPanel *en = 0;
    for (PropertyPanel *p : PropertyPanel::getPanels(getContext(), "", this))
      if ((en = dynamic_cast<EnsemblePropertyPanel*>(p))) break;
    if (!en) return "FAIL: no ensemble panel";
    if (w[1] == "select" && w.size() == 3) {
      return en->selectRanks(vector<int>(1, atoi(w[2].c_str()))) ? "ok" : "FAIL: no such pose";
    }
    if (w[1] == "pocket" && w.size() == 3) {
      en->setPocketView(w[2] == "on");
      return en->pocketInfo();
    }
    if (w[1] == "png" && w.size() == 3)
      return en->exportPng(w[2]) ? "ok" : "FAIL: render or write failed";
    if (w[1] == "note") return en->poseNote();
    if (w[1] == "thumb" && w.size() == 4)
      return en->exportThumbnail(atoi(w[2].c_str()), w[3]) ? "ok" : "FAIL";
    if (w[1] == "info") {
      string out = "kind=" + en->kind() + " population=" +
                   (en->hasPopulation() ? "1" : "0") + " label=" +
                   en->energyLabel() + " thumbs=" +
                   std::to_string(en->thumbnailRanks().size()) + " rowheight=" +
                   std::to_string(en->tableRowHeight()) + " " +
                   en->pocketInfo();
      for (const string& r : en->tableRows()) out += " ; " + r;
      return out;
    }
  }
  return "FAIL: bad vina command";
}
