#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#include "GromacsImport.H"

#include "comm/JobParser.H"
#include "dsm/EDSIFactory.H"
#include "dsm/MdTask.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "util/Ecce.H"

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

// Attaches a task's inputs, copies its outputs into a staging directory
// under the names a live run brings back, and imports that.  The job
// monitor reads the staging directory after this returns, so it is left.
bool importTask(MdTask* md, const string& taskDir, bool prepare,
                string& message)
{
  const char* tmp = getenv("ECCE_TMPDIR");
  string pattern = (tmp && *tmp ? string(tmp) : string("/tmp")) +
                   "/gmximport-XXXXXX";
  vector<char> buf(pattern.begin(), pattern.end());
  buf.push_back('\0');
  if (!mkdtemp(&buf[0])) {
    message = "cannot make a staging directory";
    return false;
  }
  const string stage = &buf[0];

  bool haveLog = false;
  vector<string> files = listDir(taskDir, false);
  for (size_t i = 0; i < files.size(); i++) {
    const string& f = files[i];
    string text;
    if (!readFile(taskDir + "/" + f, text)) continue;
    bool ok = true;
    if (hasSuffix(f, ".gmxlog") || hasSuffix(f, ".log")) {
      if (!haveLog) ok = writeFile(stage + "/gromacs.gmxlog", text);
      haveLog = true;
    } else if (f == "ecce-parse-traj.txt") {
      ok = writeFile(stage + "/" + f, text);
    } else if (prepare) {
      if (hasSuffix(f, ".gmxprep") || hasSuffix(f, ".recipe"))
        ok = putInput(md, "gromacs.gmxprep", text);
      else if (hasSuffix(f, ".pdb"))
        ok = putInput(md, stem(f) + ".gmxpdb", text);
      else if (hasSuffix(f, ".top") || hasSuffix(f, ".gmxtop"))
        ok = writeFile(stage + "/" + md->getTopologyName(), text);
      else if (hasSuffix(f, ".itp"))
        ok = writeFile(stage + "/" + f, text);
      else if (hasSuffix(f, ".gro"))
        ok = writeFile(stage + "/" + md->getRestartName(), text);
    } else {
      if (hasSuffix(f, ".mdp"))
        ok = putInput(md, "gromacs.mdp", text);
      else if (hasSuffix(f, ".top") || hasSuffix(f, ".gmxtop"))
        ok = putInput(md, stem(f) + ".gmxtop", text);
      else if (hasSuffix(f, ".itp") || hasSuffix(f, ".ndx"))
        ok = putInput(md, f, text);
      else if (f == "confout.gro")
        ok = writeFile(stage + "/" + md->getRestartName(), text);
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
  return JobParser::importGromacsTask(md, stage, message);
}

}  // namespace

string importGromacsRun(Resource* parent, const string& studyName,
                        const string& runDir, string& message)
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
    string why;
    if (!importTask(md, runDir + "/" + tasks[i], kind == "prepare", why)) {
      message = tasks[i] + ": " + why;
      return study->getURL().toString();
    }
  }
  return study->getURL().toString();
}
