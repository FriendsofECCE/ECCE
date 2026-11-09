#include "SiExport.H"

#include <cstdio>
#include <cstdlib>
#include "util/Filesystem.H"
#include <fstream>

#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/PropertyTask.H"
#include "dsm/TGBSConfig.H"
#include "dsm/VDoc.H"
#include "tdat/Fragment.H"
#include "tdat/LatticeDef.H"
#include "util/Transformations.H"
#include "tdat/PropString.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/PropValue.H"
#include "tdat/PropVector.H"
#include "tdat/SpinMult.H"
#include "tdat/TAtm.H"
#include "util/Ecce.H"

using std::string;
using std::vector;
namespace fs = ecce::fs;

namespace {

const char *NS = "http://www.emsl.pnl.gov/ecce:";

string quote(const string& s)
{
  string q;
  for (char c : s) if (c != '\'' && c != '"') q += c;
  return "'" + q + "'";
}

// Same names and file layout as a stored calculation, so the script reads
// scratch folders and real ones alike.
bool writeFolder(PropertyTask *t, const fs::path& dir)
{
  std::error_code ec;
  fs::create_directories(dir / "Props", ec);
  fs::create_directories(dir / "Parameters", ec);
  char buf[64];
  for (const char *name : {"TE", "EGIBBS", "S2", "EZEROPT"}) {
    PropValue *v = dynamic_cast<PropValue*>(t->getProperty(name));
    if (!v) continue;
    snprintf(buf, sizeof buf, "%.15g", v->value());
    std::ofstream(dir / "Props" / name) << "<value name=\"" << name
        << "\" units=\"Hartree\">" << buf << "</value>\n";
  }
  if (PropString *sv = dynamic_cast<PropString*>(t->getProperty("SOLVMODEL")))
    std::ofstream(dir / "Props" / "SOLVMODEL") << "<string name=\"SOLVMODEL\">"
        << sv->value() << "</string>\n";
  PropVector *vf = dynamic_cast<PropVector*>(t->getProperty("VIBFREQ"));
  if (vf) {
    std::ofstream f(dir / "Props" / "VIBFREQ");
    f << "<vector name=\"VIBFREQ\" rows=\"" << vf->values().size() << "\">";
    for (double d : vf->values()) { snprintf(buf, sizeof buf, "%.10g ", d); f << buf; }
    f << "</vector>\n";
  }

  Fragment *frag = t->fragment();
  if (!frag) return false;
  const int n = (int)frag->numAtoms();
  PropTSVecTable *tr = dynamic_cast<PropTSVecTable*>(t->getProperty("GEOMTRACE"));
  const vector<double> *last = 0;
  if (tr && tr->rows() == n && tr->tables() > 0) last = &tr->values(tr->tables() - 1);
  if (last && (int)last->size() < 3 * n) last = 0;
  std::ofstream m(dir / "Parameters" / "chemsys.mvm");
  m << "charge: " << frag->charge() << "\nnum_atoms: " << n
    << "\natom_info: symbol cart\natom_list:\n";
  for (int i = 0; i < n; i++) {
    const TAtm *a = frag->atomRef(i);
    const double *c = a->coordinates();
    double x = last ? (*last)[3*i] : c[0], y = last ? (*last)[3*i+1] : c[1],
           z = last ? (*last)[3*i+2] : c[2];
    snprintf(buf, sizeof buf, " %.10f %.10f %.10f", x, y, z);
    m << a->atomicSymbol() << buf << "\n";
  }
  m << "attr_list:\n";
  string mult, theory, basis;
  ICalculation *c = dynamic_cast<ICalculation*>(t);
  if (c) {
    mult = SpinMult::toString(c->spinMultiplicity());
    // The stored "category/name", as the script reads it from a real folder.
    theory = t->getProp(VDoc::getEcceNamespace() + ":theory");
    TGBSConfig *g = c->gbsConfig();
    if (g) { basis = g->name(); delete g; }
  }
  // The cell, for the Rank table's same-cell check (#703).
  if (LatticeDef *lat = frag->getLattice())
    if (vector<MPoint> *v = lat->toVectors()) {
      m.close();
      string text;
      {
        std::ifstream in(dir / "Parameters" / "chemsys.mvm");
        text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
      }
      string cell = "lattice_vectors:\n";
      for (int k = 0; k < 3 && k < (int)v->size(); k++) {
        snprintf(buf, sizeof buf, "%.6f %.6f %.6f\n", (*v)[k].x(), (*v)[k].y(), (*v)[k].z());
        cell += buf;
      }
      delete v;
      const size_t at = text.find("num_atoms:");
      if (at != string::npos) text.insert(at, cell);
      std::ofstream(dir / "Parameters" / "chemsys.mvm") << text;
    }
  delete frag;
  // ecce-thermo (Rank's --treatment) needs the stored calculation itself.
  std::ofstream(dir / ".ecce-meta", std::ios::app)
      << ".\t" << NS << "sourceurl\t\t" << t->getURL().toString() << "\n";
  // The Checks table reads the state, run type and last progress snapshot
  // (its newlines written as the two characters \n, as the store does).
  string progress = t->getProp(VDoc::getEcceNamespace() + ":progress");
  for (size_t at; (at = progress.find('\n')) != string::npos; )
    progress.replace(at, 1, "\\n");
  std::ofstream(dir / ".ecce-meta", std::ios::app)
      << ".\t" << NS << "state\t\t" << t->getProp(VDoc::getEcceNamespace() + ":state") << "\n"
      << ".\t" << NS << "runtype\t\t" << t->getProp(VDoc::getEcceNamespace() + ":runtype") << "\n"
      << ".\t" << NS << "progress\t\t" << progress << "\n";
  std::ofstream(dir / ".ecce-meta", std::ios::app)
      << ".\t" << NS << "application\t\t"
      << t->getProp(VDoc::getEcceNamespace() + ":application") << "\n"
      << ".\t" << NS << "theory\t\t" << theory << "\n"
      << ".\t" << NS << "spinmultiplicity\t\t" << mult << "\n"
      << ".\t" << NS << "excluded\t\t"
      << t->getProp(VDoc::getEcceNamespace() + ":excluded") << "\n";
  // Booked and used core-hours (#627) come from the launch settings and the
  // job's own start and end.
  for (const char *k : {"launch_totalprocs", "launch_maxwall", "startdate",
                        "completiondate"})
    std::ofstream(dir / ".ecce-meta", std::ios::app)
        << ".\t" << NS << k << "\t\t"
        << t->getProp(VDoc::getEcceNamespace() + ":" + k) << "\n";
  std::ofstream(dir / "Parameters" / ".ecce-meta")
      << "BasisSet.ecce_basisset\t" << NS << "name\t\t" << basis << "\n";
  return true;
}

}  // namespace

