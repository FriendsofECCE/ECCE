// Reading the energy, code, method and basis set of a calculation.
#include "CompareEnergies.H"

#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/PropertyTask.H"
#include "dsm/TGBSConfig.H"
#include "dsm/VDoc.H"
#include "tdat/PropVector.H"
#include "tdat/TTheory.H"
#include "util/EcceURL.H"

using std::string;

FinalEnergy finalEnergyOf(PropertyTask *task)
{
  FinalEnergy r;
  if (!task) return r;
  // MOPAC's TE is its heat of formation, so it is named as such.
  TProperty *hof = task->getProperty("HFENERGY");
  TProperty *te = hof ? 0 : task->getProperty("TE");
  // In a relaxed scan TE is whichever SCF the code printed last; the
  // result is the last point's optimised energy.
  PropVector *scan = dynamic_cast<PropVector*>(task->getProperty("SCANENERGY"));
  const bool scanned = scan && !scan->values().empty();
  TProperty *e = hof ? hof : te;
  if (!e && !scanned) return r;
  r.found = true;
  r.scanned = scanned;
  r.heat = !scanned && hof;
  r.value = scanned ? scan->values().back() : e->scalarize();
  r.units = scanned ? "Hartree" : e->units();
  return r;
}

EnergyEntry gatherEnergyEntry(const string& url)
{
  EnergyEntry s;
  s.url = url;
  s.name = url;
  PropertyTask *t = url.empty() ? 0 : EDSIFactory::getPropertyTask(EcceURL(url));
  if (!t) return s;
  s.exists = true;
  s.name = t->getName();
  s.code = t->getProp(VDoc::getEcceNamespace() + ":application");
  s.energy = finalEnergyOf(t);
  ICalculation *c = dynamic_cast<ICalculation*>(t);
  if (c) {
    TTheory *th = c->theory();
    if (th) {
      s.method = th->toConciseString();
      delete th;
    }
    TGBSConfig *gbs = c->gbsConfig();
    if (gbs) {
      s.basis = gbs->name();
      delete gbs;
    }
  }
  return s;
}

std::vector<std::pair<string, string> > calculationsIn(const string& projectUrl)
{
  std::vector<std::pair<string, string> > out;
  Resource *project = EDSIFactory::getResource(EcceURL(projectUrl));
  std::vector<Resource*> *kids = project ? project->getChildren() : 0;
  if (!kids) return out;
  for (size_t i = 0; i < kids->size(); i++)
    if (dynamic_cast<PropertyTask*>((*kids)[i]))
      out.push_back(std::make_pair((*kids)[i]->getName(),
                                   (*kids)[i]->getURL().toString()));
  return out;
}

