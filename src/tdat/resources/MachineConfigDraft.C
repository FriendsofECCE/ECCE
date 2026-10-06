#include <algorithm>

#include "tdat/ConfigFile.H"
#include "tdat/MachineConfigDraft.H"
#include "util/MiniJson.H"
#include "util/StringConverter.H"

using std::map;
using std::string;
using std::vector;

typedef MachineConfigDraft MCD;

static string lower(const string& s)
{
  string out;
  StringConverter::toLower(s, out);
  return out;
}

bool MCD::Layer::operator==(const Layer& o) const
{
  return source == o.source && file == o.file && hasValue == o.hasValue &&
         value == o.value;
}

bool MCD::KeyState::operator==(const KeyState& o) const
{
  return inherited == o.inherited && edit == o.edit &&
         (edit != Set || value == o.value);
}

bool MCD::MachineLine::operator==(const MachineLine& o) const
{
  return name == o.name && host == o.host && vendor == o.vendor &&
         model == o.model && proc == o.proc && procs == o.procs &&
         nodes == o.nodes && allocationAccount == o.allocationAccount;
}

bool MCD::QueueRow::operator==(const QueueRow& o) const
{
  return name == o.name && minProcs == o.minProcs && maxProcs == o.maxProcs &&
         defProcs == o.defProcs && maxWall == o.maxWall &&
         defWall == o.defWall && maxMem == o.maxMem && defMem == o.defMem &&
         minScratch == o.minScratch && defScratch == o.defScratch;
}

// Untouched keys (no edit, same layers) count as equal whether or not they
// have a map entry yet.
bool MCD::Content::operator==(const Content& o) const
{
  if (!(line == o.line) || qmgr != o.qmgr || queuesOwned != o.queuesOwned ||
      !(queues == o.queues))
    return false;
  map<string,KeyState>::const_iterator a = config.begin(), b = o.config.begin();
  while (a != config.end() || b != o.config.end()) {
    if (b == o.config.end() || (a != config.end() && a->first < b->first)) {
      if (a->second.edit != Inherit) return false;
      ++a;
    } else if (a == config.end() || b->first < a->first) {
      if (b->second.edit != Inherit) return false;
      ++b;
    } else {
      if (!(a->second == b->second)) return false;
      ++a; ++b;
    }
  }
  return true;
}

MCD::MachineConfigDraft(Mode mode, const string& siteFile,
                        const string& userFile)
  : p_mode(mode), p_siteFile(siteFile), p_userFile(userFile)
{
}

const string& MCD::editedFile() const
{
  return p_mode == AdminMode ? p_siteFile : p_userFile;
}

// Collapse "//" and "/./" so the explain paths and ours compare equal.
string MCD::normalize(const string& path)
{
  string out;
  for (size_t i = 0; i < path.size(); i++) {
    if (path[i] == '/' && !out.empty() && out[out.size() - 1] == '/')
      continue;
    if (path[i] == '/' && out.size() >= 2 && out.substr(out.size() - 2) == "/.")
      { out.erase(out.size() - 1); continue; }
    out += path[i];
  }
  return out;
}

// Source name for a layer that explain only reports by file (a final row
// with source "cleared").
string MCD::sourceOf(const string& file) const
{
  string f = normalize(file);
  if (f == normalize(p_userFile)) return "user";
  if (f == normalize(p_siteFile)) return "site";
  size_t sl = f.rfind('/');
  if ((sl == string::npos ? f : f.substr(sl + 1)) == "submit.site")
    return "submit.site";
  return "vendor";
}

bool MCD::loadExplain(const string& jsonLines, string& err)
{
  Content fresh;
  const string edited = normalize(editedFile());
  const string other = normalize(p_mode == AdminMode ? p_userFile : "");
  size_t pos = 0;
  int lineNo = 0;
  while (pos < jsonLines.size()) {
    size_t nl = jsonLines.find('\n', pos);
    string text = jsonLines.substr(pos, nl == string::npos ? string::npos
                                                           : nl - pos);
    pos = nl == string::npos ? jsonLines.size() : nl + 1;
    lineNo++;
    if (text.find_first_not_of(" \t\r") == string::npos)
      continue;
    char where[40];
    snprintf(where, sizeof where, "explain line %d: ", lineNo);
    MiniJson row;
    string perr;
    if (!MiniJson::parse(text, row, perr)) {
      err = where + perr;
      return false;
    }
    const MiniJson& k = row.get("key");
    const MiniJson& src = row.get("source");
    const MiniJson& val = row.get("value");
    const MiniJson& file = row.get("file");
    const MiniJson& over = row.get("overridden");
    if (!k.isString() || !src.isString() || !file.isString() ||
        (!val.isNull() && !val.isString()) ||
        (over.type() != MiniJson::Array && !over.isNull())) {
      err = string(where) + "missing key, source, file, value or overridden";
      return false;
    }
    vector<Layer> layers;
    for (size_t i = 0; i < over.items().size(); i++) {
      const MiniJson& o = over.items()[i];
      Layer l;
      l.source = o.get("source").str();
      l.file = o.get("file").str();
      l.hasValue = o.get("value").isString();
      l.value = o.get("value").str();
      layers.push_back(l);
    }
    Layer last;
    last.file = file.str();
    last.hasValue = val.isString();
    last.value = val.str();
    last.source = src.str() == "cleared" ? sourceOf(last.file) : src.str();
    layers.push_back(last);

    KeyState ks;
    ks.name = lower(k.str());
    bool haveEdited = false;
    for (size_t i = 0; i < layers.size(); i++) {
      string f = normalize(layers[i].file);
      if (!other.empty() && f == other)
        continue;                       // admin mode: not a layer
      if (f == edited) {
        haveEdited = true;
        ks.edit = layers[i].hasValue ? Set : Clear;
        ks.value = layers[i].value;
      } else {
        ks.inherited.push_back(layers[i]);
      }
    }
    if (!haveEdited)
      ks.edit = Inherit;
    fresh.config[ks.name] = ks;
  }
  p_cur.config = fresh.config;
  p_base.config = fresh.config;
  return true;
}

