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
#include "util/ThermoRun.H"

#include <algorithm>
#include <cmath>
#include <map>

using std::string;
using std::vector;

FinalEnergy finalEnergyOf(PropertyTask *task)
{
  FinalEnergy r;
  if (!task) return r;
  // An IRC's result is its TS: the energy at reaction coordinate 0 (the
  // last SCF is an arbitrary end of the path, and G16 prints no TE).
  PropVector *ircE = dynamic_cast<PropVector*>(task->getProperty("IRCENERGY"));
  PropVector *ircC = dynamic_cast<PropVector*>(task->getProperty("IRCCOORD"));
  if (ircE && ircC && !ircE->values().empty() &&
      ircE->values().size() == ircC->values().size()) {
    size_t at = 0;
    for (size_t i = 1; i < ircC->values().size(); i++)
      if (fabs(ircC->values()[i]) < fabs(ircC->values()[at])) at = i;
    r.found = true;
    r.ts = true;
    r.value = ircE->values()[at];
    r.units = "Hartree";
    return r;
  }
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
  s.url = url;
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
    GUIValue *xc = gv ? gv->get("ES.Theory.DFT.XCFunctionals") : 0;
    if (xc) s.xc = xc->getValueAsString();
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


string applyThermoTreatments(SumResult& r, const vector<EnergyTerm>& a,
                             const vector<EnergyTerm>& b, const string& treatment)
{
  SumRow *g = 0;
  for (SumRow& w : r.rows)
    if (w.label.compare(0, 1, "G") == 0) g = &w;
  if (!g || !g->available) return "";
  std::map<string, ThermoRunResult> runs;
  for (const vector<EnergyTerm> *side : {&a, &b})
    for (const EnergyTerm& t : *side) {
      if (runs.count(t.url)) continue;
      const double T = t.state.temperature > 0 ? t.state.temperature : 298.15;
      runs[t.url] = runThermo(t.url, T, false);
      if (!runs[t.url].ok)
        return "G over treatments needs the stored frequencies: " +
               t.state.name + ": " + runs[t.url].error;
    }
  const vector<ThermoTreatmentInfo>& tr = thermoTreatments();
  double lo = 0, hi = 0, ga = 0, gb = 0;
  bool first = true;
  for (const ThermoTreatmentInfo& info : tr) {
    double sa = 0, sb = 0;
    for (int s = 0; s < 2; s++)
      for (const EnergyTerm& t : s ? b : a) {
        const ThermoValue *v = runs[t.url].find(info.cli);
        if (!v) return "ecce-thermo gave no values for " + string(info.cli) + ".";
        (s ? sb : sa) += t.coeff * v->G;
      }
    const double d = sb - sa;
    if (first || d < lo) lo = d;
    if (first || d > hi) hi = d;
    first = false;
    if (treatment == info.cli) { ga = sa; gb = sb; }
  }
  if (!treatment.empty()) {
    g->a = ga;
    g->b = gb;
    for (const ThermoTreatmentInfo& info : tr)
      if (treatment == info.cli)
        r.notes.push_back(string("G is recomputed by ecce-thermo: ") + info.label +
                          ", 1 atm; the spread is the largest minus the smallest "
                          "\xCE\x94G over the six treatments of the low frequencies under G treatment (RRHO; Grimme qRRHO with \xCF\x89" "0 = 50, 100 and 200 cm\xE2\x81\xBB\xC2\xB9; Truhlar; Head-Gordon).");
  }
  if (treatment.empty())
    r.notes.push_back("G is the code's own; the spread is the largest minus the "
                      "smallest \xCE\x94G over the six treatments of the low frequencies under G treatment (RRHO; Grimme qRRHO with \xCF\x89" "0 = 50, 100 and 200 cm\xE2\x81\xBB\xC2\xB9; Truhlar; Head-Gordon), at 1 atm.");
  g->hasSpread = true;
  g->spread = hi - lo;
  return "";
}
