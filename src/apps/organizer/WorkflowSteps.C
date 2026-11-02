#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <set>
#include <sstream>

#include "tdat/Fragment.H"
#include "tdat/SpinMult.H"
#include "util/InvalidException.H"
#include "util/ResourceUtils.H"

#include "dsm/EDSIFactory.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/TGBSConfig.H"
#include "dsm/ICalculation.H"
#include "dsm/ChemistryTask.H"
#include "dsm/PropertyTask.H"
#include "dsm/ResourceType.H"
#include "dsm/QstStructures.H"
#include "dsm/ScanPoint.H"
#include "dsm/Session.H"
#include "tdat/GUIValues.H"
#include "tdat/TRunType.H"
#include "tdat/TTheory.H"
#include "dsm/VDoc.H"
#include "util/Ecce.H"
#include "CompareEnergies.H"

#include "WorkflowSteps.H"

using std::string;
using std::vector;

const char *const WorkflowSteps::kGeometryLink = "geometry";

const vector<WorkflowSteps::Code>& WorkflowSteps::codes()
{
  static const vector<Code> list = {
    {"xtb",     "xtb_es",        0x7a, 0x4f, 0xa3},
    {"CREST",   "crest_es",      0x5a, 0x3f, 0x8f},
    {"ORCA",    "orca_es",       0x1f, 0x6f, 0xb2},
    {"ECCE-QM", "ecceqm_es",     0x2b, 0x8a, 0x3e},
    {"G16",     "gaussian16_es", 0xb3, 0x5c, 0x00},
    {"CP2K",    "cp2k_es",       0x8a, 0x1c, 0x1c},
    {"QE",      "qe_es",         0x5c, 0x6b, 0x73},
    {"NWChem",  "nwchem_es",     0x9c, 0x36, 0xb5},
    {"MOPAC",   "mopac_es",      0xa6, 0x6a, 0x00}
  };
  return list;
}


const WorkflowSteps::Code *WorkflowSteps::codeOf(const Resource *step)
{
  ResourceType *rt = step ? const_cast<Resource*>(step)->getDescriptor() : 0;
  if (!rt) return 0;
  for (size_t i = 0; i < codes().size(); i++)
    if (rt->getName() == codes()[i].typeName) return &codes()[i];
  return 0;
}


string WorkflowSteps::linkName(const string& role, const string& point)
{
  return point.empty() ? role : role + "." + point;
}


void WorkflowSteps::splitLinkName(const string& name, string& role,
                                  string& point)
{
  size_t at = name.find('.');
  role = name.substr(0, at);
  point = at == string::npos ? string() : name.substr(at + 1);
}


string WorkflowSteps::linkLabel(const string& name)
{
  string role, point;
  splitLinkName(name, role, point);
  if (role == "tsguess") role = "TS guess";
  return point.empty() ? role : role + " (" + ScanPoint::words(point) + ")";
}


bool WorkflowSteps::isScan(Resource *step)
{
  return ScanPoint::isScan(dynamic_cast<PropertyTask*>(step));
}


bool WorkflowSteps::hasResultGeometry(Resource *step)
{
  PropertyTask *task = dynamic_cast<PropertyTask*>(step);
  if (!task) return false;
  Fragment *frag = task->fragment();
  if (!frag) return false;
  bool result = task->getFragmentStep(frag, -1);
  delete frag;
  return result;
}


ResourceDescriptor::RUNSTATE WorkflowSteps::stateOf(Resource *step)
{
  ICalculation *calc = dynamic_cast<ICalculation*>(step);
  return calc ? calc->getState() : ResourceDescriptor::STATE_ILLEGAL;
}


bool WorkflowSteps::isComplete(Resource *step)
{
  ResourceDescriptor::RUNSTATE s = stateOf(step);
  return s == ResourceDescriptor::STATE_COMPLETED ||
         s == ResourceDescriptor::STATE_LOADED;
}


bool WorkflowSteps::isLaunched(Resource *step)
{
  ResourceDescriptor::RUNSTATE s = stateOf(step);
  return s != ResourceDescriptor::STATE_CREATED &&
         s != ResourceDescriptor::STATE_READY;
}


string WorkflowSteps::defaultName(Session *wf, const Code& code)
{
  std::set<string> used;
  vector<Resource*> members = wf->getMembers();
  for (size_t i = 0; i < members.size(); i++) used.insert(members[i]->getName());
  string base = code.label;
  for (int n = 1;; n++) {
    string name = base + "-" + std::to_string(n);
    if (!used.count(name)) return name;
  }
}


Resource *WorkflowSteps::create(Session *wf, ResourceType *rt,
                                const string& name, Resource *source,
                                const Carry& carry, string& why)
{
  Resource *made = 0;
  try {
    // A scan point that cannot be found fails before anything is made.
    Fragment *pointFrag = 0;
    if (source && carry.geometry && !carry.point.empty()) {
      string pwhy;
      pointFrag = ScanPoint::fragment(dynamic_cast<PropertyTask*>(source),
                                      carry.point, pwhy);
      if (!pointFrag) throw InvalidException(pwhy, WHERE);
    }
    made = wf->createChild(name, rt);
    if (!made) throw InvalidException("The step could not be created", WHERE);
    wf->addMember(made);
    if (source)
      wf->addLink(source, made, Session::INPUT_PROVIDER,
                  linkName(kGeometryLink, carry.point));
    wf->saveLinkbase();

    ICalculation *from = dynamic_cast<ICalculation*>(source);
    ICalculation *to = dynamic_cast<ICalculation*>(made);
    PropertyTask *fromTask = dynamic_cast<PropertyTask*>(source);
    ChemistryTask *toTask = dynamic_cast<ChemistryTask*>(made);
    if (from && to && fromTask && toTask) {
      if (carry.geometry) {
        Fragment *frag = pointFrag ? pointFrag : fromTask->fragment();
        if (frag) {
          if (!pointFrag) fromTask->getFragmentStep(frag, -1);
          if (!carry.chargeMult) frag->charge(0);
          toTask->fragment(frag);
          delete frag;
        }
      }
      if (carry.chargeMult) {
        to->spinMultiplicity(from->spinMultiplicity());
        to->openShells(from->openShells());
      }
    }
  }
  catch (InvalidException& ex) {
    why = ex.what();
    if (made) {
      try { wf->removeMember(made); } catch (...) {}
      made->remove();
    }
    return 0;
  }
  return made;
}


