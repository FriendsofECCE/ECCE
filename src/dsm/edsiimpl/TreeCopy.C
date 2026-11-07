#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>

#include "dsm/TreeCopy.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ResourceDescriptor.H"
#include "util/SFile.H"
#include "util/TempStorage.H"

using std::string;
using std::vector;

namespace {

typedef std::unique_ptr<EDSI> EdsiPtr;

EdsiPtr edsiFor(const EcceURL& url)
{
  return EdsiPtr(EDSIFactory::getEDSI(url));
}

bool isDir(const ResourceResult& r)
{
  return r.resourcetype != ResourceDescriptor::RT_DOCUMENT;
}

// Size from the listing's own metadata when it has it, else one more call.
unsigned long sizeOf(const ResourceMetaDataResult& r)
{
  for (size_t i = 0; i < r.metaData.size(); i++)
    if (r.metaData[i].name == "DAV:getcontentlength")
      return strtoul(r.metaData[i].value.c_str(), 0, 10);
  EdsiPtr e = edsiFor(r.url);
  return e ? e->getDataSetSize() : 0;
}

// The built-in names are rebuilt by the target; the live DAV ones cannot
// be set.  Everything else, the Ecce-namespace properties included, is kept.
vector<MetaDataResult> keepable(const vector<MetaDataResult>& in)
{
  vector<MetaDataResult> out;
  for (size_t i = 0; i < in.size(); i++) {
    const string& n = in[i].name;
    if (n.find("DAV") != string::npos || n == "resourcetype" ||
        n == "contenttype" || n.empty())
      continue;
    MetaDataResult m = in[i];
    m.value = "<![CDATA[" + m.value + "]]>";     // no namespace rewriting
    out.push_back(m);
  }
  return out;
}

struct Job {
  const TreeCopy::Progress *progress;
  TreeCopy::Result *res;
  long failAfter;      // test hook: ECCE_TEST_TREECOPY_FAIL_AFTER=N files
  string error;
};

bool report(Job& j, const string& item)
{
  if (j.progress && *j.progress && !(*j.progress)(item, j.res->files, j.res->bytes)) {
    j.res->cancelled = true;
    j.error = "cancelled";
    return false;
  }
  return true;
}

bool copyProps(EDSI& to, const EcceURL& at, const vector<MetaDataResult>& props,
               Job& j)
{
  vector<MetaDataResult> keep = keepable(props);
  if (keep.empty()) return true;
  to.setURL(at);
  if (!to.putMetaData(keep)) {
    j.error = "cannot store the properties of " + at.getFilePathTail() + ": " +
              to.m_msgStack.getMessage();
    return false;
  }
  return true;
}

bool copyFile(const ResourceMetaDataResult& src, const EcceURL& destFolder,
              const string& rel, Job& j)
{
  EdsiPtr from = edsiFor(src.url), to = edsiFor(destFolder);
  if (!from || !to) { j.error = "no access to the data store"; return false; }
  const string name = src.url.getFilePathTail();
  if (!report(j, rel)) return false;
  if (j.failAfter >= 0 && j.res->files >= j.failAfter) {
    j.error = "simulated failure while copying " + rel;
    return false;
  }

  SFile *tmp = TempStorage::getTempFile();
  const string path = tmp->path();
  bool ok = false;
  unsigned long written = 0;
  {
    std::ofstream out(path.c_str(), std::ios::binary);
    ok = out && from->getDataSet(out);
    out.close();
    if (!ok) j.error = "cannot read " + rel + ": " + from->m_msgStack.getMessage();
  }
  if (ok) {
    SFile f(path.c_str());
    written = f.size();
    EcceURL *made = to->makeDataSet(name);
    if (!made) {
      j.error = "cannot create " + rel + ": " + to->m_msgStack.getMessage();
      ok = false;
    } else {
      EcceURL at(*made);
      delete made;
      to->setURL(at);
      std::ifstream in(path.c_str(), std::ios::binary);
      ok = to->putDataSet(in);
      in.close();
      if (!ok) j.error = "cannot write " + rel + ": " + to->m_msgStack.getMessage();
      else ok = copyProps(*to, at, src.metaData, j);
    }
  }
  tmp->remove();
  delete tmp;
  if (!ok) return false;

  TreeCopy::Entry e = { rel, false, written };
  j.res->manifest.push_back(e);
  j.res->files++;
  j.res->bytes += written;
  return true;
}

bool copyDir(const ResourceMetaDataResult& src, const EcceURL& destFolder,
             const string& rel, EcceURL& madeUrl, Job& j, bool unique)
{
  EdsiPtr from = edsiFor(src.url), to = edsiFor(destFolder);
  if (!from || !to) { j.error = "no access to the data store"; return false; }
  const string name = src.url.getFilePathTail();
  if (!report(j, rel.empty() ? name : rel)) return false;

  to->setURL(destFolder);
  if (!unique) {
    to->setURL(destFolder.getChild(name));
    if (to->exists()) {
      j.error = "there is already something named \"" + name + "\" there";
      return false;
    }
    to->setURL(destFolder);
  }
  EcceURL *made = to->makeCollection(name);
  if (!made) {
    j.error = "cannot create folder " + name + ": " + to->m_msgStack.getMessage();
    return false;
  }
  madeUrl = *made;
  delete made;
  if (rel.empty()) j.res->target = madeUrl;     // so a failure below removes it
  TreeCopy::Entry e = { rel, true, 0 };
  j.res->manifest.push_back(e);
  if (!copyProps(*to, madeUrl, src.metaData, j)) return false;

  vector<ResourceMetaDataResult> kids;
  if (!from->listCollection(kids)) {
    j.error = "cannot list " + name + ": " + from->m_msgStack.getMessage();
    return false;
  }
  for (size_t i = 0; i < kids.size(); i++) {
    const string kidRel = (rel.empty() ? "" : rel + "/") +
                          kids[i].url.getFilePathTail();
    if (isDir(kids[i])) {
      EcceURL sub;
      if (!copyDir(kids[i], madeUrl, kidRel, sub, j, true)) return false;
    } else if (!copyFile(kids[i], madeUrl, kidRel, j)) {
      return false;
    }
  }
  return true;
}

void listTree(const EcceURL& url, const string& rel,
              std::map<string, std::pair<bool, unsigned long> >& out, string& err)
{
  EdsiPtr e = edsiFor(url);
  vector<ResourceMetaDataResult> kids;
  if (!e || !e->listCollection(kids)) {
    err = "cannot list " + url.getFilePathTail();
    return;
  }
  for (size_t i = 0; i < kids.size() && err.empty(); i++) {
    const string r = (rel.empty() ? "" : rel + "/") + kids[i].url.getFilePathTail();
    if (isDir(kids[i])) {
      out[r] = std::make_pair(true, 0UL);
      listTree(kids[i].url, r, out, err);
    } else {
      out[r] = std::make_pair(false, sizeOf(kids[i]));
    }
  }
}

}  // namespace


