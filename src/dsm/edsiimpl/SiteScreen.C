#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ICalculation.H"
#include "dsm/JCode.H"
#include "dsm/PropertyTask.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/SiteScreen.H"
#include "dsm/TaskJob.H"
#include "dsm/VDoc.H"
#include "tdat/Fragment.H"
#include "tdat/LatticeDef.H"
#include "tdat/PropTSVecTable.H"
#include "tdat/PropValue.H"
#include "tdat/TAtm.H"
#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/Filesystem.H"

using std::string;
using std::vector;

namespace {

const double kEvPerHartree = 27.211386245988;
const string kNote = VDoc::getEcceNamespace() + ":annotation";

string quote(const string& s) { return "'" + s + "'"; }

// Why not an MLP calculation of a periodic structure, or "".
string problem(const EcceURL& url, bool needMlp)
{
  PropertyTask *task = dynamic_cast<PropertyTask*>(EDSIFactory::getResource(url));
  ICalculation *calc = dynamic_cast<ICalculation*>(task);
  if (!task || !calc) return "This is not a calculation.";
  Fragment *frag = task->fragment();
  const bool periodic = frag && frag->getLattice();
  delete frag;
  if (!periodic) return "The structure has no periodic cell.";
  const JCode *code = calc->application();
  if (needMlp && (!code || code->name() != "MLP"))
    return "Site screening runs MLP (MACE) calculations. Duplicate this "
           "calculation as an MLP calculation of the host first, then screen "
           "from that one.";
  return "";
}

// extxyz with the cell, as ecce-sites reads it.
bool writeHost(PropertyTask *task, const string& path)
{
  Fragment *frag = task->fragment();
  if (!frag || !frag->getLattice()) { delete frag; return false; }
  vector<MPoint> *v = frag->getLattice()->toVectors();
  std::ofstream f(path);
  f << frag->numAtoms() << "\nLattice=\"";
  for (int i = 0; i < 3; i++) {
    MPoint p = (*v)[i];
    char b[96];
    snprintf(b, sizeof b, "%s%.10f %.10f %.10f", i ? " " : "", p.x(), p.y(), p.z());
    f << b;
  }
  f << "\" Properties=species:S:1:pos:R:3 pbc=\"T T T\"\n";
  for (size_t i = 0; i < frag->numAtoms(); i++) {
    const TAtm *a = frag->atomRef((int)i);
    const double *c = a->coordinates();
    char b[160];
    snprintf(b, sizeof b, "%s %.10f %.10f %.10f\n", a->atomicSymbol().c_str(), c[0], c[1], c[2]);
    f << b;
  }
  delete v;
  delete frag;
  return (bool)f;
}

string slurp(const string& path)
{
  std::ifstream f(path);
  std::ostringstream s;
  s << f.rdbuf();
  return s.str();
}

// The "screen=... site=..." part of an extxyz comment line.
string description(const string& xyzPath)
{
  std::ifstream f(xyzPath);
  string line;
  std::getline(f, line);
  std::getline(f, line);
  string::size_type s = line.find("screen=");
  return s == string::npos ? string() : line.substr(s);
}

string xmlEscape(const string& s)
{
  string o;
  for (char c : s) {
    if (c == '&') o += "&amp;"; else if (c == '<') o += "&lt;";
    else if (c == '>') o += "&gt;"; else if (c == '"') o += "&quot;";
    else o += c;
  }
  return o;
}

// One finished calculation: energy, final structure and cell.
struct Member {
  SiteScreen::Row row;
  vector<string> el;
  vector<double> xyz;
  int site;
};

}  // namespace


string SiteScreen::field(const string& d, const string& key)
{
  const string k = key + "=";
  for (string::size_type p = d.find(k); p != string::npos; p = d.find(k, p + 1)) {
    if (p != 0 && d[p - 1] != ' ') continue;
    string::size_type b = p + k.size();
    if (b < d.size() && d[b] == '"') {
      string::size_type e = d.find('"', b + 1);
      return d.substr(b + 1, e == string::npos ? string::npos : e - b - 1);
    }
    return d.substr(b, d.find(' ', b) - b);
  }
  return "";
}


bool SiteScreen::isPeriodic(const EcceURL& calc)
{
  return problem(calc, false).empty();
}


