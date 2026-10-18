#include "wx/wxprec.h"
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include "util/Ecce.H"
#include "wxgui/GromacsForceFieldLibrary.H"

namespace GromacsForceFieldLibrary
{

static const wxULongLong MAX_BYTES = 300ULL * 1024 * 1024;

static wxString W(const string& s) { return wxString::FromUTF8(s.c_str()); }
static string S(const wxString& w) { return string(w.ToUTF8()); }

static bool endsWith(const string& s, const string& t)
{
  return s.size() >= t.size() && s.compare(s.size() - t.size(), t.size(), t) == 0;
}

// The name goes into the recipe (FF=) and a shell word; md.gmxprep takes the same set.
static bool validName(const string& n)
{
  if (n.empty() || !isalnum((unsigned char)n[0])) return false;
  for (size_t i = 0; i < n.size(); i++) {
    unsigned char c = n[i];
    if (!(isalnum(c) || c == '_' || c == '.' || c == '+' || c == '-')) return false;
  }
  return true;
}

string directory()
{
  if (getenv("ECCE_REALUSERHOME") == 0) return "";
  return string(Ecce::realUserPrefPath()) + "forcefields";
}

static string firstLine(const string& file)
{
  std::ifstream in(file.c_str());
  string line;
  while (std::getline(in, line)) {
    size_t a = line.find_first_not_of(" \t\r\n");
    if (a == string::npos) continue;
    size_t b = line.find_last_not_of(" \t\r\n");
    line = line.substr(a, b - a + 1);
    if (line.size() > 60) line = line.substr(0, 60);
    return line;
  }
  return "";
}

static Entry entryAt(const string& dir, const string& folder)
{
  Entry e;
  e.name = folder.substr(0, folder.size() - 3);
  e.path = dir + "/" + folder;
  e.label = firstLine(e.path + "/forcefield.doc");
  if (e.label.empty()) e.label = e.name;
  return e;
}

vector<Entry> list()
{
  vector<Entry> out;
  string dir = directory();
  if (dir.empty() || !wxDir::Exists(W(dir))) return out;
  wxDir d(W(dir));
  wxString f;
  for (bool ok = d.GetFirst(&f, "*.ff", wxDIR_DIRS); ok; ok = d.GetNext(&f))
    if (validName(S(f).substr(0, S(f).size() - 3)))
      out.push_back(entryAt(dir, S(f)));
  std::sort(out.begin(), out.end(),
            [](const Entry& a, const Entry& b) { return a.name < b.name; });
  return out;
}

bool find(const string& name, Entry& entry)
{
  vector<Entry> all = list();
  for (size_t i = 0; i < all.size(); i++)
    if (all[i].name == name) { entry = all[i]; return true; }
  return false;
}

namespace {
struct Collect : public wxDirTraverser
{
  Collect(const wxString& r) : root(r) {}
  virtual wxDirTraverseResult OnFile(const wxString& f)
  {
    wxFileName n(f);
    n.MakeRelativeTo(root);
    string rel = S(n.GetFullPath(wxPATH_UNIX));
    names.push_back(rel);
    return wxDIR_CONTINUE;
  }
  virtual wxDirTraverseResult OnDir(const wxString&) { return wxDIR_CONTINUE; }
  wxString root;
  vector<string> names;
};
struct Scratch
{
  wxString path;
  ~Scratch() { if (!path.empty()) wxFileName::Rmdir(path, wxPATH_RMDIR_RECURSIVE); }
};
}

vector<string> files(const Entry& entry)
{
  Collect c(W(entry.path));
  wxDir d(W(entry.path));
  if (d.IsOpened()) d.Traverse(c);
  std::sort(c.names.begin(), c.names.end());
  return c.names;
}

vector<string> waterModels(const Entry& entry)
{
  vector<string> out;
  std::ifstream in((entry.path + "/watermodels.dat").c_str());
  string line;
  while (std::getline(in, line)) {
    std::istringstream is(line);
    string id;
    if (is >> id && id[0] != ';' && id[0] != '#' && id != "none") out.push_back(id);
  }
  return out;
}

static bool badPath(const string& rel)
{
  if (rel.empty() || rel[0] == '/' || rel[0] == '\\') return true;
  if (rel.size() > 1 && rel[1] == ':') return true;
  if (rel.find('\\') != string::npos) return true;
  size_t p = 0;
  while (p <= rel.size()) {
    size_t q = rel.find('/', p);
    if (q == string::npos) q = rel.size();
    if (rel.substr(p, q - p) == "..") return true;
    p = q + 1;
  }
  return false;
}

static bool copyTree(const wxString& from, const wxString& to, string& message)
{
  wxULongLong total = 0;
  Collect c(from);
  wxDir d(from);
  if (!d.IsOpened()) { message = "The folder cannot be read."; return false; }
  d.Traverse(c);
  for (size_t i = 0; i < c.names.size(); i++) {
    wxFileName src(from + "/" + W(c.names[i]));
    wxFileName dst(to + "/" + W(c.names[i]));
    total += src.GetSize() == wxInvalidSize ? wxULongLong(0) : src.GetSize();
    if (total > MAX_BYTES) { message = "The folder is larger than 300 MB."; return false; }
    if (!wxFileName::Mkdir(dst.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL) ||
        !wxCopyFile(src.GetFullPath(), dst.GetFullPath())) {
      message = "Could not copy " + c.names[i] + ".";
      return false;
    }
  }
  return true;
}

// wxZipEntry strips a leading slash from names, so the central directory is
// read here to see them as stored.  False with a message on a bad one.
static bool checkZipNames(const wxString& zip, string& message)
{
  std::ifstream in(S(zip).c_str(), std::ios::binary);
  std::ostringstream os;
  os << in.rdbuf();
  const string z = os.str();
  size_t eocd = z.rfind("PK\x05\x06");
  if (eocd == string::npos || eocd + 22 > z.size()) return true;   // wx reports it
  auto u16 = [&](size_t o) { return (size_t)((unsigned char)z[o] | ((unsigned char)z[o + 1] << 8)); };
  auto u32 = [&](size_t o) { return u16(o) | (u16(o + 2) << 16); };
  size_t n = u16(eocd + 10), at = u32(eocd + 16);
  for (size_t i = 0; i < n && at + 46 <= z.size() && z.compare(at, 4, "PK\x01\x02") == 0; i++) {
    size_t nl = u16(at + 28), xl = u16(at + 30), cl = u16(at + 32);
    if (at + 46 + nl > z.size()) break;
    string name = z.substr(at + 46, nl);
    if (badPath(name)) {
      message = "The zip holds an entry with an absolute path or \"..\" (" +
                name + "); nothing was imported.";
      return false;
    }
    at += 46 + nl + xl + cl;
  }
  return true;
}

// Unpacks the one <name>.ff folder of a zip into scratch; folder is its name.
static bool unpack(const wxString& zip, const wxString& scratch, string& folder,
                   string& message)
{
  if (!checkZipNames(zip, message)) return false;
  wxFFileInputStream fin(zip);
  if (!fin.IsOk()) { message = "The zip file cannot be read."; return false; }
  wxZipInputStream zin(fin);
  vector<std::pair<string, bool> > seen;   // internal name, is a directory
  std::unique_ptr<wxZipEntry> entry;
  wxULongLong total = 0;
  while (entry.reset(zin.GetNextEntry()), entry.get() != 0) {
    string name = S(entry->GetInternalName());
    if (badPath(name)) {
      message = "The zip holds an entry with an absolute path or \"..\" (" +
                name + "); nothing was imported.";
      return false;
    }
    if (name.compare(0, 9, "__MACOSX/") == 0) continue;
    string top = name.substr(0, name.find('/'));
    if (!endsWith(top, ".ff")) continue;
    if (folder.empty()) folder = top;
    if (top != folder) {
      message = "The zip holds more than one .ff folder (" + folder + " and " +
                top + ").";
      return false;
    }
    if (entry->IsDir() || name == top + "/") continue;
    wxFileName out(scratch + "/" + W(name));
    if (!wxFileName::Mkdir(out.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
      message = "Could not create " + name + ".";
      return false;
    }
    wxFFileOutputStream os(out.GetFullPath());
    if (!os.IsOk()) { message = "Could not write " + name + "."; return false; }
    os.Write(zin);
    os.Close();
    total += out.GetSize() == wxInvalidSize ? wxULongLong(0) : out.GetSize();
    if (total > MAX_BYTES) { message = "The zip unpacks to more than 300 MB."; return false; }
  }
  if (folder.empty()) {
    message = "The zip holds no <name>.ff folder.";
    return false;
  }
  return true;
}

Result import(const string& source, bool replace, const vector<string>& builtIn,
              string& name, string& message)
{
  string lib = directory();
  if (lib.empty()) { message = "There is no ECCE user directory."; return REFUSED; }
  if (!wxFileName::Mkdir(W(lib), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
    message = "Could not create " + lib + ".";
    return REFUSED;
  }
  Scratch scratch;
  scratch.path = W(lib + "/.import-" + std::to_string((long)wxGetProcessId()));
  wxFileName::Rmdir(scratch.path, wxPATH_RMDIR_RECURSIVE);
  if (!wxMkdir(scratch.path)) { message = "Could not create a scratch folder."; return REFUSED; }

  string folder;
  bool isZip = endsWith(source, ".zip") || endsWith(source, ".ZIP");
  if (isZip) {
    if (!unpack(W(source), scratch.path, folder, message)) return REFUSED;
  } else {
    string s = source;
    while (s.size() > 1 && (s.back() == '/' || s.back() == '\\')) s.erase(s.size() - 1);
    wxFileName fn = wxFileName::DirName(W(s));
    folder = S(fn.GetDirs().IsEmpty() ? wxString() : fn.GetDirs().Last());
    if (!wxDir::Exists(W(s))) { message = "That is not a folder."; return REFUSED; }
    if (!endsWith(folder, ".ff")) {
      message = "The folder name must end in .ff (it is " + folder + ").";
      return REFUSED;
    }
    if (!copyTree(W(s), scratch.path + "/" + W(folder), message)) return REFUSED;
  }
  name = folder.substr(0, folder.size() - 3);
  if (!validName(name)) {
    message = "The force field name may only hold letters, digits and _ . + - "
              "(and start with a letter or digit).";
    return REFUSED;
  }
  string staged = S(scratch.path) + "/" + folder;
  if (!wxFileName::FileExists(W(staged + "/forcefield.itp"))) {
    message = folder + " has no forcefield.itp, so it is not a GROMACS force field.";
    return REFUSED;
  }
  wxDir sd(W(staged));
  Collect c(W(staged));
  sd.Traverse(c);
  bool rtp = false;
  for (size_t i = 0; i < c.names.size(); i++) if (endsWith(c.names[i], ".rtp")) rtp = true;
  if (!rtp) {
    message = folder + " has no .rtp residue file, which pdb2gmx needs. It can "
              "only be used through an existing topology (attach one on the "
              "Inputs page of the tasks), not by Prepare.";
    return REFUSED;
  }

  bool clash = std::find(builtIn.begin(), builtIn.end(), name) != builtIn.end();
  Entry old;
  if (find(name, old)) clash = true;
  if (clash && !replace) {
    message = "A force field named " + name + " already exists.";
    return CLASH;
  }
  string dest = lib + "/" + folder;
  if (wxDir::Exists(W(dest)) && !wxFileName::Rmdir(W(dest), wxPATH_RMDIR_RECURSIVE)) {
    message = "Could not replace " + dest + ".";
    return REFUSED;
  }
  if (!wxRenameFile(W(staged), W(dest))) {
    message = "Could not store the force field in " + lib + ".";
    return REFUSED;
  }
  return OK;
}

}