void MCD::loadMerged(const map<string,string>& merged)
{
  p_cur.config.clear();
  for (map<string,string>::const_iterator it = merged.begin();
       it != merged.end(); ++it) {
    KeyState ks;
    ks.name = it->first;
    Layer l;
    l.source = "merged";
    l.hasValue = true;
    l.value = it->second;
    ks.inherited.push_back(l);
    p_cur.config[it->first] = ks;
  }
  p_base.config = p_cur.config;
}

void MCD::loadFiles(const vector<string>& keys)
{
  ConfigFile site, edited;
  if (p_mode != AdminMode)
    site.load(p_siteFile);
  edited.load(editedFile());
  p_cur.config.clear();
  for (size_t i = 0; i < keys.size(); i++) {
    KeyState ks;
    ks.name = keys[i];
    string v;
    bool cleared = false;
    // get() is false for a cleared key too; cleared says which.
    if (p_mode != AdminMode &&
        (site.get(keys[i], v, &cleared) || cleared)) {
      Layer l;
      l.source = "site";
      l.file = p_siteFile;
      l.hasValue = !cleared;
      l.value = cleared ? string() : v;
      ks.inherited.push_back(l);
    }
    if (edited.get(keys[i], v, &cleared) || cleared) {
      ks.edit = cleared ? Clear : Set;
      if (!cleared) ks.value = v;
    }
    p_cur.config[lower(keys[i])] = ks;
  }
  p_base.config = p_cur.config;
}

void MCD::setValue(const string& key, const string& value)
{
  if (value.empty()) {
    useInherited(key);
    return;
  }
  KeyState& ks = p_cur.config[lower(key)];
  if (ks.name.empty()) ks.name = key;
  ks.edit = Set;
  ks.value = value;
}

void MCD::useInherited(const string& key)
{
  map<string,KeyState>::iterator it = p_cur.config.find(lower(key));
  if (it != p_cur.config.end()) {
    it->second.edit = Inherit;
    it->second.value.clear();
  }
}

bool MCD::clear(const string& key)
{
  map<string,KeyState>::iterator it = p_cur.config.find(lower(key));
  if (it == p_cur.config.end() || it->second.inherited.empty() ||
      !it->second.inherited.back().hasValue)
    return false;
  it->second.edit = Clear;
  it->second.value.clear();
  return true;
}

vector<string> MCD::keys() const
{
  vector<string> out;
  for (map<string,KeyState>::const_iterator it = p_cur.config.begin();
       it != p_cur.config.end(); ++it)
    out.push_back(it->first);
  return out;
}

const MCD::KeyState* MCD::state(const string& key) const
{
  map<string,KeyState>::const_iterator it = p_cur.config.find(lower(key));
  return it == p_cur.config.end() ? 0 : &it->second;
}

bool MCD::effective(const string& key, string& value, bool cppOnly) const
{
  const KeyState* ks = state(key);
  if (!ks)
    return false;
  if (ks->edit == Set) {
    value = ks->value;
    return true;
  }
  if (ks->edit == Clear)
    return false;
  for (size_t i = ks->inherited.size(); i-- > 0; ) {
    const Layer& l = ks->inherited[i];
    if (cppOnly && (l.source == "submit.site" || l.source == "vendor"))
      continue;
    if (!l.hasValue)
      return false;
    value = l.value;
    return true;
  }
  return false;
}

MCD::Tag MCD::tag(const string& key, bool cppOnly) const
{
  const KeyState* ks = state(key);
  if (!ks)
    return TagDefault;
  if (ks->edit == Set)
    return p_mode == AdminMode ? TagSiteEditing : TagYours;
  if (ks->edit == Clear)
    return TagNoValue;
  const Layer* last = 0;
  for (size_t i = ks->inherited.size(); i-- > 0 && !last; ) {
    const string& s = ks->inherited[i].source;
    if (!(cppOnly && (s == "submit.site" || s == "vendor")))
      last = &ks->inherited[i];
  }
  if (!last)
    return TagDefault;
  const string& s = last->source;
  if (p_mode == RemoteMode && s != "user")
    return TagServer;
  if (s == "site")
    return TagSite;
  if (s == "user")
    return TagYours;
  return TagSiteDefaults;
}

