//  The AutoDock Vina search box in the Builder (#455): set from the selected
//  atoms, drawn as a wireframe in the viewer, redrawn when the editor's
//  numbers change.  The box values live in the calculation's run type
//  details (ES.Runtype.Vina.*), which ai.vina turns into the config.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <wx/menu.h>

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
  vector<int> lig;
  string spec = get("ES.Runtype.Docking.LigandAtoms", "");
  for (char& c : spec) if (c == ',') c = ' ';
  std::istringstream in(spec);
  for (string part; in >> part; ) {
    int x = 0, y = 0;
    if (sscanf(part.c_str(), "%d-%d", &x, &y) == 2)
      for (int i = x; i <= y; i++) lig.push_back(i - 1);
    else if (sscanf(part.c_str(), "%d", &x) == 1) lig.push_back(x - 1);
  }
  if (lig.empty() && n) {
    // docking.pl's default: the smallest molecule.
    vector<int> root(n);
    for (int i = 0; i < n; i++) root[i] = i;
    std::function<int(int)> find = [&](int i) { return root[i] == i ? i : root[i] = find(root[i]); };
    std::unique_ptr<vector<TBond*> > bonds(frag->bonds());
    for (TBond *b : *bonds)
      root[find(b->atom1()->index())] = find(b->atom2()->index());
    std::map<int, vector<int> > parts;
    for (int i = 0; i < n; i++) parts[find(i)].push_back(i);
    if (parts.size() > 1)
      for (const auto& p : parts)
        if (lig.empty() || p.second.size() < lig.size()) lig = p.second;
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
  drawVinaBox(vinaBoxFromDetails(d));
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
  char buf[32];
  for (int k = 0; k < 3; k++) {
    box.centre[k] = round1((lo[k] + hi[k]) / 2);
    box.size[k] = std::max(1.0, round1(hi[k] - lo[k] + 2 * margin));
    snprintf(buf, sizeof buf, "%.1f", box.centre[k]);
    store(g, string("ES.Runtype.Vina.BoxCenter") + AXES[k], buf, "float", "A");
    snprintf(buf, sizeof buf, "%.1f", box.size[k]);
    store(g, string("ES.Runtype.Vina.BoxSize") + AXES[k], buf, "float", "A");
  }
  snprintf(buf, sizeof buf, "%.1f", margin);
  store(g, "ES.Runtype.Vina.BoxMargin", buf, "float", "A");
  store(g, "ES.Runtype.Vina.BoxAuto", "0", "bool", "");
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


//  ECCE_BUILDER_SCRIPT commands for the box and the Poses panel:
//    selectatoms <1-5,9>   select these atoms (numbered from 1)
//    vinabox [margin]      Tools > Docking Box Around Selection
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
    const string why = vinaBoxFromSelection(w.size() > 1 ? atof(w[1].c_str()) : -1);
    if (!why.empty()) return "FAIL: " + why;
  }
  if (w[0] == "vinabox" || w[0] == "vinaboxinfo") {
    snprintf(buf, sizeof buf, "drawn=%d auto=%d centre %.1f %.1f %.1f size %.1f %.1f %.1f",
             (int)p_vinaBox.valid, (int)p_vinaBox.automatic, p_vinaBox.centre[0],
             p_vinaBox.centre[1], p_vinaBox.centre[2], p_vinaBox.size[0],
             p_vinaBox.size[1], p_vinaBox.size[2]);
    return buf;
  }
  if (w[0] == "poses" && w.size() >= 2) {
    EnsemblePropertyPanel *en = 0;
    for (PropertyPanel *p : PropertyPanel::getPanels(getContext()))
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
    if (w[1] == "info") {
      string out = "kind=" + en->kind() + " population=" +
                   (en->hasPopulation() ? "1" : "0") + " label=" +
                   en->energyLabel() + " thumbs=" +
                   std::to_string(en->thumbnailRanks().size()) + " " +
                   en->pocketInfo();
      for (const string& r : en->tableRows()) out += " ; " + r;
      return out;
    }
  }
  return "FAIL: bad vina command";
}
