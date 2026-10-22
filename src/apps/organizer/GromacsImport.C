#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
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

// What a live run's gensub makes with `gmx editconf -o chemsys.pdb`: the
// task's output fragment, which the Builder and Viewer show.  .gro is nm.
string groToPdb(const string& gro)
{
  std::istringstream in(gro);
  string line;
  std::getline(in, line);
  std::getline(in, line);
  int n = atoi(line.c_str());
  string atoms;
  char buf[128];
  for (int i = 0; i < n && std::getline(in, line); i++) {
    if (line.size() < 44) return "";
    string res = line.substr(5, 5), name = line.substr(10, 5);
    res.erase(0, res.find_first_not_of(' '));
    name.erase(0, name.find_first_not_of(' '));
    res.erase(res.find_last_not_of(' ') + 1);
    name.erase(name.find_last_not_of(' ') + 1);
    double x = atof(line.substr(20, 8).c_str()) * 10;
    double y = atof(line.substr(28, 8).c_str()) * 10;
    double z = atof(line.substr(36, 8).c_str()) * 10;
    string el;
    for (size_t k = 0; k < name.size() && el.empty(); k++)
      if (isalpha((unsigned char)name[k])) el = name.substr(k, 1);
    if (res == name && (name == "NA" || name == "CL" || name == "ZN" ||
                        name == "MG" || name == "CA"))
      el = name;
    string pn = name.size() < 4 && el.size() == 1 ? " " + name : name;
    snprintf(buf, sizeof buf,
             "ATOM  %5d %-4s %3.3s  %4d    %8.3f%8.3f%8.3f  1.00  0.00"
             "          %2s\n",
             (i + 1) % 100000, pn.c_str(), res.c_str(),
             atoi(line.substr(0, 5).c_str()) % 10000, x, y, z, el.c_str());
    atoms += buf;
  }
  if (!std::getline(in, line)) return "";
  double v[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  std::istringstream bs(line);
  int nv = 0;
  while (nv < 9 && bs >> v[nv]) nv++;
  string out;
  if (nv >= 3) {
    // gro order: v1x v2y v3z v1y v1z v2x v2z v3x v3y
    double a[3] = { v[0], v[3], v[4] }, b[3] = { v[5], v[1], v[6] },
           c[3] = { v[7], v[8], v[2] };
    if (nv == 3) { a[1] = a[2] = b[0] = b[2] = c[0] = c[1] = 0; }
    double la = std::sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
    double lb = std::sqrt(b[0]*b[0] + b[1]*b[1] + b[2]*b[2]);
    double lc = std::sqrt(c[0]*c[0] + c[1]*c[1] + c[2]*c[2]);
    double ca = (b[0]*c[0] + b[1]*c[1] + b[2]*c[2]) / (lb * lc);
    double cb = (a[0]*c[0] + a[1]*c[1] + a[2]*c[2]) / (la * lc);
    double cg = (a[0]*b[0] + a[1]*b[1] + a[2]*b[2]) / (la * lb);
    const double deg = 180 / M_PI;
    snprintf(buf, sizeof buf, "CRYST1%9.3f%9.3f%9.3f%7.2f%7.2f%7.2f P 1\n",
             la * 10, lb * 10, lc * 10, std::acos(ca) * deg,
             std::acos(cb) * deg, std::acos(cg) * deg);
    out = buf;
  }
  return out + atoms + "END\n";
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
    } else if (f == "ecce-parse-traj.txt") {
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
  if (!view.empty()) fragment = view;
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
