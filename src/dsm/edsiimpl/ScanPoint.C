#include <cstdio>
#include <cstdlib>

#include "dsm/IPropCalculation.H"
#include "dsm/PropertyTask.H"
#include "dsm/ScanPoint.H"
#include "tdat/Fragment.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/PropVector.H"
#include "tdat/TAtm.H"

using std::string;
using std::vector;

namespace {

struct Keys { PropVector *energy; PropTSVecTable *geom; };

Keys keysOf(PropertyTask *task)
{
  Keys k = {0, 0};
  IPropCalculation *calc = task;
  if (!calc) return k;
  const char *pairs[2][2] = {{"SCANENERGY", "SCANGEOM"}, {"IRCENERGY", "IRCGEOM"}};
  for (int i = 0; i < 2 && !k.geom; i++) {
    PropVector *e = dynamic_cast<PropVector*>(calc->getProperty(pairs[i][0]));
    PropTSVecTable *g = dynamic_cast<PropTSVecTable*>(calc->getProperty(pairs[i][1]));
    if (e && g && !e->values().empty() && g->tables() == e->values().size()) {
      k.energy = e;
      k.geom = g;
    }
  }
  return k;
}

}  // namespace


bool ScanPoint::valid(const string& spec)
{
  if (spec.empty() || spec == "last" || spec == "max") return true;
  char *end = 0;
  long n = strtol(spec.c_str(), &end, 10);
  return *end == 0 && n >= 1;
}


string ScanPoint::words(const string& spec)
{
  if (spec.empty()) return "";
  if (spec == "last") return "last point";
  if (spec == "max") return "maximum";
  return "point " + spec;
}


bool ScanPoint::isScan(PropertyTask *calc)
{
  return keysOf(calc).geom != 0;
}


int ScanPoint::frame(PropertyTask *calc, const string& spec, string& why)
{
  Keys k = keysOf(calc);
  if (!k.geom) {
    why = "The calculation has no structure for each point of a scan.";
    return -1;
  }
  const vector<double>& e = k.energy->values();
  int n = (int)e.size();
  if (spec.empty() || spec == "last") return n - 1;
  if (spec == "max") {
    int best = 0;
    for (int i = 1; i < n; i++) if (e[i] > e[best]) best = i;
    return best;
  }
  int p = valid(spec) ? atoi(spec.c_str()) : 0;
  if (p < 1 || p > n) {
    char buf[96];
    snprintf(buf, sizeof buf, "The scan has points 1 to %d, not %s.", n,
             spec.c_str());
    why = buf;
    return -1;
  }
  return p - 1;
}


bool ScanPoint::apply(Fragment *frag, const vector<double>& xyz, string& why)
{
  const int n = frag->numAtoms();
  vector<int> target;
  for (int i = 0; i < n; i++)
    if (frag->atomRef(i)->atomicSymbol() != "X") target.push_back(i);
  if (3 * (size_t)n == xyz.size()) {
    target.clear();
    for (int i = 0; i < n; i++) target.push_back(i);
  } else if (3 * target.size() != xyz.size()) {
    char buf[128];
    snprintf(buf, sizeof buf, "The structure has %d atoms; the calculation has %d.",
             (int)(xyz.size() / 3), n);
    why = buf;
    return false;
  }
  for (size_t k = 0; k < target.size(); k++)
    frag->atomRef(target[k])->coordinates(xyz[3*k], xyz[3*k+1], xyz[3*k+2]);
  return true;
}


Fragment *ScanPoint::fragment(PropertyTask *calc, const string& spec,
                              string& why)
{
  int f = frame(calc, spec, why);
  if (f < 0) return 0;
  Fragment *frag = calc->fragment();
  if (!frag) { why = "The calculation has no chemical system."; return 0; }
  if (!apply(frag, keysOf(calc).geom->values(f), why)) {
    delete frag;
    return 0;
  }
  return frag;
}


bool ScanPoint::structure(PropertyTask *calc, const string& spec,
                          vector<string>& el, vector<double>& xyz, string& why)
{
  el.clear();
  xyz.clear();
  Fragment *frag = fragment(calc, spec, why);
  if (!frag) return false;
  for (size_t i = 0; i < frag->numAtoms(); i++) {
    const TAtm *a = frag->atomRef((int)i);
    el.push_back(a->atomicSymbol());
    const double *c = a->coordinates();
    xyz.push_back(c[0]); xyz.push_back(c[1]); xyz.push_back(c[2]);
  }
  delete frag;
  return !el.empty();
}
