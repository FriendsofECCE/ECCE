#include <algorithm>
#include <map>
#include <set>
#include <sstream>

#include "tdat/Fragment.H"
#include "tdat/SpinMult.H"
#include "util/InvalidException.H"
#include "util/ResourceUtils.H"

#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/ChemistryTask.H"
#include "dsm/PropertyTask.H"
#include "dsm/ResourceType.H"
#include "dsm/QstStructures.H"
#include "dsm/ScanPoint.H"
#include "dsm/Session.H"
#include "tdat/GUIValues.H"
#include "tdat/TRunType.H"

#include "WorkflowSteps.H"

using std::string;
using std::vector;

const char *const WorkflowSteps::kGeometryLink = "geometry";

const vector<WorkflowSteps::Code>& WorkflowSteps::codes()
{
  static const vector<Code> list = {
    {"xtb",     "xtb_es",        0x7a, 0x4f, 0xa3},
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
  return point.empty() ? role : role + "@" + point;
}


void WorkflowSteps::splitLinkName(const string& name, string& role,
                                  string& point)
{
  size_t at = name.find('@');
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
