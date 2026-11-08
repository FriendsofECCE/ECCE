#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#include "util/Ecce.H"
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

// Distinguishes the temporary files of two programs running at once.
long pidNow()
{
  return (long)std::filesystem::file_time_type::clock::now()
             .time_since_epoch().count();
}

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

// An ensemble: one GEOMTRACE frame and one ranking value per member, CREST's
// CONFREL (conformers) or a docking code's POSESCORE (poses), lowest first.
Keys ensembleKeys(PropertyTask *task)
{
  Keys k = {0, 0};
  IPropCalculation *calc = task;
  if (!calc) return k;
  PropTSVecTable *g = dynamic_cast<PropTSVecTable*>(calc->getProperty("GEOMTRACE"));
  static const char *score[] = {"CONFREL", "POSESCORE"};
  for (int i = 0; i < 2 && !k.geom; i++) {
    PropVector *e = dynamic_cast<PropVector*>(calc->getProperty(score[i]));
    if (e && g && !e->values().empty() && g->tables() == e->values().size()) {
      k.energy = e;
      k.geom = g;
    }
  }
  return k;
}

// "c3" is the conformer of rank 3 by relative energy; 0 if spec is not one.
int conformerRank(const string& spec)
{
  if (spec.size() < 2 || spec[0] != 'c') return 0;
  char *end = 0;
  long n = strtol(spec.c_str() + 1, &end, 10);
  return (*end == 0 && n >= 1) ? (int)n : 0;
}

// Frame indices, lowest relative energy first (ties keep the file's order).
vector<int> energyOrder(const vector<double>& e)
{
  vector<int> idx(e.size());
  for (size_t i = 0; i < idx.size(); i++) idx[i] = (int)i;
  std::stable_sort(idx.begin(), idx.end(),
                   [&e](int a, int b) { return e[a] < e[b]; });
  return idx;
}

}  // namespace


int ScanPoint::ensembleSize(PropertyTask *calc)
{
  Keys k = ensembleKeys(calc);
  return k.geom ? (int)k.energy->values().size() : 0;
}


bool ScanPoint::ensembleIsPoses(PropertyTask *calc)
{
  IPropCalculation *c = calc;
  return c && !c->getProperty("CONFREL") && c->getProperty("POSESCORE");
}


double ScanPoint::conformerEnergy(PropertyTask *calc, int rank)
{
  Keys k = ensembleKeys(calc);
  if (!k.geom || rank < 1 || rank > (int)k.energy->values().size()) return 0;
  return k.energy->values()[energyOrder(k.energy->values())[rank - 1]];
}


bool ScanPoint::valid(const string& spec)
{
  if (spec.empty() || spec == "last" || spec == "max") return true;
  if (conformerRank(spec)) return true;
  char *end = 0;
  long n = strtol(spec.c_str(), &end, 10);
  return *end == 0 && n >= 1;
}


string ScanPoint::words(const string& spec)
{
  if (spec.empty()) return "";
  if (spec == "last") return "last point";
  if (spec == "max") return "maximum";
  if (conformerRank(spec)) return "conformer " + spec.substr(1);
  return "point " + spec;
}


bool ScanPoint::isScan(PropertyTask *calc)
{
  return keysOf(calc).geom != 0;
}


