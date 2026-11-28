#include "dock/DockPrep.H"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <io.h>
#include <process.h>
#define popen _popen
#define pclose _pclose
#define getpid _getpid
#define ECCE_PATHSEP ';'
#define ECCE_DIRSEP "\\"
#else
#include <unistd.h>
#define ECCE_PATHSEP ':'
#define ECCE_DIRSEP "/"
#endif

namespace {

bool isExecutable(const std::string& p)
{
  if (p.empty()) return false;
#ifdef _WIN32
  return _access(p.c_str(), 0) == 0;
#else
  return access(p.c_str(), X_OK) == 0;
#endif
}

std::string quote(const std::string& s)
{
#ifdef _WIN32
  return "\"" + s + "\"";
#else
  std::string q = "'";
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\'') q += "'\\''"; else q += s[i];
  }
  return q + "'";
#endif
}

// Runs a command line through the shell; returns its exit status and output.
int run(const std::string& cmd, std::string& out)
{
#ifdef _WIN32
  // cmd.exe strips the outer quotes of a command that starts with one.
  FILE* p = popen(("\"" + cmd + " 2>&1\"").c_str(), "r");
#else
  FILE* p = popen((cmd + " 2>&1").c_str(), "r");
#endif
  if (!p) return -1;
  char buf[4096];
  size_t n;
  out.clear();
  while ((n = fread(buf, 1, sizeof buf, p)) > 0) out.append(buf, n);
  return pclose(p);
}

std::string lower(const std::string& s)
{
  std::string r = s;
  for (size_t i = 0; i < r.size(); i++) r[i] = (char)tolower((unsigned char)r[i]);
  return r;
}

std::string tempName(const char* suffix)
{
  static int n = 0;
  const char* d = getenv("TMPDIR");
  if (!d) d = getenv("TEMP");
  if (!d) d = getenv("TMP");
  std::ostringstream o;
  o << (d ? d : "/tmp") << ECCE_DIRSEP << "ecce-obabel-" << (long)getpid()
    << "-" << ++n << suffix;
  return o.str();
}

bool allAtOrigin(const DockMol& m)
{
  for (size_t i = 0; i < m.atoms.size(); i++)
    if (m.atoms[i].x != 0.0 || m.atoms[i].y != 0.0 || m.atoms[i].zc != 0.0)
      return false;
  return true;
}

}

std::string DockObabel::find()
{
  const char* env = getenv("ECCE_OBABEL");
  if (env && *env) return isExecutable(env) ? std::string(env) : std::string();
  const char* path = getenv("PATH");
  if (!path) return "";
  std::istringstream ss((std::string(path)));
  std::string dir;
  while (std::getline(ss, dir, ECCE_PATHSEP)) {
    if (dir.empty()) continue;
#ifdef _WIN32
    std::string c = dir + "\\obabel.exe";
#else
    std::string c = dir + "/obabel";
#endif
    if (isExecutable(c)) return c;
  }
  return "";
}

const std::vector<std::string>& DockObabel::readFormats()
{
  static std::vector<std::string> fmts;
  static bool done = false;
  if (done) return fmts;
  done = true;
  std::string ob = find(), out;
  if (ob.empty()) return fmts;
  if (run(quote(ob) + " -L formats read", out) != 0) return fmts;
  std::istringstream ss(out);
  std::string line;
  while (std::getline(ss, line)) {
    // "smi -- SMILES format"
    size_t p = line.find(" -- ");
    if (p == std::string::npos || p == 0) continue;
    fmts.push_back(lower(line.substr(0, p)));
  }
  return fmts;
}

bool DockObabel::canRead(const std::string& ext)
{
  const std::vector<std::string>& f = readFormats();
  std::string e = lower(ext);
  for (size_t i = 0; i < f.size(); i++) if (f[i] == e) return true;
  return false;
}

bool DockObabel::convertToSdf(const std::string& in, const std::string& type,
                              const std::string& outSdf, bool gen3d,
                              std::string& err)
{
  std::string ob = find();
  if (ob.empty()) { err = "Open Babel (obabel) was not found"; return false; }
  std::string cmd = quote(ob) + " -i" + lower(type) + " " + quote(in) +
                    " -osdf -O " + quote(outSdf) + " -l 1";
  if (gen3d) cmd += " --gen3d";
  std::string out;
  int rc = run(cmd, out);
  std::ifstream test(outSdf.c_str());
  if (rc != 0 || !test) {
    err = "obabel could not convert " + in + ": " + out;
    return false;
  }
  return true;
}

bool DockReader::readStructure(const std::string& path, const std::string& type,
                               DockMol& mol, std::string& err)
{
  std::string t = lower(type);
  if (t == "sdf" || t == "mol" || t == "sd") {
    if (readSdf(path, mol, err)) return true;
    if (err.find("V3000") == std::string::npos || DockObabel::find().empty())
      return false;
    t = "mol";          // V3000: only the bridge reads it
  } else if (t == "mol2") {
    return readMol2(path, mol, err);
  } else if (t == "pdbqt") {
    return readPdbqt(path, mol, err);
  } else if (!DockObabel::canRead(t)) {
    err = DockObabel::find().empty()
      ? "no reader for ." + t + " (more formats are read when Open Babel is installed)"
      : "Open Babel does not read ." + t;
    return false;
  }

  // SMILES and other 0D input comes out with every atom at the origin;
  // Open Babel builds a 3D structure (and adds the hydrogens) on request.
  for (int pass = 0; pass < 2; pass++) {
    std::string sdf = tempName(".sdf");
    bool ok = DockObabel::convertToSdf(path, t, sdf, pass == 1, err) &&
              readSdf(sdf, mol, err);
    std::remove(sdf.c_str());
    if (!ok) return false;
    if (!allAtOrigin(mol)) return true;
  }
  err = "Open Babel gave no coordinates for " + path;
  return false;
}