// Scratch folders for `calcs`, then the script with `args` appended.
static bool runScript(const vector<EcceURL>& calcs, const string& tail,
                      string& text, string& why,
                      const GatherProgress& progress = GatherProgress())
{
  static int serial = 0;
  const fs::path tmp = fs::temp_directory_path() /
      ("ecce-chk-" + std::to_string((long)fs::file_time_type::clock::now().time_since_epoch().count()) + "-" + std::to_string(serial++));
  std::error_code ec;
  string args;
  int n = 0;
  size_t seen = 0;
  for (const EcceURL& u : calcs) {
    PropertyTask *t = EDSIFactory::getPropertyTask(u);
    // The gather stays on this (GUI) thread, as EDSI is not thread-safe.
    if (progress && !progress(seen++, calcs.size(), t ? t->getName() : string())) {
      fs::remove_all(tmp, ec);
      why = "Cancelled.";
      return false;
    }
    if (!t) continue;
    string name = t->getName();
    for (char& ch : name) if (ch == '/' || ch == '\\') ch = '_';
    if (!writeFolder(t, tmp / name)) continue;
    args += " " + quote((tmp / name).string());
    n++;
  }
  if (!n) { why = "No calculation with a structure to list."; return false; }
  const char *home = getenv("ECCE_HOME");
  string script = home && *home ? string(home) + "/bin/ecce-si-export"
                                : string("ecce-si-export");
#ifdef _WIN32
  fs::path py = fs::path(Ecce::ecceHome()) / "python" / "python.exe";
  string pyExe = fs::exists(py) ? py.string() : string("python.exe");
  string cmd = "\"" + pyExe + "\" \"" + script + "\"" + tail + args;
#else
  string cmd = "'" + script + "'" + tail + args + " 2>/dev/null";
#endif
  int status = Ecce::readCommand(cmd, text);
  fs::remove_all(tmp, ec);
  if (status != 0) { why = "ecce-si-export failed"; return false; }
  return true;
}

bool checksTable(const vector<EcceURL>& calcs, const string& format,
                 string& text, string& why)
{
  return runScript(calcs, " --checks " + format, text, why);
}

bool rankTable(const vector<EcceURL>& calcs, const string& format,
               string& text, string& why, const GatherProgress& progress,
               const string& ref)
{
  return runScript(calcs, " --rank " + format +
                   (ref.empty() ? string() : " --ref " + quote(ref)),
                   text, why, progress);
}

bool siExport(const vector<EcceURL>& calcs, const string& xyzFile,
              const string& table, string& why, bool includeExcluded)
{
  static int serial = 0;
  const fs::path tmp = fs::temp_directory_path() /
      ("ecce-si-" + std::to_string((long)fs::file_time_type::clock::now().time_since_epoch().count()) + "-" + std::to_string(serial++));
  std::error_code ec;
  string args;
  int n = 0;
  for (const EcceURL& u : calcs) {
    PropertyTask *t = EDSIFactory::getPropertyTask(u);
    if (!t) continue;
    // One folder per calculation, named after it (the xyz comment's name).
    string name = t->getName();
    for (char& ch : name) if (ch == '/' || ch == '\\') ch = '_';
    if (!writeFolder(t, tmp / name)) continue;
    args += " " + quote((tmp / name).string());
    n++;
  }
  if (!n) { why = "None of the selected items is a calculation with a structure."; return false; }
  args += " -o " + quote(xyzFile);
  if (!table.empty()) args += " --table " + table;
  if (includeExcluded) args += " --include-excluded";

  const char *home = getenv("ECCE_HOME");
  string script = home && *home ? string(home) + "/bin/ecce-si-export"
                                : string("ecce-si-export");
#ifdef _WIN32
  fs::path py = fs::path(Ecce::ecceHome()) / "python" / "python.exe";
  string pyExe = fs::exists(py) ? py.string() : string("python.exe");
  string cmd = "\"" + pyExe + "\" \"" + script + "\"" + args;
#else
  string cmd = "'" + script + "'" + args + " 2>&1";
#endif
  string text;
  int status = Ecce::readCommand(cmd, text);
  fs::remove_all(tmp, ec);
  if (status != 0) { why = "ecce-si-export failed: " + text; return false; }
  return true;
}
