// What Register Machines edits for one machine, without any widgets: the
// CONFIG.<machine> keys (per key the layers below the edited file plus this
// session's edit), dirty tracking against what was loaded, and the step that
// turns the edits into ConfigFile calls.  Fed by GENSUB_EXPLAIN's JSON, so
// the values and their sources are the ones a real job script gets.
#ifndef MACHINECONFIGDRAFT_HH
#define MACHINECONFIGDRAFT_HH

#include <map>
#include <string>
#include <vector>

#include "tdat/ConfigFile.H"

class MachineConfigDraft
{
public:
  // User and remote edit ~/.ECCE/CONFIG.<m>; admin edits the site file and
  // ignores any user layer.  Remote only changes the tags ("server").
  enum Mode { UserMode, AdminMode, RemoteMode };
  enum Edit { Inherit, Set, Clear };
  enum Tag { TagDefault, TagSiteDefaults, TagSite, TagServer, TagYours,
             TagSiteEditing, TagNoValue };

  struct Layer {
    std::string source;   // submit.site, vendor, site or user
    std::string file;
    bool hasValue;        // false: this layer cleared the key
    std::string value;
    bool operator==(const Layer& o) const;
  };

  struct KeyState {
    std::string name;             // spelling used when the key is new
    std::vector<Layer> inherited; // merge order, edited file excluded
    Edit edit;
    std::string value;            // when edit == Set
    KeyState() : edit(Inherit) {}
    bool operator==(const KeyState& o) const;
  };

  struct MachineLine {
    std::string name, host, vendor, model, proc;
    unsigned procs, nodes;
    bool allocationAccount;
    MachineLine() : procs(0), nodes(0), allocationAccount(false) {}
    bool operator==(const MachineLine& o) const;
  };

  struct QueueRow {            // limits as stored: wall in minutes, memory MB
    std::string name;
    unsigned minProcs, maxProcs, defProcs, maxWall, defWall, maxMem, defMem,
             minScratch, defScratch;
    QueueRow() : minProcs(0), maxProcs(0), defProcs(0), maxWall(0),
                 defWall(0), maxMem(0), defMem(0), minScratch(0),
                 defScratch(0) {}
    bool operator==(const QueueRow& o) const;
  };

  struct Content {
    std::map<std::string, KeyState> config;   // lower-case key
    MachineLine line;
    std::string qmgr;
    bool queuesOwned;
    std::vector<QueueRow> queues;
    Content() : queuesOwned(false) {}
    bool operator==(const Content& o) const;
  };

  MachineConfigDraft(Mode mode, const std::string& siteFile,
                     const std::string& userFile);

  // Replace the draft (and its baseline) from explain's one-object-per-line
  // output.  False with err on malformed text.
  bool loadExplain(const std::string& jsonLines, std::string& err);
  // Without provenance (RefMachine::config()): every key inherits one layer
  // named "merged".
  void loadMerged(const std::map<std::string,std::string>& merged);

  // Without gensub: the given keys from the site and edited files alone
  // (the site file is the only inherited layer; admin has none).  For
  // callers that manage a few keys and need no provenance beyond that.
  void loadFiles(const std::vector<std::string>& keys);

  Mode mode() const { return p_mode; }
  // The file this session writes.
  const std::string& editedFile() const;

  // Edits.  setValue("") is useInherited(); clear() needs an inherited
  // layer that sets the key (else it would write a meaningless "key: -").
  void setValue(const std::string& key, const std::string& value);
  void useInherited(const std::string& key);
  bool clear(const std::string& key);

  // Make the key editable when no layer sets it; `spelling` is how a new
  // line is written.  Changes the baseline too, so it is not an edit.
  // Has this key been edited since load/save?  revert() puts back the value
  // as last loaded or saved.
  bool changed(const std::string& key) const;
  void revert(const std::string& key);

  void ensureKey(const std::string& spelling);

  std::vector<std::string> keys() const;
  const KeyState* state(const std::string& key) const;
  // False when no value is in effect (built-in default applies).  cppOnly
  // ignores submit.site and vendor layers, which only gensub reads.
  bool effective(const std::string& key, std::string& value,
                 bool cppOnly = false) const;
  // cppOnly: as for effective(); a layer only gensub reads is not a source.
  Tag tag(const std::string& key, bool cppOnly = false) const;
  static const char* tagText(Tag t);
  // The keys RefMachine's accessors read.
  static const std::vector<std::string>& cppKeys();
  // Keys in `keys` whose cppOnly effective value differs from the merged
  // view; one line each.
  std::vector<std::string> compareMerged(
      const std::map<std::string,std::string>& merged,
      const std::vector<std::string>& keys) const;

  // Edited content, by value; the queue fields are edited through these.
  Content& content() { return p_cur; }
  const Content& content() const { return p_cur; }
  bool isDirty() const { return !(p_cur == p_base); }
  void markSaved() { p_base = p_cur; }
  // Write the changed keys into the file (set, clear, remove), leaving all
  // other lines alone.  Does not save the file.
  bool applyTo(ConfigFile& file, std::string& err) const;
  // The same changes as a list, for a file written elsewhere (a central
  // server's site file).
  std::vector<ConfigEdit> edits() const;

  // After applyTo(): for each code the edited file now gives a program path,
  // add empty Environment/Command blocks that are missing; for each code
  // whose path was in the file and is no longer, remove those blocks where
  // still empty.  Empty blocks change nothing for any reader.
  void applySkeletons(ConfigFile& file,
                      const std::vector<std::string>& codes) const;
  // The keys a skeleton holds, in the spelling gensub documents.
  static std::vector<std::string> skeletonKeys(const std::string& code);

private:
  static std::string normalize(const std::string& path);
  std::string sourceOf(const std::string& file) const;

  Mode p_mode;
  std::string p_siteFile, p_userFile;
  Content p_cur, p_base;
};

#endif
