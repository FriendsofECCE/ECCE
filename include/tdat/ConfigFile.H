// One CONFIG.<machine> file: the line grammar that RefMachine::config() and
// scripts/gensub both read, and an editor that changes single keys while
// keeping every other line (comments, order, hand-written keys and blocks).
//
// Grammar: "key: value", "key value", "key { value }" and multi-line
// "key {" ... "}" (closing brace in column 0).  Keys are case-insensitive;
// the value "-" clears a key set by an earlier file; an empty value is no
// value at all.
#ifndef CONFIGFILE_HH
#define CONFIGFILE_HH

#include <map>
#include <string>
#include <vector>

class ConfigFile
{
public:
  struct Entry {
    std::string key;     // as spelled in the file
    std::string lkey;    // lower case
    std::string value;   // trimmed; block bodies keep inner newlines
    size_t first, last;  // line span, inclusive
    bool block;          // written with braces
  };

  ConfigFile();

  // False if the file cannot be read; a missing file loads as empty and
  // exists() stays false.
  bool load(const std::string& path);
  // Parse text instead of a file (for checks on an unsaved buffer).
  void parse(const std::string& text);

  bool exists() const { return p_exists; }
  const std::string& path() const { return p_path; }
  const std::vector<Entry>& entries() const { return p_entries; }
  // Problems the readers would silently skip or misread: an unclosed block,
  // a key with no value.
  const std::vector<std::string>& warnings() const { return p_warnings; }

  // Apply this file to a merged view the way RefMachine::config() does:
  // "-" erases the key, an empty value is ignored, otherwise it replaces.
  void mergeInto(std::map<std::string,std::string>& out) const;
  // Same, from a file on disk (a missing file adds nothing).
  static void mergeFile(const std::string& path,
                        std::map<std::string,std::string>& out);

  // The value this file alone gives a key (last non-empty occurrence);
  // cleared is set when that value is "-".
  bool get(const std::string& key, std::string& value,
           bool* cleared = 0) const;

  // Writers.  set() replaces the first occurrence in place (keeping the
  // key's spelling and position), drops later duplicates, and puts a new
  // key after the leading comment block.  It refuses ("" and "-" are for
  // remove()/clear(); a block line with "}" in column 0 cannot be written)
  // and returns false with err set.  Single-line values containing "{" are
  // written as blocks because the readers would split them as one.
  bool set(const std::string& key, const std::string& value,
           std::string* err = 0);
  void remove(const std::string& key);
  // "key: -", or "key { - }" where the key is already a block.
  void clear(const std::string& key);

  bool modified() const { return p_modified; }
  std::string text() const;
  // Atomic replace (temporary file, fsync, rename); keeps the mode and
  // always leaves the owner able to write.  Deletes the file when only
  // comments and blank lines remain after an edit.
  bool save(std::string* err = 0);

private:
  void index();

  std::string p_path;
  bool p_exists;
  bool p_modified;
  bool p_finalNewline;
  std::vector<std::string> p_lines;
  std::vector<Entry> p_entries;
  std::vector<std::string> p_warnings;
};

#endif