bool WorkflowSteps::isEnsemble(Resource *step)
{
  return ScanPoint::ensembleSize(dynamic_cast<PropertyTask*>(step)) > 0;
}


vector<Resource*> WorkflowSteps::createBest(Session *wf, ResourceType *rt,
                                            const string& name,
                                            Resource *source, int n,
                                            const Carry& carry, string& why)
{
  vector<Resource*> made;
  PropertyTask *task = dynamic_cast<PropertyTask*>(source);
  const int size = ScanPoint::ensembleSize(task);
  if (size < 1) {
    why = "The source has no conformer or pose ensemble.";
    return made;
  }
  if (n < 1 || n > size) {
    char buf[96];
    snprintf(buf, sizeof buf, "The ensemble has %d conformers; %d asked for.",
             size, n);
    why = buf;
    return made;
  }
  for (int k = 1; k <= n; k++) {
    Carry c = carry;
    c.point = "c" + std::to_string(k);
    Resource *step = create(wf, rt, name + "-conf" + std::to_string(k), source,
                            c, why);
    if (!step) {
      for (size_t i = 0; i < made.size(); i++) {
        try { wf->removeMember(made[i]); } catch (...) {}
        made[i]->remove();
      }
      made.clear();
      return made;
    }
    char note[200];
    if (ScanPoint::ensembleIsPoses(task))
      snprintf(note, sizeof note,
               "Pose %d of %d from %s, score %.3f kcal/mol "
               "(within this docking run only).",
               k, size, source->getName().c_str(),
               ScanPoint::conformerEnergy(task, k));
    else
      snprintf(note, sizeof note,
               "Conformer %d of %d from %s, relative energy %.3f kcal/mol "
               "(within the CREST method only).",
               k, size, source->getName().c_str(),
               ScanPoint::conformerEnergy(task, k));
    step->addProp(VDoc::getEcceNamespace() + ":annotation", note);
    made.push_back(step);
  }
  return made;
}


vector<Resource*> WorkflowSteps::createPatterns(Session *wf, ResourceType *rt,
                                                const string& name,
                                                Resource *source,
                                                const Carry& carry, string& why)
{
  vector<Resource*> made;
  PropertyTask *task = dynamic_cast<PropertyTask*>(source);
  vector<ScanPoint::Pattern> pats;
  if (!ScanPoint::ensemblePatterns(task, pats, why)) return made;
  const int size = ScanPoint::ensembleSize(task);
  for (size_t k = 0; k < pats.size(); k++) {
    Carry c = carry;
    c.point = "c" + std::to_string(pats[k].rank);
    Resource *step = create(wf, rt, name + "-pat" + std::to_string(k + 1),
                            source, c, why);
    if (!step) {
      for (size_t i = 0; i < made.size(); i++) {
        try { wf->removeMember(made[i]); } catch (...) {}
        made[i]->remove();
      }
      made.clear();
      return made;
    }
    char note[400];
    snprintf(note, sizeof note,
             "Pattern %d of %d, %s (%d of %d conformers): conformer %d of %s, "
             "relative energy %.3f kcal/mol (within the CREST method only).",
             (int)k + 1, (int)pats.size(), pats[k].label.c_str(),
             pats[k].count, size, pats[k].rank, source->getName().c_str(),
             pats[k].relative);
    step->addProp(VDoc::getEcceNamespace() + ":annotation", note);
    made.push_back(step);
  }
  return made;
}


bool WorkflowSteps::canJoin(const Code& code)
{
  const string t = code.typeName;
  return t == "gaussian16_es" || t == "orca_es";
}


string WorkflowSteps::joinRunType(const Code& code)
{
  return string(code.typeName) == "orca_es"
             ? "Transition State (NEB-TS) + Frequencies"
             : "Transition State (QST) + Frequencies";
}


vector<WorkflowSteps::JoinSource>
WorkflowSteps::defaultRoles(const vector<Resource*>& sources)
{
  vector<JoinSource> out;
  const bool guess = sources.size() >= 3;
  int scan = -1;
  if (guess)
    for (size_t i = 0; i < sources.size() && scan < 0; i++)
      if (isScan(sources[i])) scan = (int)i;
  if (guess && scan < 0) scan = (int)sources.size() - 1;
  int next = 0;
  for (size_t i = 0; i < sources.size(); i++) {
    JoinSource js = {sources[i], "", ""};
    if ((int)i == scan) {
      js.role = "tsguess";
      if (isScan(sources[i])) js.point = "max";
    } else {
      js.role = next++ == 0 ? "reactant" : "product";
      if (isScan(sources[i])) js.point = "last";
    }
    out.push_back(js);
  }
  return out;
}