string SiteScreen::templateProblem(const EcceURL& calc)
{
  return problem(calc, true);
}


SiteScreen::MadeSites SiteScreen::make(const EcceURL& templ, const Options& opt)
{
  MadeSites out;
  string why = problem(templ, true);
  if (!why.empty()) { out.message = why; return out; }
  if (opt.element.empty() || opt.element.find_first_not_of(
          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") != string::npos) {
    out.message = "Give the element as a symbol, for example Li.";
    return out;
  }
  PropertyTask *task = dynamic_cast<PropertyTask*>(EDSIFactory::getResource(templ));
  Resource *tres = task;

  const ecce::fs::path tmp = ecce::fs::temp_directory_path() /
      ("ecce-sites-" + std::to_string(getpid()) + "-" +
       std::to_string((long)ecce::fs::file_time_type::clock::now().time_since_epoch().count()));
  std::error_code ec;
  ecce::fs::create_directories(tmp, ec);
  const string dir = tmp.string(), host = dir + "/in.xyz", sites = dir + "/out";
  if (!writeHost(task, host)) {
    out.message = "The structure could not be written for the search.";
    ecce::fs::remove_all(tmp, ec);
    return out;
  }
  char num[64];
  string cmd = Ecce::ecceBinCommand("ecce-sites") + (opt.vacancy ? " vacancy " : " interstitial ") +
               quote(host) + " --element " + quote(opt.element);
  if (!opt.vacancy) {
    snprintf(num, sizeof num, " --min-host %g --min-sep %g", opt.minHost, opt.minSep);
    cmd += num;
  } else if (opt.maxDist > 0) {
    snprintf(num, sizeof num, " --max-dist %g", opt.maxDist);
    cmd += num;
  }
  cmd += " --out " + quote(sites) + " > " + quote(dir + "/log") + " 2>&1";
  const int rc = system(cmd.c_str());
  vector<string> files;
  for (auto& e : ecce::fs::directory_iterator(sites, ec)) {
    const string n = e.path().filename().string();
    if (n.compare(0, 5, "site-") == 0 && n.size() > 4 && n.compare(n.size() - 4, 4, ".xyz") == 0)
      files.push_back(e.path().string());
  }
  std::sort(files.begin(), files.end());
  if (rc != 0 || files.empty()) {
    out.message = rc != 0 ? "The site search failed:\n" + slurp(dir + "/log")
                          : "No site was found. Lower the minimum distances.";
    ecce::fs::remove_all(tmp, ec);
    return out;
  }

  EcceURL parent = templ.getParent();
  const string folderName = opt.folderName.empty() ? tres->getName() + " sites" : opt.folderName;
  Resource *pres = EDSIFactory::getResource(parent);
  ResourceType *ft = ResourceDescriptor::getResourceDescriptor().getResourceType(
      "collection", "ecceProject", "");
  Resource *folder = EDSIFactory::getResource(parent.getChild(folderName));
  if (!folder && pres && ft) folder = pres->createChild(folderName, ft);
  if (!folder) {
    out.message = "The folder " + folderName + " could not be made.";
    ecce::fs::remove_all(tmp, ec);
    return out;
  }
  out.folderUrl = folder->getURL().toString();

  vector<string> sources;
  sources.push_back(sites + "/host.xyz");
  sources.insert(sources.end(), files.begin(), files.end());
  out.outcomes = Series::make(templ, folder->getURL(), sources);
  int made = 0;
  for (size_t i = 0; i < out.outcomes.size(); i++) {
    Series::Outcome& o = out.outcomes[i];
    if (!o.ok) continue;
    made++;
    if (i == 0) continue;
    Resource *c = EDSIFactory::getResource(EcceURL(o.url));
    const string d = description(sources[i]);
    if (c && !d.empty()) c->addProp(kNote, d);
  }
  ecce::fs::remove_all(tmp, ec);
  out.ok = made > 0;
  char buf[200];
  snprintf(buf, sizeof buf, "%d of %d calculations made in %s (host and %d %s).",
           made, (int)sources.size(), folderName.c_str(), (int)files.size(),
           opt.vacancy ? (files.size() == 1 ? "vacancy pair" : "vacancy pairs")
                       : (files.size() == 1 ? "interstitial site" : "interstitial sites"));
  out.message = buf;
  return out;
}


SiteScreen::Report SiteScreen::results(const EcceURL& at, bool writeEnsemble)
{
  Report rep;
  Resource *r = EDSIFactory::getResource(at);
  EcceURL folderUrl = at;
  if (r && r->getContentType() == ResourceDescriptor::CT_CALCULATION) folderUrl = at.getParent();
  Resource *folder = EDSIFactory::getResource(folderUrl);
  vector<Resource*> *kids = folder ? folder->getChildren() : 0;
  if (!kids) { rep.message = "No such folder."; return rep; }
  rep.folderUrl = folderUrl.toString();
  const string resultsName = folder->getName() + " results";

  vector<Member> members;
  Member host;
  bool haveHost = false;
  for (Resource *k : *kids) {
    TaskJob *job = dynamic_cast<TaskJob*>(k);
    PropertyTask *task = dynamic_cast<PropertyTask*>(k);
    if (!job || !task || k->getName() == resultsName) continue;
    Member m;
    m.row.name = k->getName();
    m.row.url = k->getURL().toString();
    m.row.host = m.row.name == "host";
    m.row.description = k->getProp(kNote);
    m.row.env = field(m.row.description, "env");
    m.site = atoi(field(m.row.description, "site").c_str());
    // An MLP run that stops at its step or time limit ends unsuccessful with
    // its last structure and energy stored: usable for ranking, but flagged.
    const ResourceDescriptor::RUNSTATE st = job->getState();
    m.row.finished = st == ResourceDescriptor::STATE_COMPLETED ||
                     st == ResourceDescriptor::STATE_UNSUCCESSFUL;
    m.row.converged = st == ResourceDescriptor::STATE_COMPLETED;
    m.row.state = !m.row.finished ? "not completed" : m.row.converged ? "completed" : "not converged";
    if (!m.row.host && m.row.description.find("screen=") == string::npos) continue;
    if (m.row.finished) {
      PropValue *te = dynamic_cast<PropValue*>(task->getProperty("TE"));
      Fragment *f = task->getFragmentStep(-1);
      if (!te || !f) m.row.finished = false, m.row.state = "no energy or structure";
      else {
        m.row.te = te->value();
        for (size_t i = 0; i < f->numAtoms(); i++) {
          const TAtm *a = f->atomRef((int)i);
          m.el.push_back(a->atomicSymbol());
          const double *c = a->coordinates();
          m.xyz.insert(m.xyz.end(), c, c + 3);
        }
      }
      delete f;
      if (PropTSVecTable *lv = dynamic_cast<PropTSVecTable*>(task->getProperty("LATTICEVEC"))) {
        if (lv->tables() > 0) {
          char b[400];
          string s;
          const vector<double>& v = lv->values(lv->tables() - 1);
          for (size_t i = 0; i < v.size(); i++) { snprintf(b, sizeof b, "%s%.6f", i ? " " : "", v[i]); s += b; }
          m.row.cell = s;
        }
      }
    }
    if (m.row.host) { host = m; haveHost = true; } else members.push_back(m);
  }
  if (!haveHost) { rep.message = "The folder has no calculation named host."; return rep; }
  if (!host.row.finished) { rep.message = "The host has not completed: " + host.row.state + "."; rep.rows.push_back(host.row); return rep; }
  for (Member& m : members) if (m.row.finished) m.row.de = (m.row.te - host.row.te) * kEvPerHartree;
  std::stable_sort(members.begin(), members.end(), [](const Member& a, const Member& b) {
    if (a.row.finished != b.row.finished) return a.row.finished;
    if (a.row.de != b.row.de) return a.row.de < b.row.de;
    return a.site < b.site;
  });
  rep.rows.push_back(host.row);
  vector<const Member*> done;
  for (const Member& m : members) {
    rep.rows.push_back(m.row);
    if (m.row.finished) done.push_back(&m);
  }
  rep.ok = true;
  char buf[160];
  snprintf(buf, sizeof buf, "%d of %d site calculations completed.", (int)done.size(), (int)members.size());
  rep.message = buf;
  int stopped = 0;
  for (const Member *m : done) if (!m->row.converged) stopped++;
  if (stopped) {
    snprintf(buf, sizeof buf, " %d stopped at the step limit before converging; their last structures "
             "are used and marked \"not converged\".", stopped);
    rep.message += buf;
  }
  if (!host.row.converged)
    rep.message += " The host stopped at the step limit before converging: every difference "
                   "carries its error.";
  if (!writeEnsemble) return rep;
  if (done.empty()) { rep.message += " Nothing to collect yet."; return rep; }

  // Members of one atom count form one GEOMTRACE.
  const size_t n = done[0]->el.size();
  done.erase(std::remove_if(done.begin(), done.end(),
      [n](const Member *m) { return m->el.size() != n; }), done.end());

  EcceURL target = folderUrl.getChild(resultsName);
  if (Resource *old = EDSIFactory::getResource(target)) old->remove();
  Resource *src = EDSIFactory::getResource(EcceURL(done[0]->row.url));
  if (!src || !src->copy(target)) { rep.message += " The results calculation could not be made."; return rep; }
  PropertyTask *res = dynamic_cast<PropertyTask*>(EDSIFactory::getResource(target));
  VDoc *vdoc = EDSIFactory::getVdoc(target);
  if (!res || !vdoc) { rep.message += " The results calculation could not be made."; return rep; }
  res->deleteProperties();

  std::ostringstream geo, de, env;
  string labels;
  for (size_t i = 0; i < n; i++) labels += (i ? " " : "") + std::to_string(i + 1) + "-" + done[0]->el[i];
  geo << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n<tsvectable columnLabel=\"Atom\" "
         "columnLabels=\"X Y Z\" columns=\"3\" name=\"GEOMTRACE\" rowLabel=\"Geometry Step\" rowLabels=\""
      << labels << "\" rows=\"" << n << "\" units=\"Angstrom\" vectorLabel=\"Coordinate\" vectors=\"1\">\n\n";
  de << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n"
        "<tsvector name=\"SITEDE\" rowLabel=\"Geometry Step\" rows=\"1\" units=\"eV\">\n\n";
  string envLabels, envValues;
  for (size_t k = 0; k < done.size(); k++) {
    geo << "<step number=\"" << k + 1 << "\">";
    char b[64];
    for (size_t i = 0; i < done[k]->xyz.size(); i++) {
      snprintf(b, sizeof b, "%s%.10f", i ? " " : "", done[k]->xyz[i]);
      geo << b;
    }
    geo << "</step>\n";
    snprintf(b, sizeof b, "%.8f", done[k]->row.de);
    de << "<step number=\"" << k + 1 << "\">" << b << "</step>\n";
    string e = done[k]->row.env.empty() ? "-" : done[k]->row.env;
    if (!done[k]->row.converged) e = "not converged; " + e;
    std::replace(e.begin(), e.end(), ' ', '_');
    envLabels += (k ? " " : "") + std::to_string(k + 1) + "-" + done[k]->row.name;
    envValues += (k ? " " : "") + e;
  }
  geo << "</tsvectable>\n";
  de << "</tsvector>\n";
  env << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>\n<vecstring name=\"SITEENV\" rowLabel=\"Site\" "
         "rowLabels=\"" << xmlEscape(envLabels) << "\" rows=\"" << done.size() << "\" units=\"NA\">"
      << xmlEscape(envValues) << "</vecstring>\n";
  std::istringstream g(geo.str()), d(de.str()), e(env.str());
  try {
    // Without the mime-type the stored file is not listed as a property
    // (DavPropCache sets it the same way after its put).
    vector<MetaDataResult> mime(1);
    mime[0].name = VDoc::getEcceNamespace() + ":mime-type";
    mime[0].value = "text/xml";
    std::istream *in[] = {&g, &d, &e};
    const char *names[] = {"GEOMTRACE", "SITEDE", "SITEENV"};
    for (int i = 0; i < 3; i++) {
      EDSI *edsi = EDSIFactory::getEDSI(vdoc->addProperty(names[i], in[i]));
      if (edsi) { edsi->putMetaData(mime); delete edsi; }
    }
  } catch (...) {
    rep.message += " The results could not be stored.";
    return rep;
  }
  res->flushPropertyCache();
  std::ostringstream note;
  note << "Site screening results of " << folder->getName() << ": " << done.size()
       << " structures by E - E(host) in eV, lowest first. Rebuild with Site Screening Results.";
  res->addProp(kNote, note.str());
  rep.resultsUrl = target.toString();
  rep.message += " Ensemble in " + resultsName + ".";
  return rep;
}
