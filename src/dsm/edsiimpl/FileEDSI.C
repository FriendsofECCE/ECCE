#include <iostream>
  using std::ends;

#include <fstream>
  using std::ifstream;
  using std::ofstream;

#include <strstream>
  using std::ostrstream;
  using std::strstreambuf;

#include <stdio.h> // sprintf
#include <string.h>

#include "util/SDirectory.H"
#include "util/ErrMsg.H"
#include "util/ProgressEvent.H"
#include "util/TDateTime.H"
#include "util/AuthEvent.H"
#include "util/Ecce.H"

#include "dsm/FileEDSI.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/VDoc.H"
#include "util/ResourceUtils.H"
#include "util/STLUtil.H"
#include "util/LocalData.H"


FileEDSI::FileEDSI() : EDSI()
{
  p_authListener = 0;
  p_progressListener = 0;
}

FileEDSI::FileEDSI(const EcceURL& url) : EDSI(url)
{
  p_authListener = 0;
  p_progressListener = 0;

  // GDB 1/21/13  Not sure what happened that caused this logic to be needed
  // now while it seemed to be working fine before....But, there were
  // definitely file paths with $ECCE_HOME in regards to at least the
  // structure libraray (DNA builder and peptide builder) so expand the
  // path to the file name before attempting to open/read the file.
  SFile file(p_url);
  p_url = file.path(true);
  // Marks the local data folder as in use for this process's lifetime,
  // so it is never moved under a job being followed (LocalData::move).
  LocalData::dir();
}

FileEDSI::FileEDSI(const FileEDSI& rhs) : EDSI(rhs)
{
  p_authListener = rhs.p_authListener;
  p_progressListener = rhs.p_progressListener;
}

FileEDSI::~FileEDSI()
{
}

/**
 * Checks to see if server is operating and suitable for EDSI.
 *
 * For a File EDSI object, this currently means the following:
 * <li>protocol is file or nothing
 * <li>server and port are empty.
 * <li>file can be stat'd.
 */
bool FileEDSI::checkServer()
{
  m_msgStack.clear();
  bool ret = true;
  string protocol = p_url.getProtocol();
  if (protocol != "file" && !protocol.empty()) {
    ret = false;
  } else if (!p_url.getHost().empty() || p_url.getPort() != -1) {
    ret = false;
  } else {
    SFile file(p_url.getFile().c_str());
    if (!file.exists()) {
      ret = false;
    }
  }
  if (!ret)  m_msgStack.add("SERVER_NOT_FOUND",p_url.toString().c_str());
  return ret;
}

// An empty list, as DavEDSI returns: a request for no names is a request
// for every property (PROPFIND allprop), so callers see the stored ones
// too, whatever their namespace.
bool FileEDSI::describeServerMetaData(vector<string>& metadata)
{
  metadata.clear();
  return true;
}

// Listener stuff for getting passwords.  We currently don't have code to
// invoke the listener for this implementation.
void FileEDSI::addAuthEventListener(AuthEventListener *l)
{
  p_authListener = l;
}

void FileEDSI::removeAuthEventListener(AuthEventListener *l)
{
  if (p_authListener == l) p_authListener = 0;
}

void FileEDSI::addProgressEventListener(ProgressEventListener *l)
{
  p_progressListener = l;
}

void FileEDSI::removeProgressEventListener(ProgressEventListener *l)
{
  if (p_progressListener == l) p_progressListener = 0;
}


#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sstream>
#include <memory>
#include <map>