string WorkflowSteps::checkRoles(const vector<JoinSource>& sources)
{
  int n[3] = {0, 0, 0};
  for (size_t i = 0; i < sources.size(); i++) {
    const string& r = sources[i].role;
    if (r == "reactant") n[0]++;
    else if (r == "product") n[1]++;
    else if (r == "tsguess") n[2]++;
    else return "Every source needs a role: reactant, product or TS guess.";
    if (!ScanPoint::valid(sources[i].point))
      return "A scan point is last, maximum or a number.";
  }
  if (sources.size() < 2 || sources.size() > 3)
    return "A join takes two or three steps.";
  if (n[0] != 1 || n[1] != 1)
    return "There must be exactly one reactant and one product.";
  if (n[2] > 1) return "There can be only one TS guess.";
  return "";
}


Resource *WorkflowSteps::createJoin(Session *wf, ResourceType *rt,
                                    const string& name,
                                    const vector<JoinSource>& sources,
                                    string& why)
{
  const Code *code = 0;
  for (size_t i = 0; i < codes().size(); i++)
    if (rt->getName() == codes()[i].typeName) code = &codes()[i];
  if (!code || !canJoin(*code)) {
    why = "A join is made with G16 (QST2/QST3) or ORCA (NEB-TS).";
    return 0;
  }
  why = checkRoles(sources);
  if (!why.empty()) return 0;

  const bool neb = string(code->typeName) == "orca_es";
  string picks[3];
  const JoinSource *reactant = 0;
  for (size_t i = 0; i < sources.size(); i++) {
    const JoinSource& js = sources[i];
    int slot = js.role == "reactant" ? 0 : js.role == "product" ? 1 : 2;
    if (slot == 0) reactant = &js;
    picks[slot] = qstMakePick(js.step->getURL().toString(), js.point);
    // Every structure must be there before the step exists.
    vector<string> el;
    vector<double> xyz;
    bool opt;
    string w;
    if (!qstPickStructure(picks[slot], el, xyz, opt, 0, &w)) {
      why = js.step->getName() + " has no structure to take" +
            (w.empty() ? "." : ": " + w);
      return 0;
    }
  }

  Resource *made = 0;
  try {
    made = wf->createChild(name, rt);
    if (!made) throw InvalidException("The step could not be created", WHERE);
    wf->addMember(made);
    for (size_t i = 0; i < sources.size(); i++)
      wf->addLink(sources[i].step, made, Session::INPUT_PROVIDER,
                  linkName(sources[i].role, sources[i].point));
    wf->saveLinkbase();

    ICalculation *to = dynamic_cast<ICalculation*>(made);
    ChemistryTask *toTask = dynamic_cast<ChemistryTask*>(made);
    ICalculation *from = dynamic_cast<ICalculation*>(reactant->step);
    if (!to || !toTask)
      throw InvalidException("The step is not a calculation", WHERE);
    Fragment *frag = qstReactantFragment(picks[0]);
    if (!frag)
      throw InvalidException("The reactant has no structure", WHERE);
    toTask->fragment(frag);
    delete frag;
    if (from) {
      to->spinMultiplicity(from->spinMultiplicity());
      to->openShells(from->openShells());
    }

    // The keys the TS Search of the Calculation Editor stores.
    TRunType runtype;
    runtype.name(joinRunType(*code));
    to->runtype(runtype);
    const char *keys[3] = {
      neb ? "ES.Runtype.NEB.Reactant" : "ES.Runtype.GeomOpt.QSTReactant",
      neb ? "ES.Runtype.NEB.Product" : "ES.Runtype.GeomOpt.QSTProduct",
      neb ? "ES.Runtype.NEB.Guess" : "ES.Runtype.GeomOpt.QSTGuess"};
    GUIValues *g = to->guiparams();
    if (!g) g = new GUIValues();
    for (int k = 0; k < 3; k++) {
      if (picks[k].empty()) continue;
      GUIValue *e = new GUIValue();
      e->m_key = keys[k];
      e->m_type = "menu_input";
      e->m_sensitive = true;
      e->m_write = true;
      g->set(e->m_key, e);
      e->setValue(picks[k]);
    }
    to->guiparams(g);
    delete g;
  }
  catch (InvalidException& ex) {
    why = ex.what();
    if (made) {
      try { wf->removeMember(made); } catch (...) {}
      made->remove();
    }
    return 0;
  }
  return made;
}


namespace {
struct Walk {
  vector<Session::Link*> links;
  vector<WorkflowSteps::Node> out;
  std::set<const Resource*> seen;
  std::map<const Resource*, vector<WorkflowSteps::Incoming> > in;

  bool allSeen(const Resource *step) const {
    std::map<const Resource*, vector<WorkflowSteps::Incoming> >::const_iterator
        it = in.find(step);
    if (it == in.end()) return false;
    const vector<WorkflowSteps::Incoming> &v = it->second;
    for (size_t i = 0; i < v.size(); i++)
      if (!seen.count(v[i].source)) return false;
    return true;
  }

  // A step with several sources is reached from the last of them.
  void go(Resource *step) {
    if (!seen.insert(step).second) return;
    WorkflowSteps::Node n;
    n.step = step;
    n.in = in[step];
    n.source = n.in.size() == 1 ? n.in[0].source : 0;
    n.depth = 0;
    out.push_back(n);
    for (size_t i = 0; i < links.size(); i++)
      if (links[i]->linkType == Session::INPUT_PROVIDER &&
          links[i]->source == step && allSeen(links[i]->target))
        go(links[i]->target);
  }
};
}