TreeCopy::Result TreeCopy::copy(const EcceURL& src, const EcceURL& destFolder,
                                bool uniqueName, const Progress& progress)
{
  Result res;
  Job job;
  job.progress = &progress;
  job.res = &res;
  const char *fail = getenv("ECCE_TEST_TREECOPY_FAIL_AFTER");
  job.failAfter = fail ? atol(fail) : -1;

  // The source's own description (type and properties), then the walk.
  ResourceMetaDataResult root;
  {
    EdsiPtr from = edsiFor(src);
    vector<MetaDataRequest> none;
    if (!from || !from->exists() || !from->getMetaData(none, root, false)) {
      res.error = "the source no longer exists";
      return res;
    }
    root.url = src;
  }
  bool ok;
  if (isDir(root)) {
    EcceURL made;
    ok = copyDir(root, destFolder, "", made, job, uniqueName);
    if (!ok && res.target.toString().empty() == false) removeTree(res.target);
  } else {
    // A single file: same path, the file is the root.
    EdsiPtr to = edsiFor(destFolder);
    ok = true;
    if (!uniqueName && to) {
      to->setURL(destFolder.getChild(src.getFilePathTail()));
      if (to->exists()) {
        job.error = "there is already something named \"" +
                    src.getFilePathTail() + "\" there";
        ok = false;
      }
    }
    if (ok) ok = copyFile(root, destFolder, src.getFilePathTail(), job);
    if (ok) {
      res.manifest.back().path = "";     // verify() reads this as a lone file
      // copyFile names it as the target would; find it by listing.
      EdsiPtr dest = edsiFor(destFolder);
      vector<ResourceMetaDataResult> kids;
      if (dest && dest->listCollection(kids))
        for (size_t i = 0; i < kids.size(); i++)
          if (kids[i].url.getFilePathTail() == src.getFilePathTail())
            res.target = kids[i].url;
    }
  }
  res.ok = ok;
  res.error = job.error;
  return res;
}


string TreeCopy::verify(const EcceURL& target, const vector<Entry>& m)
{
  if (m.size() == 1 && !m[0].dir && m[0].path.empty()) {
    EdsiPtr e = edsiFor(target);
    return e && e->getDataSetSize() == m[0].size ? "" : "the copy differs in size";
  }
  std::map<string, std::pair<bool, unsigned long> > found;
  string err;
  listTree(target, "", found, err);
  if (!err.empty()) return err;
  size_t expected = 0;
  for (size_t i = 0; i < m.size(); i++) {
    if (m[i].path.empty()) continue;
    expected++;
    std::map<string, std::pair<bool, unsigned long> >::const_iterator f =
        found.find(m[i].path);
    if (f == found.end()) return m[i].path + " is missing in the copy";
    if (f->second.first != m[i].dir) return m[i].path + " has the wrong type";
    if (!m[i].dir && f->second.second != m[i].size)
      return m[i].path + " has a different size in the copy";
  }
  if (found.size() != expected) return "the copy holds files the source did not";
  return "";
}


bool TreeCopy::removeTree(const EcceURL& url)
{
  EdsiPtr e = edsiFor(url);
  if (!e) return false;
  bool ok = e->removeResource();
  EDSIFactory::removePoolResource(url);
  return ok;
}