namespace {

// One sidecar per directory holds the properties of its children, and of
// the directory itself under ".": a listing reads one file instead of one
// per child, and a directory copied or removed takes its subtree's
// properties along with it.
const char *SIDECAR = ".ecce-meta";

typedef std::map<string, MetaDataResult> PropMap;   // by property name
typedef std::map<string, PropMap> MetaStore;        // by resource name

// A record is one line, "resource TAB name TAB type TAB value", with
// backslash escapes so values can hold newlines and XML.
string esc(const string& in)
{
  string out;
  for (size_t i = 0; i < in.size(); i++) {
    switch (in[i]) {
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += in[i];
    }
  }
  return out;
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

bool isDirectory(const string& path)
{
  struct stat st;
  return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

string trimSlash(const string& path)
{
  string p = path;
  while (p.size() > 1 && p[p.size()-1] == '/') p.erase(p.size()-1);
  return p;
}

string parentOf(const string& path)
{
  string p = trimSlash(path);
  size_t pos = p.rfind('/');
  if (pos == string::npos) return ".";
  return pos == 0 ? "/" : p.substr(0, pos);
}

string tailOf(const string& path)
{
  string p = trimSlash(path);
  size_t pos = p.rfind('/');
  return pos == string::npos ? p : p.substr(pos+1);
}

// Which sidecar, and which record inside it, describes this path.
void locate(const string& path, string& dir, string& key)
{
  if (isDirectory(path)) {
    dir = trimSlash(path);
    key = ".";
  } else {
    dir = parentOf(path);
    key = tailOf(path);
  }
}

string sidecarPath(const string& dir) { return dir + "/" + SIDECAR; }

// Exclusive lock on a directory's sidecar and on appends to files in it,
// held across a whole read-modify-write.  flock() on a lock file next to
// the sidecar: POSIX only.  A Windows port replaces this one class with
// LockFileEx on the same file and nothing else changes.
class DirLock {
  public:
    explicit DirLock(const string& dir) : p_fd(-1)
    {
      string path = dir + "/.ecce-meta.lock";
      p_fd = open(path.c_str(), O_RDWR | O_CREAT, 0644);
      if (p_fd >= 0) {
        while (flock(p_fd, LOCK_EX) != 0 && errno == EINTR) {}
      }
    }
    ~DirLock() { if (p_fd >= 0) close(p_fd); }   // closing drops the lock
  private:
    int p_fd;
    DirLock(const DirLock&);
    DirLock& operator=(const DirLock&);
};

MetaStore loadStore(const string& dir)
{
  MetaStore store;
  ifstream in(sidecarPath(dir).c_str());
  string line;
  while (std::getline(in, line)) {
    size_t a = line.find('\t');
    size_t b = (a == string::npos) ? a : line.find('\t', a+1);
    size_t c = (b == string::npos) ? b : line.find('\t', b+1);
    if (c == string::npos) continue;
    MetaDataResult m;
    m.name = unesc(line.substr(a+1, b-a-1));
    m.type = unesc(line.substr(b+1, c-b-1));
    m.value = unesc(line.substr(c+1));
    store[unesc(line.substr(0, a))][m.name] = m;
  }
  return store;
}

// Written to a temp name and renamed, so a reader never sees half a file.
bool saveStore(const string& dir, const MetaStore& store)
{
  string path = sidecarPath(dir);
  bool any = false;
  for (MetaStore::const_iterator r = store.begin(); r != store.end(); ++r)
    if (!r->second.empty()) any = true;
  if (!any) {
    unlink(path.c_str());
    return true;
  }
  char pid[32];
  sprintf(pid, ".%d", (int)getpid());
  string tmp = path + pid;
  {
    ofstream out(tmp.c_str(), std::ios::binary);
    if (!out) return false;
    for (MetaStore::const_iterator r = store.begin(); r != store.end(); ++r) {
      for (PropMap::const_iterator p = r->second.begin();
           p != r->second.end(); ++p) {
        out << esc(r->first) << '\t' << esc(p->second.name) << '\t'
            << esc(p->second.type) << '\t' << esc(p->second.value) << '\n';
      }
    }
    out.close();
    if (!out) { unlink(tmp.c_str()); return false; }
  }
  if (rename(tmp.c_str(), path.c_str()) != 0) {
    unlink(tmp.c_str());
    return false;
  }
  return true;
}

// A value goes into a PROPPATCH as XML content, so the server keeps what
// an XML parser makes of it: callers (VDoc's rdf:Bag lists) wrap values in
// CDATA, which the parse removes and a sidecar must remove too.
string xmlContent(const string& value)
{
  const string open = "<![CDATA[", close = "]]>";
  size_t b = value.find_first_not_of(" \t\r\n");
  size_t e = value.find_last_not_of(" \t\r\n");
  if (b == string::npos || value.compare(b, open.size(), open) != 0 ||
      e + 1 < b + open.size() + close.size() ||
      value.compare(e + 1 - close.size(), close.size(), close) != 0)
    return value;
  string inner = value.substr(b + open.size(),
                              e + 1 - close.size() - b - open.size());
  if (inner.find(close) != string::npos) return value;
  return inner;
}

// Stored values replace the built-in one of the same name.
void mergeStored(vector<MetaDataResult>& list, const PropMap& stored)
{
  for (PropMap::const_iterator p = stored.begin(); p != stored.end(); ++p) {
    bool found = false;
    for (size_t i = 0; i < list.size(); i++) {
      if (list[i].name == p->first) {
        list[i] = p->second;
        found = true;
        break;
      }
    }
    if (!found) list.push_back(p->second);
  }
}

// Lock two directories in name order, so two operations cannot wait on
// each other.
class TwoDirLock {
  public:
    TwoDirLock(const string& a, const string& b) : p_first(a < b ? a : b)
    {
      if (a != b) p_second.reset(new DirLock(a < b ? b : a));
    }
  private:
    DirLock p_first;
    std::unique_ptr<DirLock> p_second;
};

// Carry a file's record to the sidecar of its new name, replacing whatever
// record that name had.  A directory's own sidecar is inside it and
// travels with it, so there is nothing to do.  The caller holds the locks
// of both directories.
bool transferFileProps(const string& from, const string& to, bool keepSource)
{
  if (isDirectory(to)) return true;
  string fdir = parentOf(from), tdir = parentOf(to);
  string fkey = tailOf(from), tkey = tailOf(to);
  MetaStore fs = loadStore(fdir);
  MetaStore::iterator it = fs.find(fkey);
  PropMap props;
  bool had = (it != fs.end());
  if (had) props = it->second;
  if (fdir == tdir) {
    if (!keepSource) fs.erase(fkey);
    if (had) fs[tkey] = props;
    else fs.erase(tkey);
    return saveStore(fdir, fs);
  }
  MetaStore ts = loadStore(tdir);
  if (had) ts[tkey] = props;
  else ts.erase(tkey);
  bool ok = saveStore(tdir, ts);
  if (!keepSource && had) {
    fs.erase(fkey);
    ok = saveStore(fdir, fs) && ok;
  }
  return ok;
}

void dropFileProps(const string& path)
{
  if (isDirectory(path)) return;       // its sidecar goes with the directory
  string dir = parentOf(path);
  DirLock lock(dir);
  MetaStore s = loadStore(dir);
  if (s.erase(tailOf(path))) saveStore(dir, s);
}

// MIME type of a file name from data/client/config/mimetypes, the table
// Apache's AddType lines mirror.  An extension not in it keeps the
// extension itself as its type (SegFactory selects on "sgm"/"frg").
const string& mimeTypeFor(const string& name, const string& fallback)
{
  static std::map<string, string> table;
  static bool loaded = false;
  if (!loaded) {
    loaded = true;
    string path = string(Ecce::ecceDataPath()) + "/client/config/mimetypes";
    ifstream in(path.c_str());
    string line;
    while (std::getline(in, line)) {
      if (line.empty() || line[0] == '#') continue;
      std::istringstream ls(line);
      string type, ext;
      ls >> type;
      while (ls >> ext) {
        for (size_t i = 0; i < ext.size(); i++) ext[i] = tolower(ext[i]);
        table[ext] = type;
      }
    }
  }
  static string result;
  size_t dot = name.rfind('.');
  string ext = dot == string::npos ? string() : name.substr(dot);
  for (size_t i = 0; i < ext.size(); i++) ext[i] = tolower(ext[i]);
  std::map<string, string>::const_iterator it = table.find(ext);
  result = (it == table.end()) ? fallback : it->second;
  return result;
}

bool readAll(const string& path, string& out)
{
  ifstream in(path.c_str(), std::ios::binary);
  if (!in) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  out = ss.str();
  return true;
}

// Truncate bytesToOverwrite off the end, then append: the contract of the
// Content-Range PUT that the property writers were built on.
bool appendBytes(const string& path, const string& data, int bytesToOverwrite)
{
  DirLock lock(parentOf(path));
  struct stat st;
  off_t size = (stat(path.c_str(), &st) == 0) ? st.st_size : 0;
  if (bytesToOverwrite > 0 && size > 0) {
    off_t keep = size - bytesToOverwrite;
    if (keep < 0) keep = 0;
    if (truncate(path.c_str(), keep) != 0) return false;
  }
  ofstream out(path.c_str(), std::ios::binary | std::ios::app);
  if (!out) return false;
  out << data;
  out.close();
  return !!out;
}

bool isStoreFile(const string& name)
{
  return name.compare(0, strlen(SIDECAR), SIDECAR) == 0;
}

// A plain file copy that refuses to replace an existing target.
bool copyFile(const string& from, const string& to)
{
  ifstream in(from.c_str(), std::ios::binary);
  if (!in) return false;
  int fd = open(to.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
  if (fd < 0) return false;
  close(fd);
  ofstream out(to.c_str(), std::ios::binary | std::ios::trunc);
  char buf[65536];
  while (in.read(buf, sizeof(buf)) || in.gcount() > 0)
    out.write(buf, in.gcount());
  out.close();
  if (!out) { unlink(to.c_str()); return false; }
  return true;
}

// SDirectory::copy flattens nested files, so recurse by hand.  The
// sidecar is copied (it is what carries the properties); its lock is not.
bool copyTree(const string& from, const string& to)
{
  if (!isDirectory(from)) return copyFile(from, to);
  if (mkdir(to.c_str(), 0755) != 0) return false;
  SDirectory dir(from);
  vector<SFile> kids = dir.get_files(false);
  for (size_t i = 0; i < kids.size(); i++) {
    string name = kids[i].filename();
    if (isStoreFile(name) && name != SIDECAR) continue;
    if (!copyTree(kids[i].path(), to + "/" + name)) return false;
  }
  return true;
}

bool pathExists(const string& path)
{
  struct stat st;
  return lstat(path.c_str(), &st) == 0;
}

// The name DavEDSI::uniqueName would give: name, then base-1.ext, ...
string uniqueIn(const string& dir, const string& name, const string& pattern)
{
  if (!pathExists(dir + "/" + name)) return name;
  string base = name, ext;
  size_t dot = name.rfind('.');
  if (dot != string::npos && dot > 0) {
    base = name.substr(0, dot);
    ext = name.substr(dot);
  }
  string format = "%s" + pattern + "%s";
  for (int i = 1; i < 10000; i++) {
    char buf[1024];
    snprintf(buf, sizeof(buf), format.c_str(), base.c_str(), i, ext.c_str());
    if (!pathExists(dir + "/" + buf)) return buf;
  }
  return name;
}

// rename() that never replaces an existing target, where the kernel and
// file system can promise it; elsewhere a check just before.
bool renameNoReplace(const string& from, const string& to)
{
#ifdef RENAME_NOREPLACE
  if (renameat2(AT_FDCWD, from.c_str(), AT_FDCWD, to.c_str(),
                RENAME_NOREPLACE) == 0) return true;
  if (errno != EINVAL && errno != ENOSYS) return false;
#endif
  if (pathExists(to)) { errno = EEXIST; return false; }
  return rename(from.c_str(), to.c_str()) == 0;
}

// Every resource below start (not start itself), as paths; store files
// are not resources.
void walk(const string& start, vector<string>& out)
{
  SDirectory dir(start);
  vector<SFile> kids = dir.get_files(false);
  for (size_t i = 0; i < kids.size(); i++) {
    // Hidden files are not resources, as listCollection treats them.
    if (kids[i].filename().compare(0, 1, ".") == 0) continue;
    string path = start + "/" + kids[i].filename();
    out.push_back(path);
    if (isDirectory(path)) walk(path, out);
  }
}

// A new child's URL in its parent's form: the resource pool is keyed by
// URL text, so "file:///a" and "/a" would be two different resources.
string childURL(const EcceURL& parent, const string& path)
{
  return parent.getProtocol() == "file" ? "file://" + path : path;
}

} // namespace


// The built-in file-system properties of a path, with the stored ones
// (if any) laid over them.
static bool describePath(const string& path, const PropMap* stored,
                         vector<MetaDataResult>& metaDataNames)
{
  SFile file(path.c_str());
  if (!file.exists()) return false;

  MetaDataResult mdr;
  char buf[128];

  mdr.name = "DAV:displayname";
  mdr.type = "string";
  mdr.value = file.filename().c_str();
  metaDataNames.push_back(mdr);

  mdr.name = "DAV:getcontentlength";
  mdr.type = "integer";
  unsigned int fileSize = file.size();
  sprintf(buf,"%u",fileSize);
  mdr.value = buf;
  metaDataNames.push_back(mdr);

  mdr.name = "resourcetype";
  mdr.type = "string";
  if (file.is_dir()) {
    mdr.value = "collection";
  } else if (file.is_link()) {
    mdr.value = "link";
  } else {
    mdr.value = "file";
  }
  metaDataNames.push_back(mdr);

  // As Apache would type it (mimeTypeFor).
  mdr.name = "contenttype";
  mdr.type = "string";
  if (file.is_dir()) {
    mdr.value = "httpd/unix-directory";
  } else if (file.is_link()) {
    mdr.value = "link";
  } else {
    mdr.value = mimeTypeFor(file.filename(), file.extension());
  }
  metaDataNames.push_back(mdr);

  // The name DavEDSI's callers ask for (Resource::getMimeType, VDoc), which
  // is how a calculation's input file is told from its other files.
  mdr.name = "DAV:getcontenttype";
  metaDataNames.push_back(mdr);

  mdr.name = "application";
  mdr.type = "string";
  mdr.value = "";
  metaDataNames.push_back(mdr);

  mdr.name = "DAV:getlastmodified";
  mdr.type = "string";
  mdr.value = file.lastModified().toString();
  metaDataNames.push_back(mdr);

  if (stored) mergeStored(metaDataNames, *stored);
  return true;
}

// What DavEDSI::getResourceMetaDataResult does: the stored ecce:contenttype
// names the kind of resource, and ecce:resourcetype marks a calculation (a
// directory on disk) as a virtual document.
static void applyStoredTypes(ResourceMetaDataResult& r,
                             const vector<MetaDataResult>& props)
{
  string ns = VDoc::getEcceNamespace();
  for (size_t i = 0; i < props.size(); i++) {
    if (props[i].name == ns + ":contenttype") {
      r.contenttype = props[i].value;
    } else if (props[i].name == ns + ":resourcetype" &&
               ResourceUtils::stringToResourceType(props[i].value) ==
               ResourceDescriptor::RT_VIRTUAL_DOCUMENT) {
      r.resourcetype = ResourceDescriptor::RT_VIRTUAL_DOCUMENT;
    }
  }
}

static const PropMap *findProps(const MetaStore& store, const string& key)
{
  MetaStore::const_iterator it = store.find(key);
  return it == store.end() ? 0 : &it->second;
}

bool FileEDSI::exists(const bool& newUser)
{
  SFile test(getURL().getPath());
  return test.exists();
}

istream* FileEDSI::getDataSubSet(int start_position, int length)
{
  std::ostringstream os;
  if (!getDataSubSet(os, start_position, length)) return NULL;
  return new std::istringstream(os.str());
}

// Bytes [start, start+length-1], clipped at end of file, as an HTTP Range.
bool FileEDSI::getDataSubSet(ostream& dest, int start_position, int length)
{
  m_msgStack.clear();
  string all;
  if (!readAll(p_url.getPath(), all)) {
    m_msgStack.add("RESOURCE_NOT_FOUND",p_url.getPath().c_str());
    return false;
  }
  if (start_position >= 0 && length > 0 && (size_t)start_position < all.size())
    dest << all.substr(start_position, length);
  return true;
}

unsigned long FileEDSI::getDataSetSize()
{
  m_msgStack.clear();
  struct stat st;
  if (stat(p_url.getPath().c_str(), &st) != 0) {
    m_msgStack.add("RESOURCE_NOT_FOUND",p_url.getPath().c_str());
    return 0;
  }
  return st.st_size;
}

// Byte for byte: appendDataSet's overwrite count is in bytes, so a
// line-based read that adds a final newline would break it.
bool FileEDSI::getDataSet(ostream& dest)
{
  m_msgStack.clear();
  SFile file(p_url.getPath().c_str());
  string all;
  if (file.exists() && file.is_regular_file() && readAll(file.path(), all)) {
    dest << all;
    return true;
  }
  return false;
}

istream *FileEDSI::getDataSet()
{
  m_msgStack.clear();
  SFile file(p_url.getPath().c_str());
  bool exists = file.exists();
  if (exists && file.is_regular_file()) {
    string all;
    if (readAll(file.path(), all)) return new std::istringstream(all);
    m_msgStack.add("UNABLE_TO_READ",file.path().c_str());
  } else if (exists) {
    m_msgStack.add("NOT_REGULAR_FILE",file.path().c_str());
  } else {
    m_msgStack.add("RESOURCE_NOT_FOUND",file.path().c_str());
  }
  return NULL;
}

bool FileEDSI::listCollection(vector<ResourceMetaDataResult>& result)
{
  return listCollection(vector<MetaDataRequest>(), result);
}

bool FileEDSI::listCollection(const vector<MetaDataRequest>& requests,
                              vector<ResourceMetaDataResult>& result)
{
  bool ret = false;
  m_msgStack.clear();

  SDirectory dir(p_url.getPath().c_str());

  if (!dir.exists()) {
    m_msgStack.add("RESOURCE_NOT_FOUND",dir.path().c_str());
  } else if (!dir.is_dir()) {
    m_msgStack.add("NOT_COLLECTION",dir.path().c_str());
  } else {
    vector<SFile> files = dir.get_files(false);
    ret = true;
    MetaStore store = loadStore(trimSlash(dir.path()));   // once per listing

    for (size_t idx=0; idx<files.size(); idx++) {
      // Dot files include the sidecar itself.
      if (files[idx].filename().compare(0, 1, ".") == 0) continue;
      ResourceMetaDataResult rmdr;
      rmdr.url = files[idx].path().c_str();
      rmdr.resourcetype = ResourceDescriptor::RT_COLLECTION;
      if (files[idx].is_regular_file())
        rmdr.resourcetype = ResourceDescriptor::RT_DOCUMENT;
      rmdr.contenttype = "";

      const PropMap *stored = 0;
      MetaStore sub;
      if (files[idx].is_dir()) {
        sub = loadStore(trimSlash(files[idx].path()));
        stored = findProps(sub, ".");
      } else {
        stored = findProps(store, files[idx].filename());
      }

      vector<MetaDataResult> tmp;
      if (describePath(files[idx].path(), stored, tmp)) {
        for (size_t t = 0; t < tmp.size(); t++) {
          if (tmp[t].name == "contenttype") rmdr.contenttype = tmp[t].value;
        }
        applyStoredTypes(rmdr, tmp);
        if (requests.empty()) rmdr.metaData = tmp;
        for (size_t q = 0; q < requests.size(); q++) {
          for (size_t t = 0; t < tmp.size(); t++) {
            if (tmp[t].name == requests[q].name) {
              rmdr.metaData.push_back(tmp[t]);
              break;
            }
          }
        }
      }
      result.push_back(rmdr);
    }
  }
  return ret;
}

bool FileEDSI::listCollection(vector<ResourceResult>& result)
{
  bool ret = false;
  m_msgStack.clear();

  SDirectory dir(p_url.getPath().c_str());
  bool exists = dir.exists();
  if (exists && dir.is_dir()) {
    ret = true;
    vector<SFile> files = dir.get_files(false);
    int cnt = files.size();
    for (int idx=0; idx<cnt; idx++) {
      if (strncmp(files[idx].filename().c_str(), ".", 1) != 0) {
        ResourceResult res;
        res.url = EcceURL(files[idx].path());
        res.resourcetype = ResourceDescriptor::RT_COLLECTION;
        res.contenttype = "httpd/unix-directory";
        if (files[idx].is_regular_file()) {
          res.resourcetype = ResourceDescriptor::RT_DOCUMENT;
          res.contenttype = mimeTypeFor(files[idx].filename(), files[idx].extension());
        }
        result.push_back(res);
      }
    }
  } else if (exists) {
    m_msgStack.add("NOT_COLLECTION",dir.path().c_str());
  } else {
    m_msgStack.add("RESOURCE_NOT_FOUND",dir.path().c_str());
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::putDataSet(const char *putStream)
{
  bool ret = false;
  m_msgStack.clear();

  SFile file(p_url.getPath().c_str());
  ofstream ofs(file.path().c_str());
  if (ofs) {
    ofs << putStream;
    ofs.close();
    ret = true;
  } else {
    m_msgStack.add("UNABLE_TO_WRITE",file.path().c_str());
  }
  return ret;
}

bool FileEDSI::putDataSet(istream& putStream)
{
  bool ret = false;
  m_msgStack.clear();

  SFile file(p_url.getPath().c_str());
  ofstream ofs(file.path().c_str());
  if (ofs) {
    const int bsz = 1024;
    char      buff[bsz];
    int nRead;
    do {
      putStream.read(buff, bsz);
      nRead = putStream.gcount();
      if (nRead > 0) {
        ofs.write(buff,nRead);
      }
   } while (putStream);
   ofs.close();
   ret = true;
  } else {
    m_msgStack.add("UNABLE_TO_WRITE",file.path().c_str());
  }
  return ret;
}


bool FileEDSI::appendDataSet(const char* putStream, int bytesToOverwrite)
{
  m_msgStack.clear();
  string path = p_url.getPath();
  if (!putStream) putStream = "";
  if (!appendBytes(path, putStream, bytesToOverwrite)) {
    m_msgStack.add("UNABLE_TO_WRITE",path.c_str());
    return false;
  }
  return true;
}

bool FileEDSI::appendDataSet(istream& putStream, int bytesToOverwrite)
{
  m_msgStack.clear();
  string path = p_url.getPath();
  std::ostringstream ss;
  ss << putStream.rdbuf();
  if (!appendBytes(path, ss.str(), bytesToOverwrite)) {
    m_msgStack.add("UNABLE_TO_WRITE",path.c_str());
    return false;
  }
  return true;
}

bool FileEDSI::getMetaData(const vector<MetaDataRequest>& requests,
    vector<MetaDataResult>& results, bool getVDocMetaData)
{
  m_msgStack.clear();
  string path = p_url.getPath();
  string dir, key;
  locate(path, dir, key);
  MetaStore store = loadStore(dir);

  vector<MetaDataResult> tmp;
  if (!describePath(path, findProps(store, key), tmp)) {
    m_msgStack.add("RESOURCE_NOT_FOUND",path.c_str());
    return false;
  }
  if (requests.empty()) results.insert(results.end(), tmp.begin(), tmp.end());
  for (size_t idx=0; idx<requests.size(); idx++) {
    for (size_t t = 0; t < tmp.size(); t++) {
      if (tmp[t].name == requests[idx].name) {
        results.push_back(tmp[t]);
        break;
      }
    }
  }
  return true;
}

bool FileEDSI::getMetaData(const vector<MetaDataRequest>& requests,
    ResourceMetaDataResult& results, bool getVDocMetaData)
{
  SFile file(p_url.getPath().c_str());

  vector<MetaDataResult> tmp;
  if (!getMetaData(requests, tmp)) return false;

  results.url = p_url;
  results.resourcetype = ResourceDescriptor::RT_COLLECTION;
  results.contenttype = "httpd/unix-directory";
  if (file.is_regular_file()) {
    results.resourcetype = ResourceDescriptor::RT_DOCUMENT;
    results.contenttype = mimeTypeFor(file.filename(), file.extension());
  }
  for (size_t i = 0; i < tmp.size(); i++) {
    results.metaData.push_back(tmp[i]);
  }
  applyStoredTypes(results, tmp);
  return true;
}

bool FileEDSI::putMetaData(const vector<MetaDataResult>& results)
{
  m_msgStack.clear();
  string path = p_url.getPath();
  if (!SFile(path.c_str()).exists()) {
    m_msgStack.add("RESOURCE_NOT_FOUND",path.c_str());
    return false;
  }
  string dir, key;
  locate(path, dir, key);
  DirLock lock(dir);
  MetaStore store = loadStore(dir);
  for (size_t i = 0; i < results.size(); i++) {
    if (results[i].name.empty()) continue;      // as DavEDSI drops them
    store[key][results[i].name] = results[i];
    store[key][results[i].name].value = xmlContent(results[i].value);
  }
  if (!saveStore(dir, store)) {
    m_msgStack.add("UNABLE_TO_WRITE",sidecarPath(dir).c_str());
    return false;
  }
  return true;
}

// Removing a property that is not there succeeds, as a PROPPATCH remove does.
bool FileEDSI::removeMetaData(const vector<MetaDataRequest>& requests)
{
  m_msgStack.clear();
  string path = p_url.getPath();
  if (!SFile(path.c_str()).exists()) {
    m_msgStack.add("RESOURCE_NOT_FOUND",path.c_str());
    return false;
  }
  string dir, key;
  locate(path, dir, key);
  DirLock lock(dir);
  MetaStore store = loadStore(dir);
  MetaStore::iterator it = store.find(key);
  if (it == store.end()) return true;
  for (size_t i = 0; i < requests.size(); i++) it->second.erase(requests[i].name);
  if (it->second.empty()) store.erase(it);
  if (!saveStore(dir, store)) {
    m_msgStack.add("UNABLE_TO_WRITE",sidecarPath(dir).c_str());
    return false;
  }
  return true;
}

bool FileEDSI::describeMetaData(vector<MetaDataResult>& metaDataNames)
{
  string path = p_url.getPath();
  string dir, key;
  locate(path, dir, key);
  MetaStore store = loadStore(dir);
  if (describePath(path, findProps(store, key), metaDataNames)) return true;
  m_msgStack.add("RESOURCE_NOT_FOUND",path.c_str());
  return false;
}

// Where a move or copy lands under the overwrite rule: SORTOF picks a
// free name (and updates targetURL), NO refuses a taken one, YES removes it.
bool FileEDSI::prepareTarget(EcceURL& targetURL, EDSIOverwrite overwrite)
{
  string to = trimSlash(targetURL.getPath());
  if (overwrite == SORTOF) {
    string name = uniqueIn(parentOf(to), tailOf(to), "-%d");
    if (name != tailOf(to)) {
      string path = parentOf(to) + "/" + name;
      targetURL = EcceURL(targetURL.getProtocol().c_str(),
                          targetURL.getHost().c_str(), targetURL.getPort(),
                          path.c_str());
    }
  } else if (overwrite == YES && pathExists(to)) {
    if (!removeHelper(targetURL)) return false;
  } else if (overwrite == NO && pathExists(to)) {
    m_msgStack.add("UNABLE_TO_COMPLETE_REQUEST",
                   "  ECCE cannot overwrite existing resource.");
    return false;
  }
  return true;
}

bool FileEDSI::moveResource(EcceURL& targetURL, EDSIOverwrite overwrite)
{
  m_msgStack.clear();
  string from = trimSlash(p_url.getPath());
  if (!pathExists(from)) {
    m_msgStack.add("RESOURCE_NOT_FOUND", from.c_str());
    return false;
  }
  if (!prepareTarget(targetURL, overwrite)) return false;
  string to = trimSlash(targetURL.getPath());
  // The rename and the property record move together under both locks.
  TwoDirLock lock(parentOf(from), parentOf(to));
  bool wasDir = isDirectory(from);
  if (!renameNoReplace(from, to)) {
    m_msgStack.add(errno == EEXIST ? "UNABLE_TO_COMPLETE_REQUEST"
                                   : "UNABLE_TO_WRITE",
                   errno == EEXIST ? "  ECCE cannot overwrite existing resource."
                                   : to.c_str());
    return false;
  }
  return wasDir || transferFileProps(from, to, false);
}

bool FileEDSI::copyResource(EcceURL& targetURL, EDSIOverwrite overwrite)
{
  m_msgStack.clear();
  string from = trimSlash(p_url.getPath());
  if (!pathExists(from)) {
    m_msgStack.add("RESOURCE_NOT_FOUND", from.c_str());
    return false;
  }
  if (!prepareTarget(targetURL, overwrite)) return false;
  string to = trimSlash(targetURL.getPath());
  if (isDirectory(from)) {
    if (!copyTree(from, to)) {
      m_msgStack.add("UNABLE_TO_WRITE", to.c_str());
      return false;
    }
    return true;
  }
  TwoDirLock lock(parentOf(from), parentOf(to));
  if (!copyFile(from, to)) {
    m_msgStack.add("UNABLE_TO_WRITE", to.c_str());
    return false;
  }
  return transferFileProps(from, to, true);
}

bool FileEDSI::removeResource()
{
  m_msgStack.clear();
  return removeHelper(p_url);
}

bool FileEDSI::removeHelper(const EcceURL& url)
{
  bool ret = false;
  string path = url.getPath();
  SDirectory dir(path.c_str());
  if (dir.exists()) {
    bool wasDir = dir.is_dir();
    if (wasDir) {
      ret = dir.remove();               // takes its own sidecar with it
    } else {
      SFile file(dir.path());
      ret = file.remove();
      if (ret) dropFileProps(path);
    }
    if (!ret) {
      m_msgStack.add("UNABLE_TO_WRITE",dir.path().c_str());
    }
  } else {
    m_msgStack.add("RESOURCE_NOT_FOUND",dir.path().c_str());
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
EcceURL *FileEDSI::makeCollection(const string& base, const string& pattern)
{
  EcceURL *ret = NULL;
  m_msgStack.clear();

  // At a minimum pattern must include int
  if (pattern.find("%d") != string::npos) {
    // Get a unique name
    string newBase = uniqueName(base);

    string path = p_url.getPath().c_str();
    path += "/" + newBase;
    SDirectory dir(p_url.getPath().c_str());
    if (dir.exists() && dir.is_dir()) {
      if (SDirectory::create(path.c_str(),0744)) {
        ret = new EcceURL(childURL(p_url, path));
      } else {
        m_msgStack.add("UNABLE_TO_WRITE",dir.path().c_str());
      }
    } else {
      if (dir.exists()) {
        m_msgStack.add("NOT_COLLECTION",dir.path().c_str());
      } else {
        m_msgStack.add("RESOURCE_NOT_FOUND",dir.path().c_str());
      }
    }
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
EcceURL *FileEDSI::makeDataSet(const string& base)
{
  EcceURL *ret = NULL;
  m_msgStack.clear();

  // Get a unique name
  string newBase = uniqueName(base);

  string path = p_url.getPath().c_str();
  path += "/" + newBase;
  SDirectory dir(p_url.getPath().c_str());

  // Check the parent directory exists
  if (dir.exists() && dir.is_dir()) {
    if (SFile::create(path.c_str(),0644)) {
      ret = new EcceURL(childURL(p_url, path));
    } else {
      m_msgStack.add("UNABLE_TO_WRITE",dir.path().c_str());
    }
  } else {
    if (dir.exists()) {
      m_msgStack.add("NOT_COLLECTION",dir.path().c_str());
    } else {
      m_msgStack.add("RESOURCE_NOT_FOUND",dir.path().c_str());
    }
  }
  return ret;
}

string FileEDSI::uniqueName(const string& guess, const string& pattern)
{
  return uniqueIn(trimSlash(p_url.getPath()), guess, pattern);
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
string FileEDSI::getClassName()
{
  return "FileEDSI";
}


bool FileEDSI::isWritable()
{
  return access(p_url.getPath().c_str(), W_OK) == 0;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
// Nothing holds a resource locked between calls, as with DavEDSI; writes
// are serialised per directory by DirLock instead.
bool FileEDSI::isLocked(string& locker)
{
  m_msgStack.clear();
  locker = "";
  return false;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::removeResources(const vector<EcceURL> urls) {
  bool ret = true;
  m_msgStack.clear();

  int size = urls.size();
  for (int idx=0; idx<size; idx++) {
    if (!removeHelper(urls[idx])) ret = false;
  }
  return ret;
}

// Find by name: every resource below start whose path below start holds
// substring.  DavEDSI matches the whole URL, which in a data folder would
// match the folder's own name in every result.
bool FileEDSI::efind(const string& key, const string& substring,
                     const EcceURL& start, vector<EcceURL>& matches)
{
  m_msgStack.clear();
  string root = trimSlash(start.getPath());
  if (!isDirectory(root)) {
    m_msgStack.add("RESOURCE_NOT_FOUND", root.c_str());
    return false;
  }
  vector<string> all;
  walk(root, all);
  for (size_t i = 0; i < all.size(); i++) {
    if (all[i].substr(root.size()).find(substring) != string::npos)
      matches.push_back(EcceURL(childURL(start, all[i])));
  }
  return true;
}

// Calculations below start whose state is substring, ignoring case.
bool FileEDSI::efindProp(const string& substring, const EcceURL& start,
                         vector<EcceURL>& matches)
{
  m_msgStack.clear();
  string root = trimSlash(start.getPath());
  if (!isDirectory(root)) {
    m_msgStack.add("RESOURCE_NOT_FOUND", root.c_str());
    return false;
  }
  string ns = VDoc::getEcceNamespace();
  string want = substring;
  STLUtil::toUpper(want);
  vector<string> all;
  walk(root, all);
  for (size_t i = 0; i < all.size(); i++) {
    if (!isDirectory(all[i])) continue;
    MetaStore store = loadStore(all[i]);
    const PropMap *props = findProps(store, ".");
    if (!props) continue;
    PropMap::const_iterator rt = props->find(ns + ":resourcetype");
    PropMap::const_iterator st = props->find(ns + ":state");
    if (rt == props->end() || st == props->end() ||
        ResourceUtils::stringToResourceType(rt->second.value) !=
        ResourceDescriptor::RT_VIRTUAL_DOCUMENT) continue;
    string val = st->second.value;
    STLUtil::toUpper(val);
    if (val == want) matches.push_back(EcceURL(childURL(start, all[i])));
  }
  return true;
}