vector<WorkflowSteps::Node> WorkflowSteps::tree(Session *wf)
{
  Walk walk;
  walk.links = wf->getLinks();
  vector<Resource*> members = wf->getMembers();
  for (size_t i = 0; i < members.size(); i++) walk.in[members[i]];
  for (size_t k = 0; k < walk.links.size(); k++)
    if (walk.links[k]->linkType == Session::INPUT_PROVIDER &&
        walk.in.count(walk.links[k]->target) && walk.in.count(walk.links[k]->source))
      walk.in[walk.links[k]->target].push_back(
          {walk.links[k]->source, walk.links[k]->linkName});
  for (size_t i = 0; i < members.size(); i++)
    if (walk.in[members[i]].empty()) walk.go(members[i]);

  std::map<const Resource*, int> depth;
  for (size_t i = 0; i < walk.out.size(); i++) {
    Node &n = walk.out[i];
    for (size_t k = 0; k < n.in.size(); k++)
      n.depth = std::max(n.depth, depth[n.in[k].source] + 1);
    depth[n.step] = n.depth;
  }
  return walk.out;
}


vector<Resource*> WorkflowSteps::readySteps(Session *wf, vector<string> *waiting)
{
  vector<Resource*> ready;
  vector<Node> nodes = tree(wf);
  for (size_t i = 0; i < nodes.size(); i++) {
    Resource *step = nodes[i].step;
    if (isLaunched(step)) continue;
    // An excluded step is not launched (#617).
    if (!step->getProp(VDoc::getEcceNamespace() + ":excluded").empty()) continue;
    bool sourcesDone = true;
    for (size_t k = 0; k < nodes[i].in.size(); k++)
      if (!isComplete(nodes[i].in[k].source)) sourcesDone = false;
    if (sourcesDone) ready.push_back(step);
    else if (waiting) waiting->push_back(step->getName());
  }
  return ready;
}


namespace {
string sourcesText(const WorkflowSteps::Node& n)
{
  if (n.in.empty()) return "-";
  string out;
  for (size_t i = 0; i < n.in.size(); i++)
    out += (i ? "+" : "") + n.in[i].source->getName();
  return out;
}
}


string WorkflowSteps::info(Session *wf)
{
  std::ostringstream out;
  vector<Node> nodes = tree(wf);
  for (size_t i = 0; i < nodes.size(); i++) {
    Resource *step = nodes[i].step;
    ICalculation *calc = dynamic_cast<ICalculation*>(step);
    PropertyTask *task = dynamic_cast<PropertyTask*>(step);
    Fragment *frag = task ? task->fragment() : 0;
    const Code *code = codeOf(step);
    if (i) out << "|";
    out << step->getName() << " " << (code ? code->label : "?") << " "
        << ResourceUtils::stateToString(stateOf(step)) << " "
        << sourcesText(nodes[i]) << " "
        << "charge=" << (frag ? frag->charge() : -99) << " "
        << "mult=" << (calc ? (int)calc->spinMultiplicity() : -1) << " "
        << "atoms=" << (frag ? (int)frag->numAtoms() : -1);
    if (needsStructure(step, !nodes[i].in.empty())) out << " needs=1";
    TTheory *th = calc ? calc->theory() : 0;
    if (th && !th->name().empty()) out << " theory=" << th->name();
    delete th;
    GUIValues *gv = calc ? calc->guiparams() : 0;
    GUIValue *xc = gv ? gv->get("ES.Theory.DFT.XCFunctionals") : 0;
    if (xc && !xc->getValueAsString().empty()) {
      string v = xc->getValueAsString();
      std::replace(v.begin(), v.end(), ' ', '_');
      out << " xc=" << v;
    }
    if (gv && code) {
      string model, solvent;
      auto value = [&](const char *key) {
        GUIValue *v = gv->get(key);
        return v ? v->getValueAsString() : string();
      };
      if (code->label == string("CREST") || code->label == string("xtb")) {
        model = value("ES.Theory.Solvent.Model");
        solvent = value("ES.Theory.Solvent.Name");
      } else if (code->label == string("ORCA")) {
        model = value("ES.Theory.SCF.UseSolvation") == "1" ? value("ES.Theory.SCF.SolvationModel") : "None";
        solvent = value("ES.Theory.SCF.Solvent");
      } else if (code->label == string("G16")) {
        model = value("ES.Theory.SCF.UseSCRF") == "1" ? value("ES.Theory.SCF.SCRF") : "None";
        solvent = value("ES.Theory.SCF.Solvent");
      } else if (code->label == string("ECCE-QM")) {
        solvent = value("ES.Theory.ECCEQM.Solvent");
        model = solvent == "None" || solvent.empty() ? "None" : "CPCM";
      }
      if (!model.empty()) out << " solvent-model=" << model << " solvent=" << solvent;
      const char *toggleKey = code->label == string("G16") ? "ES.Theory.SCF.UseSCRF"
                              : code->label == string("ORCA") ? "ES.Theory.SCF.UseSolvation" : "";
      GUIValue *toggle = gv->get(toggleKey);
      if (toggle) out << " solvent-toggle-type=" << toggle->m_type;
    }
    delete gv;
    TGBSConfig *gbs = calc ? calc->gbsConfig() : 0;
    if (gbs && !gbs->name().empty()) out << " basis=" << gbs->name();
    delete gbs;
    if (frag && frag->numAtoms() > 0) {
      const double *c = frag->atomRef(0)->coordinates();
      out << " xyz0=" << c[0] << "," << c[1] << "," << c[2];
      Fragment *last = task->fragment();
      bool result = last && task->getFragmentStep(last, -1);
      if (result) {
        const double *f = last->atomRef(0)->coordinates();
        out << " final0=" << f[0] << "," << f[1] << "," << f[2];
      }
      delete last;
    }
    delete frag;
  }
  out << " links:";
  vector<Session::Link*> links = wf->getLinks();
  for (size_t i = 0; i < links.size(); i++)
    if (links[i]->linkType == Session::INPUT_PROVIDER)
      out << " " << links[i]->source->getName() << ">"
          << links[i]->target->getName() << ":" << links[i]->linkName;
  return out.str();
}