bool MCD::changed(const string& key) const
{
  map<string,KeyState>::const_iterator c = p_cur.config.find(lower(key));
  map<string,KeyState>::const_iterator b = p_base.config.find(lower(key));
  if (c == p_cur.config.end() || b == p_base.config.end())
    return c != p_cur.config.end() || b != p_base.config.end();
  return !(c->second == b->second);
}

void MCD::revert(const string& key)
{
  map<string,KeyState>::const_iterator b = p_base.config.find(lower(key));
  if (b != p_base.config.end())
    p_cur.config[lower(key)] = b->second;
}

void MCD::ensureKey(const string& spelling)
{
  string k = lower(spelling);
  map<string,KeyState>::iterator it = p_cur.config.find(k);
  if (it == p_cur.config.end()) {
    KeyState ks;
    ks.name = spelling;
    p_cur.config[k] = ks;
    p_base.config[k] = ks;
  } else if (it->second.name == k) {
    // explain reports lower case; keep the spelling the readers document
    it->second.name = spelling;
    map<string,KeyState>::iterator b = p_base.config.find(k);
    if (b != p_base.config.end()) b->second.name = spelling;
  }
}

const char* MCD::tagText(Tag t)
{
  switch (t) {
    case TagDefault: return "default";
    case TagSiteDefaults: return "site defaults";
    case TagSite: return "site";
    case TagServer: return "server";
    case TagYours: return "yours";
    case TagSiteEditing: return "site (editing)";
    case TagNoValue: return "no value";
  }
  return "";
}

const vector<string>& MCD::cppKeys()
{
  static vector<string> k;
  if (k.empty()) {
    const char* names[] = { "shell", "sourcefile", "frontendmachine",
        "frontendbypass", "perlpath", "qmgrpath", "libpath", "xappspath",
        "noremoteaccess", "usersubmit", "singleconnect", "checkscratch" };
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++)
      k.push_back(names[i]);
  }
  return k;
}

vector<string> MCD::compareMerged(const map<string,string>& merged,
                                  const vector<string>& keys) const
{
  vector<string> bad;
  for (size_t i = 0; i < keys.size(); i++) {
    string k = lower(keys[i]), mine, theirs;
    bool a = effective(k, mine, true);
    map<string,string>::const_iterator it = merged.find(k);
    bool b = it != merged.end();
    if (b) theirs = it->second;
    if (a != b || (a && mine != theirs))
      bad.push_back(k + ": draft " + (a ? "'" + mine + "'" : string("none")) +
                    ", merged view " + (b ? "'" + theirs + "'" : string("none")));
  }
  return bad;
}

vector<ConfigEdit> MCD::edits() const
{
  vector<ConfigEdit> out;
  KeyState none;
  for (map<string,KeyState>::const_iterator it = p_cur.config.begin();
       it != p_cur.config.end(); ++it) {
    const KeyState& cur = it->second;
    map<string,KeyState>::const_iterator bi = p_base.config.find(it->first);
    const KeyState& base = bi == p_base.config.end() ? none : bi->second;
    if (cur.edit == base.edit && (cur.edit != Set || cur.value == base.value))
      continue;
    ConfigEdit e;
    e.op = cur.edit == Set ? ConfigEdit::Set
         : cur.edit == Clear ? ConfigEdit::Clear : ConfigEdit::Remove;
    e.key = cur.name;
    if (cur.edit == Set) e.value = cur.value;
    out.push_back(e);
  }
  return out;
}

bool MCD::applyTo(ConfigFile& file, string& err) const
{
  vector<ConfigEdit> list = edits();
  for (size_t i = 0; i < list.size(); i++) {
    string why;
    if (!file.apply(list[i], &why)) {
      err = list[i].key + ": " + why;
      return false;
    }
  }
  return true;
}

vector<string> MCD::skeletonKeys(const string& code)
{
  vector<string> k;
  k.push_back(code + "Environment");
  k.push_back(code + "Command");
  return k;
}

void MCD::applySkeletons(ConfigFile& file, const vector<string>& codes) const
{
  for (size_t i = 0; i < codes.size(); i++) {
    string lk = lower(codes[i]);
    map<string,KeyState>::const_iterator ci = p_cur.config.find(lk);
    map<string,KeyState>::const_iterator bi = p_base.config.find(lk);
    if (ci == p_cur.config.end())
      continue;
    if (ci->second.edit == Set)
      file.addSkeleton(codes[i], skeletonKeys(codes[i]));
    else if (bi != p_base.config.end() && bi->second.edit == Set)
      file.removeSkeleton(codes[i], skeletonKeys(codes[i]));
  }
}
