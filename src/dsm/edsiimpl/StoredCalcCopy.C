#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

#include "dsm/StoredCalcCopy.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/VDoc.H"
#include "util/EcceURL.H"

using std::map;
using std::string;
using std::vector;

namespace {

// Bump when the stored layout or what is copied changes; it is part of the
// stamp, so older copies fall back to a parse.
const char *const kFormatId = "1";

// Properties that say who or where the original import ran, or when.  The
// copying session sets its own.
const char *const kSkip[] = {"owner", "creationdate", "createdWith",
  "currentVersion", "startdate", "completiondate", "job_path", "job_jobid",
  "job_clienthost", "job_clientid", "launch_user", "launch_machine",
  "launch_remoteShell", "launch_rundir", "launch_account", "launch_profile",
  "version"};

bool skipped(const string& name)
{
  const string ns = VDoc::getEcceNamespace() + ":";
  if (name.compare(0, ns.size(), ns) != 0) return false;
  const string tail = name.substr(ns.size());
  for (const char *s : kSkip)
    if (tail == s) return true;
  return false;
}

string unesc(const string& in)
{
  string out;
  for (size_t i = 0; i < in.size(); i++) {
    if (in[i] == '\\' && i + 1 < in.size()) {
      char c = in[++i];
      out += (c == 'n') ? '\n' : (c == 'r') ? '\r' : (c == 't') ? '\t' : c;
    } else {
      out += in[i];
    }
  }
  return out;
}

// "resource TAB name TAB type TAB value" per line, as FileEDSI writes it;
// the directory itself is ".".
typedef map<string, vector<MetaDataResult> > Records;

Records readSidecar(const string& dir)
{
  Records recs;
  std::ifstream in((dir + "/.ecce-meta").c_str());
  string line;
  while (std::getline(in, line)) {
    size_t a = line.find('\t');
    size_t b = a == string::npos ? a : line.find('\t', a + 1);
    size_t c = b == string::npos ? b : line.find('\t', b + 1);
    if (c == string::npos) continue;
    MetaDataResult m;
    m.name = unesc(line.substr(a + 1, b - a - 1));
    m.type = unesc(line.substr(b + 1, c - b - 1));
    m.value = unesc(line.substr(c + 1));
    if (!skipped(m.name)) recs[unesc(line.substr(0, a))].push_back(m);
  }
  return recs;
}

bool isDir(const string& p)
{
  struct stat st;
  return stat(p.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

vector<string> names(const string& dir)
{
  vector<string> out;
  if (DIR *d = opendir(dir.c_str())) {
    while (struct dirent *e = readdir(d)) {
      string n = e->d_name;
      if (n != "." && n != ".." && n.compare(0, 10, ".ecce-meta") != 0)
        out.push_back(n);
    }
    closedir(d);
  }
  std::sort(out.begin(), out.end());
  return out;
}

string prop(const vector<MetaDataResult>& v, const string& key)
{
  const string full = VDoc::getEcceNamespace() + ":" + key;
  for (const MetaDataResult& m : v)
    if (m.name == full) return m.value;
  return "";
}

// Copy what is below srcDir into the collection at url.
bool copyTree(const EcceURL& url, const string& srcDir, string& why)
{
  Records recs = readSidecar(srcDir);
  for (const string& name : names(srcDir)) {
    const string src = srcDir + "/" + name;
    EcceURL child = url.getChild(name);
    if (isDir(src)) {
      if (!EDSIFactory::getEDSI(child)->exists()) {
        EcceURL *made = EDSIFactory::getEDSI(url)->makeCollection(name);
        if (!made) { why = "cannot create " + name; return false; }
        child = *made;
        delete made;
      }
      Records self = readSidecar(src);
      if (!self["."].empty() &&
          !EDSIFactory::getEDSI(child)->putMetaData(self["."])) {
        why = "cannot write the properties of " + name;
        return false;
      }
      if (!copyTree(child, src, why)) return false;
    } else {
      std::ifstream ifs(src.c_str(), std::ios::binary);
      EDSI *edsi = EDSIFactory::getEDSI(child);
      if (!ifs || !edsi->putDataSet(ifs)) {
        why = "cannot write " + name;
        return false;
      }
      if (!recs[name].empty() && !edsi->putMetaData(recs[name])) {
        why = "cannot write the properties of " + name;
        return false;
      }
    }
  }
  return true;
}

}  // namespace

namespace StoredCalcCopy {

string stampOf(const string& file)
{
  std::ifstream in(file.c_str(), std::ios::binary);
  if (!in) return "";
  unsigned long long h = 1469598103934665603ULL, size = 0;
  char buf[65536];
  while (in.read(buf, sizeof buf) || in.gcount() > 0) {
    const std::streamsize n = in.gcount();
    for (std::streamsize i = 0; i < n; i++) {
      h ^= (unsigned char)buf[i];
      h *= 1099511628211ULL;
    }
    size += n;
  }
  std::ostringstream out;
  out << kFormatId << " " << std::hex << h << std::dec << " " << size;
  return out.str();
}

bool isCurrent(const string& dir, const string& outputFile)
{
  std::ifstream in((dir + "/stamp").c_str());
  string have;
  if (!std::getline(in, have)) return false;
  return have == stampOf(outputFile);
}

string calcName(const string& dir)
{
  string found;
  for (const string& n : names(dir))
    if (isDir(dir + "/" + n)) found = found.empty() ? n : "";
  return found;
}

bool copy(Resource *project, const string& srcDir, const string& calcName,
          string& why)
{
  Records top = readSidecar(srcDir);
  const vector<MetaDataResult>& self = top["."];
  if (self.empty()) { why = "no properties in " + srcDir; return false; }

  ResourceType *type = ResourceDescriptor::getResourceDescriptor().
      getResourceType(prop(self, "resourcetype"), prop(self, "contenttype"),
                      prop(self, "application"));
  if (!type) { why = "unknown calculation type"; return false; }
  Resource *calc = project->createChild(calcName, type);
  if (!calc) { why = "cannot create " + calcName; return false; }

  const bool ok = EDSIFactory::getEDSI(calc->getURL())->putMetaData(self) &&
                  copyTree(calc->getURL(), srcDir, why);
  if (!ok) {
    if (why.empty()) why = "cannot write the properties of " + calcName;
    calc->remove();
  }
  return ok;
}

}  // namespace StoredCalcCopy