// ---- recipes (#710) ----

namespace {

const string kRecipeProp = ":recipe";

string nsKey(const char *suffix)
{
  return VDoc::getEcceNamespace() + suffix;
}

string uniqueName(Session *wf, const string& want)
{
  std::set<string> used;
  vector<Resource*> members = wf->getMembers();
  for (size_t i = 0; i < members.size(); i++) used.insert(members[i]->getName());
  if (!used.count(want)) return want;
  for (int n = 2;; n++) {
    string name = want + "-" + std::to_string(n);
    if (!used.count(name)) return name;
  }
}

const WorkflowSteps::Code *codeByLabel(const string& label)
{
  for (size_t i = 0; i < WorkflowSteps::codes().size(); i++)
    if (label == WorkflowSteps::codes()[i].label) return &WorkflowSteps::codes()[i];
  return 0;
}

ResourceType *typeNamed(const string& name)
{
  vector<ResourceType*> types =
      ResourceDescriptor::getResourceDescriptor().getResourceTypes();
  for (size_t i = 0; i < types.size(); i++)
    if (types[i]->getName() == name) return types[i];
  return 0;
}

void setKey(GUIValues *g, const string& key, const string& value)
{
  GUIValue *e = g->get(key);
  if (!e) {
    e = new GUIValue();
    e->m_key = key;
    e->m_type = "menu_input";
    e->m_sensitive = true;
    e->m_write = true;
    g->set(key, e);
  }
  e->setValue(value);
}

// The library names d and p polarisation with stars; a recipe may use either.
string libraryBasisName(const string& name)
{
  string n = name;
  const char *from[2] = {"(d,p)", "(d)"}, *to[2] = {"**", "*"};
  for (int k = 0; k < 2; k++) {
    size_t at = n.find(from[k]);
    if (at != string::npos) n.replace(at, strlen(from[k]), to[k]);
  }
  return n;
}

// A recipe method names a functional as the papers do (M06-2X, wB97X-D);
// the codes' choice lists drop dashes, so compare without them.
string methodKey(const string& m)
{
  string k;
  for (char c : m) if (c != '-' && c != ' ') k += (char)tolower((unsigned char)c);
  return k;
}

// Stores the role's method as the step's theory and functional, for codes
// whose functional list is known here (see WorkflowSteps::methodValue).
void applyRoleMethod(Resource *step)
{
  const string method = step->getProp(nsKey(":recipe.method"));
  ICalculation *calc = dynamic_cast<ICalculation*>(step);
  if (method.empty() || !calc) return;
  if (calc->getState() >= ResourceDescriptor::STATE_SUBMITTED) return;
  const string type = step->getDescriptor() ? step->getDescriptor()->getName() : "";
  const bool hf = methodKey(method) == "hf";
  const string value = hf ? "" : WorkflowSteps::methodValue(type, method);
  if (!hf && value.empty()) return;
  // ECCE-QM names its theories after the category ("DFT", "HF").
  const bool qm = type == "ecceqm_es";
  TTheory theory(hf ? "SCF" : "DFT", qm ? (hf ? "HF" : "DFT") : hf ? "RHF" : "RDFT");
  calc->theory(&theory);
  if (hf) return;
  GUIValues *g = calc->guiparams();
  if (!g) g = new GUIValues();
  setKey(g, "ES.Theory.DFT.XCFunctionals", value);
  calc->guiparams(g);
  delete g;
}

// A basis set is per element, so the role's one is stored once the step
// has a structure; a name the library does not know leaves the step as is.
void applyRoleBasis(Resource *step)
{
  const string name = step->getProp(nsKey(":recipe.basis"));
  ICalculation *calc = dynamic_cast<ICalculation*>(step);
  PropertyTask *task = dynamic_cast<PropertyTask*>(step);
  if (name.empty() || !calc || !task) return;
  if (calc->getState() >= ResourceDescriptor::STATE_SUBMITTED) return;
  Fragment *frag = task->fragment();
  const string tags = frag && frag->numAtoms() > 0 ? frag->uniqueTagStr() : "";
  delete frag;
  if (tags.empty()) return;
  EDSIServerCentral central;
  EDSIGaussianBasisSetLibrary lib(central.getDefaultBasisSetLibrary());
  const string names[2] = {name, libraryBasisName(name)};
  for (int k = 0; k < 2; k++) {
    vector<TGaussianBasisSet*> found =
        lib.lookup(names[k].c_str(), TGaussianBasisSet::UnknownGBSType, tags.c_str());
    for (size_t i = 0; i < found.size(); i++) delete found[i];
    if (found.empty()) continue;
    TGBSConfig *cfg = lib.simpleLookup(names[k].c_str(), tags.c_str());
    calc->gbsConfig(cfg);
    delete cfg;
    return;
  }
}

// Copies what a link carries from `source` into `made`.
void carryInto(Resource *source, Resource *made, const string& point)
{
  ICalculation *from = dynamic_cast<ICalculation*>(source);
  ICalculation *to = dynamic_cast<ICalculation*>(made);
  PropertyTask *fromTask = dynamic_cast<PropertyTask*>(source);
  ChemistryTask *toTask = dynamic_cast<ChemistryTask*>(made);
  if (!from || !to || !fromTask || !toTask) return;
  Fragment *frag = 0;
  if (!point.empty()) {
    string w;
    frag = ScanPoint::fragment(fromTask, point, w);
  } else {
    frag = fromTask->fragment();
    if (frag) fromTask->getFragmentStep(frag, -1);
  }
  if (frag && frag->numAtoms() > 0) toTask->fragment(frag);
  delete frag;
  to->spinMultiplicity(from->spinMultiplicity());
  to->openShells(from->openShells());
  applyRoleBasis(made);
}

}  // namespace


