#include <algorithm>
#include <cstdlib>
#include <memory>

#include "util/Filesystem.H"
#include "util/LocalData.H"
#include "util/TDateTime.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/TreeCopy.H"
#include "dsm/VDoc.H"
#include "dsm/RunFolderCopy.H"

using std::string;
using std::vector;

const char* RunFolderCopy::COPY_OF = "copyOf";
const char* RunFolderCopy::COPY_ORIGIN = "copyOrigin";
const char* RunFolderCopy::COPY_ORIGIN_ROOT = "copyOriginRoot";
const char* RunFolderCopy::COPY_MADE_AT = "copyMadeAt";

namespace {

// The run folder already holds the code's own output, so an Outputs file
// this large is left out of the copy rather than sent back over the link.
const unsigned long long OUTPUT_CAP = 50ULL * 1024 * 1024;

string ns(const char* key) { return VDoc::getEcceNamespace() + ":" + key; }

MetaDataResult prop(const string& name, const string& value)
{
  MetaDataResult m;
  m.name = name;
  m.value = value;
  return m;
}

string originRoot(const EcceURL& calc)
{
  if (calc.getProtocol() == "file") {
    string d = LocalData::dir();
    return d.empty() ? calc.getParent().getPath() : d;
  }
  string s = calc.getProtocol() + "://" + calc.getHost();
  if (calc.getPort() > 0) s += ":" + std::to_string(calc.getPort());
  return s;
}

}

bool RunFolderCopy::applies(const EcceURL& calc)
{
  const char* e = getenv("ECCE_RUN_FOLDER_COPY");
  if (e && string(e) == "0") return false;
  return calc.getProtocol() != "sftp";
}

string RunFolderCopy::stage(const EcceURL& calc, const string& parent,
                            const vector<string>& keepDirs,
                            const string& originHost, const string& state,
                            string& err)
{
  std::error_code ec;
  ecce::fs::remove_all(parent, ec);
  ecce::fs::create_directories(parent, ec);
  if (ec) { err = "cannot make " + parent + ": " + ec.message(); return ""; }

  TreeCopy::Result r = TreeCopy::copy(calc, EcceURL(("file://" + parent).c_str()),
                                      false, TreeCopy::Progress());
  if (!r.ok) { err = r.error; return ""; }
  const string root = r.target.getPath();

  for (ecce::fs::directory_iterator it(root, ec), end; !ec && it != end;
       it.increment(ec)) {
    const string name = it->path().filename().string();
    if (it->is_directory() &&
        std::find(keepDirs.begin(), keepDirs.end(), name) == keepDirs.end())
      ecce::fs::remove_all(it->path(), ec);
  }
  const ecce::fs::path outs = ecce::fs::path(root) / "Outputs";
  if (ecce::fs::is_directory(outs, ec)) {
    for (ecce::fs::recursive_directory_iterator it(outs, ec), end;
         !ec && it != end; it.increment(ec)) {
      if (it->is_regular_file() && it->file_size() > OUTPUT_CAP) {
        std::error_code rc;
        ecce::fs::remove(it->path(), rc);
      }
    }
  }

  vector<MetaDataResult> marks;
  marks.push_back(prop(ns(COPY_OF), calc.toString()));
  marks.push_back(prop(ns(COPY_ORIGIN), originHost));
  marks.push_back(prop(ns(COPY_ORIGIN_ROOT), originRoot(calc)));
  marks.push_back(prop(ns(COPY_MADE_AT), TDateTime().toString()));
  if (!state.empty()) marks.push_back(prop(ns("state"), state));
  std::unique_ptr<EDSI> edsi(EDSIFactory::getEDSI(r.target));
  if (!edsi || !edsi->putMetaData(marks)) {
    err = "cannot mark the copy in " + root;
    return "";
  }
  return root;
}

vector<RunFolderCopy::Batch> RunFolderCopy::batches(const string& stagedRoot)
{
  vector<Batch> out;
  std::error_code ec;
  vector<ecce::fs::path> dirs(1, ecce::fs::path(stagedRoot));
  for (size_t d = 0; d < dirs.size(); d++) {
    Batch b;
    b.dir = d == 0 ? string("")
                   : ecce::fs::relative(dirs[d], stagedRoot, ec).generic_string();
    for (ecce::fs::directory_iterator it(dirs[d], ec), end; !ec && it != end;
         it.increment(ec)) {
      const string name = it->path().filename().string();
      if (it->is_directory()) dirs.push_back(it->path());
      else if (name != ".ecce-meta.lock") b.files.push_back(it->path().string());
    }
    std::sort(b.files.begin(), b.files.end());
    out.push_back(b);
  }
  return out;
}

string RunFolderCopy::copyNote(const string& copyOf, const string& origin)
{
  if (copyOf.empty()) return "";
  EcceURL u(copyOf.c_str());
  string name = u.getFilePathTail();
  return "copy of " + name + (origin.empty() ? string("") : " on " + origin);
}
