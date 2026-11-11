#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "dsm/UmbrellaWindows.H"
#include "dsm/EDSIFactory.H"
#include "dsm/GromacsMDModel.H"
#include "dsm/MdTask.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "dsm/TaskJob.H"
#include "tdat/DynamicsModel.H"
#include "tdat/InteractionModel.H"
#include "util/InvalidException.H"
#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/Filesystem.H"
#include "util/MiniJson.H"
#include "util/SFile.H"
#include <unistd.h>
#include "dsm/VDoc.H"

using std::string;
using std::vector;

string UmbrellaWindows::propName(const char *what)
{
  return VDoc::getEcceNamespace() + ":umbrella." + what;
}

static ResourceType *typeNamed(const string& name)
{
  vector<ResourceType*> types =
      ResourceDescriptor::getResourceDescriptor().getResourceTypes();
  for (size_t i = 0; i < types.size(); i++)
    if (types[i]->getName() == name) return types[i];
  return 0;
}

static string readFile(const string& path)
{
  std::ifstream in(path.c_str(), std::ios::binary);
  std::ostringstream all;
  all << in.rdbuf();
  return all.str();
}

//  One window task: linked after `from`, the pull task's model with no
//  pull, and the window's structure when it has one of its own.
static MdTask *makeTask(Session *session, ResourceType *type, MdTask *from,
                        MdTask *pull, const string& name, int steps,
                        bool resume, const UmbrellaWindows::Window& w,
                        const string& groBytes, const string& role,
                        string *why)
{
  Resource *made = session->createChild(name, type);
  MdTask *task = dynamic_cast<MdTask*>(made);
  if (!task) { if (why) *why = "could not create " + name; return 0; }
  try {
    session->addMemberAsTarget(task, from);
  } catch (InvalidException& ex) {
    task->remove();
    if (why) *why = string("could not link ") + name + ": " + ex.what();
    return 0;
  }
  //  The model comes from the pull task, not from `from`: the equilibration
  //  task's own values would otherwise reach the production one by way of
  //  the study's defaults, but the pull settings must be the pull task's.
  GromacsMDModel model(task->getContentType());
  pull->getTaskModel(model);
  if (InteractionModel *im = model.getInteractionModel()) {
    GromacsRun run = im->getGromacsRun();
    run.pullRate = 0;
    im->setGromacsRun(run);
  }
  if (DynamicsModel *dm = model.getDynamicsModel()) {
    dm->setEquilibrationSteps(0);
    dm->setDataSteps(steps);
    dm->setResumeOpt(resume);
  }
  task->setTaskModel(&model);

  if (!groBytes.empty()) {
    char fname[64];
    snprintf(fname, sizeof fname, "window_%02d.gro", w.number);
    std::istringstream in(groBytes);
    if (!task->putInputFile(fname, &in)) {
      if (why) *why = "the structure could not be stored on " + name;
      return 0;
    }
  }
  task->launchdata(pull->launchdata());

  char num[16];
  snprintf(num, sizeof num, "%d", w.number);
  char dist[32];
  snprintf(dist, sizeof dist, "%.4f", w.distance);
  task->addProp(UmbrellaWindows::propName("pull"), pull->getURL().toString());
  task->addProp(UmbrellaWindows::propName("window"), num);
  task->addProp(UmbrellaWindows::propName("role"), role);
  task->addProp(UmbrellaWindows::propName("distance"), dist);
  return task;
}

bool UmbrellaWindows::create(MdTask *pull, const vector<Window>& windows,
                             int eqSteps, int prodSteps,
                             vector<string> *names, string *why)
{
  Session *session = pull ? pull->getSession() : 0;
  if (!session) { if (why) *why = "the pull run is not in a study"; return false; }
  if (pull->getApplicationType() != ResourceDescriptor::AT_GROMACS ||
      pull->getContentType() != ResourceDescriptor::CT_MDDYNAMICS) {
    if (why) *why = "not a GROMACS Dynamics task";
    return false;
  }
  ResourceType *eqType = typeNamed("gromacs_md_equilibrate");
  ResourceType *prodType = typeNamed("gromacs_md_dynamics");
  if (!eqType || !prodType) {
    if (why) *why = "the GROMACS task types are not registered";
    return false;
  }
  const string base = pull->getURL().getFilePathTail();
  for (size_t i = 0; i < windows.size(); i++) {
    const Window& w = windows[i];
    const string groBytes = readFile(w.groFile);
    if (groBytes.empty()) {
      if (why) *why = "cannot read " + w.groFile;
      return false;
    }
    char tag[16];
    snprintf(tag, sizeof tag, "-w%02d", w.number);
    MdTask *prev = pull;
    MdTask *eq = 0;
    if (eqSteps > 0) {
      eq = makeTask(session, eqType, pull, pull, base + tag + "-eq", eqSteps,
                    false, w, groBytes, "eq", why);
      if (!eq) return false;
      if (names) names->push_back(eq->getURL().getFilePathTail());
      prev = eq;
    }
    //  Production continues from the equilibration's structure and
    //  velocities; with none, it starts from the window structure itself.
    MdTask *prod = makeTask(session, prodType, prev, pull, base + tag,
                            prodSteps, eq != 0, w, eq ? string() : groBytes,
                            "prod", why);
    if (!prod) return false;
    if (names) names->push_back(prod->getURL().getFilePathTail());
  }
  session->saveLinkbase();
  return true;
}