vector<Resource*> WorkflowSteps::applyRecipe(Session *wf, const Recipe& recipe,
                                             const RecipeVariant& variant,
                                             const Levels& levels,
                                             Resource *after, string& why)
{
  vector<Resource*> made;
  std::map<string, Resource*> byId;
  try {
    vector<string> names;
    {
      std::set<string> used;
      vector<Resource*> members = wf->getMembers();
      for (size_t i = 0; i < members.size(); i++) used.insert(members[i]->getName());
      for (size_t i = 0; i < variant.steps.size(); i++) {
        string n = variant.steps[i].id;
        for (int k = 2; used.count(n); k++) n = variant.steps[i].id + "-" + std::to_string(k);
        used.insert(n);
        names.push_back(n);
      }
    }
    for (size_t i = 0; i < variant.steps.size(); i++) {
      const RecipeStep& rs = variant.steps[i];
      const Code *code = codeByLabel(rs.code);
      ResourceType *rt = code ? typeNamed(code->typeName) : 0;
      if (!rt) throw InvalidException("This installation has no " + rs.code + ".", WHERE);
      Resource *step = wf->createChild(names[i], rt);
      if (!step) throw InvalidException("The step " + rs.id + " could not be created", WHERE);
      // A step exists as a link source only once something is stored in it.
      ICalculation *first = dynamic_cast<ICalculation*>(step);
      if (first) {
        TRunType rtype;
        rtype.name(rs.runType);
        first->runtype(rtype);
      }
      made.push_back(step);
      byId[rs.id] = step;
    }
    // Session::createChild re-reads the linkbase from disk, dropping members
    // not yet saved, so the steps join the session only once all exist; the
    // session's own objects are used, as the links compare pointers.
    {
      vector<Resource*> *kids = wf->getChildren(false);
      for (size_t i = 0; i < made.size(); i++)
        for (size_t k = 0; kids && k < kids->size(); k++)
          if ((*kids)[k]->getURL() == made[i]->getURL()) {
            made[i] = (*kids)[k];
            byId[variant.steps[i].id] = (*kids)[k];
          }
      for (size_t i = 0; i < made.size(); i++)
        if (!wf->addMember(made[i]))
          throw InvalidException("The step " + variant.steps[i].id +
                                 " could not join the workflow", WHERE);
      if (after) {
        vector<Resource*> members = wf->getMembers();
        for (size_t k = 0; k < members.size(); k++)
          if (members[k]->getURL() == after->getURL()) after = members[k];
      }
    }
    // Links, with the picks of a TS search (what createJoin stores).
    std::map<string, string> picks[3];
    vector<bool> hasSource(variant.steps.size(), false);
    for (size_t k = 0; k < variant.links.size(); k++) {
      const RecipeLink& l = variant.links[k];
      wf->addLink(byId[l.from], byId[l.to], Session::INPUT_PROVIDER, l.name);
      for (size_t i = 0; i < variant.steps.size(); i++)
        if (variant.steps[i].id == l.to) hasSource[i] = true;
      string role, point;
      splitLinkName(l.name, role, point);
      const int slot = role == "reactant" ? 0 : role == "product" ? 1
                       : role == "tsguess" ? 2 : -1;
      if (slot >= 0) picks[slot][l.to] = qstMakePick(byId[l.from]->getURL().toString(), point);
    }
    if (after)
      for (size_t i = 0; i < variant.steps.size(); i++)
        if (!hasSource[i]) {
          wf->addLink(after, made[i], Session::INPUT_PROVIDER, kGeometryLink);
          carryInto(after, made[i], "");
          hasSource[i] = true;
        }
    wf->saveLinkbase();

    for (size_t i = 0; i < variant.steps.size(); i++) {
      const RecipeStep& rs = variant.steps[i];
      Resource *step = made[i];
      ICalculation *calc = dynamic_cast<ICalculation*>(step);
      if (!calc) continue;
      TRunType runtype;
      runtype.name(rs.runType);
      calc->runtype(runtype);
      GUIValues *g = calc->guiparams();
      if (!g) g = new GUIValues();
      const bool neb = string(codeByLabel(rs.code)->typeName) == "orca_es";
      const char *keys[3] = {
        neb ? "ES.Runtype.NEB.Reactant" : "ES.Runtype.GeomOpt.QSTReactant",
        neb ? "ES.Runtype.NEB.Product" : "ES.Runtype.GeomOpt.QSTProduct",
        neb ? "ES.Runtype.NEB.Guess" : "ES.Runtype.GeomOpt.QSTGuess"};
      for (int k = 0; k < 3; k++)
        if (picks[k].count(rs.id)) setKey(g, keys[k], picks[k][rs.id]);
      for (size_t k = 0; k < rs.settings.size(); k++)
        setKey(g, rs.settings[k].first, rs.settings[k].second);
      Levels::const_iterator lv = levels.find(rs.role);
      const string method = lv == levels.end() ? "" : lv->second.first;
      const string basis = lv == levels.end() ? "" : lv->second.second;
      auto solvent = levels.find("solvent");
      const string solventName = solvent == levels.end() ? "none" : solvent->second.second;
      for (const auto& kv : recipeSolventSettings(rs.code, solventName)) {
        setKey(g, kv.first, kv.second);
        // The theory editor restores checkboxes according to the wire type.
        if (kv.first == "ES.Theory.SCF.UseSCRF" || kv.first == "ES.Theory.SCF.UseSolvation")
          g->get(kv.first)->m_type = "toggle_input";
      }
      calc->guiparams(g);
      delete g;

      step->addProp(nsKey(":recipe.step"), rs.id);
      step->addProp(nsKey(":recipe.role"), rs.role);
      step->addProp(nsKey(":recipe.key"), rs.keyRole);
      step->addProp(nsKey(":recipe.thermo"), rs.thermo ? "1" : "0");
      step->addProp(nsKey(":recipe.method"), method);
      step->addProp(nsKey(":recipe.basis"), basis);
      step->addProp(nsKey(":recipe.caption"), rs.caption);
      applyRoleMethod(step);
      applyRoleBasis(step);
    }
    wf->addProp(nsKey(kRecipeProp.c_str()), recipe.id + "/" + variant.id);
  }
  catch (InvalidException& ex) {
    why = ex.what();
    for (size_t i = made.size(); i-- > 0;) {
      try { wf->removeMember(made[i]); } catch (...) {}
      made[i]->remove();
    }
    try { wf->saveLinkbase(); } catch (...) {}
    return vector<Resource*>();
  }
  return made;
}


