#include <dirent.h>
#include <sys/stat.h>
#include <ctime>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#include "GromacsImport.H"

#include "comm/CodePrograms.H"
#include "comm/JobParser.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSI.H"
#include "dsm/GromacsIncludes.H"
#include "dsm/GromacsStructure.H"
#include "dsm/TaskJob.H"
#include "tdat/Fragment.H"
#include "dsm/MdTask.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "util/Ecce.H"
#include "util/MdAnalysisRun.H"

#include "util/Filesystem.H"

using std::string;
using std::vector;

namespace {

vector<string> listDir(const string& dir, bool dirs)
{
  vector<string> out;
  if (DIR* d = opendir(dir.c_str())) {
    while (dirent* e = readdir(d)) {
      string n = e->d_name;
      struct stat st;
      if (n[0] == '.' || stat((dir + "/" + n).c_str(), &st) != 0) continue;
      if (bool(S_ISDIR(st.st_mode)) == dirs) out.push_back(n);
    }
    closedir(d);
  }
  std::sort(out.begin(), out.end());
  return out;
}

bool hasSuffix(const string& s, const string& suf)
{
  return s.size() >= suf.size() &&
         s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

string stem(const string& n)
{
  string::size_type dot = n.find_last_of('.');
  return dot == string::npos ? n : n.substr(0, dot);
}

bool readFile(const string& path, string& out)
{
  std::ifstream in(path.c_str(), std::ios::binary);
  if (!in) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  out = ss.str();
  return true;
}

bool writeFile(const string& path, const string& text)
{
  std::ofstream out(path.c_str(), std::ios::binary);
  out << text;
  return bool(out);
}

// Stored as the Inputs tab stores an attached file: same name replaces.
bool putInput(MdTask* md, const string& name, const string& text)
{
  std::istringstream in(text);
  md->putInputFile(name, 0);
  return md->putInputFile(name, &in);
}

// What a live run's gensub makes with `gmx editconf -o chemsys.pdb`: the
// task's output fragment, which the Builder and Viewer show.  The element
// rules (ions, noble gases) are the shared writer's, not a copy of them.
string groToPdb(const string& gro)
{
  string pdb, err;
  return GromacsStructure::groToPdb(gro, pdb, err) > 0 ? pdb : "";
}

string kindOf(const string& dirName)
{
  static const char* kinds[] = { "prepare", "optimize", "equilibrate",
                                 "dynamics" };
  for (const char* k : kinds)
    if (dirName.find(k) != string::npos) return k;
  return "";
}

ResourceType* typeNamed(const string& name)
{
  vector<ResourceType*> types =
    ResourceDescriptor::getResourceDescriptor().getResourceTypes();
  for (size_t i = 0; i < types.size(); i++)
    if (types[i]->getName() == name) return types[i];
  return 0;
}

string shellArg(const string& s)
{
#ifdef _WIN32
  return Ecce::winArg(s);
#else
  string o = "'";
  for (char c : s) o += c == '\'' ? string("'\\''") : string(1, c);
  return o + "'";
#endif
}

int runProgram(const vector<string>& argv, string& out)
{
  string cmd;
  for (size_t i = 0; i < argv.size(); i++) cmd += (i ? " " : "") + shellArg(argv[i]);
#ifdef _WIN32
  return Ecce::readCommand(cmd, out, false);
#else
  return Ecce::readCommand(cmd + " 2>&1", out);
#endif
}

// A run imported without ecce-parse-md.txt gets one as a live run's job
// script makes it (gensub, gromacsMdAnalysis): mdanalysis on the .xtc and,
// when the .mdp saved velocities, the .trr.  The topology is the one gmx
// grompp -pp resolves (library includes, #ifdef) when gmx is found, else the
// run's own .top.  Any failure only leaves the panels out.
void analyseTask(const string& taskDir, const vector<string>& files,
                 const string& stage)
{
  string xtc, trr, top, mdp, gro, ndx;
  for (const string& f : files) {
    const string p = taskDir + "/" + f;
    if (hasSuffix(f, ".xtc") && xtc.empty()) xtc = p;
    else if (hasSuffix(f, ".trr") && trr.empty()) trr = p;
    else if ((hasSuffix(f, ".top") || hasSuffix(f, ".gmxtop")) && top.empty()) top = p;
    else if (hasSuffix(f, ".mdp") && f != "mdout.mdp" && mdp.empty()) mdp = p;
    else if (hasSuffix(f, ".gro") && f != "confout.gro" && f != "view.gro" && gro.empty()) gro = p;
    else if (hasSuffix(f, ".ndx") && ndx.empty()) ndx = p;
  }
  if (xtc.empty() || top.empty() || mdp.empty()) return;
  string out;
  const char* gmxEnv = getenv("ECCE_GROMACS");
  const string gmx = gmxEnv && *gmxEnv ? string(gmxEnv) : CodePrograms::findOnPath("GROMACS");
  if (!gmx.empty() && !gro.empty()) {
    vector<string> a = { gmx, "grompp", "-f", mdp, "-c", gro, "-r", gro, "-p", top,
                         "-pp", stage + "/ecce_pp.top", "-o", stage + "/ecce_pp.tpr",
                         "-po", stage + "/ecce_pp.mdp", "-maxwarn", "999" };
    if (!ndx.empty()) { a.push_back("-n"); a.push_back(ndx); }
    if (runProgram(a, out) == 0 && access((stage + "/ecce_pp.top").c_str(), R_OK) == 0)
      top = stage + "/ecce_pp.top";
  }
  const string md = MdAnalysisRun::run(MdAnalysisRun::program(), top, mdp, xtc, trr, stage);
  if (!md.empty())
    writeFile(stage + "/ecce-parse-md.txt", md);
  // The job monitor reads the staging directory: leave only what it parses.
  for (const char* f : { "ecce_pp.top", "ecce_pp.tpr", "ecce_pp.mdp",
                         "ecce_md_msd.txt", "ecce_md_speed.txt" })
    unlink((stage + "/" + f).c_str());
}

// Removes our own gmximport-* directories more than a day old.
void sweepOldStaging(const string& parent)
{
  DIR* d = opendir(parent.c_str());
  if (!d) return;
  const time_t limit = time(0) - 24 * 3600;
  vector<string> old;
  while (struct dirent* e = readdir(d)) {
    string n = e->d_name;
    if (n.compare(0, 10, "gmximport-") != 0) continue;
    struct stat st;
    if (lstat((parent + "/" + n).c_str(), &st) != 0) continue;
    if (S_ISDIR(st.st_mode) && st.st_uid == getuid() && st.st_mtime < limit)
      old.push_back(parent + "/" + n);
  }
  closedir(d);
  for (size_t i = 0; i < old.size(); i++) {
    std::error_code ec;
    ecce::fs::remove_all(old[i], ec);
  }
}

// Attaches a task's inputs, copies its outputs into a staging directory
// under the names a live run brings back, and imports that.  The job
// monitor reads the staging directory after this returns and removes it when
// it finishes; only a failed import (no monitor started) removes it here.
bool importTaskStaged(MdTask* md, const string& taskDir, bool prepare,
                      string& message, const string& stage);

bool importTask(MdTask* md, const string& taskDir, bool prepare,
                string& message)
{
  const char* tmp = getenv("ECCE_TMPDIR");
  const string parent = tmp && *tmp ? string(tmp) : string("/tmp");
  // A monitor that dies leaves its staging copy; this bounds the growth.
  sweepOldStaging(parent);
  string pattern = parent + "/gmximport-XXXXXX";
  vector<char> buf(pattern.begin(), pattern.end());
  buf.push_back('\0');
  if (!mkdtemp(&buf[0])) {
    message = "cannot make a staging directory";
    return false;
  }
  const string stage = &buf[0];
  if (importTaskStaged(md, taskDir, prepare, message, stage)) return true;
  std::error_code ec;
  ecce::fs::remove_all(stage, ec);
  return false;
}

bool importTaskStaged(MdTask* md, const string& taskDir, bool prepare,
                      string& message, const string& stage)
{
  bool haveLog = false;
  string fragment, view;
  vector<string> files = listDir(taskDir, false);
  for (size_t i = 0; i < files.size(); i++) {
    const string& f = files[i];
    string text;
    if (!readFile(taskDir + "/" + f, text)) continue;
    bool ok = true;
    if (hasSuffix(f, ".gmxlog") || hasSuffix(f, ".log")) {
      if (!haveLog) ok = writeFile(stage + "/gromacs.gmxlog", text);
      haveLog = true;
    } else if (f == "ecce-parse-traj.txt" || f == "ecce-parse-md.txt") {
      ok = writeFile(stage + "/" + f, text);
    } else if (f == "view.gro") {
      // The structure wrapped into a dodecahedron or octahedron box, for
      // display only; the restart file stays the run's own.
      view = text;
    } else if (prepare) {
      if (hasSuffix(f, ".gmxprep") || hasSuffix(f, ".recipe"))
        ok = putInput(md, "gromacs.gmxprep", text);
      else if (hasSuffix(f, ".pdb"))
        ok = putInput(md, stem(f) + ".gmxpdb", text);
      else if (hasSuffix(f, ".top") || hasSuffix(f, ".gmxtop"))
        ok = writeFile(stage + "/" + md->getTopologyName(), text);
      else if (hasSuffix(f, ".itp"))
        ok = writeFile(stage + "/" + f, text);
      else if (hasSuffix(f, ".gro")) {
        ok = writeFile(stage + "/" + md->getRestartName(), text);
        fragment = text;
      }
    } else {
      if (hasSuffix(f, ".mdp"))
        ok = putInput(md, "gromacs.mdp", text);
      else if (hasSuffix(f, ".top") || hasSuffix(f, ".gmxtop"))
        ok = putInput(md, stem(f) + ".gmxtop", text);
      else if (hasSuffix(f, ".itp") || hasSuffix(f, ".ndx"))
        ok = putInput(md, f, text);
      else if (f == "confout.gro") {
        ok = writeFile(stage + "/" + md->getRestartName(), text);
        fragment = text;
      }
      else if (hasSuffix(f, ".gro"))
        ok = putInput(md, stem(f) + ".gro", text);
    }
    if (!ok) {
      message = "could not store " + f;
      return false;
    }
  }
  if (!haveLog) {
    message = taskDir + " has no .log or .gmxlog file";
    return false;
  }
  if (!prepare &&
      std::find(files.begin(), files.end(), "ecce-parse-md.txt") == files.end())
    analyseTask(taskDir, files, stage);
  // A run without view.gro keeps its triclinic box as the brick mdrun wrote.
  if (!view.empty()) fragment = view;
  else if (!fragment.empty()) fragment = GromacsStructure::wrapCompact(fragment);
  if (!fragment.empty()) {
    string pdb = groToPdb(fragment);
    if (!pdb.empty()) writeFile(stage + "/" + md->getOutputFragmentName(), pdb);
  }
  return JobParser::importGromacsTask(md, stage, message);
}

}  // namespace

string importGromacsRun(Resource* parent, const string& studyName,
                        const string& runDir, string& message,
                        const std::function<void(const string&)>& beforeTask)
{
  ResourceType* studyType = typeNamed("gromacs_md_study");
  if (!parent || !studyType) {
    message = "no parent or no GROMACS study type";
    return "";
  }
  vector<string> tasks;
  vector<string> dirs = listDir(runDir, true);
  for (size_t i = 0; i < dirs.size(); i++)
    if (!kindOf(dirs[i]).empty()) tasks.push_back(dirs[i]);
  if (tasks.empty()) {
    message = runDir + " holds no prepare/optimize/equilibrate/dynamics "
              "subdirectory";
    return "";
  }

  Resource* study = parent->createChild(studyName, studyType);
  Session* session = dynamic_cast<Session*>(study);
  if (!session) {
    message = "could not create the study";
    return "";
  }
  for (size_t i = 0; i < tasks.size(); i++) {
    const string kind = kindOf(tasks[i]);
    ResourceType* tt = typeNamed("gromacs_md_" + kind);
    Resource* task = tt ? study->createChild(kind, tt) : 0;
    MdTask* md = dynamic_cast<MdTask*>(task);
    if (!md) {
      message = "could not create the " + kind + " task";
      return study->getURL().toString();
    }
    session->addMemberAsTarget(task, 0);
    if (beforeTask) beforeTask(kind);
    string why;
    if (!importTask(md, runDir + "/" + tasks[i], kind == "prepare", why)) {
      message = tasks[i] + ": " + why;
      return study->getURL().toString();
    }
  }
  return study->getURL().toString();
}

namespace {

string baseName(const string& path)
{
  return path.substr(path.find_last_of('/') + 1);
}

string dirName(const string& path)
{
  string::size_type s = path.find_last_of('/');
  return s == string::npos ? string(".") : path.substr(0, s);
}

}  // namespace

string createGromacsRun(Resource* parent, const string& mdp,
                        const string& structure, const string& top,
                        const string& tpr, string& message)
{
  message.clear();
  ResourceType* type = typeNamed("gromacs_md_dynamics");
  if (!parent || !type) {
    message = "no parent or no GROMACS task type";
    return "";
  }
  if (tpr.empty() && (mdp.empty() || structure.empty() || top.empty())) {
    message = "A run needs a .mdp, a starting structure and a topology, "
              "or a ready .tpr.";
    return "";
  }
  if (hasSuffix(structure, ".pdb")) {
    message = "A .pdb structure cannot be used as it is: convert it to a "
              ".gro file (gmx editconf -f in.pdb -o out.gro) first.";
    return "";
  }
  string mdpText, groText, topText, tprText;
  if (!mdp.empty() && !readFile(mdp, mdpText)) {
    message = "cannot read " + mdp;
    return "";
  }
  if (!tpr.empty() && !readFile(tpr, tprText)) {
    message = "cannot read " + tpr;
    return "";
  }
  if (tpr.empty()) {
    string pdb, err;
    if (!readFile(structure, groText) ||
        GromacsStructure::groToPdb(groText, pdb, err) < 0) {
      message = "not a usable .gro file: " + structure +
                (err.empty() ? string() : " (" + err + ")");
      return "";
    }
    if (!readFile(top, topText)) {
      message = "cannot read " + top;
      return "";
    }
  }

  string name = stem(baseName(!mdp.empty() ? mdp : tpr));
  for (size_t i = 0; i < name.size(); i++)
    if (!isalnum((unsigned char)name[i]) && name[i] != '_' && name[i] != '-')
      name[i] = '_';
  if (name.empty()) name = "GROMACS_Run";
  EDSI* edsi = EDSIFactory::getEDSI(parent->getURL());
  name = edsi->uniqueName(name, "_%d");
  delete edsi;

  Resource* task = parent->createChild(name, type);
  MdTask* md = dynamic_cast<MdTask*>(task);
  if (!md) {
    message = "could not create the GROMACS run";
    return "";
  }
  const string url = md->getURL().toString();
  bool ok = true;
  string note;
  if (!tpr.empty()) {
    ok = putInput(md, "run.gmxtpr", tprText);
    if (ok)
      ok = putInput(md, "gromacs.mdp",
                    !mdpText.empty() ? mdpText
                    : "; this run comes from run.gmxtpr, not from this file\n");
  } else {
    ok = putInput(md, "gromacs.mdp", mdpText) &&
         putInput(md, stem(baseName(top)) + ".gmxtop", topText) &&
         putInput(md, stem(baseName(structure)) + ".gro", groText);
  }
  if (!ok) {
    message = "could not store the files: " + md->messages();
    return url;
  }

  if (tpr.empty()) {
    std::istringstream gin(groText);
    Fragment frag;
    if (frag.restoreGRO(gin, true)) md->fragment(&frag);

    vector<GromacsIncludes::File> found;
    vector<string> system, missing;
    GromacsIncludes::collect(top, found, system, missing);
    int stored = 0;
    for (size_t i = 0; i < found.size(); i++) {
      string text;
      if (readFile(found[i].path, text) &&
          putInput(md, GromacsIncludes::encodeName(found[i].name), text))
        stored++;
      else
        missing.push_back(found[i].name);
    }
    // An index file beside the topology is the user's own groups.
    vector<string> beside = listDir(dirName(top), false);
    for (size_t i = 0; i < beside.size(); i++) {
      string text;
      if (hasSuffix(beside[i], ".ndx") &&
          readFile(dirName(top) + "/" + beside[i], text) &&
          putInput(md, beside[i], text))
        stored++;
    }
    note = "Attached " + std::to_string(stored) + " include/index file" +
           (stored == 1 ? "" : "s") + ".";
    if (!missing.empty()) {
      note += " Not found:";
      for (size_t i = 0; i < missing.size(); i++) note += " " + missing[i];
      note += ".";
    }
  }
  if (TaskJob* job = dynamic_cast<TaskJob*>(md))
    job->setState(ResourceDescriptor::STATE_READY);
  message = note;
  return url;
}