int ScanPoint::frame(PropertyTask *calc, const string& spec, string& why)
{
  if (conformerRank(spec)) {
    Keys ek = ensembleKeys(calc);
    int rank = conformerRank(spec);
    int n = ek.geom ? (int)ek.energy->values().size() : 0;
    if (!n) {
      why = "The calculation has no conformer or pose ensemble.";
      return -1;
    }
    if (rank > n) {
      char buf[96];
      snprintf(buf, sizeof buf, "The ensemble has %d conformers, not %d.", n, rank);
      why = buf;
      return -1;
    }
    return energyOrder(ek.energy->values())[rank - 1];
  }
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


bool ScanPoint::adoptTraceAtoms(Fragment *frag, const PropTSVecTable *trace,
                                const vector<double>& xyz)
{
  const vector<string> *labels = trace ? trace->rowLabels() : 0;
  size_t real = 0;
  for (int i = 0; i < frag->numAtoms(); i++)
    if (frag->atomRef(i)->atomicSymbol() != "X") real++;
  if (!labels || 3 * labels->size() != xyz.size() ||
      3 * frag->numAtoms() == xyz.size() || 3 * real == xyz.size())
    return false;
  vector<string> el;
  for (const string& l : *labels) {
    size_t dash = l.find('-');
    el.push_back(dash == string::npos ? l : l.substr(dash + 1));
  }
  size_t keep = frag->numAtoms();
  bool prefix = keep < el.size();
  for (size_t i = 0; prefix && i < keep; i++)
    prefix = frag->atomRef((int)i)->atomicSymbol() == el[i];
  if (!prefix) {
    while (frag->numAtoms() > 0) frag->deleteAtom((int)frag->numAtoms() - 1);
    keep = 0;
  }
  for (size_t i = keep; i < el.size(); i++) frag->addAtom(el[i], &xyz[3 * i]);
  return true;
}


Fragment *ScanPoint::fragment(PropertyTask *calc, const string& spec,
                              string& why)
{
  int f = frame(calc, spec, why);
  if (f < 0) return 0;
  Fragment *frag = calc->fragment();
  if (!frag) { why = "The calculation has no chemical system."; return 0; }
  Keys keys = conformerRank(spec) ? ensembleKeys(calc) : keysOf(calc);
  const vector<double>& xyz = keys.geom->values(f);
  if (conformerRank(spec)) adoptTraceAtoms(frag, keys.geom, xyz);
  if (!apply(frag, xyz, why)) {
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


bool ScanPoint::ensemblePatterns(PropertyTask *calc, vector<Pattern>& out,
                                 string& why)
{
  out.clear();
  const int n = ensembleSize(calc);
  if (n < 1) { why = "The calculation has no conformer ensemble."; return false; }

  // Frames go out in rank order, so the script's index is the rank.
  static int serial = 0;
  const std::filesystem::path file = std::filesystem::temp_directory_path() /
      ("ecce-patterns-" + std::to_string(pidNow()) + "-" +
       std::to_string(serial++) + ".xyz");
  {
    std::ofstream f(file);
    if (!f) { why = "Cannot write a temporary file."; return false; }
    for (int k = 1; k <= n; k++) {
      vector<string> el;
      vector<double> xyz;
      if (!structure(calc, "c" + std::to_string(k), el, xyz, why)) {
        f.close();
        std::error_code ec;
        std::filesystem::remove(file, ec);
        return false;
      }
      char line[160];
      snprintf(line, sizeof line, "%d\nenergy: 0 relative: %.6f\n",
               (int)el.size(), conformerEnergy(calc, k));
      f << line;
      for (size_t i = 0; i < el.size(); i++) {
        snprintf(line, sizeof line, "%s %.6f %.6f %.6f\n", el[i].c_str(),
                 xyz[3*i], xyz[3*i+1], xyz[3*i+2]);
        f << line;
      }
    }
  }

  // The script is Python; on Windows it is not directly executable.
  const char *home = getenv("ECCE_HOME");
  string script = home && *home ? string(home) + "/bin/ecce-contacts"
                                : string("ecce-contacts");
#ifdef _WIN32
  // The Windows package carries its own Python (as CalcEd's pythonw); the
  // console python.exe is needed here, since the output is read back.
  std::filesystem::path py = std::filesystem::path(Ecce::ecceHome()) / "python" / "python.exe";
  string pyExe = std::filesystem::exists(py) ? py.string() : string("python.exe");
  string cmd = "\"" + pyExe + "\" \"" + script + "\" \"" + file.string() + "\" --group";
#else
  string cmd = "'" + script + "' '" + file.string() + "' --group 2>/dev/null";
#endif
  string text;
  int status = Ecce::readCommand(cmd, text);
  std::error_code ec;
  std::filesystem::remove(file, ec);
  if (status != 0) {
    why = "ecce-contacts could not be run.";
    return false;
  }

  // Header line, then pattern <tab> count <tab> representative <tab> relative.
  size_t pos = text.find('\n');
  while (pos != string::npos && pos < text.size()) {
    size_t end = text.find('\n', pos + 1);
    string line = text.substr(pos + 1, end == string::npos ? end : end - pos - 1);
    pos = end;
    vector<string> col;
    size_t a = 0, b;
    while ((b = line.find('\t', a)) != string::npos) {
      col.push_back(line.substr(a, b - a));
      a = b + 1;
    }
    col.push_back(line.substr(a));
    if (col.size() < 4) continue;
    Pattern pt;
    pt.label = col[0];
    pt.count = atoi(col[1].c_str());
    pt.rank = atoi(col[2].c_str());
    pt.relative = atof(col[3].c_str());
    if (col.size() > 4) {
      for (size_t from = 0; from < col[4].size();) {
        pt.members.push_back(atoi(col[4].c_str() + from));
        size_t comma = col[4].find(',', from);
        if (comma == string::npos) break;
        from = comma + 1;
      }
    }
    out.push_back(pt);
  }
  if (out.empty() || out[0].label == "no metal") {
    out.clear();
    why = "The ensemble has no ion to group around.";
    return false;
  }
  return true;
}