bool WorkflowSteps::needsStructure(Resource *step, bool hasSource)
{
  if (hasSource || !step || step->getProp(nsKey(":recipe.step")).empty()) return false;
  PropertyTask *task = dynamic_cast<PropertyTask*>(step);
  Fragment *frag = task ? task->fragment() : 0;
  const bool empty = !frag || frag->numAtoms() == 0;
  delete frag;
  return empty;
}


string WorkflowSteps::recipeId(Session *wf)
{
  return wf ? wf->getProp(nsKey(kRecipeProp.c_str())) : string();
}


string WorkflowSteps::refreshFromSources(Session *wf, Resource *step)
{
  if (step->getProp(nsKey(":recipe.step")).empty()) return "";
  bool carried = false;
  for (Session::Link *l : wf->getLinks()) {
    if (l->linkType != Session::INPUT_PROVIDER || l->target != step) continue;
    string role, point;
    splitLinkName(l->linkName, role, point);
    if ((role == "geometry" || role == "reactant") &&
        hasResultGeometry(l->source)) {
      carryInto(l->source, step, role == "reactant" ? "" : point);
      carried = true;
    }
  }
  if (carried) structureSet(wf, step);
  return "";
}


string WorkflowSteps::methodValue(const string& codeType, const string& method)
{
  // Copies of the functional lists in ged16theory.py, orcatheory.py and ecceqmtheory.py: a
  // method neither list has is left to the Calculation Editor.
  static const char *g16[] = {"B3LYP (hybrid)", "B3PW91 (hybrid)", "B97D3 (nonlocal)",
    "BLYP (nonlocal)", "BP86 (GGA)", "CAM-B3LYP (range)", "M06 (hybrid)",
    "M062X (hybrid)", "M06L (meta-GGA)", "MN15 (hybrid)", "PBE (GGA)",
    "PBE0 (hybrid)", "PBE1PBE (hybrid)", "TPSSh (hybrid)", "wB97XD (range)",
    "X3LYP (hybrid)", 0};
  static const char *orca[] = {"B2PLYP", "B3LYP", "B97-D3", "B97M-V", "BLYP", "BP86",
    "CAM-B3LYP", "DSD-PBEP86", "M06", "M062X", "M06L", "PBE", "PBE0", "PWPB95",
    "r2SCAN", "revPBE", "TPSS", "TPSSh", "wB97M-V", "wB97X-D3", "wB97X-V", 0};
  static const char *qm[] = {"SVWN", "PBE", "B3LYP", "PBE0", 0};   // XC_FUNCTIONALS, ecceqmtheory.py
  const char **list = codeType == "gaussian16_es" ? g16 : codeType == "orca_es" ? orca
                    : codeType == "ecceqm_es" ? qm : 0;
  if (!list) return "";
  const string want = methodKey(method);
  for (int i = 0; list[i]; i++) {
    string name = list[i];
    const size_t paren = name.find(" (");
    if (methodKey(paren == string::npos ? name : name.substr(0, paren)) == want)
      return name;
  }
  return "";
}