vector<Resource*> UmbrellaWindows::tasks(MdTask *pull, const string& role)
{
  vector<Resource*> out;
  Session *session = pull ? pull->getSession() : 0;
  if (!session) return out;
  const string url = pull->getURL().toString();
  vector<Resource*> members = session->getMembers();
  for (size_t i = 0; i < members.size(); i++)
    if (members[i]->getProp(propName("pull")) == url &&
        members[i]->getProp(propName("role")) == role)
      out.push_back(members[i]);
  std::sort(out.begin(), out.end(), [](Resource *a, Resource *b) {
    return atoi(a->getProp(propName("window")).c_str()) <
           atoi(b->getProp(propName("window")).c_str());
  });
  return out;
}

static string quote(const string& a)
{
  string q = "'";
  for (char c : a) q += c == '\'' ? string("'\\''") : string(1, c);
  return q + "'";
}

bool UmbrellaWindows::runCli(const vector<string>& args, MiniJson *out,
                             string *why)
{
  const char *home = getenv("ECCE_HOME");
  string cmd = home && *home ? quote(string(home) + "/bin/ecce-umbrella")
                             : string("ecce-umbrella");
  cmd += " --json";
  for (const string& a : args) cmd += " " + quote(a);
  cmd += " 2>/dev/null";
  string text;
  Ecce::readCommand(cmd, text);
  while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
    text.pop_back();
  const size_t nl = text.rfind('\n');
  const string last = nl == string::npos ? text : text.substr(nl + 1);
  string err;
  MiniJson j;
  if (!MiniJson::parse(last, j, err) || j.type() != MiniJson::Object) {
    if (why) *why = "ecce-umbrella could not be run (is it installed?)";
    return false;
  }
  if (!j.get("error").str().empty()) {
    if (why) *why = j.get("error").str();
    return false;
  }
  if (out) *out = j;
  return true;
}

string UmbrellaWindows::localFolder(Resource *task, const vector<string>& files)
{
  const string url = task->getURL().toString();
  if (url.compare(0, 7, "file://") == 0) {
    string p = url.substr(7);
    while (p.size() > 1 && p.back() == '/') p.pop_back();
    return p;
  }
  TaskJob *job = dynamic_cast<TaskJob*>(task);
  if (!job) return "";
  static int serial = 0;
  const ecce::fs::path dir = ecce::fs::temp_directory_path() /
      ("ecce-umb-" + std::to_string((long)getpid()) + "-" + std::to_string(serial++));
  std::error_code ec;
  ecce::fs::create_directories(dir / "Outputs", ec);
  for (const string& f : files) {
    SFile *dest = new SFile((dir / "Outputs" / f).string());
    SFile *got = job->getAnyFile(task->getURL().getChild("Outputs").getChild(f), dest);
    if (got != dest) delete got;
    delete dest;
  }
  return dir.string();
}

bool UmbrellaWindows::find(MdTask *pull, double spacing, const string& from,
                           const string& to, const string& outDir,
                           vector<Window> *windows, MiniJson *json,
                           string *why)
{
  const string folder = localFolder(pull,
      { "traj.xtc", "topol.tpr", "pullx.xvg", "pullf.xvg" });
  if (folder.empty()) { if (why) *why = "the pull run has no stored files"; return false; }
  vector<string> args = { outDir.empty() ? "windows" : "extract", folder };
  if (!outDir.empty()) { args.push_back("--out"); args.push_back(outDir); }
  char sp[32];
  snprintf(sp, sizeof sp, "%g", spacing);
  args.push_back("--spacing"); args.push_back(sp);
  if (!from.empty()) { args.push_back("--from"); args.push_back(from); }
  if (!to.empty()) { args.push_back("--to"); args.push_back(to); }
  MiniJson j;
  if (!runCli(args, &j, why)) return false;
  if (windows) {
    windows->clear();
    int n = 0;
    for (const MiniJson& w : j.get("windows").items()) {
      Window x;
      x.number = ++n;
      x.distance = atof(w.get("distance").str().c_str());
      x.groFile = w.get("file").str();
      windows->push_back(x);
    }
  }
  if (json) *json = j;
  return true;
}

bool UmbrellaWindows::wham(MdTask *pull, double temperature, bool jacobian,
                           bool csv, MiniJson *json, string *csvText,
                           int *used, string *why)
{
  vector<string> args = { "wham" };
  int n = 0;
  vector<Resource*> prod = tasks(pull, "prod");
  for (Resource *r : prod) {
    TaskJob *t = dynamic_cast<TaskJob*>(r);
    if (!t || t->getState() != ResourceDescriptor::STATE_COMPLETED) continue;
    const string f = localFolder(r, { "topol.tpr", "pullf.xvg", "pullx.xvg" });
    if (f.empty()) continue;
    args.push_back(f);
    n++;
  }
  if (used) *used = n;
  if (n < 2) {
    if (why) *why = prod.empty()
        ? "this pull run has no window calculations: create them first"
        : "fewer than two window calculations are Complete (" +
          std::to_string(n) + " of " + std::to_string(prod.size()) + ")";
    return false;
  }
  char tbuf[32];
  snprintf(tbuf, sizeof tbuf, "%g", temperature);
  args.push_back("--temp"); args.push_back(tbuf);
  if (jacobian) args.push_back("--jacobian");
  if (!csv) return runCli(args, json, why);
  //  --csv replaces --json: run it as a plain command.
  const char *home = getenv("ECCE_HOME");
  string cmd = home && *home ? quote(string(home) + "/bin/ecce-umbrella")
                             : string("ecce-umbrella");
  for (const string& a : args) cmd += " " + quote(a);
  cmd += " --csv 2>/dev/null";
  string text;
  const int rc = Ecce::readCommand(cmd, text);
  if (rc != 0 || text.empty()) { if (why) *why = "ecce-umbrella wham --csv failed"; return false; }
  if (csvText) *csvText = text;
  return true;
}
