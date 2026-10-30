#include <set>
#include <sstream>

#include "tdat/Fragment.H"
#include "tdat/SpinMult.H"
#include "util/InvalidException.H"
#include "util/ResourceUtils.H"

#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/OrbitalGuess.H"
#include "dsm/TaskJob.H"
#include "dsm/ChemistryTask.H"
#include "dsm/PropertyTask.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"

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


bool WorkflowSteps::canCarryHessian(Resource *source, ResourceType *toType)
{
  ResourceType *from = source ? source->getDescriptor() : 0;
  return from && toType && from->getName() == "orca_es" &&
         toType->getName() == "orca_es" &&
         OrbitalGuess::hessianAvailable(dynamic_cast<TaskJob*>(source));
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
    made = wf->createChild(name, rt);
    if (!made) throw InvalidException("The step could not be created", WHERE);
    wf->addMember(made);
    if (source)
      wf->addLink(source, made, Session::INPUT_PROVIDER, kGeometryLink);
    wf->saveLinkbase();

    ICalculation *from = dynamic_cast<ICalculation*>(source);
    ICalculation *to = dynamic_cast<ICalculation*>(made);
    PropertyTask *fromTask = dynamic_cast<PropertyTask*>(source);
    ChemistryTask *toTask = dynamic_cast<ChemistryTask*>(made);
    if (from && to && fromTask && toTask) {
      if (carry.geometry) {
        Fragment *frag = fromTask->fragment();
        if (frag) {
          fromTask->getFragmentStep(frag, -1);
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
    if (carry.hessian && canCarryHessian(source, rt)) {
      TaskJob *target = dynamic_cast<TaskJob*>(made);
      string hessWhy;
      if (!target ||
          !OrbitalGuess::attachHessian(target, dynamic_cast<TaskJob*>(source),
                                       hessWhy))
        throw InvalidException(hessWhy.empty() ? "The Hessian could not be "
                               "carried over" : hessWhy, WHERE);
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


namespace {
struct Walk {
  vector<Session::Link*> links;
  vector<WorkflowSteps::Node> out;
  std::set<const Resource*> seen;

  void go(Resource *step, Resource *source, int depth) {
    if (!seen.insert(step).second) return;
    out.push_back({step, source, depth});
    for (size_t i = 0; i < links.size(); i++)
      if (links[i]->linkType == Session::INPUT_PROVIDER &&
          links[i]->source == step)
        go(links[i]->target, step, depth + 1);
  }
};
}


vector<WorkflowSteps::Node> WorkflowSteps::tree(Session *wf)
{
  Walk walk;
  walk.links = wf->getLinks();
  vector<Resource*> members = wf->getMembers();
  for (size_t i = 0; i < members.size(); i++) {
    bool hasSource = false;
    for (size_t k = 0; k < walk.links.size(); k++)
      if (walk.links[k]->linkType == Session::INPUT_PROVIDER &&
          walk.links[k]->target == members[i]) hasSource = true;
    if (!hasSource) walk.go(members[i], 0, 0);
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
    if (!nodes[i].source || isComplete(nodes[i].source)) ready.push_back(step);
    else if (waiting) waiting->push_back(step->getName());
  }
  return ready;
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
        << (nodes[i].source ? nodes[i].source->getName() : "-") << " "
        << "charge=" << (frag ? frag->charge() : -99) << " "
        << "mult=" << (calc ? (int)calc->spinMultiplicity() : -1) << " "
        << "atoms=" << (frag ? (int)frag->numAtoms() : -1);
    TaskJob *job = dynamic_cast<TaskJob*>(step);
    if (job && job->hasHessianInput())
      out << " hess=" << job->getProp(TaskJob::hessianFromSourceProp());
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