string WorkflowSteps::structureSet(Session *wf, Resource *step)
{
  if (!wf || !step || step->getProp(nsKey(":recipe.step")).empty()) return "";
  const string rid = recipeId(wf);
  StructureFacts facts;
  PropertyTask *task = dynamic_cast<PropertyTask*>(step);
  Fragment *frag = task ? task->fragment() : 0;
  if (frag && frag->numAtoms() > 0) {
    facts.known = true;
    facts.charge = frag->charge();
    facts.atoms = (int)frag->numAtoms();
    for (size_t i = 0; i < frag->numAtoms(); i++) {
      const int z = frag->atomRef(i)->atomicNumber();
      if (std::find(facts.elements.begin(), facts.elements.end(), z) == facts.elements.end())
        facts.elements.push_back(z);
    }
  }
  delete frag;
  if (!facts.known) return "";

  const Recipe *recipe = 0;
  vector<Recipe> all = loadRecipes(recipeDirs());
  const string id = rid.substr(0, rid.find('/'));
  for (size_t i = 0; i < all.size(); i++) if (all[i].id == id) recipe = &all[i];
  vector<Resource*> members = wf->getMembers();
  std::ostringstream out;
  for (size_t m = 0; m < members.size(); m++) {
    Resource *r = members[m];
    const string role = r->getProp(nsKey(":recipe.role"));
    if (role.empty()) continue;
    ICalculation *calc = dynamic_cast<ICalculation*>(r);
    if (calc && calc->getState() >= ResourceDescriptor::STATE_SUBMITTED) continue;
    for (size_t k = 0; recipe && k < recipe->roles.size(); k++) {
      const RecipeRole& rr = recipe->roles[k];
      if (rr.id != role) continue;
      // Only a level that is still a recommendation follows the structure;
      // one typed in the set-up panel stays.
      const RoleChoice blank = recommend(*recipe, rr, StructureFacts());
      const RoleChoice now = recommend(*recipe, rr, facts);
      const string ruled = r->getProp(nsKey(":recipe.ruled"));
      const string method = r->getProp(nsKey(":recipe.method"));
      const string basis = r->getProp(nsKey(":recipe.basis"));
      const bool asRecommended = (method == blank.method && basis == blank.basis) ||
                                 method + "|" + basis == ruled;
      if (asRecommended) {
        r->addProp(nsKey(":recipe.method"), now.method);
        r->addProp(nsKey(":recipe.basis"), now.basis);
        r->addProp(nsKey(":recipe.ruled"), now.method + "|" + now.basis);
      }
    }
    applyRoleMethod(r);
    applyRoleBasis(r);
    out << (out.tellp() > 0 ? " " : "") << r->getName() << "="
        << r->getProp(nsKey(":recipe.method")) << "/" << r->getProp(nsKey(":recipe.basis"));
  }
  return out.str();
}


vector<KeyStep> WorkflowSteps::keySteps(Session *wf)
{
  vector<KeyStep> out;
  vector<Node> nodes = tree(wf);
  for (size_t i = 0; i < nodes.size(); i++) {
    Resource *step = nodes[i].step;
    if (step->getProp(nsKey(":recipe.step")).empty()) continue;
    KeyStep k;
    k.name = step->getName();
    k.keyRole = step->getProp(nsKey(":recipe.key"));
    k.thermo = step->getProp(nsKey(":recipe.thermo")) == "1";
    k.complete = isComplete(step);
    if (k.complete) k.term = gatherEnergyTerm(step->getURL().toString(), 1);
    out.push_back(k);
  }
  return out;
}


bool WorkflowSteps::toRecipe(Session *wf, const string& id, const string& title,
                             Recipe& out, string& why)
{
  vector<Node> nodes = tree(wf);
  if (nodes.empty()) { why = "The workflow has no steps."; return false; }
  out = Recipe();
  out.id = id;
  out.title = title;
  out.field = "Mine";
  out.summary = "Saved from the workflow " + wf->getName();
  out.description = out.summary + ": its settings and links, without structures or results.";
  RecipeVariant v;
  v.id = "main";
  std::map<const Resource*, string> ids;
  for (size_t i = 0; i < nodes.size(); i++) {
    Resource *step = nodes[i].step;
    const Code *code = codeOf(step);
    ICalculation *calc = dynamic_cast<ICalculation*>(step);
    if (!code || !calc) continue;
    RecipeStep s;
    string name = step->getName();
    for (size_t c = 0; c < name.size(); c++)
      if (name[c] == '|' || name[c] == ' ') name[c] = '_';
    s.id = name;
    ids[step] = s.id;
    s.code = code->label;
    s.runType = calc->runtype().name();
    s.caption = step->getProp(nsKey(":recipe.caption"));
    s.keyRole = step->getProp(nsKey(":recipe.key"));
    s.thermo = step->getProp(nsKey(":recipe.thermo")) == "1";
    s.role = step->getProp(nsKey(":recipe.role"));
    s.needsStructure = nodes[i].in.empty();
    GUIValues *g = calc->guiparams();
    if (g) {
      for (GUIValues::const_iterator it = g->begin(); it != g->end(); ++it) {
        const string val = it->second->getValueAsString();
        if (!it->second->m_write || it->first.compare(0, 3, "ES.") != 0 ||
            it->first.find("QST") != string::npos || it->first.find("NEB.") != string::npos ||
            val.empty() || val.find('|') != string::npos || val.find('\n') != string::npos)
          continue;
        s.settings.push_back(std::make_pair(it->first, val));
      }
      delete g;
    }
    if (v.code.empty()) { v.code = s.code; v.label = s.code; }
    v.steps.push_back(s);
  }
  for (size_t i = 0; i < nodes.size(); i++)
    for (size_t k = 0; k < nodes[i].in.size(); k++)
      if (ids.count(nodes[i].step) && ids.count(nodes[i].in[k].source))
        v.links.push_back({ids[nodes[i].in[k].source], ids[nodes[i].step],
                           nodes[i].in[k].name});
  if (v.steps.empty()) { why = "The workflow has no calculation steps."; return false; }
  // Keep a shared solution setting when the saved recipe opens its selector.
  string solvent = "none";
  for (const string& name : recipeSolvents(v)) {
    if (name == "none") continue;
    bool matches = true, mapped = false;
    for (const RecipeStep& step : v.steps) {
      const auto settings = recipeSolventSettings(step.code, name);
      if (settings.empty()) continue;
      mapped = true;
      for (const auto& kv : settings)
        if (std::find(step.settings.begin(), step.settings.end(), kv) == step.settings.end())
          matches = false;
    }
    if (mapped && matches) solvent = name;
  }
  out.roles.push_back({"solvent", "Solvent", "", solvent, "saved workflow solvent"});
  out.variants.push_back(v);
  return true;
}


string WorkflowSteps::userRecipeDir()
{
  return Ecce::userRegistrationDir() + "Recipes";
}
