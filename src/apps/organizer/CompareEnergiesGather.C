// Reading the energy, code, method and basis set of a calculation.
#include "CompareEnergies.H"


#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/PropertyTask.H"
#include "dsm/TGBSConfig.H"
#include "dsm/VDoc.H"
#include "tdat/Fragment.H"
#include "tdat/GUIValues.H"
#include "tdat/PropString.H"
#include "tdat/PropValue.H"
#include "tdat/PropVector.H"
#include "tdat/SpinMult.H"
#include "tdat/TAtm.H"
#include "tdat/TTheory.H"
#include "util/EcceURL.H"

using std::string;
using std::vector;

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

namespace {
double valueOf(PropertyTask *t, const char *key, bool *have)
{
  PropValue *p = dynamic_cast<PropValue*>(t->getProperty(key));
  *have = p != 0;
  return p ? p->value() : 0;
}
}

EnergyTerm gatherEnergyTerm(const string& url, int coeff)
{
  EnergyTerm s;
  s.coeff = coeff;
  s.state.name = url;
  PropertyTask *t = url.empty() ? 0 : EDSIFactory::getPropertyTask(EcceURL(url));
  if (!t) {
    s.state.problem = "the calculation cannot be read.";
    return s;
  }
  s.state.name = t->getName();
  s.code = t->getProp(VDoc::getEcceNamespace() + ":application");
  const FinalEnergy fe = finalEnergyOf(t);
  if (!fe.found) s.state.problem = "it has no final energy.";
  else if (!finalEnergyHartree(fe, &s.state.energy))
    s.state.problem = "its energy is in " + fe.units + ", which cannot be converted.";
  s.heat = fe.heat;
  s.scanned = fe.scanned;

  bool z, h, g, tt;
  const double zpe = valueOf(t, "EZEROPT", &z);
  const double enth = valueOf(t, "ENTHALPY", &h);
  const double gibbs = valueOf(t, "EGIBBS", &g);
  const double temp = valueOf(t, "THERMOTEMP", &tt);
  if (z && h && g) {
    s.state.thermo = true;
    s.state.zpe = zpe;
    s.state.enthalpy = enth;
    s.state.gibbs = gibbs;
    s.state.temperature = tt ? temp : 0;
  }
  PropString *sm = dynamic_cast<PropString*>(t->getProperty("SOLVMODEL"));
  if (sm) s.state.solvent = sm->value();

  ICalculation *c = dynamic_cast<ICalculation*>(t);
  if (c) {
    TTheory *th = c->theory();
    if (th) {
      s.state.method = th->toConciseString();
      delete th;
    }
    TGBSConfig *gbs = c->gbsConfig();
    if (gbs) {
      s.state.basis = gbs->name();
      delete gbs;
    }
    s.state.multiplicity = SpinMult::toString(c->spinMultiplicity());
    GUIValues *gv = c->guiparams();
    GUIValue *d = gv ? gv->get("ES.Theory.SCF.Dispersion") : 0;
    if (d) s.dispersion = d->getValueAsString();
    delete gv;
    Fragment *frag = c->fragment();
    if (frag) {
      s.state.charge = frag->charge();
      vector<TAtm*> *atoms = frag->atoms();
      if (atoms) {
        for (size_t i = 0; i < atoms->size(); i++)
          s.atoms[(*atoms)[i]->atomicSymbol()]++;
        delete atoms;
      }
      delete frag;
    }
  }
  return s;
}

vector<EnergyTerm> gatherSide(const string& spec)
{
  vector<EnergyTerm> out;
  const vector<std::pair<string, int> > terms = parseSide(spec);
  for (size_t i = 0; i < terms.size(); i++)
    out.push_back(gatherEnergyTerm(terms[i].first, terms[i].second));
  return out;
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

